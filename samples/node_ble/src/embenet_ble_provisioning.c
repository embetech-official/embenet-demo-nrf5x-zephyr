/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Bluetooth LE peripheral
 * @brief     embeNET Provisioning GATT service: K1, PSK and the active switch.
 *
 * Writes go straight to Zephyr settings: settings_save_one() puts the value in non-volatile memory, then
 * settings_runtime_set() hands it to the middleware's settings handler, which applies it once it has settled (see
 * embenet_init()). Reads of the active switch come from the settings too, through settings_runtime_get(). The keys are
 * write-only, so they cannot be read back over the air; the active switch is read-write.
 *
 * Runs on the Bluetooth RX thread, which the flash write holds up for its duration - about 50 ms on nRF52 flash next
 * to the mesh, much less on nRF54L RRAM.
 */

#include "embenet.h"
#include "embenet_ble.h"

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(embenet_ble_provisioning, CONFIG_EMBENET_LOG_LEVEL);

static struct bt_uuid_128 const service_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_PROVISIONING_SERVICE_VAL);
static struct bt_uuid_128 const k1_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_PROVISIONING_K1_VAL);
static struct bt_uuid_128 const psk_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_PROVISIONING_PSK_VAL);
static struct bt_uuid_128 const active_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_PROVISIONING_ACTIVE_VAL);

/// Checks a write that must replace a whole value of @p size bytes in one go.
static ssize_t check_whole_write(uint16_t len, uint16_t offset, uint8_t flags, size_t size) {
  if(0U != (flags & BT_GATT_WRITE_FLAG_PREPARE)) {
    return BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_LONG);
  }
  if(0U != offset) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  if(size != len) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }
  return 0;
}

/// Stores @p size bytes under settings @p key and hands them to the middleware; an ATT error if either fails.
static ssize_t store(char const *key, void const *value, size_t size) {
  int err = settings_save_one(key, value, size);
  if(0 == err) {
    err = settings_runtime_set(key, value, size);
  }
  if(0 != err) {
    LOG_ERR("storing %s failed: %d", key, err);
    return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
  }
  return 0;
}

static ssize_t write_k1(struct bt_conn *conn, struct bt_gatt_attr const *attr, void const *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  ARG_UNUSED(conn);
  ARG_UNUSED(attr);
  ssize_t const err = check_whole_write(len, offset, flags, EMBENET_KEY_SIZE);
  if(0 != err) {
    return err;
  }
  ssize_t const stored = store(EMBENET_SETTINGS_KEY_K1, buf, EMBENET_KEY_SIZE);
  if(0 != stored) {
    return stored;
  }
  LOG_INF("K1 written");
  return len;
}

static ssize_t write_psk(struct bt_conn *conn, struct bt_gatt_attr const *attr, void const *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  ARG_UNUSED(conn);
  ARG_UNUSED(attr);
  ssize_t const err = check_whole_write(len, offset, flags, EMBENET_KEY_SIZE);
  if(0 != err) {
    return err;
  }
  ssize_t const stored = store(EMBENET_SETTINGS_KEY_PSK, buf, EMBENET_KEY_SIZE);
  if(0 != stored) {
    return stored;
  }
  LOG_INF("PSK written");
  return len;
}

static ssize_t read_active(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  uint8_t value;
  if(sizeof(value) != settings_runtime_get(EMBENET_SETTINGS_KEY_ACTIVE, &value, sizeof(value))) {
    return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
  }
  return bt_gatt_attr_read(conn, attr, buf, len, offset, &value, sizeof(value));
}

static ssize_t write_active(struct bt_conn *conn, struct bt_gatt_attr const *attr, void const *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  ARG_UNUSED(conn);
  ARG_UNUSED(attr);
  ssize_t const err = check_whole_write(len, offset, flags, sizeof(uint8_t));
  if(0 != err) {
    return err;
  }
  uint8_t const value = *(uint8_t const *)buf;
  if(value > 1U) {
    return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
  }
  ssize_t const stored = store(EMBENET_SETTINGS_KEY_ACTIVE, &value, sizeof(value));
  if(0 != stored) {
    return stored;
  }
  LOG_INF("active switch written: %u", value);
  return len;
}

BT_GATT_SERVICE_DEFINE(embenet_ble_provisioning_service, BT_GATT_PRIMARY_SERVICE(&service_uuid),
                       BT_GATT_CHARACTERISTIC(&k1_uuid.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_k1, NULL),
                       BT_GATT_CHARACTERISTIC(&psk_uuid.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_psk, NULL),
                       BT_GATT_CHARACTERISTIC(&active_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                                              read_active, write_active, NULL));
