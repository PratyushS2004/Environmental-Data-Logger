#include "main.h"

HAL_StatusTypeDef BME280_Read(I2C_HandleTypeDef* handler, uint8_t start_addr, uint8_t* buffer, uint16_t data_size);