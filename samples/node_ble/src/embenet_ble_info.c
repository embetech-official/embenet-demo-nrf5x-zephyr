/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Bluetooth LE peripheral
 * @brief     embeNET Info GATT service: read-only view of the node's identity and place in the network.
 *
 * Every value comes from the middleware's getters at the time of the read and is little-endian on the air. Groups can
 * be longer than the ATT MTU allows and is then read in parts (Read Blob); the part at offset 0 takes a snapshot and the
 * following parts are served from it, so a long read never mixes two states of the list.
 *
 * Neighbors are read one at a time: a central writes the order number of a neighbor, 0 .. Neighbors Count - 1, to
 * Neighbor Select, then reads that neighbor's record from Neighbor Info. Each connection has a selection of its own,
 * 0 when it connects.
 */

#include "embenet.h"
#include "embenet_ble.h"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

/// Bytes of the Neighbor Info characteristic: EUI-64, RSSI, role.
#define NEIGHBOR_RECORD_SIZE 10U

static struct bt_uuid_128 const service_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_SERVICE_VAL);
static struct bt_uuid_128 const eui64_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_EUI64_VAL);
static struct bt_uuid_128 const stack_version_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_STACK_VERSION_VAL);
static struct bt_uuid_128 const groups_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_GROUPS_VAL);
static struct bt_uuid_128 const neighbors_count_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_NEIGHBORS_COUNT_VAL);
static struct bt_uuid_128 const neighbor_select_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_NEIGHBOR_SELECT_VAL);
static struct bt_uuid_128 const neighbor_info_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_NEIGHBOR_INFO_VAL);
static struct bt_uuid_128 const parent_eui64_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_PARENT_EUI64_VAL);
static struct bt_uuid_128 const parent_rssi_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_PARENT_RSSI_VAL);
static struct bt_uuid_128 const duty_cycle_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_DUTY_CYCLE_VAL);
static struct bt_uuid_128 const connection_parameters_uuid = BT_UUID_INIT_128(EMBENET_BLE_UUID_INFO_CONNECTION_PARAMETERS_VAL);

/// Snapshot for long reads of Groups. GATT handlers all run on the Bluetooth RX thread, so the state here needs no lock.
static struct {
  uint8_t data[CONFIG_EMBENET_BLE_INFO_MAX_GROUPS * sizeof(uint16_t)];
  size_t size;
} groups_snapshot;

/// Neighbor Select, per connection.
static uint16_t neighbor_selected[CONFIG_BT_MAX_CONN];

static ssize_t read_u64(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset, uint64_t value) {
  uint8_t le[sizeof(uint64_t)];
  sys_put_le64(value, le);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, le, sizeof(le));
}

static ssize_t read_u16(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset, uint16_t value) {
  uint8_t le[sizeof(uint16_t)];
  sys_put_le16(value, le);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, le, sizeof(le));
}

static ssize_t read_eui64(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  return read_u64(conn, attr, buf, len, offset, embenet_get_eui64());
}

static ssize_t read_stack_version(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  char const *const version = embenet_get_stack_version_string();
  return bt_gatt_attr_read(conn, attr, buf, len, offset, version, strlen(version));
}

static ssize_t read_groups(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  if(0U == offset) {
    uint16_t groups[CONFIG_EMBENET_BLE_INFO_MAX_GROUPS];
    size_t const count = embenet_get_groups(groups, ARRAY_SIZE(groups));
    for(size_t i = 0; i < count; ++i) {
      sys_put_le16(groups[i], &groups_snapshot.data[i * sizeof(uint16_t)]);
    }
    groups_snapshot.size = count * sizeof(uint16_t);
  }
  return bt_gatt_attr_read(conn, attr, buf, len, offset, groups_snapshot.data, groups_snapshot.size);
}

static ssize_t read_neighbors_count(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  return read_u16(conn, attr, buf, len, offset, embenet_get_neighbors_count());
}

static ssize_t read_neighbor_select(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  return read_u16(conn, attr, buf, len, offset, neighbor_selected[bt_conn_index(conn)]);
}

