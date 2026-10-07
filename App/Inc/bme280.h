#ifndef BME280_H
#define BME280_H

#include "main.h"

#define Block_1_Length (26)
#define Block_2_Length (7)

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} BME280_Calibration;

HAL_StatusTypeDef BME280_ReadCalibration(I2C_HandleTypeDef* handler, uint8_t calib_1[Block_1_Length], uint8_t calib_2[Block_2_Length]);
void BME280_ParseCalibration(uint8_t calib_1[26], uint8_t calib_2[7], BME280_Calibration *calibration);
HAL_StatusTypeDef BME280_Configure(I2C_HandleTypeDef* handler);
HAL_StatusTypeDef BME280_ReadMeasurement(I2C_HandleTypeDef *handler, const BME280_Calibration *calibration,
    int32_t *temperature, uint32_t *pressure, uint32_t *humidity);
#endif