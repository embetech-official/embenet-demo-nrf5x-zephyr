"""The screen, driven headless against a fake bench."""

from __future__ import annotations

import asyncio
import time
from types import SimpleNamespace

import pytest
from conftest import ADDRESS, click, connect, make_app, note_text, panel, plain, rows, summary_text, values_text, wait_for
from fakes import FakeBench, LinkLost
from textual.content import Content
from textual.pilot import Pilot
from textual.widgets import Button, Input, Static, Switch

import ble_central
from ble_central import CentralApp, KeyField, Node, NodeRow, ProvisioningPanel, State
from central import gatt
from central.gatt import embenet_uuid as U

# --- Scanning ----------------------------------------------------------------


async def test_lists_only_embenet_nodes_with_name_and_rssi(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.advertise("AA:00:00:00:00:01", "embeNET nRF54", -40)
    bench.advertise("AA:00:00:00:00:02", "Headphones", -50, uuids=("0000110b-0000-1000-8000-00805f9b34fb",))
    bench.advertise("AA:00:00:00:00:03", "embeNET nRF52", -70)
    bench.advertise("AA:00:00:00:00:01", "embeNET nRF54", -45)  # an update, not a new row
    await wait_for(pilot, lambda: rows(app)[0].border_title)

    assert [(r.node.name, r.node.rssi) for r in rows(app)] == [("embeNET nRF54", -45), ("embeNET nRF52", -70)]
    first = rows(app)[0]
    assert "embeNET nRF54" in Content.from_markup(first.border_title or "").plain
    assert "AA:00:00:00:00:01" in summary_text(first)
    assert "RSSI      -45 dBm" in summary_text(first)
    assert "state     not connected" in summary_text(first)
    assert plain(app.query_one("#status", Static).content) == "default adapter · active scan · 2 nodes · 0 connected"


async def test_scans_actively_and_hears_every_advertisement(bench: FakeBench, pilot: Pilot[None]) -> None:
    assert bench.scanner is not None
    assert bench.scanner.scanning
    assert bench.scanner.kwargs == {"scanning_mode": "active", "bluez": {"filters": {"DuplicateData": True}}}


async def test_adapter_is_passed_to_bluez(bench: FakeBench) -> None:
    app = make_app(bench, adapter="hci1")
    async with app.run_test() as pilot:
        await pilot.pause()
        assert bench.scanner is not None
        assert bench.scanner.kwargs["bluez"]["adapter"] == "hci1"
        assert plain(app.query_one("#status", Static).content).startswith("hci1 · active scan · 0 nodes")


async def test_name_arrives_with_a_later_scan_response(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.advertise(ADDRESS, None, -40)
    await pilot.pause()
    assert rows(app)[0].node.name == "?"

    bench.advertise(ADDRESS, "embeNET nRF54", -41)
    bench.advertise(ADDRESS, None, -42)  # an advertisement without the scan response keeps the name
    await pilot.pause()
    assert rows(app)[0].node.name == "embeNET nRF54"
    assert rows(app)[0].node.rssi == -42


async def test_scan_failure_is_shown(bench: FakeBench) -> None:
    bench.scan_error = LinkLost("No Bluetooth adapters found.")
    app = make_app(bench)
    async with app.run_test() as pilot:
        await pilot.pause()
        assert "scan failed: No Bluetooth adapters found." in plain(app.query_one("#status", Static).content)


async def test_node_not_heard_from_is_dimmed_and_says_since_when(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    await pilot.pause()
    row = rows(app)[0]
    assert not row.has_class("stale")

    row.node.last_seen = time.monotonic() - 20
    app._tick()

    assert row.has_class("stale")
    assert "(20 s ago)" in summary_text(row)


async def test_node_not_heard_from_for_30_s_is_removed_until_it_advertises_again(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.advertise("AA:00:00:00:00:01", "embeNET nRF54", -40)
    bench.advertise("AA:00:00:00:00:02", "embeNET nRF52", -50)
    await pilot.pause()
    gone, kept = rows(app)

    gone.node.last_seen = time.monotonic() - 31
    app._tick()
    await pilot.pause()

    assert rows(app) == [kept]
    assert list(app.rows) == ["AA:00:00:00:00:02"]
    assert plain(app.query_one("#status", Static).content) == "default adapter · active scan · 1 node · 0 connected"

    bench.advertise("AA:00:00:00:00:01", "embeNET nRF54", -42)
    await pilot.pause()
    assert [(r.node.address, r.node.rssi) for r in rows(app)] == [("AA:00:00:00:00:02", -50), ("AA:00:00:00:00:01", -42)]


async def test_connected_node_is_kept_however_long_it_is_silent(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)

    row.node.last_seen = time.monotonic() - 300  # a connected node does not advertise
    app._tick()
    await pilot.pause()

    assert rows(app) == [row]


# --- Connecting --------------------------------------------------------------


def connect_button(row: NodeRow) -> Button:
    return row.query_one(".connect", Button)


async def test_connect_button_connects_and_fills_the_row(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    await pilot.pause()
    row = rows(app)[0]
    assert str(connect_button(row).label) == "Connect"

    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.read_at)

    assert row.node.state is State.CONNECTED
    assert row.has_class("connected")
    assert str(connect_button(row).label) == "Disconnect"
    left = summary_text(row)
    assert "state     connected" in left
    assert f"read      {row.node.read_at:%H:%M:%S}" in left
    assert "interval  45 ms" in left
    assert "latency   0" in left
    assert "timeout   420 ms" in left
    assert [p.title for p in row.query(ble_central.ServicePanel)] == ["embeNET Info", "embeNET Provisioning"]
    right = values_text(row)
    assert "f4679e2bfe455876" in right
    assert "bb2e7b06d3ddf474  -55 dBm  parent" in right
    assert "node: on (0x01)" in right
    assert "Generic Access" not in right
    assert "Device Name" not in right
    assert plain(app.query_one("#status", Static).content).endswith("1 node · 1 connected")


async def test_clicking_a_row_only_highlights_it(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.add("AA:00:00:00:00:02")
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    bench.advertise("AA:00:00:00:00:02", "embeNET nRF52", -50)
    await pilot.pause()
    second = rows(app)[1]

    await click(pilot, second.query_one(".summary"))
    await pilot.pause(0.2)

    assert app.query_one(ble_central.NodeList).highlighted_child is second
    assert second.node.state is State.IDLE
    assert bench.clients == []


async def test_first_reading_shows_each_value_as_it_arrives(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    held = asyncio.Event()
    bench.peripherals[ADDRESS].read_gates[U(0x0209)] = held  # Radio Duty Cycle, the last Info value
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    await pilot.pause()
    row = rows(app)[0]

    await pilot.press("enter")
    await wait_for(pilot, lambda: "Parent RSSI" in values_text(row))

    assert "f4679e2bfe455876" in values_text(row)
    assert "Radio Duty Cycle" not in values_text(row)
    assert "node: not read yet" in values_text(row)
    assert "embeNET Info  reading…" in values_text(row)
    assert "read      in progress…" in summary_text(row)
    assert "connection parameters: not read yet" in " ".join(summary_text(row).split())

    held.set()
    await wait_for(pilot, lambda: row.node.read_at)
    assert "node: on (0x01)" in values_text(row)
    assert f"embeNET Info  read {row.node.read_at:%H:%M:%S}" in values_text(row)
    assert "interval  45 ms" in summary_text(row)


async def test_later_readings_replace_the_values_only_when_complete(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    first = row.node.read_at
    held = asyncio.Event()
    bench.peripherals[ADDRESS].read_gates[U(0x0209)] = held
    bench.peripherals[ADDRESS].values[U(0x0201)] = bytes(8)
    await pilot.press("r")
    await pilot.pause(0.2)

    assert "f4679e2bfe455876" in values_text(row)  # the whole previous reading, not half of the new one
    assert "node: on (0x01)" in values_text(row)

    held.set()
    await wait_for(pilot, lambda: row.node.read_at != first)
    assert "0000000000000000" in values_text(row)


async def test_firmware_without_connection_parameters_says_so(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    info = bench.peripherals[ADDRESS].services[2]
    info.characteristics = [c for c in info.characteristics if c.uuid != U(0x020A)]

    row = await connect(pilot, app, bench)

    assert "connection parameters: not served by this firmware" in " ".join(summary_text(row).split())


async def test_other_services_get_a_panel_of_their_own(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    from fakes import BATTERY, BATTERY_LEVEL, characteristic, service

    peripheral = bench.peripherals[ADDRESS]
    peripheral.services.append(service(BATTERY, 50, characteristic(BATTERY_LEVEL, 51, "read"), description="Battery Service"))
    peripheral.values[BATTERY_LEVEL] = b"\x5a"

    row = await connect(pilot, app, bench)

    assert [p.title for p in row.query(ble_central.ServicePanel)] == ["embeNET Info", "embeNET Provisioning", "Battery Service"]
    assert "5a" in plain(panel(row, "Battery Service").query_one(".panel-body", Static).content)


async def test_disconnect_button_disconnects_and_empties_the_row(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)

    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert not row.has_class("connected")
    assert not bench.clients[0].is_connected
    assert list(row.query(ble_central.ServicePanel)) == []
    assert note_text(row) == ""
    assert "interval" not in summary_text(row)
    assert str(connect_button(row).label) == "Connect"


async def test_enter_disconnects_too(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)

    await pilot.press("enter")
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert not bench.clients[0].is_connected


async def test_values_are_read_once_and_then_only_when_asked(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    first = row.node.read_at
    bench.peripherals[ADDRESS].values[U(0x0209)] = bytes.fromhex("e803")  # 10 %

    await pilot.pause(1.0)

    assert row.node.read_at == first
    assert "4.54 %" in values_text(row)


async def test_read_now(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    bench.peripherals[ADDRESS].values[U(0x0209)] = bytes.fromhex("e803")

    await pilot.press("r")
    await wait_for(pilot, lambda: "10.00 %" in values_text(row))


async def test_refresh_reads_only_its_own_service(bench: FakeBench) -> None:
    app = make_app(bench)
    async with app.run_test(size=(160, 50)) as pilot:
        row = await connect(pilot, app, bench)
        peripheral = bench.peripherals[ADDRESS]
        peripheral.values[U(0x0209)] = bytes.fromhex("e803")  # Info: 10 %
        peripheral.values[gatt.ACTIVE] = b"\x00"  # Provisioning: off
        provisioning = panel(row, "embeNET Provisioning")
        before = row.node.service_read_at[gatt.INFO_SERVICE]

        await click(pilot, provisioning.query_one(".refresh", Button))
        await wait_for(pilot, lambda: "node: off (0x00)" in values_text(row))

        assert "4.54 %" in values_text(row)  # Info not read again
        assert row.node.service_read_at[gatt.INFO_SERVICE] == before
        assert row.node.reading is not None
        assert row.node.reading.connection == gatt.ConnectionParameters(interval_us=45000, latency=0, timeout_ms=420)

        await click(pilot, panel(row, "embeNET Info").query_one(".refresh", Button))
        await wait_for(pilot, lambda: "10.00 %" in values_text(row))


async def test_refresh_pressed_during_a_pass_waits_for_it(bench: FakeBench) -> None:
    app = make_app(bench)
    async with app.run_test(size=(160, 50)) as pilot:
        row = await connect(pilot, app, bench)
        held = asyncio.Event()
        bench.peripherals[ADDRESS].read_gates[U(0x0209)] = held
        await pilot.press("r")
        await wait_for(pilot, lambda: row.node.reading_now)
        info = panel(row, "embeNET Info")

        await click(pilot, info.query_one(".refresh", Button))
        await pilot.pause()

        assert info.query_one(".refresh", Button).disabled  # asked for already
        assert "embeNET Info  reading…" in values_text(row)
        bench.peripherals[ADDRESS].values[U(0x0209)] = bytes.fromhex("e803")
        held.set()
        await wait_for(pilot, lambda: not row.node.refresh_requested and not row.node.reading_now and "10.00 %" in values_text(row))
        assert not info.query_one(".refresh", Button).disabled


async def test_cancel_while_connecting(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.peripherals[ADDRESS].connect_gate = asyncio.Event()  # never set: the attempt hangs until cancelled
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    await pilot.pause()
    row = rows(app)[0]
    await pilot.press("enter")
    await wait_for(pilot, lambda: row.node.state is State.CONNECTING)
    assert row.has_class("busy")
    assert str(connect_button(row).label) == "Cancel"

    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert row.node.note == ""  # on request: nothing to report


async def test_one_connection_attempt_at_a_time(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    second_address = "AA:00:00:00:00:02"
    bench.add(second_address)
    gate = bench.peripherals[ADDRESS].connect_gate = asyncio.Event()
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    bench.advertise(second_address, "embeNET nRF52", -50)
    await pilot.pause()
    first, second = rows(app)

    await click(pilot, connect_button(first))
    await wait_for(pilot, lambda: first.node.state is State.CONNECTING)
    await click(pilot, connect_button(second))
    await wait_for(pilot, lambda: second.node.state is State.WAITING)

    assert "state     waiting to connect…" in summary_text(second)
    assert str(connect_button(second).label) == "Cancel"
    assert second.has_class("busy")
    assert bench.peripherals[second_address].connect_attempts == 0  # not started while the first one is under way

    gate.set()
    await wait_for(pilot, lambda: second.node.read_at)
    assert first.node.state is State.CONNECTED


async def test_cancel_while_waiting_to_connect(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    second_address = "AA:00:00:00:00:02"
    bench.add(second_address)
    bench.peripherals[ADDRESS].connect_gate = asyncio.Event()  # never set: holds the second one back
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    bench.advertise(second_address, "embeNET nRF52", -50)
    await pilot.pause()
    first, second = rows(app)
    await click(pilot, connect_button(first))
    await wait_for(pilot, lambda: first.node.state is State.CONNECTING)
    await click(pilot, connect_button(second))
    await wait_for(pilot, lambda: second.node.state is State.WAITING)
    await pilot.pause(0.3)  # a button ignores clicks while it shows the last one (0.2 s)

    await click(pilot, connect_button(second))
    await wait_for(pilot, lambda: second.node.state is State.IDLE)

    assert note_text(second) == ""
    assert bench.peripherals[second_address].connect_attempts == 0
    assert first.node.state is State.CONNECTING  # untouched


async def test_reconnecting_waits_for_the_next_advertisement(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE)
    await pilot.pause(0.3)  # a button ignores clicks while it shows the last one (0.2 s)

    # BlueZ may have dropped the device with the connection: connecting before the node is heard again fails.
    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.WAITING)
    await pilot.pause(0.2)
    assert len(bench.clients) == 1  # no attempt yet

    bench.advertise(ADDRESS, "embeNET nRF54", -41)
    await wait_for(pilot, lambda: row.node.read_at)
    assert len(bench.clients) == 2


async def test_cancel_while_waiting_for_the_advertisement(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE)
    await pilot.pause(0.3)
    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.WAITING)
    await pilot.pause(0.3)

    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert note_text(row) == ""
    assert len(bench.clients) == 1


async def test_a_button_in_a_row_highlights_the_row(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.add("AA:00:00:00:00:02")
    bench.peripherals["AA:00:00:00:00:02"].connect_gate = asyncio.Event()
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    bench.advertise("AA:00:00:00:00:02", "embeNET nRF52", -50)
    await pilot.pause()
    second = rows(app)[1]

    await click(pilot, connect_button(second))
    await pilot.pause()

    assert app.query_one(ble_central.NodeList).highlighted_child is second


async def test_lost_connection_is_reported_with_the_time_and_bluez_reason(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)

    bench.clients[0].drop()
    await wait_for(pilot, lambda: row.node.state is State.IDLE)
    assert bench.reasons is not None
    bench.reasons.say(ADDRESS, "link lost: nothing heard from the node within the supervision timeout")
    await pilot.pause()

    note = note_text(row).splitlines()
    assert note[0].startswith("connection lost at ")
    assert note[1] == "reason: link lost: nothing heard from the node within the supervision timeout"
    assert list(row.query(ble_central.ServicePanel)) == []


async def test_reason_arriving_before_the_session_ends_is_kept(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    assert bench.reasons is not None

    bench.reasons.say(ADDRESS.lower(), "the node closed the connection")
    bench.clients[0].drop()
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert note_text(row).endswith("reason: the node closed the connection")


async def test_a_later_reason_does_not_replace_the_first(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    assert bench.reasons is not None
    bench.clients[0].drop()
    bench.reasons.say(ADDRESS, "link lost: nothing heard from the node within the supervision timeout")
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    bench.reasons.say(ADDRESS, "this computer closed the connection")  # BlueZ, about the device, later
    await pilot.pause()

    assert note_text(row).endswith("reason: link lost: nothing heard from the node within the supervision timeout")


async def test_disconnect_during_a_read_that_swallows_the_cancellation(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    client = bench.clients[0]
    read = client.read_gatt_char
    reading = asyncio.Event()

    async def read_failing_on_cancel(characteristic: object) -> bytearray:
        # As a BlueZ request may: cancelled, it fails like any other request, and the pass goes on.
        if not reading.is_set():
            reading.set()
            try:
                await asyncio.Event().wait()
            except asyncio.CancelledError:
                raise LinkLost("cancelled") from None
        return await read(characteristic)  # type: ignore[arg-type]

    client.read_gatt_char = read_failing_on_cancel  # type: ignore[method-assign]
    await pilot.press("r")
    await wait_for(pilot, reading.is_set)

    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE, within_s=10)

    assert note_text(row) == ""
    assert not client.is_connected


async def test_reason_of_a_disconnect_on_request_is_not_shown(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.state is State.IDLE)
    assert bench.reasons is not None

    bench.reasons.say(ADDRESS, "this computer closed the connection")
    bench.reasons.say("AA:00:00:00:00:99", "no reason given")  # not a node in the list
    await pilot.pause()

    assert note_text(row) == ""


async def test_new_connection_forgets_the_last_reason(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    assert bench.reasons is not None
    bench.clients[0].drop()
    bench.reasons.say(ADDRESS, "the node closed the connection")
    await wait_for(pilot, lambda: row.node.state is State.IDLE)
    bench.advertise(ADDRESS, "embeNET nRF54", -40)  # a node advertises again once disconnected

    await click(pilot, connect_button(row))
    await wait_for(pilot, lambda: row.node.read_at)

    assert row.node.lost_reason == ""


async def test_without_the_system_bus_the_status_says_reasons_are_missing(bench: FakeBench) -> None:
    bench.reasons_error = LinkLost("no system bus")
    app = make_app(bench)
    async with app.run_test() as pilot:
        await pilot.pause()
        assert "no disconnect reasons: system bus unavailable" in plain(app.query_one("#status", Static).content)


async def test_connection_lost_during_a_read_shows_no_half_read_values(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    client = bench.clients[0]
    read = client.read_gatt_char

    async def read_then_drop(characteristic: object) -> bytearray:
        value = await read(characteristic)  # type: ignore[arg-type]
        client.drop()
        return value

    client.read_gatt_char = read_then_drop  # type: ignore[method-assign]
    await pilot.press("r")
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert note_text(row).startswith("connection lost at ")


async def test_connection_that_fails_mid_session(app: CentralApp, bench: FakeBench, pilot: Pilot[None], monkeypatch: pytest.MonkeyPatch) -> None:
    row = await connect(pilot, app, bench)

    async def broken(_client: object, _progress: object, _only: object) -> object:
        raise LinkLost("org.bluez.Error.Failed")

    monkeypatch.setattr(ble_central, "read_node", broken)
    await pilot.press("r")
    await wait_for(pilot, lambda: row.node.state is State.IDLE)

    assert note_text(row).startswith("connection lost at ")
    assert note_text(row).endswith(": org.bluez.Error.Failed")


async def test_failed_connection_is_reported_on_the_row(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.peripherals[ADDRESS].connect_error = TimeoutError()
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    await pilot.pause()
    row = rows(app)[0]

    await pilot.press("enter")
    await wait_for(pilot, lambda: row.node.note)

    assert row.node.state is State.IDLE
    assert note_text(row).startswith("connection failed at ")
    assert note_text(row).endswith(": TimeoutError")


async def test_quit_disconnects_every_node(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.add("AA:00:00:00:00:02")
    await connect(pilot, app, bench)
    await connect(pilot, app, bench, "AA:00:00:00:00:02")
    assert bench.scanner is not None
    assert bench.reasons is not None
    bench.scanner.stop_error = LinkLost("adapter gone")  # it still quits, and still disconnects

    await pilot.press("q")
    await pilot.pause()

    assert [c.is_connected for c in bench.clients] == [False, False]
    assert not bench.scanner.scanning
    assert not bench.reasons.started


async def test_quit_while_connecting_ends_the_attempt_before_exiting(
    app: CentralApp, bench: FakeBench, pilot: Pilot[None], monkeypatch: pytest.MonkeyPatch
) -> None:
    bench.peripherals[ADDRESS].connect_gate = asyncio.Event()  # never set: the attempt hangs until cancelled
    bench.advertise(ADDRESS, "embeNET nRF54", -40)
    await pilot.pause()
    row = rows(app)[0]
    await pilot.press("enter")
    await wait_for(pilot, lambda: row.node.state is State.CONNECTING)
    at_exit: list[tuple[State, int]] = []
    exit_app = app.exit

    def exit_recorded() -> None:
        at_exit.append((row.node.state, bench.clients[0].disconnects))
        exit_app()

    monkeypatch.setattr(app, "exit", exit_recorded)

    await pilot.press("q")
    await pilot.pause()

    # The session had closed the half-made connection itself by then: left to the exit, a real
    # attempt is cancelled twice, and bleak's clean-up is cut short with the connection still up.
    assert at_exit == [(State.IDLE, 1)]


# --- Provisioning ------------------------------------------------------------


def key_field(row: NodeRow, label: str) -> KeyField:
    (found,) = (f for f in row.query(KeyField) if f.label == label)
    return found


def key_status(field: KeyField) -> str:
    return plain(field.query_one(".key-status", Static).content)


async def type_into(pilot: Pilot[None], field: KeyField, text: str) -> None:
    """Puts `text` in the field, focused, as if typed (a key press each would take seconds for a key)."""
    field_input = field.query_one(Input)
    field_input.focus()
    field_input.value = text
    await pilot.pause()


async def test_provisioning_panel_has_a_switch_and_two_key_fields(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)

    provisioning = row.query_one(ProvisioningPanel)
    assert [f.label for f in provisioning.query(KeyField)] == ["K1", "PSK"]
    assert provisioning.query_one(Switch).value is True
    assert not provisioning.query_one(".panel-body", Static).display  # nothing left that the controls do not show
    k1 = key_field(row, "K1")
    assert k1.query_one(Input).max_length == 32
    assert k1.query_one(Button).disabled
    assert key_status(k1) == "32 hex digits, either case"


async def test_typing_a_key_is_checked_as_it_goes(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    k1 = key_field(row, "K1")

    await type_into(pilot, k1, "0011aaBB")
    assert key_status(k1) == "8 of 32 hex digits"
    assert k1.query_one(Button).disabled

    await type_into(pilot, k1, "0011 ")
    assert key_status(k1) == "' ' is not a hex digit: use 0-9 and a-f"
    assert k1.query_one(Input).has_class("-invalid")

    await type_into(pilot, k1, "00112233445566778899AABBCCDDEEFF")
    assert key_status(k1) == "complete: Enter or Write writes it"
    assert not k1.query_one(Button).disabled
    assert not k1.query_one(Input).has_class("-invalid")

    await type_into(pilot, k1, "")
    assert key_status(k1) == "32 hex digits, either case"


async def test_write_a_key_with_enter(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    k1 = key_field(row, "K1")

    await type_into(pilot, k1, "00112233445566778899aabbccddeeff")
    await pilot.press("enter")
    await wait_for(pilot, lambda: "written" in key_status(k1))

    assert bench.peripherals[ADDRESS].written == [(U(0x0101), bytes.fromhex("00112233445566778899aabbccddeeff"), True)]
    assert "written, 16 B, at" in key_status(k1)
    assert "0011" not in key_status(k1)  # a key is not echoed
    assert row.node.state is State.CONNECTED  # Enter in the field did not disconnect


async def test_write_a_key_with_the_button(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    psk = key_field(row, "PSK")

    await type_into(pilot, psk, "33" + "00" * 15)
    await click(pilot, psk.query_one(Button))
    await wait_for(pilot, lambda: "written" in key_status(psk))

    assert bench.peripherals[ADDRESS].written == [(U(0x0102), bytes.fromhex("33" + "00" * 15), True)]


async def test_incomplete_key_is_not_sent(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    psk = key_field(row, "PSK")

    await type_into(pilot, psk, "0011")
    await pilot.press("enter")
    await pilot.pause(0.2)

    assert key_status(psk) == "4 of 32 hex digits"
    assert bench.peripherals[ADDRESS].written == []


async def test_key_write_refused_by_the_node(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.peripherals[ADDRESS].write_errors[gatt.K1] = LinkLost("ATT error 0x0e (Unlikely Error)")
    row = await connect(pilot, app, bench)
    k1 = key_field(row, "K1")

    await type_into(pilot, k1, "11" + "00" * 15)
    await pilot.press("enter")
    await wait_for(pilot, lambda: "failed" in key_status(k1))

    assert key_status(k1) == "write failed: ATT error 0x0e (Unlikely Error)"


async def test_switch_writes_active_and_reads_it_back(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    switch = row.query_one(Switch)

    await click(pilot, switch)
    await wait_for(pilot, lambda: "node: off (0x00)" in values_text(row))

    assert bench.peripherals[ADDRESS].written == [(gatt.ACTIVE, b"\x00", True)]
    assert "written off at" in values_text(row)
    assert switch.value is False
    assert not switch.disabled


async def test_switch_follows_the_node(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    row = await connect(pilot, app, bench)
    bench.peripherals[ADDRESS].values[gatt.ACTIVE] = b"\x00"  # changed by someone else

    await click(pilot, panel(row, "embeNET Provisioning").query_one(".refresh", Button))
    await wait_for(pilot, lambda: row.query_one(Switch).value is False)

    assert bench.peripherals[ADDRESS].written == []  # following the node is not a write


async def test_switch_write_refused_puts_the_switch_back(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    bench.peripherals[ADDRESS].write_errors[gatt.ACTIVE] = LinkLost("ATT error 0x13 (Value Not Allowed)")
    row = await connect(pilot, app, bench)
    switch = row.query_one(Switch)

    await click(pilot, switch)
    await wait_for(pilot, lambda: "write failed" in values_text(row))
    await wait_for(pilot, lambda: switch.value is True)

    assert "write failed: ATT error 0x13 (Value Not Allowed)" in values_text(row)


async def test_firmware_without_provisioning_controls(app: CentralApp, bench: FakeBench, pilot: Pilot[None]) -> None:
    from fakes import characteristic

    provisioning = bench.peripherals[ADDRESS].services[1]
    provisioning.characteristics = [characteristic("12345679-0000-1000-8000-00805f9b34fb", 37, "read")]
    bench.peripherals[ADDRESS].values["12345679-0000-1000-8000-00805f9b34fb"] = b"\x07"

    row = await connect(pilot, app, bench)

    panel_ = row.query_one(ProvisioningPanel)
    assert list(panel_.query(KeyField)) == []
    assert list(panel_.query(Switch)) == []
    assert panel_.query_one(".panel-body", Static).display
    assert "07" in plain(panel_.query_one(".panel-body", Static).content)


# --- Pieces ------------------------------------------------------------------


def test_format_age() -> None:
    assert ble_central.format_age(0.4) == "0 s"
    assert ble_central.format_age(119.9) == "119 s"
    assert ble_central.format_age(120) == "2 min"


def test_render_note() -> None:
    node = Node(SimpleNamespace(address=ADDRESS), "embeNET nRF54", -40, time.monotonic())  # type: ignore[arg-type]
    assert ble_central.render_note(node) == ""

    node.lost_reason = "this computer closed the connection"
    assert ble_central.render_note(node) == ""  # ended on request

    node.note = "connection lost at 16:28:15"
    assert plain(ble_central.render_note(node)) == "connection lost at 16:28:15\nreason: this computer closed the connection"


def test_a_row_not_on_screen_is_left_alone() -> None:
    row = NodeRow(Node(SimpleNamespace(address=ADDRESS), "embeNET nRF54", -40, time.monotonic()), stale_s=10)  # type: ignore[arg-type]

    row.update_from_node()  # not mounted: nothing to update, and no error

    assert row.border_title is None


def test_arguments() -> None:
    defaults = ble_central.parse_arguments([])
    assert (defaults.adapter, defaults.stale, defaults.forget) == (None, 10.0, 30.0)
    given = ble_central.parse_arguments(["--adapter", "hci1", "--stale", "30", "--forget", "60"])
    assert (given.adapter, given.stale, given.forget) == ("hci1", 30.0, 60.0)


@pytest.mark.parametrize("option", ["--stale", "--forget"])
@pytest.mark.parametrize("value", ["0", "-1", "soon"])
def test_arguments_refuse_what_is_not_a_period(option: str, value: str, capsys: pytest.CaptureFixture[str]) -> None:
    with pytest.raises(SystemExit):
        ble_central.parse_arguments([option, value])
    assert option in capsys.readouterr().err


def test_main_runs_the_app_with_the_arguments(monkeypatch: pytest.MonkeyPatch) -> None:
    started: list[CentralApp] = []
    monkeypatch.setattr(CentralApp, "run", lambda self: started.append(self))

    ble_central.main(["--adapter", "hci1", "--stale", "20", "--forget", "45"])

    (app,) = started
    assert (app.adapter, app.stale_s, app.forget_s) == ("hci1", 20.0, 45.0)
