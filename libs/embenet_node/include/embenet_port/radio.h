/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET PORT API
 * @brief     Radio interface for the EMBENET NODE Port
 *
 * The IDLE state is implementation defined. The assumption is to balance speed and energy consumption in cross-platform manner.
 * Modern transceivers may not differentiate between SLEEP and STANDBY modes (or consumed power difference is miniscule), so we can prioritize
 * minimizing energy consumption by assuming that RADIO IDLE state is SLEEP.
 * @note     Radio states:
 * UNINITIALIZED  State just after hardware reset.
 * IDLE        In this state radio may remain in low power mode.
 * TX_READY    Prepared for immediate transmission trigger.
 * RX_READY    Prepared for immediate listening trigger.
 * LISTEN      Radio listens for frames.
 * TX          Radio is transmitting frame.
 * RX          Radio is receiving frame.
 * ACTIVE      After TX or RX radio remains in state ready for fast transition to TX_READY or RX_READY.
 * HardwareReset                       -> UNINITIALIZED
 * UNINITIALIZED(EMBENET_RADIO_Init)   -> IDLE
 * IDLE(EMBENET_TxEnable)              -> TX_READY     at idleToTxReady
 * IDLE(EMBENET_RxEnable)              -> RX_READY     at idleToRxReady
 * IDLE(EMBENET_RADIO_Idle)            -> IDLE
 * RX_READY(EMBENET_RADIO_RxNow)       -> LISTEN
 * RX_READY(EMBENET_RADIO_Idle)        -> IDLE
 * TX_READY(EMBENET_RADIO_TxNow)       -> TX         at txDelay
 * TX_READY(EMBENET_RADIO_Idle)        -> IDLE
 * LISTEN(start of frame)              -> RX         at rxDelay
 * LISTEN(EMBENET_RADIO_Idle)          -> IDLE
 * RX(end of frame)                    -> ACTIVE
 * RX(EMBENET_RADIO_Idle)              -> IDLE
 * ACTIVE(EMBENET_TxEnable)             -> TX_READY      at activeToTxReady
 * ACTIVE(EMBENET_RxEnable)            -> RX_READY     at activeToRxReady
 * ACTIVE(EMBENET_RADIO_Idle)          -> IDLE
 */

// clang-format off
/**
*                                                           UNINITIALIZED (after hardware reset)
                                                                |
                                                                | EMBENET_RADIO_Init
                                                                |
                                                               \|/
                  (from any state except UNINITIALIZED) EMBENET_RADIO_Idle ------> IDLE
                                                                |
                                                                |
                                           EMBENET_TxEnable     |     EMBENET_RxEnable
                    -------> TX_READY <--------------------------------------------------------> RX_READY <---------------------------------------------------
                  |             |                                                                   |                                                        |
                  |             |  EMBENET_RADIO_TxNow                                              | EMBENET_RADIO_RxNow                                    |
                  |             | (calls onFrameStart on start of TX frame)                 |                                                        |
 EMBENET_TxEnable |            \|/                                                                 \|/                                                       |
                  |            TX                                                                 LISTEN                                                     |
                  |             |                                                                   |                                                        |
                  |             | (calls onFrameEnd on end of frame)                                | (calls onFrameStart on start of RX frame)      |
                  |             |                                                                   |                                                        |
                  |            \|/            (calls onFrameEnd on end of RX frame)               \|/                                                       |
                  -----------ACTIVE <------------------------------------------------------------- RX                                                        |
                                |                                                                                                                            |
                                |                                           EMBENET_RxEnable                                                                 |
                                ------------------------------------------------------------------------------------------------------------------------------
**/
// clang-format on

