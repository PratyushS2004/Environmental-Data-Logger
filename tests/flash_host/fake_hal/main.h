#ifndef FAKE_MAIN_H
#define FAKE_MAIN_H

#include <stdint.h>

typedef enum {
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct {
    uint32_t unused;
} SPI_HandleTypeDef;

typedef struct {
    uint32_t unused;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET = 1U
} GPIO_PinState;

#define GPIO_PIN_6 ((uint16_t)0x0040U)

extern GPIO_TypeDef fake_GPIOB;
#define GPIOB (&fake_GPIOB)

void HAL_GPIO_WritePin(
    GPIO_TypeDef *GPIOx,
    uint16_t GPIO_Pin,
    GPIO_PinState PinState
);

HAL_StatusTypeDef HAL_SPI_Transmit(
    SPI_HandleTypeDef *hspi,
    const uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout
);

HAL_StatusTypeDef HAL_SPI_Receive(
    SPI_HandleTypeDef *hspi,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout
);

uint32_t HAL_GetTick(void);

#endif