/**
 * @file
 * @brief embeNET integrated border router sample - border router and root node in one image.
 *
 * The embeNET Node runs in root mode and drives the radio, the embeNET BorderRouter runs alongside
 * it, and the two exchange bytes through an in-memory queue.
 *
 * Each library gets its own thread, so neither one's processing delays the other: the root node in
 * root.c, the border router in border_router.c. Both start on their own after a delay, so all main()
 * has to do is empty the shared queues before either of them touches them.
 */

#include <embenet/border_router.h>
#include <embenet/node.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/**
 * @brief Brings the shared queues up, then hands the gateway over to the two library threads.
 * @return Always 0; the work happens in root.c and border_router.c.
 */
int main(void) {
  LOG_INF("Starting embeNET Border Router integrated sample");
  LOG_INF("Using embeNET Node version: %s, embeNET Border Router version: %s", EMBENET_NODE_GetVersionString(), EMBENET_BR_GetVersionString());

  // Two queue purges and an atomic store, so this finishes far inside the root node thread's start
  // delay. It has to: that thread pushes to these queues as soon as it runs.
  EMBENET_PORT_COMMON_Init();

  return 0;
}
