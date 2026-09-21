# embeNET Border Router sample application

This sample application shows how to use the [embeNET Node](https://embe.net/docs/?q=doxyview/embeNET%20Node) component in the root-node role and the [embeNET BorderRouter](https://embe.net/docs/?q=doxyview/embeNET%20BRC) component to form a complete embeNET gateway (the so called "integrated" solution).

Pair it with [`samples/node`](../node) on a separate board to see a device join the network this
sample forms.

## What it does

- Starts separate threads for the root node and the border router, each with its own stack and event loop.
- The root thread initializes the node stack starts the root node operation and services it in a loop.
- The border router thread initializes the border router, hooks up the event handlers, and services it in a loop.
- Every 10 seconds the border router sends a multicast message to the network group, which any joined node can receive.
- Each event handler logs the event and its parameters.

## Compatibility

Restricted to boards the border router library supports and that have enough resources for both
roles at once:

- `nrf52840dk/nrf52840`
- `nrf54l15dk/nrf54l15/cpuapp`

The border router role requires a **C++23 toolchain (GCC 14+)** — see the module
[README](../../README.md#requirements) for the exact NCS/toolchain versions this repository is
built against. The build fails at configure time with a clear message if the toolchain is too
old.

## Building and running

```bash
west build -b nrf52840dk/nrf52840 samples/border_router_integrated
west flash
```

Logs go out over RTT (`CONFIG_USE_SEGGER_RTT=y`) — attach with J-Link RTT Viewer or `west attach`.
On startup you should see the root node come up, the border router come up behind it, and then
join/authentication events as other nodes appear.

## Configuring

All network parameters are demo constants at the top of [`src/main.c`](src/main.c), meant to be
edited directly rather than through Kconfig:

- `DEMO_KEY_BYTE` — the K1/PSK byte shared with joining nodes. Must match the key used by
  `samples/node` (or any other node) for a join to succeed.
- `DEMO_PAN_ID` / `DEMO_PAN_PREFIX` — the network's PAN ID and IPv6 prefix.
- `DEMO_BORDER_ROUTER_UID` — the border router's own node UID.
- `DEMO_ROOT_NODE_WATCHDOG_S` / `DEMO_ROOT_NODE_RESPONSE_MS` — how long the border router waits
  for the root node before declaring it unresponsive and requesting a restart.

The join rule added in `start_border_router()` uses UID `0`, meaning any node with the correct PSK
is accepted; a real deployment should probably add one rule per each node UID instead.

Beyond the application constants, `CONFIG_EMBENET_AIO_QUEUE_FRAME_COUNT` and
`CONFIG_EMBENET_AIO_RX_THREAD_STACK_SIZE` (see the module Kconfig) size the in-memory transport
itself and rarely need changing.
