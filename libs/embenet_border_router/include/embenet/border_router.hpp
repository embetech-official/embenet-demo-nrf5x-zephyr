/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief     Provides User interface for embeNET Border Router configuration and management.
 */

#ifndef EMBENET_BORDER_ROUTER_HPP
#define EMBENET_BORDER_ROUTER_HPP

#include <chrono>
#include <embenet/border_router_types.hpp> // IWYU pragma: export
#include <expected>
#include <memory>
#include <optional>
#include <string_view>
#include <cstdlib>
#include <cstdint>
#include <system_error>
#include <span>
#include <cstddef>

/**
 * @addtogroup embenet
 * @{
 *   @defgroup embenet_br embeNET Border Router API
 *   Application Interface for embeNET Border Router
 *   @{
 */

namespace embenet::border_router {

/**
 *  Border router instance. Provides translation, routing, topology information, and trust center capabilities.
 */
class BorderRouter {
public:
  /**
   * Function pointer type for event handling.
   */
  using EventHandler = void (*)(void *context, Event const &event);

  /** @name Control Interface
   *  Controls the Border Router operation
   */
  ///@{

  /**
   * Return the hard limits of every Border Router instance.
   * The limits include the maximum number of nodes, groups, and amount of join rules that are possible to define.
   *
   * The limits differ between the licensing variants of the library.
   * @return capabilities
   */
  [[nodiscard]] static Capabilities get_max_capabilities();

  /**
   * Return the default capabilities of every Border Router instance.
   * The default capabilities are a balanced set of parameters that should fit most use cases.
   *
   * @return capabilities
   */
  [[nodiscard]] static Capabilities get_default_capabilities();

  /**
   * Creates a new Border Router instance.
   *
   * @param[in] capabilities requested capabilities of this instance. The parameter should be provided if you wish to limit the memory footprint of
   * the instance. If not provided, the default capabilities will be used.
   *
   * @note the user-provided capabilities may be no greater than the maximum capabilities of the library - see @ref get_max_capabilities().
   *
   * @return Pointer to the new instance, or appropriate error code if the instance could not be created.
   * @retval std::errc::invalid_argument if capabilities are out of allowed limits
   * @retval std::errc::not_enough_memory if allocation failed
   * @retval std::errc::state_not_recoverable if internal resource creation failed
   */
  [[nodiscard]] static std::expected<BorderRouter, std::error_code> create(Capabilities capabilities = get_default_capabilities());

  /**
   * Subscribes to the Border Router network events.
   *
   * @param[in] handler callback to be invoked when an event occurs. When nullptr, the subscription is removed
   *
   * @param[in] context user-provided context to be passed to the handler
   *
   * @retval std::errc::operation_not_permitted if the router is not configured
   * @retval no_error on success
   */
  std::error_code subscribe_to_events(EventHandler handler, void *context);

  /**
   * Configures the Border Router with the provided network configuration.
   *
   * @param[in] root_node_interface name of the root node communication interface.
   * @param[in] config Network configuration
   * @param[in] wan_interface name of the system-wide WAN interface. If empty, the Border Router will not forward the LOWPAN traffic to the WAN.
   *
   * @return Status of the configuration
   *
   * @note The configuration will be copied to the Border Router instance, so the caller may safely discard it after the call
   *
   * @retval std::errc::invalid_argument if root_node_interface is empty
   * @retval std::errc::no_such_device if root node or WAN interface cannot be acquired
   * @retval std::errc::state_not_recoverable if internal resource creation failed
   * @retval no_error on success
   */
  [[nodiscard]] std::error_code configure(std::string_view root_node_interface, NetworkParameters const &config, std::string_view wan_interface = {});

  /**
   * Starts the Border Router operation.
   *
   * This call is non-blocking: it returns as soon as the WAN/root node interfaces have been opened and the
   * connection attempt to the Root Node has been initiated, without waiting for the Root Node to actually
   * confirm the network start. Outcome of that attempt (success or failure) is reported asynchronously via
   * events (@ref events::NetworkStarted / @ref events::RootNodeConnectionFailed), so it can be observed the
   * same way on every platform this library targets - including bare-metal ports with no OS-level blocking
   * primitive to wait on.
   *
   * @return Status of the operation
   *
   * @retval std::errc::operation_not_permitted if configuration is not set
   * @retval std::errc::no_such_device if WAN or root node cannot be brought up
   * @retval no_error on success
   */
  [[nodiscard]] std::error_code up();

  /**
   * Returns true if the Border Router is operational.
   *
   * @return true if operational, false otherwise
   */
  [[nodiscard]] bool is_up() const;

  /**
   * Stops the Border Router operation.
   */
  std::error_code down();

  /**
   * Handle border router processing.
   * This function MUST be called periodically to process the incoming and outgoing packets
   */
  std::error_code process(std::chrono::milliseconds timeout);
  /// @}

