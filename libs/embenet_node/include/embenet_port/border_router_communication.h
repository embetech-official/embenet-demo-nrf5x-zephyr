/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET PORT API
 * @brief     Border router communication interface for the EMBENET NODE Port
 */

#ifndef EMBENET_NODE_PORT_BORDER_ROUTER_COMMUNICATION_H_
#define EMBENET_NODE_PORT_BORDER_ROUTER_COMMUNICATION_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup embenet_node_port_brt Border Router Communication Interface
 *
 * This interface is used only by root instances of the embeNET Node. It provides the means to communicate with the embeNET Border Router,
 * typically over a UART or other serial connection.
 * @{
 */

/**
 * @brief Initializes the border router communication module.
 */
void EMBENET_BRT_Init(void);

/**
 * @brief Deinitializes the border router communication module.
 */
void EMBENET_BRT_Deinit(void);

/**
 * @brief Sends a framed data packet to the border router.
 *
 * @param[in] packet       pointer to the packet data; must not be NULL.
 * @param[in] packetLength length of the packet in bytes; must be greater than 0.
 */
void EMBENET_BRT_Send(void const *packet, uint32_t packetLength);

/**
 * @brief Receives a framed data packet from the border router (non-blocking).
 *
 * If a complete packet is available and fits in @p packetBuffer, it is copied into the buffer and its size is returned.
 * If the available packet is larger than @p packetBufferSize, no data is copied and 0 is returned.
 * If no packet is available, 0 is returned.
 *
 * @param[out] packetBuffer     buffer to store the received packet; must not be NULL.
 * @param[in]  packetBufferSize size of @p packetBuffer in bytes.
 *
 * @return Number of bytes written into @p packetBuffer, or 0 if no packet was available or the buffer was too small.
 */
uint32_t EMBENET_BRT_Receive(void *packetBuffer, uint32_t packetBufferSize);

/**
 * @brief Requests a device reset via the border router communication module.
 */
void EMBENET_BRT_Reset(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
