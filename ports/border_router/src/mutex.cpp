/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @brief     embeNET Border Router Zephyr port - mutex implementation.
 */

#if __has_include(<embenet_port/mutex.h>)
#include <embenet_port/mutex.h>
#endif

#include "sync_primitives.hpp"

#include <cstddef>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(embenet_br_mutex, CONFIG_EMBENET_LOG_LEVEL);

namespace {
constexpr std::size_t max_mutexes = 4;
EMBENET_BR_PORT_Mutex mutex_pool[max_mutexes];
k_spinlock mutex_pool_lock;
} // namespace

extern "C" {

EMBENET_BR_PORT_Mutex *EMBENET_BR_PORT_MutexCreate(void) {
  EMBENET_BR_PORT_Mutex *acquired = nullptr;
  k_spinlock_key_t const key = k_spin_lock(&mutex_pool_lock);
  for(auto &slot : mutex_pool) {
    if(!slot.in_use) {
      slot.in_use = true;
      acquired = &slot;
      break;
    }
  }
  k_spin_unlock(&mutex_pool_lock, key);
  if(acquired != nullptr) {
    k_mutex_init(&acquired->mtx);
  }
  LOG_DBG("EMBENET_BR_PORT_MutexCreate() -> %p", static_cast<void *>(acquired));
  return acquired;
}

void EMBENET_BR_PORT_MutexDestroy(EMBENET_BR_PORT_Mutex *mutex) {
  LOG_DBG("EMBENET_BR_PORT_MutexDestroy(%p)", static_cast<void *>(mutex));
  k_spinlock_key_t const key = k_spin_lock(&mutex_pool_lock);
  mutex->in_use = false;
  k_spin_unlock(&mutex_pool_lock, key);
}

void EMBENET_BR_PORT_MutexLock(EMBENET_BR_PORT_Mutex *mutex) {
  LOG_DBG("EMBENET_BR_PORT_MutexLock(%p)", static_cast<void *>(mutex));
  k_mutex_lock(&mutex->mtx, K_FOREVER);
}

void EMBENET_BR_PORT_MutexUnlock(EMBENET_BR_PORT_Mutex *mutex) {
  LOG_DBG("EMBENET_BR_PORT_MutexUnlock(%p)", static_cast<void *>(mutex));
  k_mutex_unlock(&mutex->mtx);
}

bool EMBENET_BR_PORT_MutexTryLock(EMBENET_BR_PORT_Mutex *mutex) {
  bool const acquired = k_mutex_lock(&mutex->mtx, K_NO_WAIT) == 0;
  LOG_DBG("EMBENET_BR_PORT_MutexTryLock(%p) -> %d", static_cast<void *>(mutex), acquired);
  return acquired;
}
}
