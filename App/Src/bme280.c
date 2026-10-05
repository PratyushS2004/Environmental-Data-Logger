#include "bme280.h"

#define BME280_I2C_ADDR (0x76 << 1)
#define Compensation_Start_ADDR (0x88)
#define Compensation_Block_2_ADDR (0xE1)


HAL_StatusTypeDef BME280_Read(I2C_HandleTypeDef* handler, uint8_t start_addr, uint8_t* buffer, uint16_t data_size){

    return HAL_I2C_Mem_Read(handler, BME280_I2C_ADDR, start_addr, I2C_MEMADD_SIZE_8BIT, buffer, data_size, 100);
}

HAL_StatusTypeDef BME280_ReadCalibration(I2C_HandleTypeDef* handler, uint8_t calib_1[Block_1_Length], uint8_t calib_2[Block_2_Length]){
    
    HAL_StatusTypeDef status = BME280_Read(handler, Compensation_Start_ADDR, calib_1, Block_1_Length);
    if (status != HAL_OK){
        return status;
    }

    status = BME280_Read(handler, Compensation_Block_2_ADDR, calib_2, Block_2_Length);
    return status;
}

static uint16_t BME280_CombineBytes(uint8_t low_byte, uint8_t high_byte){
    return (uint16_t)low_byte | ((uint16_t)high_byte << 8);
}


static int16_t BME280_SignExtend12(uint16_t value)
{
    if (value & 0x0800){
        value |= 0xF000;
    }
    return (int16_t)value;
}

void BME280_ParseCalibration(uint8_t calib_1[26], uint8_t calib_2[7], BME280_Calibration *calibration){
    /* Temperature calibration */

    calibration->dig_T1 = BME280_CombineBytes(calib_1[0], calib_1[1]);

    calibration->dig_T2 = (int16_t)BME280_CombineBytes(calib_1[2], calib_1[3]);

    calibration->dig_T3 = (int16_t)BME280_CombineBytes(calib_1[4], calib_1[5]);

    /* Pressure calibration */

    calibration->dig_P1 = BME280_CombineBytes(calib_1[6], calib_1[7]);

    calibration->dig_P2 = (int16_t)BME280_CombineBytes(calib_1[8], calib_1[9]);

    calibration->dig_P3 = (int16_t)BME280_CombineBytes(calib_1[10], calib_1[11]);

    calibration->dig_P4 = (int16_t)BME280_CombineBytes(calib_1[12], calib_1[13]);

    calibration->dig_P5 = (int16_t)BME280_CombineBytes(calib_1[14], calib_1[15]);

    calibration->dig_P6 = (int16_t)BME280_CombineBytes(calib_1[16], calib_1[17]);

    calibration->dig_P7 = (int16_t)BME280_CombineBytes(calib_1[18], calib_1[19]);

    calibration->dig_P8 = (int16_t)BME280_CombineBytes(calib_1[20], calib_1[21]);

    calibration->dig_P9 = (int16_t)BME280_CombineBytes(calib_1[22], calib_1[23]);


    /* Humidity calibration */

    /* 0xA0 is reserved, so 0xA1 = index 25 */
    calibration->dig_H1 = calib_1[25];

    calibration->dig_H2 = (int16_t)BME280_CombineBytes(calib_2[0], calib_2[1]);

    calibration->dig_H3 = calib_2[2];

    uint16_t raw_h4 = ((uint16_t)calib_2[3] << 4) | (calib_2[4] & 0x0F);

    calibration->dig_H4 = BME280_SignExtend12(raw_h4);

    uint16_t raw_h5 = ((uint16_t)calib_2[5] << 4) | (calib_2[4] >> 4);

    calibration->dig_H5 = BME280_SignExtend12(raw_h5);

    calibration->dig_H6 = (int8_t)calib_2[6];
}



