/**
 * @file
 * @license   Apache-2.0
 * @copyright Embetech sp. z o.o.
 * @version   2.0.0
 * @brief     embeNET middleware API - runs the Node stack and gives the application thread-safe access to it.
 *
 * The middleware owns the stack. It initializes it, joins the network with the configured credentials, keeps calling
 * EMBENET_NODE_Proc() from a thread of its own, and restarts the stack when the credentials or the active switch
 * change. The application talks to it through this header only.
 *
 * @par Configuration in Zephyr settings
 * K1, the PSK and the active switch live in Zephyr settings, under @ref EMBENET_SETTINGS_SUBTREE, and so in the
 * device's non-volatile memory. The middleware reads them through a settings handler: embenet_init() loads them, and
 * the handler sees every later change made through settings_runtime_set() or settings_load(). To change one, save it
 * with settings_save_one() and hand it over with settings_runtime_set(), as the Bluetooth Provisioning service does.
 * Keys never stored take their Kconfig defaults (CONFIG_EMBENET_SETTINGS_DEFAULT_*).
 *
 * @note All functions are thread safe. Every call into the stack, from the embeNET thread or from these functions,
 *       happens under one module mutex.
 *
 * @par Callbacks and the raw stack API
 * @ref embenet_event_cb_t runs on the embeNET thread with the module mutex held, and so do the stack's own callbacks
 * (UDP receive handlers, tasks created with EMBENET_NODE_TaskCreate()). Inside them the raw EMBENET_NODE_* API may
 * be called directly - that is where an application registers its sockets, joins groups and schedules its tasks.
 * Outside them, use only this header. Callbacks must not block: a thread waiting on them may hold the Bluetooth stack
 * or the application waiting for this module's mutex.
 */
#ifndef EMBENET_H_
#define EMBENET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Size of K1 and of the PSK, in bytes.
#define EMBENET_KEY_SIZE 16U

/// Parent RSSI and neighbor RSSI when no value is available.
#define EMBENET_RSSI_UNAVAILABLE INT8_MAX

/// @name Zephyr settings the middleware takes its configuration from
/// @{
/// Subtree of all embeNET settings.
#define EMBENET_SETTINGS_SUBTREE "embenet"
/// Network key, @ref EMBENET_KEY_SIZE bytes: the same on every device of the network.
#define EMBENET_SETTINGS_KEY_K1 EMBENET_SETTINGS_SUBTREE "/k1"
/// Pre-shared key, @ref EMBENET_KEY_SIZE bytes: what the border router's join rule admits this node with.
#define EMBENET_SETTINGS_KEY_PSK EMBENET_SETTINGS_SUBTREE "/psk"
/// Active switch, 1 byte: 1 the stack runs and joins the network; 0 the stack is stopped and the radio left to others.
#define EMBENET_SETTINGS_KEY_ACTIVE EMBENET_SETTINGS_SUBTREE "/active"
/// @}

/// Role of a neighbor relative to this node.
typedef enum {
  EMBENET_NEIGHBOR_ROLE_PARENT = 0,    ///< This node's parent.
  EMBENET_NEIGHBOR_ROLE_CHILD = 1,     ///< A child of this node.
  EMBENET_NEIGHBOR_ROLE_UNRELATED = 2, ///< Heard, but not routed through.
} embenet_neighbor_role_t;

/// One neighbor of this node.
typedef struct {
  uint64_t eui;                 ///< Neighbor's EUI-64.
  int8_t rssi;                  ///< RSSI of the last frame from it, in dBm; @ref EMBENET_RSSI_UNAVAILABLE if not known.
  embenet_neighbor_role_t role; ///< @ref embenet_neighbor_role_t
} embenet_neighbor_t;

/// embeNET events, see @ref embenet_event_cb_t.
typedef enum {
  EMBENET_EVENT_INITIALIZED,   ///< Stack initialized, about to join: register sockets, join groups, create tasks now.
  EMBENET_EVENT_JOINED,        ///< Joined the network.
  EMBENET_EVENT_LEFT,          ///< Left the network.
  EMBENET_EVENT_DEINITIALIZED, ///< Stack stopped; sockets, groups and tasks are gone with it.
} embenet_event_t;

