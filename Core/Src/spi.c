#include "spi.h"

SPI_HandleTypeDef hspi2;

/**
 * @brief SPI2 MSP 初始化 — 时钟、GPIO 配置
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio = {0};

    if (hspi->Instance == SPI2)
    {
        /* 使能 SPI2 和 GPIO 时钟 */
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* SCK  - PB13 */
        gpio.Pin   = SPI2_SCK_PIN;
        gpio.Mode  = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI2_SCK_PORT, &gpio);

        /* MISO - PB14 */
        gpio.Pin   = SPI2_MISO_PIN;
        gpio.Mode  = GPIO_MODE_INPUT;
        gpio.Pull  = GPIO_PULLUP;
        HAL_GPIO_Init(SPI2_MISO_PORT, &gpio);

        /* MOSI - PB15 */
        gpio.Pin   = SPI2_MOSI_PIN;
        gpio.Mode  = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI2_MOSI_PORT, &gpio);

        /* Flash CS  - PB12 (推挽输出, 初始拉高 = 不选中) */
        gpio.Pin   = FLASH_CS_PIN;
        gpio.Mode  = GPIO_MODE_OUTPUT_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio.Pull  = GPIO_NOPULL;
        HAL_GPIO_Init(FLASH_CS_PORT, &gpio);
        SPI_CS_HIGH(FLASH_CS_PORT, FLASH_CS_PIN);
    }
}

/**
 * @brief 初始化 SPI2 外设
 *
 * APB1 = 36 MHz, 256 分频 → 140.625 kHz,
 * CPOL=0 / CPHA=0 (Mode 0), 8-bit, MSB first.
 *
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef SPI_DRV_Init(void)
{
    hspi2.Instance = SPI2;

    /*
     * STM32F103 APB1 最高 36 MHz.
     * Prescaler 256 → 36M/256 = 140.625 kHz
     * 对于 NOR Flash 和 NRF24L01 都很安全。
     */
    hspi2.Init.Mode              = SPI_MODE_MASTER;
    hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;   /* CPOL = 0 */
    hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;     /* CPHA = 0 */
    hspi2.Init.NSS               = SPI_NSS_SOFT;        /* 软件管理片选 */
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial     = 7;

    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        Error_Handler();
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t spi2_read_write_byte(uint8_t tx_byte)
{
    HAL_SPI_TransmitReceive(&hspi2, &tx_byte, NULL, 1, 100);
    return tx_byte;
}

void spi2_set_speed(uint8_t speed)
{
    assert_param(IS_SPI_BAUDRATEPRESCALER(speed));
    __HAL_SPI_DISABLE(&hspi2);
    hspi2.Instance->CR1 &= 0xffc7;
    hspi2.Instance->CR1 |= speed<<3;
    __HAL_SPI_ENABLE(&hspi2);
}