"""embeNET BLE central: a terminal UI that lists the embeNET nodes in range, reads their
GATT services, and writes their Provisioning service.

The node sample (`samples/node_ble`) advertises the
embeNET Provisioning service UUID and, in its scan response, its name. This
station scans actively, so it gets both, and lists every node that advertises
that UUID with its name, address and RSSI. A node not connected and not heard
from for 30 s (`--forget`) is removed from the list, until it advertises again.

Each node's row has a Connect button (Enter does the same on the highlighted
row); pressed again, it disconnects. While connected, the left side of the row
shows the connection's parameters, and the right side a panel for every service
the node serves, with every readable characteristic's value, decoded where the
layout is known (`central/gatt.py`). The services are read once on connecting,
and after that only on demand: a panel's Refresh button reads its service, `r`
all of them. The Provisioning panel has a field for K1 and
for PSK, and a switch for Active. A lost connection is reported on the row with
the time and BlueZ's reason, in words (`central/bluez.py`).

Bleak runs on Textual's own asyncio loop: advertisement callbacks and each
connection's worker update the widgets directly, with no threads involved.

Run it in place, from this directory::

    python ble_central.py [--adapter hci0] [--stale 10] [--forget 30]

See README.md for the setup.
"""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import time
from collections.abc import Callable
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from enum import Enum
from typing import Any, ClassVar, Protocol

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData
from rich.console import RenderableType
from rich.table import Table
from rich.text import Text
from textual import on, work
from textual.app import App, ComposeResult
from textual.binding import Binding, BindingType
from textual.containers import Horizontal, Vertical
from textual.css.query import NoMatches
from textual.events import Click
from textual.markup import escape
from textual.message import Message
from textual.validation import ValidationResult, Validator
from textual.widgets import Button, Footer, Header, Input, Label, ListItem, ListView, Static, Switch
from textual.worker import Worker

from central import gatt
from central.bluez import DisconnectReasons, ReasonCallback
from central.client import (
    NOT_READ_YET,
    CharacteristicValue,
    NodeReading,
    ServiceValues,
    error_text,
    find_characteristic,
    merged,
    read_node,
    service_name,
    shown_services,
    write,
)

#: How long a connection attempt may take, service discovery included. BlueZ
#: opens a connection with the parameters the node last asked for; at the
#: node sample's ~400 ms interval and latency 5, discovery alone takes 35 s or more.
CONNECT_TIMEOUT_S = 120.0

#: How long quitting waits for the nodes to be disconnected.
QUIT_TIMEOUT_S = 5.0

#: Makes the scanner and the connections; bleak's own classes but for the tests.
ScannerFactory = Callable[..., BleakScanner]
ClientFactory = Callable[..., BleakClient]


class ReasonSource(Protocol):
    """Where the reasons for lost connections come from: BlueZ, or the tests' fake."""

    async def start(self) -> None: ...

    def stop(self) -> None: ...


ReasonSourceFactory = Callable[[ReasonCallback], ReasonSource]


class State(Enum):
    IDLE = "not connected"
    #: Another node is being connected to: BlueZ makes one connection at a time.
    WAITING = "waiting to connect…"
    CONNECTING = "connecting…"
    CONNECTED = "connected"
    DISCONNECTING = "disconnecting…"


#: The mark in front of a node's name, and the color of its state.
STATE_LOOK: dict[State, tuple[str, str]] = {
    State.IDLE: ("○", "$text-muted"),
    State.WAITING: ("◌", "$warning"),
    State.CONNECTING: ("◌", "$warning"),
    State.CONNECTED: ("●", "$success"),
    State.DISCONNECTING: ("◌", "$warning"),
}

#: The label of a row's connect button, in each state.
CONNECT_LABEL: dict[State, str] = {
    State.IDLE: "Connect",
    State.WAITING: "Cancel",
    State.CONNECTING: "Cancel",
    State.CONNECTED: "Disconnect",
    State.DISCONNECTING: "Disconnecting…",
}