/**
 * embeNET event callback.
 *
 * @param ctx   @ref embenet_cb_t::cb_ctx
 * @param event @ref embenet_event_t
 *
 * @note Called from the embeNET thread with the module mutex held - see the file description. Must not block.
 */
typedef void (*embenet_event_cb_t)(void *ctx, embenet_event_t event);

/**
 * Called after every EMBENET_NODE_Proc(), so the application can poll its own work on the stack's thread: at least
 * every few milliseconds while the stack runs, never while it is stopped.
 *
 * @param ctx @ref embenet_cb_t::cb_ctx
 *
 * @note Called from the embeNET thread with the module mutex held, like @ref embenet_event_cb_t. Must not block.
 */
typedef void (*embenet_proc_cb_t)(void *ctx);

/// Callbacks installed by @ref embenet_init.
typedef struct {
  embenet_event_cb_t event_cb; ///< @ref embenet_event_cb_t, may be NULL
  embenet_proc_cb_t proc_cb;   ///< @ref embenet_proc_cb_t, may be NULL
  void *cb_ctx;                ///< Context passed to every callback.
} embenet_cb_t;

/**
 * Installs the callbacks, loads the configuration from Zephyr settings (initializing the settings subsystem if needed)
 * and starts the embeNET thread, which starts the stack if the configuration says it is active.
 *
 * Every later change of an @ref EMBENET_SETTINGS_SUBTREE key that reaches the settings handler takes effect, restarting
 * the stack, 1.5 s after the last change (@c SETTINGS_APPLY_DELAY_MS in embenet.c). Switching off leaves the network
 * gently, which takes a few seconds, then stops the stack and frees the radio.
 *
 * @param cb Callbacks.
 *
 * @retval 0         started; if the settings could not be loaded, with the Kconfig defaults
 * @retval -EALREADY called before
 */
int embenet_init(embenet_cb_t cb);

/**
 * @return true while joined to a network.
 */
bool embenet_is_joined(void);

/**
 * @return This node's EUI-64. Available whether or not the stack runs.
 */
uint64_t embenet_get_eui64(void);

/**
 * @return The stack's version, a NUL-terminated UTF-8 string with static storage.
 */
char const *embenet_get_stack_version_string(void);

/**
 * Copies the multicast groups this node belongs to.
 *
 * @param[out] groups    Buffer for @p max_count group ids.
 * @param      max_count Capacity of @p groups.
 * @return Number of groups copied; 0 when the stack is stopped.
 */
size_t embenet_get_groups(uint16_t *groups, size_t max_count);

/**
 * @return Number of neighbors the node tracks; 0 when the stack is stopped.
 */
uint16_t embenet_get_neighbors_count(void);

/**
 * Copies one entry of the neighbor table.
 *
 * @param      index    Order number of the neighbor, 0 .. @ref embenet_get_neighbors_count() - 1. The table changes as
 *                      the node hears its neighbors, so an index valid a moment ago may not be any more.
 * @param[out] neighbor Not NULL; written only on success.
 * @return true if @p index named a neighbor; false if it is out of range or the stack is stopped.
 */
bool embenet_get_neighbor(size_t index, embenet_neighbor_t *neighbor);

/**
 * @return The parent's EUI-64; 0 when the node has no parent (not joined, or the root).
 */
uint64_t embenet_get_parent_eui64(void);

/**
 * @return RSSI of the last frame from the parent, in dBm; @ref EMBENET_RSSI_UNAVAILABLE when there is no parent.
 */
int8_t embenet_get_parent_rssi(void);

/**
 * @return Share of time the radio is on (ready, receiving or transmitting) since the stack started, in 0.01% units
 *         (0..10000); 0 when the stack is stopped.
 */
uint16_t embenet_get_radio_duty_cycle(void);

#ifdef __cplusplus
}
#endif

#endif // EMBENET_H_
