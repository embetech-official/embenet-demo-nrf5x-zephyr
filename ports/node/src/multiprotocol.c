/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Multiprotocol adapter backed by MPSL timeslots booked for a given time
 *
 * Implements include_private/multiprotocol.h.
 *
 * MPSL places a NORMAL timeslot at a distance from the start of the previous timeslot of the same
 * session, on the same LFCLK crystal as the system timer this adapter tells time by - the GRTC on the
 * nRF54L, RTC1 on the nRF52. So every timeslot that starts becomes the
 * anchor the next booking is placed from:
 *
 *     distance = booked start - start of the last timeslot that actually started
 *
 * That start is the one MPSL scheduled, which for a booking is the time it was booked for - measured
 * on hardware: a timeslot asked for less than about 2.2 ms ahead starts some 25 us late, and the next
 * one, placed from that measured start, as much early. The EARLIEST timeslot taken for a first anchor
 * has no booked time, so its start is measured:
 *
 *     start = system timer now - TIMER0 now        (TIMER0 is started from 0 at the start of the timeslot)
 *
 * TIMER0 starts later against MPSL's schedule in a booked timeslot than in an EARLIEST one - by 17 us,
 * measured - so the first booked timeslot after it is measured too, which carries that difference
 * into the chain once and for all. From there on, booked starts.
 *
 * A blocked timeslot never started, so it never moves the anchor. Where the adapter happens to run
 * when it hands a booking over does not enter the distance at all - only whether it is in time.
 *
 * One booking at a time, handed over in one of two ways:
 *
 *     reserve(t) -> timer at t - SUBMIT_LEAD -> thread: mpsl_timeslot_request(NORMAL)
 *     reserve(t) -> end of the running timeslot -> ACTION_REQUEST(NORMAL)     (back to back)
 *
 *                                                   SIGNAL_START  -> on_granted(t)
 *                                                   SIGNAL_TIMER0 -> on_revoked(), END or the next one
 *                                                   BLOCKED       -> on_denied(t)
 *
 * The first anchor comes from a short EARLIEST timeslot taken at init for nothing else, and so does
 * a fresh one if the last is ever too old for a distance MPSL accepts.
 *
 * @note Everything in the MPSL signal callback runs in a zero latency interrupt, except BLOCKED,
 *       CANCELLED, SESSION_IDLE and SESSION_CLOSED, which come from MPSL's work queue. In the zero
 *       latency context kernel calls and logging are not allowed, which is why it only writes to
 *       variables and why unexpected signals are counted rather than reported on the spot. The
 *       atomics are LDREX/STREX, which a zero latency interrupt does not get in the way of; and since
 *       nothing preempts it, what it reads it reads whole.
 */

#include "multiprotocol.h"

#include <hal/nrf_timer.h>
#include <mpsl_timeslot.h>

#if defined(CONFIG_NRF_GRTC_TIMER)
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#else
#include <zephyr/drivers/timer/nrf_rtc_timer.h>
#endif
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

LOG_MODULE_REGISTER(node_multiprotocol, CONFIG_EMBENET_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MPSL_TIMESLOT_SESSION_COUNT > 0,
             "the adapter books the radio through an MPSL timeslot session: set CONFIG_MPSL_TIMESLOT_SESSION_COUNT=1");
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_GRTC_TIMER) || IS_ENABLED(CONFIG_NRF_RTC_TIMER),
             "the adapter tells time by the system timer, which must run off the LFCLK MPSL schedules by: the GRTC or the RTC");
BUILD_ASSERT(EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US >= EMBENET_MULTIPROTOCOL_MIN_LEAD_US, "a booking would be handed over too late every time");

#define END_GUARD_US EMBENET_MULTIPROTOCOL_END_GUARD_US

