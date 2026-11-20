/**
 * @file
 * @brief Blinky service protocol - see blinky_service_common.h.
 */

#include "blinky_service_common.h"
#include "blinky_io.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <inttypes.h>

LOG_MODULE_REGISTER(blinky_service, LOG_LEVEL_INF);

void blinky_service_button_msg(uint8_t button, uint8_t msg[BLINKY_SERVICE_MSG_SIZE]) {
  msg[0] = (uint8_t)((button + 1U) / 2U); // 1, 2 -> LED1; 3, 4 -> LED2
  msg[1] = button % 2U;                   // odd buttons turn on, even ones off
}

void blinky_service_on_datagram(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *source_address, uint16_t source_port,
                                void const *data, size_t data_size) {
  ARG_UNUSED(socket);
  ARG_UNUSED(source_port);
  // The interface identifier, the address's last 8 bytes, names the sender.
  uint64_t const sender = sys_get_be64(&source_address->value[8]);

  if(BLINKY_SERVICE_MSG_SIZE != data_size) {
    LOG_WRN("from %016" PRIx64 ": %zu-byte datagram dropped, %u bytes expected", sender, data_size, BLINKY_SERVICE_MSG_SIZE);
    return;
  }
  uint8_t const led = ((uint8_t const *)data)[0];
  uint8_t const state = ((uint8_t const *)data)[1];

  if((led >= 1U) && (led <= BLINKY_SERVICE_LED_COUNT) && (state <= 1U)) {
    LOG_INF("from %016" PRIx64 ": LED%u %s", sender, led, (1U == state) ? "on" : "off");
    blinky_io_led_set(led, 1U == state);
  } else {
    LOG_WRN("from %016" PRIx64 ": unknown message {%u, %u} dropped", sender, led, state);
  }
}
