# Node

This sample demonstrates the embeNET stack running on a node, which is a field device that joins a network started by the border router. A node connects to the network and exchanges messages with other nodes and with the border router.

This sample is aimed at demonstrating the embeNET stack and its core features in a fairly straightforward way. One thing it omits for clarity is the coexistence with BLE. This feature is demonstrated in the [`node_ble`](../node_ble) sample.

## Requirements

The sample supports the following development kits:

| Development kit | Board target |
| --- | --- |
| nRF54L15 DK | `nrf54l15dk/nrf54l15/cpuapp` |
| nRF52840 DK | `nrf52840dk/nrf52840` |
| nRF52833 DK | `nrf52833dk/nrf52833` |
| nRF52 DK | `nrf52dk/nrf52832` |
| nRF5340 DK | `nrf5340dk/nrf5340/cpunet` |

On the nRF5340 DK the node runs on the network core, the one with the radio. The build adds the `empty_app_core` image for the application core, which starts the network core and hands it the LEDs, the buttons and the UART (see [`sysbuild/empty_app_core.overlay`](sysbuild/empty_app_core.overlay)); `west flash` programs both. The network core's log comes out on the first virtual COM port of the kit, at 115200 baud.

To see this sample in action you need at least two development kits, one for the gateway (the [`border_router_integrated`](../border_router_integrated) sample) and one for a node (this sample or [`node_ble`](../node_ble)).

To build the sample you need nRF Connect SDK v3.4.0.

## Overview

After reset the node initializes the embeNET stack and starts the network joining procedure with the demo network key K1 and pre-shared key PSK defined in [`embenet_demo_network.h`](../common/embenet_demo_network.h). These should match the `join rules` present in the border router, which admits the node. Once joined, the node starts normal operation demonstrating the features described below.

### Network wide clock synchronization (network time)

Network time is the clock that the stack keeps synchronized across all nodes of the network, and the application can schedule tasks on it. In this sample the node toggles LED 4 every 500 ms of network time, so all nodes blink in phase, no matter when each of them joined.

### Reporting to the border router

At every whole second of network time the node sends a unicast datagram to the border router on UDP port 0xf0bc (61628). The datagram carries the node's UID and the network time of the report, for example `UID 0000a1b2c3d4e5f6 time 123000`. The gateway logs every report, so you see each node in the network and the network time it reported at. Since the reports are scheduled on network time, all nodes send theirs at the same instant.

### Blinky service endpoint

The blinky service used in this sample is a simple demonstration of the embeNET UDP transport. It works in the following way:

- when a button on the gateway is pressed, a **multicast** message is sent to all nodes, which switch their LEDs accordingly;
- when a button on a node is pressed, a **unicast** message is sent to the gateway, which switches its LEDs accordingly.

On the nRF54L15 DK the LEDs and buttons are numbered from 0, so LED 1 on the other kits is LED 0 there, and so on.

| Element | Function |
| --- | --- |
| LED 1 (LED 0 on nRF54L15 DK) | Switched by the gateway: on or off, as its last message said. |
| LED 2 (LED 1) | Switched by the gateway in the same way, with the second LED of the protocol. |
| LED 3 (LED 2) | Lit while the node is joined to a network. |
| LED 4 (LED 3) | Blinks with a one second period on network time while the node is joined, in step with every other node. |
| Button 1 (Button 0) | Asks the border router to turn its LED 1 on by sending `{1, 1}`. |
| Button 2 (Button 1) | Asks the border router to turn its LED 1 off by sending `{1, 0}`. |
| Button 3 (Button 2) | Asks the border router to turn its LED 2 on by sending `{2, 1}`. |
| Button 4 (Button 3) | Asks the border router to turn its LED 2 off by sending `{2, 0}`. |

Holding a button down counts as a single press. The node also logs its progress on the DK's virtual COM port at 115200 baud, or over RTT.

## What else?

If the node loses the network, it searches again and resumes all of this after the next join.

The credentials are constants in the image. In a product they come from secure storage or a provisioning step; [`node_ble`](../node_ble) shows provisioning at runtime.

## Customization

Some customization options of the sample worth mentioning:

- **Stack life cycle and credentials.** [`src/node.c`](src/node.c) initializes the stack, joins with `join_config`, runs `EMBENET_NODE_Proc()` and passes the join and leave events to the services below.
- **The report.** [`time_report.c`](../common/time_report/time_report.c) schedules `report_task()` on network time and builds the datagram it sends to the border router.
- **The blinky protocol.** [`blinky_service_common.c`](../common/blinky_service/blinky_service_common.c) turns a button press into a message and a received message into an LED change. The node's join, leave and blink behaviour is in [`blinky_service_node.c`](../common/blinky_service/blinky_service_node.c).
- **Ports, group and demo keys** are shared with the gateway in [`embenet_demo_network.h`](../common/embenet_demo_network.h); change them together on the gateway and on every node.

## Building and running

The commands below are run from the root of the workspace described in the [main README](../../README.md#quick-start).

```bash
west build -b <board> -d build/node embenet/samples/node
west flash -d build/node
```

Use the board target of your development kit from the Requirements table, for example `-b nrf52840dk/nrf52840`. With several kits connected, add `--dev-id <serial>` to `west flash`. The gateway is built and flashed as described in the [`border_router_integrated`](../border_router_integrated/README.md#building-and-running) README.

## Testing

1. Flash [`border_router_integrated`](../border_router_integrated) on one development kit and this sample on one or more others. Open the virtual COM port of each board at 115200 baud.
2. Reset the gateway, then the nodes. Within a minute each node joins and logs:

   ```text
   Starting embeNET Node sample
   Using embeNET Node version: ...
   ready: UDP port 61627, group 200
   UID 0000a1b2c3d4e5f6
   joined network 0x0001
   ```

   Its LED 3 lights up and its LED 4 starts to blink. With several nodes, the LED 4 of all of them blink together.
3. Once a second each node logs its report, for example `sent: UID 0000a1b2c3d4e5f6 time 123000`. The gateway logs the same report as `UDP data received on port 61628 from 0000a1b2c3d4e5f6, size 32: "UID 0000a1b2c3d4e5f6 time 123000"`. With several nodes, all of them report the same time values.
4. Press Button 1 on the gateway. LED 1 lights on every node. The gateway logs `button 1: LED1 on multicast to group 200` and each node logs `from 00...: LED1 on`. Button 2 turns LED 1 off again; Buttons 3 and 4 do the same with LED 2.
5. Press Button 1 on a node. The node logs `button 1: LED1 on sent to the border router`, and LED 1 lights on the gateway, which logs `from 00...: LED1 on`. Button 2 turns it off.
