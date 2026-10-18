
#include "mock_hal.h"

GPIO_TypeDef fake_GPIOB = {0};

static uint8_t configured_status;
static uint8_t cs_is_low;
static uint8_t current_command_seen;

/* Entire allocation: leading guard, usable flash, trailing guard. */
static uint8_t guarded_flash[
    MOCK_GUARD_SIZE + MOCK_FLASH_SIZE + MOCK_GUARD_SIZE
];

static uint8_t *MockFlash_Data(void)
{
    return &guarded_flash[MOCK_GUARD_SIZE];
}

/* State for the current CS-delimited transaction. */
static uint8_t transaction_header[4];
static uint8_t transaction_header_bytes;
static uint8_t transaction_address_valid;
static uint32_t transaction_address;

static uint8_t transaction_command;
static uint32_t transaction_tx_bytes;

uint32_t mock_tx_calls;
uint32_t mock_tx_bytes;
uint32_t mock_rx_calls;
uint32_t mock_rx_bytes;
uint32_t mock_cs_writes;
uint8_t mock_command;
uint32_t mock_command_count;
uint32_t mock_read_wrap_bytes;

static bool MockFlash_RangeValid(uint32_t address, size_t length)
{
    if (address > MOCK_FLASH_SIZE) {
        return false;
    }

    /* Subtraction avoids overflow in address + length. */
    if (length > (size_t)(MOCK_FLASH_SIZE - address)) {
        return false;
    }

    return true;
}

static bool Mock_CommandHasAddress(uint8_t command)
{
    return command == 0x03U ||  /* Read Data */
           command == 0x02U ||  /* Page Program */
           command == 0x20U;    /* Sector Erase */
}

static void Mock_ResetTransaction(void)
{
    transaction_command = 0U;
    transaction_tx_bytes = 0U;
    transaction_header_bytes = 0U;
    transaction_address_valid = 0U;
    transaction_address = 0U;
    current_command_seen = 0U;

    memset(transaction_header, 0, sizeof(transaction_header));
}

void Mock_Reset(void)
{
    /* Initialize both guards and the entire flash allocation. */
    memset(guarded_flash, MOCK_GUARD_VALUE, sizeof(guarded_flash));

    /* An erased W25Q64FV flash device reads as 0xFF. */
    memset(MockFlash_Data(), 0xFF, MOCK_FLASH_SIZE);

    configured_status = 0U;
    cs_is_low = 0U;
    current_command_seen = 0U;

    mock_tx_calls = 0U;
    mock_tx_bytes = 0U;
    mock_rx_calls = 0U;
    mock_rx_bytes = 0U;
    mock_cs_writes = 0U;
    mock_command = 0U;
    mock_command_count = 0U;
    mock_read_wrap_bytes = 0U;

    Mock_ResetTransaction();
}

void Mock_SetStatus(uint8_t status)
{
    configured_status = status;
}

bool MockFlash_Seed(uint32_t address,
                    const uint8_t *data,
                    size_t length)
{
    if (data == NULL && length != 0U) {
        return false;
    }

    if (!MockFlash_RangeValid(address, length)) {
        return false;
    }

    if (length != 0U) {
        memcpy(&MockFlash_Data()[address], data, length);
    }

    return true;
}

bool MockFlash_Inspect(uint32_t address,
                       uint8_t *data,
                       size_t length)
{
    if (data == NULL && length != 0U) {
        return false;
    }

    if (!MockFlash_RangeValid(address, length)) {
        return false;
    }

    if (length != 0U) {
        memcpy(data, &MockFlash_Data()[address], length);
    }

    return true;
}

bool MockFlash_CheckGuards(void)
{
    for (size_t i = 0U; i < MOCK_GUARD_SIZE; i++) {
        if (guarded_flash[i] != MOCK_GUARD_VALUE) {
            return false;
        }
    }

    const size_t trailing_guard_start =
        MOCK_GUARD_SIZE + MOCK_FLASH_SIZE;

    for (size_t i = 0U; i < MOCK_GUARD_SIZE; i++) {
        if (guarded_flash[trailing_guard_start + i] !=
            MOCK_GUARD_VALUE) {
            return false;
        }
    }

    return true;
}


void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx,
                       uint16_t GPIO_Pin,
                       GPIO_PinState PinState)
{
    if (GPIOx != GPIOB || GPIO_Pin != GPIO_PIN_6) {
        return;
    }

    mock_cs_writes++;

    if (PinState == GPIO_PIN_RESET) {
        cs_is_low = 1U;
        Mock_ResetTransaction();
    } else {
        if (cs_is_low != 0U && current_command_seen != 0U) {
            mock_command_count++;
        }

        cs_is_low = 0U;
    }
}

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi,
                                   const uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout)
{
    (void)hspi;
    (void)Timeout;

    mock_tx_calls++;
    mock_tx_bytes += Size;

    if (pData == NULL && Size != 0U) {
        return HAL_ERROR;
    }

    if (cs_is_low == 0U || Size == 0U) {
        return HAL_OK;
    }

    for (uint16_t i = 0U; i < Size; i++) {
        const uint8_t byte = pData[i];

        /* The first transmitted byte after CS falls is the command. */
        if (transaction_tx_bytes == 0U) {
            transaction_command = byte;
            mock_command = byte;
            current_command_seen = 1U;
        }

        /*
         * Accumulate the command/address header across transmit calls.
         * Once four bytes have been collected, later bytes are payload
         * and must not be interpreted as address bytes.
         */
        if (Mock_CommandHasAddress(transaction_command) &&
            transaction_header_bytes < sizeof(transaction_header)) {
            transaction_header[transaction_header_bytes] = byte;
            transaction_header_bytes++;

            if (transaction_header_bytes == sizeof(transaction_header)) {
                transaction_address =
                    ((uint32_t)transaction_header[1] << 16) |
                    ((uint32_t)transaction_header[2] << 8) |
                    (uint32_t)transaction_header[3];

                transaction_address_valid = 1U;
            }
        }

        transaction_tx_bytes++;
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi,
                                  uint8_t *pData,
                                  uint16_t Size,
                                  uint32_t Timeout)
{
    (void)hspi;
    (void)Timeout;

    mock_rx_calls++;
    mock_rx_bytes += Size;

    if (pData == NULL && Size != 0U) {
        return HAL_ERROR;
    }

    if (cs_is_low == 0U) {
        return HAL_ERROR;
    }

    if (Size == 0U) {
        return HAL_OK;
    }

    if (transaction_command == 0x05U) {
        /* Read Status Register. Preserve the existing status behavior. */
        for (uint16_t i = 0U; i < Size; i++) {
            pData[i] = configured_status;
        }

        return HAL_OK;
    }

    if (transaction_command == 0x03U &&
        transaction_address_valid != 0U) {
        uint8_t *flash = MockFlash_Data();

        for (uint16_t i = 0U; i < Size; i++) {
            if (transaction_address >= MOCK_FLASH_SIZE) {
                mock_read_wrap_bytes++;

                /*
                 * Current mock policy: wrap to address zero.
                 * Verify this assumption against the W25Q64FV
                 * datasheet before treating it as chip behavior.
                 */
                transaction_address = 0U;
            }

            pData[i] = flash[transaction_address];
            transaction_address++;
        }

        return HAL_OK;
    }

    return HAL_ERROR;
}

uint32_t HAL_GetTick(void)
{
    return 0U;
}