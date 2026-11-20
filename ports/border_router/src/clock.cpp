/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @brief     embeNET Border Router Zephyr port - timekeeping implementation.
 */

#if __has_include(<embenet_port/clock.h>)
#include <embenet_port/clock.h>
#endif

#include <zephyr/kernel.h>

extern "C" {
int64_t EMBENET_BR_PORT_GetTime(void) { return k_uptime_get(); }
}
