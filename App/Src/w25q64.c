#include "w25q64.h"

#define FLASH_PROGRAM_TIMEOUT_MS  (10)
#define FLASH_ERASE_TIMEOUT_MS    (500)
#define FLASH_CMD_READ_DATA       (0x03)

static inline void Flash_Select(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
}

static inline void Flash_Deselect(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef Flash_ReadStatus(uint8_t *status, SPI_HandleTypeDef *handler){
    uint8_t cmd = FLASH_CMD_READ_STATUS;
    HAL_StatusTypeDef result;

    Flash_Select();

    result = HAL_SPI_Transmit(handler, &cmd, 1, FLASH_SPI_TIMEOUT_MS);
    
    if(result == HAL_OK){
        result = HAL_SPI_Receive(handler, status, 1, FLASH_SPI_TIMEOUT_MS);
    }

    Flash_Deselect();

    return result;
}

HAL_StatusTypeDef Flash_WaitBusy(uint32_t timeout_ms, SPI_HandleTypeDef *handler){
    uint32_t start = HAL_GetTick();
    uint8_t status;

    while (1){
        HAL_StatusTypeDef result = Flash_ReadStatus(&status, handler);

        if (result != HAL_OK)
        {
            return result;
        }

        if ((status & FLASH_STATUS_BUSY) == 0)
        {
            return HAL_OK;
        }

        if ((HAL_GetTick() - start) >= timeout_ms)
        {
            return HAL_TIMEOUT;
        }
    }
}

HAL_StatusTypeDef Flash_WriteEnable(SPI_HandleTypeDef *handler){
    uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

    Flash_Select();

    HAL_StatusTypeDef result = HAL_SPI_Transmit(handler, &cmd, 1, FLASH_SPI_TIMEOUT_MS);

    Flash_Deselect();

    return result;
}

HAL_StatusTypeDef Flash_SectorErase(uint32_t address, SPI_HandleTypeDef *handler){
    HAL_StatusTypeDef status;

    status = Flash_WriteEnable(handler);
    if (status != HAL_OK){
        return status;
    }
    uint8_t cmd[4];

    cmd[0] = FLASH_CMD_SECTOR_ERASE;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    Flash_Select();

    status = HAL_SPI_Transmit(handler, cmd, 4, FLASH_SPI_TIMEOUT_MS);

    Flash_Deselect();

    if (status != HAL_OK){
        return status;
    }
    return Flash_WaitBusy(FLASH_ERASE_TIMEOUT_MS, handler);
}

HAL_StatusTypeDef Flash_Read(uint32_t address, uint8_t *data, uint16_t length,  SPI_HandleTypeDef *handler){
    uint8_t cmd[4];

    cmd[0] = FLASH_CMD_READ_DATA;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    Flash_Select();

    HAL_StatusTypeDef result = HAL_SPI_Transmit(handler, cmd, 4, FLASH_SPI_TIMEOUT_MS);

    if (result == HAL_OK){
        result = HAL_SPI_Receive(handler, data, length, FLASH_SPI_TIMEOUT_MS);
    }

    Flash_Deselect();

    return result;
}