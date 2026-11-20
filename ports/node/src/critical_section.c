/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Critical section
 */

#include <embenet_port/critical_section.h>

#include <cmsis_core.h>
#include <zephyr/irq.h>

#include <stdbool.h>
#include <stdint.h>

// Enter/Exit carry no lock key, so the nesting depth and the outermost key are tracked here. Only the
// outermost Exit() restores interrupts, and to their previous state - which makes this safe to call
// from an interrupt handler.
static unsigned int outermost_key;
static int nest_depth;

/**
 * Whether the caller runs in a zero latency interrupt - MPSL's, with the stack's multiprotocol callbacks in it.
 *
 * irq_lock() does not mask those, so one can preempt Enter()/Exit() anywhere: between Exit() dropping the depth to zero and
 * restoring the key, its own Enter() overwrites the key with the locked level, and the interrupted Exit() then "restores"
 * interrupts to locked - for good. Measured on an nRF52833, whose timer read takes this lock: the stack's timer stopped. They
 * need no lock either, as nothing that touches the stack's data can preempt them, so they skip the bookkeeping altogether.
 */
static bool in_zero_latency_irq(void) {
#if defined(CONFIG_ZERO_LATENCY_IRQS)
  uint32_t const exception = __get_IPSR();
  // Below 16: thread mode or a system exception; from 16 on: an external interrupt, at priority 0 only if zero latency.
  return (exception >= 16U) && (0U == NVIC_GetPriority((IRQn_Type)(exception - 16U)));
#else
  return false;
#endif
}

void EMBENET_CRITICAL_SECTION_Enter(void) {
  if(in_zero_latency_irq()) {
    return;
  }
  unsigned int const key = irq_lock();
  if(nest_depth == 0) {
    outermost_key = key;
  }
  ++nest_depth;
}

void EMBENET_CRITICAL_SECTION_Exit(void) {
  if(in_zero_latency_irq()) {
    return;
  }
  --nest_depth;
  if(nest_depth <= 0) {
    // Clamped: an unbalanced Exit() would otherwise leave interrupts locked for good.
    nest_depth = 0;
    irq_unlock(outermost_key);
  }
}
