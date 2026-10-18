
#include "mock_hal.h"

GPIO_TypeDef fake_GPIOB = {0};

static uint8_t configured_status;
static uint8_t cs_is_low;
static uint8_t current_command_seen;

uint32_t mock_tx_calls;
uint32_t mock_tx_bytes;
uint32_t mock_rx_calls;
uint32_t mock_rx_bytes;
uint32_t mock_cs_writes;
uint8_t mock_command;
uint32_t mock_command_count;

void Mock_Reset(void)
{
    configured_status = 0U;
    cs_is_low = 0U;
    current_command_seen = 0U;

    mock_tx_calls = 0U;
    mock_tx_bytes = 0U;
    mock_rx_calls = 0U;
    mock_rx_bytes = 0U;
    mock_cs_writes = 0U;
    mock_command = 0U;
    mock_command_count = 0U;
}

void Mock_SetStatus(uint8_t status)
{
    configured_status = status;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx,
                       uint16_t GPIO_Pin,
                       GPIO_PinState PinState)
{
    if (GPIOx != GPIOB || GPIO_Pin != GPIO_PIN_6) {
        return;
    }

    mock_cs_writes++;

    if (PinState == GPIO_PIN_RESET) {
        cs_is_low = 1U;
        current_command_seen = 0U;
    } else {
        if (cs_is_low != 0U && current_command_seen != 0U) {
            mock_command_count++;
        }

        cs_is_low = 0U;
    }
}

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi,
                                   const uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout)
{
    (void)hspi;
    (void)Timeout;

    mock_tx_calls++;
    mock_tx_bytes += Size;

    if (cs_is_low != 0U && current_command_seen == 0U && Size > 0U) {
        mock_command = pData[0];
        current_command_seen = 1U;
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi,
                                  uint8_t *pData,
                                  uint16_t Size,
                                  uint32_t Timeout)
{
    (void)hspi;
    (void)Timeout;

    mock_rx_calls++;
    mock_rx_bytes += Size;

    if (cs_is_low != 0U && pData != NULL && Size > 0U) {
        pData[0] = configured_status;
    }

    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return 0U;
}
