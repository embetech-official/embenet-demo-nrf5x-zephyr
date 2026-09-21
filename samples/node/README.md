# embeNET Node sample application

This sample application shows how to use the [embeNET Node](https://embe.net/docs/?q=doxyview/embeNET%20Node) component to join a network started by the border router.
It demonstrates the minimal networked node lifecycle from initialization to online operation.
To demonstrate this you'll need another device already
acting as a border router. [`samples/border_router_integrated`](../border_router_integrated)
is a convenient counterpart: flash it on one board and this sample on another, and the node joins
over the radio.

## What it does

- Initializes the node stack and registers handlers for join, leave, join-attempt and
  unregistered-port events.
- Joins the network with a hardcoded demo keys: K1 and PSK.
- Joins a network group with a hardcoded demo group ID = 100
- Once joined, schedules a task that fires once a second and sends some data over UDP port no 1234.
- Runs `EMBENET_NODE_Proc()` in a loop; the node only stays on the network while this keeps
  running.

## Compatibility

Builds for any board the underlying `embenet_node` library ships an architecture for. Devicetree
overlays are provided for:

- `nrf52840dk/nrf52840`
- `nrf52833dk/nrf52833`
- `nrf54l15dk/nrf54l15/cpuapp`

## Building and running

```bash
west build -b nrf52840dk/nrf52840 samples/node
west flash
```

Logs go out over RTT (`CONFIG_USE_SEGGER_RTT=y`), not a UART console — attach with J-Link RTT
Viewer or `west attach` to see them, and follow with the network side (border router or
`border_router_integrated`) to watch the join complete.

## Configuring

- **Join key** — `DEMO_KEY_BYTE` at the top of [`src/main.c`](src/main.c) sets both K1 and PSK (all
  other bytes stay zero). It must match the key configured on the network's border router. This is
  a demo constant; a real device should read its key from settings or a provisioned partition
  instead of the image.
- **Log verbosity** — `CONFIG_LOG_DEFAULT_LEVEL` in [`prj.conf`](prj.conf) gates Zephyr's own log
  output. The embeNET stack logs through embetech's own logger instead; raise the
  `LOGGER_SetRuntimeLevel()` call in `main()` to `LOGGER_LEVEL_DEBUG` or `_TRACE` to see stack
  internals when chasing a join problem.
- **Role selection** — this sample only enables `CONFIG_EMBENET_NODE`. See the module
  [README](../../README.md#configuration) for the full set of Kconfig options, including the
  root-node-to-border-router transport choice this sample does not exercise.
