/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     2.4 GHz radio, nRF52 and the nRF5340 network core
 *
 * The nRF5340 network core has the nRF52 RADIO, so it shares this backend. What differs is the routing of the frame
 * events: DPPI publish/subscribe to EGU0 instead of PPI endpoint pairs to EGU3, the TX power range, and two errata
 * (see radioTrimRestore() and radioModeErrataApply()).
 */

#include <embenet_port/radio.h>
#include <embenet_port/timer.h>
#include <embetech/expect.h>
#include <hal/nrf_clock.h>
#include <hal/nrf_egu.h>
#if defined(CONFIG_SOC_NRF5340_CPUNET)
#include <hal/nrf_dppi.h>
#include <helpers/nrfx_gppi.h>
#else
#include <hal/nrf_ppi.h>
#endif
#include <hal/nrf_radio.h>
#if defined(CONFIG_EMBENET_NODE_MULTIPROTOCOL)
#include <mpsl_hwres.h>
#endif
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

#if defined(CONFIG_SOC_NRF5340_CPUNET)

// The network core has a single EGU. MPSL's low priority interrupt is SWI0, so EGU0 is free.
#define RADIO_EGU NRF_EGU0
#define RADIO_EGU_IRQn EGU0_IRQn

// Two DPPI channels carry the RADIO events over to EGU0. Allocated rather than hardcoded as on the
// nRF52 and nRF54L: here MPSL reserves channels 0 to 2 and the Bluetooth controller 3 to 12, and the
// RTC pretick workaround allocates one at boot - the allocator knows all of them. Taken once at
// boot, for the lifetime of the image: with the transceiver shared, Init() runs in MPSL's zero
// latency interrupt, where the allocator's critical section does not hold.
static int dppiChAddress = -1;
static int dppiChEnd = -1;
#define DPPI_CH_ADDRESS ((uint8_t)dppiChAddress)
#define DPPI_CH_END ((uint8_t)dppiChEnd)

#else

#define RADIO_EGU NRF_EGU3
#define RADIO_EGU_IRQn SWI3_EGU3_IRQn

// PPI channels 0 and 1 carry the RADIO events over to EGU3. Hardcoded rather than allocated; MPSL
// keeps to 19, 30 and 31, so sharing the transceiver does not move them. Asserted rather than
// assumed, because that list is MPSL's to change.
#define PPI_CH_ADDRESS NRF_PPI_CHANNEL0
#define PPI_CH_END NRF_PPI_CHANNEL1

#if defined(CONFIG_EMBENET_NODE_MULTIPROTOCOL)
BUILD_ASSERT(0U == ((BIT(PPI_CH_ADDRESS) | BIT(PPI_CH_END)) & MPSL_PPI_CHANNELS_USED_MASK),
             "the radio port and MPSL would drive the same PPI channel");
#endif

#endif // CONFIG_SOC_NRF5340_CPUNET

#define EMBENET_RADIO_CHANNEL_COUNT 40 // BLE channel numbering, 0 to 39
#define EMBENET_RADIO_MAX_CHANNEL (EMBENET_RADIO_CHANNEL_COUNT - 1)
#define EMBENET_RADIO_RSSI_OFFSET (-4)
#define EMBENET_RADIO_TX_TX_START_CORRECTION (40) // time difference between start of frame ISR and actual first bit being sent by radio
#define EMBENET_RADIO_TX_RX_END_CORRECTION (10)   // time difference between end of frame ISR on receiver and transmitter side (receiver - sender)

// Radio properties, reported through the EMBENET_RADIO_Get*() getters. Constants, so they hold in every radio state.
// RADIO_TxNow -> TX ISR 85us, (45us of radio preparing and power ramping, 40us of preamble and address field transmission)
#define RADIO_IDLE_TO_TX_READY_US 400
#define RADIO_IDLE_TO_RX_READY_US 400
#define RADIO_ACTIVE_TO_TX_READY_US 20
#define RADIO_ACTIVE_TO_RX_READY_US 20
#define RADIO_TX_DELAY_US 45
#define RADIO_RX_DELAY_US 45          // between GO signal and start listening - cannot measure but could be the same as delayTx
#define RADIO_TX_RX_START_DELAY_US 50 // additional 10us takes the transceiver to trigger start of frame ISR on receiver side
#define RADIO_SENSITIVITY_DBM (-95)   // does not consider as neighbor if RSSI will be lower
// TXPOWER takes the power in dBm, but only the steps the chip defines; anything else is undefined. The nRF52833 and
// nRF52840 define every step from +2 to +8 dBm, the nRF52832 only +3 and +4 above 0 dBm - on it a +8 dBm write
// leaves a node that hears the network but is never heard, so it synchronizes and never joins.
// The nRF5340 network core tops out at 0 dBm and defines every step down to -8 dBm, then only
// -12, -16, -20 and -40.
#if defined(CONFIG_SOC_NRF5340_CPUNET)
#define RADIO_MAX_TX_POWER_DBM 0
#define RADIO_MIN_TX_POWER_DBM (-8)
#elif defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
#define RADIO_MAX_TX_POWER_DBM 8
#define RADIO_MIN_TX_POWER_DBM 2
#else
#define RADIO_MAX_TX_POWER_DBM 4
#define RADIO_MIN_TX_POWER_DBM 3
#endif

