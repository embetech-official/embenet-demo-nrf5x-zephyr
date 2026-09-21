/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     AES-128-ECB encryption on the nRF52 ECB peripheral
 */

#include "aes128_ecb.h"

#include <hal/nrf_ecb.h>

#include <string.h>

#if !NRF_ECB_HAS_ECBDATAPTR
#error "this backend expects the nRF52-style ECB peripheral - see aes128_ecb_nrf54l.c"
#endif

// The peripheral reads one contiguous block of RAM holding key, cleartext and ciphertext in that
// order. ECBDATAPTR is written once and then never changes.
static struct {
  uint8_t key[16];
  uint8_t cleartext[16];
  uint8_t ciphertext[16];
} ecb_data;

void embenet_ecb_init(void) { nrf_ecb_data_pointer_set(NRF_ECB, &ecb_data); }

void embenet_ecb_deinit(void) {
  nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STOPECB);
  memset(&ecb_data, 0, sizeof(ecb_data));
}

void embenet_ecb_set_key(uint8_t const key[16U]) { memcpy(ecb_data.key, key, sizeof(ecb_data.key)); }

bool embenet_ecb_encrypt(uint8_t data[16U]) {
  memcpy(ecb_data.cleartext, data, sizeof(ecb_data.cleartext));

  nrf_ecb_event_clear(NRF_ECB, NRF_ECB_EVENT_ENDECB);
  nrf_ecb_event_clear(NRF_ECB, NRF_ECB_EVENT_ERRORECB);
  nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STARTECB);

  // A block takes ~7 us at 64 MHz; the bound only exists so that a wedged peripheral cannot hang
  // the slot ISR for good. ERRORECB means EasyDMA lost the bus to a higher-priority peripheral, in
  // which case the output buffer holds garbage.
  for(uint32_t guard = 0x100000U; guard != 0U; --guard) {
    if(nrf_ecb_event_check(NRF_ECB, NRF_ECB_EVENT_ENDECB)) {
      nrf_ecb_event_clear(NRF_ECB, NRF_ECB_EVENT_ENDECB);
      memcpy(data, ecb_data.ciphertext, sizeof(ecb_data.ciphertext));
      return true;
    }
    if(nrf_ecb_event_check(NRF_ECB, NRF_ECB_EVENT_ERRORECB)) {
      nrf_ecb_event_clear(NRF_ECB, NRF_ECB_EVENT_ERRORECB);
      break;
    }
  }
  return false;
}
