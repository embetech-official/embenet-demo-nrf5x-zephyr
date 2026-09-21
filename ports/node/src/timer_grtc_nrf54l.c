/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Timer driving the stack's slot schedule, on the nRF54L GRTC peripheral
 */

#include <embenet_port/timer.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>
#include <stdint.h>

LOG_MODULE_REGISTER(embenet_timer, CONFIG_EMBENET_LOG_LEVEL);

/*
 * The nRF54L has no RTC peripheral at all - the GRTC replaces it - so this backend is not a port of
 * timer_rtc_nrf52.c so much as the same interface over hardware that already does most of the work:
 *
 *   - SYSCOUNTER counts microseconds, which is what EMBENET_TimeUs is, so nothing is scaled.
 *   - It is 52 bits wide, i.e. 142 years, so there is no overflow to extend and no window in which
 *     a read races the overflow flag.
 *   - "The EVENTS_COMPARE[n] event is generated immediately if the configured compare value at
 *     CC[n] is less than the current SYSCOUNTER value" (datasheet 8.10.2), so a compare that has
 *     already passed fires by itself and there is no software interrupt to forge.
 *
 * What it does not give us is a peripheral of our own. The GRTC is the system timer and Zephyr's
 * kernel clock is one of its twelve compare channels, so this backend owns a channel rather than an
 * instance, and the compare runs from the shared GRTC interrupt at the priority its devicetree node
 * carries - not, as on the nRF52, from a line this port sets to 5 itself.
 */

// Half the range of EMBENET_TimeUs, which is where SetCompare() splits future from past. The GRTC
// imposes no limit of its own, so this is the full span the 32-bit interface can express.
#define MAX_COMPARE_DURATION_US (EMBENET_TimeUs)0x7FFFFFFF

/*
 * TSCH keeps slots aligned against the low-frequency clock, so it has to be the crystal: the RC
 * oscillator is off by a few hundred ppm, which is slot-width drift within a minute. Zephyr picks
 * the source for us and cannot be overridden at runtime - CLKCFG.CLKSEL is latched when the GRTC
 * starts - so the check belongs at build time. Neither symbol is defined when another core owns the
 * SYSCOUNTER, and then there is nothing here to check.
 */
#if defined(CONFIG_NRF_GRTC_TIMER_SOURCE_LFLPRC)
#error "embeNET needs a crystal-backed LFCLK: CONFIG_NRF_GRTC_TIMER_SOURCE_LFLPRC is the internal RC oscillator"
#elif defined(CONFIG_NRF_GRTC_TIMER_SOURCE_SYSTEM_LFCLK)
BUILD_ASSERT(IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL),
             "embeNET needs a crystal-backed LFCLK: the system LFCLK is running on the internal RC oscillator");
#endif

static EMBENET_TIMER_CompareCallback callback;
static void *callbackContext;

// Negative until the first EMBENET_TIMER_Init(). Held for the lifetime of the image rather than
// returned in Deinit(): the node restarts (see the border router's reset request), and a channel
// released and reclaimed is a channel that some other subsystem can take in between.
static int32_t compareChannel = -1;

// So that a wedged SYSCOUNTER reports itself once instead of once per slot. SetCompare() runs in
// the slot ISR, where a log flood is its own failure.
static bool compareFailureLogged;

/*
 * SYSCOUNTER value when the timer was initialised, subtracted from every reading.
 *
 * The nRF52 backend clears its RTC in Init(), so the stack's clock always starts at zero. The GRTC
 * is the system timer and cannot be cleared - Zephyr's kernel clock is counting on it, and it is
 * not even reset by a soft reset, so it simply keeps running for as long as the board has power.
 * Without this offset the stack would come up at whatever microsecond the board happened to be at,
 * which is a different starting point on every restart and drifts towards the 32-bit wrap as the
 * board stays up. Subtracting the origin makes the two backends agree: zero at Init(), counting up.
 */
static uint64_t counterOrigin;

static void onCompare(int32_t channel, uint64_t expireTime, void *context) {
  ARG_UNUSED(channel);
  ARG_UNUSED(expireTime);
  ARG_UNUSED(context);

  // One-shot, as on the nRF52: CC[n].CCEN.ACTIVE clears itself on the compare event, so nothing has
  // to disarm the channel before the stack arms it again.
  if(callback != NULL) {
    callback(callbackContext);
  }
  k_sem_give(EMBENET_PORT_COMMON_GetProcSemaphore());
}

void EMBENET_TIMER_Init(EMBENET_TIMER_CompareCallback compareCallback, void *context) {
  EMBENET_TIMER_Deinit();

  if(compareChannel < 0) {
    compareChannel = z_nrf_grtc_timer_chan_alloc();
    if(compareChannel < 0) {
      // Every channel the devicetree left to this core is taken. The stack has no way to be told,
      // so this is the only place it is visible.
      LOG_ERR("no GRTC compare channel available: the slot schedule will not run");
      return;
    }
    LOG_DBG("GRTC compare channel %d", compareChannel);
  }

  // Before the callback is armed, so the first compare is already measured against this origin.
  counterOrigin = z_nrf_grtc_timer_read();

  compareFailureLogged = false;
  callback = compareCallback;
  callbackContext = context;
}

void EMBENET_TIMER_Deinit(void) {
  // EMBENET_NODE_Deinit() lands here, so the interrupt source has to go quiet before the callback
  // does - an armed compare would otherwise re-enter the stack after it has been torn down.
  if(compareChannel >= 0) {
    z_nrf_grtc_timer_abort(compareChannel);
  }
  callback = NULL;
  callbackContext = NULL;
}

void EMBENET_TIMER_SetCompare(EMBENET_TimeUs compareValue) {
  if(compareChannel < 0) {
    return;
  }

  // The stack works in 32 bits and the SYSCOUNTER in 52, so the distance is what carries over, not
  // the value. Taking the difference as signed is what splits future from past, and it splits it at
  // exactly MAX_COMPARE_DURATION_US, so the two cannot disagree.
  // Compared in the stack's time base, armed in the SYSCOUNTER's: the difference between the two is
  // a constant, so the distance is the same either way and only the target has to be converted back.
  uint64_t const raw = z_nrf_grtc_timer_read();
  int32_t const delta = (int32_t)(compareValue - (EMBENET_TimeUs)(raw - counterOrigin));

  // A target at or behind the counter makes the hardware raise EVENTS_COMPARE immediately, which is
  // what the interface asks for when the compare is in the past. Clamping at now rather than
  // subtracting keeps the 52-bit value from wrapping below zero in the first microseconds after a
  // cold boot, when the counter itself is still small.
  uint64_t const target = (delta > 0) ? (raw + (uint64_t)delta) : raw;

  int const err = z_nrf_grtc_timer_set(compareChannel, target, onCompare, NULL);
  if(err < 0) {
    if(!compareFailureLogged) {
      compareFailureLogged = true;
      LOG_ERR("GRTC compare channel %d refused a compare: %d", compareChannel, err);
    }
  } else {
    compareFailureLogged = false;
  }
}

EMBENET_TimeUs EMBENET_TIMER_ReadCounter(void) {
  // No critical section: the read retries internally until SYSCOUNTERH reports the counter settled,
  // and the 52-bit value is consistent by the time it returns. Truncating to 32 bits is the wrap
  // the interface expects.
  return (EMBENET_TimeUs)(z_nrf_grtc_timer_read() - counterOrigin);
}

EMBENET_TimeUs EMBENET_TIMER_GetMaxCompareDuration(void) { return MAX_COMPARE_DURATION_US; }
