#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "main.h"
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MOCK_FLASH_SIZE       0x800000U
#define MOCK_GUARD_SIZE       16U
#define MOCK_GUARD_VALUE      0xA5U

void Mock_Reset(void);
void Mock_SetStatus(uint8_t status);

/* Bounded test access. These functions never expose the backing array. */
bool MockFlash_Seed(uint32_t address,
                    const uint8_t *data,
                    size_t length);

bool MockFlash_Inspect(uint32_t address,
                       uint8_t *data,
                       size_t length);

bool MockFlash_CheckGuards(void);

extern uint32_t mock_tx_calls;
extern uint32_t mock_tx_bytes;
extern uint32_t mock_rx_calls;
extern uint32_t mock_rx_bytes;
extern uint32_t mock_cs_writes;
extern uint8_t mock_command;
extern uint32_t mock_command_count;

/* Counts requested bytes whose logical address crosses flash capacity. */
extern uint32_t mock_read_wrap_bytes;

#endif
