
#include <stdint.h>
#include <stdio.h>

#include "metadata_crc.h"

static const uint8_t seeded_header[METADATA_CRC_INPUT_SIZE] =
{
    0x41U, 0x54U, 0x45U, 0x4DU,
    0x01U, 0x00U, 0xA1U, 0x53U
};

static const uint8_t complete_header[METADATA_CRC_INPUT_SIZE] =
{
    0x41U, 0x54U, 0x45U, 0x4DU,
    0x01U, 0x00U, 0x4EU, 0xC0U
};

static int CheckCRC(
    const char *name,
    uint32_t actual,
    uint32_t expected)
{
    if (actual != expected)
    {
        printf(
            "FAIL: %s: expected 0x%08lX, got 0x%08lX\n",
            name,
            (unsigned long)expected,
            (unsigned long)actual);
        return 1;
    }

    printf("PASS: %s: 0x%08lX\n",
           name,
           (unsigned long)actual);
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += CheckCRC(
        "SEEDED",
        MetadataCRC_CalculateHeader(seeded_header),
        0x0145B905U);

    failures += CheckCRC(
        "COMPLETE",
        MetadataCRC_CalculateHeader(complete_header),
        0x9A66CAD8U);

    failures += CheckCRC(
        "NULL input",
        MetadataCRC_CalculateHeader(NULL),
        0U);

    if (failures != 0)
    {
        printf("%d CRC test(s) failed.\n", failures);
        return 1;
    }

    printf("All CRC tests passed.\n");
    return 0;
}