typedef void (*FrameCallback)(void *context, uint32_t timestampUs);

#define CRC_LENGTH 2            ///< Length of CRC in 802.15.4 frames [bytes]
#define CRC_POLYNOMIAL 0x011021 ///< Polynomial used for CRC calculation in 802.15.4 frames

/// driver states
typedef enum {
  EMBENET_RADIO_STATE_UNINITIALIZED = 0, //!< uninitialized
  EMBENET_RADIO_STATE_IDLE,              //!< idle
  EMBENET_RADIO_STATE_ACTIVE,            //!< active
  EMBENET_RADIO_STATE_TX_PREPARED,       //!< prepared to transmitting
  EMBENET_RADIO_STATE_RX_PREPARED,       //!< prepared to receiving
  EMBENET_RADIO_STATE_RX_LISTENING,      //!< listening
  EMBENET_RADIO_STATE_RX_FRAME,          //!< receiving state
  EMBENET_RADIO_STATE_TX_FRAME,          //!< transmitting frame
  EMBENET_RADIO_STATE_RX_CAD,            //!< doing channel activity detection
} RadioState;

// structure to hold radio variables and states
typedef struct {
  FrameCallback onStartOfFrameHandler; ///< handler to method called when start of frame
                                       ///< interrupt occurs
  FrameCallback onEndOfFrameHandler;   ///< handler to method called when end of frame
                                       ///< interrupt occurs
  void *handlersContext;               ///< context passed to handlers
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
  if(power < RADIO_MIN_TX_POWER_DBM) {
    power = RADIO_MIN_TX_POWER_DBM;
  }
  if(power > RADIO_MAX_TX_POWER_DBM) {
    power = RADIO_MAX_TX_POWER_DBM;
  }
  // Negative levels are the two's complement in the 8-bit field; sign-extending into the reserved bits above is not.
  NRF_RADIO->TXPOWER = (uint32_t)(uint8_t)power;
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
  nrf_egu_event_clear(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED0);
  nrf_egu_event_clear(RADIO_EGU, NRF_EGU_EVENT_TRIGGERED1);
  NVIC_ClearPendingIRQ(RADIO_EGU_IRQn);
}

#if defined(CONFIG_SOC_NRF5340_CPUNET)
static int frameEventsChannelsAlloc(void) {
  dppiChAddress = nrfx_gppi_channel_alloc(0);
  dppiChEnd = nrfx_gppi_channel_alloc(0);
  if((dppiChAddress < 0) || (dppiChEnd < 0)) {
    LOG_ERR("no DPPI channel for the frame events: the radio will not initialize");
    dppiChEnd = -1;
  }
  return 0;
}

SYS_INIT(frameEventsChannelsAlloc, APPLICATION, 0);
#endif

/// The DPPI channels of frameEventsRoute() are there - always, on the nRF52.
static inline bool frameEventsChannelsReady(void) {
#if defined(CONFIG_SOC_NRF5340_CPUNET)
  return dppiChEnd >= 0;
#else
  return true;
#endif
}

