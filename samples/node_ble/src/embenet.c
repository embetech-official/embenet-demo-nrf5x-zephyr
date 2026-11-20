/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @purpose   Runs and wraps the embeNET Node.
 * @brief     Implements the thread-safe embeNET middleware API, see embenet.h.
 *
 * The embeNET thread is the only one that drives the stack's life cycle. It sleeps on the port's semaphore, which the
 * port's interrupts give when the stack has work, and runs EMBENET_NODE_Proc() under the module mutex.
 *
 * The configuration comes from Zephyr settings. The settings handler below only records a changed value; the thread
 * applies it once it has settled, by leaving the network, stopping the stack and starting it again with the new
 * credentials.
 *
 * @note All functions are thread safe.
 */

#include "embenet.h"

#include <embenet/node.h>
#include <embenet/node_diag.h>
#include <embenet_port/eui64.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <inttypes.h>
#include <string.h>

LOG_MODULE_REGISTER(embenet_middleware, CONFIG_EMBENET_LOG_LEVEL);

BUILD_ASSERT(sizeof(((EMBENET_NODE_JoinConfig *)0)->k1.val) == EMBENET_KEY_SIZE, "K1 size mismatch");
BUILD_ASSERT(sizeof(((EMBENET_NODE_JoinConfig *)0)->psk.val) == EMBENET_KEY_SIZE, "PSK size mismatch");
BUILD_ASSERT((int)EMBENET_NEIGHBOR_ROLE_PARENT == (int)EMBENET_NODE_DIAG_NEIGHBOR_ROLE_PARENT &&
                 (int)EMBENET_NEIGHBOR_ROLE_CHILD == (int)EMBENET_NODE_DIAG_NEIGHBOR_ROLE_CHILD &&
                 (int)EMBENET_NEIGHBOR_ROLE_UNRELATED == (int)EMBENET_NODE_DIAG_NEIGHBOR_ROLE_UNRELATED,
             "neighbor roles must map one to one");
BUILD_ASSERT(sizeof(CONFIG_EMBENET_SETTINGS_DEFAULT_K1) - 1U == 2U * EMBENET_KEY_SIZE, "CONFIG_EMBENET_SETTINGS_DEFAULT_K1 must be 32 hex digits");
BUILD_ASSERT(sizeof(CONFIG_EMBENET_SETTINGS_DEFAULT_PSK) - 1U == 2U * EMBENET_KEY_SIZE, "CONFIG_EMBENET_SETTINGS_DEFAULT_PSK must be 32 hex digits");

/// The configuration the stack runs with, as the settings hold it.
typedef struct {
  uint8_t k1[EMBENET_KEY_SIZE];
  uint8_t psk[EMBENET_KEY_SIZE];
  bool active;
} embenet_config_t;

/// How long the stopped stack waits for a configuration change between checks.
#define IDLE_POLL_MS 100
/// How long the running stack sleeps when no port interrupt wakes it.
#define PROC_POLL_MS 5
/// Pause before retrying a stack that failed to start.
#define START_RETRY_MS 1000
/// Priority of the embeNET thread. It sleeps until a port interrupt has work for it, so a high priority costs little
/// and keeps the stack responsive.
#define THREAD_PRIORITY 0
/// A change of K1, PSK or the active switch restarts the stack once no further change has come for this long, so a
/// provisioning tool writing K1 and PSK one after the other causes one restart, not two.
#define SETTINGS_APPLY_DELAY_MS 1500
/// Longest wait for the network to be left before the stack is stopped. A joined node leaves gently, which takes it
/// about 4 s; the stack is stopped after this long even if it has not reported leaving.
#define LEAVE_TIMEOUT_MS 6000

/// Guards everything below and every call into the stack. Recursive: stack callbacks run inside EMBENET_NODE_Proc(),
/// which already holds it, and may call the getters.
static K_MUTEX_DEFINE(mtx);
/// Given once by embenet_init() to let the embeNET thread run.
static K_SEM_DEFINE(start_sem, 0, 1);
/// Given by the settings handler to wake the stopped stack's thread.
static K_SEM_DEFINE(config_sem, 0, 1);

static bool init_called;
static embenet_cb_t callbacks;

/// Configuration as the settings hold it, applied by the embeNET thread once it has settled.
static embenet_config_t config;
/// true once @ref config holds the Kconfig defaults, before any setting overrides them.
static bool config_defaulted;
/// true from a change until the embeNET thread takes it over.
static bool config_pending;
/// Uptime of the last change, in ms.
static int64_t config_changed_ms;

