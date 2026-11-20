"""What the station reads from, and writes to, a connected embeNET node.

The bleak side of the station, with no UI in it: :func:`read_node` turns a
connection into values ready to show, and :func:`write` writes what the user
gave. Both work on a connected :class:`bleak.BleakClient`; the tests drive them
with a fake one (tests/fakes.py).
"""

from __future__ import annotations

from collections.abc import Callable, Collection
from dataclasses import dataclass

from bleak import BleakClient
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.service import BleakGATTService

from central import gatt

#: bleak's description of a UUID it has no name for.
UNKNOWN = "Unknown"

#: What the station says when the node serves no Connection Parameters.
NO_CONNECTION_PARAMETERS = "not served by this firmware"

#: What the station says of a value a pass has not got to yet.
NOT_READ_YET = "not read yet"

_WRITE_PROPERTIES = frozenset({"write", "write-without-response"})


def error_text(error: BaseException) -> str:
    """An error for the screen: its message, or its type when it has none (as TimeoutError has not)."""
    return str(error) or type(error).__name__


@dataclass(frozen=True)
class CharacteristicValue:
    name: str
    #: Decoded for the screen; may span several lines.
    value: str
    #: The value is an error message rather than what the node holds.
    error: bool = False


@dataclass(frozen=True)
class ServiceValues:
    #: Lowercase, as bleak gives it.
    uuid: str
    name: str
    characteristics: list[CharacteristicValue]


@dataclass(frozen=True)
class NodeReading:
    """One pass over a node's services."""

    services: list[ServiceValues]
    #: The parameters of the connection, or why they are not known; None when
    #: the pass left out the service that holds them.
    connection: gatt.ConnectionParameters | str | None
    #: False while the pass is still going: what has been read so far.
    complete: bool = True


def merged(old: NodeReading | None, new: NodeReading) -> NodeReading:
    """`old` with the services `new` read replaced by their new values, and the
    connection's parameters too if `new` read them."""
    if old is None:
        return new
    fresh = {s.uuid: s for s in new.services}
    services = [fresh.pop(s.uuid, s) for s in old.services] + list(fresh.values())
    return NodeReading(services, old.connection if new.connection is None else new.connection)


def _bleak_name(uuid: str, description: str) -> str:
    return uuid if description == UNKNOWN else description


def characteristic_name(characteristic: BleakGATTCharacteristic) -> str:
    """The embeNET name, bleak's name for a standard one, or the UUID."""
    return gatt.characteristic_name(characteristic.uuid, _bleak_name(characteristic.uuid, characteristic.description))


def service_name(service: BleakGATTService) -> str:
    return gatt.service_name(service.uuid, _bleak_name(service.uuid, service.description))


def shown_services(client: BleakClient) -> list[BleakGATTService]:
    """The services the station shows: all but Generic Access and Generic Attribute,
    embeNET ones first, each group in handle order."""
    return sorted(
        (s for s in client.services if s.uuid.lower() not in gatt.HIDDEN_SERVICES),
        key=lambda s: (s.uuid.lower() not in gatt.SERVICE_NAMES, s.handle),
    )


def _in_handle_order(service: BleakGATTService) -> list[BleakGATTCharacteristic]:
    return sorted(service.characteristics, key=lambda c: c.handle)


def find_characteristic(client: BleakClient, uuid: str) -> BleakGATTCharacteristic | None:
    """The node's characteristic `uuid`, if it serves it."""
    return next((c for s in client.services for c in s.characteristics if c.uuid.lower() == uuid), None)


async def write(client: BleakClient, characteristic: BleakGATTCharacteristic, text: str) -> int:
    """Writes what the user gave for `characteristic`, and returns how many bytes that was.

    Raises ValueError, saying what is wrong, when the text does not encode, and
    bleak's errors when the write fails. A characteristic that takes Write
    Requests gets one, so a refusal by the node is reported rather than lost.
    """
    data = gatt.encode(characteristic.uuid, text)
    await client.write_gatt_char(characteristic, data, response="write" in characteristic.properties)
    return len(data)


