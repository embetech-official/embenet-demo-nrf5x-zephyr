"""Reading and writing a connected node, without the screen."""

from __future__ import annotations

import pytest
from fakes import BATTERY, BATTERY_LEVEL, FakeClient, FakePeripheral, LinkLost, characteristic, service

from central import client, gatt
from central.gatt import embenet_uuid as U


async def connected(peripheral: FakePeripheral) -> FakeClient:
    fake = FakeClient(peripheral, lambda _client: None)
    await fake.connect()
    return fake


def values(reading: client.NodeReading, service_name: str) -> dict[str, str]:
    (found,) = (s for s in reading.services if s.name == service_name)
    return {c.name: c.value for c in found.characteristics}


async def test_reads_every_embenet_value_decoded_in_handle_order() -> None:
    reading = await client.read_node(await connected(FakePeripheral("A")))

    assert [s.name for s in reading.services] == ["embeNET Info", "embeNET Provisioning"]
    info = values(reading, "embeNET Info")
    assert list(info) == ["EUI-64", "Stack Version", "Groups", "Neighbors Count", "Neighbors", "Parent EUI-64", "Parent RSSI", "Radio Duty Cycle"]
    assert info["EUI-64"] == "f4679e2bfe455876"
    assert info["Neighbors"] == "bb2e7b06d3ddf474  -55 dBm  parent\n10557f308d1e8c3e  -20 dBm  unrelated"
    assert info["Radio Duty Cycle"] == "4.54 %"
    assert values(reading, "embeNET Provisioning") == {"K1": "write-only", "PSK": "write-only", "Active": "on (0x01)"}
    assert reading.connection == gatt.ConnectionParameters(interval_us=45000, latency=0, timeout_ms=420)


async def test_progress_reports_every_value_read_so_far() -> None:
    seen: list[client.NodeReading] = []

    final = await client.read_node(await connected(FakePeripheral("A")), seen.append)

    assert all(not reading.complete for reading in seen)
    assert final.complete
    counts = [sum(len(s.characteristics) for s in reading.services) for reading in seen]
    assert counts == sorted(counts)  # only ever grows
    assert seen[0].services[0].characteristics == [client.CharacteristicValue("EUI-64", "f4679e2bfe455876")]
    assert seen[0].connection == client.NOT_READ_YET
    assert seen[-1].services == final.services
    assert seen[-1].connection == final.connection


async def test_generic_access_and_neighbor_select_are_not_shown() -> None:
    reading = await client.read_node(await connected(FakePeripheral("A")))

    names = {c.name for s in reading.services for c in s.characteristics}
    assert "Device Name" not in names
    assert "Neighbor Select" not in names
    assert "Connection Parameters" not in names


async def test_other_services_follow_with_bleak_names_or_uuids() -> None:
    peripheral = FakePeripheral("A")
    vendor_service, vendor_char = "12345678-0000-1000-8000-00805f9b34fb", "12345679-0000-1000-8000-00805f9b34fb"
    peripheral.services.insert(
        0,
        service(BATTERY, 50, characteristic(BATTERY_LEVEL, 51, "read", "notify", description="Battery Level"), description="Battery Service"),
    )
    peripheral.services.append(service(vendor_service, 60, characteristic(vendor_char, 61, "notify")))
    peripheral.values[BATTERY_LEVEL] = b"\x5a"

    reading = await client.read_node(await connected(peripheral))

    assert [s.name for s in reading.services] == ["embeNET Info", "embeNET Provisioning", "Battery Service", vendor_service]
    assert values(reading, "Battery Service") == {"Battery Level": "5a"}
    assert values(reading, vendor_service) == {vendor_char: "notify"}


async def test_a_failed_read_is_shown_and_the_rest_still_read() -> None:
    peripheral = FakePeripheral("A", read_errors={U(0x0201): TimeoutError()})

    reading = await client.read_node(await connected(peripheral))

    eui, version = reading.services[0].characteristics[:2]
    assert eui == client.CharacteristicValue("EUI-64", "read failed: TimeoutError", error=True)
    assert version == client.CharacteristicValue("Stack Version", "1.1.4+6b937e4")


async def test_neighbor_table_that_shrinks_while_walked_ends_with_what_was_read() -> None:
    peripheral = FakePeripheral("A")
    peripheral.on_neighbor_read = lambda index: peripheral.neighbors.pop()  # gone before the next selection

    info = values(await client.read_node(await connected(peripheral)), "embeNET Info")

    assert info["Neighbors"] == "bb2e7b06d3ddf474  -55 dBm  parent"


async def test_empty_neighbor_table() -> None:
    info = values(await client.read_node(await connected(FakePeripheral("A", neighbors=[]))), "embeNET Info")

    assert info["Neighbors"] == "none"
    assert info["Neighbors Count"] == "0"


