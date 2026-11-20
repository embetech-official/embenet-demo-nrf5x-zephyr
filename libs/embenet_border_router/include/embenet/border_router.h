/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief     embeNET Border Router C API.
 */

#ifndef EMBENET_BORDER_ROUTER_H
#define EMBENET_BORDER_ROUTER_H

#include <embenet/border_router_types.h> // IWYU pragma: export
#include <embenet/udp.h>                 // IWYU pragma: export
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
/** @addtogroup embenet_border_router_c_api embeNET Border Router C API
 *  @{
 */

/** @name Control Interface
 *  Controls the Border Router operation
 *  @{
 */

/**
 * Return the hard limits of the Border Router instance.
 * The limits include the maximum number of nodes, groups, and amount of join rules that are possible to define.
 *
 * The limits differ between the licensing variants of the library.
 *
 * @return EMBENET_BR_Capabilities
 */
EMBENET_BR_Capabilities EMBENET_BR_GetMaxCapabilities(void);

/**
 * Return the default capabilities of the Border Router instance.
 * The default capabilities are a balanced set of parameters that should fit most use cases.
 *
 * @return EMBENET_BR_Capabilities
 */
EMBENET_BR_Capabilities EMBENET_BR_GetDefaultCapabilities(void);

/**
 * Initializes the Border Router.
 * This function must be called before any other Border Router function.
 * The Border Router is initialized in internal, static storage.
 * This API is limited to one Border Router instance. If you need multiple instances, use the C++ API.
 *
 * @param[in] capabilities requested capabilities of this instance. If unsure, pass the result of @ref EMBENET_BR_GetDefaultCapabilities.
 * When customizing, make sure that the requested capabilities do not exceed the maximum capabilities of the library - see @ref
 * EMBENET_BR_GetMaxCapabilities.
 *
 * @return EMBENET_BR_Result
 *
 * @note calling this function will also enable calling of the EMBENET UDP API functions.
 */
EMBENET_BR_Result EMBENET_BR_Init(EMBENET_BR_Capabilities capabilities);

/**
 * Deinitialize the border router network process.
 * This function stops and deinitialize the Border Router, and frees any allocated resources.
 *
 * After this call, no other Border Router function must be called except for @ref EMBENET_BR_Init.
 */
void EMBENET_BR_Deinit(void);

/**
 * Subscribes to the Border Router network events.
 *
 * @param[in] event_handlers structure containing event handler callbacks. When nullptr, the subscription is removed
 * @param[in] context user-provided context to be passed to the handlers
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_SubscribeToEvents(EMBENET_BR_EventHandlers event_handlers, void *context);

/**
 * Configures the Border Router with the provided network configuration.
 *
 * @param[in] root_node_interface name of the root node communication interface.
 * @param[in] config pointer to Network configuration.
 * @param[in] wan_interface name of the system-wide WAN interface.
 *
 * @note It is allowed to set wan_interface to "", in which case the Border Router will not forward the LOWPAN traffic to the WAN.
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_Configure(char const *root_node_interface, EMBENET_BR_NetworkParameters const *config, char const *wan_interface);

/**
 * Starts the Border Router operation.
 *
 * This call is non-blocking: it returns as soon as the WAN/root node interfaces have been opened and the
 * connection attempt to the Root Node has been initiated, without waiting for the Root Node to actually
 * confirm the network start. Outcome of that attempt (success or failure) is reported asynchronously via
 * events (on_network_started / on_root_node_connection_failed in @ref EMBENET_BR_EventHandlers), so it can be
 * observed the same way on every platform this library targets - including bare-metal ports with no
 * OS-level blocking primitive to wait on.
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_Up(void);

/**
 * Stops the network operation.
 * User will no longer be able to send and/or receive packets from the network. The list of connected nodes and
 * registered multicast groups will be cleared. List of join rules will not be cleared.
 */
EMBENET_BR_Result EMBENET_BR_Down(void);

/**
 * Handle border router processing.
 * This function MUST be called periodically to process the incoming and outgoing packets
 */
EMBENET_BR_Result EMBENET_BR_Proc(void);
/** @} */

/** @name Network Topology Information
 *  Obtain information about the network topology
 *  @{
 */

/**
 * Returns the address of the root node, if present.
 *
 * @return Root Node Address.
 */
EMBENET_IPV6 EMBENET_BR_GetRootNodeAddress(void);

/**
 * Gets the IPv6 Network Prefix (First 8 Bytes).
 *
 * @return Network prefix
 */
EMBENET_NetworkPrefix EMBENET_BR_GetNetworkPrefix(void);

/**
 * Returns the address of the WAN interface, if present.
 *
 * @return WAN address
 */
EMBENET_IPV6 EMBENET_BR_GetWanAddress(void);

