"""Fixtures for the station's tests: a fake bench, the app wired to it, and ways to read the screen."""

from __future__ import annotations

import io
from collections.abc import AsyncIterator, Callable
from typing import Any

import pytest
from fakes import FakeBench
from rich.console import Console
from textual.content import Content
from textual.geometry import Region
from textual.pilot import Pilot
from textual.widget import Widget
from textual.widgets import Static

from ble_central import CentralApp, NodeRow, Panels, ServicePanel

ADDRESS = "CC:24:B5:E4:06:BA"


@pytest.fixture
def bench() -> FakeBench:
    """One node in range, at ADDRESS."""
    bench = FakeBench()
    bench.add(ADDRESS)
    return bench


@pytest.fixture
def app(bench: FakeBench) -> CentralApp:
    return make_app(bench)


def make_app(bench: FakeBench, **kwargs: Any) -> CentralApp:
    """The app, wired to `bench`."""
    return CentralApp(scanner_factory=bench.scanner_factory, client_factory=bench.client_factory, reasons_factory=bench.reasons_factory, **kwargs)


@pytest.fixture
async def pilot(app: CentralApp) -> AsyncIterator[Pilot[None]]:
    async with app.run_test(size=(160, 50)) as pilot:
        yield pilot


def rows(app: CentralApp) -> list[NodeRow]:
    return list(app.query(NodeRow))


def plain(content: object, width: int = 130) -> str:
    """What a Static shows, as plain text, whether it holds markup or a Rich renderable."""
    if isinstance(content, str):
        return Content.from_markup(content).plain
    console = Console(width=width, record=True, file=io.StringIO())
    console.print(content)
    return console.export_text()


def summary_text(row: NodeRow) -> str:
    return plain(row.query_one(".summary", Static).content)


def values_text(row: NodeRow) -> str:
    """Everything the row's service panels say, in order."""
    return "\n".join(plain(s.content) for s in row.query_one(Panels).query(Static) if s.display)


def note_text(row: NodeRow) -> str:
    """Why the row's node is not connected."""
    return plain(row.query_one(".note", Static).content).strip()


def panel(row: NodeRow, title: str) -> ServicePanel:
    (found,) = (p for p in row.query(ServicePanel) if p.title == title)
    return found


async def wait_for(pilot: Pilot[None], condition: Callable[[], object], within_s: float = 3.0) -> None:
    """Lets the app run until `condition` holds."""
    for _ in range(int(within_s / 0.02)):
        if condition():
            return
        await pilot.pause(0.02)
    raise AssertionError("condition not met in time")


async def settle(pilot: Pilot[None], within_s: float = 3.0) -> None:
    """Lets the app run until its layout stops changing.

    A row an advertisement adds is laid out on a later refresh, which on a slow machine can still be pending after a
    pause.
    """

    def layout() -> list[Region]:
        return [w.region for w in pilot.app.screen.walk_children()]

    before = layout()
    for _ in range(int(within_s / 0.02)):
        await pilot.pause()
        await pilot.pause(0.02)
        after = layout()
        if after == before:
            return
        before = after
    raise AssertionError("layout did not settle in time")


async def click(pilot: Pilot[None], widget: Widget) -> None:
    """Clicks `widget` once it is laid out and the layout has settled.

    Pilot.click() aims at the widget's region as it is at the call, and only reports a miss. A widget not laid out yet
    has an empty region at the screen's top left, so the click would land on the header instead.
    """
    await wait_for(pilot, lambda: widget.region.area > 0)
    await settle(pilot)
    assert await pilot.click(widget), f"the click missed {widget!r}"


async def connect(pilot: Pilot[None], app: CentralApp, bench: FakeBench, address: str = ADDRESS) -> NodeRow:
    """Advertises the node, presses Enter on it, and waits for its first complete reading."""
    bench.advertise(address, "embeNET nRF54", -40)
    await pilot.pause()
    row = app.rows[address]
    app.query_one("#nodes").index = rows(app).index(row)  # type: ignore[attr-defined]
    await pilot.press("enter")
    await wait_for(pilot, lambda: row.node.read_at)
    return row