/// Connects the RADIO's ADDRESS and END events to the EGU, whose interrupt runs radio_isr().
static inline void frameEventsRoute(void) {
#if defined(CONFIG_SOC_NRF5340_CPUNET)
  // DPPI carries the connection as a channel both ends name: the RADIO publishes its event onto a
  // channel, the EGU subscribes to it. The RADIO's power cycle in Init() clears its publish
  // registers, so they are set again every time.
  nrf_radio_publish_set(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS, DPPI_CH_ADDRESS);
  nrf_radio_publish_set(RADIO_DEVICE, NRF_RADIO_EVENT_END, DPPI_CH_END);
  nrf_egu_subscribe_set(RADIO_EGU, NRF_EGU_TASK_TRIGGER0, DPPI_CH_ADDRESS);
  nrf_egu_subscribe_set(RADIO_EGU, NRF_EGU_TASK_TRIGGER1, DPPI_CH_END);
  nrf_dppi_channels_enable(NRF_DPPIC, BIT(DPPI_CH_ADDRESS) | BIT(DPPI_CH_END));
#else
  nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_CH_ADDRESS, (uint32_t)&NRF_RADIO->EVENTS_ADDRESS, (uint32_t)&RADIO_EGU->TASKS_TRIGGER[0]);
  nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_CH_END, (uint32_t)&NRF_RADIO->EVENTS_END, (uint32_t)&RADIO_EGU->TASKS_TRIGGER[1]);
  nrf_ppi_channel_enable(NRF_PPI, PPI_CH_ADDRESS);
  nrf_ppi_channel_enable(NRF_PPI, PPI_CH_END);
#endif
}

static inline void frameEventsUnroute(void) {
#if defined(CONFIG_SOC_NRF5340_CPUNET)
  if(dppiChEnd < 0) {
    return; // never routed
  }
  nrf_dppi_channels_disable(NRF_DPPIC, BIT(DPPI_CH_ADDRESS) | BIT(DPPI_CH_END));
  nrf_radio_publish_clear(RADIO_DEVICE, NRF_RADIO_EVENT_ADDRESS);
  nrf_radio_publish_clear(RADIO_DEVICE, NRF_RADIO_EVENT_END);
#else
  nrf_ppi_channel_disable(NRF_PPI, PPI_CH_ADDRESS);
  nrf_ppi_channel_disable(NRF_PPI, PPI_CH_END);
#endif
}

/**
 * nRF5340 network core erratum 158: powering the RADIO off and on through POWER loses the trim values the MDK copied from
 * FICR at boot, so the RADIO's own are copied back - as the 802.15.4 driver does. After every power cycle: in Init(), and
 * in Deinit() when the transceiver goes back to the other protocol. Nothing on the nRF52.
 */
static inline void radioTrimRestore(void) {
#if NRF53_ERRATA_158_ENABLE_WORKAROUND
  if(nrf53_errata_158()) {
    for(uint32_t i = 0; i < ARRAY_SIZE(NRF_FICR->TRIMCNF); i++) {
      uint32_t const address = NRF_FICR->TRIMCNF[i].ADDR;
      if((address & 0xFFFFF000UL) == (uint32_t)NRF_RADIO) {
        *(uint32_t volatile *)address = NRF_FICR->TRIMCNF[i].DATA;
      }
    }
  }
#endif
}

/**
 * nRF5340 network core erratum 117: changing MODE needs a FICR value written to an undocumented RADIO register; this is
 * the value for the non-2 Mbit modes, as in Zephyr's Bluetooth controller. Nothing on the nRF52.
 */
static inline void radioModeErrataApply(void) {
#if NRF53_ERRATA_117_ENABLE_WORKAROUND
  if(nrf53_errata_117()) {
    *(uint32_t volatile *)0x41008588UL = *(uint32_t const volatile *)0x01FF0080UL;
  }
#endif
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

#if defined(CONFIG_EMBENET_NODE_MULTIPROTOCOL)

/**
 * Leaves the crystal to MPSL.
 *
 * Sharing the transceiver turns EMBENET_RADIO_Init() and EMBENET_RADIO_Deinit() into per-timeslot
 * calls made from MPSL's zero latency signal interrupt, where onoff_request() may not be called at
 * all and where there is nowhere near enough room to spin out the ~360 us the crystal takes to
 * settle. It is already running: every timeslot is asked for with
 * MPSL_TIMESLOT_HFCLK_CFG_XTAL_GUARANTEED (see src/multiprotocol.c), and hfclkValidate() below still
 * checks that before the radio is armed.
 */
static bool hfclkAcquireXtal(void) { return true; }

static void hfclkReleaseXtal(void) {}

#else

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

#endif // CONFIG_EMBENET_NODE_MULTIPROTOCOL

ISR_DIRECT_DECLARE(radio_isr) { //(const void *args) {
  // First statement in the ISR: the stack synchronizes on this timestamp, so anything done before
  // reading it - logging above all - lands in the frame timing as jitter.
  uint32_t now = EMBENET_TIMER_ReadCounter();

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

    if(addressEvent) {
      if(admin.onStartOfFrameHandler) {
        uint32_t startOfFrame = now;
        if(EMBENET_RADIO_STATE_TX_FRAME == state) {
          startOfFrame -= EMBENET_RADIO_TX_TX_START_CORRECTION;
        }
        if(EMBENET_RADIO_STATE_RX_LISTENING == state) {
          state = EMBENET_RADIO_STATE_RX_FRAME;
          startOfFrame -= RADIO_TX_RX_START_DELAY_US;
        }
        admin.onStartOfFrameHandler(admin.handlersContext, startOfFrame);
        LOG_DBG("radio_isr: onStartOfFrameHandler called, state=%s", getStateName());
      }
    }
    if(endEvent) {
      if(EMBENET_RADIO_STATE_RX_FRAME == state) {
        now -= EMBENET_RADIO_TX_RX_END_CORRECTION;
      }
      if(admin.onEndOfFrameHandler) {
        admin.onEndOfFrameHandler(admin.handlersContext, now);
        LOG_DBG("radio_isr: onEndOfFrameHandler called, state=%s", getStateName());
      }
    }
  }
  k_sem_give(EMBENET_PORT_COMMON_GetProcSemaphore());
  ISR_DIRECT_PM(); // done after do_stuff() due to latency concerns

  return 1;
}

