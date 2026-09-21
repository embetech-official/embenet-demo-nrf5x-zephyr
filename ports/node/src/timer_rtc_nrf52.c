/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Timer driving the stack's slot schedule, on the nRF52 RTC peripheral
 */

#include <embenet_port/critical_section.h>
#include <embenet_port/timer.h>
#include <embenet_port_common/embenet_port_common.h>
#include <hal/nrf_clock.h>
#include <hal/nrf_rtc.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>

#if !defined(NRF_RTC2)
#error "this backend expects the nRF52-style RTC peripheral - see timer_grtc_nrf54l.c"
#endif

// Zephyr's system clock takes RTC1, the SoftDevice would take RTC0, so RTC2 is the one left for an
// application to own outright - counter, compare channels and NVIC line alike.
#define EMBENET_PORT_TIMER NRF_RTC2
#define EMBENET_PORT_TIMER_IRQn RTC2_IRQn
#define EMBENET_PORT_TIMER_IRQ_HANDLER RTC2_IRQHandler

#define GUARD_US 70
// scaling factor for the purpose of reducing integer calculating errors
#define SCALE_FACTOR (uint64_t)(1000000000)
// scaled up period by @ref SCALE_FACTOR
#define PERIOD_SCALED_UP (uint64_t)(30517578125)
#define TIMER_MAX_VALUE (uint32_t)0xFFFFFF
#define MAX_COMPARE_DURATION_TICKS 0x7FFFFF
// max value reduced by MSB of used timer expressed in us (approximation)
#define MAX_COMPARE_DURATION_US (EMBENET_TimeUs)(250000000)
#define OVERFLOW_EXTENSION_US (uint64_t)(512000000)

static EMBENET_TIMER_CompareCallback callback;
static void *callbackContext;
static uint64_t volatile timerValueExtension;
static bool volatile softwareIrq;

ISR_DIRECT_DECLARE(rtc_timer_isr) {
  if(nrf_rtc_event_check(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW)) {
    nrf_rtc_event_clear(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW);
    timerValueExtension += (uint64_t)OVERFLOW_EXTENSION_US;
  }
  if(nrf_rtc_event_check(EMBENET_PORT_TIMER, NRF_RTC_EVENT_COMPARE_0) || softwareIrq) {
    nrf_rtc_event_clear(EMBENET_PORT_TIMER, NRF_RTC_EVENT_COMPARE_0);
    nrf_rtc_int_disable(EMBENET_PORT_TIMER, NRF_RTC_INT_COMPARE0_MASK);
    softwareIrq = false;

    if(callback != NULL) {
      callback(callbackContext);
    }
  }
  k_sem_give(EMBENET_PORT_COMMON_GetProcSemaphore());
  ISR_DIRECT_PM(); // done after do_stuff() due to latency concerns
  return 1;
}

void EMBENET_TIMER_Init(EMBENET_TIMER_CompareCallback compareCallback, void *context) {
  // if LFCLK is not running, configure it
  if(!nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, NULL)) {
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
    while(!nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, NULL)) {
      // wait for LFCLK to start
    }
  }

  // nrf LF clock must be running
  __ASSERT(nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, NULL), "LFCLK must be running");

  // configure timer
  EMBENET_TIMER_Deinit();
  nrf_rtc_prescaler_set(EMBENET_PORT_TIMER, 0);
  nrf_rtc_event_clear(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW);
  nrf_rtc_event_enable(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW);
  nrf_rtc_int_enable(EMBENET_PORT_TIMER, NRF_RTC_INT_OVERFLOW_MASK);
  nrf_rtc_event_clear(EMBENET_PORT_TIMER, NRF_RTC_EVENT_COMPARE_0);
  nrf_rtc_event_enable(EMBENET_PORT_TIMER, NRF_RTC_EVENT_COMPARE_0);
  nrf_rtc_task_trigger(EMBENET_PORT_TIMER, NRF_RTC_TASK_START);

  static bool isr_enabled = false;
  if(false == isr_enabled) {
    irq_disable(EMBENET_PORT_TIMER_IRQn);
    IRQ_DIRECT_CONNECT(EMBENET_PORT_TIMER_IRQn, 5, rtc_timer_isr, 0);
    irq_enable(EMBENET_PORT_TIMER_IRQn);
    isr_enabled = true;
  }
  irq_enable(EMBENET_PORT_TIMER_IRQn);

  callback = compareCallback;
  callbackContext = context;

  timerValueExtension = 0;
  softwareIrq = false;
}

