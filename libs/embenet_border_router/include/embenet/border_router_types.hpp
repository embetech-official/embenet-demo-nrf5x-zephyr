/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief     embeNET Border Router C++ API common types.
 */

#ifndef BORDER_ROUTER_TYPES_HPP
#define BORDER_ROUTER_TYPES_HPP
#include <cstddef>
#include <cstdint>
#include <embenet/border_router_types.h> // IWYU pragma: export
#include <system_error>

namespace embenet::border_router {

/// @copydoc EMBENET_BR_Capabilities
using Capabilities = EMBENET_BR_Capabilities;

/// Network prefix (64 bits)
using NetworkPrefix = EMBENET_NetworkPrefix;

/// Node unique identifier.
using EUI64 = EMBENET_EUI64;

/// IPv6 address.
using Address = EMBENET_IPV6;

/// Multicast group ID.
using GID = EMBENET_GroupId;

/// UDP socket descriptor.
using Socket = EMBENET_UDP_SocketDescriptor;

/// @copydoc EMBENET_BR_JoinRule
using JoinRule = EMBENET_BR_JoinRule;

/// @copydoc EMBENET_BR_JoinPolicy
using JoinPolicy = EMBENET_BR_JoinPolicy;

/// @copydoc EMBENET_BR_NetworkParameters
using NetworkParameters = EMBENET_BR_NetworkParameters;

/// @copydoc EMBENET_BR_NodeDetails
using NodeDetails = EMBENET_BR_NodeDetails;

/// Error codes used by Border Router components.
enum class errc { // NOLINT(performance-enum-size)
  none = 0,
  invalid_argument,
  not_configured,
  not_found,
  already_exists,
  not_enough_memory,
  no_buffer_space,
  address_not_available,

  no_root_node,

  bad_address,
  bad_socket,
  no_link,
  network_down,
  not_connected,

  allocation_failure,
  mutex_failure,
  semaphore_failure,
  network_adapter_failure,
  root_node_connector_failure,
};

/// Returns the Border Router error category.
std::error_category const &border_router_error_category();

/**
 * Creates an std::error_code from embenet::border_router::errc.
 *
 * @param error Error code value
 * @return constexpr std::error_code
 */
[[nodiscard]] constexpr std::error_code make_error_code(errc error) { return {static_cast<int>(error), border_router_error_category()}; }

/// @copydoc EMBENET_BR_AuthResult
using AuthResult = EMBENET_BR_AuthResult;

/// Types of events emitted by the Border Router.
enum class EventType : std::uint16_t {
  network_started = 0,
  network_stopped = 1,

  node_joined_network = 10,
  node_left_network = 11,
  node_joined_group = 12,
  node_left_group = 13,
  authentication_attempt = 14,

  data_on_unregistered_port = 20,

  asn_update = 30,

  root_node_connection_failed = 40,
  root_node_identified = 41,
};

/// Common alignment of Event and every concrete event struct in events.Allows safe use of reinterpret_cast.
inline constexpr std::size_t event_alignment = alignof(std::max_align_t);

/// Base class for all event structures.
struct alignas(event_alignment) Event {

