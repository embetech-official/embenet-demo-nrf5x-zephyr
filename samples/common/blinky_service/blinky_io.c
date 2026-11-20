/**
 * @file
 * @brief Blinky service I/O - see blinky_io.h.
 *
 * Every button edge restarts that button's debounce timer; when it expires, the timer samples the settled level. A
 * settled release arms the button, a settled press of an armed button latches it and disarms it. One spinlock guards
 * the initialized flag, so an LED write racing blinky_io_deinit() never drives a released pin.
 */

#include "blinky_io.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <errno.h>

LOG_MODULE_REGISTER(blinky_io, LOG_LEVEL_INF);

/// How long a button's level must stay unchanged to count, in milliseconds.
#define DEBOUNCE_MS 20

static struct gpio_dt_spec const leds[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};
static struct gpio_dt_spec const buttons[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios),
};
BUILD_ASSERT(ARRAY_SIZE(leds) == BLINKY_IO_COUNT && ARRAY_SIZE(buttons) == BLINKY_IO_COUNT, "4 LEDs and 4 buttons expected");

/// One per button: the buttons may sit on different GPIO ports (they do on the nRF54L15 DK).
static struct gpio_callback button_cbs[BLINKY_IO_COUNT];
static struct k_timer debounce_timers[BLINKY_IO_COUNT];
/// Bit n-1 set while button n has been released since its last latched press.
static atomic_t armed;
/// Bit n-1 set when button n was pressed and not yet taken.
static atomic_t latched;
static blinky_io_wakeup_t wakeup_fn;

/// Guards @ref initialized against the LED writes.
static struct k_spinlock lock;
static bool initialized;

static void on_debounced(struct k_timer *timer) {
  size_t const i = ARRAY_INDEX(debounce_timers, timer);
  int const level = gpio_pin_get_dt(&buttons[i]);

  if(0 == level) {
    atomic_set_bit(&armed, i);
  } else if((1 == level) && atomic_test_and_clear_bit(&armed, i)) {
    atomic_set_bit(&latched, i);
    if(NULL != wakeup_fn) {
      wakeup_fn();
    }
  }
}

static void on_button_edge(struct device const *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
  ARG_UNUSED(port);
  ARG_UNUSED(pins);
  k_timer_start(&debounce_timers[ARRAY_INDEX(button_cbs, cb)], K_MSEC(DEBOUNCE_MS), K_NO_WAIT);
}

/// Undoes whatever blinky_io_init() got to; every call tolerates a pin that was never configured.
static void release_pins(void) {
  for(size_t i = 0; i < BLINKY_IO_COUNT; ++i) {
    (void)gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_DISABLE);
    (void)gpio_remove_callback_dt(&buttons[i], &button_cbs[i]);
    k_timer_stop(&debounce_timers[i]);
    (void)gpio_pin_configure_dt(&buttons[i], GPIO_DISCONNECTED);
    (void)gpio_pin_set_dt(&leds[i], 0);
    (void)gpio_pin_configure_dt(&leds[i], GPIO_DISCONNECTED);
  }
}

int blinky_io_init(blinky_io_wakeup_t wakeup) {
  bool already = false;
  K_SPINLOCK(&lock) { already = initialized; }
  if(already) {
    return -EALREADY;
  }

  for(size_t i = 0; i < BLINKY_IO_COUNT; ++i) {
    if(!gpio_is_ready_dt(&leds[i]) || !gpio_is_ready_dt(&buttons[i])) {
      LOG_ERR("GPIO controller of LED/button %zu not ready", i + 1U);
      return -ENODEV;
    }
  }

  wakeup_fn = wakeup;
  (void)atomic_clear(&armed);
  (void)atomic_clear(&latched);

  int err = 0;
  for(size_t i = 0; (0 == err) && (i < BLINKY_IO_COUNT); ++i) {
    k_timer_init(&debounce_timers[i], on_debounced, NULL);
    gpio_init_callback(&button_cbs[i], on_button_edge, BIT(buttons[i].pin));

    err = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
    if(0 == err) {
      err = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
    }
    if(0 == err) {
      err = gpio_add_callback_dt(&buttons[i], &button_cbs[i]);
    }
    if(0 == err) {
      err = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_BOTH);
    }
    if(0 != err) {
      LOG_ERR("LED/button %zu setup failed: %d", i + 1U, err);
    } else {
      // Samples the starting level: a button already released is armed, one held down waits for its release.
      k_timer_start(&debounce_timers[i], K_MSEC(DEBOUNCE_MS), K_NO_WAIT);
    }
  }
  if(0 != err) {
    release_pins();
    return err;
  }

  K_SPINLOCK(&lock) { initialized = true; }
  return 0;
}

void blinky_io_deinit(void) {
  bool was = false;
  K_SPINLOCK(&lock) {
    was = initialized;
    initialized = false;
  }
  if(!was) {
    return;
  }
  release_pins();
  (void)atomic_clear(&armed);
  (void)atomic_clear(&latched);
}

bool blinky_io_button_pressed(uint8_t button) {
  if((button < 1U) || (button > BLINKY_IO_COUNT)) {
    return false;
  }
  return atomic_test_and_clear_bit(&latched, button - 1U);
}

void blinky_io_led_set(uint8_t led, bool on) {
  if((led < 1U) || (led > BLINKY_IO_COUNT)) {
    return;
  }
  K_SPINLOCK(&lock) {
    if(initialized) {
      (void)gpio_pin_set_dt(&leds[led - 1U], on);
    }
  }
}
