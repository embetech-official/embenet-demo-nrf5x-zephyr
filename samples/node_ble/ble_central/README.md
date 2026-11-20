# embeNET BLE central

A terminal UI for a Linux PC with BlueZ and Bluetooth LE. It needs a Node running [`samples/node_ble`](../README.md); the services it reads are in [GATT.md](../GATT.md). Built with [Textual](https://textual.textualize.io/) and [bleak](https://github.com/hbldh/bleak).

## Install and run

Requirements: Linux with BlueZ and the Bluetooth adapter powered on (`bluetoothctl power on`), and Python 3.11 or newer. From this directory:

```bash
python3 -m venv .venv
.venv/bin/pip install .
.venv/bin/python ble_central.py
```

| Option | Meaning |
|---|---|
| `--adapter hci1` | Bluetooth adapter to use; the system default if omitted. |
| `--stale 10` | Seconds without an advertisement before a Node is dimmed; 10 if omitted. |
| `--forget 30` | Seconds without an advertisement before a Node that is not connected is removed from the list; 30 if omitted. It comes back with its next advertisement. |

## User interface

- **Find and connect.** Nodes appear as they advertise. Connect with the button in a Node's box, or Enter on the highlighted one; connecting can take a while on a long connection interval.
- **Read.** A connected Node shows its connection parameters and a panel per service: EUI-64, stack version, groups, neighbor table, parent, radio duty cycle. Refresh in a panel, or `r`, reads again.
- **Provision.** The default keys already match the demo border router. Write K1 and PSK (32 hex digits each) only to move a Node to another network; the Active switch turns its stack off and on.

`q` disconnects every Node and quits.
