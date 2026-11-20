/**
 * @file
 * @brief Blinky service, border router side - see blinky_service_router.h.
 */

#include <blinky_service_router.h>

#include "blinky_io.h"
#include "blinky_service_common.h"

#include <embenet/border_router.h>

#include <zephyr/logging/log.h>

#include <errno.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(blinky_router, LOG_LEVEL_INF);

static EMBENET_UDP_SocketDescriptor socket = {
    .port = DEMO_BLINKY_UDP_PORT,
    .rxDataHandler = blinky_service_on_datagram,
    .groupId = DEMO_BLINKY_GROUP,
    .userContext = NULL,
    .handledTraffic = EMBENET_UDP_TRAFFIC_ALL,
};

static bool running;

/**
 * Multicasts a message to the service group.
 * @return true if the border router took it.
 */
static bool multicast(uint8_t const msg[BLINKY_SERVICE_MSG_SIZE]) {
  EMBENET_IPV6 const dst = EMBENET_AssembleMulticastIpv6(EMBENET_BR_GetNetworkPrefix(), DEMO_BLINKY_GROUP);
  return 0 == EMBENET_BR_Send(&socket, &dst, socket.port, msg, BLINKY_SERVICE_MSG_SIZE);
}

int blinky_service_router_init(void) {
  if(running) {
    return -EALREADY;
  }
  int const err = blinky_io_init(NULL); // polled from the border router loop; no wakeup needed
  if(0 != err) {
    return err;
  }

  EMBENET_BR_Result const result = EMBENET_BR_RegisterSocket(&socket);
  if(0 != result) {
    LOG_ERR("registering UDP port %u failed: %d", (unsigned)socket.port, result);
    blinky_io_deinit();
    return -EIO;
  }
  running = true;
  LOG_INF("running: UDP port %u, group %u", (unsigned)socket.port, (unsigned)DEMO_BLINKY_GROUP);
  return 0;
}

void blinky_service_router_deinit(void) {
  if(!running) {
    return;
  }
  running = false;
  (void)EMBENET_BR_UnregisterSocket(&socket);
  blinky_io_deinit();
  LOG_INF("stopped");
}

void blinky_service_router_proc(void) {
  for(uint8_t button = 1U; button <= BLINKY_IO_COUNT; ++button) {
    // Reads false for every button while not running: the latches are cleared and the interrupts off.
    if(!blinky_io_button_pressed(button)) {
      continue;
    }
    uint8_t msg[BLINKY_SERVICE_MSG_SIZE];
    blinky_service_button_msg(button, msg);
    if(multicast(msg)) {
      LOG_INF("button %u: LED%u %s multicast to group %u", button, msg[0], (1U == msg[1]) ? "on" : "off", (unsigned)DEMO_BLINKY_GROUP);
    } else {
      LOG_WRN("button %u: multicast not sent", button);
    }
  }
}
