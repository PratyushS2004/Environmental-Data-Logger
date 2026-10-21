#ifndef METADATA_CRC_H
#define METADATA_CRC_H

#include <stdint.h>

#define METADATA_CRC_INPUT_SIZE 8U

/*
 * Calculates CRC-32/MPEG-2 over an eight-byte serialized header.
 *
 * The input bytes are assembled into two little-endian 32-bit words.
 * Each word is then processed MSB-first using the STM32-compatible
 * word-processing convention.
 *
 * Parameters:
 *   header: Pointer to eight serialized bytes, or NULL.
 *
 * Returns:
 *   CRC-32/MPEG-2 result. Returns 0 for NULL input.
 */
uint32_t MetadataCRC_CalculateHeader(
    const uint8_t header[METADATA_CRC_INPUT_SIZE]);

#endif /* METADATA_CRC_H */