/**
 * What MPSL is asked to do about the crystal for every timeslot.
 *
 * NO_GUARANTEE: MPSL does not start it. Whoever drives the transceiver starts it when it needs it, so
 * a timeslot the radio is not used in - or one that is blocked - costs no crystal start-up. Until
 * someone does, HFCLK, and TIMER0 with it, may run on the internal RC oscillator.
 *
 * The radio ports do not start the crystal themselves yet, so NO_GUARANTEE only where MPSL has been
 * measured to start it for every timeslot anyway - the nRF54L. Elsewhere XTAL_GUARANTEED, which the
 * transceiver cannot do without.
 *
 * Overridable so that the two policies can be benchmarked against each other from one source.
 */
#ifndef EMBENET_MULTIPROTOCOL_HFCLK_CFG
#if defined(CONFIG_SOC_SERIES_NRF54L)
#define EMBENET_MULTIPROTOCOL_HFCLK_CFG MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE
#else
#define EMBENET_MULTIPROTOCOL_HFCLK_CFG MPSL_TIMESLOT_HFCLK_CFG_XTAL_GUARANTEED
#endif
#endif

/**
 * Longest stretch of radio time asked for in one go.
 *
 * MPSL grants a timeslot only if the whole of it is free, so one long request loses to anything the
 * other protocol has scheduled anywhere inside it. Asking in chunks and extending means only the first
 * chunk has to fit for the timeslot to start at all, at the cost of one signal per chunk.
 */
#define CHUNK_US 5500U

/**
 * How often a granted timeslot looks for a release, and so the longest the other protocol waits for
 * time the client has given back. Each look is one short zero latency interrupt.
 */
#define RELEASE_POLL_US 1000U

/// The timeslot taken only to measure an anchor: as short as MPSL allows, and placed whenever MPSL
/// finds room within the timeout.
#define ANCHOR_LENGTH_US MPSL_TIMESLOT_LENGTH_MIN_US
#define ANCHOR_TIMEOUT_US 1000000U

/// How often the thread wakes up with nothing to do, only to report MPSL trouble.
#define IDLE_REPORT_PERIOD K_SECONDS(5)

/// The timeslot handed to MPSL, from the request until it ends or is blocked. Changed by CAS from
/// every context.
enum {
  SLOT_NONE = 0,  ///< no request in flight, or the client released it
  SLOT_ANCHORING, ///< an EARLIEST timeslot wanted only for its start
  SLOT_SUBMITTED, ///< a booking handed to MPSL, not started yet
  SLOT_GRANTED,   ///< started; the client is driving the transceiver
};

/// The booking not yet handed to MPSL.
enum {
  BOOKING_NONE = 0,
  BOOKING_BUSY,    ///< being written by reserve() or taken - fields not to be trusted
  BOOKING_WAITING, ///< fields valid, waiting for its turn
};

static atomic_t slot = ATOMIC_INIT(SLOT_NONE);
static atomic_t booking = ATOMIC_INIT(BOOKING_NONE);

/// The waiting booking. Written by reserve() under BOOKING_BUSY, read only once taken.
static uint64_t bookedStartUs;
static uint32_t bookedLengthUs;

/// The booking handed to MPSL. Written right before the request, read by the signal callback.
static uint64_t requestedStartUs;
static uint32_t requestedLengthUs;

/// Start of the last timeslot that actually started, as MPSL scheduled it. Written at SIGNAL_START
/// only; read by the thread only while it holds sessionFree, when no timeslot can be starting.
static uint64_t anchorUs;

/// Start of the running timeslot as measured, for the client to time its activity by. Written at
/// SIGNAL_START.
static uint64_t measuredStartUs;

/// The anchor came from an EARLIEST timeslot, so the next booked one is measured rather than trusted.
/// Signal callback only.
static bool anchorFromEarliest;

static MultiprotocolClient const *client;
static mpsl_timeslot_session_id_t sessionId;
static bool volatile ready;

/// Given when a booking is due to be handed over, taken by the thread.
static K_SEM_DEFINE(submitSignal, 0, 1);

/// Held from handing a request to MPSL until MPSL reports the session idle again: only one request
/// may be in flight. A request made from the end of a timeslot inherits the one that timeslot held.
static K_SEM_DEFINE(sessionFree, 0, 1);

