#ifndef SENSOR_RECORD_H
#define SENSOR_RECORD_H

#include <stdint.h>
#include <stddef.h>

#define SENSOR_RECORD_SIZE_BYTES 16U
#define SENSOR_RECORD_CRC_OFFSET 12U

#define SENSOR_RECORD_CRC_WORDS (SENSOR_RECORD_CRC_OFFSET / sizeof(uint32_t))

/* Stored record layout: 16 bytes */
typedef struct {
    uint32_t seq;                  // Offset 0
    uint32_t pressure_pa;          // Offset 4
    int16_t  temperature_centi_c;  // Offset 8
    uint16_t humidity_centi_pct;   // Offset 10
    uint32_t crc;                  // Offset 12
} SensorRecord;

_Static_assert(sizeof(SensorRecord) == SENSOR_RECORD_SIZE_BYTES, "SensorRecord must be 16 bytes");

_Static_assert(offsetof(SensorRecord, crc) == SENSOR_RECORD_CRC_OFFSET, "SensorRecord CRC offset must be 12");

_Static_assert((SENSOR_RECORD_CRC_OFFSET % sizeof(uint32_t)) == 0U, "CRC payload must contain whole 32-bit words");

_Static_assert(SENSOR_RECORD_CRC_OFFSET + sizeof(uint32_t) == sizeof(SensorRecord), "CRC must be the final 4-byte field");

typedef enum {
    SENSOR_RECORD_OK = 0,
    SENSOR_RECORD_ERROR_NULL,
    SENSOR_RECORD_ERROR_TEMPERATURE,
    SENSOR_RECORD_ERROR_PRESSURE,
    SENSOR_RECORD_ERROR_HUMIDITY
} SensorRecordStatus;

typedef enum {
    SENSOR_RECORD_BLANK,
    SENSOR_RECORD_VALID,
    SENSOR_RECORD_CORRUPT
} SensorRecordClassification;

SensorRecordStatus SensorRecord_Build(SensorRecord *out, uint32_t seq, int32_t temperature_centi_c,
    uint32_t pressure_q24_8,uint32_t humidity_q22_10);
uint32_t SensorRecord_CalculateCRC(const SensorRecord *record);
SensorRecordClassification SensorRecord_Classify(const uint8_t data[SENSOR_RECORD_SIZE_BYTES]);    
#endif