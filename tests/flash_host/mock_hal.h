#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "main.h"
#include <stdint.h>

void Mock_Reset(void);
void Mock_SetStatus(uint8_t status);

extern uint32_t mock_tx_calls;
extern uint32_t mock_tx_bytes;
extern uint32_t mock_rx_calls;
extern uint32_t mock_rx_bytes;
extern uint32_t mock_cs_writes;
extern uint8_t mock_command;
extern uint32_t mock_command_count;

#endif