static void onSubmitTimer(struct k_timer *timer) {
  ARG_UNUSED(timer);
  k_sem_give(&submitSignal);
}

/// Fires when the waiting booking is due. A cancelled booking's timer is left to fire into nothing.
static K_TIMER_DEFINE(submitTimer, onSubmitTimer, NULL);

/// Mirrors MultiprotocolStats field for field, as atomics: most counters are bumped from more than
/// one context.
static struct {
  atomic_t submitted;
  atomic_t chained;
  atomic_t granted;
  atomic_t blocked;
  atomic_t too_late;
  atomic_t failed;
  atomic_t revoked;
  atomic_t released;
  atomic_t extend_failed;
  atomic_t anchored;
  atomic_t unexpected;
} stats;

/// Last signal the callback did not expect, reported from the thread.
static uint32_t volatile unexpectedSignal;

uint64_t embenet_multiprotocol_now_us(void) {
#if defined(CONFIG_NRF_GRTC_TIMER)
  // One LDRD on the Cortex-M33, no lock - which is what makes it usable in the zero latency callback.
  return z_nrf_grtc_timer_read();
#else
  // The RTC1 system timer: no lock either, and written to be read from an interrupt that preempts its
  // own overflow handling. It counts 32768 Hz ticks of the same crystal MPSL schedules by, so the
  // microseconds are exact multiples of 1000000/32768 = 15625/512 - a 30.5 us grain, which only the
  // measured anchors carry (see the top of the file), as one constant offset well inside what the
  // stack tolerates.
  return (z_nrf_rtc_timer_read() * 15625U) / 512U;
#endif
}

uint64_t embenet_multiprotocol_slot_start_us(void) { return measuredStartUs; }

/**
 * Next piece of a booking with @p leftUs still to be asked for: a chunk, or all of it when what would
 * remain after a chunk is too short for MPSL to extend by.
 */
static uint32_t nextChunkUs(uint32_t leftUs) { return (leftUs < (CHUNK_US + MPSL_TIMESLOT_EXTENSION_TIME_MIN_US)) ? leftUs : CHUNK_US; }

/// Microseconds from now to @p timeUs; negative once it has gone by.
static int64_t untilUs(uint64_t timeUs) { return (int64_t)(timeUs - embenet_multiprotocol_now_us()); }

/// Due to be handed over: waiting, and SUBMIT_LEAD or less from its start. Read without taking it, so
/// only a hint - the booking is taken before it is acted on.
static bool bookingDue(void) {
  return (BOOKING_WAITING == atomic_get(&booking)) && (untilUs(bookedStartUs) <= (int64_t)EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US);
}

/**
 * Wakes the thread when the waiting booking falls due: SUBMIT_LEAD before its start rather than now,
 * because until then the booking can still be cancelled and MPSL keeps the time free for the other
 * protocol.
 *
 * Also called by the thread when it wakes to a booking not due yet: the kernel counts in 32 us ticks,
 * and a timer that lands on the tick before the due moment would otherwise leave the booking waiting
 * for good.
 */
static void armSubmitTimer(void) {
  int64_t const untilSubmitUs = untilUs(bookedStartUs) - (int64_t)EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US;
  if(untilSubmitUs <= 0) {
    k_sem_give(&submitSignal);
  } else {
    k_timer_start(&submitTimer, K_USEC(untilSubmitUs), K_NO_WAIT);
  }
}

/// Takes the waiting booking, so that neither a cancel nor the other submitter can have it.
static bool takeBooking(uint64_t *startUs, uint32_t *lengthUs) {
  if(!atomic_cas(&booking, BOOKING_WAITING, BOOKING_BUSY)) {
    return false;
  }
  *startUs = bookedStartUs;
  *lengthUs = bookedLengthUs;
  atomic_set(&booking, BOOKING_NONE);
  return true;
}

