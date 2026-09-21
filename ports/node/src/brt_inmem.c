/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
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

void EMBENET_BRT_SendRaw(void const *data, size_t dataLength) {
  if((data == NULL) || (dataLength == 0)) {
    return;
  }
  if(!EMBENET_PORT_COMMON_BrFramePush(data, dataLength)) {
    LOG_WRN("border router queue full, dropped %zu bytes", dataLength);
  }
}

size_t EMBENET_BRT_ReceiveRaw(void *data, size_t dataBufferSize) {
  if((data == NULL) || (dataBufferSize == 0)) {
    return 0;
  }
  // Polled from EMBENET_NODE_Proc(), which must not block.
  return EMBENET_PORT_COMMON_RootFramePop(data, dataBufferSize, K_NO_WAIT);
}

void EMBENET_BRT_Send(void const *packet, size_t packetLength) { EMBENET_BRT_SendRaw(packet, packetLength); }

size_t EMBENET_BRT_Receive(void *packetBuffer, size_t packetBufferSize) {
  LOG_DBG("EMBENET_BRT_Receive called with buffer size %zu", packetBufferSize);
  size_t ret = EMBENET_BRT_ReceiveRaw(packetBuffer, packetBufferSize);
  if(ret) {
    LOG_DBG("EMBENET_BRT_ReceiveRaw received %zu bytes", ret);
    LOG_HEXDUMP_DBG(packetBuffer, ret, "RX");
  }
  return ret;
}

void EMBENET_BRT_Reset(void) {
  // Not sys_reboot(): the border router asking its root node to restart must not take the border
  // router down with it. The application loop picks the request up and restarts the node alone.
  LOG_WRN("reset requested by the stack");
  EMBENET_PORT_COMMON_ResetRequestPush();
}

// Queue writes complete synchronously, so nothing is ever in flight.
bool EMBENET_BRT_IsBusy(void) { return false; }
