#include <stdio.h>
#include <stdint.h>

#include "w25q64.h"
#include "mock_hal.h"

static uint32_t Test_MockFlashRead(void)
{
    SPI_HandleTypeDef spi = {0};

    const uint8_t seed_data[4] = {
        0x11U, 0x22U, 0x33U, 0x44U
    };

    const uint8_t read_command[4] = {
        0x03U, 0x00U, 0x01U, 0x00U
    };

    uint8_t received_data[4] = {0};
    uint32_t failures = 0U;
    HAL_StatusTypeDef result;

    Mock_Reset();

    if (!MockFlash_Seed(0x100U, seed_data, sizeof(seed_data))) {
        printf("FAIL mock read: could not seed flash\n");
        return 1U;
    }

    /* Begin one CS-delimited Read Data transaction. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    result = HAL_SPI_Transmit(
        &spi, read_command, sizeof(read_command), 100U
    );

    if (result != HAL_OK) {
        printf("FAIL mock read: transmit returned %u\n",
               (unsigned int)result);
        failures++;
    }

    if (result == HAL_OK) {
        result = HAL_SPI_Receive(
            &spi, received_data, sizeof(received_data), 100U
        );

        if (result != HAL_OK) {
            printf("FAIL mock read: receive returned %u\n",
                   (unsigned int)result);
            failures++;
        }
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

    for (size_t i = 0U; i < sizeof(seed_data); i++) {
        if (received_data[i] != seed_data[i]) {
            printf(
                "FAIL mock read: byte %u expected 0x%02X, got 0x%02X\n",
                (unsigned int)i,
                (unsigned int)seed_data[i],
                (unsigned int)received_data[i]
            );
            failures++;
        }
    }

    if (mock_tx_calls != 1U || mock_tx_bytes != 4U) {
        printf("FAIL mock read: expected TX 1 call / 4 bytes, got %u / %u\n",
               (unsigned int)mock_tx_calls,
               (unsigned int)mock_tx_bytes);
        failures++;
    }

    if (mock_rx_calls != 1U || mock_rx_bytes != 4U) {
        printf("FAIL mock read: expected RX 1 call / 4 bytes, got %u / %u\n",
               (unsigned int)mock_rx_calls,
               (unsigned int)mock_rx_bytes);
        failures++;
    }

    if (mock_cs_writes != 2U) {
        printf("FAIL mock read: expected 2 CS writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (mock_command_count != 1U || mock_command != 0x03U) {
        printf("FAIL mock read: expected one 0x03 command, got count=%u command=0x%02X\n",
               (unsigned int)mock_command_count,
               (unsigned int)mock_command);
        failures++;
    }

    if (!MockFlash_CheckGuards()) {
        printf("FAIL mock read: flash guard corrupted\n");
        failures++;
    }

    if (failures == 0U) {
        printf("PASS MockFlashRead\n");
    } else {
        printf("FAIL MockFlashRead: %u check(s) failed\n",
               (unsigned int)failures);
    }

    return failures;
}

static uint32_t Test_MockFlashReadWrap(void)
{
    SPI_HandleTypeDef spi = {0};

    const uint8_t last_byte = 0x3CU;
    const uint8_t first_byte = 0x5AU;
    const uint8_t read_command[4] = {
        0x03U, 0x7FU, 0xFFU, 0xFFU
    };

    uint8_t received[2] = {0xA5U, 0xA5U};
    uint32_t failures = 0U;
    HAL_StatusTypeDef result;

    Mock_Reset();

    if (!MockFlash_Seed(0x7FFFFFU, &last_byte, 1U)) {
        printf("FAIL MockFlashReadWrap: could not seed last byte\n");
        return 1U;
    }

    if (!MockFlash_Seed(0x000000U, &first_byte, 1U)) {
        printf("FAIL MockFlashReadWrap: could not seed first byte\n");
        return 1U;
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    result = HAL_SPI_Transmit(
        &spi, read_command, sizeof(read_command), 100U
    );

    if (result == HAL_OK) {
        result = HAL_SPI_Receive(
            &spi, received, sizeof(received), 100U
        );
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

    if (result != HAL_OK) {
        printf("FAIL MockFlashReadWrap: SPI returned %u\n",
               (unsigned int)result);
        failures++;
    }

    if (received[0] != last_byte || received[1] != first_byte) {
        printf("FAIL MockFlashReadWrap: expected {0x3C, 0x5A}, got {0x%02X, 0x%02X}\n",
               (unsigned int)received[0],
               (unsigned int)received[1]);
        failures++;
    }

    if (mock_read_wrap_bytes != 1U) {
        printf("FAIL MockFlashReadWrap: expected 1 wrapped byte, got %u\n",
               (unsigned int)mock_read_wrap_bytes);
        failures++;
    }

    if (!MockFlash_CheckGuards()) {
        printf("FAIL MockFlashReadWrap: flash guard corrupted\n");
        failures++;
    }

    if (mock_command_count != 1U || mock_command != 0x03U) {
        printf("FAIL MockFlashReadWrap: incorrect command tracking\n");
        failures++;
    }

    if (failures == 0U) {
        printf("PASS MockFlashReadWrap\n");
    } else {
        printf("FAIL MockFlashReadWrap: %u check(s) failed\n",
               (unsigned int)failures);
    }

    return failures;
}

static uint32_t Test_FlashReadBoundary(void)
{
    SPI_HandleTypeDef spi = {0};

    uint8_t received[2] = {0xC7U, 0xD9U};
    uint32_t failures = 0U;
    HAL_StatusTypeDef result;

    Mock_Reset();

    result = Flash_Read(0x7FFFFFU, received, 2U, &spi);

    if (result != HAL_ERROR) {
        printf("FAIL FlashReadBoundary: expected HAL_ERROR, got %u\n",
               (unsigned int)result);
        failures++;
    }

    if (received[0] != 0xC7U || received[1] != 0xD9U) {
        printf("FAIL FlashReadBoundary: output buffer changed\n");
        failures++;
    }

    if (mock_tx_calls != 0U || mock_rx_calls != 0U) {
        printf("FAIL FlashReadBoundary: SPI activity occurred "
               "(TX=%u, RX=%u)\n",
               (unsigned int)mock_tx_calls,
               (unsigned int)mock_rx_calls);
        failures++;
    }

    if (mock_cs_writes != 0U) {
        printf("FAIL FlashReadBoundary: expected 0 CS writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (!MockFlash_CheckGuards()) {
        printf("FAIL FlashReadBoundary: flash guard corrupted\n");
        failures++;
    }

    if (failures == 0U) {
        printf("PASS FlashReadBoundary\n");
    } else {
        printf("FAIL FlashReadBoundary: %u check(s) failed\n",
               (unsigned int)failures);
    }

    return failures;
}


static uint32_t Test_FlashReadRejectsAddress0x800000(void)
{
    SPI_HandleTypeDef spi = {0};

    uint8_t received = 0xC7U;
    uint32_t failures = 0U;
    HAL_StatusTypeDef result;

    Mock_Reset();

    result = Flash_Read(0x800000U, &received, 1U, &spi);

    if (result != HAL_ERROR) {
        printf("FAIL FlashReadRejectsAddress0x800000: "
               "expected HAL_ERROR, got %u\n",
               (unsigned int)result);
        failures++;
    }

    if (received != 0xC7U) {
        printf("FAIL FlashReadRejectsAddress0x800000: "
               "output buffer changed to 0x%02X\n",
               (unsigned int)received);
        failures++;
    }

    if (mock_tx_calls != 0U || mock_rx_calls != 0U) {
        printf("FAIL FlashReadRejectsAddress0x800000: "
               "SPI activity occurred (TX=%u, RX=%u)\n",
               (unsigned int)mock_tx_calls,
               (unsigned int)mock_rx_calls);
        failures++;
    }

    if (mock_cs_writes != 0U) {
        printf("FAIL FlashReadRejectsAddress0x800000: "
               "expected 0 CS writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (!MockFlash_CheckGuards()) {
        printf("FAIL FlashReadRejectsAddress0x800000: "
               "flash guard corrupted\n");
        failures++;
    }

    if (failures == 0U) {
        printf("PASS FlashReadRejectsAddress0x800000\n");
    } else {
        printf("FAIL FlashReadRejectsAddress0x800000: "
               "%u check(s) failed\n",
               (unsigned int)failures);
    }

    return failures;
}


static uint32_t Test_FlashReadRejectsAddress0x800001(void)
{
    SPI_HandleTypeDef spi = {0};

    uint8_t received = 0xC7U;
    uint32_t failures = 0U;
    HAL_StatusTypeDef result;

    Mock_Reset();

    /*
     * Isolates the address validation term:
     * capacity - address wraps in unsigned arithmetic,
     * so the length check alone does not reject this request.
     */
    result = Flash_Read(0x00800001U, &received, 1U, &spi);

    if (result != HAL_ERROR) {
        printf("FAIL FlashReadRejectsAddress0x800001: "
               "expected HAL_ERROR, got %u\n",
               (unsigned int)result);
        failures++;
    }

    if (received != 0xC7U) {
        printf("FAIL FlashReadRejectsAddress0x800001: "
               "output buffer changed to 0x%02X\n",
               (unsigned int)received);
        failures++;
    }

    if (mock_tx_calls != 0U || mock_rx_calls != 0U) {
        printf("FAIL FlashReadRejectsAddress0x800001: "
               "SPI activity occurred (TX=%u, RX=%u)\n",
               (unsigned int)mock_tx_calls,
               (unsigned int)mock_rx_calls);
        failures++;
    }

    if (mock_cs_writes != 0U) {
        printf("FAIL FlashReadRejectsAddress0x800001: "
               "expected 0 CS writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (!MockFlash_CheckGuards()) {
        printf("FAIL FlashReadRejectsAddress0x800001: "
               "flash guard corrupted\n");
        failures++;
    }

    if (failures == 0U) {
        printf("PASS FlashReadRejectsAddress0x800001\n");
    } else {
        printf("FAIL FlashReadRejectsAddress0x800001: "
               "%u check(s) failed\n",
               (unsigned int)failures);
    }

    return failures;
}


static uint32_t Test_erase_rejects_unaligned_0x1001(void)
{
    SPI_HandleTypeDef spi = {0};
    uint32_t failures = 0U;
    HAL_StatusTypeDef result;

    Mock_Reset();
    Mock_SetStatus(0x02U);

    result = Flash_SectorErase(0x1001U, &spi);

    if (result != HAL_ERROR) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected HAL_ERROR, got %u\n",
               (unsigned int)result);
        failures++;
    }

    if (mock_tx_calls != 0U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected 0 TX calls, got %u\n",
               (unsigned int)mock_tx_calls);
        failures++;
    }

    if (mock_tx_bytes != 0U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected 0 TX bytes, got %u\n",
               (unsigned int)mock_tx_bytes);
        failures++;
    }

    if (mock_rx_calls != 0U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected 0 RX calls, got %u\n",
               (unsigned int)mock_rx_calls);
        failures++;
    }

    if (mock_rx_bytes != 0U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected 0 RX bytes, got %u\n",
               (unsigned int)mock_rx_bytes);
        failures++;
    }

    if (mock_cs_writes != 0U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected 0 CS writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (mock_command_count != 0U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected 0 commands, got %u\n",
               (unsigned int)mock_command_count);
        failures++;
    }

    if (mock_command != 0x00U) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "expected last command 0x00, got 0x%02X\n",
               (unsigned int)mock_command);
        failures++;
    }

    if (!MockFlash_CheckGuards()) {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "flash guard corrupted\n");
        failures++;
    }

    if (failures == 0U) {
        printf("PASS erase_rejects_unaligned_0x1001\n");
    } else {
        printf("FAIL erase_rejects_unaligned_0x1001: "
               "%u check(s) failed\n",
               (unsigned int)failures);
    }

    return failures;
}

