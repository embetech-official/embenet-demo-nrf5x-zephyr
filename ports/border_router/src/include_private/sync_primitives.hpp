/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     embeNET Border Router Zephyr port - shared synchronization primitive helpers.
 */

#ifndef EMBENET_BORDER_ROUTER_PORT_ZEPHYR_SYNC_PRIMITIVES_HPP
#define EMBENET_BORDER_ROUTER_PORT_ZEPHYR_SYNC_PRIMITIVES_HPP

#if __has_include(<embenet_port/condition_variable.h>)
#include <embenet_port/condition_variable.h>
#endif

#include <zephyr/kernel.h>

struct EMBENET_BR_PORT_Mutex {
  k_mutex mtx;
  bool in_use;
};

struct EMBENET_BR_PORT_ConditionVariable {
  k_condvar cv;
  bool in_use;
};

#endif
