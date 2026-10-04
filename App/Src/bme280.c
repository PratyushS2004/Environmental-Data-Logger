#include "bme280.h"

#define BME280_I2C_ADDR (0x76 << 1)

HAL_StatusTypeDef BME280_Read(I2C_HandleTypeDef* handler, uint8_t start_addr, uint8_t* buffer, uint16_t data_size){

    return HAL_I2C_Mem_Read(handler, BME280_I2C_ADDR, start_addr, I2C_MEMADD_SIZE_8BIT, buffer, data_size, 100);
}

HAL_StatusTypeDef BME280_ReadCalibration(I2C_HandleTypeDef* handler, uint8_t calib_1[26], uint8_t calib_2[7]){

HAL_StatusTypeDef status1 = BME280_Read(handler, 0x88, calib_1, 26);
HAL_StatusTypeDef status2 = BME280_Read(handler, 0xE1, calib_2, 7);
}

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


