/**
 * @file
 * @brief embeNET Node sample - joins a network and keeps servicing it.
 *
 * Once joined, the node listens on UDP port 1234 and in group 100, and sends one unicast datagram
 * per second to the border router, aligned to network time so every node in the network fires
 * together. The gateway's multicast to the same group arrives on the same socket.
 */

#include <embenet/node.h>
#include <embenet/node_udp.h>

#include <embenet_port_common/embenet_port_common.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <inttypes.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/// @brief First byte of the demo K1/PSK; the rest are zero. K1 must match every device on the network, the PSK the border router's join rule.
#define DEMO_K1_BYTE 0x11
#define DEMO_PSK_BYTE 0x33

/// @brief Credentials the node joins with. In a product these come from secure settings or a provisioned partition, not from the image.
static EMBENET_NODE_JoinConfig const join_config = {
    .k1 = {.val = {DEMO_K1_BYTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    .psk = {.val = {DEMO_PSK_BYTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

/// @brief Id of @ref network_synchronized_task, valid from EMBENET_NODE_TaskCreate() onwards.
static EMBENET_TaskId taskId;

/**
 * @brief Called when the node joins a network.
 * @param[in] pan_id The PAN ID of the network that was joined.
 * @param[in] quick_join_credentials The credentials that can be used to fast rejoin.
 */
static void on_joined(EMBENET_PANID pan_id, EMBENET_NODE_QuickJoinCredentials const *quick_join_credentials) {
  ARG_UNUSED(quick_join_credentials);
  LOG_INF("joined network 0x%04" PRIx16, pan_id);

  uint64_t network_time = EMBENET_NODE_GetNetworkTime();
  // First run on the next whole second of network time, so every node in the network fires together.
  network_time /= 1000;
  network_time *= 1000;
  network_time += 1000;
  EMBENET_NODE_TaskSchedule(taskId, EMBENET_NODE_TIME_SOURCE_NETWORK, network_time);
}

/**
 * @brief Called when the node leaves a network.
 */
static void on_left(void) {
  LOG_WRN("left the network");
  EMBENET_NODE_TaskCancel(taskId);
}

/**
 * @brief Called when the node attempts to join a network (synchronized but not yet connected).
 * @param[in] pan_id The PAN ID of the network that is being joined.
 * @param[in] pan_data The PAN data that was received from the network.
 * @param[in] pan_data_size The size of the PAN data.
 */
static void on_join_attempt(EMBENET_PANID pan_id, uint8_t const *pan_data, size_t pan_data_size) {
  bool const pan_data_is_c_string = pan_data != NULL && pan_data_size > 0 && pan_data[pan_data_size - 1] == '\0';

  LOG_WRN("NODE attempting to join network 0x%04" PRIx16 ",%zu bytes of PAN data: '%s'", pan_id, pan_data_size,
          pan_data_is_c_string ? (char const *)pan_data : "(not a C-string)");
}

/**
 * @brief Called when the node receives a datagram on an unregistered port.
 * @param[in] port The port number on which the datagram was received.
 */
static void on_data_on_unregistered_port(uint16_t port) { LOG_WRN("Datagram on unregistered port %" PRIu16, port); }

/**
 * @brief Called on every datagram received on the registered UDP socket.
 * @param[in] socket The socket on which the datagram was received.
 * @param[in] sourceAddress The source address of the datagram.
 * @param[in] sourcePort The source port of the datagram.
 * @param[in] data The data that was received.
 * @param[in] dataSize The size of the data that was received.
 */
static void on_udp_data_received(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *sourceAddress, uint16_t sourcePort, void const *data,
                                 size_t dataSize) {
  LOG_INF("UDP data received on port %" PRIu16 " size %zu", socket->port, dataSize);
}

/// @brief The UDP socket descriptor for the registered socket.
static EMBENET_UDP_SocketDescriptor socket = {
    .port = 1234,
    .rxDataHandler = on_udp_data_received,
    .groupId = 100,
    .userContext = NULL,
    .handledTraffic = EMBENET_UDP_TRAFFIC_ALL,
};

/**
 * @brief The task that runs every second, synchronized to the network time.
 * @param[in] taskId The task ID of the task.
 * @param[in] timeSource The time source that triggered the task.
 * @param[in] t The time at which the task was triggered.
 * @param[in] context The user-defined context that was passed to the task when it was created.
 */
void network_synchronized_task(EMBENET_TaskId taskId, EMBENET_NODE_TimeSource timeSource, uint64_t t, void *context) {
  // Rescheduled off t rather than off the current time, so the period does not drift with the work below.
  EMBENET_NODE_TaskSchedule(taskId, timeSource, t + 1000);

  EMBENET_IPV6 dst;
  char msg[] = "the quick brown fox jumps over the lazy dog";
  // The address is only known while joined, so this both picks the destination and skips the send when there is none.
  if(EMBENET_RESULT_OK == EMBENET_NODE_GetBorderRouterAddress(&dst)) {
    LOG_INF("Time %" PRIu64 ", sending unicast packet", t);
    size_t dataAmountToSend = EMBENET_NODE_GetRandomValue(1, sizeof(msg));
    if(EMBENET_UDP_RESULT_OK != EMBENET_NODE_Send(&socket, &dst, socket.port, msg, dataAmountToSend)) {
      LOG_ERR("Datagram of length %zu not sent", dataAmountToSend);
    }
  }
}

/**
 * @brief Starts the node, then services the stack until the board is reset.
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

  EMBENET_Result result = EMBENET_NODE_Init(&handlers);
  if(result != EMBENET_RESULT_OK) {
    LOG_ERR("EMBENET_NODE_Init() failed: %d", (int)result);
    return 0;
  }

  EMBENET_NODE_RegisterSocket(&socket);
  EMBENET_NODE_JoinGroup(100);

  LOG_INF("UID %016" PRIx64, EMBENET_NODE_GetUID());

  result = EMBENET_NODE_Join(&join_config);
  if(result != EMBENET_RESULT_OK) {
    LOG_ERR("EMBENET_NODE_Join() failed: %d", (int)result);
    return 0;
  }

  taskId = EMBENET_NODE_TaskCreate(network_synchronized_task, NULL);

  // The stack only runs inside EMBENET_NODE_Proc(). The port ISRs give the semaphore as soon as there
  // is work; the timeout only bounds how long this sleeps when the radio is quiet.
  for(;;) {
    k_sem_take(EMBENET_PORT_COMMON_GetProcSemaphore(), K_MSEC(5));
    EMBENET_NODE_Proc();
  }
}
