/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     embeNET Border Router Zephyr port - condition variable implementation.
 */

#if __has_include(<embenet_port/condition_variable.h>)
#include <embenet_port/condition_variable.h>
#endif

#include "sync_primitives.hpp"

#include <cstddef>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(embenet_br_cv, CONFIG_EMBENET_LOG_LEVEL);

namespace {
constexpr std::size_t max_condition_variables = 4;
EMBENET_BR_PORT_ConditionVariable condition_variable_pool[max_condition_variables];
k_spinlock condition_variable_pool_lock;
} // namespace

extern "C" {

EMBENET_BR_PORT_ConditionVariable *EMBENET_BR_PORT_ConditionVariableCreate(void) {
  EMBENET_BR_PORT_ConditionVariable *acquired = nullptr;
  k_spinlock_key_t const key = k_spin_lock(&condition_variable_pool_lock);
  for(auto &slot : condition_variable_pool) {
    if(!slot.in_use) {
      slot.in_use = true;
      acquired = &slot;
      break;
    }
  }
  k_spin_unlock(&condition_variable_pool_lock, key);
  if(acquired != nullptr) {
    k_condvar_init(&acquired->cv);
  }
  LOG_DBG("EMBENET_BR_PORT_ConditionVariableCreate() -> %p", static_cast<void *>(acquired));
  return acquired;
}

void EMBENET_BR_PORT_ConditionVariableDestroy(EMBENET_BR_PORT_ConditionVariable *condition_variable) {
  LOG_DBG("EMBENET_BR_PORT_ConditionVariableDestroy(%p)", static_cast<void *>(condition_variable));
  k_spinlock_key_t const key = k_spin_lock(&condition_variable_pool_lock);
  condition_variable->in_use = false;
  k_spin_unlock(&condition_variable_pool_lock, key);
}

void EMBENET_BR_PORT_ConditionVariableNotifyOne(EMBENET_BR_PORT_ConditionVariable *condition_variable) {
  LOG_DBG("EMBENET_BR_PORT_ConditionVariableNotifyOne(%p)", static_cast<void *>(condition_variable));
  k_condvar_signal(&condition_variable->cv);
}

bool EMBENET_BR_PORT_ConditionVariableWaitFor(EMBENET_BR_PORT_ConditionVariable *condition_variable, EMBENET_BR_PORT_Mutex *mutex,
                                              int64_t timeout_ms) {
  bool const woken = k_condvar_wait(&condition_variable->cv, &mutex->mtx, (timeout_ms <= 0) ? K_NO_WAIT : K_MSEC(timeout_ms)) == 0;
  LOG_DBG("EMBENET_BR_PORT_ConditionVariableWaitFor(%p, %p, %lld) -> %d", static_cast<void *>(condition_variable), static_cast<void *>(mutex),
          static_cast<long long>(timeout_ms), woken);
  return woken;
}
}
