"""The embeNET GATT services: their UUIDs and names, how to show their values,
and how to turn what the user gives into a value to write.

This is the central's side of the contract in ../../src/README.md
(section "Bluetooth LE services"). If a characteristic's layout changes there,
its decoder or encoder changes here. No UI and no Bluetooth in this module, so it
is tested on its own (tests/test_gatt.py).
"""

from __future__ import annotations

import string
from collections.abc import Callable, Iterable
from dataclasses import dataclass

#: The vendor base all embeNET UUIDs share; they differ in the 16-bit field xxxx.
_BASE = "-e237-4bc2-80c7-3ac1568ef5a6"


def embenet_uuid(field: int) -> str:
    """The 128-bit embeNET UUID with 16-bit field `field`, lowercase as bleak gives it."""
    return f"bab8{field:04x}{_BASE}"


#: What a central filters advertisements on to find embeNET nodes.
PROVISIONING_SERVICE = embenet_uuid(0x0100)
INFO_SERVICE = embenet_uuid(0x0200)

SERVICE_NAMES: dict[str, str] = {
    PROVISIONING_SERVICE: "embeNET Provisioning",
    INFO_SERVICE: "embeNET Info",
}

#: The Info service reports this RSSI when it has none to report.
RSSI_UNAVAILABLE = 127

#: Neighbor Info roles, as `embenet_neighbor_t.role` numbers them.
ROLES = {0: "parent", 1: "child", 2: "unrelated"}

#: Bytes of the Neighbor Info characteristic: EUI-64, RSSI, role.
NEIGHBOR_RECORD_SIZE = 10

#: The neighbor table is read one entry at a time: write the order number, 0 ..
#: Neighbors Count - 1, to Neighbor Select, then read that entry from Neighbor Info.
NEIGHBORS_COUNT = embenet_uuid(0x0204)
NEIGHBOR_SELECT = embenet_uuid(0x0205)
NEIGHBOR_INFO = embenet_uuid(0x0206)

#: The connection's interval, peripheral latency and supervision timeout, as the
#: node sees them. A central on BlueZ cannot learn them any other way without root.
CONNECTION_PARAMETERS = embenet_uuid(0x020A)

#: Generic Access and Generic Attribute: every node serves them and they say
#: nothing about the node, so the station does not show them.
HIDDEN_SERVICES = frozenset({"00001800-0000-1000-8000-00805f9b34fb", "00001801-0000-1000-8000-00805f9b34fb"})


class Malformed(ValueError):
    """A value whose length does not match its characteristic's layout."""


def _exact(raw: bytes, size: int) -> bytes:
    if len(raw) != size:
        raise Malformed(f"{len(raw)} B, expected {size} B")
    return raw


def _multiple(raw: bytes, size: int) -> bytes:
    if len(raw) % size:
        raise Malformed(f"{len(raw)} B, not a multiple of {size} B")
    return raw


def format_eui64(raw: bytes) -> str:
    """An EUI-64 the way the node logs it: 16 hex digits, most significant first."""
    return f"{int.from_bytes(_exact(raw, 8), 'little'):016x}"


def format_rssi(value: int) -> str:
    """An RSSI in dBm, or n/a for the value the node reports when it has none."""
    return "n/a" if value == RSSI_UNAVAILABLE else f"{value} dBm"


def _u16(raw: bytes) -> int:
    return int.from_bytes(_exact(raw, 2), "little")


def _text(raw: bytes) -> str:
    return raw.decode("utf-8", errors="replace")


def _active(raw: bytes) -> str:
    value = _exact(raw, 1)[0]
    return {0: "off (0x00)", 1: "on (0x01)"}.get(value, f"0x{value:02x}?")


def _groups(raw: bytes) -> str:
    groups = [int.from_bytes(raw[i : i + 2], "little") for i in range(0, len(_multiple(raw, 2)), 2)]
    return ", ".join(map(str, groups)) if groups else "none"


def neighbors_count(raw: bytes) -> int:
    """Neighbors Count as a number, for walking the table."""
    return _u16(raw)


def neighbor_select(index: int) -> bytes:
    """The value that selects neighbor `index` in Neighbor Select."""
    return index.to_bytes(2, "little")


def _neighbor(raw: bytes) -> str:
    record = _exact(raw, NEIGHBOR_RECORD_SIZE)
    rssi = int.from_bytes(record[8:9], "little", signed=True)
    role = ROLES.get(record[9], f"role {record[9]}")
    return f"{format_eui64(record[:8])}  {format_rssi(rssi):>7}  {role}"


def _parent(raw: bytes) -> str:
    eui = format_eui64(raw)
    return "none" if int(eui, 16) == 0 else eui


def _parent_rssi(raw: bytes) -> str:
    return format_rssi(int.from_bytes(_exact(raw, 1), "little", signed=True))


def _duty_cycle(raw: bytes) -> str:
    return f"{_u16(raw) / 100:.2f} %"


