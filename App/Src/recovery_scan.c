#include "recovery_scan.h"
#include "sensor_record.h"
#include <string.h>

/* Scans slots sequentially.
 *
 * VALID: remember the record sequence number.
 * CORRUPT: skip the slot without trusting its contents.
 * BLANK: stop at the first blank slot and return it as the
 *        append candidate.
 *
 * BLANK must mean that all 16 bytes are 0xFF. That definition
 * belongs to SensorRecord_Classify(), not to the scan.
 *
 * If every slot is occupied, returns SCAN_FULL and sets
 * append_slot to RECOVERY_SCAN_NO_SLOT.
 *
 * On an I/O or argument error, result is initialized to
 * RECOVERY_SCAN_NO_SLOT and next_seq = 0 when result is non-NULL.
 */
ScanStatus RecoveryScan(const ScanConfig *config, ScanResult *result) {
    uint8_t bytes[SENSOR_RECORD_SIZE_BYTES];
    uint32_t last_valid_seq = 0U;
    uint8_t have_valid = 0U;

    if (result != NULL) {
        result->append_slot = RECOVERY_SCAN_NO_SLOT;
        result->next_seq = 0U;
    }

    if (config == NULL || result == NULL || config->read == NULL || config->slot_count == 0U ||
        config->base_address >= W25Q64_CAPACITY_BYTES || config->slot_count >
            (W25Q64_CAPACITY_BYTES - config->base_address) / SENSOR_RECORD_SIZE_BYTES) {
        return SCAN_BAD_ARGUMENT;
    }

    for (uint32_t slot = 0U; slot < config->slot_count; ++slot) {
        uint32_t address = config->base_address + slot * SENSOR_RECORD_SIZE_BYTES;

        if (config->read(config->context, address, bytes, SENSOR_RECORD_SIZE_BYTES) != SCAN_READ_OK) {
            return SCAN_IO_ERROR;
        }

        /*
         * SensorRecord_Classify is the single authority for
         * blank, valid, and corrupt classification.
         */
        SensorRecordClassification classification = config->classify(bytes);

        if (classification == SENSOR_RECORD_BLANK) {
            result->append_slot = slot;
            result->next_seq = have_valid ? last_valid_seq + 1U : 0U;

            return SCAN_OK;
        }

        if (classification == SENSOR_RECORD_VALID) {
            SensorRecord record;
            memcpy(&record, bytes, sizeof(record));

            last_valid_seq = record.seq;
            have_valid = 1U;
        }

        /* Corrupt records are skipped without trusting their seq. */
        
        
    }

    result->append_slot = RECOVERY_SCAN_NO_SLOT;
    result->next_seq = have_valid ? last_valid_seq + 1U : 0U;

    return SCAN_FULL;
}
