/**
 * @file
 * @brief embeNET Node sample - joins a network, blinks an LED on network time, reports to the border router every
 * second and links the DK's buttons and LEDs with the gateway's.
 */

#include <blinky_service_node.h>
#include <embenet_demo_network.h>
#include <time_report.h>

#include <embenet/node.h>

#include <embenet_port_common/embenet_port_common.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <inttypes.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/// @brief Credentials the node joins with. In a product these come from secure settings or a provisioned partition, not from the image.
static EMBENET_NODE_JoinConfig const join_config = {
    .k1 = {.val = {DEMO_K1_BYTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    .psk = {.val = {DEMO_PSK_BYTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

static void on_joined(EMBENET_PANID pan_id, EMBENET_NODE_QuickJoinCredentials const *quick_join_credentials) {
  ARG_UNUSED(quick_join_credentials);
  LOG_INF("joined network 0x%04" PRIx16, pan_id);
  blinky_service_node_on_joined();
  time_report_on_joined();
}

static void on_left(void) {
  LOG_WRN("left the network");
  blinky_service_node_on_left();
  time_report_on_left();
}

static void on_join_attempt(EMBENET_PANID pan_id, uint8_t const *pan_data, size_t pan_data_size) {
  bool const pan_data_is_c_string = pan_data != NULL && pan_data_size > 0 && pan_data[pan_data_size - 1] == '\0';

  LOG_INF("attempting to join network 0x%04" PRIx16 ", %zu bytes of PAN data: '%s'", pan_id, pan_data_size,
          pan_data_is_c_string ? (char const *)pan_data : "(not a C-string)");
}

static void on_data_on_unregistered_port(uint16_t port) { LOG_WRN("datagram on unregistered port %" PRIu16, port); }

/**
 * @brief Starts the node and its services, then runs the stack until the board is reset.
 * @return 0 if the stack failed to start; otherwise never returns.
 */
int main(void) {
  LOG_INF("Starting embeNET Node sample");
  LOG_INF("Using embeNET Node version: %s", EMBENET_NODE_GetVersionString());

  EMBENET_NODE_EventHandlers const handlers = {
      .onJoined = on_joined,
      .onLeft = on_left,
      .onJoinAttempt = on_join_attempt,
      .onQuickJoinCredentialsObsolete = NULL,
      .onDataOnUnregisteredPort = on_data_on_unregistered_port,
  };

  EMBENET_Result result = EMBENET_NODE_Init(&handlers, EMBENET_NODE_GetConfig_2400_2480Mhz_BLE_PHY());
  if(result != EMBENET_RESULT_OK) {
    LOG_ERR("EMBENET_NODE_Init() failed: %d", (int)result);
    return 0;
  }

  if((0 != blinky_service_node_init(NULL)) || (0 != time_report_init())) {
    return 0;
  }

  LOG_INF("UID %016" PRIx64, EMBENET_NODE_GetUID());

  result = EMBENET_NODE_Join(&join_config);
  if(result != EMBENET_RESULT_OK) {
    LOG_ERR("EMBENET_NODE_Join() failed: %d", (int)result);
    return 0;
  }

  // The stack runs only inside EMBENET_NODE_Proc(). The port gives the semaphore as soon as there is work; the
  // timeout also bounds how long a button press waits.
  for(;;) {
    k_sem_take(EMBENET_PORT_COMMON_GetProcSemaphore(), K_MSEC(5));
    EMBENET_NODE_Proc();
    blinky_service_node_proc();
  }
}
