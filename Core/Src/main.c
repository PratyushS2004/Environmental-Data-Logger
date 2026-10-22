/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "flash_layout.h"
#include "recovery_scan.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PROVISION_LOG 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Test_Print(const char *message)
{
    HAL_UART_Transmit(&huart2,
                     (uint8_t *)message,
                     (uint16_t)strlen(message),
                     HAL_MAX_DELAY);
}

#define PROVISION_LOG_BASE          FLASH_LOG_BASE_ADDRESS
#define PROVISION_LOG_SIZE          FLASH_LOG_SIZE_BYTES
#define PROVISION_SECTOR_SIZE       W25Q64_SECTOR_SIZE
#define PROVISION_SECTOR_COUNT      \
    (PROVISION_LOG_SIZE / PROVISION_SECTOR_SIZE)

#define PROVISION_VERIFY_CHUNK_SIZE 256U

_Static_assert(
    (PROVISION_LOG_SIZE % W25Q64_SECTOR_SIZE) == 0U,
    "Log size must contain whole flash sectors"
);

_Static_assert(
    PROVISION_LOG_BASE % W25Q64_SECTOR_SIZE == 0U,
    "Log base must be sector-aligned"
);

_Static_assert(
    PROVISION_LOG_BASE + PROVISION_LOG_SIZE ==
        FLASH_LOG_END_ADDRESS,
    "Provisioning region must match the approved log layout"
);

static void Test_PrintHex32(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char output[8];

    for (uint32_t i = 0; i < 8U; i++)
    {
        uint32_t shift = 28U - (i * 4U);
        output[i] = hex[(value >> shift) & 0x0FU];
    }

    HAL_UART_Transmit(&huart2,
                      (uint8_t *)output,
                      sizeof(output),
                      HAL_MAX_DELAY);
}

#if PROVISION_LOG
static void Test_PrintHexByte(uint8_t value)
{
    const char hex[] = "0123456789ABCDEF";
    char text[3];

    text[0] = hex[(value >> 4) & 0x0F];
    text[1] = hex[value & 0x0F];
    text[2] = '\0';

    Test_Print(text);
}

static HAL_StatusTypeDef ProvisionLog(void)
{
    uint8_t buffer[PROVISION_VERIFY_CHUNK_SIZE];

    Test_Print("PROVISION START base=0x");
    Test_PrintHex32(PROVISION_LOG_BASE);
    Test_Print(" size=0x");
    Test_PrintHex32(PROVISION_LOG_SIZE);
    Test_Print(" sectors=0x");
    Test_PrintHex32(PROVISION_SECTOR_COUNT);
    Test_Print("\r\n");

    for (uint32_t sector = 0;
         sector < PROVISION_SECTOR_COUNT;
         sector++)
    {
        uint32_t sector_address =
            PROVISION_LOG_BASE + (sector * PROVISION_SECTOR_SIZE);

        /* Erase this sector. */
        HAL_StatusTypeDef status =
            Flash_SectorErase(sector_address, &hspi1);

        if (status != HAL_OK)
        {
            Test_Print("PROVISION ERASE FAIL sector=0x");
            Test_PrintHex32(sector);
            Test_Print(" addr=0x");
            Test_PrintHex32(sector_address);
            Test_Print(" status=0x");
            Test_PrintHexByte((uint8_t)status);
            Test_Print("\r\n");

            return status;
        }

        Test_Print("PROVISION ERASE sector=0x");
        Test_PrintHex32(sector);
        Test_Print(" addr=0x");
        Test_PrintHex32(sector_address);
        Test_Print(" OK\r\n");

        /* Verify every byte in the sector. */
        for (uint32_t offset = 0;
             offset < PROVISION_SECTOR_SIZE;
             offset += PROVISION_VERIFY_CHUNK_SIZE)
        {
            uint32_t address = sector_address + offset;

            status = Flash_Read(address,
                                buffer,
                                (uint16_t)sizeof(buffer),
                                &hspi1);

            if (status != HAL_OK)
            {
                Test_Print("PROVISION READ FAIL sector=0x");
                Test_PrintHex32(sector);
                Test_Print(" addr=0x");
                Test_PrintHex32(address);
                Test_Print(" status=0x");
                Test_PrintHexByte((uint8_t)status);
                Test_Print("\r\n");

                return status;
            }

            for (uint32_t i = 0; i < sizeof(buffer); i++)
            {
                if (buffer[i] != 0xFFU)
                {
                    Test_Print("PROVISION VERIFY FAIL sector=0x");
                    Test_PrintHex32(sector);
                    Test_Print(" addr=0x");
                    Test_PrintHex32(address + i);
                    Test_Print(" expected=0xFF actual=0x");
                    Test_PrintHexByte(buffer[i]);
                    Test_Print("\r\n");

                    return HAL_ERROR;
                }
            }
        }

        Test_Print("PROVISION VERIFY sector=0x");
        Test_PrintHex32(sector);
        Test_Print(" addr=0x");
        Test_PrintHex32(sector_address);
        Test_Print(" OK\r\n");
    }

    Test_Print("PROVISION COMPLETE bytes_verified=16384\r\n");

    return HAL_OK;
}
#endif