async def read_neighbors(client: BleakClient, service: BleakGATTService) -> str:
    """The Info service's neighbor table, one neighbor per line.

    The node serves one entry at a time: Neighbor Select takes the order number,
    0 .. Neighbors Count - 1, and Neighbor Info then holds that entry. The table
    can shrink while it is walked; the node then refuses the selection or the
    read (Out of Range), and the walk ends with the entries it has.
    """
    by_uuid = {c.uuid.lower(): c for c in service.characteristics}
    count = gatt.neighbors_count(bytes(await client.read_gatt_char(by_uuid[gatt.NEIGHBORS_COUNT])))
    lines = []
    for index in range(count):
        try:
            await client.write_gatt_char(by_uuid[gatt.NEIGHBOR_SELECT], gatt.neighbor_select(index), response=True)
            raw = await client.read_gatt_char(by_uuid[gatt.NEIGHBOR_INFO])
        except Exception:  # Out of Range; or the link is gone, which the session notices by itself
            break
        lines.append(gatt.decode(gatt.NEIGHBOR_INFO, bytes(raw)))
    return "\n".join(lines) if lines else "none"


async def _read_value(client: BleakClient, characteristic: BleakGATTCharacteristic) -> CharacteristicValue:
    name = characteristic_name(characteristic)
    properties = characteristic.properties
    if "read" not in properties:
        return CharacteristicValue(name, "write-only" if _WRITE_PROPERTIES & set(properties) else ", ".join(properties))
    try:
        raw = await client.read_gatt_char(characteristic)
    except Exception as error:  # bleak and D-Bus errors alike: show it, and go on with the rest
        return CharacteristicValue(name, f"read failed: {error_text(error)}", error=True)
    return CharacteristicValue(name, gatt.decode(characteristic.uuid, bytes(raw)))


async def read_node(
    client: BleakClient,
    progress: Callable[[NodeReading], None] | None = None,
    only: Collection[str] | None = None,
) -> NodeReading:
    """Every shown service and every characteristic's value, and the connection's parameters;
    with `only`, just the services with those (lowercase) UUIDs.

    Neighbor Select is the Info service's control rather than a value, so it is
    left out, and Neighbor Info shows the whole table, walked through it.
    Connection Parameters is returned on its own rather than among the values.

    Each request takes a connection interval or more, which with a node that
    asks for long intervals adds up to many seconds. `progress`, if given, gets
    the reading so far after every value, marked incomplete.
    """
    services: list[ServiceValues] = []
    connection: gatt.ConnectionParameters | str | None = None
    for service in shown_services(client):
        uuid_of_service = service.uuid.lower()
        if only is not None and uuid_of_service not in only:
            continue
        values: list[CharacteristicValue] = []
        for characteristic in _in_handle_order(service):
            uuid = characteristic.uuid.lower()
            if uuid == gatt.NEIGHBOR_SELECT:
                continue
            if uuid == gatt.CONNECTION_PARAMETERS:
                try:
                    connection = gatt.connection_parameters(bytes(await client.read_gatt_char(characteristic)))
                except Exception as error:  # including gatt.Malformed
                    connection = f"read failed: {error_text(error)}"
            elif uuid == gatt.NEIGHBOR_INFO:
                try:
                    values.append(CharacteristicValue("Neighbors", await read_neighbors(client, service)))
                except Exception as error:
                    values.append(CharacteristicValue("Neighbors", f"read failed: {error_text(error)}", error=True))
            else:
                values.append(await _read_value(client, characteristic))
            if progress is not None:
                so_far = [*services, ServiceValues(uuid_of_service, service_name(service), list(values))]
                progress(NodeReading(so_far, connection if connection is not None else NOT_READ_YET, complete=False))
        services.append(ServiceValues(uuid_of_service, service_name(service), values))
    if connection is None and only is None:
        connection = NO_CONNECTION_PARAMETERS
    return NodeReading(services, connection)
