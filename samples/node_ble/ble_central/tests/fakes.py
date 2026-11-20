"""Stand-ins for bleak: a bench of fake embeNET nodes, the scanner that hears them, and the clients that connect to them.

They implement what the station uses of bleak, and nothing more. The values are
the ones read from real nodes on the bench.
"""

from __future__ import annotations

import asyncio
from collections.abc import Callable
from dataclasses import dataclass, field
from types import SimpleNamespace
from typing import Any

from central import gatt
from central.gatt import embenet_uuid as U

GAP = "00001800-0000-1000-8000-00805f9b34fb"
DEVICE_NAME = "00002a00-0000-1000-8000-00805f9b34fb"
BATTERY = "0000180f-0000-1000-8000-00805f9b34fb"
BATTERY_LEVEL = "00002a19-0000-1000-8000-00805f9b34fb"


def characteristic(uuid: str, handle: int, *properties: str, description: str = "Unknown") -> SimpleNamespace:
    return SimpleNamespace(uuid=uuid, handle=handle, properties=list(properties), description=description)


def service(uuid: str, handle: int, *characteristics: SimpleNamespace, description: str = "Unknown") -> SimpleNamespace:
    return SimpleNamespace(uuid=uuid, handle=handle, description=description, characteristics=list(characteristics))


def embenet_services() -> list[SimpleNamespace]:
    """What the node sample serves, in an order bleak might list it (not handle order)."""
    return [
        service(GAP, 1, characteristic(DEVICE_NAME, 2, "read", description="Device Name"), description="Generic Access Profile"),
        service(
            gatt.PROVISIONING_SERVICE,
            30,
            characteristic(U(0x0101), 31, "write"),
            characteristic(U(0x0102), 33, "write"),
            characteristic(U(0x0103), 35, "read", "write"),
        ),
        service(
            gatt.INFO_SERVICE,
            10,
            characteristic(U(0x0207), 23, "read"),
            characteristic(U(0x0201), 11, "read"),
            characteristic(U(0x0202), 13, "read"),
            characteristic(U(0x0203), 15, "read"),
            characteristic(U(0x0204), 17, "read"),
            characteristic(U(0x0205), 19, "read", "write"),
            characteristic(U(0x0206), 21, "read"),
            characteristic(U(0x0208), 25, "read"),
            characteristic(U(0x0209), 27, "read"),
            characteristic(gatt.CONNECTION_PARAMETERS, 29, "read"),
        ),
    ]


def embenet_values() -> dict[str, bytes]:
    return {
        DEVICE_NAME: b"embeNET nRF54",
        U(0x0103): b"\x01",
        U(0x0201): bytes.fromhex("765845fe2b9e67f4"),
        U(0x0202): b"1.1.4+6b937e4",
        U(0x0203): bytes.fromhex("6400"),
        U(0x0207): bytes.fromhex("74f4ddd3067b2ebb"),
        U(0x0208): bytes([0xC9]),
        U(0x0209): bytes.fromhex("c601"),
        gatt.CONNECTION_PARAMETERS: bytes.fromhex("c8af000000002a00"),  # 45 ms, latency 0, 420 ms
    }


#: Neighbor Info records: the parent, and an unrelated neighbor.
NEIGHBORS = [bytes.fromhex("74f4ddd3067b2ebbc900"), bytes.fromhex("3e8c1e8d307f5510ec02")]


class OutOfRange(Exception):
    """What a node answers to a Neighbor Select or Neighbor Info past the end of its table."""


class LinkLost(Exception):
    """What a request on a dropped connection fails with."""


@dataclass
class FakePeripheral:
    """One node: what it serves, and how it misbehaves when a test asks it to."""

    address: str
    services: list[SimpleNamespace] = field(default_factory=embenet_services)
    values: dict[str, bytes] = field(default_factory=embenet_values)
    neighbors: list[bytes] = field(default_factory=lambda: list(NEIGHBORS))
    #: Raised by the connection attempt.
    connect_error: Exception | None = None
    #: When set, the connection attempt waits for it.
    connect_gate: asyncio.Event | None = None
    #: Connection attempts started.
    connect_attempts: int = 0
    #: Reads of the characteristic with that UUID wait for the event.
    read_gates: dict[str, asyncio.Event] = field(default_factory=dict)
    #: Raised by reads of, and writes to, the characteristic with that UUID.
    read_errors: dict[str, Exception] = field(default_factory=dict)
    write_errors: dict[str, Exception] = field(default_factory=dict)
    #: Called after each Neighbor Info read, with the index read.
    on_neighbor_read: Callable[[int], None] | None = None
    #: Every write but Neighbor Select's: (UUID, value, with response).
    written: list[tuple[str, bytes, bool]] = field(default_factory=list)
    selected: int = 0


