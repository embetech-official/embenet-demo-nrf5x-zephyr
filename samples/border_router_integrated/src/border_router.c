/**
 * @file
 * @brief embeNET border router - the gateway half that forms the network, on its own thread.
 *
 * Everything the border router library touches happens here: EMBENET_BR_Proc(), the event handlers
 * it calls from inside it, and the frames the root node sends through EMBENET_PORT_COMMON_BrFrameProc().
 * The library requires the frames on a thread - its data-received callback must not run in interrupt
 * context - which is why the in-memory connector has no thread of its own.
 */

#include <embenet/border_router.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <inttypes.h>

LOG_MODULE_REGISTER(border_router, LOG_LEVEL_INF);

/// @brief Below the root node thread, which owns the radio schedule.
#define BR_THREAD_PRIORITY 6
#define BR_THREAD_STACK_SIZE 4096

/// @brief How long the thread sits in BrFrameProc() with nothing to do. It doubles as the tick for EMBENET_BR_Proc().
#define BR_PROC_INTERVAL K_MSEC(1)

/// @brief Started after the root node: the border router opens the link, and the root node clears both shared queues as it comes up.
#define BR_THREAD_START_DELAY_MS 200

/// @brief Network identity. Every node on the network is given the PAN ID and prefix in its advertisements.
#define DEMO_PAN_ID 0x0001U
#define DEMO_PAN_PREFIX UINT64_C(0xFD00000000000000)
#define DEMO_BORDER_ROUTER_UID UINT64_C(0x0000000000000001)
#define DEMO_PANDATA "nrf demo" // must be <= EMBENET_BR_MAX_PAN_DATA_SIZE

/// @brief First byte of the demo K1/PSK; the rest are zero. K1 must match every device on the network.
#define DEMO_K1_BYTE 0x11
#define DEMO_PSK_BYTE 0x33

/// @brief Left non-zero: at 0 a root node that goes silent is never detected.
#define DEMO_ROOT_NODE_WATCHDOG_S 3
#define DEMO_ROOT_NODE_RESPONSE_MS 2500

/// @brief The in-memory queue in place of the UART of a two-board gateway. The name after the colon is not interpreted by the connector.
#define ROOT_NODE_INTERFACE "message:embenet-aio-queue"

/// @brief Interval between the demo multicast datagrams sent to group 100 [ms].
#define DEMO_MULTICAST_INTERVAL_MS 10000

// Every handler the border router offers is wired, so nothing the stack reports goes unseen. All of
// them run from EMBENET_BR_Proc(), i.e. on this thread, so logging from here is safe.

/**
 * @brief Called when the root node announces itself. The network handshake still has to complete.
 * @param[in] root_node_uid The UID of the root node that answered on the link.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_root_node_identified(uint64_t root_node_uid, void *context) {
  ARG_UNUSED(context);
  LOG_INF("root node identified, UID %016" PRIx64, root_node_uid);
}

/**
 * @brief Called when the root node did not answer in time. A single attempt: recovering is the application's call.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_root_node_connection_failed(void *context) {
  ARG_UNUSED(context);
  LOG_ERR("root node did not answer within the response timeout");
}

/**
 * @brief Called when the network is up and nodes may start joining.
 * @param[in] root_node_uid The UID of the root node driving the radio.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_network_started(uint64_t root_node_uid, void *context) {
  ARG_UNUSED(context);
  LOG_INF("network started, root node UID %016" PRIx64, root_node_uid);
}

/**
 * @brief Called when the network goes down. Restarting it means EMBENET_BR_Down() then EMBENET_BR_Up(); this sample only reports it.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_network_stopped(void *context) {
  ARG_UNUSED(context);
  LOG_WRN("network stopped by the border router");
}

/**
 * @brief Called when a node has joined the network.
 * @param[in] uid The UID of the node that joined.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_node_joined(EMBENET_EUI64 uid, void *context) {
  ARG_UNUSED(context);
  LOG_INF("node %016" PRIx64 " joined (%zu total)", uid, EMBENET_BR_GetNodesCount());
}

/**
 * @brief Called when a node has left the network.
 * @param[in] uid The UID of the node that left.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_node_left(EMBENET_EUI64 uid, void *context) {
  ARG_UNUSED(context);
  LOG_INF("node %016" PRIx64 " left", uid);
}

/**
 * @brief Names an authentication result for the log.
 * @param[in] result The result reported by the stack.
 * @return A static, human-readable name of the result.
 */
