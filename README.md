# embeNET for Zephyr on nRF devices

[embeNET](https://embe.net) is a modern take on the wireless mesh network, designed for industrial and professional IoT applications. It is a self-organizing, secure, time-synchronized mesh networking protocol with full IPv6 support suitable especially for constrained low-power devices and large-scale networks.

## embeNET features in a nutshell

- IPv6 addressing, UDP sockets, unicast and multicast groups
- TSCH (time slotted channel hopping, IEEE 802.15.4): fewer collisions and less idle listening even in dense networks, scaling from dozens to over a thousand nodes, with channel hopping against interference and multipath fading
- Self-organizing, self-healing mesh: devices look for better links, keep backup connections and restore them on failure; multiple hops extend the coverage
- Network time with scheduled tasks
- AES-encrypted data links, access controlled by flexible join rules
- Configurable physical layer: 2.4 GHz or sub-GHz bands (depending on the chip capabilities)
- Built in remote network diagnostics and large-scale firmware updates
- **Radio sharing with Bluetooth LE on the same chip**

## Supported platforms

embeNET is lightweight and hardware agnostic. It runs on a variety of microcontrollers and radios, including the Nordic Semiconductor **nRF52x**, **nRF53x** and **nRF54x** families. This repository delivers it as a Zephyr module with sample applications that you can build, flash and extend.

## Network architecture

An embeNET network consists of **nodes** which are basically field devices. One of the nodes, responsible for starting and managing the network is called **root node**. An entity called **Border Router** controls that root node and by doing that it routes all packets up and down the network, authenticates and authorizes joining nodes through join rules, and is the gateway to other networks.

Usually the root node and the border router are deployed on separate CPUs connected by a serial interface such as UART. We call this **split architecture**.

This repository however demonstrates the **integrated architecture**, where the root node and the border router run on one chip. Such a single integrated border router is thus enough to start and manage the network and connect it to the Internet.

## Addressing

Every node in the embeNET network has a unique IPv6 address. Applications running in the nodes exchange data over UDP sockets using the following addressing schemes:

- unicast: a single datagram addressed to a single node
- multicast: a single datagram addressed to a group of nodes

## Time Synchronization

embeNET uses TSCH (time slotted channel hopping, IEEE 802.15.4) to avoid collisions and idle listening, which in turn allows us to support large-scale networks with thousands of nodes. The network is synchronized up to microsecond-level accuracy across all nodes, and the network clock is available to applications. The user application can schedule tasks on it, so that many field devices may act at the same instant.

## Security

Security is built in. All data links are AES encrypted, with session keys negotiated between the border router and each device. A node can join the network only if a certain join rule is present in the border router. Such join rules can define a global pre-shared key (PSK) for the whole network, part of the network or a unique PSK per each device. The latter being of course the most secure option.

## Multiprotocol operation

embeNET can share the radio with Bluetooth LE on the same chip. A node can stay in the mesh while a phone or a PC connects to it over Bluetooth LE for provisioning, commissioning and inspection. This way you need no second radio and no separate commissioning hardware. We call that **multiprotocol operation** and this feature is also demonstrated in this repository.

## Development kits supported by samples

Currently, the samples are prepared for these development kits:

| Chip | Development kit |  Supported sample |
| --- | --- | --- |
| nRF54L15 | nRF54L15 DK | [`border_router_integrated`](samples/border_router_integrated) [`node`](samples/node) [`node_ble`](samples/node_ble) |
| nRF52840 | nRF52840 DK | [`border_router_integrated`](samples/border_router_integrated) [`node`](samples/node) [`node_ble`](samples/node_ble) |
| nRF52833 | nRF52833 DK | [`border_router_integrated`](samples/border_router_integrated) [`node`](samples/node) [`node_ble`](samples/node_ble) |
| nRF52832 | nRF52 DK | [`node`](samples/node) [`node_ble`](samples/node_ble) |
| nRF5340 | nRF5340 DK | [`node`](samples/node) [`node_ble`](samples/node_ble) |

You can mix and match the boards in a single network. 

## Samples

To see the samples in action, one board must run the integrated border router firmware, and one or more boards must run the node samples (either `node` or `node_ble`).

| Sample | Role | What it shows |
| --- | --- | --- |
| [`border_router_integrated`](samples/border_router_integrated) | Gateway | The integrated border router: the root node and the border router deployed on one chip. Needed by every other sample. |
| [`node`](samples/node) | Node | Joining a network, blinking an LED on network time, reporting to the border router every second and linking buttons and LEDs with the gateway. A starting point for your application. |
| [`node_ble`](samples/node_ble) | Node | Same as `node`, but with the additional capability of sharing the radio with Bluetooth LE. GATT service is used to report the node's state and provision its keys, with the credentials kept in settings. |

### What do the samples do?

- All nodes blink an LED using the network time, demonstrating that the network is synchronized
- All nodes send a datagram to the border router every second on network time. The datagram contains the node's unique ID and the current network time. The border router logs the received datagrams, so you can see that the node is in the network and that its clock is synchronized with the network clock.
- Pressing a button on the Border Router sends out a command to all nodes to switch their LEDs, demonstrating multicast addressing.
- Pressing a button on a node sends a command to the Border Router to switch its LED, demonstrating unicast addressing.
- The `node_ble` sample also advertises a Bluetooth LE GATT service that reports the node's state and allows provisioning of its keys. A phone or a PC can connect to it over Bluetooth LE, so you need no second radio and no separate commissioning hardware.

## Why two samples for the node?

The `node` sample is the shortest complete node, without Bluetooth LE. It is a good starting point to get to know embeNET without the complexity of Bluetooth LE.

The `node_ble` sample is a complete node with Bluetooth LE, demonstrating the multiprotocol operation. It is thus more complex, but it is the one to use if you want to provision and inspect the node over Bluetooth LE.

## Quick start

You need nRF Connect SDK v3.4.0 and two development kits, for example an nRF54L15 DK for the gateway and an nRF52840 DK for the node. Create a workspace with this module and the matching SDK:

```bash
west init -m https://github.com/embetech-official/embenet-demo-nrf5x-zephyr --mr main embenet-workspace
cd embenet-workspace && west update
```

From the workspace root, build the gateway and the node, each for the board of its development kit from the table above:

```bash
west build -b <gateway board> -d build/gateway embenet/samples/border_router_integrated
west build -b <node board> -d build/node embenet/samples/node
```

For example, `-b nrf54l15dk/nrf54l15/cpuapp` for the gateway and `-b nrf52840dk/nrf52840` for the node.

With both kits connected, flash each by the serial number of its J-Link, which `nrfutil device list` shows:

```bash
west flash -d build/gateway --dev-id <gateway kit serial>
west flash -d build/node --dev-id <node kit serial>
```

Each kit appears as its own serial port, for example `/dev/ttyACM0` and `/dev/ttyACM1`. Open one terminal per board at 115200 baud; the log is also available over RTT.

1. Reset the gateway. It logs `network started, root node UID ...`.
2. Reset the node. Within a minute the gateway logs `node 00... joined (1 total)`.
3. On the node, LED 3 (LED 2 on the nRF54L15 DK) lights and LED 4 (LED 3) blinks in step with the other nodes. Press a button on the gateway to switch the nodes' LEDs; press a button on a node to switch the gateway's.

## Using embeNET in your application

Add this repository as a west project in your manifest, or pass it to CMake as an extra module:

```bash
west build -b nrf52840dk/nrf52840 my_app -- -DZEPHYR_EXTRA_MODULES=/path/to/this/repo
```

Then enable the role in `prj.conf`. For a node:

```conf
CONFIG_EMBENET=y
CONFIG_EMBENET_NODE=y
```

For a gateway, with the border router and root node in one image:

```conf
CONFIG_EMBENET=y
CONFIG_EMBENET_NODE=y
CONFIG_EMBENET_BORDER_ROUTER=y
CONFIG_GLIBCXX_LIBCPP=y
```

The application includes `<embenet/node.h>` and `<embenet/node_udp.h>` for a node, `<embenet/border_router.h>` for a border router. [`samples/node`](samples/node) is the shortest complete node, [`samples/border_router_integrated`](samples/border_router_integrated) the gateway to start from.

## Sharing the radio with Bluetooth LE (multiprotocol operation)

A node and a Bluetooth LE peripheral can share one radio on the same chip. The device stays in the mesh while a phone or a PC connects to it over Bluetooth LE for provisioning, commissioning and inspection, so you need no second radio and no separate commissioning hardware. Add to `prj.conf`:

```conf
CONFIG_MPSL=y
CONFIG_MPSL_TIMESLOT_SESSION_COUNT=1
CONFIG_EMBENET_NODE_MULTIPROTOCOL=y
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
```

[`node_ble`](samples/node_ble) is a complete example, with GATT services that provision the node and report its state.

## Evaluation and Professional editions

The libraries in this repository are the Evaluation edition of embeNET. They are provided solely for the purpose of evaluation and have the following limitations:

- Maximum number of nodes in the network is limited to 10.
- Maximum number of hops in the network is limited to 2.

These limitations are removed in the Professional edition, which also include additional features and support. To learn more visit [embe.net](https://embe.net).

## Documentation

More documentation and resources are available here: [embe.net/documentation-and-resources](https://embe.net/documentation-and-resources/)