void EMBENET_TIMER_Deinit(void) {
  // EMBENET_NODE_Deinit() lands here, so the interrupt source has to go quiet before the callback
  // does. Leaving either the overflow interrupt or the NVIC line armed lets a late RTC2 interrupt
  // re-enter the stack after it has been torn down.
  nrf_rtc_int_disable(EMBENET_PORT_TIMER, NRF_RTC_INT_COMPARE0_MASK | NRF_RTC_INT_OVERFLOW_MASK);
  nrf_rtc_event_disable(EMBENET_PORT_TIMER, NRF_RTC_EVENT_COMPARE_0);
  nrf_rtc_task_trigger(EMBENET_PORT_TIMER, NRF_RTC_TASK_STOP);
  nrf_rtc_task_trigger(EMBENET_PORT_TIMER, NRF_RTC_TASK_CLEAR);
  nrf_rtc_event_clear(EMBENET_PORT_TIMER, NRF_RTC_EVENT_COMPARE_0);
  nrf_rtc_event_clear(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW);

  irq_disable(EMBENET_PORT_TIMER_IRQn);
  NVIC_ClearPendingIRQ(EMBENET_PORT_TIMER_IRQn);

  // softwareIrq alone is enough to make the ISR invoke the callback, so it has to be cleared too.
  softwareIrq = false;
  callback = NULL;
  callbackContext = NULL;
}

void EMBENET_TIMER_SetCompare(EMBENET_TimeUs compareValue) {
  EMBENET_CRITICAL_SECTION_Enter();
  // make sure that the interrupt is active (first compare)
  nrf_rtc_int_enable(EMBENET_PORT_TIMER, NRF_RTC_INT_COMPARE0_MASK);

  // get current time and counter
  uint32_t counter = nrf_rtc_counter_get(EMBENET_PORT_TIMER);
  EMBENET_TimeUs now = (EMBENET_TimeUs)((uint64_t)counter * PERIOD_SCALED_UP / SCALE_FACTOR + timerValueExtension);
  if(nrf_rtc_event_check(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW)) {
    counter = nrf_rtc_counter_get(EMBENET_PORT_TIMER);
    now = (EMBENET_TimeUs)((uint64_t)counter * PERIOD_SCALED_UP / SCALE_FACTOR + timerValueExtension + OVERFLOW_EXTENSION_US);
  }

  EMBENET_TimeUs delta = compareValue - now;
  if((EMBENET_TimeUs)(delta - GUARD_US) < MAX_COMPARE_DURATION_US) {
    // arm the compare interrupt
    nrf_rtc_cc_set(EMBENET_PORT_TIMER, 0, (counter + (uint32_t)((uint64_t)(delta)*SCALE_FACTOR / PERIOD_SCALED_UP)) & TIMER_MAX_VALUE);
  } else {
    // compareValue is too close to current value and timer will be late, interrupt is triggered immediately
    NVIC_SetPendingIRQ(EMBENET_PORT_TIMER_IRQn);
    softwareIrq = true;
  }

  EMBENET_CRITICAL_SECTION_Exit();
}

EMBENET_TimeUs EMBENET_TIMER_ReadCounter(void) {
  // note that during assembly of the actual time, the counter may increment and overflow itself
  EMBENET_CRITICAL_SECTION_Enter();
  // this thing is sophisticated
  // it can be called from non-priviledged mode (1) and from @ref EMBENET_PORT_TIMER_IRQ_HANDLER ISR (2)
  // (1A) after entering the critical section and before getting the timer value it can increment and overflow,
  // OVF flag will be set and timerValueExtension will be not incremented - OVF FLAG MUST BE CHECKED!
  // (1B) during code execution inside the critical section timer may increment and overflow itself - no harm
  // (2) before entering critical section timerValueExtension may not be incremented and OVF flag will be set - OVF FLAG MUST BE CHECKED!
  uint64_t now = (uint64_t)nrf_rtc_counter_get(EMBENET_PORT_TIMER) * PERIOD_SCALED_UP / SCALE_FACTOR + timerValueExtension;
  if(nrf_rtc_event_check(EMBENET_PORT_TIMER, NRF_RTC_EVENT_OVERFLOW)) {
    now = (uint64_t)nrf_rtc_counter_get(EMBENET_PORT_TIMER) * PERIOD_SCALED_UP / SCALE_FACTOR + timerValueExtension + OVERFLOW_EXTENSION_US;
  }
  EMBENET_CRITICAL_SECTION_Exit();

  return (EMBENET_TimeUs)(now);
}

EMBENET_TimeUs EMBENET_TIMER_GetMaxCompareDuration(void) { return (EMBENET_TimeUs)MAX_COMPARE_DURATION_US; }
