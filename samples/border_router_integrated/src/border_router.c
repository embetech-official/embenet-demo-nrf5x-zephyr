/**
 * @file
 * @brief Border router half of the integrated border router: starts the network, handles node joins and logs the nodes' datagrams on
 * DEMO_DATAGRAM_UDP_PORT. It also runs the router side of the blinky service.
 */

#include <blinky_service_router.h>
#include <embenet/border_router.h>
#include <embenet_demo_network.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

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

/// @brief Left non-zero: at 0 a root node that goes silent is never detected.
#define DEMO_ROOT_NODE_WATCHDOG_S 3
#define DEMO_ROOT_NODE_RESPONSE_MS 2500

/// @brief The in-memory queue in place of the UART of a two-board gateway. The name after the colon is not interpreted by the connector.
#define ROOT_NODE_INTERFACE "message:embenet-aio-queue"

// Every handler the border router offers is wired, so nothing the stack reports goes unseen. All of
// them run from EMBENET_BR_Proc(), i.e. on this thread, so logging from here is safe.

/**
 * @brief Called on every datagram received on @ref datagram_socket: logs the sender and the message.
 * @param[in] socket The socket on which the datagram was received.
 * @param[in] source_address The source address of the datagram.
 * @param[in] source_port The source port of the datagram.
 * @param[in] data The data that was received.
 * @param[in] data_size The size of the data that was received.
 */
static void on_datagram_data_received(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *source_address, uint16_t source_port,
                                      void const *data, size_t data_size) {
  ARG_UNUSED(source_port);
  // The interface identifier, the address's last 8 bytes, names the sender.
  uint64_t const sender = sys_get_be64(&source_address->value[8]);
  LOG_INF("UDP data received on port %" PRIu16 " from %016" PRIx64 ", size %zu: \"%.*s\"", socket->port, sender, data_size, (int)data_size,
          (char const *)data);
}

/// @brief The socket the nodes' once-a-second datagram arrives on.
static EMBENET_UDP_SocketDescriptor datagram_socket = {
    .port = DEMO_DATAGRAM_UDP_PORT,
    .rxDataHandler = on_datagram_data_received,
    .groupId = DEMO_DATAGRAM_GROUP,
    .userContext = NULL,
    .handledTraffic = EMBENET_UDP_TRAFFIC_ALL,
};

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
  int const err = blinky_service_router_init();
  if(0 != err) {
    LOG_ERR("blinky_service_router_init() failed: %d", err);
  }
  EMBENET_BR_Result const result = EMBENET_BR_RegisterSocket(&datagram_socket);
  if(0 != result) {
    LOG_ERR("registering UDP port %u failed: %d", (unsigned)datagram_socket.port, result);
  } else {
    LOG_INF("listening on UDP port %u", (unsigned)datagram_socket.port);
  }
}

/**
 * @brief Called when the network goes down. Restarting it means EMBENET_BR_Down() then EMBENET_BR_Up(); this sample only reports it.
 * @param[in] context The user-defined context passed to EMBENET_BR_SubscribeToEvents().
 */
static void on_network_stopped(void *context) {
  ARG_UNUSED(context);
  LOG_WRN("network stopped by the border router");
  blinky_service_router_deinit();
  (void)EMBENET_BR_UnregisterSocket(&datagram_socket);
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

  // UID 0 admits any node that knows the PSK. Demonstration only - never use it in production code;
  // add one rule per node, with its own UID and PSK.
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

  for(;;) {
    // Blocks here when the link is quiet, so an idle gateway costs one wakeup per interval rather
    // than a spin. Frames land in the connector's handler from inside this call.
    (void)EMBENET_PORT_COMMON_BrFrameProc(BR_PROC_INTERVAL);
    (void)EMBENET_BR_Proc();
    // Multicasts the button presses; does nothing while the network is down.
    blinky_service_router_proc();
  }
}

K_THREAD_DEFINE(br_thread_id, BR_THREAD_STACK_SIZE, border_router_thread_entry, NULL, NULL, NULL, BR_THREAD_PRIORITY, 0, BR_THREAD_START_DELAY_MS);