@dataclass(eq=False)
class Node:
    """One embeNET node: what its advertisements say, and the connection to it."""

    device: BLEDevice
    name: str
    rssi: int
    #: time.monotonic() of the last advertisement.
    last_seen: float
    state: State = State.IDLE
    #: Why the last connection failed or ended, when not on request.
    note: str = ""
    #: Why BlueZ says the last connection ended, in words.
    lost_reason: str = ""
    #: The last pass over the node's services, while connected; during the
    #: first pass, what it has read so far.
    reading: NodeReading | None = None
    #: When the last pass ended.
    read_at: datetime | None = None
    #: When each service, by UUID, was last read.
    service_read_at: dict[str, datetime] = field(default_factory=dict)
    #: Services whose Refresh button was pressed: the next pass reads only those.
    refresh_requested: set[str] = field(default_factory=set)
    #: The services the pass under way reads.
    reading_now: frozenset[str] = frozenset()
    client: BleakClient | None = None
    worker: Worker[None] | None = None
    #: Set to read the values now, or to end a session that waits for that.
    wake: asyncio.Event = field(default_factory=asyncio.Event)
    #: time.monotonic() when the last connection, or attempt, ended. BlueZ may forget the device
    #: then, and know it again only from its next advertisement.
    ended_at: float = -1.0
    #: Set by every advertisement.
    advertised: asyncio.Event = field(default_factory=asyncio.Event)

    @property
    def address(self) -> str:
        return self.device.address

    def request_refresh(self, uuid: str) -> None:
        """Reads service `uuid` again, as soon as the pass under way, if any, ends."""
        self.refresh_requested.add(uuid)
        self.wake.set()

    def service_values(self, uuid: str) -> ServiceValues | None:
        services = self.reading.services if self.reading else []
        return next((s for s in services if s.uuid == uuid), None)


def format_age(seconds: float) -> str:
    whole = int(seconds)
    return f"{whole} s" if whole < 120 else f"{whole // 60} min"


def render_summary(node: Node) -> str:
    """The left side of a node's row, as Textual markup."""
    _mark, color = STATE_LOOK[node.state]
    age = time.monotonic() - node.last_seen
    # A connected node does not advertise, so while connected the RSSI and the
    # time are those of the last advertisement before the connection.
    seen_at = (datetime.now() - timedelta(seconds=age)).strftime("%H:%M:%S")
    lines = [
        node.address,
        f"[$text-muted]RSSI[/]      {node.rssi} dBm",
        f"[$text-muted]last adv.[/] {seen_at} ({format_age(age)} ago)",
        f"[$text-muted]state[/]     [{color}]{node.state.value}[/]",
    ]
    if node.state is State.CONNECTED and node.reading is not None:
        read = f"{node.read_at:%H:%M:%S}" if node.read_at else "in progress…"
        lines.append(f"[$text-muted]read[/]      {read}")
        connection = node.reading.connection
        if isinstance(connection, gatt.ConnectionParameters):
            lines += [
                f"[$text-muted]interval[/]  {connection.interval_us / 1000:g} ms",
                f"[$text-muted]latency[/]   {connection.latency}",
                f"[$text-muted]timeout[/]   {connection.timeout_ms} ms",
            ]
        else:
            lines.append(f"[$text-muted]connection parameters: {escape(connection or NOT_READ_YET)}[/]")
    return "\n".join(lines)


def render_note(node: Node) -> str:
    """Why the last connection failed or ended, and BlueZ's reason; nothing if it ended on request."""
    if not node.note:
        return ""
    reason = f"\nreason: {escape(node.lost_reason)}" if node.lost_reason else ""
    return f"[$error]{escape(node.note)}{reason}[/]"


def render_values(characteristics: list[CharacteristicValue]) -> RenderableType:
    """A service's values, one characteristic a line."""
    table = Table(show_header=False, box=None, padding=(0, 1, 0, 0))
    table.add_column(style="dim", no_wrap=True)
    table.add_column()
    for characteristic in characteristics:
        table.add_row(characteristic.name, Text(characteristic.value, style="red" if characteristic.error else ""))
    return table