/// true between EMBENET_NODE_Init() and EMBENET_NODE_Deinit(); the stack's getters abort outside it.
static bool stack_initialized;
static bool joined;
/// Set when the stack reports leaving; ends the wait in stop_stack().
static bool left;

/// Raises @p event to the application. Called with @ref mtx held.
static void notify(embenet_event_t event) {
  if(NULL != callbacks.event_cb) {
    callbacks.event_cb(callbacks.cb_ctx, event);
  }
}

static bool config_equal(embenet_config_t const *a, embenet_config_t const *b) {
  return (a->active == b->active) && (0 == memcmp(a->k1, b->k1, EMBENET_KEY_SIZE)) && (0 == memcmp(a->psk, b->psk, EMBENET_KEY_SIZE));
}

/// Records a change. Called with @ref mtx held.
static void config_changed(void) {
  config_pending = true;
  config_changed_ms = k_uptime_get();
  k_sem_give(&config_sem);
}

/// Fills @ref config with the Kconfig defaults, once. Called with @ref mtx held.
static void config_default(void) {
  if(config_defaulted) {
    return;
  }
  size_t const k1 = hex2bin(CONFIG_EMBENET_SETTINGS_DEFAULT_K1, 2U * EMBENET_KEY_SIZE, config.k1, EMBENET_KEY_SIZE);
  size_t const psk = hex2bin(CONFIG_EMBENET_SETTINGS_DEFAULT_PSK, 2U * EMBENET_KEY_SIZE, config.psk, EMBENET_KEY_SIZE);
  __ASSERT(EMBENET_KEY_SIZE == k1 && EMBENET_KEY_SIZE == psk, "the default K1 and PSK must be hex digits");
  ARG_UNUSED(k1);
  ARG_UNUSED(psk);
  config.active = IS_ENABLED(CONFIG_EMBENET_SETTINGS_DEFAULT_ACTIVE);
  config_defaulted = true;
}

// ----- settings handler for EMBENET_SETTINGS_SUBTREE -----

/**
 * Takes a value from the settings: at settings_load() for what is stored, at settings_runtime_set() for a change.
 * Called in the caller's thread.
 *
 * @param name    Key below EMBENET_SETTINGS_SUBTREE.
 * @param len     Size of the value.
 * @param read_cb Reads the value.
 * @param cb_arg  Argument for @p read_cb.
 * @return 0, -ENOENT for an unknown key, -EINVAL for a value of the wrong size or range.
 */
static int settings_set(char const *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
  bool const is_k1 = settings_name_steq(name, "k1", NULL);
  bool const is_psk = settings_name_steq(name, "psk", NULL);
  bool const is_active = settings_name_steq(name, "active", NULL);
  if(!is_k1 && !is_psk && !is_active) {
    return -ENOENT;
  }

  uint8_t value[EMBENET_KEY_SIZE];
  size_t const size = is_active ? 1U : EMBENET_KEY_SIZE;
  if(len != size) {
    LOG_WRN("setting " EMBENET_SETTINGS_SUBTREE "/%s has %zu bytes, expected %zu - ignored", name, len, size);
    return -EINVAL;
  }
  ssize_t const read = read_cb(cb_arg, value, size);
  if(read != (ssize_t)size) {
    return (read < 0) ? (int)read : -EINVAL;
  }
  if(is_active && (value[0] > 1U)) {
    LOG_WRN("setting " EMBENET_SETTINGS_SUBTREE "/active is %u, expected 0 or 1 - ignored", value[0]);
    return -EINVAL;
  }

  k_mutex_lock(&mtx, K_FOREVER);
  config_default();
  bool changed;
  if(is_active) {
    changed = (config.active != (1U == value[0]));
    config.active = (1U == value[0]);
  } else {
    uint8_t *const key = is_k1 ? config.k1 : config.psk;
    changed = (0 != memcmp(key, value, EMBENET_KEY_SIZE));
    memcpy(key, value, EMBENET_KEY_SIZE);
  }
  if(changed) {
    config_changed();
  }
  k_mutex_unlock(&mtx);

  if(changed) {
    // Only the first byte of a key: enough to tell configurations apart in a log, not enough to give them away.
    LOG_INF("setting " EMBENET_SETTINGS_SUBTREE "/%s now %02x%s", name, value[0], is_active ? "" : "..");
  }
  return 0;
}

