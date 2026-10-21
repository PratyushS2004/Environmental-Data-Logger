#include "metadata_journal.h"
#include "metadata_crc.h"

#include <stddef.h>
#include <string.h>

#define METADATA_TAIL_OFFSET_BYTES 12U

static uint32_t MetadataJournal_ReadU32LE(const uint8_t *data)
{
    return ((uint32_t)data[0])
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static uint16_t MetadataJournal_ReadU16LE(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0])
         | ((uint16_t)data[1] << 8));
}

MetadataEntryClassification MetadataJournal_ClassifyEntry(
    const uint8_t entry[METADATA_ENTRY_SIZE_BYTES],
    MetadataState *state_out)
{
    if (state_out != NULL)
    {
        *state_out = METADATA_STATE_NONE;
    }

    if (entry == NULL || state_out == NULL)
    {
        return METADATA_ENTRY_INVALID;
    }

    bool all_erased = true;

    for (uint32_t i = 0U; i < METADATA_ENTRY_SIZE_BYTES; ++i)
    {
        if (entry[i] != 0xFFU)
        {
            all_erased = false;
            break;
        }
    }

    if (all_erased)
    {
        return METADATA_ENTRY_BLANK;
    }

    const uint32_t magic = MetadataJournal_ReadU32LE(&entry[0]);
    const uint16_t version = MetadataJournal_ReadU16LE(&entry[4]);
    const uint16_t raw_state = MetadataJournal_ReadU16LE(&entry[6]);
    const uint32_t stored_crc = MetadataJournal_ReadU32LE(&entry[8]);

    if (magic != METADATA_MAGIC || version != METADATA_VERSION)
    {
        return METADATA_ENTRY_INVALID;
    }

    MetadataState decoded_state;

    if (raw_state == METADATA_STATE_SEEDED_RAW)
    {
        decoded_state = METADATA_STATE_SEEDED;
    }
    else if (raw_state == METADATA_STATE_COMPLETE_RAW)
    {
        decoded_state = METADATA_STATE_COMPLETE;
    }
    else
    {
        return METADATA_ENTRY_INVALID;
    }

    const uint32_t calculated_crc =
        MetadataCRC_CalculateHeader(entry);

    if (stored_crc != calculated_crc)
    {
        return METADATA_ENTRY_INVALID;
    }

    *state_out = decoded_state;
    return METADATA_ENTRY_VALID;
}