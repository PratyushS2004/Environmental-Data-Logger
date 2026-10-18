#include <stdio.h>
#include <stdint.h>

#include "w25q64.h"
#include "mock_hal.h"

int main(void)
{
    SPI_HandleTypeDef spi = {0};
    uint8_t status = 0xA5U;
    HAL_StatusTypeDef result;
    uint32_t failures = 0U;

    Mock_Reset();
    Mock_SetStatus(0x03U);

    result = Flash_ReadStatus(&status, &spi);

    if (result != HAL_OK) {
        printf("FAIL Flash_ReadStatus: expected result HAL_OK, got %u\n",
               (unsigned int)result);
        failures++;
    }

    if (status != 0x03U) {
        printf("FAIL Flash_ReadStatus: expected status 0x03, got 0x%02X\n",
               (unsigned int)status);
        failures++;
    }

    if (mock_tx_calls != 1U || mock_tx_bytes != 1U) {
        printf("FAIL SPI transmit: expected 1 call / 1 byte, got %u / %u\n",
               (unsigned int)mock_tx_calls,
               (unsigned int)mock_tx_bytes);
        failures++;
    }

    if (mock_rx_calls != 1U || mock_rx_bytes != 1U) {
        printf("FAIL SPI receive: expected 1 call / 1 byte, got %u / %u\n",
               (unsigned int)mock_rx_calls,
               (unsigned int)mock_rx_bytes);
        failures++;
    }

    if (mock_cs_writes != 2U) {
        printf("FAIL chip select: expected 2 GPIO writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (mock_command_count != 1U || mock_command != 0x05U) {
        printf("FAIL command: expected one 0x05 command, got count=%u command=0x%02X\n",
               (unsigned int)mock_command_count,
               (unsigned int)mock_command);
        failures++;
    }

    if (failures == 0U) {
        printf("PASS Flash_ReadStatus\n");
        return 0;
    }

    printf("FAIL Flash_ReadStatus: %u check(s) failed\n",
           (unsigned int)failures);
    return 1;
}