/// Thread context only: from the adapter's thread, or MPSL's work queue for BLOCKED.
static void deny(uint64_t startUs, MultiprotocolDenial reason) {
  static atomic_t *const counters[] = {
      [MULTIPROTOCOL_DENIED_TOO_LATE] = &stats.too_late,
      [MULTIPROTOCOL_DENIED_BLOCKED] = &stats.blocked,
      [MULTIPROTOCOL_DENIED_FAILED] = &stats.failed,
  };
  atomic_inc(counters[reason]);
  client->on_denied(startUs, reason, client->context);
}

/// Fills in a NORMAL request for the booking, measured from the anchor, and marks it in flight.
static void prepareRequest(mpsl_timeslot_request_t *request, uint64_t startUs, uint32_t lengthUs) {
  requestedStartUs = startUs;
  requestedLengthUs = lengthUs;
  *request = (mpsl_timeslot_request_t){
      .request_type = MPSL_TIMESLOT_REQ_TYPE_NORMAL,
      .params.normal =
          {
              .hfclk = EMBENET_MULTIPROTOCOL_HFCLK_CFG,
              .priority = MPSL_TIMESLOT_PRIORITY_NORMAL,
              .distance_us = (uint32_t)(startUs - anchorUs),
              .length_us = nextChunkUs(lengthUs),
          },
  };
  atomic_set(&slot, SLOT_SUBMITTED);
}

int embenet_multiprotocol_reserve(uint64_t start_us, uint32_t length_us) {
  if(!ready || (length_us < EMBENET_MULTIPROTOCOL_SHORTEST_US)) {
    return -EINVAL;
  }
  if(untilUs(start_us) < (int64_t)EMBENET_MULTIPROTOCOL_MIN_LEAD_US) {
    return -ETIME;
  }
  if(!atomic_cas(&booking, BOOKING_NONE, BOOKING_BUSY)) {
    return -EBUSY;
  }
  bookedStartUs = start_us;
  bookedLengthUs = length_us;
  atomic_set(&booking, BOOKING_WAITING);

  armSubmitTimer();
  return 0;
}

bool embenet_multiprotocol_cancel(void) { return atomic_cas(&booking, BOOKING_WAITING, BOOKING_NONE); }

/**
 * The timeslot itself is ended by the signal callback: it is the only context allowed to end one, and
 * it looks at this state on every check to decide whether to hand the radio back. A request not yet
 * started is ended as soon as it does, without a grant.
 */
void embenet_multiprotocol_release(void) {
  if(!atomic_cas(&slot, SLOT_GRANTED, SLOT_NONE)) {
    (void)atomic_cas(&slot, SLOT_SUBMITTED, SLOT_NONE);
  }
}

void embenet_multiprotocol_adapter_get_stats(MultiprotocolStats *out) {
  *out = (MultiprotocolStats){
      .submitted = (uint32_t)atomic_get(&stats.submitted),
      .chained = (uint32_t)atomic_get(&stats.chained),
      .granted = (uint32_t)atomic_get(&stats.granted),
      .blocked = (uint32_t)atomic_get(&stats.blocked),
      .too_late = (uint32_t)atomic_get(&stats.too_late),
      .failed = (uint32_t)atomic_get(&stats.failed),
      .revoked = (uint32_t)atomic_get(&stats.revoked),
      .released = (uint32_t)atomic_get(&stats.released),
      .extend_failed = (uint32_t)atomic_get(&stats.extend_failed),
      .anchored = (uint32_t)atomic_get(&stats.anchored),
      .unexpected = (uint32_t)atomic_get(&stats.unexpected),
  };
}

/// Radio time MPSL has committed to, what is still to be asked for, what an extension in flight would
/// add, and when the compare last fired. Only ever touched from the zero latency signals, which are one
/// context. All in microseconds into the timeslot, which is what TIMER0 counts.
static uint32_t grantedUs;
static uint32_t remainingUs;
static uint32_t extendingUs;
static uint32_t lastCheckUs;

/// The request handed back from inside a timeslot. MPSL reads it after the callback has returned.
static mpsl_timeslot_request_t chainRequest;

