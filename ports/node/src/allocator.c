/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Allocator
 */

#include <embenet_port/allocator.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(embenet_allocator, CONFIG_EMBENET_LOG_LEVEL);

// The stack allocates once during EMBENET_NODE_Init() and frees once during Deinit. The system heap
// covers it - CONFIG_HEAP_MEM_POOL_ADD_SIZE_EMBENET_NODE reserves the space.
void *EMBENET_ALLOCATOR_Alloc(size_t size) {
  void *allocation = k_malloc(size);
  if(allocation == NULL) {
    LOG_ERR("cannot allocate %zu bytes - raise CONFIG_HEAP_MEM_POOL_ADD_SIZE_EMBENET_NODE", size);
  }
  return allocation;
}

void EMBENET_ALLOCATOR_Free(void *pool) { k_free(pool); }
