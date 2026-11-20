# embeNET GATT services

Reference for writing your own central; [`ble_central`](ble_central/README.md) already does all of this.

The sample starts connectable advertising, which resumes after a central disconnects. One central can be connected at a time.

| Packet | Content |
|---|---|
| Advertisement | Flags and the **Provisioning** service UUID |
| Scan response | Complete local name: `CONFIG_BT_DEVICE_NAME` (`embeNET <chip>`, e.g. `embeNET nRF52840`) |

Centrals find embeNET Nodes by filtering the scan on the Provisioning UUID.

The Node asks for a 200-400 ms connection interval to leave the radio to the mesh; allow a moment for each GATT request.

## UUIDs

All UUIDs share one base and differ in the 16-bit field `xxxx`:

```
BAB8xxxx-E237-4BC2-80C7-3AC1568EF5A6
```

Each service takes a multiple of `0x0100`, its characteristics count up from there. [src/nrfconnect_uuid_definitions.json](src/nrfconnect_uuid_definitions.json) names them all in nRF Connect (Desktop: *Device options, Open UUID definitions file*; Mobile: *Menu, Definitions, Import*).

## Provisioning service, `BAB80100-...`

| Characteristic | `xxxx` | Access | Size [B] | Value |
|---|---|---|---|---|
| **K1** | `0101` | Write | 16 | Network key. Same on every device of the network. |
| **PSK** | `0102` | Write | 16 | Pre-shared key, matched by the border router's join rule. |
| **Active** | `0103` | Read, Write | 1 | `0x00` stack off, `0x01` stack on. |

- Values are stored in settings and survive resets. K1 and PSK written together cause one restart.
- A write must replace the whole value in a single Write Request. Failed requests return standard ATT errors (wrong length, out-of-range neighbor, write to a read-only value).

## Info service, `BAB80200-...`

Read-only except Neighbor Select. Integers are little-endian.

| Characteristic | `xxxx` | Size [B] | Value |
|---|---|---|---|
| **EUI-64** | `0201` | 8 | `uint64_t`, this Node's EUI-64 |
| **Stack Version** | `0202` | variable | UTF-8 string, e.g. `1.1.4+6b937e4` |
| **Groups** | `0203` | 2 x *N* | `uint16_t[N]`, multicast groups of the Node |
| **Neighbors Count** | `0204` | 2 | `uint16_t` |
| **Neighbor Select** | `0205` | 2 | `uint16_t`, read/write: which neighbor Neighbor Info reports, 0 .. count - 1 |
| **Neighbor Info** | `0206` | 10 | The selected neighbor, see below |
| **Parent EUI-64** | `0207` | 8 | `uint64_t`, 0 without a parent |
| **Parent RSSI** | `0208` | 1 | `int8_t` dBm, 127 without a parent |
| **Radio Duty Cycle** | `0209` | 2 | `uint16_t`, 0.01% units: share of time the radio has been on since the stack started |
| **Connection Parameters** | `020A` | 8 | Of the connection the read arrives on: `uint32_t` interval in us, `uint16_t` latency, `uint16_t` supervision timeout in 10 ms units |

Neighbor Info record:

| Offset | Size [B] | Field |
|---|---|---|
| 0 | 8 | `uint64_t eui` |
| 8 | 1 | `int8_t rssi`, dBm; 127 if unknown |
| 9 | 1 | `uint8_t role`: 0 parent, 1 child, 2 unrelated |

**Reading the neighbor table.** Read Neighbors Count, then for each *n* from 0 to count - 1 write *n* to Neighbor Select and read Neighbor Info. Each connection keeps its own selection. An index outside 0 .. count - 1, or a neighbor that has since left the table, gives ATT error `0xFF` (Out of Range): on the write to Neighbor Select, or on the read of Neighbor Info. The table changes as the Node hears its neighbors, so order numbers are not stable over time.

With [bleak](https://github.com/hbldh/bleak):

```python
import struct
from bleak import BleakClient

U = lambda x: f"bab8{x:04x}-e237-4bc2-80c7-3ac1568ef5a6"

async with BleakClient(address) as c:
    eui = struct.unpack("<Q", await c.read_gatt_char(U(0x0201)))[0]
    count = struct.unpack("<H", await c.read_gatt_char(U(0x0204)))[0]
    neighbors = []
    for n in range(count):
        await c.write_gatt_char(U(0x0205), struct.pack("<H", n), response=True)  # Neighbor Select
        neighbors.append(struct.unpack("<QbB", await c.read_gatt_char(U(0x0206))))  # Neighbor Info

    await c.write_gatt_char(U(0x0101), bytes.fromhex("11" + "00" * 15), response=True)  # K1
    await c.write_gatt_char(U(0x0103), b"\x01", response=True)                           # active
```

## Security

For production, require pairing: set `BT_GATT_PERM_WRITE_AUTHEN` on the Provisioning characteristics and enable `CONFIG_BT_SMP`. The services are open, so any central in range can otherwise read the Node's state and reprovision it.
