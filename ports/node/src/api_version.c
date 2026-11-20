/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Port API version marker
 *
 * EMBENET_NODE_Init() calls it, so a stack built for another major version of the port API fails to link against this
 * port instead of running with mismatched signatures.
 */

#include <embenet_port/api_version.h>

void EMBENET_PORT_API_V2(void) {}
