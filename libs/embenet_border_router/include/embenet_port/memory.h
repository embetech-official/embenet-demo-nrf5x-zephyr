/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief embeNET Border Router dynamic memory allocation port interface.
 */
#ifndef EMBENET_BORDER_ROUTER_PORT_MEMORY_H
#define EMBENET_BORDER_ROUTER_PORT_MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Allocates size bytes of uninitialized storage.
 *
 * If allocation succeeds, the function SHALL return a pointer to the lowest (first) byte in the allocated memory block that is suitably aligned for
 * any scalar type (at least as strictly as max_align_t). If the allocation fails, the function SHALL return a null pointer.
 *
 * @param[in] size Number of bytes to allocate.
 *
 * @pre size != 0.
 *
 * @return Pointer to the beginning of newly allocated memory, or NULL on failure.
 *
 * @post The memory block is valid until it is deallocated by @ref EMBENET_BR_PORT_Deallocate().
 * @post The memory block is aligned to the largest alignment requirement of any scalar type.
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
void *EMBENET_BR_PORT_Allocate(size_t size);

/**
 * Deallocates the space previously allocated by @ref EMBENET_BR_PORT_Allocate().
 *
 * @param[in] ptr Pointer to the memory to deallocate.
 *
 * @pre ptr was obtained from @ref EMBENET_BR_PORT_Allocate().
 *
 * @warning The implementation of this function MUST be thread-safe.
 *
 */
void EMBENET_BR_PORT_Deallocate(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
