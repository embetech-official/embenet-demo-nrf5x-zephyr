/**
 * @file
 * @license   see LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.0.0
 * @purpose   embeNET Node/Border Router port for Zephyr
 * @brief     Abort handler for the libraries' EXPECT contract checks
 */

#include <embetech/expect.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(embenet_expect, CONFIG_EMBENET_LOG_LEVEL);

// Shared by both roles: the symbol may be defined only once in an all-in-one image.
void EXPECT_OnAbortHandler(char const *why, char const *file, int line) {
  // The release bundles in libs/ were built with EMBEUTILS_EXPECT_VERBOSE=0, so why and file arrive
  // as "<verbose-disabled>" and the caller address is the only thing identifying the failed check.
  // Resolve it with: arm-zephyr-eabi-addr2line -f -e zephyr.elf <address>
  LOG_ERR("embeNET contract violation: %s (%s:%d), called from %p", why, file, line, __builtin_return_address(0));

  // Routed into Zephyr's fatal-error path so the failure reaches whatever the application has
  // configured for a panic. Must not return.
  LOG_PANIC();
  k_panic();
  CODE_UNREACHABLE;
}