class ServicePanel(Vertical):
    """One service of a connected node: its name, when it was read, a Refresh button, and its values."""

    def __init__(self, node: Node, uuid: str, title: str) -> None:
        super().__init__(classes="panel")
        # As wide as its widest line: without this, Textual measures the panel's width as if it had none to spare.
        self.shrink = False
        self.node = node
        self.uuid = uuid
        self.title = title

    def compose(self) -> ComposeResult:
        with Horizontal(classes="panel-head"):
            yield Static(classes="panel-title")
            yield Button("Refresh", compact=True, classes="refresh")
        yield Static(classes="panel-body")

    def on_mount(self) -> None:
        self.call_after_refresh(self.show)

    @on(Button.Pressed, ".refresh")
    def _refresh(self, event: Button.Pressed) -> None:
        event.stop()
        self.node.request_refresh(self.uuid)
        self.show()

    def show(self) -> None:
        """Brings the panel up to date with its node."""
        node = self.node
        requested = self.uuid in node.refresh_requested
        if requested or self.uuid in node.reading_now:
            when = "[$warning]reading…[/]"
        elif self.uuid in node.service_read_at:
            when = f"[$text-muted]read {node.service_read_at[self.uuid]:%H:%M:%S}[/]"
        else:
            when = f"[$text-muted]{NOT_READ_YET}[/]"
        try:
            self.query_one(".panel-title", Static).update(f"[b]{escape(self.title)}[/]  {when}")
            self.query_one(".refresh", Button).disabled = requested
            self.show_values(node.service_values(self.uuid))
        except NoMatches:
            pass  # not composed yet, or being taken apart

    def show_values(self, values: ServiceValues | None) -> None:
        body = self.query_one(".panel-body", Static)
        body.update(render_values(values.characteristics) if values else "")


class KeyValidator(Validator):
    """K1 or PSK as typed: 32 hex digits, either case."""

    def validate(self, value: str) -> ValidationResult:
        problem = gatt.key_problem(value)
        return self.success() if problem is None else self.failure(problem)


#: What a key field says while it is empty.
KEY_HINT = f"{gatt.KEY_DIGITS} hex digits, either case"


class KeyField(Vertical):
    """K1 or PSK: the field, its Write button, and what is wrong with what is typed, or how the write went."""

    def __init__(self, node: Node, client: BleakClient, characteristic: BleakGATTCharacteristic, name: str) -> None:
        super().__init__(classes="key-field")
        self.node = node
        self.client = client
        self.characteristic = characteristic
        self.label = name

    def compose(self) -> ComposeResult:
        with Horizontal(classes="control"):
            yield Label(self.label, classes="control-name")
            yield Input(placeholder=KEY_HINT, max_length=gatt.KEY_DIGITS, validators=[KeyValidator()], compact=True)
            yield Button("Write", compact=True, disabled=True)
        yield Static(f"[$text-muted]{KEY_HINT}[/]", classes="key-status")

    def _status(self, markup: str) -> None:
        self.query_one(".key-status", Static).update(markup)

    @on(Input.Changed)
    def _changed(self, event: Input.Changed) -> None:
        event.stop()
        text = event.value
        problem = gatt.key_problem(text)
        self.query_one(Button).disabled = problem is not None
        if not text:
            self._status(f"[$text-muted]{KEY_HINT}[/]")
        elif problem is None:
            self._status("[$success]complete: Enter or Write writes it[/]")
        elif all(c in "0123456789abcdefABCDEF" for c in text):
            self._status(f"[$text-muted]{escape(problem)}[/]")
        else:
            self._status(f"[$error]{escape(problem)}[/]")

    @on(Input.Submitted)
    @on(Button.Pressed)
    def _requested(self, event: Input.Submitted | Button.Pressed) -> None:
        event.stop()
        if gatt.key_problem(self.query_one(Input).value) is None:
            self.send()

    @work(group="write")
    async def send(self) -> None:
        """Writes the key in the field, and says how it went under it."""
        self._status("[$warning]writing…[/]")
        try:
            size = await write(self.client, self.characteristic, self.query_one(Input).value)
        except Exception as error:
            self._status(f"[$error]write failed: {escape(error_text(error))}[/]")
            return
        # The value itself is not echoed: it is a key.
        self._status(f"[$success]written, {size} B, at {datetime.now():%H:%M:%S}[/]")


