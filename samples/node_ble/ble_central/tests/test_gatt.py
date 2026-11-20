"""Decoding the embeNET characteristics, and encoding what the user types. The raw values were read from a node on the bench."""

import re

import pytest

from central import gatt
from central.gatt import embenet_uuid as U


def test_uuids_match_the_firmware():
    assert gatt.PROVISIONING_SERVICE == "bab80100-e237-4bc2-80c7-3ac1568ef5a6"
    assert gatt.INFO_SERVICE == "bab80200-e237-4bc2-80c7-3ac1568ef5a6"


def test_eui64_is_shown_most_significant_first_like_the_node_logs_it():
    assert gatt.decode(U(0x0201), bytes.fromhex("765845fe2b9e67f4")) == "f4679e2bfe455876"


def test_stack_version_is_text():
    assert gatt.decode(U(0x0202), b"1.1.4+6b937e4-dirty") == "1.1.4+6b937e4-dirty"


def test_groups_are_little_endian_uint16():
    assert gatt.decode(U(0x0203), bytes.fromhex("64000100")) == "100, 1"
    assert gatt.decode(U(0x0203), b"") == "none"


def test_neighbor_info_is_one_record():
    assert gatt.decode(U(0x0206), bytes.fromhex("74f4ddd3067b2ebbc900")) == "bb2e7b06d3ddf474  -55 dBm  parent"
    assert gatt.decode(U(0x0206), bytes.fromhex("3e8c1e8d307f5510ec02")) == "10557f308d1e8c3e  -20 dBm  unrelated"


def test_neighbor_with_unknown_rssi():
    assert gatt.decode(U(0x0206), bytes(8) + bytes([127, 1])) == "0000000000000000      n/a  child"


def test_neighbor_select():
    assert U(0x0204) == gatt.NEIGHBORS_COUNT
    assert U(0x0205) == gatt.NEIGHBOR_SELECT
    assert U(0x0206) == gatt.NEIGHBOR_INFO
    assert gatt.neighbor_select(2) == bytes.fromhex("0200")
    assert gatt.neighbors_count(bytes.fromhex("0300")) == 3
    assert gatt.decode(U(0x0205), bytes.fromhex("0100")) == "1"


def test_parent():
    assert gatt.decode(U(0x0207), bytes.fromhex("74f4ddd3067b2ebb")) == "bb2e7b06d3ddf474"
    assert gatt.decode(U(0x0207), bytes(8)) == "none"
    assert gatt.decode(U(0x0208), bytes([0xC9])) == "-55 dBm"
    assert gatt.decode(U(0x0208), bytes([127])) == "n/a"


def test_counts_and_duty_cycle():
    assert gatt.decode(U(0x0204), bytes.fromhex("0300")) == "3"
    assert gatt.decode(U(0x0209), bytes.fromhex("c601")) == "4.54 %"


def test_active():
    assert gatt.decode(U(0x0103), b"\x01") == "on (0x01)"
    assert gatt.decode(U(0x0103), b"\x00") == "off (0x00)"


def test_wrong_length_is_shown_in_hex_and_marked():
    assert gatt.decode(U(0x0201), bytes.fromhex("0102")) == "0102  (malformed: 2 B, expected 8 B)"
    assert "malformed" in gatt.decode(U(0x0206), bytes(11))
    assert gatt.decode(U(0x0203), bytes(3)) == "000000  (malformed: 3 B, not a multiple of 2 B)"
    assert gatt.decode(U(0x0103), b"\x07") == "0x07?"
    assert gatt.decode(U(0x0103), b"") == "  (malformed: 0 B, expected 1 B)"


def test_unknown_characteristic_is_hex():
    assert gatt.decode("00002b2a-0000-1000-8000-00805f9b34fb", bytes.fromhex("c907")) == "c907"


def test_connection_parameters():
    raw = bytes.fromhex("c8af000000009001")  # 45 ms, latency 0, 4 s
    assert gatt.connection_parameters(raw) == gatt.ConnectionParameters(interval_us=45000, latency=0, timeout_ms=4000)
    assert gatt.decode(gatt.CONNECTION_PARAMETERS, bytes.fromhex("4c1d000004002a00")) == "7.5 ms, latency 4, timeout 420 ms"
    assert "malformed" in gatt.decode(gatt.CONNECTION_PARAMETERS, bytes(6))


def test_names():
    assert gatt.service_name(gatt.INFO_SERVICE.upper(), "Unknown") == "embeNET Info"
    assert gatt.service_name("00001800-0000-1000-8000-00805f9b34fb", "Generic Access Profile") == "Generic Access Profile"
    assert gatt.characteristic_name(U(0x0101), "Unknown") == "K1"


def test_is_embenet():
    assert gatt.is_embenet(["BAB80100-E237-4BC2-80C7-3AC1568EF5A6"])
    assert not gatt.is_embenet(["0000180f-0000-1000-8000-00805f9b34fb"])


def test_keys_are_encoded_from_32_hex_digits_in_typed_order_in_either_case():
    assert gatt.encode(gatt.K1, "00112233445566778899aabbccddeeff") == bytes(range(0, 256, 17))
    assert gatt.encode(gatt.PSK, "00112233445566778899AABBccDDeeFF") == bytes(range(0, 256, 17))


@pytest.mark.parametrize(
    ("text", "problem"),
    [
        ("", "0 of 32 hex digits"),
        ("0011", "4 of 32 hex digits"),
        ("0" * 31, "31 of 32 hex digits"),
        ("zz" * 16, "'z' is not a hex digit: use 0-9 and a-f"),
        ("00:11", "':' is not a hex digit: use 0-9 and a-f"),
        ("0x" + "00" * 15, "'x' is not a hex digit: use 0-9 and a-f"),
        ("00 11", "' ' is not a hex digit: use 0-9 and a-f"),
    ],
)
def test_key_problem_says_what_is_wrong(text: str, problem: str) -> None:
    assert gatt.key_problem(text) == problem
    with pytest.raises(ValueError, match=re.escape(problem)):
        gatt.encode(gatt.K1, text)


def test_a_complete_key_has_no_problem() -> None:
    assert gatt.key_problem("0123456789abcdefABCDEF0123456789") is None


def test_active_is_written_as_on_or_off() -> None:
    assert gatt.encode(gatt.ACTIVE, " ON ") == b"\x01"
    assert gatt.encode(gatt.ACTIVE, "0") == b"\x00"
    with pytest.raises(ValueError, match="on or off"):
        gatt.encode(gatt.ACTIVE, "maybe")
    assert gatt.is_on(gatt.decode(gatt.ACTIVE, b"\x01"))
    assert not gatt.is_on(gatt.decode(gatt.ACTIVE, b"\x00"))


def test_the_station_writes_only_the_provisioning_characteristics() -> None:
    with pytest.raises(ValueError, match="does not write"):
        gatt.encode(U(0x0201), "00")
    with pytest.raises(ValueError, match="does not write"):
        gatt.encode("0000aaaa-0000-1000-8000-00805f9b34fb", "00")