/**
 * Measures where the timeslot that has just started started.
 *
 * TIMER0 was started from zero at the start, however late this signal runs; capturing it and reading
 * the system timer back to back - nothing preempts a zero latency interrupt - leaves a fixed offset of
 * a register access or two on the GRTC, and up to one 30.5 us RTC tick on the nRF52.
 */
static void measureStart(void) {
  nrf_timer_task_trigger(MPSL_TIMER0, NRF_TIMER_TASK_CAPTURE1);
  uint64_t const nowUs = embenet_multiprotocol_now_us();
  measuredStartUs = nowUs - nrf_timer_cc_get(MPSL_TIMER0, NRF_TIMER_CC_CHANNEL1);
}

/// End of the radio time the client may use, granted so far: where the hand-back has to begin.
static uint32_t endOfSlotUs(void) { return grantedUs - END_GUARD_US; }

/**
 * Arms the next look at the timeslot: RELEASE_POLL_US after the last one, or the end of the slot if
 * that comes first.
 *
 * Re-armed after every extension, which is what makes a refused one harmless - there is always a
 * compare standing at or before the end of time that has actually been granted.
 */
static void armCheck(void) {
  nrf_timer_cc_set(MPSL_TIMER0, NRF_TIMER_CC_CHANNEL0, MIN(lastCheckUs + RELEASE_POLL_US, endOfSlotUs()));
  nrf_timer_int_enable(MPSL_TIMER0, NRF_TIMER_INT_COMPARE0_MASK);
}

static void disarmCheck(void) {
  nrf_timer_int_disable(MPSL_TIMER0, NRF_TIMER_INT_COMPARE0_MASK);
  nrf_timer_event_clear(MPSL_TIMER0, NRF_TIMER_EVENT_COMPARE0);
}

/**
 * Ends the timeslot - and, if the waiting booking is due before the session could go idle and the
 * thread hand it over, requests it from here instead, which is what lets timeslots follow each other
 * back to back.
 *
 * Left to the thread: a booking not due yet, so that it can still be cancelled; and one that starts
 * before this timeslot was booked to end, or has started already, which the thread denies.
 *
 * @note Measured: a timeslot requested here less than about 2.2 ms before its start - one that
 *       follows a timeslot held to its end closely - is granted, but starts some 25 us late. Nothing
 *       earlier is possible: asking for the next timeslot ends this one. The anchor is unaffected, it
 *       follows the booked start; the client sees the real start in
 *       embenet_multiprotocol_slot_start_us().
 */
static void endOrChain(mpsl_timeslot_signal_return_param_t *action) {
  action->callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_END;

  if(BOOKING_WAITING != atomic_get(&booking)) {
    return;
  }
  int64_t const leadUs = untilUs(bookedStartUs);
  if((leadUs <= 0) || (leadUs > (int64_t)EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US) || (bookedStartUs < (anchorUs + grantedUs))) {
    return;
  }

  uint64_t startUs;
  uint32_t lengthUs;
  if(!takeBooking(&startUs, &lengthUs)) {
    return;
  }
  prepareRequest(&chainRequest, startUs, lengthUs);
  atomic_inc(&stats.submitted);
  atomic_inc(&stats.chained);
  action->callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_REQUEST;
  action->params.request.p_next = &chainRequest;
}

/// Hands the timeslot back early because the client released it.
static void endReleased(mpsl_timeslot_signal_return_param_t *action) {
  disarmCheck();
  atomic_inc(&stats.released);
  endOrChain(action);
}

/**
 * Makes the timeslot safe to sit in for as long as MPSL has granted, then asks for the next chunk.
 *
 * The whole booking is secured straight away: MPSL runs EXTEND_SUCCEEDED in the context of the signal
 * that asked for it, so the chunks chain up back to back at START.
 */
static void holdOrExtend(mpsl_timeslot_signal_return_param_t *action) {
  armCheck();

  if(0U != remainingUs) {
    extendingUs = nextChunkUs(remainingUs);
    action->callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_EXTEND;
    action->params.extend.length_us = extendingUs;
  }
}

