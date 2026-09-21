/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Border Router port
 * @brief     WAN network adapter
 */

// The mesh runs without a WAN side: no adapter is offered, so pass NULL as the wan_interface to
// EMBENET_BR_Configure().

#include <embenet_port/network_adapter.h>

EMBENET_BR_PORT_NetworkAdapter *EMBENET_BR_PORT_NetworkAdapterCreate(char const *key) {
  (void)key;
  return NULL;
}

void EMBENET_BR_PORT_NetworkAdapterDestroy(EMBENET_BR_PORT_NetworkAdapter *adapter) { (void)adapter; }

void EMBENET_BR_PORT_NetworkAdapterSetDataReceivedCallback(EMBENET_BR_PORT_NetworkAdapter *adapter, void (*callback)(void *, void const *, size_t),
                                                           void *context) {
  (void)adapter;
  (void)callback;
  (void)context;
}

void EMBENET_BR_PORT_NetworkAdapterSetLinkStateChangedCallback(EMBENET_BR_PORT_NetworkAdapter *adapter, void (*callback)(void *, bool),
                                                               void *context) {
  (void)adapter;
  (void)callback;
  (void)context;
}

bool EMBENET_BR_PORT_NetworkAdapterUp(EMBENET_BR_PORT_NetworkAdapter *adapter) {
  (void)adapter;
  return false;
}

void EMBENET_BR_PORT_NetworkAdapterDown(EMBENET_BR_PORT_NetworkAdapter *adapter) { (void)adapter; }

char const *EMBENET_BR_PORT_NetworkAdapterGetName(EMBENET_BR_PORT_NetworkAdapter const *adapter) {
  (void)adapter;
  return "";
}

bool EMBENET_BR_PORT_NetworkAdapterGetAddress(EMBENET_BR_PORT_NetworkAdapter const *adapter, uint8_t *address_buffer) {
  (void)adapter;
  (void)address_buffer;
  return false;
}

bool EMBENET_BR_PORT_NetworkAdapterSetAddress(EMBENET_BR_PORT_NetworkAdapter *adapter, uint8_t const *address) {
  (void)adapter;
  (void)address;
  return false;
}

bool EMBENET_BR_PORT_NetworkAdapterEnableForwarding(EMBENET_BR_PORT_NetworkAdapter *adapter) {
  (void)adapter;
  return false;
}

uint16_t EMBENET_BR_PORT_NetworkAdapterSetMTU(EMBENET_BR_PORT_NetworkAdapter *adapter, uint16_t mtu) {
  (void)adapter;
  return mtu;
}

bool EMBENET_BR_PORT_NetworkAdapterWrite(EMBENET_BR_PORT_NetworkAdapter *adapter, void const *data, size_t data_length) {
  (void)adapter;
  (void)data;
  (void)data_length;
  return false;
}
