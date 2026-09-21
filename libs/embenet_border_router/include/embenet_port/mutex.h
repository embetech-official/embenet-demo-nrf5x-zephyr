/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief embeNET Border Router mutual exclusion port interface.
 */
#ifndef EMBENET_BORDER_ROUTER_PORT_MUTEX_H
#define EMBENET_BORDER_ROUTER_PORT_MUTEX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type alias to port-defined structure that represents a mutex.
 */
typedef struct EMBENET_BR_PORT_Mutex EMBENET_BR_PORT_Mutex;

/**
 * Returns a new mutex instance.
 *
 * @return new mutex instance, or NULL if the mutex cannot be created.
 *
 * @post The returned instance is valid until it is destroyed by @ref EMBENET_BR_PORT_MutexDestroy().
 * @post The mutex instance is in an UNLOCKED state.
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
EMBENET_BR_PORT_Mutex *EMBENET_BR_PORT_MutexCreate(void);

/**
 * Releases the mutex instance.
 *
 * @param[in] mutex Instance to be destroyed.
 *
 * @pre mutex was obtained from @ref EMBENET_BR_PORT_MutexCreate().
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
void EMBENET_BR_PORT_MutexDestroy(EMBENET_BR_PORT_Mutex *mutex);

/**
 * Locks the mutex.
 * If another thread has already locked the mutex, the function MUST block execution until the lock is acquired.
 *
 * @param[in] mutex Instance to be locked.
 *
 * @pre mutex was obtained from @ref EMBENET_BR_PORT_MutexCreate().
 *
 * @post The mutex instance is in a LOCKED state.
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
void EMBENET_BR_PORT_MutexLock(EMBENET_BR_PORT_Mutex *mutex);

/**
 * Unlocks the mutex.
 *
 * @param[in] mutex Instance to be unlocked.
 *
 * @pre mutex was obtained from @ref EMBENET_BR_PORT_MutexCreate().
 * @pre The mutex instance MUST be in a LOCKED state.
 *
 * @post The mutex is in an UNLOCKED state.
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
void EMBENET_BR_PORT_MutexUnlock(EMBENET_BR_PORT_Mutex *mutex);

/**
 * Tries to lock the mutex. Returns immediately. On successful lock acquisition returns true, otherwise returns false.
 * This function is allowed to fail spuriously and return false even if the mutex is not currently locked by any other thread.
 *
 * @param[in] mutex Instance to be locked.
 *
 * @pre mutex was obtained from @ref EMBENET_BR_PORT_MutexCreate().
 *
 * @return true if the lock was acquired successfully, otherwise false.
 *
 * @post The mutex instance is in a LOCKED state if the function returns true.
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
bool EMBENET_BR_PORT_MutexTryLock(EMBENET_BR_PORT_Mutex *mutex);

#ifdef __cplusplus
}
#endif

#endif
