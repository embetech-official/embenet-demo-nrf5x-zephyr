/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief embeNET Border Router network adapter port interface.
 *
 * The network adapter is used to send and receive IPv6 packets between the Border Router and the rest of the world.
 */
#ifndef EMBENET_BORDER_ROUTER_PORT_NETWORK_ADAPTER_H
#define EMBENET_BORDER_ROUTER_PORT_NETWORK_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type alias to port-defined structure representing a network adapter.
 */
typedef struct EMBENET_BR_PORT_NetworkAdapter EMBENET_BR_PORT_NetworkAdapter;

/**
 * Returns handle to a network adapter instance with given key.
 *
 * @param[in] key Port-specific network adapter identifier.
 *
 * @return pointer to the valid network adapter instance, or nullptr if the network adapter cannot be acquired/created.
 *
 * @post the instance is valid until it is destroyed by @ref EMBENET_BR_PORT_NetworkAdapterDestroy().
 * @post the instance is in DOWN state.
 */
EMBENET_BR_PORT_NetworkAdapter *EMBENET_BR_PORT_NetworkAdapterCreate(char const *key);

/**
 * Releases the network adapter instance.
 *
 * @param[in] adapter Instance to be released.
 *
 * @pre adapter was obtained from @ref EMBENET_BR_PORT_NetworkAdapterCreate().
 */
void EMBENET_BR_PORT_NetworkAdapterDestroy(EMBENET_BR_PORT_NetworkAdapter *adapter);

/**
 * Sets the callback to be called when data is received on the network adapter.
 *
 * @param[in] adapter Instance to set the callback for.
 * @param[in] callback Callback to be called when data is received on the network adapter.
 * @param[in] context User-provided context to be passed to the callback.
 *
 * @pre adapter was obtained from @ref EMBENET_BR_PORT_NetworkAdapterCreate().
 */
void
EMBENET_BR_PORT_NetworkAdapterSetDataReceivedCallback(EMBENET_BR_PORT_NetworkAdapter *adapter,
                                                      void (*callback)(void *context, void const *data, size_t data_length), void *context);

/**
 * Sets the callback to be called when link state of the network adapter changes.
 *
 * @param adapter Instance to set the callback for.
 * @param callback Callback to be called when link state changes.
 * @param context   User-provided context to be passed to the callback.
 */
void EMBENET_BR_PORT_NetworkAdapterSetLinkStateChangedCallback(EMBENET_BR_PORT_NetworkAdapter *adapter,
                                                                                            void (*callback)(void *context, bool is_up),
                                                                                            void *context);
/**
 * Brings the network adapter instance to the UP state.
 *
 * @param[in] adapter Instance to be brought UP.
 *
 * @return true when after the call the network adapter is UP and ready to send and receive packets, false otherwise.
 */
bool EMBENET_BR_PORT_NetworkAdapterUp(EMBENET_BR_PORT_NetworkAdapter *adapter);

/**
 * Brings the network adapter instance to the DOWN state.
 *
 * @param[in] adapter Instance to be brought DOWN.
 */
void EMBENET_BR_PORT_NetworkAdapterDown(EMBENET_BR_PORT_NetworkAdapter *adapter);

/**
 * Returns the name of the network adapter instance.
 *
 * @param[in] adapter Network adapter instance.
 *
 * @return C-string containing the name of the network adapter instance.
 */
char const *EMBENET_BR_PORT_NetworkAdapterGetName(EMBENET_BR_PORT_NetworkAdapter const *adapter);

/**
 * Returns the current unicast address of the network adapter instance.
 *
 * If the network adapter uses multiple addresses, the function SHALL return the first one.
 * If the network adapter does not have an address or is in DOWN state, the function SHALL return zeroed-out value.
 *
 * @param[in] adapter Network adapter instance.
 * @param[in] address_buffer Buffer to store the address. Must be at least 16 bytes long.
 *
 * @retval true if the interface has any unicast IPv6 Address.
 * @retval false if the interface has no unicast IPv6 Address.
 *
 * @post address_buffer is filled with the current unicast IPv6 address of the network adapter, or zeroed-out value if no address is assigned.
 */
bool EMBENET_BR_PORT_NetworkAdapterGetAddress(EMBENET_BR_PORT_NetworkAdapter const *adapter, uint8_t *address_buffer);

/**
 * Sets the unicast address of the network adapter instance.
 *
 * If the function is used when the network adapter is DOWN, it may return false.
 * This function is successful only, if the network adapter will be able to receive packets sent to the address.
 *
 * @param[in] adapter Network adapter instance.
 * @param[in] address New IPv6 address to be set. The address MUST be 16 bytes long.
 *
 * @return true if the address was set successfully, false otherwise.
 */
bool EMBENET_BR_PORT_NetworkAdapterSetAddress(EMBENET_BR_PORT_NetworkAdapter *adapter, uint8_t const *address);

/**
 * Enables packet forwarding on the network adapter instance.
 *
 * @param[in] adapter Network adapter instance.
 *
 * @return true if forwarding was enabled successfully, false otherwise.
 */
bool EMBENET_BR_PORT_NetworkAdapterEnableForwarding(EMBENET_BR_PORT_NetworkAdapter *adapter);

/**
 * Set the MTU of the network adapter instance.
 *
 * @param[in] adapter Network adapter instance.
 * @param[in] mtu New MTU to be set.
 *
 * @return current MTU of the network adapter instance.
 *
 */
uint16_t EMBENET_BR_PORT_NetworkAdapterSetMTU(EMBENET_BR_PORT_NetworkAdapter *adapter, uint16_t mtu);

/**
 * Writes the data to the network adapter.
 * It is assumed that the data represent a single IPv6 Packet.
 * The function SHALL return false if the network adapter is DOWN or if the data cannot be sent for any reason.
 * The function MUST preserve the order of the packets.
 *
 * Whether this function sends the data immediately or queues it for later sending is port-defined.
 * If the function queues the data, the internal queue MUST be flushed when the network adapter is brought UP or DOWN.
 *
 * @param[in] adapter Network adapter instance.
 * @param[in] data Pointer to the data to be sent.
 * @param[in] data_length Length of the data to be sent.
 *
 * @return true if the data was sent successfully, false otherwise.
 */
bool EMBENET_BR_PORT_NetworkAdapterWrite(EMBENET_BR_PORT_NetworkAdapter *adapter, void const *data, size_t data_length);

#ifdef __cplusplus
}
#endif

#endif
