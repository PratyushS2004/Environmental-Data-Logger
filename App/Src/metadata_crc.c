#include "metadata_crc.h"
#include <stddef.h>

#define METADATA_CRC_POLYNOMIAL 0x04C11DB7U
#define METADATA_CRC_INITIAL    0xFFFFFFFFU
#define METADATA_CRC_WORD_BITS  32U

static uint32_t MetadataCRC_ReadU32LE(const uint8_t *data)
{
    return ((uint32_t)data[0])
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static uint32_t MetadataCRC_ProcessWord(
    uint32_t crc,
    uint32_t word)
{
    crc ^= word;

    for (uint32_t bit = 0U; bit < METADATA_CRC_WORD_BITS; ++bit)
    {
        if ((crc & 0x80000000U) != 0U)
        {
            crc = (crc << 1) ^ METADATA_CRC_POLYNOMIAL;
        }
        else
        {
            crc <<= 1;
        }
    }

    return crc;
}

uint32_t MetadataCRC_CalculateHeader(
    const uint8_t header[METADATA_CRC_INPUT_SIZE])
{
    if (header == NULL)
    {
        return 0U;
    }

    const uint32_t word1 = MetadataCRC_ReadU32LE(&header[0]);
    const uint32_t word2 = MetadataCRC_ReadU32LE(&header[4]);

    uint32_t crc = METADATA_CRC_INITIAL;

    crc = MetadataCRC_ProcessWord(crc, word1);
    crc = MetadataCRC_ProcessWord(crc, word2);

    return crc;
}