int main(void)
{
    SPI_HandleTypeDef spi = {0};
    uint8_t status = 0xA5U;
    HAL_StatusTypeDef result;
    uint32_t failures = 0U;

    Mock_Reset();
    Mock_SetStatus(0x03U);

    result = Flash_ReadStatus(&status, &spi);

    if (result != HAL_OK) {
        printf("FAIL Flash_ReadStatus: expected result HAL_OK, got %u\n",
               (unsigned int)result);
        failures++;
    }

    if (status != 0x03U) {
        printf("FAIL Flash_ReadStatus: expected status 0x03, got 0x%02X\n",
               (unsigned int)status);
        failures++;
    }

    if (mock_tx_calls != 1U || mock_tx_bytes != 1U) {
        printf("FAIL SPI transmit: expected 1 call / 1 byte, got %u / %u\n",
               (unsigned int)mock_tx_calls,
               (unsigned int)mock_tx_bytes);
        failures++;
    }

    if (mock_rx_calls != 1U || mock_rx_bytes != 1U) {
        printf("FAIL SPI receive: expected 1 call / 1 byte, got %u / %u\n",
               (unsigned int)mock_rx_calls,
               (unsigned int)mock_rx_bytes);
        failures++;
    }

    if (mock_cs_writes != 2U) {
        printf("FAIL chip select: expected 2 GPIO writes, got %u\n",
               (unsigned int)mock_cs_writes);
        failures++;
    }

    if (mock_command_count != 1U || mock_command != 0x05U) {
        printf("FAIL command: expected one 0x05 command, got count=%u command=0x%02X\n",
               (unsigned int)mock_command_count,
               (unsigned int)mock_command);
        failures++;
    }

    if (failures == 0U) {
        printf("PASS Flash_ReadStatus\n");
    } else {
        printf("FAIL Flash_ReadStatus: %u check(s) failed\n",
           (unsigned int)failures);
    }

    failures += Test_MockFlashRead();

    failures += Test_MockFlashReadWrap();

    failures += Test_FlashReadBoundary();

    failures += Test_FlashReadRejectsAddress0x800000();

    failures += Test_FlashReadRejectsAddress0x800001();

    failures += Test_erase_rejects_unaligned_0x1001();

    return (failures == 0U) ? 0 : 1;
}
