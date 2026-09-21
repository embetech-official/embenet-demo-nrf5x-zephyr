/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Random number generator
 *
 * Called once (twice on a zero seed reroll) from EMBENET_NODE_Init(), in thread context, to seed
 * the stack's internal LFSR - not from an ISR, so the default random subsystem is fine as-is.
 */

#include <embenet_port/random.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(node_rnd, CONFIG_EMBENET_LOG_LEVEL);

uint32_t EMBENET_RANDOM_Get(void) { return sys_rand32_get(); }
