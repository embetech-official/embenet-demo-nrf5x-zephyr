/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET PORT API
 * @brief     Multiprotocol interface for the EMBENET NODE Port
 *
 * Lets the stack share the transceiver with another protocol on the same device, such as Bluetooth LE. The stack requests
 * every timeslot ahead of time, uses the transceiver only in the timeslots the port grants, and releases each one when done.
 *
 * Only a port that shares the transceiver implements this interface. It calls @ref EMBENET_MULTIPROTOCOL_OnGranted and
 * @ref EMBENET_MULTIPROTOCOL_OnEndGuard, and that reference is what links the stack's multiprotocol support in. A port that
 * does not share the transceiver leaves this interface out entirely: nothing references it, and the stack's multiprotocol code
 * is not linked.
 *
 *     stack                                                    port
 *       | EMBENET_MULTIPROTOCOL_Request(now, start, duration)    |   one timeslot ahead
 *       | -----------------------------------------------------> |
 *       |                     EMBENET_MULTIPROTOCOL_OnGranted()  |   at start
 *       | <----------------------------------------------------- |
 *       | EMBENET_RADIO_Init(), frame exchange,                  |
 *       | EMBENET_RADIO_Idle(), EMBENET_RADIO_Deinit()           |
 *       | EMBENET_MULTIPROTOCOL_Release()                        |
 *       | -----------------------------------------------------> |
 *
 *       |                    EMBENET_MULTIPROTOCOL_OnEndGuard()  |   at start + duration - end_guard_us, if not released
 *       | <----------------------------------------------------- |
 *       | EMBENET_RADIO_Idle(), EMBENET_RADIO_Deinit()           |   the activity is abandoned
 *
 * All times are in microseconds, in the time base of @ref EMBENET_TIMER_ReadCounter.
 *
 * @note While the transceiver is shared, it is initialized and deinitialized in every granted timeslot.
 */

#ifndef EMBENET_NODE_PORT_MULTIPROTOCOL_H_
#define EMBENET_NODE_PORT_MULTIPROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup embenet_node_port_multiprotocol Multiprotocol Interface
 * @{
 */

/**
 * @brief Starts sharing the transceiver.
 *
 * Called by @ref EMBENET_NODE_Init, instead of @ref EMBENET_RADIO_Init, when the port's multiprotocol support is linked in.
 * A port that can share the transceiver but is not to do so now, for example by its configuration, returns false; the stack
 * then owns the transceiver from @ref EMBENET_NODE_Init to @ref EMBENET_NODE_Deinit and calls no other function of this
 * interface.
 *
 * Once it returns true, the port calls @ref EMBENET_MULTIPROTOCOL_OnGranted with @p context at the start of a requested
 * timeslot, as close to the requested start as it can; up to ~100 us late is tolerated. It calls
 * @ref EMBENET_MULTIPROTOCOL_OnEndGuard with @p context @p end_guard_us before the end of a granted timeslot the stack has not
 * released; the stack abandons its activity and deinitializes the transceiver before returning, and the timeslot is then
 * released.
 *
 * @param[in] context      passed to both callbacks
 * @param[in] end_guard_us how long before the end of a granted timeslot @ref EMBENET_MULTIPROTOCOL_OnEndGuard is called
 * @return true if the transceiver is shared from now on
 */
bool EMBENET_MULTIPROTOCOL_Init(void *context, uint32_t end_guard_us);

/**
 * @brief Stops sharing the transceiver.
 *
 * Called by @ref EMBENET_NODE_Deinit, only if @ref EMBENET_MULTIPROTOCOL_Init returned true. No callback may be called after it
 * returns.
 */
void EMBENET_MULTIPROTOCOL_Deinit(void);

/**
 * @brief Requests a timeslot.
 *
 * The stack requests a timeslot about one timeslot ahead. A new request replaces an earlier one that the port can still
 * withdraw; one already started or committed to stands. The port may decline any request, for example when the other
 * protocol needs the transceiver; the stack then skips that timeslot.
 *
 * @param[in] now_us      current time, read by the stack when making the request
 * @param[in] start_us    when the timeslot starts; now_us + (start_us - now_us) with wrap-around
 * @param[in] duration_us how long the timeslot lasts
 *
 * @note May be called from interrupt context; must not block.
 */
void EMBENET_MULTIPROTOCOL_Request(uint32_t now_us, uint32_t start_us, uint32_t duration_us);

/**
 * @brief Releases the granted timeslot.
 *
 * Called with the transceiver already deinitialized, normally well before the end guard, and from within the grant callback
 * for a timeslot the stack refuses. Does nothing when no timeslot is granted.
 *
 * @note May be called from interrupt context, including the stack's callbacks; must not block.
 */
void EMBENET_MULTIPROTOCOL_Release(void);

/**
 * @name Provided by the stack, called by the port
 * Both may be called from any interrupt, even one the critical section does not mask, as long as the stack's timer and radio
 * interrupts do not preempt them. They call @ref EMBENET_TIMER_ReadCounter, @ref EMBENET_RADIO_Init,
 * @ref EMBENET_RADIO_SetCallbacks, @ref EMBENET_RADIO_Idle, @ref EMBENET_RADIO_Deinit and @ref EMBENET_MULTIPROTOCOL_Release,
 * which must work there.
 * @{
 */

/**
 * @brief Tells the stack a requested timeslot has started and the transceiver is its own.
 * @param[in] context the context passed to @ref EMBENET_MULTIPROTOCOL_Init
 */
void EMBENET_MULTIPROTOCOL_OnGranted(void *context);

/**
 * @brief Tells the stack a granted timeslot ends in end_guard_us (see @ref EMBENET_MULTIPROTOCOL_Init).
 * @param[in] context the context passed to @ref EMBENET_MULTIPROTOCOL_Init
 */
void EMBENET_MULTIPROTOCOL_OnEndGuard(void *context);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif // EMBENET_NODE_PORT_MULTIPROTOCOL_H_
