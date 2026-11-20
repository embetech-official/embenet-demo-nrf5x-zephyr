/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     The stack's multiprotocol port interface, on top of the MPSL timeslot adapter
 *
 * Implements embenet_port/multiprotocol.h with the adapter in multiprotocol.c:
 *
 *     stack                                   adapter
 *       EMBENET_MULTIPROTOCOL_Request()  -->   embenet_multiprotocol_cancel() + _reserve()
 *       EMBENET_MULTIPROTOCOL_Release()  -->   embenet_multiprotocol_release()
 *       EMBENET_MULTIPROTOCOL_OnGranted()    <--   on_granted()
 *       EMBENET_MULTIPROTOCOL_OnEndGuard()   <--   on_revoked()
 *                                              on_denied()    - nothing: denial is silent
 *
 * This file is built only with CONFIG_EMBENET_NODE_MULTIPROTOCOL. Its calls to EMBENET_MULTIPROTOCOL_OnGranted() and
 * EMBENET_MULTIPROTOCOL_OnEndGuard() are what link the stack's multiprotocol support in; a node built without it implements
 * nothing from embenet_port/multiprotocol.h and the stack owns the transceiver.
 *
 * The adapter raises on_revoked() EMBENET_MULTIPROTOCOL_END_GUARD_US before the end of what it booked, and hands the
 * timeslot back right after. The stack wants its end guard endGuardUs before the end of the timeslot it asked for. So
 * each timeslot is booked shorter by the difference: its end guard comes when the stack expects it, and the time after
 * - which the stack does not use anyway - goes to the other protocol.
 *
 * @note The stack's callbacks are run from MPSL's zero latency interrupt, which the contract allows.
 */

#include "multiprotocol.h"
#include "timer_counter.h"

#include <embenet_port/multiprotocol.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

LOG_MODULE_REGISTER(node_multiprotocol_port, CONFIG_EMBENET_LOG_LEVEL);

/// Set by Init(), cleared by Deinit(), read in the zero latency interrupt: the stack is to be called back.
static bool volatile stackActive;
static void *volatile callbackContext;
static uint32_t endGuardUs;

/// The adapter opens its MPSL session once for the lifetime of the image; a node restarted by the border router
/// reinitializes the stack, not the session.
static bool adapterReady;

/// Requests the adapter would not take, by reason - for diagnostics.
static atomic_t refusedTooLate;
static atomic_t refusedOther;

static void adapterGranted(uint64_t start_us, void *context) {
  ARG_UNUSED(start_us);
  ARG_UNUSED(context);
  if(stackActive) {
    EMBENET_MULTIPROTOCOL_OnGranted(callbackContext);
  } else {
    embenet_multiprotocol_release(); // the stack is gone; it would refuse the timeslot anyway
  }
}

static void adapterRevoked(void *context) {
  ARG_UNUSED(context);
  if(stackActive) {
    EMBENET_MULTIPROTOCOL_OnEndGuard(callbackContext);
  }
}

static void adapterDenied(uint64_t start_us, MultiprotocolDenial reason, void *context) {
  ARG_UNUSED(start_us);
  ARG_UNUSED(reason);
  ARG_UNUSED(context);
  // Silent by contract: the stack finds no grant when its slot comes and skips the activity.
}

bool EMBENET_MULTIPROTOCOL_Init(void *context, uint32_t newEndGuardUs) {
  static MultiprotocolClient const client = {
      .on_granted = adapterGranted,
      .on_denied = adapterDenied,
      .on_revoked = adapterRevoked,
      .context = NULL,
  };

  endGuardUs = newEndGuardUs;
  callbackContext = context; // before the flag: the interrupt reads it after finding the flag set
  stackActive = true;

  if(!adapterReady) {
    int32_t const err = embenet_multiprotocol_init(&client);
    if(0 != err) {
      // Every request is then refused, so the node never gets the transceiver - visible, but only here. Still true: the
      // Bluetooth controller owns the radio too, so letting the stack drive it outright would be worse than a silent node.
      LOG_ERR("multiprotocol adapter not available: %d", (int)err);
      return true;
    }
    adapterReady = true;
  }
  return true;
}

void EMBENET_MULTIPROTOCOL_Deinit(void) {
  stackActive = false;
  (void)embenet_multiprotocol_cancel();
  embenet_multiprotocol_release();
}

void EMBENET_MULTIPROTOCOL_Request(uint32_t nowUs, uint32_t timestampUs, uint32_t durationUs) {
  ARG_UNUSED(nowUs); // the conversion reads the two clocks itself, closer together than the stack and this call can

  if(!adapterReady || (durationUs <= endGuardUs)) {
    atomic_inc(&refusedOther);
    return;
  }

  // A new request replaces the one still waiting to be handed to MPSL; one handed over already stands.
  (void)embenet_multiprotocol_cancel();

  uint32_t const lengthUs = durationUs - endGuardUs + EMBENET_MULTIPROTOCOL_END_GUARD_US;
  int const err = embenet_multiprotocol_reserve(embenet_port_timer_to_counter(timestampUs), lengthUs);
  if(-ETIME == err) {
    atomic_inc(&refusedTooLate); // the slot is too close for MPSL to schedule: not granted, by contract silently
  } else if(0 != err) {
    atomic_inc(&refusedOther);
  }
}

void EMBENET_MULTIPROTOCOL_Release(void) { embenet_multiprotocol_release(); }

void embenet_multiprotocol_get_stats(MultiprotocolStats *stats) {
  embenet_multiprotocol_adapter_get_stats(stats);
  stats->refused_too_late = (uint32_t)atomic_get(&refusedTooLate);
  stats->refused_other = (uint32_t)atomic_get(&refusedOther);
}
