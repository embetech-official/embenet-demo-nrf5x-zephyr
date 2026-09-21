/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node/Border Router port for Zephyr
 * @brief     Shared queues linking the node and border router in one image
 */

#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <string.h>

/// Given in port ISRs, so the application loop can run EMBENET_NODE_Proc() as soon as one returns.
K_SEM_DEFINE(embenet_proc_semaphore, 0, 10);

struct k_sem *EMBENET_PORT_COMMON_GetProcSemaphore(void) { return &embenet_proc_semaphore; }

// A flag rather than a counter: the only sensible answer to "reset twice" is to reset once.
static atomic_t reset_requested;

void EMBENET_PORT_COMMON_ResetRequestPush(void) { (void)atomic_set(&reset_requested, 1); }

bool EMBENET_PORT_COMMON_ResetRequestPop(void) { return atomic_set(&reset_requested, 0) != 0; }

// Zero when no all-in-one transport is configured - there is then nobody to talk to, and the two
// queues would be several kilobytes of RAM nothing ever touches.
#if CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT > 0

/// One queue slot. Fixed size, because that is what k_msgq carries.
typedef struct {
  size_t length;
  uint8_t data[EMBENET_PORT_COMMON_MAX_FRAME_SIZE];
} frame_t;

K_MSGQ_DEFINE(br_frames, sizeof(frame_t), CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT, __alignof__(frame_t));
K_MSGQ_DEFINE(root_frames, sizeof(frame_t), CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT, __alignof__(frame_t));

static bool frame_push(struct k_msgq *queue, void const *data, size_t length) {
  if((data == NULL) || (length == 0) || (length > EMBENET_PORT_COMMON_MAX_FRAME_SIZE)) {
    return false;
  }
  frame_t frame;
  frame.length = length;
  memcpy(frame.data, data, length);
  // Never blocks, so a producer holding a lock cannot stall on a full queue.
  return k_msgq_put(queue, &frame, K_NO_WAIT) == 0;
}

static size_t frame_pop(struct k_msgq *queue, void *buffer, size_t buffer_size, k_timeout_t timeout) {
  if(buffer == NULL) {
    return 0;
  }
  frame_t frame;
  if(k_msgq_get(queue, &frame, timeout) != 0) {
    return 0;
  }
  if(frame.length > buffer_size) {
    return 0; // does not fit - discarded
  }
  memcpy(buffer, frame.data, frame.length);
  return frame.length;
}

bool EMBENET_PORT_COMMON_BrFramePush(void const *data, size_t length) { return frame_push(&br_frames, data, length); }

size_t EMBENET_PORT_COMMON_BrFramePop(void *buffer, size_t buffer_size, k_timeout_t timeout) {
  return frame_pop(&br_frames, buffer, buffer_size, timeout);
}

// Registered by the root node connector, called on the border router's own thread.
static EMBENET_PORT_COMMON_FrameHandler br_frame_handler;
static void *br_frame_handler_context;

void EMBENET_PORT_COMMON_SetBrFrameHandler(EMBENET_PORT_COMMON_FrameHandler handler, void *context) {
  // Ordered so the handler is never seen paired with a context from a previous registration.
  br_frame_handler = NULL;
  br_frame_handler_context = context;
  br_frame_handler = handler;
}

size_t EMBENET_PORT_COMMON_BrFrameProc(k_timeout_t timeout) {
  // One frame's worth of the caller's stack, rather than a buffer per queue slot.
  BUILD_ASSERT(EMBENET_PORT_COMMON_MAX_FRAME_SIZE <= 512, "frame buffer lives on the caller's stack");
  uint8_t frame[EMBENET_PORT_COMMON_MAX_FRAME_SIZE];
  size_t delivered = 0;

  // Only the first pop waits: after that, drain whatever the producer piled up behind it so a burst
  // costs one call rather than one call per frame.
  for(k_timeout_t wait = timeout;; wait = K_NO_WAIT) {
    size_t const length = EMBENET_PORT_COMMON_BrFramePop(frame, sizeof(frame), wait);
    if(length == 0) {
      break;
    }
    // Reread per frame: the connector may deregister while this loop is running.
    EMBENET_PORT_COMMON_FrameHandler const handler = br_frame_handler;
    if(handler != NULL) {
      handler(br_frame_handler_context, frame, length);
    }
    ++delivered;
  }
  return delivered;
}

bool EMBENET_PORT_COMMON_RootFramePush(void const *data, size_t length) { return frame_push(&root_frames, data, length); }

size_t EMBENET_PORT_COMMON_RootFramePop(void *buffer, size_t buffer_size, k_timeout_t timeout) {
  return frame_pop(&root_frames, buffer, buffer_size, timeout);
}

void EMBENET_PORT_COMMON_ResetQueues(void) {
  k_msgq_purge(&br_frames);
  k_msgq_purge(&root_frames);
}

#endif // CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT > 0

void EMBENET_PORT_COMMON_Init(void) {
  (void)atomic_set(&reset_requested, 0);
#if CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT > 0
  k_msgq_purge(&br_frames);
  k_msgq_purge(&root_frames);
#endif
}
