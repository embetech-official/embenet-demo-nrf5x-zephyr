# embeNET middleware

A thread-safe layer between your application ([node.c](node.c)) and the embeNET Node stack, with a Bluetooth LE peripheral that provisions the Node and reports its state. It is sample code: copy it into your application and adapt it.

| Part | Files | Header |
|---|---|---|
| Stack management | [embenet.c](embenet.c) | [embenet.h](embenet.h) |
| Bluetooth LE services, see [GATT.md](../GATT.md) | [embenet_ble.c](embenet_ble.c), [embenet_ble_provisioning.c](embenet_ble_provisioning.c), [embenet_ble_info.c](embenet_ble_info.c) | [embenet_ble.h](embenet_ble.h) |

## The API

```c
#include "embenet.h"

static void on_event(void *ctx, embenet_event_t event) {
  switch(event) {
  case EMBENET_EVENT_INITIALIZED:    // register sockets, join groups, create tasks
  case EMBENET_EVENT_JOINED:         // schedule network-time tasks
  case EMBENET_EVENT_LEFT:
  case EMBENET_EVENT_DEINITIALIZED:  // sockets, groups and tasks are gone with the stack
    break;
  }
}

static void on_proc(void *ctx) {
  // after every EMBENET_NODE_Proc(): poll the application's own work
}

embenet_init((embenet_cb_t){.event_cb = on_event, .proc_cb = on_proc, .cb_ctx = NULL});
```

`embenet_init()` loads K1, the PSK and the active switch from settings and starts a thread that joins the network and keeps it running.

| Group | Functions |
|---|---|
| Life cycle | `embenet_init` |
| State | `embenet_is_joined`, `embenet_get_eui64`, `embenet_get_stack_version_string`, `embenet_get_groups`, `embenet_get_neighbors_count`, `embenet_get_neighbor`, `embenet_get_parent_eui64`, `embenet_get_parent_rssi`, `embenet_get_radio_duty_cycle` |

- The functions may be called from any thread. While the stack is stopped, the getters return neutral values (0, an empty list, `EMBENET_RSSI_UNAVAILABLE`).
- The event callback, the proc callback and the stack's own callbacks (UDP handlers, tasks) run on the middleware's thread. The raw `EMBENET_NODE_*` API may be called inside them. Keep them short.
- To change K1, PSK or the active switch from code, store the value with `settings_save_one()`, then apply it with `settings_runtime_set()`; `settings_runtime_get()` reads it back. A wrong size, or an active value other than 0 or 1, returns `-EINVAL`. Keys and defaults: [sample README](../README.md#configuration).
- `CONFIG_EMBENET_MIDDLEWARE_THREAD_STACK_SIZE` sets the stack of the middleware's thread.