/**
 * Gives a value to settings_runtime_get().
 *
 * @param name        Key below EMBENET_SETTINGS_SUBTREE.
 * @param val         Buffer for the value.
 * @param val_len_max Size of @p val.
 * @return Size of the value, -ENOENT for an unknown key, -EINVAL if @p val is too small.
 */
static int settings_get(char const *name, char *val, int val_len_max) {
  uint8_t value[EMBENET_KEY_SIZE];
  int size;

  k_mutex_lock(&mtx, K_FOREVER);
  config_default();
  if(settings_name_steq(name, "k1", NULL)) {
    memcpy(value, config.k1, EMBENET_KEY_SIZE);
    size = EMBENET_KEY_SIZE;
  } else if(settings_name_steq(name, "psk", NULL)) {
    memcpy(value, config.psk, EMBENET_KEY_SIZE);
    size = EMBENET_KEY_SIZE;
  } else if(settings_name_steq(name, "active", NULL)) {
    value[0] = config.active ? 1U : 0U;
    size = 1;
  } else {
    size = -ENOENT;
  }
  k_mutex_unlock(&mtx);

  if(size < 0) {
    return size;
  }
  if(val_len_max < size) {
    return -EINVAL;
  }
  memcpy(val, value, (size_t)size);
  return size;
}

SETTINGS_STATIC_HANDLER_DEFINE(embenet_middleware, EMBENET_SETTINGS_SUBTREE, settings_get, settings_set, NULL, NULL);

// ----- stack callbacks, called from EMBENET_NODE_Proc() with mtx held -----

static void on_joined(EMBENET_PANID pan_id, EMBENET_NODE_QuickJoinCredentials const *quick_join_credentials) {
  ARG_UNUSED(quick_join_credentials);
  LOG_INF("joined network 0x%04" PRIx16, pan_id);
  joined = true;
  notify(EMBENET_EVENT_JOINED);
}

static void on_left(void) {
  LOG_WRN("left the network");
  joined = false;
  left = true;
  notify(EMBENET_EVENT_LEFT);
}

static void on_join_attempt(EMBENET_PANID pan_id, uint8_t const *pan_data, size_t pan_data_size) {
  ARG_UNUSED(pan_data);
  LOG_INF("synchronized to network 0x%04" PRIx16 " (%zu bytes of PAN data), joining", pan_id, pan_data_size);
}

static void on_data_on_unregistered_port(uint16_t port) { LOG_DBG("datagram on unregistered port %" PRIu16, port); }

// ----- embeNET thread -----

/// Sleeps until a port interrupt has work for the stack, or @p timeout, then runs it.
static void proc(k_timeout_t timeout) {
  (void)k_sem_take(EMBENET_PORT_COMMON_GetProcSemaphore(), timeout);
  k_mutex_lock(&mtx, K_FOREVER);
  EMBENET_NODE_Proc();
  if(NULL != callbacks.proc_cb) {
    callbacks.proc_cb(callbacks.cb_ctx);
  }
  k_mutex_unlock(&mtx);
}

/**
 * Takes over a settled configuration change.
 *
 * @param[in,out] in_force Configuration the stack runs with; replaced when a settled change differs from it.
 * @return true if @p in_force was replaced.
 */
static bool take_config_change(embenet_config_t *in_force) {
  bool replaced = false;
  k_mutex_lock(&mtx, K_FOREVER);
  if(config_pending && (k_uptime_get() - config_changed_ms >= SETTINGS_APPLY_DELAY_MS)) {
    config_pending = false;
    if(!config_equal(in_force, &config)) {
      *in_force = config;
      replaced = true;
    }
  }
  k_mutex_unlock(&mtx);
  return replaced;
}

/**
 * Initializes the stack and starts joining with @p cfg.
 * @return true if the stack runs.
 */