class ProvisioningPanel(ServicePanel):
    """The Provisioning service: a switch for Active, and a field each for K1 and PSK.

    Its body shows only what the controls do not: nothing, for the firmware as it is.
    """

    #: The characteristics the controls stand for, by name.
    CONTROLLED = frozenset(gatt.CHARACTERISTICS[uuid].name for uuid in (gatt.K1, gatt.PSK, gatt.ACTIVE))

    def __init__(self, node: Node, uuid: str, title: str, client: BleakClient) -> None:
        super().__init__(node, uuid, title)
        self.client = client
        self.active = find_characteristic(client, gatt.ACTIVE)
        self.keys = [(gatt.CHARACTERISTICS[uuid].name, c) for uuid in (gatt.K1, gatt.PSK) if (c := find_characteristic(client, uuid)) is not None]
        #: How the last write of Active went, shown next to the node's value.
        self.active_written = ""
        self.writing_active = False

    def compose(self) -> ComposeResult:
        yield from super().compose()
        if self.active is not None:
            with Horizontal(classes="control"):
                yield Label("Active", classes="control-name")
                yield Switch(animate=False)
                yield Static(classes="active-status")
        for name, characteristic in self.keys:
            yield KeyField(self.node, self.client, characteristic, name)

    def show_values(self, values: ServiceValues | None) -> None:
        characteristics = values.characteristics if values else []
        rest = [c for c in characteristics if c.name not in self.CONTROLLED]
        body = self.query_one(".panel-body", Static)
        body.display = bool(rest)
        body.update(render_values(rest))
        if self.active is None:
            return
        now = next((c for c in characteristics if c.name == gatt.CHARACTERISTICS[gatt.ACTIVE].name), None)
        node_says = f"[$error]{escape(now.value)}[/]" if now and now.error else escape(now.value) if now else NOT_READ_YET
        written = f"  {self.active_written}" if self.active_written else ""
        self.query_one(".active-status", Static).update(f"[$text-muted]node:[/] {node_says}{written}")
        if now is not None and not now.error and not self.writing_active:
            switch = self.query_one(Switch)
            with switch.prevent(Switch.Changed):
                switch.value = gatt.is_on(now.value)

    @on(Switch.Changed)
    def _switched(self, event: Switch.Changed) -> None:
        event.stop()
        self.write_active(event.value)

    @work(group="write")
    async def write_active(self, on: bool) -> None:
        """Writes Active, then reads the service back, which also puts the switch right if the write failed."""
        assert self.active is not None
        switch = self.query_one(Switch)
        self.writing_active, switch.disabled = True, True
        self.active_written = "[$warning]writing…[/]"
        self.show()
        try:
            await write(self.client, self.active, "on" if on else "off")
            self.active_written = f"[$success]written {'on' if on else 'off'} at {datetime.now():%H:%M:%S}[/]"
        except Exception as error:
            self.active_written = f"[$error]write failed: {escape(error_text(error))}[/]"
        finally:
            self.writing_active, switch.disabled = False, False
        self.node.request_refresh(self.uuid)
        self.show()


class Panels(Horizontal):
    """The service panels of a connected node."""

    def on_click(self, event: Click) -> None:
        # Not on to the row: there it would select the row, and the list would take the focus from the field clicked.
        event.stop()


