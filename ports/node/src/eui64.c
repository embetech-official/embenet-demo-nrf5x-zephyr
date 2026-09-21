/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     EUI-64 address
 */

#include <embenet_port/eui64.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>

#include <stdint.h>
#include <sys/types.h>

LOG_MODULE_REGISTER(embenet_eui64, CONFIG_EMBENET_LOG_LEVEL);

// Taken from hwinfo, whose per-SoC driver knows which device-id registers to read.
uint64_t EMBENET_EUI64_Get(void) {
  uint8_t device_id[sizeof(uint64_t)] = {0};

  ssize_t const length = hwinfo_get_device_id(device_id, sizeof(device_id));
  if(length <= 0) {
    // The stack uses the EUI-64 as a network identity, so a zero here would collide with every other
    // node that also failed to read one.
    LOG_ERR("no unique device id available (%d)", (int)length);
    return 0;
  }

  // Assembled big-endian so a given chip always yields the same address.
  uint64_t eui64 = 0;
  for(ssize_t i = 0; i < length; ++i) {
    eui64 = (eui64 << 8) | (uint64_t)device_id[i];
  }
  return eui64;
}
