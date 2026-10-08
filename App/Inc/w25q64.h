#ifndef W25Q64_H
#define W25Q64_H

#include "main.h"

#define FLASH_CS_GPIO_Port GPIOB
#define FLASH_CS_Pin       GPIO_PIN_6

#define FLASH_CMD_READ_STATUS    0x05
#define FLASH_STATUS_BUSY        0x01
#define FLASH_CMD_WRITE_ENABLE   0x06
#define FLASH_SPI_TIMEOUT_MS     100
#define FLASH_CMD_SECTOR_ERASE   0x20
#define FLASH_STATUS_WEL        (1U << 1)

HAL_StatusTypeDef Flash_ReadStatus(uint8_t *status, SPI_HandleTypeDef *handler);
HAL_StatusTypeDef Flash_WaitBusy(uint32_t timeout_ms, SPI_HandleTypeDef *handler);
HAL_StatusTypeDef Flash_WriteEnable(SPI_HandleTypeDef *handler);
HAL_StatusTypeDef Flash_SectorErase(uint32_t address, SPI_HandleTypeDef *handler);
HAL_StatusTypeDef Flash_Read(uint32_t address, uint8_t *data, uint16_t length,  SPI_HandleTypeDef *handler);

#endif