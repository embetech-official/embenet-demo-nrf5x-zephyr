/**
 * @file
 * @brief Self-test for ports/common: frame queues, reset request, proc semaphore.
 */

#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <string.h>

#define QUEUE_CAPACITY CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT

ZTEST(port_common, test_frames_keep_their_boundaries) {
  uint8_t out[EMBENET_PORT_COMMON_MAX_FRAME_SIZE];

  // Frames of different lengths come back whole and in order, rather than as a byte stream.
  static uint8_t const short_frame[] = {1, 2, 3};
  static uint8_t const long_frame[EMBENET_PORT_COMMON_MAX_FRAME_SIZE] = {0xAA};

  zassert_true(EMBENET_PORT_COMMON_BrFramePush(short_frame, sizeof(short_frame)));
  zassert_true(EMBENET_PORT_COMMON_BrFramePush(long_frame, sizeof(long_frame)));

  zassert_equal(sizeof(short_frame), EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
  zassert_mem_equal(out, short_frame, sizeof(short_frame));
  zassert_equal(sizeof(long_frame), EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
  zassert_mem_equal(out, long_frame, sizeof(long_frame));

  zassert_equal(0, EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
}

ZTEST(port_common, test_the_two_directions_are_independent) {
  uint8_t out[EMBENET_PORT_COMMON_MAX_FRAME_SIZE];
  static uint8_t const frame[] = {7};

  zassert_true(EMBENET_PORT_COMMON_RootFramePush(frame, sizeof(frame)));
  zassert_equal(0, EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
  zassert_equal(sizeof(frame), EMBENET_PORT_COMMON_RootFramePop(out, sizeof(out), K_NO_WAIT));
}

ZTEST(port_common, test_rejected_pushes) {
  static uint8_t const oversized[EMBENET_PORT_COMMON_MAX_FRAME_SIZE + 1] = {0};

  zassert_false(EMBENET_PORT_COMMON_BrFramePush(oversized, sizeof(oversized)));
  zassert_false(EMBENET_PORT_COMMON_BrFramePush(oversized, 0));

  // Full queue: refused rather than blocking, because a producer may hold a lock.
  static uint8_t const frame[] = {1};
  for(size_t i = 0; i < QUEUE_CAPACITY; ++i) {
    zassert_true(EMBENET_PORT_COMMON_BrFramePush(frame, sizeof(frame)));
  }
  zassert_false(EMBENET_PORT_COMMON_BrFramePush(frame, sizeof(frame)));
}

ZTEST(port_common, test_frame_too_big_for_the_buffer_is_discarded) {
  static uint8_t const frame[] = {1, 2, 3, 4};
  uint8_t out[2];

  zassert_true(EMBENET_PORT_COMMON_BrFramePush(frame, sizeof(frame)));
  zassert_equal(0, EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
  // Discarded, not left at the head to jam the queue behind it.
  zassert_equal(0, EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
}

ZTEST(port_common, test_pop_blocks_until_a_frame_arrives) {
  uint8_t out[EMBENET_PORT_COMMON_MAX_FRAME_SIZE];
  int64_t const started = k_uptime_get();

  zassert_equal(0, EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_MSEC(20)));
  zassert_true(k_uptime_get() - started >= 20);
}

ZTEST(port_common, test_reset_requests_coalesce) {
  zassert_false(EMBENET_PORT_COMMON_ResetRequestPop());

  EMBENET_PORT_COMMON_ResetRequestPush();
  EMBENET_PORT_COMMON_ResetRequestPush();
  zassert_true(EMBENET_PORT_COMMON_ResetRequestPop());
  zassert_false(EMBENET_PORT_COMMON_ResetRequestPop());
}

ZTEST(port_common, test_init_clears_everything) {
  static uint8_t const frame[] = {1};
  uint8_t out[EMBENET_PORT_COMMON_MAX_FRAME_SIZE];

  zassert_true(EMBENET_PORT_COMMON_BrFramePush(frame, sizeof(frame)));
  zassert_true(EMBENET_PORT_COMMON_RootFramePush(frame, sizeof(frame)));
  EMBENET_PORT_COMMON_ResetRequestPush();

  EMBENET_PORT_COMMON_Init();

  zassert_equal(0, EMBENET_PORT_COMMON_BrFramePop(out, sizeof(out), K_NO_WAIT));
  zassert_equal(0, EMBENET_PORT_COMMON_RootFramePop(out, sizeof(out), K_NO_WAIT));
  zassert_false(EMBENET_PORT_COMMON_ResetRequestPop());
}

ZTEST(port_common, test_proc_semaphore_is_shared) {
  struct k_sem *const semaphore = EMBENET_PORT_COMMON_GetProcSemaphore();

  zassert_not_null(semaphore);
  zassert_equal_ptr(semaphore, EMBENET_PORT_COMMON_GetProcSemaphore());
  k_sem_give(semaphore);
  zassert_ok(k_sem_take(semaphore, K_NO_WAIT));
}

static void reset_state(void *fixture) {
  ARG_UNUSED(fixture);
  EMBENET_PORT_COMMON_Init();
}

ZTEST_SUITE(port_common, NULL, NULL, reset_state, NULL, NULL);
