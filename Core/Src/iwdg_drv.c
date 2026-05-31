#include "iwdg_drv.h"

static IWDG_HandleTypeDef hiwdg;

// /*
//  * Initialize the Independent Watchdog.
//  *
//  * LSI clock ≈ 40kHz.
//  * Prescaler /32 → 1.25kHz → 0.8ms per counter tick.
//  * Reload = 5000 → timeout = 5000 × 0.8ms = 4 seconds.
//  *
//  * Must be called once, before the RTOS scheduler starts
//  * (IWDG cannot be disabled after init on STM32F1).
//  */
// void IWDG_DRV_Init(void)
// {
//     hiwdg.Instance = IWDG;
//     hiwdg.Init.Prescaler = IWDG_PRESCALER_32;   /* LSI/32 ≈ 1.25 kHz */
//     hiwdg.Init.Reload = 5000;                    /* 5000 ticks = ~4s */

//     if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
//         Error_Handler();
// }

/**
 * @brief 初始化独立看门狗
 * @param timeout_s 期望的超时时间（秒）
 * @retval HAL status
 */
HAL_StatusTypeDef IWDG_DRV_Init(uint32_t timeout_s)
{
    /* 使用LSI的典型值计算，实际需要根据芯片校准 */
    uint32_t lsi_freq = 32000;                     /* 标称32kHz，实际需校准 */
    uint32_t prescaler_div = 32;                   /* IWDG_PRESCALER_32对应的分频值 */
    uint32_t iwdg_freq = lsi_freq / prescaler_div; /* ~1000Hz */

    /* 计算重载值，IWDG重载值范围: 0-0xFFF (4095) */
    uint32_t reload_value = timeout_s * iwdg_freq;

    /* 检查是否超出最大值 */
    if (reload_value > 0xFFF)
    {
        reload_value = 0xFFF; /* 使用最大值 */
        // 或者调整预分频器
    }

    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = reload_value;

    HAL_StatusTypeDef status = HAL_IWDG_Init(&hiwdg);

    if (status != HAL_OK)
    {
        Error_Handler();
    }

    return status;
}

/* Feed (reload) the watchdog to prevent system reset. */
void IWDG_DRV_Feed(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}
