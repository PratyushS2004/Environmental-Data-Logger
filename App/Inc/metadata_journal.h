
#ifndef METADATA_JOURNAL_H
#define METADATA_JOURNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define METADATA_ENTRY_SIZE_BYTES 16U
#define METADATA_SCAN_NO_SLOT     UINT32_MAX

#define METADATA_MAGIC             0x4D455441U
#define METADATA_VERSION           1U

#define METADATA_STATE_SEEDED_RAW   0x53A1U
#define METADATA_STATE_COMPLETE_RAW 0xC04EU

typedef enum
{
    METADATA_STATE_NONE = 0,
    METADATA_STATE_SEEDED,
    METADATA_STATE_COMPLETE
} MetadataState;

typedef enum
{
    METADATA_ENTRY_BLANK = 0,
    METADATA_ENTRY_INVALID,
    METADATA_ENTRY_VALID
} MetadataEntryClassification;

typedef enum
{
    METADATA_SCAN_BLANK = 0,
    METADATA_SCAN_VALID,
    METADATA_SCAN_INVALID,
    METADATA_SCAN_FULL,
    METADATA_SCAN_IO_ERROR,
    METADATA_SCAN_BAD_ARGUMENT
} MetadataScanStatus;

typedef bool (*MetadataScanReadFn)(
    void *context,
    uint32_t address,
    uint8_t *buffer,
    size_t length);

typedef struct
{
    MetadataScanReadFn read;
    void *context;
    uint32_t base_address;
    uint32_t entry_count;
} MetadataScanConfig;

typedef struct
{
    bool has_valid_state;
    MetadataState last_state;
    uint32_t next_index;
    bool anomaly_detected;
} MetadataScanResult;

/*
 * Classifies one serialized 16-byte metadata entry.
 *
 * Format:
 *   Bytes 0..3:  magic, little-endian
 *   Bytes 4..5:  version, little-endian
 *   Bytes 6..7:  raw state, little-endian
 *   Bytes 8..11: CRC, little-endian
 *   Bytes 12..15: reserved tail, expected erased but ignored
 *                when determining entry validity
 *
 * CRC covers bytes 0..7 using the agreed STM32-compatible
 * CRC-32/MPEG-2 word-processing convention.
 *
 * Returns BLANK only if all 16 bytes are 0xFF.
 * Returns VALID only if the header fields and CRC are valid.
 * Returns INVALID for malformed entries or NULL arguments.
 *
 * If state_out is non-NULL, it is initialized to NONE.
 * A decoded state is returned through state_out only for VALID.
 */
MetadataEntryClassification MetadataJournal_ClassifyEntry(
    const uint8_t entry[METADATA_ENTRY_SIZE_BYTES],
    MetadataState *state_out);

/*
 * Scans entries in physical order and stops at the first
 * completely blank entry.
 *
 * Invalid nonblank entries occupy physical slots but do not
 * affect valid-state history.
 *
 * Legal valid-state transitions:
 *   NONE     -> SEEDED only
 *   SEEDED   -> COMPLETE only
 *   COMPLETE -> no subsequent valid state
 *
 * Illegal transitions latch anomaly_detected. last_state still
 * updates to the newest valid state, including anomalous ones.
 *
 * On IO_ERROR or BAD_ARGUMENT, result fields are reset to safe
 * defaults if result is non-NULL.
 */
MetadataScanStatus MetadataJournal_Scan(
    const MetadataScanConfig *config,
    MetadataScanResult *result);

#endif /* METADATA_JOURNAL_H */
