
#ifndef RECOVERY_SCAN_H
#define RECOVERY_SCAN_H

#include <stdint.h>
#include "sensor_record.h"

#define RECOVERY_SCAN_NO_SLOT UINT32_MAX
#define W25Q64_CAPACITY_BYTES 0x00800000U

typedef enum {
    SCAN_OK = 0,
    SCAN_FULL,
    SCAN_IO_ERROR,
    SCAN_BAD_ARGUMENT
} ScanStatus;

typedef enum {
    SCAN_READ_OK,
    SCAN_READ_FAILED
} ScanReadStatus;

typedef ScanReadStatus (*ScanReadFn)(
    void *context,
    uint32_t address,
    uint8_t *buffer,
    uint32_t length
);

typedef SensorRecordClassification (*ScanClassifyFn)(
    const uint8_t data[SENSOR_RECORD_SIZE_BYTES]
);

typedef struct {
    uint32_t append_slot;
    uint32_t next_seq;
} ScanResult;

typedef struct {
    ScanReadFn read;
    ScanClassifyFn classify;
    void *context;
    uint32_t base_address;
    uint32_t slot_count;
} ScanConfig;

ScanStatus RecoveryScan(const ScanConfig *config, ScanResult *result);

#endif
