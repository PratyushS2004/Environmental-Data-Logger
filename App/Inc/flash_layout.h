#ifndef FLASH_LAYOUT_H
#define FLASH_LAYOUT_H

#include <stdint.h>
#include "flash_config.h"
#include "sensor_record.h"

#define FLASH_LOG_SLOT_COUNT 1024U

#define FLASH_LOG_SIZE_BYTES \
    (FLASH_LOG_SLOT_COUNT * SENSOR_RECORD_SIZE_BYTES)

/*
 * Three reserved sectors follow the log:
 * guard sector, test sector A, test sector B.
 */
#define FLASH_RESERVED_SECTOR_COUNT 3U

#define FLASH_LOG_BASE_ADDRESS \
    (W25Q64_CAPACITY_BYTES \
     - (FLASH_RESERVED_SECTOR_COUNT * W25Q64_SECTOR_SIZE) \
     - FLASH_LOG_SIZE_BYTES)

#define FLASH_LOG_END_ADDRESS \
    (FLASH_LOG_BASE_ADDRESS + FLASH_LOG_SIZE_BYTES)

#define FLASH_GUARD_SECTOR_ADDRESS \
    FLASH_LOG_END_ADDRESS

#define FLASH_TEST_SECTOR_A_ADDRESS \
    (FLASH_GUARD_SECTOR_ADDRESS + W25Q64_SECTOR_SIZE)

#define FLASH_TEST_SECTOR_B_ADDRESS \
    (FLASH_TEST_SECTOR_A_ADDRESS + W25Q64_SECTOR_SIZE)

#define FLASH_TEST_SECTOR_B_END_ADDRESS \
    (FLASH_TEST_SECTOR_B_ADDRESS + W25Q64_SECTOR_SIZE)

/* Record and layout assumptions. */
_Static_assert(
    SENSOR_RECORD_SIZE_BYTES > 0U,
    "Sensor record size must be nonzero"
);

_Static_assert(
    W25Q64_CAPACITY_BYTES >=
        (FLASH_RESERVED_SECTOR_COUNT * W25Q64_SECTOR_SIZE),
    "Reserved sectors exceed flash capacity"
);

/*
 * Bound the slot count before multiplying it by the record size.
 * This prevents the size check from passing due to unsigned wrap.
 */
_Static_assert(
    FLASH_LOG_SLOT_COUNT <=
        (W25Q64_CAPACITY_BYTES -
         (FLASH_RESERVED_SECTOR_COUNT * W25Q64_SECTOR_SIZE))
        / SENSOR_RECORD_SIZE_BYTES,
    "Log and reserved sectors exceed flash capacity"
);

_Static_assert(
    FLASH_LOG_SLOT_COUNT > 0U,
    "Log must contain at least one record"
);

/* A sector contains 256 records with the approved geometry. */
_Static_assert(
    (W25Q64_SECTOR_SIZE % SENSOR_RECORD_SIZE_BYTES) == 0U,
    "Sector size must be a multiple of record size"
);

_Static_assert(
    (FLASH_LOG_SLOT_COUNT %
     (W25Q64_SECTOR_SIZE / SENSOR_RECORD_SIZE_BYTES)) == 0U,
    "Log slot count must be a whole number of sectors"
);

/* Approved layout pins. */
_Static_assert(
    FLASH_LOG_BASE_ADDRESS == 0x007F9000U,
    "Log base changed; review existing flash data"
);

_Static_assert(
    (FLASH_TEST_SECTOR_A_ADDRESS % W25Q64_SECTOR_SIZE) == 0U,
    "Test sector A must be sector-aligned"
);

_Static_assert(
    FLASH_TEST_SECTOR_B_END_ADDRESS == W25Q64_CAPACITY_BYTES,
    "Test sector B must end at flash capacity"
);

#endif
