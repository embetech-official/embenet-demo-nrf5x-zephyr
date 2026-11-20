"""Why a connection ended, as BlueZ tells it.

bleak only reports that a connection ended. BlueZ says why in the
org.bluez.Device1 ``Disconnected`` signal (recent BlueZ; 5.85 on the bench), carrying the
reason the kernel took from the controller's Disconnection Complete event, such
as ``org.bluez.Reason.Timeout`` for a supervision timeout. :class:`DisconnectReasons`
listens for that signal on the system bus, and hands each one on in words.
"""

from __future__ import annotations

from collections.abc import Awaitable, Callable
from typing import Any

#: The reasons BlueZ gives, in words.
REASONS: dict[str, str] = {
    "org.bluez.Reason.Timeout": "link lost: nothing heard from the node within the supervision timeout",
    "org.bluez.Reason.Remote": "the node closed the connection",
    "org.bluez.Reason.Local": "this computer closed the connection",
    "org.bluez.Reason.Authentication": "authentication failed",
    "org.bluez.Reason.Suspend": "this computer went to sleep",
    "org.bluez.Reason.Unknown": "no reason given",
}

#: What the station listens for.
MATCH_RULE = "type='signal',sender='org.bluez',interface='org.bluez.Device1',member='Disconnected'"

#: Called with the node's address and the reason, in words.
ReasonCallback = Callable[[str, str], None]


def reason_text(reason: str, message: str) -> str:
    """A reason for the screen: ours for the reasons we know, BlueZ's message for any other."""
    return REASONS.get(reason) or message or reason


def address_of(path: str) -> str | None:
    """The address of the device at D-Bus object path `path` (``/org/bluez/hci0/dev_CC_24_...``)."""
    last = path.rsplit("/", 1)[-1]
    return last[4:].replace("_", ":") if last.startswith("dev_") else None


async def _system_bus() -> Any:
    from dbus_fast import BusType
    from dbus_fast.aio import MessageBus

    return await MessageBus(bus_type=BusType.SYSTEM).connect()


class DisconnectReasons:
    """Hands on every ``Disconnected`` signal BlueZ sends, as an address and a reason."""

    def __init__(self, callback: ReasonCallback, bus_factory: Callable[[], Awaitable[Any]] = _system_bus) -> None:
        self.callback = callback
        self.bus_factory = bus_factory
        self.bus: Any = None

    async def start(self) -> None:
        """Connects to the system bus and subscribes; raises when that fails (no D-Bus, no BlueZ)."""
        from dbus_fast import Message

        bus = await self.bus_factory()
        bus.add_message_handler(self._on_message)
        await bus.call(
            Message(
                destination="org.freedesktop.DBus",
                path="/org/freedesktop/DBus",
                interface="org.freedesktop.DBus",
                member="AddMatch",
                signature="s",
                body=[MATCH_RULE],
            )
        )
        self.bus = bus

    def stop(self) -> None:
        if self.bus is not None:
            self.bus.disconnect()
            self.bus = None

    def _on_message(self, message: Any) -> None:
        if message.interface != "org.bluez.Device1" or message.member != "Disconnected":
            return
        address = address_of(message.path or "")
        if address is None or len(message.body) < 2:
            return
        self.callback(address, reason_text(str(message.body[0]), str(message.body[1])))