/**
 * Returns the maximum number of nodes the router can handle.
 *
 * @return size_t Maximum number of nodes.
 */
size_t EMBENET_BR_GetNodesCapacity(void);

/**
 * Return the current node count.
 *
 * @return node count
 */
size_t EMBENET_BR_GetNodesCount(void);

/**
 * Get the list of the nodes EUI64 in the network.
 *
 * @param[out] nodes_buffer Pointer to memory location where the UIDs will be written.
 *                         If NULL, no copying will occur, and EMBENET_BR_NodesList::count will be zero.
 * @param[in] nodes_buffer_size Number of elements that can be stored in nodes_buffer.
 * @param[in] offset Index of the first node to retrieve.
 * @param[in] count Maximum number of nodes to retrieve.
 *                  If SIZE_MAX - fills as much as possible.
 *
 * @return EMBENET_BR_NodesList:
 * - EMBENET_BR_NodesList::total_count will contain the total number of nodes in the network.
 * - EMBENET_BR_NodesList::count will contain the number of UIDs written to the buffer.
 * - EMBENET_BR_NodesList::nodes will point to the first node entry written to the buffer.
 * If the buffer is too small to hold all entries, only as many as fit will be written, and total_count > count.
 *
 * @note The nodes list may change between subsequent calls to this function.
 */
EMBENET_BR_NodesList EMBENET_BR_GetNodes(EMBENET_EUI64 *nodes_buffer, size_t nodes_buffer_size, size_t offset, size_t count);

/**
 * Checks if a node with the given UID is present in the network.
 *
 * @param[in] uid UID of the node to check.
 *
 * @return true if the node is present, false otherwise.
 */
bool EMBENET_BR_IsNodePresent(EMBENET_EUI64 uid);

/**
 * Gets detailed information about the node with the given UID.
 *
 * The returned information contains the parent node UID, last communication timestamp, and the list of multicast groups the node is registered to.
 *
 * @param[in] uid UID of the node to query.
 *
 * @return EMBENET_BR_NodeDetails
 */
EMBENET_BR_NodeDetails EMBENET_BR_GetNodeDetails(EMBENET_EUI64 uid);

/**
 * Return the duration of node inactivity, after which the node is considered inactive.
 *
 * @return Duration of inactivity timeout in seconds.
 */
int32_t EMBENET_BR_GetNodesInactivityTimeout(void);

/**
 * Sets the nodes inactivity timeout (in seconds).
 *
 * @param[in] timeout_sec Timeout duration in seconds. If set to 0, the timeout is disabled
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_SetNodesInactivityTimeout(int32_t timeout_sec);

/**
 * Get the maximum number of multicast groups the router can handle.
 *
 * @return capacity of the multicast groups list
 */
size_t EMBENET_BR_GetMulticastGroupCapacity(void);

/**
 * Returns the current number of registered multicast groups.
 *
 * @return Number of registered multicast groups in entire network.
 */
size_t EMBENET_BR_GetMulticastGroupCount(void);

/**
 * Get the list of the registered multicast groups in the network.
 *
 * @param[out] groups_buffer User-provided buffer to store group IDs.
 *                          if NULL, no copying will occur, and EMBENET_BR_GroupsList::count will be zero.
 * @param[in] groups_buffer_size Number of entries groups_buffer can store
 * @param[in] offset Index of the first group to retrieve.
 * @param[in] count Maximum number of groups to retrieve.
 *                  If SIZE_MAX, fills as much as possible.
 *
 * @return EMBENET_BR_GroupsList:
 * - EMBENET_BR_GroupsList::total_count will contain the total number of registered multicast groups in the network.
 * - EMBENET_BR_GroupsList::count will contain the number of group IDs written to the buffer.
 * - EMBENET_BR_GroupsList::groups will point to the first group ID written to the buffer.
 * If the buffer is too small to hold all entries, only as many as fit will be written, and total_count > count.
 *
 * @note The group list may change between subsequent calls to this function.
 */
EMBENET_BR_GroupsList EMBENET_BR_GetMulticastGroups(EMBENET_GroupId *groups_buffer, size_t groups_buffer_size, size_t offset, size_t count);

/**
 * Checks if a multicast group with the given GID is registered.
 *
 * @param[in] gid Group ID to check.
 *
 * @return true if the group is registered, false otherwise.
 */
bool EMBENET_BR_IsMulticastGroupRegistered(EMBENET_GroupId gid);

/** @} */

/** @name UDP Packet Injection
 *  Allows to inject the packets directly to the network
 *  @{
 */

/**
 * Injects a UDP packet into the network.
 *
 * @param[in] socket Pointer to the UDP socket descriptor.
 * @param[in] dst Pointer to the destination IPv6 address.
 * @param[in] dst_port Destination port.
 * @param[in] data Pointer to the data to be sent.
 * @param[in] data_len Length of the data.
 *
 * @return EMBENET_BR_ResultCode
 */
