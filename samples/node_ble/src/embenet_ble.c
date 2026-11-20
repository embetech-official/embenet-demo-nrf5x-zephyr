/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Bluetooth LE peripheral
 * @brief     Advertising and connection handling for the embeNET GATT services.
 *
 * The advertisement carries the Provisioning service UUID, which is what a central filters on to find embeNET nodes.
 * A 128-bit UUID and the name do not fit in one 31-byte advertisement together, so the name goes in the scan response.
 */

#include "embenet_ble.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <inttypes.h>

LOG_MODULE_REGISTER(embenet_ble, CONFIG_EMBENET_LOG_LEVEL);

BUILD_ASSERT(CONFIG_EMBENET_BLE_ADV_INTERVAL_MIN_MS <= CONFIG_EMBENET_BLE_ADV_INTERVAL_MAX_MS, "advertising interval range is inverted");

#if defined(CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS)
// The rule bt_le_conn_params_valid() applies before sending the connection parameter update request, in the units
// Kconfig uses (1.25 ms intervals, 10 ms timeout). Broken, the request is never sent and nothing says so.
BUILD_ASSERT(4 * CONFIG_BT_PERIPHERAL_PREF_TIMEOUT > (1 + CONFIG_BT_PERIPHERAL_PREF_LATENCY) * CONFIG_BT_PERIPHERAL_PREF_MAX_INT,
             "supervision timeout must exceed (1 + latency) x max interval x 2");
#endif

static struct bt_data const advertisement[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, EMBENET_BLE_UUID_PROVISIONING_SERVICE_VAL),
};

static struct bt_data const scan_response[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1U),
};

static struct bt_le_adv_param const advertising_parameters =
    BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONN, BT_GAP_MS_TO_ADV_INTERVAL(CONFIG_EMBENET_BLE_ADV_INTERVAL_MIN_MS),
                         BT_GAP_MS_TO_ADV_INTERVAL(CONFIG_EMBENET_BLE_ADV_INTERVAL_MAX_MS), NULL);

static int start_advertising(void) {
  int const err = bt_le_adv_start(&advertising_parameters, advertisement, ARRAY_SIZE(advertisement), scan_response, ARRAY_SIZE(scan_response));
  if((0 != err) && (-EALREADY != err)) {
    LOG_ERR("bt_le_adv_start() failed: %d", err);
    return err;
  }
  LOG_INF("advertising as \"%s\"", CONFIG_BT_DEVICE_NAME);
  return 0;
}

static void restart_advertising(struct k_work *work) {
  ARG_UNUSED(work);
  (void)start_advertising();
}

/// Connectable advertising stops when a central connects. It is restarted once the connection object is recycled -
/// before that the controller may have no room for a new connection - and from a work item, off the Bluetooth thread.
static K_WORK_DEFINE(advertising_restart, restart_advertising);

static void on_connected(struct bt_conn *conn, uint8_t err) {
  if(0U != err) {
    LOG_WRN("connection failed: 0x%02x", err);
    return;
  }
  // The interval is what the mesh has to fit its timeslots around, so it is worth knowing.
  struct bt_conn_info info;
  if(0 == bt_conn_get_info(conn, &info)) {
    LOG_INF("central connected, interval %" PRIu32 " us", info.le.interval_us);
  }
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason) {
  ARG_UNUSED(conn);
  LOG_INF("central disconnected: 0x%02x", reason);
}

static void on_recycled(void) { k_work_submit(&advertising_restart); }

static void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout) {
  ARG_UNUSED(conn);
  LOG_INF("connection interval now %u us, latency %u, timeout %u ms", BT_CONN_INTERVAL_TO_US(interval), latency, timeout * 10U);
}

BT_CONN_CB_DEFINE(embenet_ble_conn_callbacks) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .recycled = on_recycled,
    .le_param_updated = on_le_param_updated,
};

int embenet_ble_start(void) {
  int const err = bt_enable(NULL);
  if((0 != err) && (-EALREADY != err)) {
    LOG_ERR("bt_enable() failed: %d", err);
    return err;
  }
  return start_advertising();
}
