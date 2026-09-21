/**
 * @file
 * @brief embeNET Node timer port test.
 *
 * Exercises EMBENET_TIMER_* over the behaviour the stack's slot schedule actually depends on: that
 * the counter advances one unit per microsecond, that a compare fires once and on time, and - the
 * part a port is most likely to get wrong - that a compare already in the past fires straight away
 * instead of being read as one nearly a wrap into the future.
 *
 * The rate check measures the port against a TIMER instance rather than against the kernel clock.
 * On the nRF54L the kernel clock *is* the GRTC, so k_busy_wait() and k_cycle_get_32() would be the
 * same counter under another name and would agree with it however wrong it was. TIMER20 runs off
 * HFCLK and shares nothing with the GRTC but the die.
 */

#include <embenet_port/timer.h>
#include <embenet_port_common/embenet_port_common.h>

#include <hal/nrf_timer.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdlib.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Free on the nRF54L15 DK: every TIMER node is disabled in devicetree and nrfx claims none of them.
// PCLK is 16 MHz, so a prescaler of 4 gives the 1 MHz that makes its ticks microseconds too.
#define REF_TIMER NRF_TIMER20
#define REF_TIMER_PRESCALER 4

// Passed to EMBENET_TIMER_Init() and checked on arrival, so a port that drops the context pointer
// or hands back its own is caught rather than assumed correct.
static uint32_t const CONTEXT_MAGIC = 0xC0FFEE5AU;

static struct k_sem compareSem;

static uint32_t volatile compareCount;
static EMBENET_TimeUs volatile compareReadAt; // counter value sampled inside the callback
static void *volatile compareContext;

static void onCompare(void *context) {
  // First statement, so the figure the accuracy tests report is the callback's own lateness and not
  // the bookkeeping below it.
  compareReadAt = EMBENET_TIMER_ReadCounter();
  compareContext = context;
  ++compareCount;
  k_sem_give(&compareSem);
}