EMBENET_BR_Result EMBENET_BR_Send(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *dst, uint16_t dst_port, void const *data,
                                  size_t data_len);

/**
 * Registers a UDP socket.
 *
 * @param[in] socket Pointer to the UDP socket descriptor.
 *
 * @return EMBENET_BR_ResultCode
 */
EMBENET_BR_Result EMBENET_BR_RegisterSocket(EMBENET_UDP_SocketDescriptor *socket);

/**
 * Unregisters a UDP socket.
 *
 * @param[in] socket Pointer to the UDP socket descriptor.
 *
 * @return EMBENET_BR_ResultCode
 */
EMBENET_BR_Result EMBENET_BR_UnregisterSocket(EMBENET_UDP_SocketDescriptor *socket);

/**
 * Returns true if the socket is registered.
 *
 * @param[in] socket Pointer to the UDP socket descriptor.
 *
 * @return true if registered, false otherwise.
 */
bool EMBENET_BR_IsSocketRegistered(EMBENET_UDP_SocketDescriptor const *socket);

/**
 * Returns the max datagram length for the socket.
 *
 * @param[in] socket Pointer to the UDP socket descriptor.
 *
 * @return size_t Maximum datagram length.
 */
size_t EMBENET_BR_GetMaxDatagramLength(EMBENET_UDP_SocketDescriptor const *socket);
/** @} */

/** @name Trust Authority Management
 *  Manages internal Trust Authority
 *  @{
 */

/**
 * Gets the maximum number of user-defined join rules.
 *
 * @return Maximum number of join rules.
 */
size_t EMBENET_BR_GetJoinRulesCapacity(void);

/**
 * Gets the number of constrained join rules currently added to the list of rules.
 * See @ref EMBENET_BR_JoinRule for more information on joining rules.
 *
 * @return number of rules currently on the rule list
 */
size_t EMBENET_BR_GetJoinRulesCount(void);

/**
 * Gets the list of join rules in the network.
 *
 * @param[out] rules_buffer User-provided buffer to store the join rules.
 * @param[in] rules_buffer_size Number of entries that can be stored in the rules_buffer.
 * @param[in] offset Index of the first rule to retrieve.
 * @param[in] count Maximum number of rules to retrieve. If 0, fills as much as possible.
 *
 * @return EMBENET_BR_JoinRulesList.
 */
EMBENET_BR_JoinRulesList EMBENET_BR_GetJoinRules(EMBENET_BR_JoinRule *rules_buffer, size_t rules_buffer_size, size_t offset, size_t count);

/**
 * Adds a constrained join rule, governing the joining process of remote nodes.
 * See @ref EMBENET_BR_JoinRule for more information on how joining rules work.
 *
 * @param[in] rule pointer to a structure describing the rule to be added. The rule's storage
 * duration may be automatic, as its content will be copied to internal storage
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_AddJoinRule(EMBENET_BR_JoinRule *rule);

/**
 * Removes a constrained join rule from the list of rules.
 * See @ref EMBENET_BR_JoinRule for more information on joining rules.
 *
 * @param[in] rule pointer to the entry to be removed. The rule's storage duration may
 * be automatic
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_RemoveJoinRule(EMBENET_BR_JoinRule *rule);

/**
 * Removes all user-defined join rules.
 */
EMBENET_BR_Result EMBENET_BR_ClearJoinRules(void);

/**
 * Retrieves the current join policy.
 *
 * @return EMBENET_BR_JoinPolicy. If error occurs, the values will be negative
 */
EMBENET_BR_JoinPolicy EMBENET_BR_GetJoinPolicy();

/**
 * Sets the join policy. If policy == NULL, removes the current policy.
 *
 * @param[in] policy Pointer to the policy structure to set, or NULL to remove the policy.
 */
EMBENET_BR_Result EMBENET_BR_SetJoinPolicy(EMBENET_BR_JoinPolicy const *policy);
/** @} */

/** @name Debugging
 *  Provides additional capabilities, not suitable for production use.
 *  @{
 */
/**
 * Configures the promiscuous mode of the Border Router.
 * In promiscuous mode, checksum verification will be skipped for LOWPAN packets.
 *
 * @param[in] enable true to enable promiscuous mode, false to disable it.
 *
 * @return EMBENET_BR_Result
 */
EMBENET_BR_Result EMBENET_BR_ConfigurePromiscuousMode(bool enable);

/**
 * Returns the version string of the Border Router recipient.
 * @return char const*
 */
char const *EMBENET_BR_GetVersionString(void);
/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif // EMBENET_BR_H_
