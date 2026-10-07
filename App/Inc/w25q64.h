#ifndef W25Q64_H
#define W25Q64_H

#include "main.h"

#define FLASH_CS_GPIO_Port GPIOB
#define FLASH_CS_Pin       GPIO_PIN_6

#define FLASH_CMD_READ_STATUS    0x05
#define FLASH_STATUS_BUSY        0x01
#define FLASH_CMD_WRITE_ENABLE   0x06

uint8_t Flash_ReadStatus(SPI_HandleTypeDef *handler);
HAL_StatusTypeDef Flash_WaitBusy(uint32_t timeout_ms, SPI_HandleTypeDef *handler);
HAL_StatusTypeDef Flash_WriteEnable(SPI_HandleTypeDef *handler);
#endif