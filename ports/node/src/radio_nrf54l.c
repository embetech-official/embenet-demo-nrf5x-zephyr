/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     2.4 GHz radio, nRF54L
 *
 * Same driver as radio_nrf52.c - same state machine, same frame layout, same callbacks - over a
 * peripheral set that differs in four places:
 *
 *   - Events reach the EGU over DPPI publish/subscribe instead of PPI endpoint pairs.
 *   - The end-of-frame shortcut is PHYEND -> DISABLE; this RADIO has no END -> DISABLE.
 *   - There is no POWER register, so a reset is DISABLE plus a full reconfiguration.
 *   - TXPOWER is an enumeration of supported levels, not a signed dBm number written raw.
 *
 * RADIO, EGU10 and DPPIC10 all sit in the same peripheral domain, so the frame events reach the EGU
 * without crossing a PPIB bridge.
 */

#include <embenet_port/radio.h>
#include <embenet_port/random.h>
#include <embenet_port/timer.h>
#include <embenet_port_common/embenet_port_common.h>
#include <embetech/expect.h>

#include <hal/nrf_clock.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_egu.h>
#include <hal/nrf_radio.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

LOG_MODULE_REGISTER(node_radio, CONFIG_EMBENET_LOG_LEVEL);

#if defined(RADIO_POWER_POWER_Msk)
#error "this backend expects the nRF54L-style RADIO - see radio_nrf52.c"
#endif

#define RADIO_DEVICE NRF_RADIO

// Same domain as RADIO, and left disabled in devicetree, so nothing else in the image claims them.
#define RADIO_EGU NRF_EGU10
#define RADIO_EGU_IRQn EGU10_IRQn
#define RADIO_DPPIC NRF_DPPIC10

// Two of DPPIC10's 24 channels. Fixed rather than allocated, because this port is the only user of
// this DPPI instance - the same reasoning that lets radio_nrf52.c hardcode PPI channels 0 and 1.
#define DPPI_CH_ADDRESS 0U
#define DPPI_CH_END 1U

#define EMBENET_RADIO_MAX_CHANNEL 39
#define EMBENET_RADIO_RSSI_OFFSET (-4)

/*
 * Carried over from the nRF52 port and NOT yet measured on this SoC. The nRF54L ramps its radio up
 * on a different schedule, so every figure below is a placeholder that keeps the stack's arithmetic
 * well-formed rather than a number to trust. They have to be measured against a second board before
 * this port can hold a TSCH slot - see tests/radio.
 */
#define EMBENET_RADIO_TX_TX_START_CORRECTION (40) // start-of-frame ISR to first bit on air
#define EMBENET_RADIO_TX_RX_END_CORRECTION (10)   // end-of-frame ISR, receiver minus transmitter

static EMBENET_RADIO_Capabilities const timings = {.idleToTxReady = 400,
                                                   .idleToRxReady = 400,
                                                   .activeToTxReady = 20,
                                                   .activeToRxReady = 20,
                                                   .txDelay = 45,
                                                   .rxDelay = 45,
                                                   .txRxStartDelay = 50,
                                                   .sensitivity = -95,
                                                   .maxOutputPower = 8,
                                                   .minOutputPower = 2};

#define CRC_LENGTH 2            ///< Length of CRC in 802.15.4 frames [bytes]
#define CRC_POLYNOMIAL 0x011021 ///< Polynomial used for CRC calculation in 802.15.4 frames

/// driver states
typedef enum {
  EMBENET_RADIO_STATE_UNINITIALIZED = 0, //!< uninitialized
  EMBENET_RADIO_STATE_IDLE,              //!< idle
  EMBENET_RADIO_STATE_ACTIVE,            //!< active
  EMBENET_RADIO_STATE_TX_PREPARED,       //!< prepared to transmitting
  EMBENET_RADIO_STATE_RX_PREPARED,       //!< prepared to receiving
  EMBENET_RADIO_STATE_TX_CONTINUOUS_PN9, //!< transmitting continuous PN 9 signal
  EMBENET_RADIO_STATE_TX_CONTINUOUS_CW,  //!< transmitting continuous carrier wave signal
  EMBENET_RADIO_STATE_RX_LISTENING,      //!< listening
  EMBENET_RADIO_STATE_RX_FRAME,          //!< receiving state
  EMBENET_RADIO_STATE_TX_FRAME,          //!< transmitting frame
  EMBENET_RADIO_STATE_RX_CAD,            //!< doing channel activity detection
} RadioState;

// structure to hold radio variables and states
typedef struct {
  EMBENET_RADIO_CaptureCbt onStartOfFrameHandler; ///< called when start of frame interrupt occurs
  EMBENET_RADIO_CaptureCbt onEndOfFrameHandler;   ///< called when end of frame interrupt occurs
  void *handlersContext;                          ///< context passed to handlers
  uint8_t frame[EMBENET_RADIO_MAX_PSDU_LENGTH + 1];
} Admin;

static Admin admin;

static RadioState volatile state; ///< radio driver state

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static char const *getStateName(void) {
  switch(state) {
  case EMBENET_RADIO_STATE_UNINITIALIZED:
    return "UNINITIALIZED";
  case EMBENET_RADIO_STATE_IDLE:
    return "IDLE";
  case EMBENET_RADIO_STATE_ACTIVE:
    return "ACTIVE";
  case EMBENET_RADIO_STATE_TX_PREPARED:
    return "TX_PREPARED";
  case EMBENET_RADIO_STATE_RX_PREPARED:
    return "RX_PREPARED";
  case EMBENET_RADIO_STATE_TX_CONTINUOUS_PN9:
    return "TX_CONTINUOUS_PN9";
  case EMBENET_RADIO_STATE_TX_CONTINUOUS_CW:
    return "TX_CONTINUOUS_CW";
  case EMBENET_RADIO_STATE_RX_LISTENING:
    return "RX_LISTENING";
  case EMBENET_RADIO_STATE_RX_FRAME:
    return "RX_FRAME";
  case EMBENET_RADIO_STATE_TX_FRAME:
    return "TX_FRAME";
  case EMBENET_RADIO_STATE_RX_CAD:
    return "RX_CAD";
  default:
    return "UNKNOWN";
  }
}

#pragma GCC diagnostic pop

static inline void setChannel(uint8_t channel) {
  if(channel > EMBENET_RADIO_MAX_CHANNEL) {
    channel = EMBENET_RADIO_MAX_CHANNEL;
  }
  if(channel <= 10) {
    nrf_radio_frequency_set(RADIO_DEVICE, (uint16_t)(2404 + channel * 2));
  }
  if((channel >= 11) && (channel <= 36)) {
    nrf_radio_frequency_set(RADIO_DEVICE, (uint16_t)(2406 + channel * 2));
  }
  if(37 == channel) {
    nrf_radio_frequency_set(RADIO_DEVICE, 2402);
  }
  if(38 == channel) {
    nrf_radio_frequency_set(RADIO_DEVICE, 2426);
  }
  if(39 == channel) {
    nrf_radio_frequency_set(RADIO_DEVICE, 2480);
  }
}

/**
 * TXPOWER here enumerates the levels the PA actually supports, so a dBm number cannot be written
 * straight into the register the way the nRF52's signed field allows. Anything between two supported
 * levels rounds down, which keeps the radio inside the power the caller asked for.
 */
static inline void setPower(int8_t power) {
  if(power < timings.minOutputPower) {
    power = timings.minOutputPower;
  }
  if(power > timings.maxOutputPower) {
    power = timings.maxOutputPower;
  }

  nrf_radio_txpower_t level;
  if(power >= 8) {
    level = NRF_RADIO_TXPOWER_POS8DBM;
  } else if(power >= 7) {
    level = NRF_RADIO_TXPOWER_POS7DBM;
  } else if(power >= 6) {
    level = NRF_RADIO_TXPOWER_POS6DBM;
  } else if(power >= 5) {
    level = NRF_RADIO_TXPOWER_POS5DBM;
  } else if(power >= 4) {
    level = NRF_RADIO_TXPOWER_POS4DBM;
  } else if(power >= 3) {
    level = NRF_RADIO_TXPOWER_POS3DBM;
  } else {
    level = NRF_RADIO_TXPOWER_POS2DBM;
  }
  nrf_radio_txpower_set(RADIO_DEVICE, level);
}

/**
 * Drops every event left over from the previous operation.
 *
 * EMBENET_RADIO_Idle() masks the EGU interrupt at the NVIC, which stops the handler from running but
 * not the events from latching. Without this, re-enabling the line in RxNow()/TxNow() immediately
 * re-enters the ISR with the previous frame's ADDRESS and END still set, and the stack is told a
 * frame started and ended before the receive window even opened.
 */
static inline void clearPendingFrameEvents(void) {
  nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS);
  nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_END);
  nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_PHYEND);
  nrf_egu_event_clear(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED0);
  nrf_egu_event_clear(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED1);
  NVIC_ClearPendingIRQ(RADIO_EGU_IRQn);
}

/// The radio needs the crystal oscillator, not the internal RC, to meet its timing.
static inline bool hfclkRunningOnXtal(void) {
  nrf_clock_hfclk_t source;
  return nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK, &source) && (NRF_CLOCK_HFCLK_HIGH_ACCURACY == source);
}

/**
 * Refuses to arm the radio while HFCLK is on the internal RC oscillator.
 *
 * On the RC oscillator the transmitter still raises its ADDRESS and END events, so from the outside
 * everything looks healthy, but the carrier is far enough off that no receiver ever matches an
 * access address - one board transmits, the other simply never hears it.
 */
static bool hfclkValidate(char const *what) {
  if(hfclkRunningOnXtal()) {
    return true;
  }
  LOG_ERR("%s refused: HFCLK is on the internal RC oscillator, not the crystal", what);
  return false;
}

static struct onoff_client hfclkClient;
static bool hfclkHeld;

/**
 * Switches HFCLK over to the crystal and keeps it there.
 *
 * The request goes through Zephyr's clock control rather than the oscillator tasks directly, because
 * the driver reference-counts the crystal - starting it behind the driver's back lets another user's
 * release turn it off again underneath the radio.
 */
static bool hfclkAcquireXtal(void) {
  if(hfclkHeld) {
    return true;
  }

  struct onoff_manager *const manager = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
  if(NULL == manager) {
    LOG_ERR("no HFCLK onoff manager");
    return false;
  }

  sys_notify_init_spinwait(&hfclkClient.notify);
  int err = onoff_request(manager, &hfclkClient);
  if(err < 0) {
    LOG_ERR("onoff_request(HFCLK) failed: %d", err);
    return false;
  }

  int result = 0;
  while(sys_notify_fetch_result(&hfclkClient.notify, &result) == -EAGAIN) {
    // The request completes once the crystal has settled; this runs at init, outside any slot.
  }
  if(result < 0) {
    LOG_ERR("HFCLK start failed: %d", result);
    return false;
  }

  hfclkHeld = true;
  return true;
}

static void hfclkReleaseXtal(void) {
  if(!hfclkHeld) {
    return;
  }
  struct onoff_manager *const manager = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
  if(NULL != manager) {
    (void)onoff_release(manager);
  }
  hfclkHeld = false;
}

/**
 * Brings the radio back to a known state.
 *
 * radio_nrf52.c power-cycles the peripheral, which resets every register for free. There is no POWER
 * register here, so this disables the radio and waits for it to say so, and EMBENET_RADIO_Init()
 * then writes out the full configuration rather than relying on reset values.
 */
static void radioReset(void) {
  nrf_radio_shorts_set(RADIO_DEVICE, 0);
  nrf_radio_int_disable(RADIO_DEVICE, ~0U);
  nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_DISABLED);
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_DISABLE);

  // Bounded: a radio that never reports DISABLED must not hang the caller for good.
  for(uint32_t guard = 0x100000U; guard != 0U; --guard) {
    if(nrf_radio_event_check(RADIO_DEVICE, NRF_RADIO_EVENT_DISABLED)) {
      break;
    }
  }
  nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_DISABLED);
}

ISR_DIRECT_DECLARE(radio_isr) {
  // First statement in the ISR: the stack synchronizes on this timestamp, so anything done before
  // reading it - logging above all - lands in the frame timing as jitter.
  EMBENET_TimeUs now = EMBENET_TIMER_ReadCounter();

  LOG_DBG("radio_isr, state=%s", getStateName());

  if(nrf_egu_event_check(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED0)) {
    nrf_egu_event_clear(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED0);
  }
  if(nrf_egu_event_check(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED1)) {
    nrf_egu_event_clear(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED1);
  }

  if(EMBENET_RADIO_STATE_UNINITIALIZED != state) {
    // The events are cleared whether or not a handler wants them. Leaving one latched because no
    // handler was registered makes the next frame look like it ended the instant it started.
    bool const addressEvent = nrf_radio_event_check(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS);
    bool const endEvent = nrf_radio_event_check(RADIO_DEVICE, NRF_RADIO_EVENT_END);
    nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS);
    nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_END);
    nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_PHYEND);

    if(addressEvent) {
      if(admin.onStartOfFrameHandler && (EMBENET_RADIO_STATE_TX_CONTINUOUS_PN9 != state)) {
        EMBENET_TimeUs startOfFrame = now;
        if(EMBENET_RADIO_STATE_TX_FRAME == state) {
          startOfFrame -= EMBENET_RADIO_TX_TX_START_CORRECTION;
        }
        if(EMBENET_RADIO_STATE_RX_LISTENING == state) {
          state = EMBENET_RADIO_STATE_RX_FRAME;
          startOfFrame -= timings.txRxStartDelay;
        }
        admin.onStartOfFrameHandler(admin.handlersContext, startOfFrame);
        LOG_DBG("radio_isr: onStartOfFrameHandler called, state=%s", getStateName());
      }
    }
    if(endEvent) {
      if(EMBENET_RADIO_STATE_RX_FRAME == state) {
        now -= EMBENET_RADIO_TX_RX_END_CORRECTION;
      }
      if(admin.onEndOfFrameHandler && (EMBENET_RADIO_STATE_TX_CONTINUOUS_PN9 != state)) {
        admin.onEndOfFrameHandler(admin.handlersContext, now);
        LOG_DBG("radio_isr: onEndOfFrameHandler called, state=%s", getStateName());
      }
    }
  }
  k_sem_give(EMBENET_PORT_COMMON_GetProcSemaphore());
  ISR_DIRECT_PM(); // done after do_stuff() due to latency concerns

  return 1;
}

EMBENET_RADIO_Status EMBENET_RADIO_Init(void) {
  if(!hfclkAcquireXtal()) {
    return EMBENET_RADIO_STATUS_GENERAL_ERROR;
  }
  // default state
  admin.frame[0] = 0; // the first byte the transceiver stores here is the packet length
  nrf_dppi_channels_disable(RADIO_DPPIC, BIT(DPPI_CH_ADDRESS) | BIT(DPPI_CH_END));
  radioReset();

  // shorts. PHYEND rather than END: this RADIO has no END -> DISABLE shortcut, and on the 1 Mbit
  // PHY the two events coincide, so the frame still ends by disabling the radio.
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_DISABLE_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK);

  // modulation
  nrf_radio_mode_set(RADIO_DEVICE, NRF_RADIO_MODE_BLE_1MBIT);

  // packet config
  nrf_radio_packet_conf_t packetConf = {
      .lflen = 8,                              // LENGTH field is 8 bit long
      .s0len = 0,                              // S0 field disabled
      .s1len = 0,                              // S1 field disabled
      .s1incl = 0,                             // S1 field not included
      .plen = 0,                               // 8 bit preamble
      .maxlen = EMBENET_RADIO_MAX_PSDU_LENGTH, // maximum packet length
      .statlen = 0,                            // static length disabled
      .balen = 3,                              // base address has 3B (+1B of prefix)
      .big_endian = false,                     // transmission bit order
      .whiteen = true                          // whitening enabled
  };
  nrf_radio_packet_configure(RADIO_DEVICE, &packetConf);
  nrf_radio_base0_set(RADIO_DEVICE, 0xE7E7E7E7); // same as the Nordic ESB default values
  nrf_radio_base1_set(RADIO_DEVICE, 0x43434343);
  nrf_radio_prefix0_set(RADIO_DEVICE, 0x23C343E7);
  nrf_radio_prefix1_set(RADIO_DEVICE, 0x13E363A3);
  nrf_radio_txaddress_set(RADIO_DEVICE, 0);
  nrf_radio_rxaddresses_set(RADIO_DEVICE, 1 << 0);
  nrf_radio_crc_configure(RADIO_DEVICE, CRC_LENGTH, NRF_RADIO_CRC_ADDR_SKIP, CRC_POLYNOMIAL);
  nrf_radio_crcinit_set(RADIO_DEVICE, 0xFFFFFFFF);
  nrf_radio_fast_ramp_up_enable_set(RADIO_DEVICE, true); // MODECNF0.RU on the nRF52
  nrf_radio_packetptr_set(RADIO_DEVICE, admin.frame);

  static bool isr_enabled = false;
  if(false == isr_enabled) {
    isr_enabled = true;
    irq_disable(RADIO_EGU_IRQn);
    IRQ_DIRECT_CONNECT(RADIO_EGU_IRQn, 5, radio_isr, 0);
    irq_enable(RADIO_EGU_IRQn);

    nrf_egu_int_enable(RADIO_EGU, NRF_EGU_INT_TRIGGERED0);
    nrf_egu_int_enable(RADIO_EGU, NRF_EGU_INT_TRIGGERED1);

    // DPPI carries the connection as a channel both ends name, rather than the PPI pair of
    // endpoint addresses: the RADIO publishes its event onto a channel, the EGU subscribes to it.
    nrf_radio_publish_set(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS, DPPI_CH_ADDRESS);
    nrf_radio_publish_set(RADIO_DEVICE, NRF_RADIO_EVENT_END, DPPI_CH_END);
    nrf_egu_subscribe_set(RADIO_EGU, NRF_EGU_TASK_TRIGGER0, DPPI_CH_ADDRESS);
    nrf_egu_subscribe_set(RADIO_EGU, NRF_EGU_TASK_TRIGGER1, DPPI_CH_END);
  }
  nrf_dppi_channels_enable(RADIO_DPPIC, BIT(DPPI_CH_ADDRESS) | BIT(DPPI_CH_END));
  state = EMBENET_RADIO_STATE_IDLE;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Status EMBENET_RADIO_InitBleAdv(void) {
  EMBENET_RADIO_Init();
  // these register values are obtained from the observer example application
  nrf_radio_packet_conf_t packetConf = {
      .lflen = 8,
      .s0len = 1,
      .s1len = 0,
      .s1incl = 0,
      .plen = 0,
      .maxlen = 37,
      .statlen = 0,
      .balen = 3,
      .big_endian = false,
      .whiteen = true,
  };
  nrf_radio_packet_configure(RADIO_DEVICE, &packetConf);
  nrf_radio_base0_set(RADIO_DEVICE, 0x89bed600); // address is 3B BASE0 and 1B PREFIX0
  nrf_radio_prefix0_set(RADIO_DEVICE, 0x0000008e);
  nrf_radio_rxaddresses_set(RADIO_DEVICE, 1 << 0);
  nrf_radio_crc_configure(RADIO_DEVICE, 3, NRF_RADIO_CRC_ADDR_SKIP, 0x0000065b);
  nrf_radio_crcinit_set(RADIO_DEVICE, 0x00555555);
  nrf_radio_datawhiteiv_set(RADIO_DEVICE, 0x26);
  return EMBENET_RADIO_STATUS_SUCCESS;
}

void EMBENET_RADIO_SetCallbacks(EMBENET_RADIO_CaptureCbt onStartFrame, EMBENET_RADIO_CaptureCbt onEndFrame, void *cbtContext) {
  admin.onStartOfFrameHandler = onStartFrame;
  admin.onEndOfFrameHandler = onEndFrame;
  admin.handlersContext = cbtContext;
}

void EMBENET_RADIO_Deinit(void) {
  irq_disable(RADIO_EGU_IRQn);

  admin.onStartOfFrameHandler = NULL;
  admin.onEndOfFrameHandler = NULL;

  nrf_dppi_channels_disable(RADIO_DPPIC, BIT(DPPI_CH_ADDRESS) | BIT(DPPI_CH_END));

  radioReset();
  hfclkReleaseXtal();
  state = EMBENET_RADIO_STATE_UNINITIALIZED;
}

EMBENET_RADIO_Status EMBENET_RADIO_Idle(void) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  irq_disable(RADIO_EGU_IRQn); // fool proof method of disabling radio handler from being called

  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_DISABLE);
  clearPendingFrameEvents();

  state = EMBENET_RADIO_STATE_IDLE;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Status EMBENET_RADIO_TxEnable(EMBENET_RADIO_Channel channel, EMBENET_RADIO_Power txp, uint8_t const *psdu, size_t psduLen) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  if(!hfclkValidate("tx enable")) {
    return EMBENET_RADIO_STATUS_GENERAL_ERROR;
  }
  LOG_DBG("tx enable channel %d, power %d, psduLen %d", channel, txp, psduLen);

  setChannel(channel);
  setPower(txp);

  // this short may be disabled or enabled by @EMBENET_RADIO_StartContinuousTx
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
  nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_DISABLE_MASK);

  if(psduLen > EMBENET_RADIO_MAX_PSDU_LENGTH) {
    psduLen = EMBENET_RADIO_MAX_PSDU_LENGTH;
  }
  if(psduLen < EMBENET_RADIO_MIN_PSDU_LENGTH) {
    return EMBENET_RADIO_STATUS_GENERAL_ERROR;
  }
  admin.frame[0] = (uint8_t)psduLen;
  memcpy(&admin.frame[1], psdu, psduLen);

  state = EMBENET_RADIO_STATE_TX_PREPARED;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Status EMBENET_RADIO_TxNow(void) {
  EXPECT_EXTRA(hfclkRunningOnXtal())
  OR_ABORT("HFCLK not running");

  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  clearPendingFrameEvents();
  irq_enable(RADIO_EGU_IRQn);
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_TXEN);

  state = EMBENET_RADIO_STATE_TX_FRAME;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Status EMBENET_RADIO_RxEnable(EMBENET_RADIO_Channel channel) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  if(!hfclkValidate("rx enable")) {
    return EMBENET_RADIO_STATUS_GENERAL_ERROR;
  }

  setChannel(channel);
  // this short may be disabled or enabled by @EMBENET_RADIO_StartContinuousTx
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
  nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_DISABLE_MASK);

  state = EMBENET_RADIO_STATE_RX_PREPARED;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Status EMBENET_RADIO_RxNow(void) {
  EXPECT_EXTRA(hfclkRunningOnXtal())
  OR_ABORT("HFCLK not running");

  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  clearPendingFrameEvents();
  irq_enable(RADIO_EGU_IRQn);
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_RXEN);

  state = EMBENET_RADIO_STATE_RX_LISTENING;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_RxInfo EMBENET_RADIO_GetReceivedFrame(uint8_t *buffer, size_t bufferLength) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return (EMBENET_RADIO_RxInfo){.crcValid = false, .lqi = 0, .mpduLength = 0, .rssi = (int8_t)(-127)};
  }
  LOG_DBG("get received frame bufferLength %d", bufferLength);

  bool s0inPayload = (NRF_RADIO->PCNF0 & 0x00000100) ? true : false; // S0 field included in payload
  uint8_t packetLength = s0inPayload ? admin.frame[1] : admin.frame[0];
  EMBENET_RADIO_RxInfo info = {.lqi = 0,
                               .mpduLength = packetLength,
                               .crcValid = packetLength ? nrf_radio_crc_status_check(RADIO_DEVICE) : false,
                               .rssi = (int8_t)(EMBENET_RADIO_RSSI_OFFSET - nrf_radio_rssi_sample_get(RADIO_DEVICE))};
  size_t dataAmountToCopy = info.mpduLength;
  if(dataAmountToCopy > bufferLength) {
    dataAmountToCopy = bufferLength;
  }
  memcpy(buffer, &admin.frame[s0inPayload ? 2 : 1], dataAmountToCopy);
  return info;
}

EMBENET_RADIO_Status EMBENET_RADIO_StartContinuousTx(EMBENET_RADIO_ContinuousTxMode mode, EMBENET_RADIO_Channel channel, EMBENET_RADIO_Power txp) {
  EXPECT_EXTRA(hfclkRunningOnXtal())
  OR_ABORT("HFCLK not running");

  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  switch(mode) {
  case EMBENET_RADIO_CONTINUOUS_TX_MODE_PN9:
    admin.frame[0] = EMBENET_RADIO_MAX_PSDU_LENGTH;
    for(size_t i = 1; i < sizeof(admin.frame); ++i) {
      admin.frame[i] = (uint8_t)EMBENET_RANDOM_Get();
    }

    setPower(txp);
    setChannel(channel);
    nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_START_MASK);
    nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_PHYEND_DISABLE_MASK);
    nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_TXEN);

    state = EMBENET_RADIO_STATE_TX_CONTINUOUS_PN9;
    break;
  case EMBENET_RADIO_CONTINUOUS_TX_MODE_CARRIER:
    setPower(txp);
    setChannel(channel);
    nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
    nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_TXEN);

    state = EMBENET_RADIO_STATE_TX_CONTINUOUS_CW;
    break;
  default:
    return EMBENET_RADIO_STATUS_GENERAL_ERROR;
  }

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Capabilities const *EMBENET_RADIO_GetCapabilities(void) { return &timings; }
