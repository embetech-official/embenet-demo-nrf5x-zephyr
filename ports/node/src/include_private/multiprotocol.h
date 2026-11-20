/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Multiprotocol adapter: books the radio transceiver for a given time through MPSL timeslots
 *
 * Lets one client take turns on the radio with whatever else MPSL schedules - a Bluetooth link, an
 * 802.15.4 driver. The client books the transceiver for a stretch of time starting at a moment of its
 * choosing, and is told whether it got it.
 *
 * Times are microseconds of the system timer, as returned by embenet_multiprotocol_now_us() -
 * the clock MPSL itself schedules by, so a granted timeslot starts when it was booked for, not when
 * the adapter got round to asking. One booking at a time:
 *
 *     client                                      adapter
 *       embenet_multiprotocol_reserve(t, len) -->  booked; handed to MPSL shortly before t
 *                                                  on_granted(t)   - at t, the radio is the client's
 *                                             or   on_denied(t, why) - t is lost, the client moves on
 *       embenet_multiprotocol_release()  -->       timeslot handed back within a millisecond
 *                                             or   on_revoked()    - t + len is near, stop now
 *
 * The next booking can be made as soon as the previous one has been handed to MPSL, which is
 * EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US before its start - in particular while its timeslot runs, so
 * timeslots can follow each other back to back.
 *
 * @note The system timer is the GRTC on the nRF54L and RTC1 on the nRF52 - both counting the LFCLK crystal
 *       MPSL schedules by, so there is no drift between the two; RTC1 only in a coarser, 30.5 us, grain.
 */

#ifndef EMBENET_NODE_PORT_MULTIPROTOCOL_ADAPTER_H_
#define EMBENET_NODE_PORT_MULTIPROTOCOL_ADAPTER_H_

#include <embenet_port_node/multiprotocol_stats.h>

#include <stdbool.h>
#include <stdint.h>

/// How long before its start a booking is handed to MPSL - after which it can no longer be cancelled.
#ifndef EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US
#define EMBENET_MULTIPROTOCOL_SUBMIT_LEAD_US 3000U
#endif

/**
 * Least time MPSL needs between being asked for a timeslot and its start - mostly the HFXO start-up
 * it budgets for, the devicetree's hfxo startup-time-us (1650 us on the nRF54L15 DK). Measured with
 * nothing else on the radio: every request blocked at 1700 us, 6% at 1800 us, none at 2000 us. A
 * booking that cannot be handed over at least this early is denied as too late. Does not apply
 * between back-to-back timeslots, which are booked from inside the earlier one.
 */
#ifndef EMBENET_MULTIPROTOCOL_MIN_LEAD_US
#define EMBENET_MULTIPROTOCOL_MIN_LEAD_US 2000U
#endif

/// Taken out of the end of every timeslot for handing it back: on_revoked() comes this much before
/// start + length.
#define EMBENET_MULTIPROTOCOL_END_GUARD_US 75U

/// Shortest booking. Anything shorter could reach its end guard before the start signal has finished
/// arming it, which MPSL punishes with an assert for overstaying.
#define EMBENET_MULTIPROTOCOL_SHORTEST_US 500U

/// Why a booking did not get its timeslot.
typedef enum {
  MULTIPROTOCOL_DENIED_TOO_LATE, ///< could not be handed to MPSL early enough, e.g. the session was busy
  MULTIPROTOCOL_DENIED_BLOCKED,  ///< MPSL had the time promised to something else
  MULTIPROTOCOL_DENIED_FAILED,   ///< MPSL refused the request, or no timeslot to measure from could be had
} MultiprotocolDenial;

/// The client's side of the adapter.
typedef struct {
  /**
   * The timeslot booked for @p start_us has started; the transceiver and the crystal are the client's
   * until on_revoked() or a release.
   *
   * @warning MPSL's zero latency interrupt: register access and plain memory only - no kernel calls,
   *          no logging - and back out quickly, the timeslot is ticking.
   */
  void (*on_granted)(uint64_t start_us, void *context);
  /// The booking for @p start_us will not get its timeslot. Thread context - the adapter's, or MPSL's
  /// work queue - so keep it short; it may book again.
  void (*on_denied)(uint64_t start_us, MultiprotocolDenial reason, void *context);
  /// The timeslot is about to end. The client stops using the transceiver before returning. Same
  /// context and rules as on_granted().
  void (*on_revoked)(void *context);
  void *context;
} MultiprotocolClient;

/**
 * Opens the MPSL timeslot session and takes the first timeslot, only to measure where it starts:
 * every booking is placed relative to the start of the timeslot before it.
 *
 * @param client Callbacks for every booking from here on. Must outlive the adapter.
 * @return 0, a negative error from mpsl_timeslot_session_open(), or -ETIMEDOUT if MPSL would not
 *         grant the first timeslot.
 * @note Call from a thread. Takes as long as MPSL needs to find room for a short timeslot.
 */
int32_t embenet_multiprotocol_init(MultiprotocolClient const *client);

/// The time all bookings are expressed in: microseconds of the system timer. Any context.
uint64_t embenet_multiprotocol_now_us(void);

/**
 * Books the transceiver for [@p start_us, @p start_us + @p length_us).
 *
 * Exactly one of on_granted() and on_denied() follows, for @p start_us, unless the booking is
 * cancelled first. The timeslot after this one can be booked once this one is handed to MPSL.
 *
 * @retval 0        booked
 * @retval -EBUSY   a booking is already waiting to be handed to MPSL
 * @retval -ETIME   @p start_us is less than EMBENET_MULTIPROTOCOL_MIN_LEAD_US away
 * @retval -EINVAL  @p length_us is too short, or the adapter is not initialised
 * @note Any context but MPSL's zero latency one.
 */
int embenet_multiprotocol_reserve(uint64_t start_us, uint32_t length_us);

/**
 * Withdraws the waiting booking, without a callback.
 *
 * @return true if it was withdrawn; false if there was none, or it has already been handed to MPSL -
 *         MPSL offers no way to take a request back - in which case its callback still follows and
 *         a grant can be given back with embenet_multiprotocol_release().
 * @note Any context but MPSL's zero latency one.
 */
bool embenet_multiprotocol_cancel(void);

/**
 * Gives back the timeslot handed to MPSL, granted or not.
 *
 * The client must already have stopped using the transceiver. MPSL only lets a timeslot be ended from
 * its own signal, so the radio actually goes to the other protocol at the adapter's next check, within
 * a millisecond. Neither on_revoked() nor on_denied() is called for it.
 *
 * @note Any context.
 */
void embenet_multiprotocol_release(void);

/// Measured start of the timeslot being granted, in embenet_multiprotocol_now_us() time. Valid from
/// on_granted() until the timeslot ends; for the client to time its radio activity against.
uint64_t embenet_multiprotocol_slot_start_us(void);

/// Copies the adapter's counters out: every field of @p stats but the refusals, which the port's
/// embenet_multiprotocol_get_stats() adds. Any context; the copy is not atomic across fields.
void embenet_multiprotocol_adapter_get_stats(MultiprotocolStats *stats);

#endif // EMBENET_NODE_PORT_MULTIPROTOCOL_ADAPTER_H_
