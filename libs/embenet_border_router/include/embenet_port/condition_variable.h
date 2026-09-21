/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief embeNET Border Router condition variable port interface.
 */
#ifndef EMBENET_BORDER_ROUTER_PORT_CONDITION_VARIABLE_H
#define EMBENET_BORDER_ROUTER_PORT_CONDITION_VARIABLE_H

#include <embenet_port/mutex.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type alias to port-defined structure that represents a condition variable.
 */
typedef struct EMBENET_BR_PORT_ConditionVariable EMBENET_BR_PORT_ConditionVariable;

/**
 * Returns a new condition variable instance.
 *
 * @return new condition variable instance, or NULL if it cannot be created.
 *
 * @post The returned instance is valid until it is destroyed by @ref EMBENET_BR_PORT_ConditionVariableDestroy().
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
EMBENET_BR_PORT_ConditionVariable *EMBENET_BR_PORT_ConditionVariableCreate(void);

/**
 * Releases the condition variable instance.
 *
 * @param[in] condition_variable Instance to be destroyed.
 *
 * @pre condition_variable was obtained from @ref EMBENET_BR_PORT_ConditionVariableCreate().
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
void EMBENET_BR_PORT_ConditionVariableDestroy(EMBENET_BR_PORT_ConditionVariable *condition_variable);

/**
 * Wakes up one thread (if any) currently blocked in @ref EMBENET_BR_PORT_ConditionVariableWaitFor() on this
 * condition variable. Always safe to call, any number of times, whether or not a thread is currently waiting.
 *
 * @param[in] condition_variable Instance to notify.
 *
 * @pre condition_variable was obtained from @ref EMBENET_BR_PORT_ConditionVariableCreate().
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
void EMBENET_BR_PORT_ConditionVariableNotifyOne(EMBENET_BR_PORT_ConditionVariable *condition_variable);

/**
 * Atomically unlocks @p mutex and blocks the calling thread until either another thread calls
 * @ref EMBENET_BR_PORT_ConditionVariableNotifyOne() on @p condition_variable, or @p timeout_ms elapses. In both cases (and also
 * on a spurious wakeup), @p mutex is re-locked before this function returns - the caller must always re-check
 * its own predicate after this function returns, regardless of the returned value.
 *
 * A port that cannot actually block is allowed to return immediately (as if woken by a notification, or as if
 * the timeout expired) without violating this contract.
 *
 * @param[in] condition_variable Instance to wait on.
 * @param[in] mutex Mutex, currently locked by the calling thread, to unlock while waiting and re-lock before
 * returning.
 * @param[in] timeout_ms Maximum time to wait, in milliseconds. If zero, the function must not block.
 *
 * @pre condition_variable was obtained from @ref EMBENET_BR_PORT_ConditionVariableCreate().
 * @pre mutex was obtained from @ref EMBENET_BR_PORT_MutexCreate() and is currently locked by the calling thread.
 *
 * @return true if woken by a notification (or spuriously), false if @p timeout_ms elapsed first.
 *
 * @post mutex is locked by the calling thread again.
 *
 * @warning The implementation of this function MUST be thread-safe.
 */
bool EMBENET_BR_PORT_ConditionVariableWaitFor(EMBENET_BR_PORT_ConditionVariable *condition_variable, EMBENET_BR_PORT_Mutex *mutex,
                                                                           int64_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
