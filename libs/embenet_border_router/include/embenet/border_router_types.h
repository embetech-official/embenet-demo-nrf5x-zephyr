/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief     embeNET Border Router C API common types.
 */

#ifndef EMBENET_BORDER_ROUTER_TYPES_H
#define EMBENET_BORDER_ROUTER_TYPES_H

#include <embenet/udp.h> // IWYU pragma: export
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup embenet_common embeNET common definitions
 *  @{
 */

enum {
  /// Length of the Cryptographic keys.
  EMBENET_BR_CRYPTO_KEY_LENGTH = 16,

  /// Maximum size of the PAN-wide data that can be broadcasted by the nodes in the network.
  EMBENET_BR_MAX_PAN_DATA_SIZE = 16,

  /// Maximum multicast groups that node can be registered to.
  EMBENET_BR_MAX_NODE_MULTICAST_GROUPS = 16,
};

/**
 * Result codes returned by Border Router functions.
 * 0 indicates success, any other value indicates an error.
 * See [<errno.h>](https://en.cppreference.com/w/cpp/error/errno_macros.html) for details
 */
typedef int EMBENET_BR_Result;

/// Link Layer unique Network Identifier
typedef uint16_t EMBENET_PANID;

/// IPv6 subnet prefix (64 bits)
typedef uint64_t EMBENET_PAN_Prefix;

/// Type used to store the cryptographic keys value.
typedef struct {
  /// Stores key value
  uint8_t value[EMBENET_BR_CRYPTO_KEY_LENGTH];
} EMBENET_BR_CryptoKey;

/**
 * Called once the network is fully operational: the Root Node has confirmed the network start request.
 *
 * @param[in] root_node_uid UID of the root node
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnNetworkStarted)(uint64_t root_node_uid, void *context);

/**
 * Called when network stops due to an error.
 *
 * @param[in] context User-defined context
 *
 * @note To make the network operational after it stops @ref EMBENET_BR_Down should be called
 * and then Border Router may be started once again (@ref EMBENET_BR_Up).
 */
typedef void (*EMBENET_BR_OnNetworkStopped)(void *context);

/**
 * Called when a node joins the network.
 *
 * @param[in] uid UID of the joining node
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnNodeJoined)(EMBENET_EUI64 uid, void *context);

/**
 * Called when a node leaves the network.
 *
 * @param[in] uid UID of the leaving node
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnNodeLeft)(EMBENET_EUI64 uid, void *context);

/// Outcome of an ENCJS authentication attempt.
typedef enum {
  EMBENET_BR_AUTH_RESULT_AUTHENTICATED,    ///< A matching join rule was found and the node authenticated successfully.
  EMBENET_BR_AUTH_RESULT_REJECTED,         ///< A matching join rule was found, but authentication with it failed.
  EMBENET_BR_AUTH_RESULT_NO_MATCHING_RULE, ///< No join rule matching the node was found.
} EMBENET_BR_AuthResult;

/**
 * Called for every ENCJS authentication attempt, regardless of its outcome.
 *
 * @param[in] uid UID of the node that attempted authentication
 * @param[in] result Outcome of the authentication attempt
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnAuthenticationAttempt)(EMBENET_EUI64 uid, EMBENET_BR_AuthResult result, void *context);

/**
 * Called when a node is added to a group.
 *
 * @param[in] group_id ID of the group
 * @param[in] uid UID of the node
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnNodeAddedToGroup)(EMBENET_GroupId group_id, EMBENET_EUI64 uid, void *context);

/**
 * Called when a node is removed from a group.
 *
 * @param[in] group_id ID of the group
 * @param[in] uid UID of the node
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnNodeRemovedFromGroup)(EMBENET_GroupId group_id, EMBENET_EUI64 uid, void *context);

/**
 * Called when a UDP datagram was received on an unregistered port.
 *
 * @param[in] destination_port destination port number.
 * @param[in] source_port source port number.
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnDataOnUnregisteredPort)(uint16_t destination_port, uint16_t source_port, void *context);

/**
 * Called when the Absolute Slot Number (ASN) is updated.
 *
 * @param[in] asn Updated ASN value.
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnAsnUpdate)(uint64_t asn, void *context);

/**
 * Called when the Root Node fails to (re)establish a connection before the response timeout elapses.
 * This is a single, non-retried attempt: it is not automatically retried afterwards.
 *
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnRootNodeConnectionFailed)(void *context);

/**
 * Called once the Root Node has identified itself (sent its start notification). At this point its
 * UID is known, but the network handshake with it is not complete yet - the network is not
 * operational until @ref EMBENET_BR_OnNetworkStarted fires.
 *
 * @param[in] root_node_uid UID of the root node
 * @param[in] context User-defined context
 */
typedef void (*EMBENET_BR_OnRootNodeIdentified)(uint64_t root_node_uid, void *context);

/// Structure holding embeNET Border Router stack event handlers.
typedef struct {
  /// Event handler that is called when the network is started in the border router.
  EMBENET_BR_OnNetworkStarted on_network_started;

  /// Event handler that is called when the network is stopped by the border router.
  EMBENET_BR_OnNetworkStopped on_network_stopped;

  /// Event handler that is called when the node joins the network
  EMBENET_BR_OnNodeJoined on_node_joined;

  /// Event handler that is called when the node leaves the network.
  EMBENET_BR_OnNodeLeft on_node_left;

  /// Event handler that is called for every ENCJS authentication attempt, regardless of its outcome.
  EMBENET_BR_OnAuthenticationAttempt on_authentication_attempt;

  /// Event handler that is called when the node joins a group.
  EMBENET_BR_OnNodeAddedToGroup on_node_added_to_group;

  /// Event handler that is called when the node is removed from a group.
  EMBENET_BR_OnNodeRemovedFromGroup on_node_removed_from_group;

  /// Event handler that is called when a UDP datagram was received on an unregistered port.
  EMBENET_BR_OnDataOnUnregisteredPort on_datagram_on_unregistered_port;

  /// Event handler that is called when the Absolute Slot Number (ASN) is updated.
  EMBENET_BR_OnAsnUpdate on_asn_update;

  /// Event handler that is called when the Root Node fails to (re)establish a connection before the response
  /// timeout elapses. This is a single, non-retried attempt: it is not automatically retried afterwards.
  EMBENET_BR_OnRootNodeConnectionFailed on_root_node_connection_failed;

  /// Event handler that is called once the Root Node has identified itself, before the network is operational.
  EMBENET_BR_OnRootNodeIdentified on_root_node_identified;
} EMBENET_BR_EventHandlers;

/// Stores Border Router configuration needed to start the network.
typedef struct {

  /**
   * Network key used to validate origin of the messages across all of the
   * network nodes.
   * The provided key MUST be the same in all of the nodes,
   * otherwise they won't be able to connect to the network
   */
  EMBENET_BR_CryptoKey k1;

  /**
   * IPv6 subnet prefix.
   * The network always uses \64 subnet prefix.
   */
  EMBENET_PAN_Prefix pan_prefix;

  /**
   * When the value is other than EMBENET_EUI64_INVALID, overrides node's
   * unique identifier. Otherwise the field does nothing
   */
  EMBENET_EUI64 root_node_uid_override;

  /**
   * The Border Router's own unique identifier (used as the IID of its IPv6 address, which
   * shares pan_prefix with the rest of the network and also serves as the RPL DODAGID).
   *
   * @note This field is MANDATORY - it MUST be set to a value other than EMBENET_EUI64_INVALID.
   * There is no default: EMBENET_BorderRouter_Configure() fails configuration if this field is
   * left at EMBENET_EUI64_INVALID.
   */
  EMBENET_EUI64 own_uid;

  /// Network ID.
  EMBENET_PANID pan_id;

  /// PAN Data size in bytes.
  uint8_t pan_data_size;

  /// PAN Data buffer that will be broadcasted by all nodes in the network
  uint8_t pan_data[EMBENET_BR_MAX_PAN_DATA_SIZE];

  /**
   * Duration of silence (no valid traffic from the Root Node) after which an established connection
   * is considered lost and the Root Node connection is reset. Set to 0 to disable this watchdog -
   * there is no automatic substitute value (a recommended starting point is 1 second).
   *
   * @warning Disabling the watchdog means a Root Node that goes silent without a clean disconnect
   * will never be detected and the connection will appear ESTABLISHED indefinitely.
   */
  uint32_t root_node_watchdog_timeout_s;

  /**
   * Maximum time to wait for the Root Node to respond to a RESET or a START_NETWORK_REQUEST before
   * giving up on the connection attempt and reporting @ref EMBENET_BR_OnRootNodeConnectionFailed
   * (a single, non-retried attempt - see that handler's documentation). Set to 0 to disable this
   * timeout (wait indefinitely) - there is no automatic substitute value (a recommended starting
   * point is 500 milliseconds).
   */
  uint32_t root_node_response_timeout_ms;
} EMBENET_BR_NetworkParameters;

/// Stores Border Router configuration parameters.
typedef struct {
  /// Maximum number of nodes in the network.
  uint32_t max_nodes;

  ///  Maximum number of multicast groups in the network.
  uint32_t max_groups;

  ///  Maximum number of join rules in the network.
  uint32_t max_join_rules;

  /// Capacity of the packet queue. Set to 0 to use the typical (default) value.
  uint32_t packet_queue_capacity;

  /// Capacity of the event queue. Set to 0 to use the typical (default) value.
  uint32_t event_queue_capacity;
} EMBENET_BR_Capabilities;

/// Single rule that governs the constrained join process.
typedef struct {
  /**
   * ID of the remote device, UID=0 is a special case of universal rule called
   * zero-rule. zero-rule may be used to support many devices with the same PSK
   * key without a necessity of formulating individual rules for every one of
   * them. Note that many zero-rules may co-exists simultaneously.
   */
  EMBENET_EUI64 uid;

  /// 128-bit pre-shared key used to authenticate the joining device.
  EMBENET_BR_CryptoKey psk;
} EMBENET_BR_JoinRule;

/// Trust Authority join policy, used to limit the the surges of authentication requests.
typedef struct {
  /// Determines how many authentication requests can be processed immediately. Must not be negative.
  int32_t max_immediate_requests;

  /// Minimum interval between processing authentication requests. Must be strictly positive.
  int32_t min_request_interval_ms;
} EMBENET_BR_JoinPolicy;

/// List of nodes present in the network.
typedef struct {
  /// Total number of nodes in the network.
  size_t total_count;

  /// Number of nodes present in the entries field.
  size_t count;

  /**
   * List of nodes' unique identifiers (EUI64).
   *
   * @note This field contains user-provided address to the buffer, that was passed to
   * EMBENET_BR_GetNodes() function. Its lifetime is managed by the user.
   * The field may be NULL if the user did not provide any buffer.
   */
  EMBENET_EUI64 *nodes;
} EMBENET_BR_NodesList;

/// List of multicast groups registered in the network.
typedef struct {
  /// Total number of registered multicast groups in the network.
  size_t total_count;

  /// Number of group IDs present in the groups field.
  size_t count;

  /**
   * List of registered multicast group IDs.
   *
   * @note This field contains user-provided address to the buffer, that was passed to
   * EMBENET_BR_GetMulticastGroups() function. Its lifetime is managed by the user.
   * The field may be NULL if the user did not provide any buffer.
   */
  EMBENET_GroupId *groups;
} EMBENET_BR_GroupsList;

/// List of join rules present in the network.
typedef struct {
  size_t total_count;         ///< Total number of join rules configured in the border router.
  size_t count;               ///< Number of join rules in the @ref rules array (may be less than @ref total_count when paging).
  EMBENET_BR_JoinRule *rules; ///< Pointer to the array of join rules.
} EMBENET_BR_JoinRulesList;

/// Topology information about the node in the Network.
typedef struct {
  /**
   * Node's unique identifier.
   * If for some reason the node is not known, the field will contain EMBENET_EUI64_INVALID.
   * If this is the case, other fields in the structure will contain no meaningful values, and MUST be ignored.
   */
  EMBENET_EUI64 uid;

  /// Node's parent UID or EMBENET_EUI64_INVALID if the node has no parent.
  EMBENET_EUI64 parent_uid;

  /// Time since last received message from the node.
  int64_t last_seen;

  /// Number of multicast groups that the node is registered to.
  size_t registered_group_count;

  /// Multicast groups that the node is registered to.
  EMBENET_GroupId groups[EMBENET_BR_MAX_NODE_MULTICAST_GROUPS];
} EMBENET_BR_NodeDetails;

/** @} */

#ifdef __cplusplus
}
#endif

#endif
