/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     EUI-64 address
 *
 * The EUI-64 is the chip's Bluetooth LE random static address, an EUI-48 widened to 64 bits with zeros: the same
 * 48 bits, in the same order, that a Bluetooth scanner shows for the device. A node that also runs Bluetooth is then
 * known by one address on both radios, and one without Bluetooth by the address it would have.
 *
 * The address is built the way the Bluetooth controller builds it (Zephyr's hci_vendor_read_static_addr(); the
 * SoftDevice Controller gives the same): FICR DEVICEADDR[0] is the low 32 bits, the low 16 bits of DEVICEADDR[1] the
 * high 16, and the two most significant bits are set, as the Bluetooth Core Specification requires of a static address.
 */

#include <embenet_port/eui64.h>

#include <hal/nrf_ficr.h>
#include <zephyr/logging/log.h>

#include <stdint.h>

LOG_MODULE_REGISTER(embenet_eui64, CONFIG_EMBENET_LOG_LEVEL);

#if !(NRF_FICR_HAS_DEVICE_ADDR || NRF_FICR_HAS_BLE_ADDR)
#error "the EUI-64 is the FICR device address, which this SoC does not have"
#endif

/// Bits 47:46 of a Bluetooth LE random static address, both 1.
#define STATIC_ADDRESS_MARK 0xC000U

uint64_t EMBENET_EUI64_Get(void) {
  uint32_t const low = nrf_ficr_deviceaddr_get(NRF_FICR, 0);
  uint32_t const high = nrf_ficr_deviceaddr_get(NRF_FICR, 1) & UINT16_MAX;

  if((UINT32_MAX == low) && (UINT16_MAX == high)) {
    // An erased FICR. The stack uses the EUI-64 as a network identity, so a zero here would collide with every other
    // node that also failed to read one.
    LOG_ERR("FICR device address not programmed");
    return 0;
  }

  return ((uint64_t)(high | STATIC_ADDRESS_MARK) << 32) | low;
}
