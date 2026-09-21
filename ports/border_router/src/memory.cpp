/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     embeNET Border Router Zephyr port - memory allocation implementation.
 */

#if __has_include(<embenet_port/memory.h>)
#include <embenet_port/memory.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(embenet_br_mem, CONFIG_EMBENET_LOG_LEVEL);

extern "C" {

void *EMBENET_BR_PORT_Allocate(size_t size) {
  auto *ptr = k_malloc(size);
  LOG_DBG("EMBENET_BR_PORT_Allocate(%zu) -> %p", size, ptr);
  return ptr;
}

void EMBENET_BR_PORT_Deallocate(void *ptr) {
  LOG_DBG("EMBENET_BR_PORT_Deallocate(%p)", ptr);
  k_free(ptr);
}
}
