/**
 * @file
 * @brief embeNET Node with Bluetooth LE - the node sample's network behaviour, with a radio shared with a Bluetooth LE
 * peripheral that provisions the node and reports its place in the network.
 *
 * The stack runs under the embeNET middleware (embenet.h), which owns its thread and its life cycle. The network
 * credentials and the active switch live in Zephyr settings, under "embenet/", which the middleware reads itself; over
 * Bluetooth, the embeNET Provisioning service (embenet_ble.h) writes them.
 */

#include "embenet.h"
#include "embenet_ble.h"

#include <blinky_service_node.h>
#include <time_report.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <inttypes.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/**
 * @brief Logs where the node sits in the network, through the middleware's getters.
 */
static void report_topology(void) {
  uint16_t const duty_cycle = embenet_get_radio_duty_cycle();
  LOG_INF("topology: uid %016" PRIx64 " parent %016" PRIx64 " rssi %d neighbors %u duty cycle %u.%02u%% | %s", embenet_get_eui64(),
          embenet_get_parent_eui64(), (int)embenet_get_parent_rssi(), (unsigned)embenet_get_neighbors_count(), duty_cycle / 100U, duty_cycle % 100U,
          embenet_is_joined() ? "joined" : "not joined");
}

/**
 * @brief Runs the blinky service and the time report along with the stack. Called on the middleware's thread.
 */
static void on_embenet_event(void *context, embenet_event_t event) {
  ARG_UNUSED(context);
  switch(event) {
  case EMBENET_EVENT_INITIALIZED:
    (void)blinky_service_node_init(NULL);
    (void)time_report_init();
    break;
  case EMBENET_EVENT_JOINED:
    blinky_service_node_on_joined();
    time_report_on_joined();
    break;
  case EMBENET_EVENT_LEFT:
    blinky_service_node_on_left();
    time_report_on_left();
    break;
  case EMBENET_EVENT_DEINITIALIZED:
    break;
  }
}

static void on_embenet_proc(void *context) {
  ARG_UNUSED(context);
  blinky_service_node_proc();
}

/**
 * @brief Starts Bluetooth and the node, then reports every 10 s.
 * @return 0 if the node could not be started; otherwise never returns.
 */
int main(void) {
  LOG_INF("Starting embeNET Node sample");

  // The other protocol on the radio. Order against the mesh does not matter - MPSL, which arbitrates between the two,
  // is up before main() runs. It goes first, before the middleware loads its settings, since Bluetooth may keep
  // settings of its own.
  (void)embenet_ble_start();

  // Loads K1, PSK and the active switch from the settings and starts the stack if active.
  int const err = embenet_init((embenet_cb_t){.event_cb = on_embenet_event, .proc_cb = on_embenet_proc, .cb_ctx = NULL});
  if(0 != err) {
    LOG_ERR("embenet_init() failed: %d", err);
    return 0;
  }

  for(;;) {
    k_sleep(K_SECONDS(10));
    report_topology();
  }
}