class NodeRow(ListItem):
    """A node: who it is, how it is connected and its connect button on the left; its services, or why it is not connected, on the right."""

    class Toggle(Message):
        """Connect to the row's node, or disconnect from it."""

        def __init__(self, row: NodeRow) -> None:
            super().__init__()
            self.row = row

    def __init__(self, node: Node, stale_s: float) -> None:
        super().__init__()
        self.node = node
        self.stale_s = stale_s

    def compose(self) -> ComposeResult:
        with Horizontal():
            with Vertical(classes="left"):
                yield Static(classes="summary")
                yield Button(CONNECT_LABEL[State.IDLE], compact=True, classes="connect")
            yield Static(classes="note")
            yield Panels(classes="panels")

    def on_mount(self) -> None:
        # Not straight away: in on_mount the children are not ready to be updated yet.
        self.call_after_refresh(self.update_from_node)

    def on_descendant_focus(self) -> None:
        """Moves the list's highlight here when a button or field in this row takes the focus, so r reads this node."""
        nodes = self.parent
        if isinstance(nodes, ListView):
            nodes.index = list(nodes.children).index(self)

    @on(Button.Pressed, ".connect")
    def _connect_pressed(self, event: Button.Pressed) -> None:
        event.stop()
        self.post_message(self.Toggle(self))

    async def show_services(self, client: BleakClient) -> None:
        """A panel for every service the node serves, Provisioning with its controls."""
        panels = self.query_one(Panels)
        await panels.remove_children()
        await panels.mount_all(
            ProvisioningPanel(self.node, uuid, service_name(s), client)
            if (uuid := s.uuid.lower()) == gatt.PROVISIONING_SERVICE
            else ServicePanel(self.node, uuid, service_name(s))
            for s in shown_services(client)
        )

    async def clear_services(self) -> None:
        with contextlib.suppress(NoMatches):  # the app is closing, and the row is gone already
            await self.query_one(Panels).remove_children()

    def update_from_node(self, *, with_values: bool = True) -> None:
        """Brings the row up to date with its node; the values only if `with_values`, as rendering them takes time."""
        if not self.is_mounted:
            return  # not added yet
        try:
            summary, note = self.query_one(".summary", Static), self.query_one(".note", Static)
            button, panels = self.query_one(".connect", Button), self.query_one(Panels)
        except NoMatches:
            return  # the app is closing: a connection ending now finds its row already taken apart
        node = self.node
        connected = node.state is State.CONNECTED
        stale = node.state is State.IDLE and time.monotonic() - node.last_seen > self.stale_s
        self.set_class(connected, "connected")
        self.set_class(node.state in (State.WAITING, State.CONNECTING, State.DISCONNECTING), "busy")
        self.set_class(stale, "stale")
        mark, color = STATE_LOOK[node.state]
        self.border_title = f"[{color}]{mark}[/] [b]{escape(node.name)}[/]"
        summary.update(render_summary(node))
        button.label = CONNECT_LABEL[node.state]
        button.disabled = node.state is State.DISCONNECTING
        note.display, panels.display = not connected, connected
        note.update(render_note(node))
        if with_values:
            for panel in panels.query(ServicePanel):
                panel.show()


class NodeList(ListView):
    BINDINGS: ClassVar[list[BindingType]] = [Binding("enter", "toggle_connection", "Connect / disconnect")]

    def action_toggle_connection(self) -> None:
        row = self.highlighted_child
        if isinstance(row, NodeRow):
            self.post_message(NodeRow.Toggle(row))


def _plural(count: int, noun: str) -> str:
    return f"{count} {noun}" if count == 1 else f"{count} {noun}s"


