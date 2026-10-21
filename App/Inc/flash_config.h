#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include <stdint.h>

/* W25Q64 physical geometry. */
#define W25Q64_CAPACITY_BYTES 0x00800000U
#define W25Q64_SECTOR_SIZE    0x00001000U
#define W25Q64_PAGE_SIZE_BYTES 0x00000100U

/*
 * Hardware reference:
 * W25Q64 datasheet specifies 64 Mbit = 8 MiB.
 * Observed JEDEC ID EF 40 17; capacity byte 0x17
 * represents 2^23 bytes for this device family.
 */
_Static_assert(
    W25Q64_CAPACITY_BYTES == 0x00800000U,
    "W25Q64 capacity changed; verify the physical device and layout"
);

/* Sector size is also part of the approved physical geometry. */
_Static_assert(
    W25Q64_SECTOR_SIZE == 0x00001000U,
    "W25Q64 sector size changed; verify the datasheet and layout"
);

_Static_assert(
    W25Q64_PAGE_SIZE_BYTES == 256U,
    "W25Q64 page size changed; verify the datasheet and layout"
);

_Static_assert(
    (W25Q64_SECTOR_SIZE % W25Q64_PAGE_SIZE_BYTES) == 0U,
    "Sector size must be a multiple of page size"
);

#endif
