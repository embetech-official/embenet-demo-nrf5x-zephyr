/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node port for Zephyr
 * @brief     Backend interface to the ECB peripheral, one implementation per SoC family
 */

#ifndef EMBENET_NODE_PORT_AES128_ECB_H_
#define EMBENET_NODE_PORT_AES128_ECB_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Every function here is called from the slot ISR, so an implementation may only touch registers
 * and plain memory: no kernel objects, no allocation, no PSA. The peripheral encrypts only - the
 * decrypt side lives in aes128.c and runs a software cipher.
 */

/// Prepares the peripheral. Called once from EMBENET_AES128_Init(), in thread context.
void embenet_ecb_init(void);

/// Stops the peripheral and wipes whatever key material it holds.
void embenet_ecb_deinit(void);

/// Loads the encryption key. Called from the slot ISR, so it must stay a handful of stores.
void embenet_ecb_set_key(uint8_t const key[16U]);

/**
 * Encrypts one block in place. Called from the slot ISR.
 *
 * @retval true  @p data holds the ciphertext.
 * @retval false the peripheral failed; @p data is unchanged.
 */
bool embenet_ecb_encrypt(uint8_t data[16U]);

#endif // EMBENET_NODE_PORT_AES128_ECB_H_