#ifndef EMBENET_NODE_PORT_RADIO_H_
#define EMBENET_NODE_PORT_RADIO_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup embenet_node_port_radio Radio Interface
 *
 * Provides interface to the radio transceiver.
 *
 * Only primitive types are used, so a port may implement these functions without including this header. Times are in microseconds,
 * in the time base of @ref EMBENET_TIMER_ReadCounter; powers are in dBm.
 *
 * Function arguments are valid by contract (channels are in [0, @ref EMBENET_RADIO_GetChannelCount), checked against the stack configuration
 * in @c EMBENET_NODE_Init, power is within
 * [@ref EMBENET_RADIO_GetMinTxPowerDbm, @ref EMBENET_RADIO_GetMaxTxPowerDbm], frame length within [1, @ref EMBENET_RADIO_MAX_PSDU_LENGTH]).
 * Hardware failures are not reported: the MAC watchdogs recover from a radio that does not do what it was told. The port must always
 * leave the radio in a state from which @ref EMBENET_RADIO_Idle recovers.
 *
 * @{
 */

/// Maximum PSDU length in bytes the stack sends and expects to receive.
#define EMBENET_RADIO_MAX_PSDU_LENGTH 128

/**
 * @brief Initializes the radio transceiver and places it in the IDLE state.
 *
 * @note Called once during @ref EMBENET_NODE_Init - or, while the transceiver is shared (see embenet_port/multiprotocol.h),
 *       from the grant callback of every timeslot, paired with @ref EMBENET_RADIO_Deinit at its end.
 *
 * @retval true  on success
 * @retval false the radio hardware does not respond; @c EMBENET_NODE_Init fails, or the shared timeslot is abandoned
 */
bool EMBENET_RADIO_Init(void);

/**
 * @brief Registers frame boundary callbacks. Both are called from the radio interrupt.
 *
 * @param[in] onFrameStart called when the sync word of an incoming or outgoing frame is detected, with a back-calculated timestamp
 *                         of the first bit; NULL disables this callback.
 * @param[in] onFrameEnd   called when the last bit of a frame has been transmitted or received; NULL disables this callback.
 * @param[in] context      opaque pointer passed as the first argument to both callbacks.
 */
void EMBENET_RADIO_SetCallbacks(void (*onFrameStart)(void *context, uint32_t timestampUs), void (*onFrameEnd)(void *context, uint32_t timestampUs),
                                void *context);

/**
 * @brief Deinitializes the radio transceiver and places it in the lowest energy consumption mode available.
 */
void EMBENET_RADIO_Deinit(void);

/**
 * @brief Aborts any ongoing reception or transmission, clears internal buffers, and places the radio in the IDLE state.
 *
 * May be called from any state except UNINITIALIZED.
 */
void EMBENET_RADIO_Idle(void);

/**
 * @brief Wakes the transceiver and loads a frame into the TX buffer ready for immediate transmission (IDLE or ACTIVE -> TX_READY).
 *
 * @param[in] channel    radio channel to transmit on
 * @param[in] txPowerDbm transmit power in dBm
 * @param[in] psdu       frame payload; must stay valid until the end-of-frame callback has fired
 * @param[in] psduLength frame length in bytes, in [1, @ref EMBENET_RADIO_MAX_PSDU_LENGTH]
 */
void EMBENET_RADIO_TxEnable(uint8_t channel, int8_t txPowerDbm, uint8_t const *psdu, uint32_t psduLength);

/**
 * @brief Triggers immediate transmission of the frame loaded by @ref EMBENET_RADIO_TxEnable (TX_READY -> TX).
 *
 * @retval true  transmission started, or failed for a reason other than a busy channel (left to the MAC watchdog)
 * @retval false the frame was not sent because the channel is busy (listen before talk / CCA); the stack retries later
 */
bool EMBENET_RADIO_TxNow(void);

/**
 * @brief Wakes the transceiver and prepares it for reception (IDLE or ACTIVE -> RX_READY).
 *
 * @param[in] channel radio channel to listen on
 */
void EMBENET_RADIO_RxEnable(uint8_t channel);

/**
 * @brief Triggers immediate listening (RX_READY -> LISTEN).
 */
void EMBENET_RADIO_RxNow(void);

/**
 * @brief Copies the last received frame into the caller-supplied buffer.
 *
 * Called after the end-of-frame callback of a received frame. At most @p bufferLength bytes are copied.
 *
 * @param[out] buffer       destination buffer
 * @param[in]  bufferLength size of @p buffer in bytes
 * @param[out] rssiDbm      RSSI of the frame in dBm; always written (INT8_MIN when there is no frame)
 * @param[out] crcValid     true if the frame CRC is correct; always written (false when there is no frame)
 *
 * @return Length of the received PSDU in bytes, not clamped to @p bufferLength (so the stack detects oversize frames); 0 when there is
 *         no frame.
 */
uint32_t EMBENET_RADIO_GetReceivedFrame(uint8_t *buffer, uint32_t bufferLength, int8_t *rssiDbm, bool *crcValid);

/**
 * @brief Returns the number of radio channels; valid channel numbers are [0, count).
 *
 * Channel number to frequency mapping is the port's channel plan and must match the band of the configuration passed to
 * @c EMBENET_NODE_Init (863-870 MHz: channel n = 863.1 MHz + n * 100 kHz, 69 channels; 2400-2480 MHz BLE PHY: 40 channels).
 * @c EMBENET_NODE_Init fails when the configuration uses a channel outside this range.
 */
uint32_t EMBENET_RADIO_GetChannelCount(void);

/**
 * @name Radio properties
 * Worst-case values determined by the port implementor. Read by the stack during initialization; must work in the UNINITIALIZED
 * state as well, since a shared transceiver (see embenet_port/multiprotocol.h) is uninitialized most of the time.
 * @{
 */
uint32_t EMBENET_RADIO_GetIdleToTxReadyUs(void);   ///< Maximum time to reach TX_READY from IDLE.
uint32_t EMBENET_RADIO_GetIdleToRxReadyUs(void);   ///< Maximum time to reach RX_READY from IDLE.
uint32_t EMBENET_RADIO_GetActiveToTxReadyUs(void); ///< Maximum time to reach TX_READY from ACTIVE (end-of-frame callback to TX_READY).
uint32_t EMBENET_RADIO_GetActiveToRxReadyUs(void); ///< Maximum time to reach RX_READY from ACTIVE (end-of-frame callback to RX_READY).
uint32_t EMBENET_RADIO_GetTxDelayUs(void);         ///< Maximum time from TX_READY until the radio signal appears on the antenna port.
uint32_t EMBENET_RADIO_GetRxDelayUs(void);         ///< Maximum time from RX_READY until the radio starts listening.
uint32_t EMBENET_RADIO_GetTxRxStartDelayUs(void);  ///< Time between the first preamble bit and the start-of-frame callback.
int8_t EMBENET_RADIO_GetSensitivityDbm(void);      ///< Receive power below which the packet error rate rises significantly.
int8_t EMBENET_RADIO_GetMaxTxPowerDbm(void);       ///< Maximum TX output power.
int8_t EMBENET_RADIO_GetMinTxPowerDbm(void);       ///< Minimum TX output power.
/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif // EMBENET_NODE_PORT_RADIO_H_
