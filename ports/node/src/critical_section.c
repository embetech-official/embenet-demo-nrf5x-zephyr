/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Critical section
 */

#include <embenet_port/critical_section.h>

#include <zephyr/irq.h>

// Enter/Exit carry no lock key, so the nesting depth and the outermost key are tracked here. Only the
// outermost Exit() restores interrupts, and to their previous state - which makes this safe to call
// from an interrupt handler.
static unsigned int outermost_key;
static int nest_depth;

void EMBENET_CRITICAL_SECTION_Enter(void) {
  unsigned int const key = irq_lock();
  if(nest_depth == 0) {
    outermost_key = key;
  }
  ++nest_depth;
}

void EMBENET_CRITICAL_SECTION_Exit(void) {
  --nest_depth;
  if(nest_depth <= 0) {
    // Clamped: an unbalanced Exit() would otherwise leave interrupts locked for good.
    nest_depth = 0;
    irq_unlock(outermost_key);
  }
}
