# Node coexisting with Bluetooth LE (multiprotocol operation)

This sample demonstrates the embeNET stack running on a node, which is a field device that joins a network started by the border router. A node connects to the network and exchanges messages with other nodes and with the border router.

While the [`node`](../node) sample shows the embeNET stack running on a node with no other radio activity, this sample shows how to run it together with a Bluetooth LE peripheral on the same chip.

The node behaves like the [`node`](../node) sample but also advertises as a Bluetooth LE peripheral, which a phone or a PC can find, read its state and provision it with new network keys. The node's radio is shared between the mesh and Bluetooth LE, so the node can stay in the mesh while a central connects to it over Bluetooth LE.

## Requirements

Currently, the sample supports the following development kits:

| Development kit | Board target |
| --- | --- |
| nRF54L15 DK | `nrf54l15dk/nrf54l15/cpuapp` |
| nRF52840 DK | `nrf52840dk/nrf52840` |
| nRF52833 DK | `nrf52833dk/nrf52833` |
| nRF52 DK | `nrf52dk/nrf52832` |
| nRF5340 DK | `nrf5340dk/nrf5340/cpunet` |

To use the Bluetooth LE side you also need one of:

* a smartphone or tablet with [nRF Connect for Mobile](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile);
* a PC with [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop/download)
* a PC with the [`ble_central`](ble_central/README.md) tool of this sample, which lists the nodes, shows their state and provisions them.

## Overview

The sample has two parts.

The first part is the embeNET node stack. It uses the [embeNET middleware](src/README.md), which stores the network keys in flash, starts the stack and reports the node's state. With no provisioning it uses the demo keys and joins the demo network by itself. The sample also demonstrates the same features as the [`node`](../node) sample: network time, the blinky service and the periodic reporting to the border router.

The second part, built when the Bluetooth LE configuration is enabled, is a connectable Bluetooth LE peripheral that advertises as `embeNET <chip-name>`, for example `embeNET nRF54L15`. The advertisement carries the UUID of the embeNET Provisioning service, so a central can pick the nodes out of everything else it hears. Two GATT services are offered. The Provisioning service writes the network key K1, the pre-shared key PSK and the switch that turns the stack on and off. The Info service reads the node's EUI-64, stack version, multicast groups, neighbor table, parent, radio duty cycle and the parameters of the current connection. The services are described in [GATT.md](GATT.md).

Mesh and Bluetooth LE use the same radio, so the node asks the central for a long connection interval and advertises slowly, which leaves most of the radio time to the mesh.

## User interface

The LEDs and buttons work as in the [`node`](../node) sample. The node also logs its progress on the DK's virtual COM port at 115200 baud, or over RTT.

## Configuration

| Option | Default | Meaning |
| --- | --- | --- |
| `CONFIG_EMBENET_BLE_ADV_INTERVAL_MIN_MS`, `CONFIG_EMBENET_BLE_ADV_INTERVAL_MAX_MS` | 500, 600 | Advertising interval. A longer one leaves more radio time to the mesh; a shorter one makes the node easier to find. |
| `CONFIG_EMBENET_SETTINGS_DEFAULT_K1` | `11000000000000000000000000000000` | K1 used until one is stored. |
| `CONFIG_EMBENET_SETTINGS_DEFAULT_PSK` | `33000000000000000000000000000000` | PSK used until one is stored. |
| `CONFIG_EMBENET_SETTINGS_DEFAULT_ACTIVE` | y | Whether the stack runs until a switch value is stored. |
| `CONFIG_BT_DEVICE_NAME` | `embeNET <chip>` | Advertised name. |

The application is [`src/node.c`](src/node.c): `main()` starts Bluetooth LE and the node, `on_embenet_event()` and `on_embenet_proc()` run the blinky service and the time report with the stack, and `report_topology()` produces the 10 second log. Add your own code there, using the callbacks described in the [middleware README](src/README.md). The GATT services are in `src/embenet_ble_*.c`.

K1, the PSK and the active switch are kept in [Zephyr settings](https://docs.zephyrproject.org/latest/services/storage/settings/index.html) and survive resets. Over Bluetooth LE they are written through the Provisioning service.

| Key | Size [B] | Value | Default until stored |
| --- | --- | --- | --- |
| `embenet/k1` | 16 | Network key | `CONFIG_EMBENET_SETTINGS_DEFAULT_K1` |
| `embenet/psk` | 16 | Pre-shared key, matched by the border router's join rule | `CONFIG_EMBENET_SETTINGS_DEFAULT_PSK` |
| `embenet/active` | 1 | `0` stack off, `1` stack on | `CONFIG_EMBENET_SETTINGS_DEFAULT_ACTIVE` |

A change takes effect about 1.5 s after the last one: a joined node leaves the network, then the stack restarts with the new values. To return to the defaults, erase the settings partition with `west flash --erase`. The [middleware README](src/README.md) describes how to read and write the same settings from your own code.

## Building and running

The commands below are run from the root of the workspace described in the [main README](../../README.md#quick-start):

```bash
west build -b <board> -d build/node_ble embenet/samples/node_ble
west flash -d build/node_ble
```

Use the board target of your development kit from the Requirements table, for example `-b nrf54l15dk/nrf54l15/cpuapp`. With several kits connected, add `--dev-id <serial>` to `west flash`.

## Testing

1. Flash [`border_router_integrated`](../border_router_integrated) on one development kit and this sample on another. Open the virtual COM port of each board at 115200 baud.
2. Reset the node. It starts Bluetooth LE advertising and the stack, and within a minute joins the network:

   ```
   Starting embeNET Node sample
   advertising as "embeNET nRF54L15"
   embeNET Node 1.1.x, active
   started, EUI-64 00..., joining with K1 11.. PSK 33..
   joined network 0x0001
   ```

   The gateway logs `node 00... joined`. LED 3 lights up, LED 4 blinks and the buttons work as in the [`node`](../node#testing) sample.
3. Every 10 seconds the node logs its place in the network:

   ```
   topology: uid 00... parent 00... rssi -52 neighbors 2 duty cycle 1.35% | joined
   ```

4. To look at the node from a phone, open nRF Connect for Mobile, scan, and connect to `embeNET nRF54L15`. The node logs `central connected, interval ... us`. To see the characteristics by name, import [`src/nrfconnect_uuid_definitions.json`](src/nrfconnect_uuid_definitions.json): Menu, Definitions, Import. The Info service then shows the EUI-64, the neighbors and the parent.
5. To provision the node, write 16 bytes to the K1 characteristic of the Provisioning service, for example `22000000000000000000000000000000`. The node logs `K1 written` and, about 1.5 seconds later, `stopping` and `started, EUI-64 ..., joining with K1 22.. PSK 33..`. The node now joins a network that uses this key; write `11000000000000000000000000000000` to return to the demo network.
6. Write `00` to the Active characteristic to stop the stack, leaving the radio to Bluetooth LE, and `01` to start it again. The node logs `active switch written: 0`, then `stopping` and `inactive, the radio is left to other protocols`; after `01` it logs `active switch written: 1` and `started, ...`.
7. To do the same from a Linux PC, run [`ble_central`](ble_central/README.md): it lists every embeNET node in range, shows the Info service and writes the Provisioning service.

For writing your own central, see [GATT.md](GATT.md).
