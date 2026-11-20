/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief embeNET Border Router random number generation port interface.
 */
#ifndef EMBENET_BORDER_ROUTER_PORT_RANDOM_H
#define EMBENET_BORDER_ROUTER_PORT_RANDOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns uniformly distributed random number from range [0, UINT32_MAX].
 *
 * @return Uniformly distributed random number from range [0, UINT32_MAX].
 */
uint32_t EMBENET_BR_PORT_GetRandom(void);

#ifdef __cplusplus
}
#endif

#endif
