/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET PORT API
 * @brief     Port API version marker for the EMBENET NODE Port
 */

#ifndef EMBENET_NODE_PORT_API_VERSION_H_
#define EMBENET_NODE_PORT_API_VERSION_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup embenet_node_port_api_version Port API Version Marker
 *
 * The C linker does not check function signatures, so a port written for another major version of the port API could link and
 * misbehave. Every port defines the marker of the major version it implements; a port for another version fails to link with
 * an undefined reference to this function.
 * @{
 */

/**
 * @brief Marks the port as implementing port API version 2.
 *
 * Called once by @c EMBENET_NODE_Init. The implementation is empty: @code void EMBENET_PORT_API_V2(void) {} @endcode
 */
void EMBENET_PORT_API_V2(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // EMBENET_NODE_PORT_API_VERSION_H_
