/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     AES-128-ECB encryption on the nRF54L ECB peripheral
 */

#include "aes128_ecb.h"

#include <hal/nrf_ecb.h>

#include <string.h>

#if !NRF_ECB_HAS_KEY || !defined(NRF_ECB00)
#error "this backend expects the nRF54L-style ECB peripheral - see aes128_ecb_nrf52.c"
#endif

// nRF54L15 has no devicetree node for ECB00, so nothing else in the system claims it.
#define ECB_DEVICE NRF_ECB00

// The key lives in registers and the data moves over EasyVDMA, so the peripheral gets two
// single-entry job lists rather than one data block. The job attribute and the key byte order
// follow Nordic's own ECB test, modules/bsim_hw_models/nrf_hw_models/tests/zephyr_apps/
// 54_aar_ccm_ecb/src/test_ecb.c.
enum { ECB_JOB_ATTRIBUTE = 0x0BU };

static uint8_t ecb_cleartext[16];
static uint8_t ecb_ciphertext[16];

// A zero-length entry terminates a job list.
//
// Deliberately not const: EasyDMA reads the job list itself over the DMA bus, so the list has to
// live in RAM just as much as the buffers it points at. Marking it const puts it in .rodata, which
// on this SoC is RRAM - and the datasheet's rule that INPTR, OUTPTR and the job entries must all
// point at DMA-connected memory then fails the transfer with an ERROR event rather than a fault.
static nrf_vdma_job_t ecb_in_job[] = {{.p_buffer = ecb_cleartext, .size = sizeof(ecb_cleartext), .attributes = ECB_JOB_ATTRIBUTE}, {0}};
static nrf_vdma_job_t ecb_out_job[] = {{.p_buffer = ecb_ciphertext, .size = sizeof(ecb_ciphertext), .attributes = ECB_JOB_ATTRIBUTE}, {0}};

void embenet_ecb_init(void) {
  nrf_ecb_in_ptr_set(ECB_DEVICE, ecb_in_job);
  nrf_ecb_out_ptr_set(ECB_DEVICE, ecb_out_job);
}

void embenet_ecb_deinit(void) {
  nrf_ecb_task_trigger(ECB_DEVICE, NRF_ECB_TASK_STOP);
  uint8_t const zeros[16] = {0};
  nrf_ecb_key_set(ECB_DEVICE, (uint32_t const *)zeros);
  memset(ecb_cleartext, 0, sizeof(ecb_cleartext));
  memset(ecb_ciphertext, 0, sizeof(ecb_ciphertext));
}

void embenet_ecb_set_key(uint8_t const key[16U]) {
  // KEY.VALUE[] reads the key back to front.
  uint8_t swapped[16];
  for(size_t i = 0; i < sizeof(swapped); ++i) {
    swapped[sizeof(swapped) - 1U - i] = key[i];
  }
  nrf_ecb_key_set(ECB_DEVICE, (uint32_t const *)swapped);
}

bool embenet_ecb_encrypt(uint8_t data[16U]) {
  memcpy(ecb_cleartext, data, sizeof(ecb_cleartext));

  nrf_ecb_event_clear(ECB_DEVICE, NRF_ECB_EVENT_END);
  nrf_ecb_event_clear(ECB_DEVICE, NRF_ECB_EVENT_ERROR);
  nrf_ecb_task_trigger(ECB_DEVICE, NRF_ECB_TASK_START);

  // A block takes a few microseconds; the bound only exists so that a wedged peripheral cannot hang
  // the slot ISR for good.
  for(uint32_t guard = 0x100000U; guard != 0U; --guard) {
    if(nrf_ecb_event_check(ECB_DEVICE, NRF_ECB_EVENT_END)) {
      nrf_ecb_event_clear(ECB_DEVICE, NRF_ECB_EVENT_END);
      memcpy(data, ecb_ciphertext, sizeof(ecb_ciphertext));
      return true;
    }
    if(nrf_ecb_event_check(ECB_DEVICE, NRF_ECB_EVENT_ERROR)) {
      nrf_ecb_event_clear(ECB_DEVICE, NRF_ECB_EVENT_ERROR);
      break;
    }
  }
  return false;
}
