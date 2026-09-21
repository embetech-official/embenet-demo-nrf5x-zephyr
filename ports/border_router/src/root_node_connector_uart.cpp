/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     embeNET Border Router Zephyr port - UART root node connector implementation.
 */

#if __has_include(<embenet_port/root_node_connector.h>)
#include <embenet_port/root_node_connector.h>
#endif

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

// Zephyr's minimal C++ library has no <cstring> - use the plain C header (and unqualified strncpy() etc.)
#include <string.h>

#include <cstddef>

LOG_MODULE_REGISTER(embenet_br_rn, CONFIG_EMBENET_LOG_LEVEL);

// The root node UART is looked up exclusively via a devicetree alias, never a hardcoded peripheral name.
// A board/app enables this backend simply by adding, e.g.:
//   / { aliases { embenet-root-node-uart = &usart2; }; };
// to its devicetree overlay. If no such alias exists, DT_NODE_EXISTS below is false at compile time and
// every EMBENET_BR_PORT_RootNodeConnectorCreate() call fails.
#define EMBENET_ROOT_NODE_UART_ALIAS embenet_root_node_uart

namespace {

constexpr std::size_t max_connectors = 1; // one physical root node UART
constexpr std::size_t max_connector_name_length = 31;
constexpr std::size_t rx_ring_buffer_size = 256;

#if DT_NODE_EXISTS(DT_ALIAS(EMBENET_ROOT_NODE_UART_ALIAS))
device const *const root_node_uart_device = DEVICE_DT_GET(DT_ALIAS(EMBENET_ROOT_NODE_UART_ALIAS));
constexpr bool root_node_uart_device_present = true;
#else
device const *const root_node_uart_device = nullptr;
constexpr bool root_node_uart_device_present = false;
#endif

} // namespace

extern "C" struct EMBENET_BR_PORT_RootNodeConnector {
  bool in_use = false;
  bool up_state = false;
  char name[max_connector_name_length + 1] = {};
  void (*data_callback)(void *context, void const *data, size_t data_length) = nullptr;
  void *data_context = nullptr;
  void (*link_callback)(void *context, bool is_up) = nullptr;
  void *link_context = nullptr;
  ring_buf rx_rb{};
  uint8_t rx_storage[rx_ring_buffer_size];
  k_work rx_work{};
};

namespace {

EMBENET_BR_PORT_RootNodeConnector connector_pool[max_connectors];
k_spinlock connector_pool_lock;

char const *printable_name(EMBENET_BR_PORT_RootNodeConnector const *connector) { return (connector != nullptr) ? connector->name : "null"; }

// Runs in the system workqueue thread (not interrupt context) - safe to call the arbitrary user callback
// from here, unlike from uart_irq_handler() itself.
void rx_work_handler(k_work *work) {
  auto *connector = CONTAINER_OF(work, EMBENET_BR_PORT_RootNodeConnector, rx_work);
  uint8_t chunk[32];
  for(;;) {
    uint32_t const bytes_read = ring_buf_get(&connector->rx_rb, chunk, sizeof(chunk));
    if(bytes_read == 0) {
      break;
    }
    if(connector->data_callback != nullptr) {
      connector->data_callback(connector->data_context, chunk, bytes_read);
    }
  }
}

void uart_irq_handler(device const *dev, void *user_data) {
  auto *connector = static_cast<EMBENET_BR_PORT_RootNodeConnector *>(user_data);
  if(uart_irq_update(dev) == 0) {
    return;
  }
  if(uart_irq_rx_ready(dev) != 0) {
    uint8_t byte = 0;
    while(uart_fifo_read(dev, &byte, 1) == 1) {
      // ISR-safe single-producer put; a full ring buffer drops the byte rather than blocking the ISR.
      (void)ring_buf_put(&connector->rx_rb, &byte, 1);
    }
    k_work_submit(&connector->rx_work);
  }
}

} // namespace

extern "C" {

EMBENET_BR_PORT_RootNodeConnector *EMBENET_BR_PORT_RootNodeConnectorCreate(char const *key) {
  LOG_DBG("create(\"%s\")", (key != nullptr) ? key : "null");

  if((key == nullptr) || !root_node_uart_device_present) {
    LOG_ERR("no '" STRINGIFY(EMBENET_ROOT_NODE_UART_ALIAS) "' devicetree alias defined");
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
  ring_buf_init(&connector->rx_rb, sizeof(connector->rx_storage), connector->rx_storage);
  k_work_init(&connector->rx_work, rx_work_handler);
  uart_irq_callback_user_data_set(root_node_uart_device, uart_irq_handler, connector);

  return connector;
}

void EMBENET_BR_PORT_RootNodeConnectorDestroy(EMBENET_BR_PORT_RootNodeConnector *connector) {
  LOG_DBG("destroy(%s)", printable_name(connector));
  uart_irq_rx_disable(root_node_uart_device);
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
  connector->up_state = device_is_ready(root_node_uart_device);
  if(connector->up_state) {
    uart_irq_rx_enable(root_node_uart_device);
  }
  LOG_DBG("up(%s) -> %d", printable_name(connector), connector->up_state);
  if((connector->up_state != was_up) && (connector->link_callback != nullptr)) {
    connector->link_callback(connector->link_context, connector->up_state);
  }
  return connector->up_state;
}

void EMBENET_BR_PORT_RootNodeConnectorDown(EMBENET_BR_PORT_RootNodeConnector *connector) {
  LOG_DBG("down(%s)", printable_name(connector));
  uart_irq_rx_disable(root_node_uart_device);
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
  auto const *bytes = static_cast<uint8_t const *>(data);
  for(std::size_t i = 0; i < data_length; ++i) {
    uart_poll_out(root_node_uart_device, bytes[i]);
  }
  return true;
}
}