bool EMBENET_RADIO_Init(void) {
  if(!frameEventsChannelsReady() || !hfclkAcquireXtal()) {
    return false;
  }
  // default state
  admin.frame[0] = 0; // the first byte transceiver stores here is the packet length
  frameEventsUnroute();
  nrf_radio_power_set(RADIO_DEVICE,
                      false); // restart radio
  nrf_radio_power_set(RADIO_DEVICE,
                      true); // all registers will be set to default
  radioTrimRestore();
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
  radioModeErrataApply();

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
    irq_disable(RADIO_EGU_IRQn);
    // ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RADIO_IRQn, 0, 0, reschedule);
    IRQ_DIRECT_CONNECT(RADIO_EGU_IRQn, 5, radio_isr, 0);
    irq_enable(RADIO_EGU_IRQn);
  }
  // On every Init, not just the first: with the transceiver shared, Init runs once per timeslot and
  // whatever drove the hardware in between is not obliged to leave this routing alone.
  nrf_egu_int_enable(RADIO_EGU, NRF_EGU_INT_TRIGGERED0);
  nrf_egu_int_enable(RADIO_EGU, NRF_EGU_INT_TRIGGERED1);
  frameEventsRoute();
  state = EMBENET_RADIO_STATE_IDLE;

  return true;
}

void EMBENET_RADIO_SetCallbacks(void (*onFrameStart)(void *context, uint32_t timestampUs), void (*onFrameEnd)(void *context, uint32_t timestampUs),
                                void *context) {
  admin.onStartOfFrameHandler = onFrameStart;
  admin.onEndOfFrameHandler = onFrameEnd;
  admin.handlersContext = context;
}

void EMBENET_RADIO_Deinit(void) {
  irq_disable(RADIO_EGU_IRQn);

  admin.onStartOfFrameHandler = NULL;
  admin.onEndOfFrameHandler = NULL;

  frameEventsUnroute();

  nrf_radio_power_set(RADIO_DEVICE, false);
#if defined(CONFIG_EMBENET_NODE_MULTIPROTOCOL)
  // Shared, the transceiver goes back to the other protocol right after this, which expects it powered: the power cycle
  // restores every register to its reset value, and the power stays on.
  nrf_radio_power_set(RADIO_DEVICE, true);
  radioTrimRestore();
#endif
  hfclkReleaseXtal();
  state = EMBENET_RADIO_STATE_UNINITIALIZED;
}

void EMBENET_RADIO_Idle(void) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return;
  }
  irq_disable(RADIO_EGU_IRQn); // fool proof method of disabling radio handler from being called

  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_DISABLE);
  clearPendingFrameEvents();

  state = EMBENET_RADIO_STATE_IDLE;
}

// Failures here are not reported: the radio stays where it was, and the MAC watchdog recovers from a slot that does not happen.
void EMBENET_RADIO_TxEnable(uint8_t channel, int8_t txPowerDbm, uint8_t const *psdu, uint32_t psduLength) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return;
  }
  if(!hfclkValidate("tx enable")) {
    return;
  }
  // Slot ISR, once per transmit slot - see the note in EMBENET_RADIO_RxEnable().
  LOG_DBG("tx enable channel %d, power %d, psduLength %u", channel, txPowerDbm, psduLength);

  setChannel(channel);
  setPower(txPowerDbm);

  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
  nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_END_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_END_DISABLE_MASK);

  if(psduLength > EMBENET_RADIO_MAX_PSDU_LENGTH) {
    psduLength = EMBENET_RADIO_MAX_PSDU_LENGTH;
  }
  admin.frame[0] = (uint8_t)psduLength;
  memcpy(&admin.frame[1], psdu, psduLength);

  state = EMBENET_RADIO_STATE_TX_PREPARED;
}