static bool start_stack(embenet_config_t const *cfg) {
  EMBENET_NODE_EventHandlers const handlers = {
      .onJoined = on_joined,
      .onLeft = on_left,
      .onJoinAttempt = on_join_attempt,
      .onQuickJoinCredentialsObsolete = NULL,
      .onDataOnUnregisteredPort = on_data_on_unregistered_port,
  };

  k_mutex_lock(&mtx, K_FOREVER);

  EMBENET_Result result = EMBENET_NODE_Init(&handlers, EMBENET_NODE_GetConfig_2400_2480Mhz_BLE_PHY());
  if(EMBENET_RESULT_OK != result) {
    k_mutex_unlock(&mtx);
    LOG_ERR("EMBENET_NODE_Init() failed: %d", (int)result);
    return false;
  }
  stack_initialized = true;
  joined = false;
  notify(EMBENET_EVENT_INITIALIZED);

  EMBENET_NODE_JoinConfig join_config;
  memcpy(join_config.k1.val, cfg->k1, EMBENET_KEY_SIZE);
  memcpy(join_config.psk.val, cfg->psk, EMBENET_KEY_SIZE);
  result = EMBENET_NODE_Join(&join_config);
  if(EMBENET_RESULT_OK != result) {
    EMBENET_NODE_Deinit();
    stack_initialized = false;
    notify(EMBENET_EVENT_DEINITIALIZED);
    k_mutex_unlock(&mtx);
    LOG_ERR("EMBENET_NODE_Join() failed: %d", (int)result);
    return false;
  }

  k_mutex_unlock(&mtx);

  // Only the first byte of each key: enough to tell configurations apart in a log, not enough to give them away.
  LOG_INF("started, EUI-64 %016" PRIx64 ", joining with K1 %02x.. PSK %02x..", embenet_get_eui64(), cfg->k1[0], cfg->psk[0]);
  return true;
}

/// Leaves the network gently, if in one, and stops the stack.
static void stop_stack(void) {
  LOG_INF("stopping");

  k_mutex_lock(&mtx, K_FOREVER);
  left = false;
  // Only a joined node has a network to tell; the stack reports leaving only then, so waiting otherwise would only run
  // out the timeout.
  bool const leaving = joined && (EMBENET_RESULT_OK == EMBENET_NODE_Leave());
  k_mutex_unlock(&mtx);

  if(leaving) {
    int64_t const deadline = k_uptime_get() + LEAVE_TIMEOUT_MS;
    // left is written only by on_left(), which runs in proc() on this same thread.
    while(!left && (k_uptime_get() < deadline)) {
      proc(K_MSEC(PROC_POLL_MS));
    }
  }

  k_mutex_lock(&mtx, K_FOREVER);
  if(joined) {
    joined = false;
    notify(EMBENET_EVENT_LEFT);
  }
  EMBENET_NODE_Deinit();
  stack_initialized = false;
  notify(EMBENET_EVENT_DEINITIALIZED);
  k_mutex_unlock(&mtx);

  LOG_INF("stopped");
}

static void embenet_thread_main(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  (void)k_sem_take(&start_sem, K_FOREVER);

  embenet_config_t in_force;
  k_mutex_lock(&mtx, K_FOREVER);
  in_force = config;
  k_mutex_unlock(&mtx);

  for(;;) {
    if(in_force.active && start_stack(&in_force)) {
      while(!take_config_change(&in_force)) {
        proc(K_MSEC(PROC_POLL_MS));
      }
      stop_stack();
      continue;
    }

    // Stopped - by configuration, or because the stack failed to start, in which case it is retried after a pause.
    if(!in_force.active) {
      LOG_INF("inactive, the radio is left to other protocols");
    }
    int64_t const retry_at = k_uptime_get() + START_RETRY_MS;
    bool replaced = false;
    while(!replaced && !(in_force.active && (k_uptime_get() >= retry_at))) {
      (void)k_sem_take(&config_sem, K_MSEC(IDLE_POLL_MS));
      replaced = take_config_change(&in_force);
    }
  }
}

K_THREAD_DEFINE(embenet_thread, CONFIG_EMBENET_MIDDLEWARE_THREAD_STACK_SIZE, embenet_thread_main, NULL, NULL, NULL, THREAD_PRIORITY, K_FP_REGS, 0);

// ----- API -----

int embenet_init(embenet_cb_t cb) {
  k_mutex_lock(&mtx, K_FOREVER);
  if(init_called) {
    k_mutex_unlock(&mtx);
    return -EALREADY;
  }
  init_called = true;
  callbacks = cb;
  config_default();
  k_mutex_unlock(&mtx);

  // Outside the lock: the settings subsystem calls back into settings_set(), which takes it.
  int err = settings_subsys_init();
  if(0 == err) {
    err = settings_load_subtree(EMBENET_SETTINGS_SUBTREE);
  }
  if(0 != err) {
    LOG_ERR("settings unavailable (%d), running with the Kconfig defaults", err);
  }

  k_mutex_lock(&mtx, K_FOREVER);
  // What was loaded is the starting configuration, not a change to apply.
  config_pending = false;
  bool const active = config.active;
  k_mutex_unlock(&mtx);

  LOG_INF("embeNET Node %s, %s", EMBENET_NODE_GetVersionString(), active ? "active" : "inactive");
  k_sem_give(&start_sem);
  return 0;
}

