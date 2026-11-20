/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief     embeNET Border Router timekeeping port interface.
 */

#ifndef EMBENET_BORDER_ROUTER_PORT_CLOCK_H
#define EMBENET_BORDER_ROUTER_PORT_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the current time milliseconds.
 *
 * @return Time in milliseconds.
 *
 * @post The returned values MUST be monotonic.
 */
int64_t EMBENET_BR_PORT_GetTime(void);

#ifdef __cplusplus
}
#endif

#endif