class FakeClient:
    """A connection to a :class:`FakePeripheral`, as much of BleakClient as the station uses."""

    def __init__(self, peripheral: FakePeripheral, disconnected_callback: Callable[[Any], None]) -> None:
        self.peripheral = peripheral
        self.disconnected_callback = disconnected_callback
        self.services = peripheral.services
        self.is_connected = False
        #: Calls to disconnect(), whether connected or not.
        self.disconnects = 0

    async def connect(self) -> None:
        self.peripheral.connect_attempts += 1
        if self.peripheral.connect_gate is not None:
            await self.peripheral.connect_gate.wait()
        if self.peripheral.connect_error is not None:
            raise self.peripheral.connect_error
        self.is_connected = True

    async def disconnect(self) -> None:
        self.is_connected = False
        self.disconnects += 1

    def _check(self, uuid: str, errors: dict[str, Exception]) -> None:
        if not self.is_connected:
            raise LinkLost("not connected")
        if uuid in errors:
            raise errors[uuid]

    async def read_gatt_char(self, characteristic: SimpleNamespace) -> bytearray:
        node = self.peripheral
        if characteristic.uuid in node.read_gates:
            await node.read_gates[characteristic.uuid].wait()
        self._check(characteristic.uuid, node.read_errors)
        if characteristic.uuid == gatt.NEIGHBORS_COUNT:
            return bytearray(len(node.neighbors).to_bytes(2, "little"))
        if characteristic.uuid == gatt.NEIGHBOR_SELECT:
            return bytearray(node.selected.to_bytes(2, "little"))
        if characteristic.uuid == gatt.NEIGHBOR_INFO:
            if node.selected >= len(node.neighbors):
                raise OutOfRange()
            value = node.neighbors[node.selected]
            if node.on_neighbor_read:
                node.on_neighbor_read(node.selected)
            return bytearray(value)
        return bytearray(node.values[characteristic.uuid])

    async def write_gatt_char(self, characteristic: SimpleNamespace, data: bytes, response: bool = False) -> None:
        node = self.peripheral
        self._check(characteristic.uuid, node.write_errors)
        if characteristic.uuid == gatt.NEIGHBOR_SELECT:
            index = int.from_bytes(data, "little")
            if index >= len(node.neighbors):
                raise OutOfRange()
            node.selected = index
            return
        node.written.append((characteristic.uuid, bytes(data), response))
        if characteristic.uuid in node.values:
            node.values[characteristic.uuid] = bytes(data)

    def drop(self) -> None:
        """The node goes out of range."""
        self.is_connected = False
        self.disconnected_callback(self)


class FakeScanner:
    def __init__(self, callback: Callable[[Any, Any], None], **kwargs: Any) -> None:
        self.callback = callback
        self.kwargs = kwargs
        self.start_error: Exception | None = None
        self.stop_error: Exception | None = None
        self.scanning = False

    async def start(self) -> None:
        if self.start_error:
            raise self.start_error
        self.scanning = True

    async def stop(self) -> None:
        self.scanning = False
        if self.stop_error:
            raise self.stop_error


class FakeReasons:
    """BlueZ's Disconnected signals, as the station's DisconnectReasons hands them on."""

    def __init__(self, callback: Callable[[str, str], None]) -> None:
        self.callback = callback
        self.start_error: Exception | None = None
        self.started = False

    async def start(self) -> None:
        if self.start_error:
            raise self.start_error
        self.started = True

    def stop(self) -> None:
        self.started = False

    def say(self, address: str, reason: str) -> None:
        """BlueZ reports why the connection to `address` ended."""
        self.callback(address, reason)


class FakeBench:
    """The radio environment of one test: the nodes in it, the scanner, the source of disconnect reasons, every connection made."""

    def __init__(self) -> None:
        self.peripherals: dict[str, FakePeripheral] = {}
        self.clients: list[FakeClient] = []
        self.scanner: FakeScanner | None = None
        self.reasons: FakeReasons | None = None
        #: Set by a test before the app starts, to make scanning, or listening for disconnect reasons, fail.
        self.scan_error: Exception | None = None
        self.reasons_error: Exception | None = None

    def add(self, address: str, **kwargs: Any) -> FakePeripheral:
        self.peripherals[address] = FakePeripheral(address, **kwargs)
        return self.peripherals[address]

    def scanner_factory(self, callback: Callable[[Any, Any], None], **kwargs: Any) -> FakeScanner:
        self.scanner = FakeScanner(callback, **kwargs)
        self.scanner.start_error = self.scan_error
        return self.scanner

    def reasons_factory(self, callback: Callable[[str, str], None]) -> FakeReasons:
        self.reasons = FakeReasons(callback)
        self.reasons.start_error = self.reasons_error
        return self.reasons

    def client_factory(self, device: Any, timeout: float, disconnected_callback: Callable[[Any], None]) -> FakeClient:
        client = FakeClient(self.peripherals[device.address], disconnected_callback)
        self.clients.append(client)
        return client

    def advertise(self, address: str, name: str | None, rssi: int, uuids: tuple[str, ...] = (gatt.PROVISIONING_SERVICE,)) -> None:
        """One advertisement, with its scan response, as BlueZ reports it."""
        assert self.scanner is not None, "the app has not started scanning yet"
        device = SimpleNamespace(address=address, name=name)
        self.scanner.callback(device, SimpleNamespace(local_name=name, rssi=rssi, service_uuids=list(uuids)))
