#!/usr/bin/env python3
"""Generate C fixtures for MetadataJournal_ClassifyEntry host tests."""

import struct

MASK32 = 0xFFFFFFFF
POLY = 0x04C11DB7
INITIAL = 0xFFFFFFFF
MAGIC = 0x4D455441
SEEDED_STATE = 0x53A1
COMPLETE_STATE = 0xC04E
ERASED_TAIL = b"\xFF" * 4


def crc32_mpeg2_bytes(data: bytes) -> int:
    """MSB-first CRC-32/MPEG-2; no reflection or final XOR."""
    crc = INITIAL
    for byte in data:
        crc ^= byte << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ POLY) & MASK32
            else:
                crc = (crc << 1) & MASK32
    return crc


def crc32_mpeg2_words(words) -> int:
    """Emulate MSB-first processing of 32-bit words."""
    crc = INITIAL
    for word in words:
        word &= MASK32  # Match conversion to uint32_t.
        for shift in (24, 16, 8, 0):
            crc ^= ((word >> shift) & 0xFF) << 24
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ POLY) & MASK32
                else:
                    crc = (crc << 1) & MASK32
    return crc


def make_entry(word0: int, word1: int,
               tail: bytes = ERASED_TAIL) -> bytes:
    header = struct.pack("<II", word0 & MASK32, word1 & MASK32)
    crc = crc32_mpeg2_words((word0, word1))
    entry = header + struct.pack("<I", crc) + tail
    assert len(entry) == 16
    return entry


def c_array(name: str, data: bytes) -> str:
    assert len(data) == 16, (
        f"{name}: expected 16 bytes, got {len(data)}"
    )
    lines = [f"static const uint8_t {name}[16] = {{"]
    for start in range(0, 16, 8):
        chunk = data[start:start + 8]
        suffix = "," if start + 8 < 16 else ""
        values = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {values}{suffix}")
    lines.append("};")
    return "\n".join(lines)


def main() -> None:
    # Published independent check value.
    assert crc32_mpeg2_bytes(b"123456789") == 0x0376E6E7

    seeded = (0x4D455441, 0x53A10001)
    complete = (0x4D455441, 0xC04E0001)
    bad_state = (0x4D455441, 0x53A20001)
    bad_version = (0x4D455441, 0x53A10002)

    # Literal CRC anchors from the word-based reference.
    assert crc32_mpeg2_words(seeded) == 0x0145B905
    assert crc32_mpeg2_words(complete) == 0x9A66CAD8
    assert crc32_mpeg2_words(bad_state) == 0x032C4C8C
    assert crc32_mpeg2_words(bad_version) == 0x0C069FDC

    # Reverse bytes within each word, preserving word order.
    seeded_crc_input = bytes.fromhex(
        "4D 45 54 41 53 A1 00 01"
    )
    assert crc32_mpeg2_bytes(seeded_crc_input) == 0x0145B905
    assert crc32_mpeg2_bytes(seeded_crc_input) == (
        crc32_mpeg2_words(seeded)
    )

    seeded_entry = make_entry(*seeded)
    complete_entry = make_entry(*complete)

    corrupted = bytearray(seeded_entry)
    corrupted[8] ^= 0x01
    assert corrupted[:8] == seeded_entry[:8]
    assert corrupted[9:] == seeded_entry[9:]
    assert int.from_bytes(corrupted[8:12], "little") != (
        crc32_mpeg2_words(seeded)
    )
    assert (corrupted[8] ^ seeded_entry[8]).bit_count() == 1

    bad_state_entry = make_entry(*bad_state)
    bad_version_entry = make_entry(*bad_version)
    assert bad_state_entry[6] != seeded_entry[6]
    assert all(
        bad_state_entry[i] == seeded_entry[i]
        for i in range(8) if i != 6
    )
    assert bad_version_entry[4] != seeded_entry[4]
    assert all(
        bad_version_entry[i] == seeded_entry[i]
        for i in range(8) if i != 4
    )

    assert int.from_bytes(bad_state_entry[8:12], "little") == (
        0x032C4C8C
    )
    assert int.from_bytes(bad_version_entry[8:12], "little") == (
        0x0C069FDC
    )

    fixtures = [
        ("blank_entry", b"\xFF" * 16),
        ("valid_seeded", seeded_entry),
        ("valid_complete", complete_entry),
        ("bad_magic", make_entry(0x4D455440, 0x53A10001)),
        ("bad_state", bad_state_entry),
        ("bad_version", bad_version_entry),
        ("corrupted_crc", bytes(corrupted)),
        ("tail_non_erased", make_entry(
            *seeded, tail=bytes.fromhex("00 12 34 56")
        )),
        ("partial_blank_tail", b"\xFF" * 12 + b"\x00" + b"\xFF" * 3),
        ("torn_after_8", seeded_entry[:8] + b"\xFF" * 8),
    ]

    for name, data in fixtures:
        print(c_array(name, data))
        print()

    print("PASS: published check value 0x0376E6E7")
    print("PASS: SEEDED word CRC 0x0145B905")
    print("PASS: COMPLETE word CRC 0x9A66CAD8")
    print("PASS: BAD_STATE word CRC 0x032C4C8C")
    print("PASS: BAD_VERSION word CRC 0x0C069FDC")
    print("PASS: byte-wise SEEDED CRC agrees with word routine")
    print(f"PASS: generated {len(fixtures)} 16-byte fixtures")


if __name__ == "__main__":
    main()