static void onStart(mpsl_timeslot_signal_return_param_t *action) {
  measureStart();

  grantedUs = nextChunkUs(requestedLengthUs);
  remainingUs = requestedLengthUs - grantedUs;
  extendingUs = 0U;
  lastCheckUs = 0U;

  if(atomic_cas(&slot, SLOT_ANCHORING, SLOT_NONE)) {
    // Wanted for its start alone; the thread waiting for it carries on once the session is idle.
    anchorUs = measuredStartUs;
    anchorFromEarliest = true;
    atomic_inc(&stats.anchored);
    action->callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_END;
    return;
  }

  // A booking, released or not: started where it was booked to, as far as MPSL is concerned - except
  // the first after an EARLIEST anchor, which lines the chain up with booked timeslots.
  anchorUs = anchorFromEarliest ? measuredStartUs : requestedStartUs;
  anchorFromEarliest = false;

  if(!atomic_cas(&slot, SLOT_SUBMITTED, SLOT_GRANTED)) {
    // Released while it was waiting to start. Still a start - the anchor has moved.
    endOrChain(action);
    return;
  }

  atomic_inc(&stats.granted);
  client->on_granted(requestedStartUs, client->context);

  holdOrExtend(action);
}

static void onCheck(mpsl_timeslot_signal_return_param_t *action) {
  lastCheckUs = nrf_timer_cc_get(MPSL_TIMER0, NRF_TIMER_CC_CHANNEL0);
  nrf_timer_event_clear(MPSL_TIMER0, NRF_TIMER_EVENT_COMPARE0);

  if(SLOT_GRANTED != atomic_get(&slot)) {
    // release() cannot end the timeslot itself - it may run after the timeslot is over, when TIMER0
    // is MPSL's again - so it is noticed here, at most RELEASE_POLL_US late.
    endReleased(action);
    return;
  }

  if(lastCheckUs < endOfSlotUs()) {
    armCheck();
    return;
  }

  disarmCheck();
  if(atomic_cas(&slot, SLOT_GRANTED, SLOT_NONE)) {
    atomic_inc(&stats.revoked);
    client->on_revoked(client->context);
  } else {
    // Released between the look above and the CAS.
    atomic_inc(&stats.released);
  }
  endOrChain(action);
}

/**
 * Called by MPSL for everything that happens to the session.
 *
 * @warning Mostly a zero latency interrupt: no kernel calls, no logging, and back out quickly. See
 *          the note at the top of the file for the four signals that are not.
 */
static mpsl_timeslot_signal_return_param_t *onTimeslotSignal(mpsl_timeslot_session_id_t id, uint32_t signal) {
  ARG_UNUSED(id);

  static mpsl_timeslot_signal_return_param_t action; // must outlive the call - MPSL reads it after the return

  action.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;

  switch(signal) {
  case MPSL_TIMESLOT_SIGNAL_START:
    onStart(&action);
    break;

  case MPSL_TIMESLOT_SIGNAL_EXTEND_SUCCEEDED:
    grantedUs += extendingUs;
    remainingUs -= extendingUs;
    extendingUs = 0U;

    if(SLOT_GRANTED != atomic_get(&slot)) {
      // Released from on_granted() already.
      endReleased(&action);
      break;
    }

    holdOrExtend(&action);
    break;

  case MPSL_TIMESLOT_SIGNAL_EXTEND_FAILED:
    // Something else is scheduled in the time that was asked for. The compare armed for the length
    // already granted stands, so the booking is cut short there.
    atomic_inc(&stats.extend_failed);
    extendingUs = 0U;
    remainingUs = 0U;
    break;

  case MPSL_TIMESLOT_SIGNAL_TIMER0:
    onCheck(&action);
    break;

  case MPSL_TIMESLOT_SIGNAL_BLOCKED:
  case MPSL_TIMESLOT_SIGNAL_CANCELLED:
    // Never started, so the anchor stays at the last timeslot that did. No SESSION_IDLE follows -
    // measured: a session waited for one after a block never got it - so this is what lets the next
    // request through. The client hears of it here, from MPSL's work queue.
    if(atomic_cas(&slot, SLOT_SUBMITTED, SLOT_NONE)) {
      k_sem_give(&sessionFree);
      deny(requestedStartUs, MULTIPROTOCOL_DENIED_BLOCKED);
    } else {
      // An anchor that could not be had, which its taker sees from the counter; or a booking already
      // released.
      (void)atomic_cas(&slot, SLOT_ANCHORING, SLOT_NONE);
      k_sem_give(&sessionFree);
    }
    break;

  case MPSL_TIMESLOT_SIGNAL_SESSION_IDLE:
  case MPSL_TIMESLOT_SIGNAL_SESSION_CLOSED:
    k_sem_give(&sessionFree);
    break;

  default:
    // INVALID_RETURN and OVERSTAYED arrive in the context of the signal that caused them, which is
    // the zero latency one - hence a counter and not a kernel call or a log line. Whatever went
    // wrong, SESSION_IDLE still follows and releases the session.
    unexpectedSignal = signal;
    atomic_inc(&stats.unexpected);
    break;
  }

  return &action;
}