static char const *auth_result_name(EMBENET_BR_AuthResult result) {
  switch(result) {
  case EMBENET_BR_AUTH_RESULT_AUTHENTICATED:
    return "authenticated";
  case EMBENET_BR_AUTH_RESULT_REJECTED:
    return "rejected";
  case EMBENET_BR_AUTH_RESULT_NO_MATCHING_RULE:
    return "no matching join rule";
  default:
    return "unknown";
  }
}

/**
 * @brief Called on every authentication attempt, so a node that cannot join says why here rather than staying silent.
 * @param[in] uid The UID of the node that tried to authenticate.
 * @param[in] result Whether the node was admitted, and if not, why.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_authentication_attempt(EMBENET_EUI64 uid, EMBENET_BR_AuthResult result, void *context) {
  ARG_UNUSED(context);
  if(result == EMBENET_BR_AUTH_RESULT_AUTHENTICATED) {
    LOG_INF("node %016" PRIx64 " %s", uid, auth_result_name(result));
  } else {
    LOG_WRN("node %016" PRIx64 " %s", uid, auth_result_name(result));
  }
}

/**
 * @brief Called when a node joins a multicast group.
 * @param[in] group_id The group the node joined.
 * @param[in] uid The UID of the node.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_node_added_to_group(EMBENET_GroupId group_id, EMBENET_EUI64 uid, void *context) {
  ARG_UNUSED(context);
  LOG_INF("node %016" PRIx64 " added to group %" PRIu16, uid, group_id);
}

/**
 * @brief Called when a node leaves a multicast group.
 * @param[in] group_id The group the node left.
 * @param[in] uid The UID of the node.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_node_removed_from_group(EMBENET_GroupId group_id, EMBENET_EUI64 uid, void *context) {
  ARG_UNUSED(context);
  LOG_INF("node %016" PRIx64 " removed from group %" PRIu16, uid, group_id);
}

/**
 * @brief Called when a datagram arrives for a port no socket is bound to.
 * @param[in] destination_port The port the datagram was addressed to.
 * @param[in] source_port The port the datagram was sent from.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_datagram_on_unregistered_port(uint16_t destination_port, uint16_t source_port, void *context) {
  ARG_UNUSED(context);
  LOG_WRN("datagram dropped: no socket bound to port %" PRIu16 " (from port %" PRIu16 ")", destination_port, source_port);
}

/**
 * @brief Called once per slot, which would drown every other line at INF - raise CONFIG_LOG_DEFAULT_LEVEL to 4 to see it.
 * @param[in] asn The absolute slot number the network has reached.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_asn_update(uint64_t asn, void *context) {
  ARG_UNUSED(context);
  LOG_DBG("ASN %" PRIu64, asn);
}

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
  LOG_INF("UDP data received on port %" PRIu16 " from %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, size %zu", socket->port, sourceAddress->value[0],
          sourceAddress->value[1], sourceAddress->value[2], sourceAddress->value[3], sourceAddress->value[4], sourceAddress->value[5],
          sourceAddress->value[6], sourceAddress->value[7], dataSize);
}

/// @brief The UDP socket descriptor for the registered socket. Group 100 is the one the demo nodes join.
static EMBENET_UDP_SocketDescriptor socket = {
    .port = 1234,
    .rxDataHandler = on_udp_data_received,
    .groupId = 100,
    .userContext = NULL,
    .handledTraffic = EMBENET_UDP_TRAFFIC_ALL,
};

static bool start_border_router(void) {
  EMBENET_BR_Capabilities capabilities = EMBENET_BR_GetDefaultCapabilities();
  capabilities.packet_queue_capacity = 10;
  capabilities.event_queue_capacity = 10;
  capabilities.max_groups = 2;

  EMBENET_BR_Result result = EMBENET_BR_Init(capabilities);
  if(result != 0) {
    LOG_ERR("EMBENET_BR_Init() failed: %d", result);
    return false;
  }

  EMBENET_BR_EventHandlers const handlers = {
      .on_network_started = on_network_started,
      .on_network_stopped = on_network_stopped,
      .on_node_joined = on_node_joined,
      .on_node_left = on_node_left,
      .on_authentication_attempt = on_authentication_attempt,
      .on_node_added_to_group = on_node_added_to_group,
      .on_node_removed_from_group = on_node_removed_from_group,
      .on_datagram_on_unregistered_port = on_datagram_on_unregistered_port,
      .on_asn_update = on_asn_update,
      .on_root_node_connection_failed = on_root_node_connection_failed,
      .on_root_node_identified = on_root_node_identified,
  };
  result = EMBENET_BR_SubscribeToEvents(handlers, NULL);
  if(result != 0) {
    LOG_ERR("EMBENET_BR_SubscribeToEvents() failed: %d", result);
    return false;
  }

  EMBENET_BR_NetworkParameters parameters = {
      .k1 = {.value =
                 {
                     DEMO_K1_BYTE,
                     0,
                 }},
      .pan_prefix = DEMO_PAN_PREFIX,
      .pan_id = DEMO_PAN_ID,
      .root_node_uid_override = EMBENET_EUI64_INVALID,
      .own_uid = DEMO_BORDER_ROUTER_UID,
      .pan_data = DEMO_PANDATA,
      .pan_data_size = sizeof(DEMO_PANDATA),
      .root_node_watchdog_timeout_s = DEMO_ROOT_NODE_WATCHDOG_S,
      .root_node_response_timeout_ms = DEMO_ROOT_NODE_RESPONSE_MS,
  };
  result = EMBENET_BR_Configure(ROOT_NODE_INTERFACE, &parameters, "");
  if(result != 0) {
    LOG_ERR("EMBENET_BR_Configure() failed: %d", result);
    return false;
  }

  // Zero-rule: UID 0 means one PSK admits any node. For a deployment, add a per-node rule instead.
  EMBENET_BR_JoinRule join_rule = {
      .uid = 0,
      .psk = {.value =
                  {
                      DEMO_PSK_BYTE,
                      0,
                  }},
  };
  result = EMBENET_BR_AddJoinRule(&join_rule);
  if(result != 0) {
    LOG_ERR("EMBENET_BR_AddJoinRule() failed: %d", result);
    return false;
  }

  result = EMBENET_BR_RegisterSocket(&socket);
  if(result != 0) {
    LOG_ERR("EMBENET_BR_RegisterSocket() failed: %d", result);
    return false;
  }

  result = EMBENET_BR_Up();
  if(result != 0) {
    LOG_ERR("EMBENET_BR_Up() failed: %d", result);
    return false;
  }
  return true;
}

static void border_router_thread_entry(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  k_thread_name_set(k_current_get(), "embenet_br");

  if(!start_border_router()) {
    LOG_ERR("border router failed to start, giving up");
    return;
  }

  int64_t multicast_sending_timestamp = k_uptime_get() + 10000;

  for(;;) {
    // Blocks here when the link is quiet, so an idle gateway costs one wakeup per interval rather
    // than a spin. Frames land in the connector's handler from inside this call.
    (void)EMBENET_PORT_COMMON_BrFrameProc(BR_PROC_INTERVAL);
    (void)EMBENET_BR_Proc();

    if(k_uptime_get() >= multicast_sending_timestamp) {
      multicast_sending_timestamp = k_uptime_get() + 10000;
      EMBENET_IPV6 dst = EMBENET_AssembleMulticastIpv6(DEMO_PAN_PREFIX, 100);
      char msg[] = "the quick brown fox jumps over the lazy dog";
      // send packet to all nodes
      LOG_INF("Time %" PRIu64 ", sending multicast packet", multicast_sending_timestamp);
      if(EMBENET_UDP_RESULT_OK != EMBENET_BR_Send(&socket, &dst, socket.port, msg, sizeof(msg))) {
        LOG_ERR("Multicast datagram of length %zu not sent", sizeof(msg));
      }
    }
  }
}

K_THREAD_DEFINE(br_thread_id, BR_THREAD_STACK_SIZE, border_router_thread_entry, NULL, NULL, NULL, BR_THREAD_PRIORITY, 0, BR_THREAD_START_DELAY_MS);
