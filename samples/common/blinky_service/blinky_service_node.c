/**
 * @file
 * @brief Blinky service, node side - see blinky_service_node.h.
 */

#include <blinky_service_node.h>

#include "blinky_io.h"
#include "blinky_service_common.h"

#include <embenet/node.h>
#include <embenet/node_udp.h>

#include <zephyr/logging/log.h>

#include <errno.h>
#include <inttypes.h>

LOG_MODULE_REGISTER(blinky_node, LOG_LEVEL_INF);

static EMBENET_UDP_SocketDescriptor socket = {
    .port = DEMO_BLINKY_UDP_PORT,
    .rxDataHandler = blinky_service_on_datagram,
    .groupId = DEMO_BLINKY_GROUP,
    .userContext = NULL,
    // The border router's unicast and its multicast to the group both arrive here.
    .handledTraffic = EMBENET_UDP_TRAFFIC_ALL,
};

static bool joined;
/// The LED4 blink task; created on the first join, valid until blinky_service_node_deinit().
static EMBENET_TaskId blink_task_id = EMBENET_TASKID_INVALID;

/**
 * Toggles LED4 every BLINKY_SERVICE_NODE_BLINK_HALF_PERIOD_MS of network time.
 *
 * The state is taken from the network time the task was scheduled for, not flipped from the previous one: every node
 * runs at the same multiples of the half period and computes the same state there, so all of them blink in phase,
 * whenever each one joined.
 *
 * @param t The network time it was scheduled for, in milliseconds - a multiple of the half period.
 */
static void blink_task(EMBENET_TaskId id, EMBENET_NODE_TimeSource time_source, uint64_t t, void *context) {
  ARG_UNUSED(context);
  // Rescheduled off t rather than off the current time, so the blink does not drift with the work below.
  (void)EMBENET_NODE_TaskSchedule(id, time_source, t + BLINKY_SERVICE_NODE_BLINK_HALF_PERIOD_MS);
  bool const on = 0U == ((t / BLINKY_SERVICE_NODE_BLINK_HALF_PERIOD_MS) % 2U);
  blinky_io_led_set(BLINKY_SERVICE_NODE_BLINK_LED, on);
  LOG_DBG("LED4 %s at network time %" PRIu64 ", ran %+" PRId64 " ms off schedule", on ? "on" : "off", t,
          (int64_t)(EMBENET_NODE_GetNetworkTime() - t));
}

/**
 * Sends a message to the border router.
 * @return true if the stack took it.
 */
static bool send_to_border_router(uint8_t const msg[BLINKY_SERVICE_MSG_SIZE]) {
  EMBENET_IPV6 dst;
  if(EMBENET_RESULT_OK != EMBENET_NODE_GetBorderRouterAddress(&dst)) {
    return false;
  }
  return EMBENET_UDP_RESULT_OK == EMBENET_NODE_Send(&socket, &dst, socket.port, msg, BLINKY_SERVICE_MSG_SIZE);
}

int blinky_service_node_init(blinky_service_node_wakeup_t wakeup) {
  int const err = blinky_io_init(wakeup);
  // -EALREADY: a new stack instance after one that went without deinit; the buttons and LEDs are fine as they are.
  if((0 != err) && (-EALREADY != err)) {
    return err;
  }
  joined = false;
  // A task of a stack instance torn down without blinky_service_node_deinit() went with it; on_joined creates a new one.
  blink_task_id = EMBENET_TASKID_INVALID;

  if(EMBENET_UDP_RESULT_OK != EMBENET_NODE_RegisterSocket(&socket)) {
    LOG_ERR("registering UDP port %u failed", (unsigned)socket.port);
    goto fail_socket;
  }
  if(!EMBENET_NODE_JoinGroup(DEMO_BLINKY_GROUP)) {
    LOG_ERR("joining group %u failed", (unsigned)DEMO_BLINKY_GROUP);
    goto fail_group;
  }
  LOG_INF("ready: UDP port %u, group %u", (unsigned)socket.port, (unsigned)DEMO_BLINKY_GROUP);
  return 0;

fail_group:
  (void)EMBENET_NODE_UnregisterSocket(&socket);
fail_socket:
  blinky_io_deinit();
  return -EIO;
}

void blinky_service_node_deinit(void) {
  joined = false;
  if(EMBENET_TASKID_INVALID != blink_task_id) {
    (void)EMBENET_NODE_TaskCancel(blink_task_id);
    (void)EMBENET_NODE_TaskDestroy(blink_task_id);
    blink_task_id = EMBENET_TASKID_INVALID;
  }
  (void)EMBENET_NODE_LeaveGroup(DEMO_BLINKY_GROUP);
  (void)EMBENET_NODE_UnregisterSocket(&socket);
  blinky_io_deinit();
}

void blinky_service_node_on_joined(void) {
  joined = true;
  // Presses made before the join were never meant for this network.
  for(uint8_t button = 1U; button <= BLINKY_IO_COUNT; ++button) {
    (void)blinky_io_button_pressed(button);
  }
  blinky_io_led_set(BLINKY_SERVICE_NODE_JOINED_LED, true);

  // Created once and kept across leave and rejoin; only the schedule is cancelled on leave.
  if(EMBENET_TASKID_INVALID == blink_task_id) {
    blink_task_id = EMBENET_NODE_TaskCreate(blink_task, NULL);
    if(EMBENET_TASKID_INVALID == blink_task_id) {
      LOG_ERR("creating the blink task failed");
      return;
    }
  }
  // First run on the next whole half period of network time, so every node runs on the same grid.
  uint64_t const now = EMBENET_NODE_GetNetworkTime();
  uint64_t const first = now - (now % BLINKY_SERVICE_NODE_BLINK_HALF_PERIOD_MS) + BLINKY_SERVICE_NODE_BLINK_HALF_PERIOD_MS;
  if(EMBENET_RESULT_OK != EMBENET_NODE_TaskSchedule(blink_task_id, EMBENET_NODE_TIME_SOURCE_NETWORK, first)) {
    LOG_ERR("scheduling the blink task failed");
  }
}

void blinky_service_node_on_left(void) {
  joined = false;
  blinky_io_led_set(BLINKY_SERVICE_NODE_JOINED_LED, false);
  if(EMBENET_TASKID_INVALID != blink_task_id) {
    (void)EMBENET_NODE_TaskCancel(blink_task_id);
  }
  blinky_io_led_set(BLINKY_SERVICE_NODE_BLINK_LED, false);
}

void blinky_service_node_proc(void) {
  for(uint8_t button = 1U; button <= BLINKY_IO_COUNT; ++button) {
    if(!blinky_io_button_pressed(button)) {
      continue;
    }
    if(!joined) {
      LOG_INF("button %u dropped: not joined", button);
      continue;
    }
    uint8_t msg[BLINKY_SERVICE_MSG_SIZE];
    blinky_service_button_msg(button, msg);
    if(send_to_border_router(msg)) {
      LOG_INF("button %u: LED%u %s sent to the border router", button, msg[0], (1U == msg[1]) ? "on" : "off");
    } else {
      LOG_WRN("button %u: not sent", button);
    }
  }
}
