import struct

def crc32_mpeg2(data: bytes) -> int:
    crc = 0xFFFFFFFF
    polynomial = 0x04C11DB7

    for byte in data:
        # Mix the incoming byte into the top 8 bits.
        crc ^= byte << 24

        # Process each bit, most significant bit first.
        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ polynomial
            else:
                crc = crc << 1

            # Keep the register at 32 bits.
            crc &= 0xFFFFFFFF

    return crc

print("Standard check:", hex(crc32_mpeg2(b"123456789")))
assert crc32_mpeg2(b"123456789") == 0x0376E6E7

payload = struct.pack(
    "<IIhH",
    0x12345678,  # seq
    98765,       # pressure_pa
    -1234,       # temperature_centi_c
    4321,        # humidity_centi_pct
)

crc = crc32_mpeg2(payload)

print("Payload:", payload.hex(" "))
print(f"CRC-32/MPEG-2: 0x{crc:08X}")