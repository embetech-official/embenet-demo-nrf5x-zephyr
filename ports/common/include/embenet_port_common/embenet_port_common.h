/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node/Border Router port for Zephyr
 * @brief     Shared queues linking the node and border router in one image
 *
 * Lives here so neither port has to include the other's headers. Both queues are Zephyr message
 * queues carrying whole frames, so either end may be fed and consumed from any thread, and a
 * consumer may block on one.
 *
 * Each queue is named after the side that *consumes* it:
 *
 * | Queue | Pushed by                         | Popped by                           |
 * |-------|-----------------------------------|-------------------------------------|
 * | Br    | node, `EMBENET_BRT_SendRaw()`     | border router, root node connector  |
 * | Root  | border router, connector write    | node, `EMBENET_BRT_ReceiveRaw()`    |
 *
 * The node polls its queue from `EMBENET_NODE_Proc()`, so it pops with `K_NO_WAIT`. The border
 * router has to be pushed to - the library hands received frames to a callback that must not run in
 * interrupt context - so the application's border router thread calls
 * @ref EMBENET_PORT_COMMON_BrFrameProc, which pops and dispatches on that thread.
 */

#ifndef EMBENET_PORT_COMMON_H_
#define EMBENET_PORT_COMMON_H_

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum size, in bytes, of a single frame.
 *
 * Covers the largest frame either direction carries: 23 bytes of RCP header plus up to 255 bytes of
 * payload.
 */
#define EMBENET_PORT_COMMON_MAX_FRAME_SIZE 300U

/**
 * @brief Initializes the embeNET Node/Border Router common part of port.
 *
 * Drops whatever both queues hold and clears any pending reset request. Safe to call again to
 * return the link to a known state - the node port does exactly that from @c EMBENET_BRT_Init(),
 * so restarting the root node also restarts the link.
 *
 * @note Initialize before calling Node/Border Router API functions.
 */
void EMBENET_PORT_COMMON_Init(void);

void EMBENET_PORT_COMMON_ResetQueues(void);

/**
 * @brief Pushes a reset request to the Node/Border Router port.
 *
 * Requests coalesce: pushing twice before a pop still yields one request.
 */
void EMBENET_PORT_COMMON_ResetRequestPush(void);

/**
 * @brief Pops a reset request from the Node/Border Router port.
 * @return true if a reset request was popped, false if the queue is empty.
 */
bool EMBENET_PORT_COMMON_ResetRequestPop(void);

/**
 * @brief Returns the semaphore that the node and border router port gives in its ISR to wake proc thread.
 * @return pointer to the semaphore.
 */
struct k_sem *EMBENET_PORT_COMMON_GetProcSemaphore(void);

/**
 * @brief Enqueues one frame towards the border router.
 *
 * @param[in] data Frame bytes to copy into the queue.
 * @param[in] length Frame length, in bytes.
 *
 * @return true if the frame was enqueued, false if the queue is full or @p length is 0 or exceeds
 * @ref EMBENET_PORT_COMMON_MAX_FRAME_SIZE.
 */
bool EMBENET_PORT_COMMON_BrFramePush(void const *data, size_t length);

/**
 * @brief Dequeues one frame addressed to the border router.
 *
 * @param[out] buffer Destination buffer for the frame bytes.
 * @param[in] buffer_size Capacity of @p buffer, in bytes.
 * @param[in] timeout How long to wait for a frame; @c K_NO_WAIT to poll, @c K_FOREVER to block.
 *
 * @return Length of the dequeued frame, in bytes, or 0 if the timeout expired with the queue empty
 * or @p buffer_size was too small to hold the frame (the frame is discarded either way).
 */
size_t EMBENET_PORT_COMMON_BrFramePop(void *buffer, size_t buffer_size, k_timeout_t timeout);

/**
 * @brief Handler invoked for each frame delivered to the border router.
 *
 * Runs in whatever thread called @ref EMBENET_PORT_COMMON_BrFrameProc.
 *
 * @param[in] context The context handed to @ref EMBENET_PORT_COMMON_SetBrFrameHandler.
 * @param[in] data Frame bytes; valid only for the duration of the call.
 * @param[in] length Frame length, in bytes.
 */
typedef void (*EMBENET_PORT_COMMON_FrameHandler)(void *context, void const *data, size_t length);

/**
 * @brief Registers who receives frames addressed to the border router.
 *
 * Set by the root node connector, which owns the library callback the frames end up in. It lives
 * here rather than in either port so that the producer does not have to include the consumer's
 * headers.
 *
 * @param[in] handler Handler to invoke per frame, or NULL to drop frames instead.
 * @param[in] context Passed back to @p handler unchanged.
 */
void EMBENET_PORT_COMMON_SetBrFrameHandler(EMBENET_PORT_COMMON_FrameHandler handler, void *context);

/**
 * @brief Delivers queued frames to the registered border router handler.
 *
 * Call from the thread that runs @c EMBENET_BR_Proc(). Waits up to @p timeout for the first frame,
 * then drains whatever else is already queued without waiting again, so one call clears a burst.
 * The library requires its data-received callback to run outside interrupt context, which is what
 * calling this from a thread provides.
 *
 * @param[in] timeout How long to wait for the first frame; @c K_NO_WAIT to poll, @c K_FOREVER to
 * block until one arrives.
 *
 * @return Number of frames delivered.
 */
size_t EMBENET_PORT_COMMON_BrFrameProc(k_timeout_t timeout);

/**
 * @brief Enqueues one frame towards the root node.
 *
 * @param[in] data Frame bytes to copy into the queue.
 * @param[in] length Frame length, in bytes.
 *
 * @return true if the frame was enqueued, false if the queue is full or @p length is 0 or exceeds
 * @ref EMBENET_PORT_COMMON_MAX_FRAME_SIZE.
 */
bool EMBENET_PORT_COMMON_RootFramePush(void const *data, size_t length);

/**
 * @brief Dequeues one frame addressed to the root node.
 *
 * @param[out] buffer Destination buffer for the frame bytes.
 * @param[in] buffer_size Capacity of @p buffer, in bytes.
 * @param[in] timeout How long to wait for a frame; @c K_NO_WAIT to poll, @c K_FOREVER to block.
 *
 * @return Length of the dequeued frame, in bytes, or 0 if the timeout expired with the queue empty
 * or @p buffer_size was too small to hold the frame (the frame is discarded either way).
 */
size_t EMBENET_PORT_COMMON_RootFramePop(void *buffer, size_t buffer_size, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif
