/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     2.4 GHz radio, nRF52
 */

#include <embenet_port/radio.h>
#include <embenet_port/random.h>
#include <embenet_port/timer.h>
#include <embetech/expect.h>
#include <hal/nrf_clock.h>
#include <hal/nrf_egu.h>
#include <hal/nrf_ppi.h>
#include <hal/nrf_radio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(node_radio, CONFIG_EMBENET_LOG_LEVEL);

#if !defined(RADIO_POWER_POWER_Msk)
#error "this backend expects the nRF52-style RADIO - see radio_nrf54l.c"
#endif

#define RADIO_DEVICE NRF_RADIO

#define EMBENET_RADIO_MAX_CHANNEL 39
#define EMBENET_RADIO_RSSI_OFFSET (-4)
#define EMBENET_RADIO_TX_TX_START_CORRECTION (40) // time difference between start of frame ISR and actual first bit being sent by radio
#define EMBENET_RADIO_TX_RX_END_CORRECTION (10)   // time difference between end of frame ISR on receiver and transmitter side (receiver - sender)

// RADIO_TxNow -> TX ISR 85us, (45us of radio preparing and power ramping, 40us of preamble and address field transmission)
static EMBENET_RADIO_Capabilities const timings = {
    .idleToTxReady = 400,
    .idleToRxReady = 400,
    .activeToTxReady = 20,
    .activeToRxReady = 20,
    .txDelay = 45,
    .rxDelay = 45,        // between GO signal and start listening - cannot measure but could be the same as delayTx
    .txRxStartDelay = 50, // additional 10us takes the transceiver to trigger start of frame ISR on receiver side
    .sensitivity = -95,   // [dBm], does not consider as neighbor if RSSI will be lower
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
  EMBENET_RADIO_STATE_TX_CONTINUOUS_PN9, //!< transmitting continuous PN 9
                                         //!< signal
  EMBENET_RADIO_STATE_TX_CONTINUOUS_CW,  //!< transmitting continuous carrier
                                         //!< wave signal
  EMBENET_RADIO_STATE_RX_LISTENING,      //!< listening
  EMBENET_RADIO_STATE_RX_FRAME,          //!< receiving state
  EMBENET_RADIO_STATE_TX_FRAME,          //!< transmitting frame
  EMBENET_RADIO_STATE_RX_CAD,            //!< doing channel activity detection
} RadioState;

// structure to hold radio variables and states
typedef struct {
  EMBENET_RADIO_CaptureCbt onStartOfFrameHandler; ///< handler to method called when start of frame
                                                  ///< interrupt occurs
  EMBENET_RADIO_CaptureCbt onEndOfFrameHandler;   ///< handler to method called when end of frame
                                                  ///< interrupt occurs
  void *handlersContext;                          ///< context passed to handlers
  uint8_t frame[EMBENET_RADIO_MAX_PSDU_LENGTH + 1];
} Admin;

static Admin admin;

static RadioState volatile state; ///< radio driver state

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function" // warning or error suppress on unused function

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

static inline void setPower(int8_t power) {
  if(power < timings.minOutputPower) {
    power = timings.minOutputPower;
  }
  if(power > timings.maxOutputPower) {
    power = timings.maxOutputPower;
  }
  NRF_RADIO->TXPOWER = (uint32_t)power;
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
  nrf_egu_event_clear(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED0);
  nrf_egu_event_clear(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED1);
  NVIC_ClearPendingIRQ(SWI3_EGU3_IRQn);
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
 * access address - one board transmits, the other simply never hears it. That is worth failing
 * loudly for, which is why this is a plain check and not EXPECT_EXTRA(): the latter compiles out in
 * this build, which is how the condition went unnoticed in the first place.
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
 * Switches HFCLK over to the 32 MHz crystal and keeps it there.
 *
 * Without this the SoC runs the radio off the internal RC oscillator, whose accuracy is nowhere near
 * what the 2.4 GHz PLL needs: the transmitter still raises its ADDRESS and END events, so from the
 * outside it looks like it is working, but the carrier is far enough off that no receiver ever
 * matches an access address.
 *
 * The request goes through Zephyr's clock control rather than HFCLKSTART directly, because the
 * driver reference-counts HFXO - starting it behind the driver's back lets another user's release
 * turn it off again underneath the radio.
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
    // The request completes once HFXO has settled; this runs at init, outside any slot.
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

ISR_DIRECT_DECLARE(radio_isr) { //(const void *args) {
  // First statement in the ISR: the stack synchronizes on this timestamp, so anything done before
  // reading it - logging above all - lands in the frame timing as jitter.
  EMBENET_TimeUs now = EMBENET_TIMER_ReadCounter();

  LOG_DBG("radio_isr, state=%s", getStateName());

  if(nrf_egu_event_check(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED0)) {
    nrf_egu_event_clear(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED0);
  }
  if(nrf_egu_event_check(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED1)) {
    nrf_egu_event_clear(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED1);
  }

  if(EMBENET_RADIO_STATE_UNINITIALIZED != state) {
    // The events are cleared whether or not a handler wants them. Leaving one latched because no
    // handler was registered makes the next frame look like it ended the instant it started.
    bool const addressEvent = nrf_radio_event_check(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS);
    bool const endEvent = nrf_radio_event_check(RADIO_DEVICE, NRF_RADIO_EVENT_END);
    nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS);
    nrf_radio_event_clear(RADIO_DEVICE, NRF_RADIO_EVENT_END);

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
  admin.frame[0] = 0; // the first byte transceiver stores here is the packet length
  nrf_ppi_channel_disable(NRF_PPI, NRF_PPI_CHANNEL0);
  nrf_ppi_channel_disable(NRF_PPI, NRF_PPI_CHANNEL1);
  nrf_radio_power_set(RADIO_DEVICE,
                      false); // restart radio
  nrf_radio_power_set(RADIO_DEVICE,
                      true); // all registers will be set to default
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_DISABLE);

  // shorts
  nrf_radio_shorts_enable(RADIO_DEVICE,
                          NRF_RADIO_SHORT_READY_START_MASK); // shortcut between READY event and
                                                             // START task
  nrf_radio_shorts_enable(RADIO_DEVICE,
                          NRF_RADIO_SHORT_END_DISABLE_MASK); // shortcut between END event and
                                                             // DISABLE task
  nrf_radio_shorts_enable(RADIO_DEVICE,
                          NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK); // shortcut between ADDRESS
                                                                   // event and RSSISTART task

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
      .whiteen = true                          // no whitening at the beginning};
  };
  nrf_radio_packet_configure(RADIO_DEVICE, &packetConf);
  nrf_radio_base0_set(RADIO_DEVICE,
                      0xE7E7E7E7); // configure radio address registers (same
                                   // as Nordic ESB default values)
  nrf_radio_base1_set(RADIO_DEVICE, 0x43434343);
  nrf_radio_prefix0_set(RADIO_DEVICE, 0x23C343E7);
  nrf_radio_prefix1_set(RADIO_DEVICE, 0x13E363A3);
  nrf_radio_txaddress_set(RADIO_DEVICE, 0);
  nrf_radio_rxaddresses_set(RADIO_DEVICE, 1 << 0);
  nrf_radio_crc_configure(RADIO_DEVICE, CRC_LENGTH, NRF_RADIO_CRC_ADDR_SKIP, CRC_POLYNOMIAL);
  nrf_radio_crcinit_set(RADIO_DEVICE, 0xFFFFFFFF);
  nrf_radio_modecnf0_set(RADIO_DEVICE, true, 2);
  nrf_radio_packetptr_set(RADIO_DEVICE, admin.frame);

  static bool isr_enabled = false;
  if(false == isr_enabled) {
    isr_enabled = true;
    // /* Ensure IRQs are disabled before attaching. */
    irq_disable(SWI3_EGU3_IRQn);
    // ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RADIO_IRQn, 0, 0, reschedule);
    IRQ_DIRECT_CONNECT(SWI3_EGU3_IRQn, 5, radio_isr, 0);
    irq_enable(SWI3_EGU3_IRQn);

    nrf_egu_int_enable(NRF_EGU3, NRF_EGU_INT_TRIGGERED0);
    nrf_egu_int_enable(NRF_EGU3, NRF_EGU_INT_TRIGGERED1);
    nrf_ppi_channel_endpoint_setup(NRF_PPI, NRF_PPI_CHANNEL0, (uint32_t)&NRF_RADIO->EVENTS_ADDRESS, (uint32_t)&NRF_EGU3->TASKS_TRIGGER[0]);
    nrf_ppi_channel_endpoint_setup(NRF_PPI, NRF_PPI_CHANNEL1, (uint32_t)&NRF_RADIO->EVENTS_END, (uint32_t)&NRF_EGU3->TASKS_TRIGGER[1]);
  }
  nrf_ppi_channel_enable(NRF_PPI, NRF_PPI_CHANNEL0);
  nrf_ppi_channel_enable(NRF_PPI, NRF_PPI_CHANNEL1);
  state = EMBENET_RADIO_STATE_IDLE;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_Status EMBENET_RADIO_InitBleAdv(void) {
  EMBENET_RADIO_Init();
  // these register values are obtained from the observer example application
  NRF_RADIO->PCNF0 = 0x00000108; // 8b of length field, 1B of S0 field, 0b of S1 field, S1 not in RAM, 8-bit preamble,
  NRF_RADIO->PCNF1 =
      0x02030025; // max length 37B, no static length, BALEN=3 (total address length 4B), least Significant bit on air first, whitening enabled
  NRF_RADIO->BASE0 = 0x89bed600; // address in composed of 3B BASE0 and 1B PREFIX0
  NRF_RADIO->PREFIX0 = 0x0000008e;
  NRF_RADIO->RXADDRESSES = 0x00000001; // enabled reception on logical address 0 (3B BASE0 and 1B PREFIX0)
  NRF_RADIO->CRCCNF =
      0x00000103; // 3B of CRC, CRC calculation does not include address field, the CRC calc will start at the first byte after the address.
  NRF_RADIO->CRCPOLY = 0x0000065b;
  NRF_RADIO->CRCINIT = 0x00555555;
  NRF_RADIO->DATAWHITEIV = 0x00000026;
  return EMBENET_RADIO_STATUS_SUCCESS;
}

void EMBENET_RADIO_SetCallbacks(EMBENET_RADIO_CaptureCbt onStartFrame, EMBENET_RADIO_CaptureCbt onEndFrame, void *cbtContext) {
  admin.onStartOfFrameHandler = onStartFrame;
  admin.onEndOfFrameHandler = onEndFrame;
  admin.handlersContext = cbtContext;
}

void EMBENET_RADIO_Deinit(void) {
  irq_disable(SWI3_EGU3_IRQn);

  admin.onStartOfFrameHandler = NULL;
  admin.onEndOfFrameHandler = NULL;

  nrf_ppi_channel_disable(NRF_PPI, NRF_PPI_CHANNEL0);
  nrf_ppi_channel_disable(NRF_PPI, NRF_PPI_CHANNEL1);

  nrf_radio_power_set(RADIO_DEVICE, false);
  hfclkReleaseXtal();
  state = EMBENET_RADIO_STATE_UNINITIALIZED;
}

EMBENET_RADIO_Status EMBENET_RADIO_Idle(void) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return EMBENET_RADIO_STATUS_WRONG_STATE;
  }
  irq_disable(SWI3_EGU3_IRQn); // fool proof method of disabling radio handler from being called

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
  // Slot ISR, once per transmit slot - see the note in EMBENET_RADIO_RxEnable().
  LOG_DBG("tx enable channel %d, power %d, psduLen %d", channel, txp, psduLen);

  setChannel(channel);
  setPower(txp);

  // this short may be disabled or enabled by @EMBENET_RADIO_StartContinuousTx
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
  nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_END_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_END_DISABLE_MASK);

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
  irq_enable(SWI3_EGU3_IRQn);
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
  nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_END_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_END_DISABLE_MASK);

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
  irq_enable(SWI3_EGU3_IRQn);
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_RXEN);

  state = EMBENET_RADIO_STATE_RX_LISTENING;

  return EMBENET_RADIO_STATUS_SUCCESS;
}

EMBENET_RADIO_RxInfo EMBENET_RADIO_GetReceivedFrame(uint8_t *buffer, size_t bufferLength) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return (EMBENET_RADIO_RxInfo){.crcValid = false, .lqi = 0, .mpduLength = 0, .rssi = (int8_t)(-127)};
  }
  // Slot ISR - see the note in EMBENET_RADIO_RxEnable().
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
    nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_END_START_MASK);
    nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_END_DISABLE_MASK);
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
