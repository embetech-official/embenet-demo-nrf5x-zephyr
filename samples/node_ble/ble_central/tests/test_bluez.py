"""The reasons BlueZ gives for a lost connection, against a fake system bus."""

from __future__ import annotations

from types import SimpleNamespace
from typing import Any

from central import bluez


def test_address_of_a_device_path() -> None:
    assert bluez.address_of("/org/bluez/hci0/dev_CC_24_B5_E4_06_BA") == "CC:24:B5:E4:06:BA"
    assert bluez.address_of("/org/bluez/hci0") is None


def test_reasons_in_words() -> None:
    assert bluez.reason_text("org.bluez.Reason.Timeout", "Connection timeout") == "link lost: nothing heard from the node within the supervision timeout"
    assert bluez.reason_text("org.bluez.Reason.Remote", "") == "the node closed the connection"
    assert bluez.reason_text("org.bluez.Reason.Future", "Something new") == "Something new"  # one BlueZ adds later
    assert bluez.reason_text("org.bluez.Reason.Future", "") == "org.bluez.Reason.Future"


class FakeBus:
    def __init__(self) -> None:
        self.handlers: list[Any] = []
        self.calls: list[Any] = []
        self.connected = True

    def add_message_handler(self, handler: Any) -> None:
        self.handlers.append(handler)

    async def call(self, message: Any) -> None:
        self.calls.append(message)

    def disconnect(self) -> None:
        self.connected = False

    def send(self, interface: str, member: str, path: str, *body: str) -> None:
        for handler in self.handlers:
            handler(SimpleNamespace(interface=interface, member=member, path=path, body=list(body)))


async def test_hands_on_every_disconnected_signal_and_nothing_else() -> None:
    bus = FakeBus()
    heard: list[tuple[str, str]] = []

    async def connect() -> FakeBus:
        return bus

    reasons = bluez.DisconnectReasons(lambda address, reason: heard.append((address, reason)), connect)
    await reasons.start()

    (match,) = bus.calls
    assert (match.member, match.body) == ("AddMatch", [bluez.MATCH_RULE])
    bus.send("org.bluez.Device1", "Disconnected", "/org/bluez/hci0/dev_CC_24_B5_E4_06_BA", "org.bluez.Reason.Timeout", "Connection timeout")
    bus.send("org.freedesktop.DBus.Properties", "PropertiesChanged", "/org/bluez/hci0/dev_CC_24_B5_E4_06_BA")
    bus.send("org.bluez.Device1", "Disconnected", "/org/bluez/hci0", "org.bluez.Reason.Local", "")
    bus.send("org.bluez.Device1", "Disconnected", "/org/bluez/hci0/dev_CC_24_B5_E4_06_BA")  # no body

    assert heard == [("CC:24:B5:E4:06:BA", bluez.REASONS["org.bluez.Reason.Timeout"])]
    reasons.stop()
    assert not bus.connected
    reasons.stop()  # stopping twice is harmless
