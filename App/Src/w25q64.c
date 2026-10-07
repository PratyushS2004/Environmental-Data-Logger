#include "w25q64.h";

#define FLASH_PROGRAM_TIMEOUT_MS  (10)
#define FLASH_ERASE_TIMEOUT_MS    (500)
#define FLASH_SPI_TIMEOUT_MS      (100)

uint8_t Flash_ReadStatus(SPI_HandleTypeDef *handler){
    uint8_t cmd = FLASH_CMD_READ_STATUS;
    uint8_t status = 0;

    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

    HAL_SPI_Transmit(handler, &cmd, 1, FLASH_SPI_TIMEOUT_MS);
    HAL_SPI_Receive(handler, &status, 1, FLASH_SPI_TIMEOUT_MS);

    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

    return status;
}

HAL_StatusTypeDef Flash_WaitBusy(uint32_t timeout_ms, SPI_HandleTypeDef *handler){
    uint32_t start = HAL_GetTick();

    while (Flash_ReadStatus(handler) & FLASH_STATUS_BUSY)
    {
        if ((HAL_GetTick() - start) >= timeout_ms){
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef Flash_WriteEnable(SPI_HandleTypeDef *handler){
    uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef result = HAL_SPI_Transmit(handler, &cmd, 1, FLASH_SPI_TIMEOUT_MS);

    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

    return result;
}