/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     embeNET Border Router Zephyr port - random number generator implementation.
 */

#if __has_include(<embenet_port/random.h>)
#include <embenet_port/random.h>
#endif

#include <zephyr/random/random.h>

extern "C" {
uint32_t EMBENET_BR_PORT_GetRandom(void) { return sys_rand32_get(); }
}