@dataclass(frozen=True)
class ConnectionParameters:
    """The parameters of the connection the node was read over, as the node sees them."""

    interval_us: int
    #: Peripheral latency: connection events the node may skip.
    latency: int
    timeout_ms: int

    def __str__(self) -> str:
        return f"{self.interval_us / 1000:g} ms, latency {self.latency}, timeout {self.timeout_ms} ms"


def connection_parameters(raw: bytes) -> ConnectionParameters:
    """Connection Parameters: uint32 interval [us], uint16 latency, uint16 timeout [10 ms]."""
    _exact(raw, 8)
    return ConnectionParameters(
        interval_us=int.from_bytes(raw[0:4], "little"),
        latency=int.from_bytes(raw[4:6], "little"),
        timeout_ms=int.from_bytes(raw[6:8], "little") * 10,
    )


@dataclass(frozen=True)
class Characteristic:
    name: str
    #: Turns the raw value into text; None for a write-only characteristic.
    decode: Callable[[bytes], str] | None
    #: Turns what the user typed into the value to write; raises ValueError
    #: saying what is wrong. None for a characteristic the station does not write.
    encode: Callable[[str], bytes] | None = None


#: Hex digits in a K1 or PSK as typed: 16 bytes, two digits each.
KEY_DIGITS = 32


def key_problem(text: str) -> str | None:
    """What is wrong with `text` as a K1 or PSK, or None when nothing is.

    A key is typed as exactly 32 hex digits, in either case, with nothing
    between them: the bytes in the order they are written.
    """
    bad = next((c for c in text if c not in string.hexdigits), None)
    if bad is not None:
        return f"{bad!r} is not a hex digit: use 0-9 and a-f"
    if len(text) != KEY_DIGITS:
        return f"{len(text)} of {KEY_DIGITS} hex digits"
    return None


def _key(text: str) -> bytes:
    problem = key_problem(text)
    if problem is not None:
        raise ValueError(problem)
    return bytes.fromhex(text)


def _encode_active(text: str) -> bytes:
    word = text.strip().lower()
    if word in ("1", "on"):
        return b"\x01"
    if word in ("0", "off"):
        return b"\x00"
    raise ValueError("Active is on or off")


def encode(uuid: str, text: str) -> bytes:
    """What the user gave for characteristic `uuid`, as the bytes to write."""
    known = CHARACTERISTICS.get(uuid.lower())
    if known is None or known.encode is None:
        raise ValueError(f"the station does not write {uuid}")
    return known.encode(text)


def is_on(value: str) -> bool:
    """Whether Active, as decoded for the screen, says the stack runs."""
    return value == _active(b"\x01")


#: The Provisioning service's characteristics.
K1 = embenet_uuid(0x0101)
PSK = embenet_uuid(0x0102)
ACTIVE = embenet_uuid(0x0103)


CHARACTERISTICS: dict[str, Characteristic] = {
    K1: Characteristic("K1", None, _key),
    PSK: Characteristic("PSK", None, _key),
    ACTIVE: Characteristic("Active", _active, _encode_active),
    embenet_uuid(0x0201): Characteristic("EUI-64", format_eui64),
    embenet_uuid(0x0202): Characteristic("Stack Version", _text),
    embenet_uuid(0x0203): Characteristic("Groups", _groups),
    NEIGHBORS_COUNT: Characteristic("Neighbors Count", lambda raw: str(_u16(raw))),
    NEIGHBOR_SELECT: Characteristic("Neighbor Select", lambda raw: str(_u16(raw))),
    NEIGHBOR_INFO: Characteristic("Neighbor Info", _neighbor),
    embenet_uuid(0x0207): Characteristic("Parent EUI-64", _parent),
    embenet_uuid(0x0208): Characteristic("Parent RSSI", _parent_rssi),
    embenet_uuid(0x0209): Characteristic("Radio Duty Cycle", _duty_cycle),
    CONNECTION_PARAMETERS: Characteristic("Connection Parameters", lambda raw: str(connection_parameters(raw))),
}


def service_name(uuid: str, fallback: str) -> str:
    """The service's embeNET name, or `fallback` (bleak's description) for any other."""
    return SERVICE_NAMES.get(uuid.lower(), fallback)


def characteristic_name(uuid: str, fallback: str) -> str:
    """The characteristic's embeNET name, or `fallback` for any other."""
    known = CHARACTERISTICS.get(uuid.lower())
    return known.name if known else fallback


def decode(uuid: str, raw: bytes) -> str:
    """`raw`, as read from characteristic `uuid`, as text for the screen.

    A known characteristic is decoded by its layout; anything else is shown in
    hex. A value that does not fit its layout is shown in hex too, marked, so a
    firmware/central mismatch is visible rather than hidden.
    """
    known = CHARACTERISTICS.get(uuid.lower())
    if known is None or known.decode is None:
        return raw.hex() if raw else "(empty)"
    try:
        return known.decode(raw)
    except Malformed as error:
        return f"{raw.hex()}  (malformed: {error})"


def is_embenet(service_uuids: Iterable[str]) -> bool:
    """Whether an advertisement's service UUIDs mark an embeNET node."""
    return PROVISIONING_SERVICE in (uuid.lower() for uuid in service_uuids)