bool EMBENET_RADIO_TxNow(void) {
  EXPECT_EXTRA(hfclkRunningOnXtal())
  OR_ABORT("HFCLK not running");

  // No listen before talk: the channel is never reported busy, so this always answers true.
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return true;
  }
  clearPendingFrameEvents();
  irq_enable(RADIO_EGU_IRQn);
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_TXEN);

  state = EMBENET_RADIO_STATE_TX_FRAME;

  return true;
}

void EMBENET_RADIO_RxEnable(uint8_t channel) {
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return;
  }
  if(!hfclkValidate("rx enable")) {
    return;
  }

  setChannel(channel);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_READY_START_MASK);
  nrf_radio_shorts_disable(RADIO_DEVICE, NRF_RADIO_SHORT_END_START_MASK);
  nrf_radio_shorts_enable(RADIO_DEVICE, NRF_RADIO_SHORT_END_DISABLE_MASK);

  state = EMBENET_RADIO_STATE_RX_PREPARED;
}

void EMBENET_RADIO_RxNow(void) {
  EXPECT_EXTRA(hfclkRunningOnXtal())
  OR_ABORT("HFCLK not running");

  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return;
  }
  clearPendingFrameEvents();
  irq_enable(RADIO_EGU_IRQn);
  nrf_radio_task_trigger(RADIO_DEVICE, NRF_RADIO_TASK_RXEN);

  state = EMBENET_RADIO_STATE_RX_LISTENING;
}

uint32_t EMBENET_RADIO_GetReceivedFrame(uint8_t *buffer, uint32_t bufferLength, int8_t *rssiDbm, bool *crcValid) {
  *rssiDbm = INT8_MIN;
  *crcValid = false;
  if(EMBENET_RADIO_STATE_UNINITIALIZED == state) {
    return 0;
  }
  // Slot ISR - see the note in EMBENET_RADIO_RxEnable().
  LOG_DBG("get received frame bufferLength %u", bufferLength);

  uint32_t const packetLength = admin.frame[0];
  if(0 == packetLength) {
    return 0;
  }
  *crcValid = nrf_radio_crc_status_check(RADIO_DEVICE);
  *rssiDbm = (int8_t)(EMBENET_RADIO_RSSI_OFFSET - (int)nrf_radio_rssi_sample_get(RADIO_DEVICE));
  // The length is reported as received, not clamped, so the stack recognizes an oversize frame.
  memcpy(buffer, &admin.frame[1], MIN(packetLength, bufferLength));
  return packetLength;
}

uint32_t EMBENET_RADIO_GetChannelCount(void) { return EMBENET_RADIO_CHANNEL_COUNT; }
uint32_t EMBENET_RADIO_GetIdleToTxReadyUs(void) { return RADIO_IDLE_TO_TX_READY_US; }
uint32_t EMBENET_RADIO_GetIdleToRxReadyUs(void) { return RADIO_IDLE_TO_RX_READY_US; }
uint32_t EMBENET_RADIO_GetActiveToTxReadyUs(void) { return RADIO_ACTIVE_TO_TX_READY_US; }
uint32_t EMBENET_RADIO_GetActiveToRxReadyUs(void) { return RADIO_ACTIVE_TO_RX_READY_US; }
uint32_t EMBENET_RADIO_GetTxDelayUs(void) { return RADIO_TX_DELAY_US; }
uint32_t EMBENET_RADIO_GetRxDelayUs(void) { return RADIO_RX_DELAY_US; }
uint32_t EMBENET_RADIO_GetTxRxStartDelayUs(void) { return RADIO_TX_RX_START_DELAY_US; }
int8_t EMBENET_RADIO_GetSensitivityDbm(void) { return RADIO_SENSITIVITY_DBM; }
int8_t EMBENET_RADIO_GetMaxTxPowerDbm(void) { return RADIO_MAX_TX_POWER_DBM; }
int8_t EMBENET_RADIO_GetMinTxPowerDbm(void) { return RADIO_MIN_TX_POWER_DBM; }
