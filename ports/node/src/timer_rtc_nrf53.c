/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Timer driving the stack's slot schedule, on a compare channel of the nRF5340 network core's RTC1
 */

#include "timer_counter.h"

#include <embenet_port/timer.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/drivers/timer/nrf_rtc_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>
#include <stdint.h>

LOG_MODULE_REGISTER(embenet_timer, CONFIG_EMBENET_LOG_LEVEL);

/*
 * The nRF5340 network core has two RTC instances and neither is free: RTC1 is Zephyr's system clock and RTC0 is MPSL's.
 * So, as on the nRF54L with its GRTC, this backend owns a compare channel of the system clock's RTC rather than an
 * instance, through the driver's user channel API:
 *
 *   - z_nrf_rtc_timer_read() extends the 24-bit counter to 64 bits, so there is no overflow to track here and no window
 *     in which a read races the overflow flag.
 *   - z_nrf_rtc_timer_set() raises the interrupt by itself for a target already in the past or too close to arm, so
 *     there is no software interrupt to forge.
 *   - The compare runs from the RTC1 interrupt at the priority its devicetree node carries, not from a line this port
 *     sets to 5 itself as on the nRF52 - the same arrangement as the GRTC on the nRF54L.
 *
 * The RTC ticks at 32768 Hz, so a tick is 1000000 / 32768 = 15625 / 512 us exactly, and the conversions below are exact
 * integer arithmetic rather than the scaled approximation of timer_rtc_nrf52.c.
 */

BUILD_ASSERT(CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT >= 1,
             "the embeNET timer needs an RTC1 compare channel: set CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT=1");

// TSCH keeps slots aligned against the low-frequency clock, so it has to be the crystal: the RC oscillator is off by a few
// hundred ppm, which is slot-width drift within a minute.
BUILD_ASSERT(IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL), "embeNET needs a crystal-backed LFCLK");

#define US_PER_TICK_NUM 15625U
#define US_PER_TICK_DEN 512U

// What the stack may ask for, kept below the driver's limit of NRF_RTC_TIMER_MAX_SCHEDULE_SPAN ticks (256 s) - the same
// value the nRF52 backend gives, for the same 24-bit RTC.
#define MAX_COMPARE_DURATION_US (uint32_t)(250000000)
BUILD_ASSERT(((uint64_t)MAX_COMPARE_DURATION_US * US_PER_TICK_DEN / US_PER_TICK_NUM) < NRF_RTC_TIMER_MAX_SCHEDULE_SPAN,
             "a compare the stack may ask for would not fit in the RTC timer's schedule span");

static void (*callback)(void *context);
static void *callbackContext;

// Negative until the first EMBENET_TIMER_Init(). Held for the lifetime of the image rather than returned in Deinit(), as
// on the nRF54L: the node restarts, and a channel released and reclaimed is one some other subsystem can take in between.
static int32_t compareChannel = -1;

// So that a refused compare reports itself once instead of once per slot. SetCompare() runs in the slot ISR, where a log
// flood is its own failure.
static bool compareFailureLogged;

// RTC1 ticks when the timer was initialised, subtracted from every reading, so the stack's clock starts at zero on every
// Init() - as with the nRF52 backend, which clears its own RTC. RTC1 is the system clock and cannot be cleared.
static uint64_t counterOrigin;

static inline uint64_t ticksToUs(uint64_t ticks) { return ticks * US_PER_TICK_NUM / US_PER_TICK_DEN; }

static inline uint64_t usToTicks(uint64_t us) { return us * US_PER_TICK_DEN / US_PER_TICK_NUM; }

static void onCompare(int32_t channel, uint64_t expireTime, void *context) {
  ARG_UNUSED(channel);
  ARG_UNUSED(expireTime);
  ARG_UNUSED(context);

  // One-shot: the driver disarms the channel before calling here, so nothing has to before the stack arms it again.
  if(callback != NULL) {
    callback(callbackContext);
  }
  k_sem_give(EMBENET_PORT_COMMON_GetProcSemaphore());
}

void EMBENET_TIMER_Init(void (*compareCallback)(void *context), void *context) {
  EMBENET_TIMER_Deinit();

  if(compareChannel < 0) {
    compareChannel = z_nrf_rtc_timer_chan_alloc();
    if(compareChannel < 0) {
      // Every user channel is taken. The stack has no way to be told, so this is the only place it is visible.
      LOG_ERR("no RTC1 compare channel available: the slot schedule will not run");
      return;
    }
    LOG_DBG("RTC1 compare channel %d", compareChannel);
  }

  // Before the callback is armed, so the first compare is already measured against this origin.
  counterOrigin = z_nrf_rtc_timer_read();

  compareFailureLogged = false;
  callback = compareCallback;
  callbackContext = context;
}

void EMBENET_TIMER_Deinit(void) {
  // EMBENET_NODE_Deinit() lands here, so the interrupt source has to go quiet before the callback does - an armed compare
  // would otherwise re-enter the stack after it has been torn down.
  if(compareChannel >= 0) {
    z_nrf_rtc_timer_abort(compareChannel);
  }
  callback = NULL;
  callbackContext = NULL;
}

void EMBENET_TIMER_SetCompare(uint32_t compareValue) {
  if(compareChannel < 0) {
    return;
  }

  // The stack works in 32-bit microseconds and the RTC in 64-bit ticks, so the distance is what carries over, not the
  // value. Taking it as signed splits future from past; anything not ahead within the span is due now.
  uint64_t const raw = z_nrf_rtc_timer_read();
  uint64_t const nowUs = ticksToUs(raw - counterOrigin);
  int32_t const delta = (int32_t)(compareValue - (uint32_t)nowUs);

  // Converted back from the absolute target, not from the distance, so the rounding of each conversion does not add up.
  // A target at or behind the counter makes the driver raise the interrupt at once, which is what the interface asks for
  // when the compare is in the past.
  uint64_t const target = ((delta > 0) && ((uint32_t)delta < MAX_COMPARE_DURATION_US)) ? (counterOrigin + usToTicks(nowUs + (uint64_t)delta)) : raw;

  int const err = z_nrf_rtc_timer_set(compareChannel, target, onCompare, NULL);
  if(err < 0) {
    if(!compareFailureLogged) {
      compareFailureLogged = true;
      LOG_ERR("RTC1 compare channel %d refused a compare: %d", compareChannel, err);
    }
  } else {
    compareFailureLogged = false;
  }
}

uint32_t EMBENET_TIMER_ReadCounter(void) {
  // No critical section: the driver's read is consistent by itself. Truncating to 32 bits is the wrap the interface
  // expects.
  return (uint32_t)ticksToUs(z_nrf_rtc_timer_read() - counterOrigin);
}

uint32_t EMBENET_TIMER_GetMaxCompareDurationUs(void) { return MAX_COMPARE_DURATION_US; }

uint64_t embenet_port_timer_to_counter(uint32_t time_us) {
  // To the time base of the multiprotocol adapter: microseconds of the same RTC1 ticks, counted from zero rather than from
  // counterOrigin. The distance from now carries over exactly - they are the same counter - however long ago the stack
  // read its time.
  uint64_t const raw = z_nrf_rtc_timer_read();
  return ticksToUs(raw) + (uint64_t)(int64_t)(int32_t)(time_us - (uint32_t)ticksToUs(raw - counterOrigin));
}
