/**
 * @file
 * @brief embeNET root node - the node half of the gateway, on its own thread.
 *
 * Everything the node stack touches happens here: EMBENET_NODE_Proc(), the event handlers it calls
 * from inside it, and the restart the border router asks for.
 */

#include <embenet/node.h>
#include <embenet_port_common/embenet_port_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <inttypes.h>

LOG_MODULE_REGISTER(root, LOG_LEVEL_INF);

/// @brief Above the border router thread: a late Proc() here costs radio slots, there only latency.
#define ROOT_THREAD_PRIORITY 5
#define ROOT_THREAD_STACK_SIZE 4096

/// @brief How long the thread sleeps when the radio is quiet. The port ISRs give the semaphore sooner.
#define ROOT_PROC_INTERVAL K_MSEC(1)

/// @brief Started after main() has cleared the shared queues and before the border router opens the link. See BR_THREAD_START_DELAY_MS.
#define ROOT_THREAD_START_DELAY_MS 100

// In root mode only onLeft and onDataOnUnregisteredPort are expected to fire. The rest are wired all
// the same: a root node reporting a join means the stack is not in the state this sample assumes.

/**
 * @brief Called when the root node joins someone else's network as a regular node.
 * @param[in] pan_id The PAN ID of the network that was joined.
 * @param[in] quick_join_credentials The credentials that can be used to fast rejoin.
 */
static void on_root_joined(EMBENET_PANID pan_id, EMBENET_NODE_QuickJoinCredentials const *quick_join_credentials) {
  LOG_WRN("root node joined network 0x%04" PRIx16 " as a regular node (credentials %s)", pan_id,
          (NULL != quick_join_credentials) ? "provided" : "absent");
}

/// @brief Called when the network stops. A root node never leaves on its own, so this reports what the border router drove.
static void on_root_left(void) { LOG_WRN("network stopped"); }

/**
 * @brief Called when the root node attempts to join a network (synchronized but not yet connected).
 * @param[in] pan_id The PAN ID of the network that is being joined.
 * @param[in] pan_data The PAN data that was received from the network.
 * @param[in] pan_data_size The size of the PAN data.
 */
static void on_root_join_attempt(EMBENET_PANID pan_id, uint8_t const *pan_data, size_t pan_data_size) {
  bool const pan_data_is_c_string = pan_data != NULL && pan_data_size > 0 && pan_data[pan_data_size - 1] == '\0';

  LOG_WRN("root node attempting to join network 0x%04" PRIx16 " (%zu bytes of PAN data, %s C-string: %s)", pan_id, pan_data_size,
          pan_data_is_c_string ? "is" : "is not", pan_data_is_c_string ? (char const *)pan_data : "(not a C-string)");
}

/// @brief Called when the stored quick join credentials expire. This sample stores none, so there is nothing to discard.
static void on_root_quick_join_credentials_obsolete(void) { LOG_WRN("quick join credentials are obsolete"); }

/**
 * @brief Called when the node receives a datagram on an unregistered port.
 * @param[in] port The port number on which the datagram was received.
 */
static void on_root_data_on_unregistered_port(uint16_t port) { LOG_WRN("datagram on unregistered port %" PRIu16, port); }

/**
 * @brief Initializes the node and starts it in root mode. Also used for the restart the border router asks for.
 * @return true if the node is running, false if a stack call failed.
 */
static bool start_root_node(void) {
  EMBENET_NODE_EventHandlers const handlers = {
      .onJoined = on_root_joined,
      .onLeft = on_root_left,
      .onJoinAttempt = on_root_join_attempt,
      .onQuickJoinCredentialsObsolete = on_root_quick_join_credentials_obsolete,
      .onDataOnUnregisteredPort = on_root_data_on_unregistered_port,
  };

  EMBENET_Result result = EMBENET_NODE_Init(&handlers);
  if(result != EMBENET_RESULT_OK) {
    LOG_ERR("EMBENET_NODE_Init() failed: %d", (int)result);
    return false;
  }

  result = EMBENET_NODE_RootStart();
  if(result != EMBENET_RESULT_OK) {
    LOG_ERR("EMBENET_NODE_RootStart() failed: %d", (int)result);
    return false;
  }
  LOG_INF("root node started, UID %016" PRIx64, EMBENET_NODE_GetUID());
  return true;
}

/**
 * @brief Thread entry: starts the root node, then services the stack until the board is reset.
 * @param[in] p1,p2,p3 Unused.
 */
static void root_thread_entry(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  k_thread_name_set(k_current_get(), "embenet_root");

  if(!start_root_node()) {
    LOG_ERR("root node failed to start, giving up");
    return;
  }

  for(;;) {
    k_sem_take(EMBENET_PORT_COMMON_GetProcSemaphore(), ROOT_PROC_INTERVAL);
    EMBENET_NODE_Proc();

    // Handled here rather than by the border router, because restarting the node re-runs
    // EMBENET_BRT_Init(), which clears both shared queues - and this is the thread that owns them.
    if(EMBENET_PORT_COMMON_ResetRequestPop()) {
      LOG_WRN("root node restart requested by the border router");
      EMBENET_NODE_Deinit();
      if(!start_root_node()) {
        LOG_ERR("root node failed to restart, giving up");
        return;
      }
    }
  }
}

K_THREAD_DEFINE(root_thread_id, ROOT_THREAD_STACK_SIZE, root_thread_entry, NULL, NULL, NULL, ROOT_THREAD_PRIORITY, 0, ROOT_THREAD_START_DELAY_MS);