  /** @name Network Topology Information
   *  Obtain information about the network topology
   */
  ///@{

  /**
   * Obtain the network prefix if configured
   *
   * @return Network prefix or appropriate error code if not available
   */
  [[nodiscard]] std::expected<NetworkPrefix, std::error_code> get_network_prefix() const;

  /**
   * Return the address assigned to the Border Router in the LOWPAN network.
   * In most cases, this address is the same as the address of the WAN interface, however this address MUST be available even if no WAN interface is
   * used.
   *
   * @return Address of the Border Router in the LOWPAN network, or appropriate error code if not available
   */
  [[nodiscard]] std::expected<Address, std::error_code> get_own_address() const;

  /**
   * Return the address of the root node, if one is present.
   *
   * @return Address of the root node, or appropriate error code if not available
   */
  [[nodiscard]] std::expected<Address, std::error_code> get_root_node_address() const;

  /**
   * Return the address of the WAN interface, if one is present.
   *
   * @return Address of the WAN interface, or appropriate error code if not available
   */
  [[nodiscard]] std::expected<Address, std::error_code> get_wan_address() const;

  /**
   * Return the maximum number of nodes the router can handle.
   *
   * @return capacity of the nodes list
   */
  [[nodiscard]] std::size_t get_nodes_capacity() const;

  /**
   * Return the current node count.
   *
   * @return node count
   */
  [[nodiscard]] std::size_t get_nodes_count() const;

  /**
   * Get the list of the nodes EUI64 in the network.
   *
   * @param[in] buffer user-provided buffer to store the nodes EUI64
   * @param[in] offset index of the first node to retrieve
   * @param[in] count maximum number of nodes to retrieve. If std::dynamic_extent, the function will fill the provided buffer as much as possible.
   *
   * @return subspan of the buffer containing at most count entries, filled with the current nodes EUI64, starting from the given offset
   */
  [[nodiscard]] std::span<EUI64> get_nodes(std::span<EUI64> buffer, std::size_t offset = 0, std::size_t count = std::dynamic_extent) const;

  /**
   * Check if a node with the given UID is present in the network.
   *
   * @param uid UID of the node to check
   *
   * @return true if the node is present, false otherwise
   */
  [[nodiscard]] bool is_node_present(EUI64 uid) const;

  /**
   * Return the detailed information about the node with the given UID.
   *
   * @param[in] uid UID of the node to query
   *
   * @return If node is present NodeDetails object containing the node details, otherwise - std::nullopt.
   */
  [[nodiscard]] std::optional<NodeDetails> get_node_details(EUI64 uid) const;

  /**
   * Return the duration of node inactivity, after which the node is considered inactive.
   *
   * @return std::chrono::seconds
   */
  [[nodiscard]] std::chrono::seconds get_nodes_inactivity_timeout() const;

  /**
   * Set the nodes inactivity timeout.
   *
   * @param[in] timeout duration of inactivity after which the node is considered inactive. If 0, the timeout is disabled.
   *
   * @retval std::errc::invalid_argument if timeout is too short
   * @retval no_error on success
   */
  std::error_code set_nodes_inactivity_timeout(std::chrono::seconds timeout);

  /**
   * Get the maximum number of multicast groups the router can handle.
   *
   * @return capacity of the multicast groups list
   */
  [[nodiscard]] std::size_t get_multicast_group_capacity() const;

  /**
   * Get the current multicast group count.
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t get_multicast_group_count() const;

  /**
   * Get the list of the registered multicast groups in the network.
   *
   * @param[in] buffer user-provided buffer to store the group IDs
   * @param[in] offset index of the first group to retrieve
   * @param[in] count maximum number of group to retrieve. If std::dynamic_extent, the function will fill the provided buffer as much as possible.
   *
   * @return subspan of the buffer containing at most count entries, filled with the current registered multicast GID, starting from the given offset
   */
  [[nodiscard]] std::span<GID> get_multicast_groups(std::span<GID> buffer, std::size_t offset = 0, std::size_t count = std::dynamic_extent) const;

  /**
   * Check if a multicast group with the given GID is registered in the network.
   *
   * @param[in] gid GID of the group to check
   *
   * @return true if the group is registered, false otherwise
   */
  [[nodiscard]] bool is_multicast_group_registered(GID gid) const;

  /// @}

  /** @name UDP Packet Injection
   *  Allows to inject the packets directly to the network
   */
  ///@{
  /// Sends a UDP datagram via the given socket to the specified destination address and port.
  std::error_code send(Socket const &socket, Address const &dst, uint16_t dst_port, std::span<std::byte const> data);
  /// Registers a UDP socket with the border router to receive incoming datagrams.
  std::error_code register_socket(Socket &socket);
  /// Unregisters a previously registered UDP socket.
  std::error_code unregister_socket(Socket &socket);
  /// Returns true if the given socket is currently registered.
  [[nodiscard]] bool is_socket_registered(Socket const &socket) const;
  /// Returns the maximum datagram payload length for the given socket.
  [[nodiscard]] std::size_t get_max_datagram_length(Socket const &socket) const;
  /// @}

