/**
 * @file
 * @brief embeNET Node AES-128 port test.
 *
 * Checks EMBENET_AES128_* against the FIPS-197 known-answer vectors and then over the paths a port
 * is most likely to get wrong: a rekey between two keys, and a deinit/init cycle.
 *
 * The two directions do not share an implementation - encrypt runs on the SoC's ECB peripheral,
 * decrypt on a software cipher, because ECB has no inverse - so the tests deliberately report the
 * two separately. A known-answer failure on one and not the other says immediately which half is at
 * fault, and a round trip that passes while both known-answer tests fail means the two halves agree
 * with each other but not with AES.
 */

#include <embenet_port/aes128.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

enum { AES128_BLOCK_SIZE = 16U };

// FIPS-197, appendix C.1.
static uint8_t const FIPS197_KEY[AES128_BLOCK_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                                       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
static uint8_t const FIPS197_PLAINTEXT[AES128_BLOCK_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
static uint8_t const FIPS197_CIPHERTEXT[AES128_BLOCK_SIZE] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
                                                              0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};

// FIPS-197, appendix B. A second key, so that a port which ignores EMBENET_AES128_SetKey and always
// uses the first one it was given cannot pass.
static uint8_t const APPB_KEY[AES128_BLOCK_SIZE] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static uint8_t const APPB_PLAINTEXT[AES128_BLOCK_SIZE] = {0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
                                                          0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34};
static uint8_t const APPB_CIPHERTEXT[AES128_BLOCK_SIZE] = {0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb,
                                                           0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32};

static bool check_block(char const *what, uint8_t const actual[AES128_BLOCK_SIZE], uint8_t const expected[AES128_BLOCK_SIZE]) {
  if(0 == memcmp(actual, expected, AES128_BLOCK_SIZE)) {
    return true;
  }
  LOG_ERR("%s: wrong block", what);
  LOG_HEXDUMP_ERR(expected, AES128_BLOCK_SIZE, "expected");
  LOG_HEXDUMP_ERR(actual, AES128_BLOCK_SIZE, "actual  ");
  return false;
}

/// Encrypts a known plaintext and compares against the published ciphertext.
static bool test_known_answer_encrypt(void) {
  uint8_t block[AES128_BLOCK_SIZE];

  EMBENET_AES128_SetKey(FIPS197_KEY);
  memcpy(block, FIPS197_PLAINTEXT, sizeof(block));
  EMBENET_AES128_Encrypt(block);
  return check_block("encrypt(FIPS-197 C.1)", block, FIPS197_CIPHERTEXT);
}

/// Decrypts a known ciphertext and compares against the published plaintext.
static bool test_known_answer_decrypt(void) {
  uint8_t block[AES128_BLOCK_SIZE];

  EMBENET_AES128_SetKey(FIPS197_KEY);
  memcpy(block, FIPS197_CIPHERTEXT, sizeof(block));
  EMBENET_AES128_Decrypt(block);
  return check_block("decrypt(FIPS-197 C.1)", block, FIPS197_PLAINTEXT);
}

/// The same, under a different key, in both directions.
static bool test_known_answer_second_key(void) {
  uint8_t block[AES128_BLOCK_SIZE];
  bool ok = true;

  EMBENET_AES128_SetKey(APPB_KEY);

  memcpy(block, APPB_PLAINTEXT, sizeof(block));
  EMBENET_AES128_Encrypt(block);
  ok = check_block("encrypt(FIPS-197 B)", block, APPB_CIPHERTEXT) && ok;

  memcpy(block, APPB_CIPHERTEXT, sizeof(block));
  EMBENET_AES128_Decrypt(block);
  ok = check_block("decrypt(FIPS-197 B)", block, APPB_PLAINTEXT) && ok;

  return ok;
}

/**
 * Encrypts then decrypts a spread of blocks and expects the original back.
 *
 * The patterns are fixed rather than random so that a failure is reproducible, and they cover the
 * cases where a byte-order or length slip shows up most readily: all-zero, all-ones, a byte ramp,
 * and a counter that walks a single changing byte through the block.
 */
static bool test_round_trip(void) {
  uint8_t original[AES128_BLOCK_SIZE];
  uint8_t block[AES128_BLOCK_SIZE];
  bool ok = true;

  EMBENET_AES128_SetKey(FIPS197_KEY);

  for(unsigned pattern = 0; pattern < 4U + AES128_BLOCK_SIZE; ++pattern) {
    for(size_t i = 0; i < AES128_BLOCK_SIZE; ++i) {
      switch(pattern) {
      case 0:
        original[i] = 0x00;
        break;
      case 1:
        original[i] = 0xFF;
        break;
      case 2:
        original[i] = (uint8_t)i;
        break;
      case 3:
        original[i] = (uint8_t)(0xFF - i);
        break;
      default:
        // One block per byte position, that byte set and the rest clear.
        original[i] = (i == (pattern - 4U)) ? 0xA5 : 0x00;
        break;
      }
    }

    memcpy(block, original, sizeof(block));
    EMBENET_AES128_Encrypt(block);

    if(0 == memcmp(block, original, sizeof(block))) {
      LOG_ERR("round trip pattern %u: encrypt left the block unchanged", pattern);
      ok = false;
      continue;
    }

    EMBENET_AES128_Decrypt(block);
    if(0 != memcmp(block, original, sizeof(block))) {
      LOG_ERR("round trip pattern %u: decrypt did not restore the plaintext", pattern);
      LOG_HEXDUMP_ERR(original, AES128_BLOCK_SIZE, "expected");
      LOG_HEXDUMP_ERR(block, AES128_BLOCK_SIZE, "actual  ");
      ok = false;
    }
  }
  return ok;
}

/**
 * Alternates between two keys, checking every operation against its own vector.
 *
 * This is the test that catches a port which prepares a key schedule once and then keeps using it
 * after EMBENET_AES128_SetKey has been called again - a plausible optimisation on the decrypt side,
 * where building the inverse schedule is the expensive part.
 */
static bool test_rekey(void) {
  uint8_t block[AES128_BLOCK_SIZE];
  bool ok = true;

  for(unsigned round = 0; round < 3U; ++round) {
    EMBENET_AES128_SetKey(FIPS197_KEY);
    memcpy(block, FIPS197_CIPHERTEXT, sizeof(block));
    EMBENET_AES128_Decrypt(block);
    ok = check_block("rekey: decrypt under key 1", block, FIPS197_PLAINTEXT) && ok;

    EMBENET_AES128_SetKey(APPB_KEY);
    memcpy(block, APPB_CIPHERTEXT, sizeof(block));
    EMBENET_AES128_Decrypt(block);
    ok = check_block("rekey: decrypt under key 2", block, APPB_PLAINTEXT) && ok;

    // The encrypt side has its own idea of the current key - on this port a hardware key register
    // rather than a schedule in RAM - so it needs its own check under the second key. Without this
    // the whole test only ever encrypts under key 1, and a port that latched the first key it was
    // given would sail through.
    memcpy(block, APPB_PLAINTEXT, sizeof(block));
    EMBENET_AES128_Encrypt(block);
    ok = check_block("rekey: encrypt under key 2", block, APPB_CIPHERTEXT) && ok;

    // Back to the first key, to prove the switch works in both directions and not just once.
    EMBENET_AES128_SetKey(FIPS197_KEY);
    memcpy(block, FIPS197_PLAINTEXT, sizeof(block));
    EMBENET_AES128_Encrypt(block);
    ok = check_block("rekey: encrypt under key 1", block, FIPS197_CIPHERTEXT) && ok;
  }
  return ok;
}

/// A deinit/init cycle has to leave the port usable, the way a stack restart drives it.
static bool test_reinit(void) {
  uint8_t block[AES128_BLOCK_SIZE];
  bool ok = true;

  EMBENET_AES128_Deinit();
  EMBENET_AES128_Init();

  // Port behaviour, not an AES property: with no key loaded there is no way to report failure
  // through this API, so a decrypt leaves the caller's block alone rather than filling it with
  // rubbish derived from a zero key.
  memcpy(block, FIPS197_CIPHERTEXT, sizeof(block));
  EMBENET_AES128_Decrypt(block);
  if(0 != memcmp(block, FIPS197_CIPHERTEXT, sizeof(block))) {
    LOG_ERR("reinit: decrypt without a key modified the block");
    ok = false;
  }

  EMBENET_AES128_SetKey(FIPS197_KEY);

  memcpy(block, FIPS197_PLAINTEXT, sizeof(block));
  EMBENET_AES128_Encrypt(block);
  ok = check_block("reinit: encrypt", block, FIPS197_CIPHERTEXT) && ok;

  memcpy(block, FIPS197_CIPHERTEXT, sizeof(block));
  EMBENET_AES128_Decrypt(block);
  ok = check_block("reinit: decrypt", block, FIPS197_PLAINTEXT) && ok;

  return ok;
}

/**
 * Checks that neither direction writes outside the 16 bytes it was handed.
 *
 * Every other test compares exactly the block, so a port that runs off the end of it is invisible to
 * them. That is worth checking here rather than assuming: both backends move the block through a
 * staging buffer on its way to the peripheral - a memcpy pair on nRF52, an EasyVDMA job with its own
 * length field on nRF54L - and a length slip in either would corrupt whatever sits next to the
 * caller's buffer instead of failing visibly.
 */
static bool test_block_bounds(void) {
  enum { GUARD_SIZE = 8U, GUARD_BYTE = 0x5AU };
  struct {
    uint8_t before[GUARD_SIZE];
    uint8_t block[AES128_BLOCK_SIZE];
    uint8_t after[GUARD_SIZE];
  } guarded;
  bool ok = true;

  EMBENET_AES128_SetKey(FIPS197_KEY);

  for(unsigned direction = 0; direction < 2U; ++direction) {
    bool const encrypting = (0U == direction);
    char const *const what = encrypting ? "bounds: encrypt" : "bounds: decrypt";

    memset(&guarded, GUARD_BYTE, sizeof(guarded));
    memcpy(guarded.block, encrypting ? FIPS197_PLAINTEXT : FIPS197_CIPHERTEXT, sizeof(guarded.block));

    if(encrypting) {
      EMBENET_AES128_Encrypt(guarded.block);
    } else {
      EMBENET_AES128_Decrypt(guarded.block);
    }

    // The transform itself still has to be right, or a port that does nothing at all would pass.
    ok = check_block(what, guarded.block, encrypting ? FIPS197_CIPHERTEXT : FIPS197_PLAINTEXT) && ok;

    for(size_t i = 0; i < GUARD_SIZE; ++i) {
      if(GUARD_BYTE != guarded.before[i]) {
        LOG_ERR("%s: wrote %u bytes before the block", what, (unsigned)(GUARD_SIZE - i));
        ok = false;
        break;
      }
    }
    for(size_t i = 0; i < GUARD_SIZE; ++i) {
      if(GUARD_BYTE != guarded.after[i]) {
        LOG_ERR("%s: wrote %u bytes past the block", what, (unsigned)(i + 1U));
        ok = false;
        break;
      }
    }
  }
  return ok;
}

static struct {
  char const *name;
  bool (*run)(void);
} const tests[] = {
    {"known-answer encrypt", test_known_answer_encrypt},
    {"known-answer decrypt", test_known_answer_decrypt},
    {"known-answer second key", test_known_answer_second_key},
    {"round trip", test_round_trip},
    {"rekey", test_rekey},
    {"block bounds", test_block_bounds},
    {"deinit/init", test_reinit},
};

int main(void) {
  LOG_INF("embeNET Node AES-128 port test");

  EMBENET_AES128_Init();

  unsigned failed = 0;
  for(size_t i = 0; i < ARRAY_SIZE(tests); ++i) {
    if(tests[i].run()) {
      LOG_INF("PASS  %s", tests[i].name);
    } else {
      LOG_ERR("FAIL  %s", tests[i].name);
      ++failed;
    }
  }

  EMBENET_AES128_Deinit();

  if(0U == failed) {
    LOG_INF("PROJECT EXECUTION SUCCESSFUL: %zu/%zu tests passed", ARRAY_SIZE(tests), ARRAY_SIZE(tests));
  } else {
    LOG_ERR("PROJECT EXECUTION FAILED: %u of %zu tests failed", failed, ARRAY_SIZE(tests));
  }
  return 0;
}
