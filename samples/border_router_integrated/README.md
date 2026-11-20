# Border Router (integrated)

This sample demonstrates the embeNET Border Router together with the root node deployed on the same chip (so called `integrated architecture`), constituting a gateway for the embeNET network to which other nodes can join.

## Requirements

Currently, the sample supports the following development kits:

| Development kit | Board target |
| --- | --- |
| nRF54L15 DK | `nrf54l15dk/nrf54l15/cpuapp` |
| nRF52840 DK | `nrf52840dk/nrf52840` |
| nRF52833 DK | `nrf52833dk/nrf52833` |

To see the network in action you need at least two development kits, one for the gateway (this `border_router_integrated` sample) and one for a node (either `node` or `node_ble` sample). 

To build the sample you need nRF Connect SDK v3.4.0, which provides Zephyr SDK 1.0.1 with GCC 14, required by the border router library.

## Overview

The sample has two parts: the integrated border router itself, and the blinky endpoint that gives the gateway's buttons and LEDs a role in the demo.

### Integrated border router

An embeNET network consists of **nodes** and one of them is a **root node**. The **Border Router** controls the root node and by doing that it routes all packets up and down the network, authenticates and authorizes joining nodes through join rules, and is the gateway to other networks.

This sample demonstrates the **integrated architecture**, where the root node and the border router run on one chip. Such a single integrated border router is thus enough to start and manage the network and connect it to the Internet.

## Join rules

The border router decides which nodes may join the network maintaining a list of so called `join rules`. Each rule is a pair of node UID and PSK (pre-shared key). This sample uses a single rule with UID 0, which admits every node that presents the demo PSK, common for all nodes. The network key K1 is the demo key. Both match the keys used in the node samples.

## Log output

The border router logs its operation as well as the activities of the nodes in the network, including: authentication attempts, node join/leave events, and multicast group membership changes.

The border router also listens on UDP port 0xf0bc (61628) and logs every datagram that arrives there, with the sender's identifier and the payload. Every node sends such a report to the border router every second on network time, with its UID and the network time, so a running node shows up as a steady stream of log lines.

### Blinky service endpoint

The blinky service used in this sample is a simple demonstration of the embeNET UDP transport. It works in the following way:

- when a button on the gateway is pressed, a **multicast** message is sent to all nodes, which switch their LEDs accordingly;
- when a button on a node is pressed, a **unicast** message is sent to the gateway, which switches its LEDs accordingly.

The buttons and LEDs are thus controlled by the blinky service endpoint. On the nRF54L15 DK they are numbered from 0, so LED 1 and Button 1 on the other kits are LED 0 and Button 0 there.

| Element | Function |
| --- | --- |
| Button 1 (Button 0 on nRF54L15 DK) | Turns LED 1 on, on every node. |
| Button 2 (Button 1) | Turns LED 1 off, on every node. |
| Button 3 (Button 2) | Turns LED 2 on, on every node. |
| Button 4 (Button 3) | Turns LED 2 off, on every node. |
| LED 1 (LED 0) | Lit when a node last asked for LED 1 to be on. |
| LED 2 (LED 1) | Lit when a node last asked for LED 2 to be on. |

## Customization

Some customization options of the sample worth mentioning:

- **Network identity, keys and join rules** are set in `start_border_router()` in [`src/border_router.c`](src/border_router.c): the PAN ID, IPv6 prefix and PAN data (`DEMO_PAN_ID`, `DEMO_PAN_PREFIX`, `DEMO_PANDATA`, at most 16 bytes of PAN data) and the join rule. The sample adds a single join rule with UID 0, which matches every node that knows the PSK. This is a shortcut for the demonstration only and usually is not used in production code. Instead, we should have one join rule per node with `EMBENET_BR_AddJoinRule()`, each with the node's own UID and PSK.
- **Reaction to network events**, such as a node joining or leaving, is implemented in the event handlers (in the same file).
- **UDP ports, multicast groups and demo keys** are shared with the node samples in [`samples/common/embenet_demo_network.h`](../common/embenet_demo_network.h); change them together on the gateway and on every node.
- **Log level**: set `CONFIG_LOG_DEFAULT_LEVEL` to 4 in `prj.conf` to log the network's absolute slot number on every slot.

## Building and running

The commands below are run from the root of the workspace described in the [main README](../../README.md#quick-start). Use the board target of your development kit from the Requirements table:

```bash
west build -b <board> -d build/gateway embenet/samples/border_router_integrated
west flash -d build/gateway
```

For example, `-b nrf54l15dk/nrf54l15/cpuapp`. With several kits connected, add `--dev-id <serial>` to `west flash`.

## Testing

1. Build and flash this sample on one development kit, and flash one of the node samples on one or more others.
2. Connect the gateway to your computer and open its virtual COM port in a terminal at 115200 baud. The same log is available over RTT.
3. Reset the gateway. After a moment it logs the start of the root node and of the network:

   ```
   Starting embeNET Border Router sample
   root node started, UID 00...
   root node identified, UID 00...
   network started, root node UID 00...
   running: UDP port 61627, group 200
   listening on UDP port 61628
   ```

4. Power up the node boards. Within a minute each of them is reported:

   ```
   node 00... authenticated
   node 00... joined (1 total)
   node 00... added to group 200
   ```

   A node whose PSK does not match the join rule is reported as `rejected` or `no matching join rule`.
5. A line like this appears once a second for every node:

   ```
   UDP data received on port 61628 from 00..., size 32: "UID 0000a1b2c3d4e5f6 time 123000"
   ```

   The payload carries the node's UID and the network time of the report. All nodes report the same time values.
6. Press Button 1 on the gateway. LED 1 lights on every node and the gateway logs `button 1: LED1 on multicast to group 200`. Press Button 2 to turn it off. Pressing a button on a node switches the gateway's LED and is logged here as `from 00...: LED1 on`.