static ssize_t write_neighbor_select(struct bt_conn *conn, struct bt_gatt_attr const *attr, void const *buf, uint16_t len, uint16_t offset,
                                     uint8_t flags) {
  ARG_UNUSED(attr);
  if(0U != (flags & BT_GATT_WRITE_FLAG_PREPARE)) {
    return BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_LONG);
  }
  if(0U != offset) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  if(sizeof(uint16_t) != len) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
  }
  uint16_t const index = sys_get_le16(buf);
  if(index >= embenet_get_neighbors_count()) {
    return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
  }
  neighbor_selected[bt_conn_index(conn)] = index;
  return len;
}

static ssize_t read_neighbor_info(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  embenet_neighbor_t neighbor;
  // The table may have shrunk since the selection was written, or the stack stopped.
  if(!embenet_get_neighbor(neighbor_selected[bt_conn_index(conn)], &neighbor)) {
    return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
  }
  uint8_t record[NEIGHBOR_RECORD_SIZE];
  sys_put_le64(neighbor.eui, &record[0]);
  record[8] = (uint8_t)neighbor.rssi;
  record[9] = (uint8_t)neighbor.role;
  return bt_gatt_attr_read(conn, attr, buf, len, offset, record, sizeof(record));
}

static ssize_t read_parent_eui64(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  return read_u64(conn, attr, buf, len, offset, embenet_get_parent_eui64());
}

static ssize_t read_parent_rssi(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  int8_t const rssi = embenet_get_parent_rssi();
  return bt_gatt_attr_read(conn, attr, buf, len, offset, &rssi, sizeof(rssi));
}

static ssize_t read_duty_cycle(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  return read_u16(conn, attr, buf, len, offset, embenet_get_radio_duty_cycle());
}

/// The parameters of the connection the read arrives on - what a central cannot learn from its own Bluetooth stack
/// without privileges. 8 bytes: interval in us (uint32), peripheral latency in events (uint16), supervision timeout in
/// 10 ms units (uint16).
static ssize_t read_connection_parameters(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf, uint16_t len, uint16_t offset) {
  struct bt_conn_info info;
  if(0 != bt_conn_get_info(conn, &info)) {
    return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
  }
  uint8_t value[8];
  sys_put_le32(info.le.interval_us, &value[0]);
  sys_put_le16(info.le.latency, &value[4]);
  sys_put_le16(info.le.timeout, &value[6]);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

static void on_connected(struct bt_conn *conn, uint8_t err) {
  if(0U == err) {
    neighbor_selected[bt_conn_index(conn)] = 0U;
  }
}

BT_CONN_CB_DEFINE(embenet_ble_info_conn_callbacks) = {
    .connected = on_connected,
};

#define INFO_CHARACTERISTIC(uuid128, read) BT_GATT_CHARACTERISTIC(&(uuid128).uuid, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read, NULL, NULL)

BT_GATT_SERVICE_DEFINE(embenet_ble_info_service, BT_GATT_PRIMARY_SERVICE(&service_uuid), INFO_CHARACTERISTIC(eui64_uuid, read_eui64),
                       INFO_CHARACTERISTIC(stack_version_uuid, read_stack_version), INFO_CHARACTERISTIC(groups_uuid, read_groups),
                       INFO_CHARACTERISTIC(neighbors_count_uuid, read_neighbors_count),
                       BT_GATT_CHARACTERISTIC(&neighbor_select_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_neighbor_select, write_neighbor_select, NULL),
                       INFO_CHARACTERISTIC(neighbor_info_uuid, read_neighbor_info), INFO_CHARACTERISTIC(parent_eui64_uuid, read_parent_eui64),
                       INFO_CHARACTERISTIC(parent_rssi_uuid, read_parent_rssi), INFO_CHARACTERISTIC(duty_cycle_uuid, read_duty_cycle),
                       INFO_CHARACTERISTIC(connection_parameters_uuid, read_connection_parameters));