static const char *RecoveryStatusName(ScanStatus status)
{
    switch (status)
    {
        case SCAN_OK:
            return "SCAN_OK";

        case SCAN_FULL:
            return "SCAN_FULL";

        case SCAN_IO_ERROR:
            return "SCAN_IO_ERROR";

        case SCAN_BAD_ARGUMENT:
            return "SCAN_BAD_ARGUMENT";

        default:
            return "UNKNOWN";
    }
}

static ScanReadStatus Recovery_FlashRead(
    void *context,
    uint32_t address,
    uint8_t *buffer,
    uint32_t length)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)context;

    if (hspi == NULL ||
        buffer == NULL ||
        length == 0U ||
        length > UINT16_MAX)
    {
        return SCAN_READ_FAILED;
    }

    if (Flash_Read(address,
                   buffer,
                   (uint16_t)length,
                   hspi) != HAL_OK)
    {
        return SCAN_READ_FAILED;
    }

    return SCAN_READ_OK;
}

static HAL_StatusTypeDef Logger_AppendRecord(
    uint32_t slot,
    const SensorRecord *record,
    SPI_HandleTypeDef *hspi
  );


static HAL_StatusTypeDef Logger_AppendRecord(
    uint32_t slot,
    const SensorRecord *record,
    SPI_HandleTypeDef *hspi)
{
    uint8_t existing[SENSOR_RECORD_SIZE_BYTES];
    uint8_t readback[SENSOR_RECORD_SIZE_BYTES];
    const uint8_t *record_bytes;
    uint32_t address;
    HAL_StatusTypeDef status;
    SensorRecordClassification classification;

    if (record == NULL || hspi == NULL ||
        slot >= FLASH_LOG_SLOT_COUNT)
    {
        return HAL_ERROR;
    }

    address = FLASH_LOG_BASE_ADDRESS +
              slot * SENSOR_RECORD_SIZE_BYTES;

    /*
     * Never program a slot unless the entire destination is erased.
     */
    status = Flash_Read(
        address,
        existing,
        SENSOR_RECORD_SIZE_BYTES,
        hspi);

    if (status != HAL_OK)
    {
        Test_Print("APPEND ERROR: DESTINATION READ\r\n");
        return status;
    }

    for (uint32_t i = 0U; i < SENSOR_RECORD_SIZE_BYTES; i++)
    {
        if (existing[i] != 0xFFU)
        {
            Test_Print("APPEND ERROR: DESTINATION NOT BLANK\r\n");
            return HAL_ERROR;
        }
    }

    /*
     * SensorRecord is statically checked to be exactly 16 bytes.
     * Program once only; an error can leave the slot uncertain.
     */
    record_bytes = (const uint8_t *)record;

    status = Flash_PageProgram(
        address,
        record_bytes,
        SENSOR_RECORD_SIZE_BYTES,
        hspi);

    if (status != HAL_OK)
    {
        Test_Print("APPEND ERROR: PAGE PROGRAM\r\n");
        return status;
    }

    /*
     * Verify using a separate buffer. Do not trust a successful
     * PageProgram return as proof that the record is valid.
     */
    status = Flash_Read(
        address,
        readback,
        SENSOR_RECORD_SIZE_BYTES,
        hspi);

    if (status != HAL_OK)
    {
        Test_Print("APPEND ERROR: READBACK\r\n");
        return status;
    }

    classification = SensorRecord_Classify(readback);

if (classification != SENSOR_RECORD_VALID)
{
    if (classification == SENSOR_RECORD_BLANK)
    {
        Test_Print("APPEND ERROR: READBACK BLANK\r\n");
    }
    else
    {
        Test_Print("APPEND ERROR: READBACK NOT VALID\r\n");
    }

    return HAL_ERROR;
}

/*
 * A valid CRC alone does not establish that this is the
 * exact record we intended to program.
 */
if (memcmp(readback, record_bytes, SENSOR_RECORD_SIZE_BYTES) != 0)
{
    Test_Print("APPEND ERROR: READBACK MISMATCH\r\n");
    return HAL_ERROR;
}

    Test_Print("APPEND VERIFIED slot=0x");
    Test_PrintHex32(slot);
    Test_Print(" seq=0x");
    Test_PrintHex32(record->seq);
    Test_Print("\r\n");

    return HAL_OK;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  __HAL_SPI_ENABLE(&hspi1);   /* drive SCK to Mode 0 idle low before CS is ever asserted */
  /* USER CODE END 2 */

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  #if PROVISION_LOG

    if (ProvisionLog() == HAL_OK)
    {
        Test_Print(
            "PROVISION HALTED. FLASH NORMAL BUILD.\r\n");
    }
    else
    {
        Test_Print(
            "PROVISION HALTED. FAILURE REPORTED ABOVE.\r\n");
    }

    while (1)
    {
    }

  #endif

    /* USER CODE BEGIN RECOVERY_SCAN */

  ScanConfig recovery_config = {
      .read = Recovery_FlashRead,
      .classify = SensorRecord_Classify,
      .context = &hspi1,
      .base_address = FLASH_LOG_BASE_ADDRESS,
      .slot_count = FLASH_LOG_SLOT_COUNT
  };

  ScanResult recovery_result;

  uint32_t recovery_start = HAL_GetTick();

  ScanStatus recovery_status =
      RecoveryScan(&recovery_config, &recovery_result);

  uint32_t recovery_elapsed =
      HAL_GetTick() - recovery_start;

  Test_Print("RECOVERY status=");
  Test_Print(RecoveryStatusName(recovery_status));

  Test_Print(" append_slot=0x");
  Test_PrintHex32(recovery_result.append_slot);

  Test_Print(" next_seq=0x");
  Test_PrintHex32(recovery_result.next_seq);

  Test_Print(" elapsed_ticks=0x");
  Test_PrintHex32(recovery_elapsed);

  Test_Print("\r\n");

  switch (recovery_status)
  {
      case SCAN_OK:
          if (recovery_result.append_slot < FLASH_LOG_SLOT_COUNT)
          {
              Test_Print(
                  "RECOVERY READY. LOGGER STARTING.\r\n");
          }
          else
          {
              Test_Print(
                  "RECOVERY FAULT. INVALID APPEND SLOT.\r\n");
          }
          break;

      case SCAN_FULL:
          Test_Print(
              "RECOVERY FULL. WRITES DISABLED.\r\n");
          break;

      case SCAN_IO_ERROR:
          Test_Print(
              "RECOVERY IO FAULT. WRITES DISABLED.\r\n");
          break;

      case SCAN_BAD_ARGUMENT:
      default:
          Test_Print(
              "RECOVERY CONFIG FAULT. WRITES DISABLED.\r\n");
          break;
  }


/*
 * Continuous logger.
 * Recovery must succeed before any sensor or storage activity.
 */
if ((recovery_status == SCAN_OK) &&
    (recovery_result.append_slot < FLASH_LOG_SLOT_COUNT))
{
    uint8_t calib_1[Block_1_Length];
    uint8_t calib_2[Block_2_Length];
    BME280_Calibration calibration;

    uint32_t append_slot = recovery_result.append_slot;
    uint32_t next_seq = recovery_result.next_seq;
    uint32_t next_sample_tick = HAL_GetTick();

    HAL_StatusTypeDef sensor_status;

    sensor_status = BME280_Configure(&hi2c1);

    if (sensor_status != HAL_OK)
    {
        Test_Print("SENSOR ERROR: CONFIGURE. LOGGER STOPPED.\r\n");
        while (1) { HAL_Delay(1000); }
    }

    sensor_status = BME280_ReadCalibration(
        &hi2c1, calib_1, calib_2);

    if (sensor_status != HAL_OK)
    {
        Test_Print("SENSOR ERROR: CALIBRATION. LOGGER STOPPED.\r\n");
        while (1) { HAL_Delay(1000); }
    }

    BME280_ParseCalibration(
        calib_1, calib_2, &calibration);

    Test_Print("LOGGER STARTED period_ms=0x000000FA\r\n");

    while (1)
    {
        uint32_t now;

        /*
         * Wait for the next scheduled sample start.
         * Recheck the schedule after every delay.
         */
        while ((int32_t)(HAL_GetTick() - next_sample_tick) < 0)
        {
            HAL_Delay(1);
        }

        /*
         * A long operation or debugger pause may have caused
         * scheduled sample slots to be missed.
         * Skip them as a group, without writing flash.
         */
        now = HAL_GetTick();

        if ((int32_t)(now - next_sample_tick) >= 250)
        {
            uint32_t late_ms = now - next_sample_tick;
            uint32_t missed = (late_ms / 250U) + 1U;
            uint32_t first_skipped_seq = next_seq;
            uint32_t last_skipped_seq;

            /*
             * Never permit sequence-number wraparound.
             */
            if ((next_seq == UINT32_MAX) ||
                (missed > (UINT32_MAX - next_seq)))
            {
                Test_Print(
                    "LOGGER FAULT: SEQUENCE EXHAUSTED. WRITES STOPPED.\r\n");
                while (1) { HAL_Delay(1000); }
            }

            last_skipped_seq = next_seq + missed - 1U;
            next_seq += missed;
            next_sample_tick += missed * 250U;

            Test_Print("SAMPLES SKIPPED first_seq=0x");
            Test_PrintHex32(first_skipped_seq);
            Test_Print(" last_seq=0x");
            Test_PrintHex32(last_skipped_seq);
            Test_Print("\r\n");

            /*
             * Return to the top to recheck the deadline.
             * Printing may itself have taken time.
             */
            continue;
        }

        /*
         * A full log is an expected terminal condition,
         * distinct from a storage fault.
         */
        if (append_slot >= FLASH_LOG_SLOT_COUNT)
        {
            Test_Print("LOG FULL. WRITES STOPPED.\r\n");
            while (1) { HAL_Delay(1000); }
        }

        /*
         * A sequence of UINT32_MAX is reserved as the
         * exhausted state, preventing increment wraparound.
         */
        if (next_seq == UINT32_MAX)
        {
            Test_Print(
                "LOGGER FAULT: SEQUENCE EXHAUSTED. WRITES STOPPED.\r\n");
            while (1) { HAL_Delay(1000); }
        }

        int32_t temperature_centi_c;
        uint32_t pressure_q24_8;
        uint32_t humidity_q22_10;

        SensorRecord record;
        SensorRecordStatus record_status;

        sensor_status = BME280_ReadMeasurement(
            &hi2c1,
            &calibration,
            &temperature_centi_c,
            &pressure_q24_8,
            &humidity_q22_10);

        if (sensor_status != HAL_OK)
        {
            Test_Print("LOGGER FAULT: SENSOR READ FAILED. WRITES STOPPED.\r\n");
            while (1) { HAL_Delay(1000); }
        }

        record_status = SensorRecord_Build(
            &record,
            next_seq,
            temperature_centi_c,
            pressure_q24_8,
            humidity_q22_10);

        if (record_status != SENSOR_RECORD_OK)
        {
            Test_Print("LOGGER FAULT: RECORD BUILD FAILED. WRITES STOPPED.\r\n");
            while (1) { HAL_Delay(1000); }
        }

        /*
         * Print the stored engineering units, not raw sensor
         * fixed-point values. No floating-point formatting.
         */
        {
            char line[128];
            int32_t temp = record.temperature_centi_c;
            uint32_t temp_magnitude =
                (temp < 0) ? (uint32_t)(-temp) : (uint32_t)temp;

            if (temp < 0)
            {
                (void)snprintf(
                    line, sizeof(line),
                    "SAMPLE seq=0x%08lX temp=-%lu.%02luC pressure=%luPa humidity=%lu.%02lu%%RH\r\n",
                    (unsigned long)record.seq,
                    (unsigned long)(temp_magnitude / 100U),
                    (unsigned long)(temp_magnitude % 100U),
                    (unsigned long)record.pressure_pa,
                    (unsigned long)(record.humidity_centi_pct / 100U),
                    (unsigned long)(record.humidity_centi_pct % 100U));
            }
            else
            {
                (void)snprintf(
                    line, sizeof(line),
                    "SAMPLE seq=0x%08lX temp=%lu.%02luC pressure=%luPa humidity=%lu.%02lu%%RH\r\n",
                    (unsigned long)record.seq,
                    (unsigned long)(temp_magnitude / 100U),
                    (unsigned long)(temp_magnitude % 100U),
                    (unsigned long)record.pressure_pa,
                    (unsigned long)(record.humidity_centi_pct / 100U),
                    (unsigned long)(record.humidity_centi_pct % 100U));
            }

            Test_Print(line);
        }

        /*
         * Logger_AppendRecord performs the destination blank
         * check, one program attempt, readback classification,
         * and exact byte comparison.
         */
        if (Logger_AppendRecord(
                append_slot, &record, &hspi1) != HAL_OK)
        {
            Test_Print(
                "LOGGER FAULT: APPEND FAILED, WRITES STOPPED.\r\n");
            while (1) { HAL_Delay(1000); }
        }

        /*
         * Advance state only after verified append.
         * Skipped scheduled slots advance next_seq separately.
         */
        append_slot++;
        next_seq++;

        if (append_slot >= FLASH_LOG_SLOT_COUNT)
        {
            Test_Print("LOG FULL. WRITES STOPPED.\r\n");
            while (1) { HAL_Delay(1000); }
        }

        /*
         * Schedule the next sample start relative to the
         * previous scheduled start, not completion time.
         */
        next_sample_tick += 250U;
    }
}

  /* USER CODE END RECOVERY_SCAN */
  
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