bool embenet_is_joined(void) {
  k_mutex_lock(&mtx, K_FOREVER);
  bool const r = joined;
  k_mutex_unlock(&mtx);
  return r;
}

uint64_t embenet_get_eui64(void) {
  k_mutex_lock(&mtx, K_FOREVER);
  // The stack's UID once it runs; until then the hardware identity the stack takes it from.
  uint64_t const eui = stack_initialized ? EMBENET_NODE_GetUID() : EMBENET_EUI64_Get();
  k_mutex_unlock(&mtx);
  return eui;
}

char const *embenet_get_stack_version_string(void) { return EMBENET_NODE_GetVersionString(); }

size_t embenet_get_groups(uint16_t *groups, size_t max_count) {
  __ASSERT_NO_MSG((NULL != groups) || (0U == max_count));
  size_t count = 0;
  k_mutex_lock(&mtx, K_FOREVER);
  if(stack_initialized) {
    count = MIN(EMBENET_NODE_GetGroupCount(), max_count);
    for(size_t i = 0; i < count; ++i) {
      groups[i] = EMBENET_NODE_GetGroupByIndex(i);
    }
  }
  k_mutex_unlock(&mtx);
  return count;
}

uint16_t embenet_get_neighbors_count(void) {
  unsigned count = 0;
  k_mutex_lock(&mtx, K_FOREVER);
  if(stack_initialized) {
    count = EMBENET_NODE_DIAG_GetNeighborCount();
  }
  k_mutex_unlock(&mtx);
  return (uint16_t)MIN(count, UINT16_MAX);
}

bool embenet_get_neighbor(size_t index, embenet_neighbor_t *neighbor) {
  __ASSERT_NO_MSG(NULL != neighbor);
  bool found = false;
  k_mutex_lock(&mtx, K_FOREVER);
  // The stack numbers the neighbors it tracks 0 .. GetNeighborCount() - 1 and answers EUI 0 outside that range.
  if(stack_initialized && (index < EMBENET_NODE_DIAG_GetNeighborCount())) {
    EMBENET_NODE_DIAG_NeighborInfo const info = EMBENET_NODE_DIAG_GetNeighborInfo((unsigned)index);
    if(0U != info.eui) {
      *neighbor = (embenet_neighbor_t){.eui = info.eui, .rssi = info.rssi, .role = (embenet_neighbor_role_t)info.role};
      found = true;
    }
  }
  k_mutex_unlock(&mtx);
  return found;
}

uint64_t embenet_get_parent_eui64(void) {
  uint64_t eui = 0;
  k_mutex_lock(&mtx, K_FOREVER);
  if(stack_initialized && joined) {
    eui = EMBENET_NODE_DIAG_GetParentEUI64();
  }
  k_mutex_unlock(&mtx);
  return eui;
}

int8_t embenet_get_parent_rssi(void) {
  int8_t rssi = EMBENET_RSSI_UNAVAILABLE;
  k_mutex_lock(&mtx, K_FOREVER);
  // The stack answers INT8_MIN when not joined and INT8_MAX when the RSSI is unknown; one marker covers both.
  if(stack_initialized && joined && (0U != EMBENET_NODE_DIAG_GetParentEUI64())) {
    int8_t const r = EMBENET_NODE_DIAG_GetParentRSSI();
    rssi = (INT8_MIN == r) ? EMBENET_RSSI_UNAVAILABLE : r;
  }
  k_mutex_unlock(&mtx);
  return rssi;
}

uint16_t embenet_get_radio_duty_cycle(void) {
  uint32_t duty_cycle = 0;
  k_mutex_lock(&mtx, K_FOREVER);
  if(stack_initialized) {
    // The three states are exclusive, so their shares add up to the time the radio is on.
    duty_cycle =
        (uint32_t)EMBENET_NODE_DIAG_GetRadioReadyDutyCycle() + EMBENET_NODE_DIAG_GetRadioTxDutyCycle() + EMBENET_NODE_DIAG_GetRadioRxDutyCycle();
  }
  k_mutex_unlock(&mtx);
  return (uint16_t)MIN(duty_cycle, 10000U);
}