/// Human-readable MPSL signal names, for the one place that reports them from thread context.
static char const *signalName(uint32_t signal) {
  static char const *const names[] = {"START",     "TIMER0",       "RADIO",          "EXTEND_FAILED",  "EXTEND_SUCCEEDED", "BLOCKED",
                                      "CANCELLED", "SESSION_IDLE", "INVALID_RETURN", "SESSION_CLOSED", "OVERSTAYED"};
  return (signal < ARRAY_SIZE(names)) ? names[signal] : "UNKNOWN";
}

/// Reports whatever the signal callback could not report itself.
static void reportTrouble(void) {
  static uint32_t reported;

  uint32_t const count = (uint32_t)atomic_get(&stats.unexpected);
  if(count != reported) {
    LOG_WRN("MPSL reported %" PRIu32 " unexpected signal(s), last %s", count - reported, signalName(unexpectedSignal));
    reported = count;
  }
}

/**
 * Takes a short EARLIEST timeslot only to measure its start.
 *
 * @pre The caller holds sessionFree, and still does on return.
 * @return true once a timeslot has started; false if MPSL would not place one within the timeout.
 */
static bool takeAnchor(void) {
  atomic_val_t const before = atomic_get(&stats.anchored);

  mpsl_timeslot_request_t const request = {
      .request_type = MPSL_TIMESLOT_REQ_TYPE_EARLIEST,
      .params.earliest =
          {
              .hfclk = EMBENET_MULTIPROTOCOL_HFCLK_CFG,
              .priority = MPSL_TIMESLOT_PRIORITY_NORMAL,
              .length_us = ANCHOR_LENGTH_US,
              .timeout_us = ANCHOR_TIMEOUT_US,
          },
  };

  atomic_set(&slot, SLOT_ANCHORING);
  int32_t const err = mpsl_timeslot_request(sessionId, &request);
  if(0 != err) {
    atomic_set(&slot, SLOT_NONE);
    LOG_WRN("anchoring mpsl_timeslot_request() failed: %" PRId32, err);
    return false;
  }

  // Started or blocked, SESSION_IDLE follows - at the latest ANCHOR_TIMEOUT_US from now.
  (void)k_sem_take(&sessionFree, K_FOREVER);
  return atomic_get(&stats.anchored) != before;
}

/**
 * Hands the waiting booking to MPSL.
 *
 * @pre The caller holds sessionFree. Gives it back unless a request is left in flight.
 */
