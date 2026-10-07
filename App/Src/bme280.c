#include "bme280.h"

#define BME280_I2C_ADDR (0x76 << 1)
#define Compensation_Start_ADDR (0x88)
#define Compensation_Block_2_ADDR (0xE1)
#define BME280_I2C_TIMEOUT_MS (100)

#define BME280_CTRL_HUM_ADDR       (0xF2)
#define BME280_CTRL_MEAS_ADDR      (0xF4)
#define BME280_STATUS_ADDR         (0xF3)
#define BME280_PRESS_TEMP_HUM_ADDR (0xF7)

#define BME280_STATUS_MEASURING    (0x08)

#define BME280_MEASUREMENT_TIMEOUT_MS (20)

static HAL_StatusTypeDef BME280_Read(I2C_HandleTypeDef* handler, uint8_t start_addr, uint8_t* buffer, uint16_t data_size){
    return HAL_I2C_Mem_Read(handler, BME280_I2C_ADDR, start_addr, I2C_MEMADD_SIZE_8BIT, buffer, data_size, BME280_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef BME280_ReadCalibration(I2C_HandleTypeDef* handler, uint8_t calib_1[Block_1_Length], uint8_t calib_2[Block_2_Length]){
    /* Calibration coefficients are stored in two separate register blocks. */
    HAL_StatusTypeDef status = BME280_Read(handler, Compensation_Start_ADDR, calib_1, Block_1_Length);
    if (status != HAL_OK){
        return status;
    }

    return BME280_Read(handler, Compensation_Block_2_ADDR, calib_2, Block_2_Length);
}

static uint16_t BME280_CombineBytes(uint8_t low_byte, uint8_t high_byte){
    /* Calibration coefficients are stored in two separate register blocks. */
    return (uint16_t)low_byte | ((uint16_t)high_byte << 8);
}


static int16_t BME280_SignExtend12(uint16_t value){
    /* Convert a signed 12-bit humidity coefficient to a signed 16-bit value. */
    if (value & 0x0800){
        value |= 0xF000;
    }
    return (int16_t)value;
}

void BME280_ParseCalibration(uint8_t calib_1[Block_1_Length], uint8_t calib_2[Block_2_Length], BME280_Calibration *calibration){
    /*
     * Unpack the raw calibration bytes into the signed/unsigned
     * coefficients used by the compensation formulas.
     */

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

static HAL_StatusTypeDef BME280_WriteReg(I2C_HandleTypeDef* handler, uint8_t reg_addr, uint8_t data){
    return HAL_I2C_Mem_Write(handler,BME280_I2C_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, &data, 1, BME280_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef BME280_Configure(I2C_HandleTypeDef* handler){
    HAL_StatusTypeDef status;

    /* Humidity oversampling = x1 */
    status = BME280_WriteReg(handler, BME280_CTRL_HUM_ADDR, 0x01);
    if (status != HAL_OK) {
        return status;
    }

    /* Temperature x1, pressure x1, sleep mode */
    return BME280_WriteReg(handler, BME280_CTRL_MEAS_ADDR, 0x24);
}

static HAL_StatusTypeDef BME280_TriggerMeasurement(I2C_HandleTypeDef* handler)
{
    /* Temperature x1, pressure x1, forced mode */
    return BME280_WriteReg(handler, BME280_CTRL_MEAS_ADDR, 0x25);
}

static HAL_StatusTypeDef BME280_WaitForMeasurement(I2C_HandleTypeDef* handler){
    uint32_t start = HAL_GetTick();
    uint8_t status_val;

    /* Give the sensor time to enter measuring state. */
    HAL_Delay(1);

    while (1){
        HAL_StatusTypeDef status = BME280_Read(handler, BME280_STATUS_ADDR, &status_val, 1);

        if (status != HAL_OK){
            return status;
        }

        /* Sensor clears the measuring bit when the conversion is complete. */
        if ((status_val & BME280_STATUS_MEASURING) == 0){
            return HAL_OK;
        }

        if ((HAL_GetTick() - start) >= BME280_MEASUREMENT_TIMEOUT_MS){
            return HAL_TIMEOUT;
        }

        HAL_Delay(1);
    }
}

static int32_t BME280_CompensateTemperature(const BME280_Calibration *calibration, int32_t adc_T, int32_t *t_fine){
    int32_t var1;
    int32_t var2;

    /* Apply the BME280 calibration coefficients to the raw temperature ADC value. */
    var1 = ((((adc_T >> 3) - ((int32_t)calibration->dig_T1 << 1)) * ((int32_t)calibration->dig_T2)) >> 11);

    var2 = (((((adc_T >> 4) - ((int32_t)calibration->dig_T1)) * ((adc_T >> 4) - ((int32_t)calibration->dig_T1))) >> 12) *
    ((int32_t)calibration->dig_T3)) >> 14;

    /* t_fine is used by the pressure compensation calculation. */
    *t_fine = var1 + var2;

    /* Temperature is returned in degrees Celsius x100. */
    return (*t_fine * 5 + 128) >> 8;
}

static HAL_StatusTypeDef BME280_CompensatePressure(const BME280_Calibration *calibration, int32_t adc_P, int32_t t_fine, uint32_t *pressure){
    int64_t var1;
    int64_t var2;
    int64_t p;

    /* Calculate the intermediate values using the temperature compensation. */
    var1 = ((int64_t)t_fine) - 128000;

    var2 = var1 * var1 * (int64_t)calibration->dig_P6;
    var2 = var2 + ((var1 * (int64_t)calibration->dig_P5) << 17);
    var2 = var2 + (((int64_t)calibration->dig_P4) << 35);

    var1 = ((var1 * var1 * (int64_t)calibration->dig_P3) >> 8) + ((var1 * (int64_t)calibration->dig_P2) << 12);

    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)calibration->dig_P1) >> 33;

    /* Prevent division by zero if the calibration data is invalid. */
    if (var1 == 0)
    {
        return HAL_ERROR;
    }

    p = 1048576 - adc_P;

    p = (((p << 31) - var2) * 3125) / var1;

    var1 = ((int64_t)calibration->dig_P9 * (p >> 13) * (p >> 13)) >> 25;

    var2 = ((int64_t)calibration->dig_P8 * p) >> 19;

    /* Apply the final pressure compensation and return pressure in Q24.8 format. */
    p = ((p + var1 + var2) >> 8) + (((int64_t)calibration->dig_P7) << 4);

    *pressure = (uint32_t)p;

    return HAL_OK;
}

static uint32_t BME280_CompensateHumidity(const BME280_Calibration *calibration, int32_t adc_H, int32_t t_fine){
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;

    var1 = t_fine - 76800;

    var2 = (adc_H << 14) - ((int32_t)calibration->dig_H4 << 20) - ((int32_t)calibration->dig_H5 * var1);

    var2 = (var2 + 16384) >> 15;

    var3 = (var1 * (int32_t)calibration->dig_H6) >> 10;

    var4 = ((var1 * (int32_t)calibration->dig_H3) >> 11) + 32768;

    var3 = (var3 * var4) >> 10;

    var3 = var3 + 2097152;

    var3 = (var3 * (int32_t)calibration->dig_H2 + 8192) >> 14;

    var2 = var2 * var3;

    var1 = (var2 >> 15) * (var2 >> 15);

    var1 = (var1 >> 7) * (int32_t)calibration->dig_H1;

    var1 = var1 >> 4;

    var2 = var2 - var1;

    if (var2 < 0)
    {
        var2 = 0;
    }

    if (var2 > 419430400)
    {
        var2 = 419430400;
    }

    return (uint32_t)(var2 >> 12);
}

HAL_StatusTypeDef BME280_ReadMeasurement(I2C_HandleTypeDef *handler, const BME280_Calibration *calibration,
    int32_t *temperature, uint32_t *pressure, uint32_t *humidity){

    uint8_t raw[8];

    int32_t adc_T;
    int32_t adc_P;
    int32_t adc_H;
    int32_t t_fine;

    int32_t compensated_temperature;
    uint32_t compensated_pressure;
    uint32_t compensated_humidity;

    HAL_StatusTypeDef status;

    status = BME280_TriggerMeasurement(handler);
    if (status != HAL_OK){
        return status;
    }

    status = BME280_WaitForMeasurement(handler);
    if (status != HAL_OK){
        return status;
    }

    status = BME280_Read(handler, BME280_PRESS_TEMP_HUM_ADDR, raw, 8);

    if (status != HAL_OK){
        return status;
    }
    
    /* Reconstruct the 20-bit pressure and temperature ADC values and 16-bit humidity value. */
    adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4)  | ((int32_t)raw[2] >> 4);

    adc_T = ((int32_t)raw[3] << 12)  | ((int32_t)raw[4] << 4)  | ((int32_t)raw[5] >> 4);

    adc_H = ((int32_t)raw[6] << 8)  | (int32_t)raw[7];

    /* Reject skipped measurements. */
    if (adc_T == 0x80000){
        return HAL_ERROR;
    }

    if (adc_P == 0x80000){
        return HAL_ERROR;
    }

    if (adc_H == 0x8000){
        return HAL_ERROR;
    }

    /*
     * Compensate temperature first because pressure and
     * humidity both depend on the resulting t_fine.
     */
    compensated_temperature = BME280_CompensateTemperature(calibration, adc_T, &t_fine);

    status = BME280_CompensatePressure(calibration, adc_P, t_fine, &compensated_pressure);

    if (status != HAL_OK){
        return status;
    }

    compensated_humidity = BME280_CompensateHumidity(calibration, adc_H, t_fine);

    /*
     * Only update caller-visible values after the
     * complete measurement succeeded.
     */
    *temperature = compensated_temperature;
    *pressure = compensated_pressure;
    *humidity = compensated_humidity;

    return HAL_OK;
}