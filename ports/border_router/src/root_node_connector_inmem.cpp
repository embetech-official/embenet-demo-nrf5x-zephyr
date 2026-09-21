/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     Root node connector over the shared in-memory queue (all-in-one builds)
 *
 * All-in-one counterpart of root_node_connector_uart.cpp: the root node lives in this same image, so
 * frames move through the queues in ports/common instead of over a wire. The key is not interpreted
 * here.
 *
 * The queue preserves frame boundaries, so this connector satisfies the MESSAGE contract - one
 * complete frame per write and per data-received callback. Framing stays the library's business.
 */

#if __has_include(<embenet_port/root_node_connector.h>)
#include <embenet_port/root_node_connector.h>
#endif

#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Zephyr's minimal C++ library has no <cstring> - use the plain C header (and unqualified strncpy() etc.)
#include <string.h>

#include <cstddef>
#include <cstdint>

LOG_MODULE_REGISTER(br_rnc, CONFIG_EMBENET_LOG_LEVEL);

namespace {

constexpr std::size_t max_connectors = 1; // one root node per image
constexpr std::size_t max_connector_name_length = 31;

} // namespace

extern "C" struct EMBENET_BR_PORT_RootNodeConnector {
  bool in_use = false;
  bool up_state = false;
  char name[max_connector_name_length + 1] = {};
  void (*data_callback)(void *context, void const *data, size_t data_length) = nullptr;
  void *data_context = nullptr;
  void (*link_callback)(void *context, bool is_up) = nullptr;
  void *link_context = nullptr;
};

namespace {

EMBENET_BR_PORT_RootNodeConnector connector_pool[max_connectors];
k_spinlock connector_pool_lock;

char const *printable_name(EMBENET_BR_PORT_RootNodeConnector const *connector) { return (connector != nullptr) ? connector->name : "null"; }

// No thread of its own any more: the application's border router thread calls
// EMBENET_PORT_COMMON_BrFrameProc(), which pops from the queue and lands here. That keeps the
// library's data callback on the same thread as EMBENET_BR_Proc() and out of interrupt context,
// which is all the library asks for.
void on_br_frame(void *context, void const *data, size_t length) {
  auto *connector = static_cast<EMBENET_BR_PORT_RootNodeConnector *>(context);
  // Frames arriving while the link is down belong to a session that no longer exists. They are
  // dropped here rather than left queued, which is what keeps the queue from filling up behind us.
  if(connector->up_state && (connector->data_callback != nullptr)) {
    connector->data_callback(connector->data_context, data, length);
    LOG_DBG("on_br_frame(%s) received %zu bytes", printable_name(connector), length);
    LOG_HEXDUMP_DBG(data, length, "RX");
  }
}

} // namespace

extern "C" {

EMBENET_BR_PORT_RootNodeConnector *EMBENET_BR_PORT_RootNodeConnectorCreate(char const *key) {
  LOG_DBG("create(\"%s\")", (key != nullptr) ? key : "null");

  if(key == nullptr) {
    return nullptr;
  }

  EMBENET_BR_PORT_RootNodeConnector *connector = nullptr;
  k_spinlock_key_t const lock_key = k_spin_lock(&connector_pool_lock);
  for(auto &slot : connector_pool) {
    if(!slot.in_use) {
      slot.in_use = true;
      connector = &slot;
      break;
    }
  }
  k_spin_unlock(&connector_pool_lock, lock_key);
  if(connector == nullptr) {
    return nullptr;
  }

  strncpy(connector->name, key, sizeof(connector->name) - 1);

  EMBENET_PORT_COMMON_ResetQueues();

  EMBENET_PORT_COMMON_SetBrFrameHandler(on_br_frame, connector);

  return connector;
}

void EMBENET_BR_PORT_RootNodeConnectorDestroy(EMBENET_BR_PORT_RootNodeConnector *connector) {
  LOG_DBG("destroy(%s)", printable_name(connector));
  // Before the reset below, or a frame delivered in between would reach a cleared connector.
  EMBENET_PORT_COMMON_SetBrFrameHandler(nullptr, nullptr);
  k_spinlock_key_t const lock_key = k_spin_lock(&connector_pool_lock);
  *connector = EMBENET_BR_PORT_RootNodeConnector{};
  k_spin_unlock(&connector_pool_lock, lock_key);
}

char const *EMBENET_BR_PORT_RootNodeConnectorGetName(EMBENET_BR_PORT_RootNodeConnector const *connector) { return printable_name(connector); }

void EMBENET_BR_PORT_RootNodeConnectorSetDataReceivedCallback(EMBENET_BR_PORT_RootNodeConnector *connector,
                                                              void (*callback)(void *context, void const *data, size_t data_length), void *context) {
  connector->data_callback = callback;
  connector->data_context = context;
}

void EMBENET_BR_PORT_RootNodeConnectorSetLinkStateChangedCallback(EMBENET_BR_PORT_RootNodeConnector *connector,
                                                                  void (*callback)(void *context, bool is_up), void *context) {
  connector->link_callback = callback;
  connector->link_context = context;
}

bool EMBENET_BR_PORT_RootNodeConnectorUp(EMBENET_BR_PORT_RootNodeConnector *connector) {
  bool const was_up = connector->up_state;
  // The peer is code in this same image, so the link is up as soon as it is asked to be.
  connector->up_state = true;
  LOG_DBG("up(%s)", printable_name(connector));
  if(!was_up && (connector->link_callback != nullptr)) {
    connector->link_callback(connector->link_context, true);
  }
  return true;
}

void EMBENET_BR_PORT_RootNodeConnectorDown(EMBENET_BR_PORT_RootNodeConnector *connector) {
  LOG_DBG("down(%s)", printable_name(connector));
  bool const was_up = connector->up_state;
  connector->up_state = false;
  if(was_up && (connector->link_callback != nullptr)) {
    connector->link_callback(connector->link_context, false);
  }
}

bool EMBENET_BR_PORT_RootNodeConnectorWrite(EMBENET_BR_PORT_RootNodeConnector *connector, void const *data, size_t data_length) {
  LOG_DBG("write(%s, %zu)", printable_name(connector), data_length);
  if(!connector->up_state) {
    return false;
  }
  return EMBENET_PORT_COMMON_RootFramePush(data, data_length);
}
}