async def test_neighbor_table_that_cannot_be_read() -> None:
    peripheral = FakePeripheral("A", read_errors={gatt.NEIGHBORS_COUNT: LinkLost("gone")})

    reading = await client.read_node(await connected(peripheral))

    (neighbors,) = (c for c in reading.services[0].characteristics if c.name == "Neighbors")
    assert neighbors.value == "read failed: gone"
    assert neighbors.error


async def test_connection_parameters_unknown() -> None:
    absent = FakePeripheral("A")
    info = absent.services[2]
    info.characteristics = [c for c in info.characteristics if c.uuid != gatt.CONNECTION_PARAMETERS]
    assert (await client.read_node(await connected(absent))).connection == client.NO_CONNECTION_PARAMETERS

    failing = FakePeripheral("B", read_errors={gatt.CONNECTION_PARAMETERS: LinkLost("gone")})
    assert (await client.read_node(await connected(failing))).connection == "read failed: gone"

    malformed = FakePeripheral("C")
    malformed.values[gatt.CONNECTION_PARAMETERS] = bytes(6)
    assert (await client.read_node(await connected(malformed))).connection == "read failed: 6 B, expected 8 B"


async def test_only_the_services_asked_for_are_read() -> None:
    reading = await client.read_node(await connected(FakePeripheral("A")), only={gatt.PROVISIONING_SERVICE})

    assert [s.uuid for s in reading.services] == [gatt.PROVISIONING_SERVICE]
    assert reading.connection is None  # the Info service holds it, and was left out

    info_only = await client.read_node(await connected(FakePeripheral("A")), only={gatt.INFO_SERVICE})
    assert info_only.connection == gatt.ConnectionParameters(interval_us=45000, latency=0, timeout_ms=420)


async def test_merged_replaces_only_what_was_read_again() -> None:
    peripheral = FakePeripheral("A")
    fake = await connected(peripheral)
    full = await client.read_node(fake)
    peripheral.values[gatt.ACTIVE] = b"\x00"
    peripheral.values[U(0x0209)] = bytes.fromhex("e803")

    provisioning = await client.read_node(fake, only={gatt.PROVISIONING_SERVICE})
    result = client.merged(full, provisioning)

    assert [s.name for s in result.services] == ["embeNET Info", "embeNET Provisioning"]
    assert values(result, "embeNET Provisioning")["Active"] == "off (0x00)"
    assert values(result, "embeNET Info")["Radio Duty Cycle"] == "4.54 %"  # not read again
    assert result.connection == full.connection
    assert client.merged(None, provisioning) is provisioning


async def test_merged_takes_a_service_new_to_it() -> None:
    fake = await connected(FakePeripheral("A"))
    info = await client.read_node(fake, only={gatt.INFO_SERVICE})
    provisioning = await client.read_node(fake, only={gatt.PROVISIONING_SERVICE})

    result = client.merged(info, provisioning)

    assert [s.name for s in result.services] == ["embeNET Info", "embeNET Provisioning"]
    assert result.connection == info.connection


async def test_find_characteristic() -> None:
    fake = await connected(FakePeripheral("A"))

    found = client.find_characteristic(fake, gatt.K1)
    assert found is not None
    assert client.characteristic_name(found) == "K1"
    assert client.find_characteristic(fake, "0000aaaa-0000-1000-8000-00805f9b34fb") is None


async def test_write_encodes_and_asks_for_a_response() -> None:
    peripheral = FakePeripheral("A")
    fake = await connected(peripheral)
    k1, active = client.find_characteristic(fake, gatt.K1), client.find_characteristic(fake, gatt.ACTIVE)
    assert k1 is not None
    assert active is not None

    assert await client.write(fake, k1, "00112233445566778899aabbccddeeff") == 16
    assert await client.write(fake, active, "off") == 1
    assert peripheral.written == [(U(0x0101), bytes.fromhex("00112233445566778899aabbccddeeff"), True), (U(0x0103), b"\x00", True)]


async def test_write_without_response_characteristic() -> None:
    peripheral = FakePeripheral("A")
    provisioning = peripheral.services[1]
    provisioning.characteristics[2].properties = ["read", "write-without-response"]
    fake = await connected(peripheral)
    active = client.find_characteristic(fake, gatt.ACTIVE)
    assert active is not None

    await client.write(fake, active, "on")

    assert peripheral.written == [(gatt.ACTIVE, b"\x01", False)]


async def test_write_of_something_that_does_not_encode_sends_nothing() -> None:
    peripheral = FakePeripheral("A")
    fake = await connected(peripheral)
    k1 = client.find_characteristic(fake, gatt.K1)
    assert k1 is not None

    with pytest.raises(ValueError, match="4 of 32 hex digits"):
        await client.write(fake, k1, "0011")
    assert peripheral.written == []


def test_error_text() -> None:
    assert client.error_text(TimeoutError()) == "TimeoutError"
    assert client.error_text(LinkLost("ATT error 0x0e")) == "ATT error 0x0e"