  /** @name Trust authority Management
   *  Manages internal Trust Authority
   */
  ///@{

  /**
   * Get the maximum number of user-defined join rules.
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t get_join_rules_capacity() const;

  /**
   * Get the current join rules count.
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t get_join_rules_count() const;

  /**
   * Get the list of the join rules in the network.
   *
   * @param[in] buffer user-provided buffer to store the join rules
   * @param[in] offset index of the first rule to retrieve
   * @param[in] count maximum number of rule to retrieve. If std::dynamic_extent, the function will fill the provided buffer as much as possible.
   *
   * @return subspan of the buffer containing at most count entries, filled with the current join rules, starting from the given offset
   */
  [[nodiscard]] std::span<JoinRule> get_join_rules(std::span<JoinRule> buffer, std::size_t offset = 0, std::size_t count = std::dynamic_extent) const;

  /**
   * Adds a new join rule.
   *
   * @param[in] rule rule to add
   *
   * @retval std::errc::not_enough_memory if there is no space left for new rules
   * @retval std::errc::file_exists if the rule already exists
   * @retval no_error on success
   */
  [[nodiscard]] std::error_code add_join_rule(JoinRule const &rule);

  /**
   * Removes a join rule.
   *
   * @param rule rule to remove
   * @return number of removed rules
   */
  std::error_code remove_join_rule(JoinRule const &rule);

  /**
   * Clears all user-defined join rules.
   */
  std::error_code clear_join_rules();

  /**
   * Obtain the current join policy.
   *
   * The join policy allows to limit the number of authentication requests that can be processed.
   *
   * @return valid JoinPolicy if the Trust Authority is enabled and the policy is active, std::nullopt otherwise
   */
  [[nodiscard]] std::optional<JoinPolicy> get_join_policy() const;

  /**
   * Set the join policy object
   *
   * @param[in] policy policy to be set. if std::nullopt, the policy will be removed, and all requests will be processed immediately.
   */
  std::error_code set_join_policy(std::optional<JoinPolicy> policy);

  /// @}

  /** @name Debugging
   *  Provides additional capabilities, not suitable for production use.
   */
  ///@{
  /**
   * Enable or disable promiscuous mode on the Border Router.
   * Enabling promiscuous mode will have the following effects:
   * - Verification of checksums of incoming packets from root node will be skipped
   * - Outgoing multicast packets will NOT be filtered based on the registered multicast groups
   */
  void configure_promiscuous_mode(bool enable = true);

  /// @}

  /// @cond DOXYGEN_SHOULD_IGNORE_THIS

  /// Declaration of internal implementation class for PIMPL idiom
  class Implementation;

  // Allow introspection of the implementation for testing purposes
  friend Implementation;

protected:
  ///  Pointer to the implementation type
  using PimplType = std::unique_ptr<Implementation, void (*)(Implementation *)>;

  /**
   * Construct a new Border Router object with the given implementation.
   * @param[in] impl pointer to implementation
   */
  explicit BorderRouter(PimplType impl) noexcept;

  /// Pointer to the implementation
  PimplType implementation;

  /// @endcond DOXYGEN_SHOULD_IGNORE_THIS
};

/**
 * @brief Returns the version string of the Border Router library.
 *
 * The string follows the standard semantic versioning format: MAJOR.MINOR.PATCH+METADATA
 *
 * @return char const*
 */
[[nodiscard]] char const *get_version_string();

/**
 * Assigns the BorderRouter instance to be used by the UDP API functions (EMBENET_UDP_Send and friends),
 * which have no instance parameter of their own.
 *
 * Called automatically by @ref EMBENET_BR_Init / @ref EMBENET_BR_Deinit for instances created through the C
 * API. Applications that create their own instance directly through the C++ API must call this themselves to
 * make it reachable from the UDP API.
 *
 * @param[in] instance pointer to the BorderRouter instance, or nullptr to disable UDP API functions
 */
void set_border_router_udp_instance(BorderRouter *instance);

/**
 * Returns the BorderRouter instance currently assigned via @ref embenet::border_router::set_border_router_udp_instance(), if any.
 *
 * Intended for internal wiring between the UDP API implementation and the BorderRouter instance backing it -
 * not meant for general application use.
 *
 * @return pointer to the assigned instance, or nullptr if none is assigned.
 */
[[nodiscard]] BorderRouter *get_border_router_udp_instance();

} // namespace embenet::border_router

///@}
///@}
#endif