static void refTimerStart(void) {
  nrf_timer_task_trigger(REF_TIMER, NRF_TIMER_TASK_STOP);
  nrf_timer_mode_set(REF_TIMER, NRF_TIMER_MODE_TIMER);
  nrf_timer_bit_width_set(REF_TIMER, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_prescaler_set(REF_TIMER, REF_TIMER_PRESCALER);
  nrf_timer_task_trigger(REF_TIMER, NRF_TIMER_TASK_CLEAR);
  nrf_timer_task_trigger(REF_TIMER, NRF_TIMER_TASK_START);
}

static uint32_t refTimerRead(void) {
  nrf_timer_task_trigger(REF_TIMER, NRF_TIMER_TASK_CAPTURE0);
  return nrf_timer_cc_get(REF_TIMER, NRF_TIMER_CC_CHANNEL0);
}

/**
 * Arms a compare @p delayUs from now and waits for the callback.
 *
 * @param[out] errorUs how late the callback ran, against the value that was asked for. Negative
 *                     would mean early, which the hardware should never produce.
 */
static bool armAndWait(int32_t delayUs, k_timeout_t wait, int32_t *errorUs) {
  k_sem_reset(&compareSem);
  uint32_t const before = compareCount;

  EMBENET_TimeUs const target = EMBENET_TIMER_ReadCounter() + (EMBENET_TimeUs)delayUs;
  EMBENET_TIMER_SetCompare(target);

  if(0 != k_sem_take(&compareSem, wait)) {
    return false;
  }
  if(errorUs != NULL) {
    // Subtracting in 32 bits and reading the result as signed keeps this correct across the wrap,
    // which is the same arithmetic the port itself has to get right.
    *errorUs = (int32_t)(compareReadAt - target);
  }
  return compareCount == (before + 1U);
}

/**
 * As armAndWait(), but spins instead of sleeping.
 *
 * The difference matters: the SYSCOUNTER stops ticking when every CPU is idle and has to resynchronise
 * against the low-frequency timer on the way back, and that resynchronisation is quantized to the
 * 30.5 us LFCLK. Waiting on a semaphore idles the CPU and pays it; a node inside a slot does not.
 */
static bool armAndSpin(int32_t delayUs, int32_t *errorUs) {
  uint32_t const before = compareCount;

  EMBENET_TimeUs const target = EMBENET_TIMER_ReadCounter() + (EMBENET_TimeUs)delayUs;
  EMBENET_TIMER_SetCompare(target);

  uint32_t const guardStart = refTimerRead();
  while(compareCount == before) {
    if((uint32_t)(refTimerRead() - guardStart) > 1000000U) {
      return false;
    }
  }
  if(errorUs != NULL) {
    *errorUs = (int32_t)(compareReadAt - target);
  }
  return true;
}

/// One microsecond per tick, measured against a clock the port does not use.
static bool test_tick_rate(void) {
  enum { MEASURE_US = 200000 };

  uint32_t const refStart = refTimerRead();
  EMBENET_TimeUs const portStart = EMBENET_TIMER_ReadCounter();

  // Busy-wait rather than sleep: this must not depend on the kernel clock either.
  while((uint32_t)(refTimerRead() - refStart) < (uint32_t)MEASURE_US) {
  }

  EMBENET_TimeUs const portElapsed = EMBENET_TIMER_ReadCounter() - portStart;
  uint32_t const refElapsed = refTimerRead() - refStart;

  // 2% covers HFINT's untrimmed error and the crystal's together; it is nowhere near loose enough
  // to admit a counter running on the raw 32.768 kHz clock, which is the mistake worth catching.
  int32_t const error = (int32_t)portElapsed - (int32_t)refElapsed;
  uint32_t const tolerance = refElapsed / 50U;

  LOG_INF("  port %u us vs reference %u us over %d ms (%+d us)", portElapsed, refElapsed, MEASURE_US / 1000, error);
  if((uint32_t)abs(error) > tolerance) {
    LOG_ERR("  off by more than 2%%");
    return false;
  }
  return true;
}

/**
 * Init() puts the counter back to zero.
 *
 * Everything else here measures differences, so a counter that starts at an arbitrary offset passes
 * the whole rest of the suite. It is still wrong: the stack works in 32-bit microseconds, and a port
 * that hands it a large starting value has already spent part of the range before the first slot and
 * restarts somewhere different every time. The nRF52 backend clears its RTC; the GRTC cannot be
 * cleared, so its backend has to subtract an origin instead, and this is what checks that it does.
 */
static bool test_counter_origin(void) {
  EMBENET_TIMER_Deinit();
  EMBENET_TIMER_Init(onCompare, (void *)&CONTEXT_MAGIC);

  EMBENET_TimeUs const start = EMBENET_TIMER_ReadCounter();
  LOG_INF("  reads %u us immediately after Init()", start);

  // Generous: the read is a handful of register accesses after the origin is taken.
  if(start > 1000U) {
    LOG_ERR("  counter did not start from zero");
    return false;
  }
  return true;
}

/// The counter only ever advances.
static bool test_monotonic(void) {
  EMBENET_TimeUs previous = EMBENET_TIMER_ReadCounter();
  for(unsigned i = 0; i < 200000U; ++i) {
    EMBENET_TimeUs const now = EMBENET_TIMER_ReadCounter();
    if((int32_t)(now - previous) < 0) {
      LOG_ERR("  counter went backwards: %u then %u", previous, now);
      return false;
    }
    previous = now;
  }
  return true;
}

/// Must be half the range the 32-bit interface can express, which is where SetCompare() splits
/// future from past. A smaller value would make the stack treat live compares as expired.
static bool test_max_compare_duration(void) {
  EMBENET_TimeUs const reported = EMBENET_TIMER_GetMaxCompareDuration();
  LOG_INF("  reports %u us", reported);
  if(reported != 0x7FFFFFFFU) {
    LOG_ERR("  expected %u", 0x7FFFFFFFU);
    return false;
  }
  return true;
}

/// A compare fires, exactly once, and carries the context it was given.
static bool test_compare_fires(void) {
  compareContext = NULL;
  int32_t error = 0;
  if(!armAndWait(10000, K_MSEC(100), &error)) {
    LOG_ERR("  no callback within 100 ms");
    return false;
  }
  if(compareContext != &CONTEXT_MAGIC) {
    LOG_ERR("  callback got the wrong context");
    return false;
  }

  // Nothing re-arms it, so a channel left periodic by mistake shows up here.
  uint32_t const after = compareCount;
  k_sleep(K_MSEC(50));
  if(compareCount != after) {
    LOG_ERR("  fired again on its own (%u extra)", compareCount - after);
    return false;
  }
  return true;
}

/// Lateness across the horizons the slot schedule actually asks for.
static bool test_compare_accuracy(void) {
  static int32_t const horizons[] = {100, 500, 1000, 5000, 20000, 100000};
  bool ok = true;

  for(size_t i = 0; i < ARRAY_SIZE(horizons); ++i) {
    int32_t error = 0;
    if(!armAndWait(horizons[i], K_MSEC(500), &error)) {
      LOG_ERR("  no callback for a %d us compare", horizons[i]);
      ok = false;
      continue;
    }
    LOG_INF("  %6d us -> %+d us", horizons[i], error);
    // Early is impossible by construction, so it would mean the callback read a counter that is not
    // the one the compare was set against.
    if(error < 0) {
      LOG_ERR("  fired early, which the hardware cannot do");
      ok = false;
    }
  }
  return ok;
}

/**
 * The figure that decides whether sharing Zephyr's GRTC interrupt is good enough for TSCH: the
 * spread between the earliest and latest a compare is serviced, over a long run.
 */
static bool test_compare_jitter(void) {
  enum { ROUNDS = 500, HORIZON_US = 2000 };

  int32_t worst = INT32_MIN;
  int32_t best = INT32_MAX;
  int64_t total = 0;

  for(unsigned i = 0; i < ROUNDS; ++i) {
    int32_t error = 0;
    if(!armAndWait(HORIZON_US, K_MSEC(200), &error)) {
      LOG_ERR("  no callback on round %u", i);
      return false;
    }
    worst = MAX(worst, error);
    best = MIN(best, error);
    total += error;
  }

  LOG_INF("  %d compares at %d us: min %+d us, mean %+d us, max %+d us", ROUNDS, HORIZON_US, best, (int32_t)(total / ROUNDS), worst);
  if(best < 0) {
    LOG_ERR("  fired early, which the hardware cannot do");
    return false;
  }
  return true;
}

/**
 * The same measurement with the CPU kept awake, which is the state a node is in during a slot.
 *
 * Reported rather than bounded: what the two figures are worth is the difference between them, and
 * a threshold on the absolute number would just encode this particular board and build.
 */
static bool test_compare_jitter_cpu_awake(void) {
  enum { ROUNDS = 500, HORIZON_US = 2000 };

  int32_t worst = INT32_MIN;
  int32_t best = INT32_MAX;
  int64_t total = 0;

  for(unsigned i = 0; i < ROUNDS; ++i) {
    int32_t error = 0;
    if(!armAndSpin(HORIZON_US, &error)) {
      LOG_ERR("  no callback on round %u", i);
      return false;
    }
    worst = MAX(worst, error);
    best = MIN(best, error);
    total += error;
  }

  LOG_INF("  %d compares at %d us: min %+d us, mean %+d us, max %+d us", ROUNDS, HORIZON_US, best, (int32_t)(total / ROUNDS), worst);
  if(best < 0) {
    LOG_ERR("  fired early, which the hardware cannot do");
    return false;
  }
  return true;
}

/**
 * A compare behind the counter must fire now, not in a wrap's time.
 *
 * This is the one that separates a working port from one that merely looks like it works: the
 * interface hands over an absolute 32-bit value with no sign, so a port that widens it without
 * treating the distance as signed schedules the event 71 minutes out and the stack simply stops.
 */
static bool test_compare_in_past(void) {
  static int32_t const behindUs[] = {-1, -100, -100000, -(int32_t)0x40000000};
  bool ok = true;

  for(size_t i = 0; i < ARRAY_SIZE(behindUs); ++i) {
    uint32_t const before = compareCount;
    k_sem_reset(&compareSem);

    EMBENET_TIMER_SetCompare(EMBENET_TIMER_ReadCounter() + (EMBENET_TimeUs)behindUs[i]);

    if(0 != k_sem_take(&compareSem, K_MSEC(50))) {
      LOG_ERR("  %d us behind: never fired", behindUs[i]);
      ok = false;
      continue;
    }
    if(compareCount != (before + 1U)) {
      LOG_ERR("  %d us behind: fired %u times", behindUs[i], compareCount - before);
      ok = false;
    }
  }
  return ok;
}

/// A compare a couple of microseconds out is past by the time it is written. It must still fire.
static bool test_compare_immediate(void) {
  static int32_t const horizons[] = {0, 1, 2, 5, 10};
  bool ok = true;

  for(size_t i = 0; i < ARRAY_SIZE(horizons); ++i) {
    if(!armAndWait(horizons[i], K_MSEC(50), NULL)) {
      LOG_ERR("  %d us ahead: never fired - a compare was lost", horizons[i]);
      ok = false;
    }
  }
  return ok;
}

/// The callback gives the semaphore the stack's processing loop waits on.
static bool test_proc_semaphore(void) {
  struct k_sem *const proc = EMBENET_PORT_COMMON_GetProcSemaphore();
  k_sem_reset(proc);

  if(!armAndWait(5000, K_MSEC(100), NULL)) {
    LOG_ERR("  no callback");
    return false;
  }
  if(0 != k_sem_take(proc, K_MSEC(10))) {
    LOG_ERR("  proc semaphore was not given: the stack would never wake");
    return false;
  }
  return true;
}

/// Deinit() has to silence an armed compare, or a late one re-enters a stack that has been torn down.
static bool test_deinit_silences(void) {
  k_sem_reset(&compareSem);
  EMBENET_TIMER_SetCompare(EMBENET_TIMER_ReadCounter() + 20000U);

  uint32_t const before = compareCount;
  EMBENET_TIMER_Deinit();
  k_sleep(K_MSEC(100));

  bool const ok = (compareCount == before);
  if(!ok) {
    LOG_ERR("  callback ran after Deinit()");
  }

  // Back to a usable timer whatever happened, so a failure here does not cascade.
  EMBENET_TIMER_Init(onCompare, (void *)&CONTEXT_MAGIC);
  return ok;
}

/// And Init() after Deinit() has to leave a working timer behind.
static bool test_reinit(void) {
  EMBENET_TIMER_Deinit();
  EMBENET_TIMER_Init(onCompare, (void *)&CONTEXT_MAGIC);

  if(!armAndWait(10000, K_MSEC(100), NULL)) {
    LOG_ERR("  no callback after a deinit/init cycle");
    return false;
  }
  return true;
}

static struct {
  char const *name;
  bool (*run)(void);
} const tests[] = {
    {"counter origin", test_counter_origin},
    {"tick rate", test_tick_rate},
    {"monotonic counter", test_monotonic},
    {"max compare duration", test_max_compare_duration},
    {"compare fires once", test_compare_fires},
    {"compare accuracy", test_compare_accuracy},
    {"compare jitter (CPU idle)", test_compare_jitter},
    {"compare jitter (CPU awake)", test_compare_jitter_cpu_awake},
    {"compare in the past", test_compare_in_past},
    {"compare already due", test_compare_immediate},
    {"proc semaphore", test_proc_semaphore},
    {"deinit silences compare", test_deinit_silences},
    {"deinit/init", test_reinit},
};

int main(void) {
  LOG_INF("embeNET Node timer port test");

  k_sem_init(&compareSem, 0, 1);
  refTimerStart();
  EMBENET_TIMER_Init(onCompare, (void *)&CONTEXT_MAGIC);

  unsigned failed = 0;
  for(size_t i = 0; i < ARRAY_SIZE(tests); ++i) {
    LOG_INF("---- %s", tests[i].name);
    if(tests[i].run()) {
      LOG_INF("PASS  %s", tests[i].name);
    } else {
      LOG_ERR("FAIL  %s", tests[i].name);
      ++failed;
    }
  }

  EMBENET_TIMER_Deinit();

  if(0U == failed) {
    LOG_INF("PROJECT EXECUTION SUCCESSFUL: %zu/%zu tests passed", ARRAY_SIZE(tests), ARRAY_SIZE(tests));
  } else {
    LOG_ERR("PROJECT EXECUTION FAILED: %u of %zu tests failed", failed, ARRAY_SIZE(tests));
  }
  return 0;
}