  /**
   * Unique identifier of the event.
   * Each derived class MUST initialize this field with UNIQUE value
   */
  EventType type;
};

namespace events {
/// Event emitted once the Root Node has identified itself (sent its start notification). At this point
/// its UID is known, but the network handshake with it is not complete yet - the network is not
/// operational until @ref NetworkStarted is emitted.
struct alignas(event_alignment) RootNodeIdentified {
  EventType type{EventType::root_node_identified};                                     ///< Unique event identifier
  EUI64 root_uid{0};                                                                   ///< EUI64 of the root node that identified itself
  explicit(false) constexpr RootNodeIdentified(EUI64 root_uid) : root_uid{root_uid} {} ///< Constructor
};

/// Event emitted once the network is fully operational: the Root Node has confirmed the network start
/// request, containing the UID of the root node.
struct alignas(event_alignment) NetworkStarted {
  EventType type{EventType::network_started};                                      ///< Unique event identifier
  EUI64 root_uid{0};                                                               ///< EUI64 of the root node that started the network
  explicit(false) constexpr NetworkStarted(EUI64 root_uid) : root_uid{root_uid} {} ///< Constructor
};

/// Event emitted when the network is stopped.
struct alignas(event_alignment) NetworkStopped {
  EventType type{EventType::network_stopped}; ///< Unique event identifier
};

/// Event emitted when a node joins the network, containing the UID of the node
struct alignas(event_alignment) NodeJoinedNetwork {
  EventType type{EventType::node_joined_network};                                     ///< Unique event identifier
  EUI64 node_eui{0};                                                                  ///< EUI64 of the node that joined the network
  explicit(false) constexpr NodeJoinedNetwork(EUI64 node_eui) : node_eui{node_eui} {} ///< Constructor
};

/// Event emitted when a node leaves the network, containing the UID of the node
struct alignas(event_alignment) NodeLeftNetwork {
  EventType type{EventType::node_left_network};                                     ///< Unique event identifier
  EUI64 node_eui{0};                                                                ///< EUI64 of the node that left the network
  explicit(false) constexpr NodeLeftNetwork(EUI64 node_eui) : node_eui{node_eui} {} ///< Constructor
};

/// Event emitted for every ENCJS authentication attempt, regardless of its outcome.
struct alignas(event_alignment) AuthenticationAttempt {
  EventType type{EventType::authentication_attempt};                                                         ///< Unique event identifier
  EUI64 node_eui{0};                                                                                          ///< EUI64 of the node that attempted authentication
  AuthResult result;                                                                                          ///< Outcome of the authentication attempt
  constexpr AuthenticationAttempt(EUI64 node_eui, AuthResult result) : node_eui{node_eui}, result{result} {} ///< Constructor
};

/// Event emitted when a node joins a group, containing the UID of the node and the group
struct alignas(event_alignment) NodeJoinedGroup {
  EventType type{EventType::node_joined_group};                                                          ///< Unique event identifier
  EUI64 node_eui{0};                                                                                     ///< EUI64 of the node that joined the group
  GID group_uid{0};                                                                                      ///< ID of the group that was joined
  constexpr NodeJoinedGroup(EUI64 node_eui, GID group_uid) : node_eui{node_eui}, group_uid{group_uid} {} ///< Constructor
};

/// Event emitted when a node leaves a group, containing the UID of the node and the group
struct alignas(event_alignment) NodeLeftGroup {
  EventType type{EventType::node_left_group};                                                          ///< Unique event identifier
  EUI64 node_eui{0};                                                                                   ///< EUI64 of the node that left the group
  embenet::border_router::GID group_uid{0};                                                            ///< ID of the group that was left
  constexpr NodeLeftGroup(EUI64 node_eui, GID group_uid) : node_eui{node_eui}, group_uid{group_uid} {} ///< Constructor
};

/// Event emitted when data on unregistered port is received
struct alignas(event_alignment) DataOnUnregisteredPort {
  EventType type{EventType::data_on_unregistered_port}; ///< Unique event identifier
  std::uint16_t destination_port;                       ///< Destination port on which data was received
  std::uint16_t source_port;                            ///< Source port from which data was received
  constexpr DataOnUnregisteredPort(std::uint16_t destination_port, std::uint16_t source_port)
      : destination_port{destination_port}, source_port{source_port} {} ///< Constructor
};

/// Event emitted every time ASN is updated
struct alignas(event_alignment) AsnUpdate {
  EventType type{EventType::asn_update};                               ///< Unique event identifier
  std::uint64_t asn{0};                                                ///< Absolute Slot Number (ASN) of the network at the time of the event
  explicit(false) constexpr AsnUpdate(std::uint64_t asn) : asn{asn} {} ///< Constructor
};

/// Event emitted when the Root Node fails to (re)establish a connection before the response timeout elapses.
/// This is a single, non-retried attempt: the border router does not automatically retry after this event,
/// a new connection attempt must be triggered explicitly (e.g. by calling up() again).
struct alignas(event_alignment) RootNodeConnectionFailed {
  EventType type{EventType::root_node_connection_failed}; ///< Unique event identifier
};

} // namespace events
} // namespace embenet::border_router
#endif
