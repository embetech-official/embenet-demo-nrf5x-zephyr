/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Counters of the radio sharing, for diagnostics
 *
 * With CONFIG_EMBENET_NODE_MULTIPROTOCOL the port shares the radio transceiver with another protocol stack through
 * MPSL timeslots: the node stack asks for the radio ahead of each of its slots, the port books it, and MPSL grants
 * or denies it. These counters tell how that went. They are the port's only public multiprotocol API; the rest is
 * between the stack and the port.
 */

#ifndef EMBENET_PORT_NODE_MULTIPROTOCOL_STATS_H_
#define EMBENET_PORT_NODE_MULTIPROTOCOL_STATS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// What has happened to the stack's requests for the radio since the image started. Counters only grow.
typedef struct {
  uint32_t submitted;        ///< handed to MPSL as timeslot requests
  uint32_t chained;          ///< of those, requested from the end of the timeslot before
  uint32_t granted;          ///< granted: the radio was the node's for its slot
  uint32_t blocked;          ///< denied: MPSL had the time promised to the other protocol
  uint32_t too_late;         ///< denied: booked, but not handed to MPSL in time
  uint32_t failed;           ///< denied: MPSL refused the request
  uint32_t revoked;          ///< held to the end of the timeslot
  uint32_t released;         ///< handed back early, once the node was done with the radio
  uint32_t extend_failed;    ///< cut short because the next part of a long slot was taken by the other protocol
  uint32_t anchored;         ///< timeslots taken only to measure a start from
  uint32_t unexpected;       ///< MPSL signals the port did not expect, e.g. an overstayed timeslot
  uint32_t refused_too_late; ///< not booked at all: asked for too close to its start for MPSL to schedule
  uint32_t refused_other;    ///< not booked at all for any other reason, e.g. a slot too short or no MPSL session
} MultiprotocolStats;

/**
 * Copies the counters out.
 *
 * Only in an image built with CONFIG_EMBENET_NODE_MULTIPROTOCOL.
 *
 * @param[out] stats The counters.
 * @note Any context; the copy is not atomic across fields.
 */
void embenet_multiprotocol_get_stats(MultiprotocolStats *stats);

#ifdef __cplusplus
}
#endif

#endif // EMBENET_PORT_NODE_MULTIPROTOCOL_STATS_H_