class CentralApp(App[None]):
    CSS_PATH = "ble_central.tcss"
    TITLE = "embeNET BLE central"
    BINDINGS: ClassVar[list[BindingType]] = [
        Binding("r", "read_now", "Read now"),
        Binding("q", "quit", "Quit"),
    ]

    def __init__(
        self,
        adapter: str | None = None,
        stale_s: float = 10.0,
        forget_s: float = 30.0,
        scanner_factory: ScannerFactory = BleakScanner,
        client_factory: ClientFactory = BleakClient,
        reasons_factory: ReasonSourceFactory = DisconnectReasons,
    ) -> None:
        super().__init__()
        self.adapter = adapter
        self.stale_s = stale_s
        #: Seconds without an advertisement after which a node not connected is removed from the list.
        self.forget_s = forget_s
        self.scanner_factory = scanner_factory
        self.client_factory = client_factory
        self.reasons_factory = reasons_factory
        self.scanner: BleakScanner | None = None
        self.scan_error = ""
        self.reasons: ReasonSource | None = None
        #: Held by the connection attempt under way.
        self.connecting = asyncio.Lock()
        #: Every node listed, by address, in the order they were found.
        self.rows: dict[str, NodeRow] = {}

    def compose(self) -> ComposeResult:
        yield Header()
        yield Static(
            "Nodes advertising the embeNET Provisioning service, found by active scanning. "
            "Connect (or Enter on the highlighted node) connects; again, it disconnects. "
            "Refresh reads a service again at once.",
            id="hint",
        )
        yield NodeList(id="nodes")
        yield Static(id="status")
        yield Footer()

    async def on_mount(self) -> None:
        bluez: dict[str, Any] = {"filters": {"DuplicateData": True}}  # every advertisement, so "last adv." stays true
        if self.adapter:
            bluez["adapter"] = self.adapter
        self.scanner = self.scanner_factory(self._on_advertisement, scanning_mode="active", bluez=bluez)
        try:
            await self.scanner.start()
        except Exception as error:  # no adapter, adapter off, BlueZ not running...
            self.scan_error = error_text(error)
        reasons = self.reasons_factory(self._on_disconnect_reason)
        try:
            await reasons.start()
            self.reasons = reasons
        except Exception:  # no system bus: the rows say a connection was lost, but not why
            self.reasons = None
        self.set_interval(1.0, self._tick)
        self._show_status()
        self.query_one(NodeList).focus()

    def _on_advertisement(self, device: BLEDevice, advertisement: AdvertisementData) -> None:
        if not gatt.is_embenet(advertisement.service_uuids):
            return
        name = advertisement.local_name or device.name
        row = self.rows.get(device.address)
        if row is None:
            row = NodeRow(Node(device, name or "?", advertisement.rssi, time.monotonic()), self.stale_s)
            self.rows[device.address] = row
            nodes = self.query_one(NodeList)
            nodes.append(row)
            if nodes.index is None:
                nodes.index = 0  # the list starts empty, and Enter needs a highlighted row
            self._show_status()
            return
        node = row.node
        node.device, node.rssi, node.last_seen = device, advertisement.rssi, time.monotonic()
        node.advertised.set()
        node.name = name or node.name
        row.update_from_node(with_values=False)

    def _on_disconnect_reason(self, address: str, reason: str) -> None:
        row = self.rows.get(address.upper())
        # The first reason after a connection is the one: BlueZ can send more for a device it no longer has a connection to.
        if row is not None and not row.node.lost_reason:
            row.node.lost_reason = reason
            row.update_from_node(with_values=False)

    @on(NodeRow.Toggle)
    def _toggle(self, event: NodeRow.Toggle) -> None:
        row = event.row
        node = row.node
        if node.state is State.IDLE:
            node.note = ""
            node.worker = self.run_worker(self._session(row), group="session")
        elif node.state in (State.WAITING, State.CONNECTING, State.CONNECTED) and node.worker is not None:
            node.state = State.DISCONNECTING
            node.wake.set()  # ends the session even if a read under way takes the cancellation for its own failure
            node.worker.cancel()
        self._changed(row)

    async def _session(self, row: NodeRow) -> None:
        """One connection: connect and read every service, then read again only when asked, until cancelled or lost."""
        node = row.node
        lost = asyncio.Event()

        def on_disconnected(_client: BleakClient) -> None:
            lost.set()
            node.wake.set()

        def show_so_far(reading: NodeReading) -> None:
            if not lost.is_set():
                node.reading = reading
                row.update_from_node()

        node.state, node.lost_reason = State.WAITING, ""
        self._changed(row)
        client: BleakClient | None = None
        try:
            # After a connection, or a cancelled attempt, BlueZ may have dropped the device: connecting to it
            # fails with "not found" until the node's next advertisement makes BlueZ add it again.
            while node.last_seen <= node.ended_at:
                node.advertised.clear()
                await node.advertised.wait()
            client = self.client_factory(node.device, timeout=CONNECT_TIMEOUT_S, disconnected_callback=on_disconnected)
            # One attempt at a time, each with its whole timeout: the controller makes one connection at
            # a time anyway, and the attempts would otherwise share, and run out of, their time.
            async with self.connecting:
                node.state = State.CONNECTING
                self._changed(row)
                await client.connect()
            node.client, node.state = client, State.CONNECTED
            await row.show_services(client)
            self._changed(row)
            while not lost.is_set() and node.state is State.CONNECTED:
                node.wake.clear()
                only, node.refresh_requested = node.refresh_requested, set()
                node.reading_now = frozenset(only) if only else frozenset(s.uuid.lower() for s in shown_services(client))
                row.update_from_node()
                # A pass can take many seconds: the first one shows each value as it
                # arrives, the later ones replace the values only once complete.
                reading = await read_node(client, None if node.reading else show_so_far, only or None)
                node.reading_now = frozenset()
                if lost.is_set():
                    break  # the reads failed for that reason: no point showing them
                node.reading = merged(node.reading, reading) if only else reading
                node.read_at = datetime.now()
                node.service_read_at.update(dict.fromkeys((s.uuid for s in reading.services), node.read_at))
                row.update_from_node()
                if not node.refresh_requested:
                    await node.wake.wait()  # a Refresh button, r, a write of Active, or the connection lost
            if node.state is State.CONNECTED:  # not on request
                node.note = f"connection lost at {datetime.now():%H:%M:%S}"
        except Exception as error:
            if node.state is not State.DISCONNECTING:
                what = "connection failed" if node.state is State.CONNECTING else "connection lost"
                node.note = f"{what} at {datetime.now():%H:%M:%S}: {error_text(error)}"
        finally:
            # Also on cancellation, and also while connecting: that cancels the attempt in BlueZ.
            if client is not None:
                with contextlib.suppress(Exception):
                    await client.disconnect()
                node.ended_at = time.monotonic()
            node.client, node.worker, node.reading, node.read_at, node.state = None, None, None, None, State.IDLE
            node.service_read_at, node.refresh_requested, node.reading_now = {}, set(), frozenset()
            await row.clear_services()
            self._changed(row)

    def _changed(self, row: NodeRow) -> None:
        row.update_from_node()
        self._show_status()

    def _tick(self) -> None:
        self._forget_gone()
        for row in self.rows.values():
            row.update_from_node(with_values=False)

    def _forget_gone(self) -> None:
        """Removes the rows of nodes not connected and not heard from for `forget_s`; a node advertising again gets a new row."""
        now = time.monotonic()
        gone = [a for a, row in self.rows.items() if row.node.state is State.IDLE and now - row.node.last_seen > self.forget_s]
        if not gone:
            return
        nodes = self.query_one(NodeList)
        children = list(nodes.children)
        nodes.remove_items([children.index(self.rows.pop(address)) for address in gone])
        self._show_status()

    def _show_status(self) -> None:
        scan = f"[$error]scan failed: {escape(self.scan_error)}[/]" if self.scan_error else "active scan"
        connected = sum(row.node.state is State.CONNECTED for row in self.rows.values())
        status = f"{self.adapter or 'default adapter'} · {scan} · {_plural(len(self.rows), 'node')} · {connected} connected"
        if self.reasons is None:
            status += " · [$warning]no disconnect reasons: system bus unavailable[/]"
        with contextlib.suppress(NoMatches):  # the app is closing, and the status bar is gone already
            self.query_one("#status", Static).update(status)

    def _highlighted(self) -> Node | None:
        row = self.query_one(NodeList).highlighted_child
        return row.node if isinstance(row, NodeRow) else None

    def action_read_now(self) -> None:
        node = self._highlighted()
        if node is not None and node.state is State.CONNECTED:
            node.wake.set()

    async def action_quit(self) -> None:
        """Stops scanning and disconnects every node before leaving.

        BlueZ keeps a connection up after the process that made it is gone, and a
        connected node does not advertise, so leaving without disconnecting would
        hide the node from the next run.
        """
        if self.scanner is not None:
            with contextlib.suppress(Exception):
                await self.scanner.stop()
        if self.reasons is not None:
            with contextlib.suppress(Exception):
                self.reasons.stop()
        # Every session, connecting ones included, ends its connection itself when cancelled. Leaving it to
        # the app's exit would cancel a connection attempt twice: bleak's own clean-up, which closes the
        # half-made connection, would be cut short, leaving the connection up and a broken-pipe traceback.
        workers = [row.node.worker for row in self.rows.values() if row.node.worker is not None]
        for worker in workers:
            worker.cancel()
        with contextlib.suppress(TimeoutError):
            await asyncio.wait_for(asyncio.gather(*(w.wait() for w in workers), return_exceptions=True), QUIT_TIMEOUT_S)
        self.exit()


def _positive_seconds(text: str) -> float:
    try:
        value = float(text)
    except ValueError:
        raise argparse.ArgumentTypeError(f"not a number of seconds: {text!r}") from None
    if value <= 0:
        raise argparse.ArgumentTypeError(f"must be more than 0 s: {text!r}")
    return value


def parse_arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="List embeNET nodes over Bluetooth LE, read their GATT services and write them.")
    parser.add_argument("--adapter", help="Bluetooth adapter, e.g. hci0 (default: the system's default)")
    parser.add_argument("--stale", type=_positive_seconds, default=10.0, help="seconds without an advertisement before a node is dimmed (default: 10)")
    parser.add_argument(
        "--forget", type=_positive_seconds, default=30.0, help="seconds without an advertisement before a node not connected is removed (default: 30)"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> None:
    arguments = parse_arguments(argv)
    CentralApp(adapter=arguments.adapter, stale_s=arguments.stale, forget_s=arguments.forget).run()


if __name__ == "__main__":
    main()