static void submitBooking(void) {
  uint64_t startUs;
  uint32_t lengthUs;
  // Gone since the thread was woken - cancelled, requested from a timeslot end, or replaced by one
  // that is not due yet and has its own timer running.
  if(!bookingDue() || !takeBooking(&startUs, &lengthUs)) {
    k_sem_give(&sessionFree);
    return;
  }

  // Only started timeslots move the anchor; one left far enough behind takes a distance MPSL refuses.
  if(((startUs - anchorUs) > MPSL_TIMESLOT_DISTANCE_MAX_US) && !takeAnchor()) {
    k_sem_give(&sessionFree);
    deny(startUs, MULTIPROTOCOL_DENIED_FAILED);
    return;
  }

  if(untilUs(startUs) < (int64_t)EMBENET_MULTIPROTOCOL_MIN_LEAD_US) {
    k_sem_give(&sessionFree);
    deny(startUs, MULTIPROTOCOL_DENIED_TOO_LATE);
    return;
  }

  mpsl_timeslot_request_t request;
  prepareRequest(&request, startUs, lengthUs);
  int32_t const err = mpsl_timeslot_request(sessionId, &request);
  if(0 != err) {
    atomic_set(&slot, SLOT_NONE);
    k_sem_give(&sessionFree);
    LOG_WRN("mpsl_timeslot_request() failed: %" PRId32, err);
    deny(startUs, MULTIPROTOCOL_DENIED_FAILED);
    return;
  }
  atomic_inc(&stats.submitted);
}

/**
 * Hands bookings to MPSL when they fall due.
 *
 * Exists because mpsl_timeslot_request() is a thread-context call while the client books from
 * wherever it likes.
 */
static void submitThread(void *unused0, void *unused1, void *unused2) {
  ARG_UNUSED(unused0);
  ARG_UNUSED(unused1);
  ARG_UNUSED(unused2);

  for(;;) {
    bool const signalled = (0 == k_sem_take(&submitSignal, IDLE_REPORT_PERIOD));

    reportTrouble();
    if(!signalled) {
      continue;
    }

    if((BOOKING_WAITING == atomic_get(&booking)) && !bookingDue()) {
      armSubmitTimer();
      continue;
    }

    while(bookingDue()) {
      if(0 == k_sem_take(&sessionFree, K_USEC(EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US))) {
        submitBooking();
        break;
      }

      // The timeslot before is still running. Its end requests the booking if it can; if the start
      // goes by first, that timeslot ran into it.
      uint64_t startUs;
      uint32_t lengthUs;
      if((BOOKING_WAITING == atomic_get(&booking)) && (untilUs(bookedStartUs) < 0) && takeBooking(&startUs, &lengthUs)) {
        deny(startUs, MULTIPROTOCOL_DENIED_TOO_LATE);
      }
    }
  }
}

/// Stack of @ref submitThread: an mpsl_timeslot_request_t, the MPSL call, a deferred log line and the client's
/// on_denied(), which multiprotocol_port.c leaves empty. Measured at 276 bytes on an nRF52840 and 292 on an nRF54L15,
/// with the node joined and a Bluetooth central connected; the late and failed paths, which log a warning, did not
/// run. CONFIG_LOG_MODE_IMMEDIATE would format those warnings here and need more.
#define SUBMIT_THREAD_STACK_SIZE 1024

K_THREAD_DEFINE(embenet_multiprotocol_thread, SUBMIT_THREAD_STACK_SIZE, submitThread, NULL, NULL, NULL,
                // Cooperative and above anything else the application runs: a booking handed over
                // late is a booking lost.
                K_HIGHEST_APPLICATION_THREAD_PRIO, 0, 0);

int32_t embenet_multiprotocol_init(MultiprotocolClient const *newClient) {
  client = newClient;

  int32_t const err = mpsl_timeslot_session_open(onTimeslotSignal, &sessionId);
  if(0 != err) {
    LOG_ERR("mpsl_timeslot_session_open() failed: %" PRId32, err);
    return err;
  }

  // The session starts out idle, so the token starts out here: held through the anchor, then given.
  bool const anchored = takeAnchor();
  k_sem_give(&sessionFree);
  if(!anchored) {
    LOG_ERR("no timeslot within %u us to measure from", ANCHOR_TIMEOUT_US);
    return -ETIMEDOUT;
  }

  ready = true;
  LOG_INF("radio shared through MPSL timeslots, first anchored at %" PRIu64 " us", anchorUs);
  return 0;
}
