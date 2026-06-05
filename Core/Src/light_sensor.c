#include "light_sensor.h"

static ADC_HandleTypeDef hadc3;

/**
 * @brief ADC3 MSP 初始化 — 时钟、GPIO
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef gpio = {0};

    if (hadc->Instance == LS_ADC_INSTANCE)
    {
        /* 使能 ADC3 和 GPIOF 时钟 */
        __HAL_RCC_ADC3_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();

        /* PF8 — ADC3_IN6, 模拟输入 */
        gpio.Pin  = LS_ADC_PIN;
        gpio.Mode = GPIO_MODE_ANALOG;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(LS_ADC_PORT, &gpio);
    }
}

/**
 * @brief 初始化光敏传感器 (ADC3 Channel 6 → PF8)
 *
 * 独立模式、软件触发、单通道轮询采样。
 * 采样时间 239.5 周期，最慢以保证精度。
 *
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef LS_DRV_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc3.Instance                   = LS_ADC_INSTANCE;
    hadc3.Init.ScanConvMode          = ADC_SCAN_DISABLE;       /* 单通道 */
    hadc3.Init.ContinuousConvMode    = ENABLE;                 /* 连续转换 */
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConv      = ADC_SOFTWARE_START;     /* 软件触发 */
    hadc3.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion       = 1;

    if (HAL_ADC_Init(&hadc3) != HAL_OK)
    {
        Error_Handler();
        return HAL_ERROR;
    }

    /* 配置通道 6, 采样时间设到最长以保证精度 */
    sConfig.Channel      = LS_ADC_CHANNEL;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        Error_Handler();
        return HAL_ERROR;
    }

    /* 启动 ADC3 */
    if (HAL_ADC_Start(&hadc3) != HAL_OK)
    {
        Error_Handler();
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 读取光敏传感器原始 ADC 值
 * @return uint16_t ADC 原始值 (0-4095)，值越大光线越强
 */
uint16_t LS_DRV_ReadRaw(void)
{
    /* 等待转换完成并获取值 */
    HAL_ADC_PollForConversion(&hadc3, HAL_MAX_DELAY);
    return (uint16_t)HAL_ADC_GetValue(&hadc3);
}

/**
 * @brief 读取光敏传感器百分比 (0 = 最暗, 100 = 最亮)
 * @return uint8_t 0-100
 */
uint8_t LS_DRV_ReadPercent(void)
{
    uint32_t sum = 0;

    /* 多次采样取平均，降低噪声 */
    for (uint8_t i = 0; i < 8; i++)
    {
        sum += LS_DRV_ReadRaw();
    }
    sum /= 8;

    return (uint8_t)(100 - (sum * 100 / 4095));
}
