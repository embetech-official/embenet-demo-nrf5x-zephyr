/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Capability template the stack runs with
 */

#include <embenet_port/capabilities.h>

// 2.4 GHz: the only band nRF52 and nRF54 radios support.
EMBENET_Config const *EMBENET_CAPABILITIES_Init(void) { return embenetNodeConfigTemplate_2400_2480Mhz_BLE_PHY_10nodes_demo; }
