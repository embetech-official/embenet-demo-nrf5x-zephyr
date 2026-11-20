/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Border router link over the shared in-memory queue
 *
 * All-in-one counterpart of brt_uart.c: the border router runs in this same image, so frames go
 * through the queues in ports/common instead of over a wire.
 */

#include <embenet_port/border_router_communication.h>

#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(node_rnc, CONFIG_EMBENET_LOG_LEVEL);

void EMBENET_BRT_Init(void) { EMBENET_PORT_COMMON_ResetQueues(); }

void EMBENET_BRT_Deinit(void) {}

void EMBENET_BRT_Send(void const *packet, uint32_t packetLength) {
  if((packet == NULL) || (packetLength == 0)) {
    return;
  }
  if(!EMBENET_PORT_COMMON_BrFramePush(packet, packetLength)) {
    LOG_WRN("border router queue full, dropped %u bytes", (unsigned)packetLength);
  }
}

uint32_t EMBENET_BRT_Receive(void *packetBuffer, uint32_t packetBufferSize) {
  if((packetBuffer == NULL) || (packetBufferSize == 0)) {
    return 0;
  }
  // Polled from EMBENET_NODE_Proc(), which must not block.
  uint32_t const received = (uint32_t)EMBENET_PORT_COMMON_RootFramePop(packetBuffer, packetBufferSize, K_NO_WAIT);
  if(received) {
    LOG_DBG("received %u bytes", (unsigned)received);
    LOG_HEXDUMP_DBG(packetBuffer, received, "RX");
  }
  return received;
}

void EMBENET_BRT_Reset(void) {
  // Not sys_reboot(): the border router asking its root node to restart must not take the border
  // router down with it. The application loop picks the request up and restarts the node alone.
  LOG_WRN("reset requested by the stack");
  EMBENET_PORT_COMMON_ResetRequestPush();
}
