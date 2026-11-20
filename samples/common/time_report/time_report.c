/**
 * @file
 * @brief Time report - see time_report.h.
 */

#include <time_report.h>

#include <embenet_demo_network.h>

#include <embenet/node.h>
#include <embenet/node_udp.h>

#include <zephyr/logging/log.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>

LOG_MODULE_REGISTER(time_report, LOG_LEVEL_INF);

static void on_datagram(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *source_address, uint16_t source_port, void const *data,
                        size_t data_size) {
  ARG_UNUSED(source_address);
  ARG_UNUSED(source_port);
  ARG_UNUSED(data);
  LOG_INF("datagram received on port %" PRIu16 ", size %zu", socket->port, data_size);
}

static EMBENET_UDP_SocketDescriptor socket_descriptor = {
    .port = DEMO_DATAGRAM_UDP_PORT,
    .rxDataHandler = on_datagram,
    .groupId = DEMO_DATAGRAM_GROUP,
    .userContext = NULL,
    .handledTraffic = EMBENET_UDP_TRAFFIC_ALL,
};

static EMBENET_TaskId report_task_id = EMBENET_TASKID_INVALID;

/**
 * Sends the report.
 * @param t The network time it was scheduled for, in milliseconds - a whole second.
 */
static void report_task(EMBENET_TaskId id, EMBENET_NODE_TimeSource time_source, uint64_t t, void *context) {
  ARG_UNUSED(context);
  // Rescheduled off t rather than off the current time, so the period does not drift with the work below.
  (void)EMBENET_NODE_TaskSchedule(id, time_source, t + TIME_REPORT_PERIOD_MS);

  EMBENET_IPV6 border_router;
  if(EMBENET_RESULT_OK != EMBENET_NODE_GetBorderRouterAddress(&border_router)) {
    return;
  }
  char report[48];
  int const length = snprintf(report, sizeof(report), "UID %016" PRIx64 " time %" PRIu64, EMBENET_NODE_GetUID(), t);
  if(EMBENET_UDP_RESULT_OK == EMBENET_NODE_Send(&socket_descriptor, &border_router, DEMO_DATAGRAM_UDP_PORT, report, (size_t)length)) {
    LOG_INF("sent: %s", report);
  } else {
    LOG_WRN("report at time %" PRIu64 " not sent", t);
  }
}

int time_report_init(void) {
  if(EMBENET_UDP_RESULT_OK != EMBENET_NODE_RegisterSocket(&socket_descriptor)) {
    LOG_ERR("registering UDP port %u failed", (unsigned)DEMO_DATAGRAM_UDP_PORT);
    return -EIO;
  }
  report_task_id = EMBENET_NODE_TaskCreate(report_task, NULL);
  if(EMBENET_TASKID_INVALID == report_task_id) {
    LOG_ERR("creating the report task failed");
    (void)EMBENET_NODE_UnregisterSocket(&socket_descriptor);
    return -EIO;
  }
  return 0;
}

void time_report_on_joined(void) {
  // First run on the next whole second of network time, so every node in the network reports together.
  uint64_t const now = EMBENET_NODE_GetNetworkTime();
  uint64_t const first = now - (now % TIME_REPORT_PERIOD_MS) + TIME_REPORT_PERIOD_MS;
  if(EMBENET_RESULT_OK != EMBENET_NODE_TaskSchedule(report_task_id, EMBENET_NODE_TIME_SOURCE_NETWORK, first)) {
    LOG_ERR("scheduling the report task failed");
  }
}

void time_report_on_left(void) { (void)EMBENET_NODE_TaskCancel(report_task_id); }
