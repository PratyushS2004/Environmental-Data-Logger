#include "main.h"
#include "sensor_record.h"

extern CRC_HandleTypeDef hcrc;

uint32_t SensorRecord_CalculateCRC(const SensorRecord *record)
{
    if (record == NULL) {
        return 0U;
    }

    uint32_t crc_words[SENSOR_RECORD_CRC_WORDS];

    crc_words[0] = record->seq;
    crc_words[1] = record->pressure_pa;

    /*
     * Pack temperature and humidity into the third word.
     * The STM32 CRC peripheral processes each word MSB first.
     */
    crc_words[2] = ((uint32_t)record->humidity_centi_pct << 16) | (uint32_t)(uint16_t)record->temperature_centi_c;

    return HAL_CRC_Calculate(&hcrc, crc_words, SENSOR_RECORD_CRC_WORDS);
}

SensorRecordStatus SensorRecord_Build(SensorRecord *out, uint32_t seq, int32_t temperature_centi_c,
    uint32_t pressure_q24_8,uint32_t humidity_q22_10) {

    /* Convert inputs using wide intermediates before validation. */
    uint32_t pressure_pa = (uint32_t)(((uint64_t)pressure_q24_8 + 128U) >> 8);

    uint64_t humidity_scaled = (uint64_t)humidity_q22_10 * 100U + 512U;

    uint64_t humidity_converted = humidity_scaled / 1024U;

    uint16_t humidity_centi_pct;

    /* Check the output pointer before accessing it. */
    if (out == NULL) {
        return SENSOR_RECORD_ERROR_NULL;
    }

    /*
     * Validation priority is temperature, pressure, then humidity.
     * Return the first failing status. On any error, *out remains
     * completely unchanged.
     */
    if (temperature_centi_c < -4000 || temperature_centi_c > 8500) {
        return SENSOR_RECORD_ERROR_TEMPERATURE;
    }

    if (pressure_pa < 30000U || pressure_pa > 110000U) {
        return SENSOR_RECORD_ERROR_PRESSURE;
    }

    /*
     * The converted humidity is unsigned, so it cannot be negative.
     * Only an upper-bound check is needed.
     */
    if (humidity_converted > 10000U) {
        return SENSOR_RECORD_ERROR_HUMIDITY;
    }

    humidity_centi_pct = (uint16_t)humidity_converted;

    /* All validation passed. Populate the record. */
    out->seq = seq;
    out->pressure_pa = pressure_pa;
    out->temperature_centi_c = (int16_t)temperature_centi_c;
    out->humidity_centi_pct = humidity_centi_pct;

    /* Calculate CRC over the finalized first three 32-bit words. */
    out->crc = SensorRecord_CalculateCRC(out);

    return SENSOR_RECORD_OK;
}

SensorRecordClassification SensorRecord_Classify(const uint8_t data[SENSOR_RECORD_SIZE_BYTES]) {
    if (data == NULL) {
        return SENSOR_RECORD_CORRUPT;
    }

    /* An entirely erased flash slot is blank. */
    uint8_t all_ff = 1U;

    for (uint32_t i = 0U; i < SENSOR_RECORD_SIZE_BYTES; i++) {
        if (data[i] != 0xFFU) {
            all_ff = 0U;
            break;
        }
    }

    if (all_ff != 0U) {
        return SENSOR_RECORD_BLANK;
    }

    /* Copy raw bytes into an aligned local record object. */
    SensorRecord record;
    memcpy(&record, data, sizeof(record));

    if (SensorRecord_CalculateCRC(&record) == record.crc) {
        return SENSOR_RECORD_VALID;
    }

    return SENSOR_RECORD_CORRUPT;
}
