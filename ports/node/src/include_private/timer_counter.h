/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Conversion from the stack's time to the multiprotocol adapter's
 */

#ifndef EMBENET_NODE_PORT_TIMER_COUNTER_H_
#define EMBENET_NODE_PORT_TIMER_COUNTER_H_

#include <embenet_port/timer.h>

#include <stdint.h>

/**
 * The time, in embenet_multiprotocol_now_us() microseconds, at which the stack's clock (EMBENET_TIMER_ReadCounter()) reads
 * @p time_us - the nearest such moment, within half the 32-bit range of now.
 *
 * Exact on the nRF54L, where both are the GRTC (timer_grtc_nrf54l.c); within an RTC tick on the nRF52, where they are two RTCs
 * over one crystal (timer_rtc_nrf52.c).
 *
 * @note Any context but the zero latency one.
 */
uint64_t embenet_port_timer_to_counter(uint32_t time_us);

#endif // EMBENET_NODE_PORT_TIMER_COUNTER_H_
