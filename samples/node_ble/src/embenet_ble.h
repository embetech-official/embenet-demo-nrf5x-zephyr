/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @brief     embeNET Bluetooth LE peripheral: advertising, and the Provisioning and Info GATT services.
 *
 * All UUIDs share one randomly generated vendor base, BAB8xxxx-E237-4BC2-80C7-3AC1568EF5A6, and differ in the 16-bit
 * field xxxx: a service takes a multiple of 0x0100 and its characteristics count up from it. The services and the
 * byte layout of every characteristic are described in README.md next to this file.
 */
#ifndef EMBENET_BLE_H_
#define EMBENET_BLE_H_

#include <zephyr/bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Encodes the 128-bit embeNET UUID with 16-bit field @p id, for BT_UUID_INIT_128() and BT_UUID_DECLARE_128().
#define EMBENET_BLE_UUID_ENCODE(id) BT_UUID_128_ENCODE(0xBAB80000U | (id), 0xE237, 0x4BC2, 0x80C7, 0x3AC1568EF5A6ULL)

/// @name embeNET Provisioning service
/// @{
#define EMBENET_BLE_UUID_PROVISIONING_SERVICE_VAL EMBENET_BLE_UUID_ENCODE(0x0100)
#define EMBENET_BLE_UUID_PROVISIONING_K1_VAL EMBENET_BLE_UUID_ENCODE(0x0101)
#define EMBENET_BLE_UUID_PROVISIONING_PSK_VAL EMBENET_BLE_UUID_ENCODE(0x0102)
#define EMBENET_BLE_UUID_PROVISIONING_ACTIVE_VAL EMBENET_BLE_UUID_ENCODE(0x0103)
/// @}

/// @name embeNET Info service
/// @{
#define EMBENET_BLE_UUID_INFO_SERVICE_VAL EMBENET_BLE_UUID_ENCODE(0x0200)
#define EMBENET_BLE_UUID_INFO_EUI64_VAL EMBENET_BLE_UUID_ENCODE(0x0201)
#define EMBENET_BLE_UUID_INFO_STACK_VERSION_VAL EMBENET_BLE_UUID_ENCODE(0x0202)
#define EMBENET_BLE_UUID_INFO_GROUPS_VAL EMBENET_BLE_UUID_ENCODE(0x0203)
#define EMBENET_BLE_UUID_INFO_NEIGHBORS_COUNT_VAL EMBENET_BLE_UUID_ENCODE(0x0204)
#define EMBENET_BLE_UUID_INFO_NEIGHBOR_SELECT_VAL EMBENET_BLE_UUID_ENCODE(0x0205)
#define EMBENET_BLE_UUID_INFO_NEIGHBOR_INFO_VAL EMBENET_BLE_UUID_ENCODE(0x0206)
#define EMBENET_BLE_UUID_INFO_PARENT_EUI64_VAL EMBENET_BLE_UUID_ENCODE(0x0207)
#define EMBENET_BLE_UUID_INFO_PARENT_RSSI_VAL EMBENET_BLE_UUID_ENCODE(0x0208)
#define EMBENET_BLE_UUID_INFO_DUTY_CYCLE_VAL EMBENET_BLE_UUID_ENCODE(0x0209)
#define EMBENET_BLE_UUID_INFO_CONNECTION_PARAMETERS_VAL EMBENET_BLE_UUID_ENCODE(0x020A)
/// @}

/**
 * Enables Bluetooth, if not already enabled, and starts connectable advertising of the Provisioning service UUID with
 * CONFIG_BT_DEVICE_NAME in the scan response. Advertising resumes by itself after a central disconnects.
 *
 * The services are registered statically; they serve requests as soon as Bluetooth is enabled, whether or not
 * embenet_init() has been called yet.
 *
 * @return 0 when advertising, a negative error code from bt_enable() or bt_le_adv_start() otherwise.
 *
 * @note Call from a thread: bt_enable() waits for the controller.
 */
int embenet_ble_start(void);

#ifdef __cplusplus
}
#endif

#endif // EMBENET_BLE_H_
