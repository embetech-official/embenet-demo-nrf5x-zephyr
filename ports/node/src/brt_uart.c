/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Border router link over UART
 *
 * A raw byte pipe: framing and the RCP protocol live inside the stack, so Send/Receive and
 * SendRaw/ReceiveRaw are the same thing here.
 *
 * The UART comes from a devicetree alias:
 *   / { aliases { embenet-border-router-uart = &uart1; }; };
 */

#include <embenet_port/border_router_communication.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/toolchain.h>

#include <stdint.h>

LOG_MODULE_REGISTER(embenet_brt, CONFIG_EMBENET_LOG_LEVEL);

#define EMBENET_BORDER_ROUTER_UART_ALIAS embenet_border_router_uart

#if DT_NODE_EXISTS(DT_ALIAS(EMBENET_BORDER_ROUTER_UART_ALIAS))
static struct device const *const uart_device = DEVICE_DT_GET(DT_ALIAS(EMBENET_BORDER_ROUTER_UART_ALIAS));
#define UART_DEVICE_PRESENT 1
#else
static struct device const *const uart_device = NULL;
#define UART_DEVICE_PRESENT 0
#endif

RING_BUF_DECLARE(tx_ring_buffer, CONFIG_EMBENET_NODE_BRT_UART_TX_BUFFER_SIZE);
RING_BUF_DECLARE(rx_ring_buffer, CONFIG_EMBENET_NODE_BRT_UART_RX_BUFFER_SIZE);

static void uart_irq_handler(struct device const *dev, void *user_data) {
  ARG_UNUSED(user_data);

  if(uart_irq_update(dev) == 0) {
    return;
  }

  if(uart_irq_rx_ready(dev) != 0) {
    uint8_t byte = 0;
    while(uart_fifo_read(dev, &byte, 1) == 1) {
      // A full buffer drops the byte instead of stalling the ISR; the link layer tolerates loss.
      (void)ring_buf_put(&rx_ring_buffer, &byte, 1);
    }
  }

  if(uart_irq_tx_ready(dev) != 0) {
    uint8_t byte = 0;
    if(ring_buf_get(&tx_ring_buffer, &byte, 1) == 1) {
      (void)uart_fifo_fill(dev, &byte, 1);
    } else {
      uart_irq_tx_disable(dev);
    }
  }
}

void EMBENET_BRT_Init(void) {
  if(!UART_DEVICE_PRESENT) {
    LOG_ERR("no '" STRINGIFY(EMBENET_BORDER_ROUTER_UART_ALIAS) "' devicetree alias");
    return;
  }
  if(!device_is_ready(uart_device)) {
    LOG_ERR("%s is not ready", uart_device->name);
    return;
  }

  ring_buf_reset(&tx_ring_buffer);
  ring_buf_reset(&rx_ring_buffer);

  uart_irq_rx_disable(uart_device);
  uart_irq_tx_disable(uart_device);
  uart_irq_callback_user_data_set(uart_device, uart_irq_handler, NULL);
  uart_irq_rx_enable(uart_device);
}

void EMBENET_BRT_Deinit(void) {
  if(!UART_DEVICE_PRESENT) {
    return;
  }
  uart_irq_rx_disable(uart_device);
  uart_irq_tx_disable(uart_device);
  ring_buf_reset(&tx_ring_buffer);
  ring_buf_reset(&rx_ring_buffer);
}

void EMBENET_BRT_SendRaw(void const *data, size_t dataLength) {
  if(!UART_DEVICE_PRESENT || (data == NULL) || (dataLength == 0)) {
    return;
  }

  uint32_t const written = ring_buf_put(&tx_ring_buffer, data, dataLength);
  if(written != dataLength) {
    LOG_WRN("TX buffer full, dropped %u of %zu bytes", (unsigned)(dataLength - written), dataLength);
  }
  // The handler disables the TX interrupt once the buffer drains, so enabling it here is safe whether
  // or not a transmission is already running.
  if(written > 0) {
    uart_irq_tx_enable(uart_device);
  }
}

size_t EMBENET_BRT_ReceiveRaw(void *data, size_t dataBufferSize) {
  if((data == NULL) || (dataBufferSize == 0)) {
    return 0;
  }
  return ring_buf_get(&rx_ring_buffer, data, dataBufferSize);
}

void EMBENET_BRT_Send(void const *packet, size_t packetLength) { EMBENET_BRT_SendRaw(packet, packetLength); }

size_t EMBENET_BRT_Receive(void *packetBuffer, size_t packetBufferSize) { return EMBENET_BRT_ReceiveRaw(packetBuffer, packetBufferSize); }

void EMBENET_BRT_Reset(void) {
  LOG_WRN("reset requested by the stack");
  sys_reboot(SYS_REBOOT_COLD);
}

bool EMBENET_BRT_IsBusy(void) { return !ring_buf_is_empty(&tx_ring_buffer); }
