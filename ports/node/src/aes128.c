/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     AES-128-ECB: hardware ECB peripheral to encrypt, software cipher to decrypt
 */

#include <embenet_port/aes128.h>

#include "aes128_ecb.h"

#include <ocrypto_aes_ecb.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(embenet_aes128, CONFIG_EMBENET_LOG_LEVEL);

enum { AES128_BLOCK_SIZE = 16U };

// Every function here is reachable from the slot ISR, decrypt included, so none of them may touch
// PSA: its dispatch layer takes a mutex on the way to the driver. That rules out psa_cipher_*() and
// rules in two things that are pure computation - the ECB peripheral for encryption, and ocrypto
// for decryption, since ECB has no inverse cipher.
//
// ocrypto_aes_ecb_init_dec() builds the inverse key schedule, which is the expensive half (the
// previous port measured >100 us). Keeping the schedule across calls means SetKey stays a handful
// of stores and only the first decrypt after a rekey pays for it.

static uint8_t current_key[AES128_BLOCK_SIZE];
/// Set by SetKey, cleared once dec_ctx holds the schedule for current_key.
static bool volatile dec_ctx_stale = true;
/// False until SetKey has run, so that Decrypt reports a missing key instead of deciphering zeros.
static bool volatile key_present;
static ocrypto_aes_ecb_ctx dec_ctx;

void EMBENET_AES128_Init(void) { embenet_ecb_init(); }

void EMBENET_AES128_Deinit(void) {
  embenet_ecb_deinit();
  memset(current_key, 0, sizeof(current_key));
  memset(&dec_ctx, 0, sizeof(dec_ctx));
  dec_ctx_stale = true;
  key_present = false;
}

void EMBENET_AES128_SetKey(uint8_t const key[16U]) {
  embenet_ecb_set_key(key);
  memcpy(current_key, key, sizeof(current_key));
  dec_ctx_stale = true;
  key_present = true;
}

void EMBENET_AES128_Encrypt(uint8_t data[16U]) {
  // No way to report failure through the port API, so leave the caller's block untouched rather
  // than half-transformed.
  if(!embenet_ecb_encrypt(data)) {
    LOG_ERR("ECB encrypt failed");
  }
}

void EMBENET_AES128_Decrypt(uint8_t data[16U]) {
  if(!key_present) {
    LOG_ERR("no key set");
    return;
  }
  if(dec_ctx_stale) {
    ocrypto_aes_ecb_init_dec(&dec_ctx, current_key, sizeof(current_key));
    dec_ctx_stale = false;
  }
  ocrypto_aes_ecb_update_dec(&dec_ctx, data, data, AES128_BLOCK_SIZE);
}
