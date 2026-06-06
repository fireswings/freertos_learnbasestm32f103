#include "breath_led.h"
#include <math.h>

TIM_HandleTypeDef htim3;

static uint16_t gamma_lut[BL_LUT_SIZE];
static uint16_t bl_period_ms = 2000;
static float    bl_gamma = BL_GAMMA_DEFAULT;
static uint8_t  bl_enabled  = 1;

/**
 * @brief 构建 gamma 校正查找表
 *        lut[i] = round((i / MAX)^γ × MAX)
 *        使 PWM 的实际亮度经 gamma 编码后，人眼感知为线性
 */
static void BL_BuildGammaLUT(void)
{
    float max_f = (float)BL_PWM_MAX_DUTY;

    for (uint32_t i = 0; i < BL_LUT_SIZE; i++)
    {
        float t   = (float)i / max_f;
        float val = powf(t, bl_gamma) * max_f + 0.5f;  /* +0.5 = round */
        if (val > max_f) val = max_f;
        gamma_lut[i] = (uint16_t)val;
    }
}

/**
 * @brief TIM3 PWM MSP 初始化 — 时钟 + GPIO 复用
 */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef gpio = {0};

    if (htim->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_AFIO_CLK_ENABLE();

        /* TIM3 部分重映射: CH2 从 PA7 → PB5 (LED0) */
        __HAL_AFIO_REMAP_TIM3_PARTIAL();

        /* PB5 — TIM3_CH2, 复用推挽 */
        gpio.Pin       = LED0_Pin;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(LED0_GPIO_Port, &gpio);
    }
}

/**
 * @brief TIM3 PWM MSP 反初始化
 */
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_DISABLE();
        HAL_GPIO_DeInit(LED0_GPIO_Port, LED0_Pin);
    }
}

/**
 * @brief 初始化呼吸灯 PWM (TIM3 CH2 → PB5)
 *
 * APB1 = 36 MHz, 预分频 35 → 1 MHz, ARR = 999 → 1 kHz PWM.
 * 构建 gamma 校正表（可通过 BL_SetGamma() 调参）。
 */
void BL_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* 构造 gamma 校正查找表 */
    BL_BuildGammaLUT();

    /* TIM3 基础配置 */
    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 35;               /* 36 MHz / 36 = 1 MHz */
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = BL_PWM_PERIOD;    /* 1 MHz / 1000 = 1 kHz */
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
        Error_Handler();

    /* CH2 PWM 模式 1 */
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;                        /* 初始占空比 = 0 */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
        Error_Handler();

    /* 启动 PWM 输出 */
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
        Error_Handler();
}

/**
 * @brief 呼吸灯处理函数 — 在主循环或任务中周期调用
 *
 * 使用三角波 + gamma 校正:
 *   - 三角波保证每级感知亮度停留时间均等，无峰值"逗留"
 *   - gamma 表将线性感知亮度映射为实际的 PWM 占空比
 * 调用越快效果越平滑，建议每 5~10ms 调用一次。
 */
void BL_Process(void)
{
    if (!bl_enabled)
        return;

    uint32_t t  = HAL_GetTick() % bl_period_ms;
    float    tri;

    /*
     * 三角波: t ∈ [0, period)
     *   前半段: 0 → 1 线性爬升
     *   后半段: 1 → 0 线性下降
     * 无需 sin/sinf, 每级亮度停留时间均等。
     */
    float half = (float)bl_period_ms * 0.5f;
    if ((float)t < half)
        tri = (float)t / half;               /* 上升段 0 → 1 */
    else
        tri = (float)(bl_period_ms - t) / half; /* 下降段 1 → 0 */

    /* 映射到 PWM 阶梯后查 gamma 表 */
    uint16_t target_duty = (uint16_t)(tri * (float)BL_PWM_MAX_DUTY + 0.5f);
    if (target_duty > BL_PWM_MAX_DUTY)
        target_duty = BL_PWM_MAX_DUTY;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, gamma_lut[target_duty]);
}

/**
 * @brief 设置呼吸周期
 * @param period_ms 一个完整呼吸循环的时长 (ms), 典型值 1000~5000
 */
void BL_SetPeriod_ms(uint16_t period_ms)
{
    if (period_ms < 100)
        period_ms = 100;
    bl_period_ms = period_ms;
}

/**
 * @brief 设置 gamma 值并重建查找表
 * @param gamma gamma 指数，典型值:
 *             0.5~1.0 → 低亮区拉伸（暗部细节多），视觉效果偏亮
 *             2.0~2.4 → 接近人眼感知特征
 *             2.8~3.5 → 低亮区压缩更明显，亮部过渡更快
 */
void BL_SetGamma(float gamma)
{
    if (gamma < 0.1f)
        gamma = 0.1f;
    if (gamma > 5.0f)
        gamma = 5.0f;
    bl_gamma = gamma;
    BL_BuildGammaLUT();
}

/**
 * @brief 获取当前 gamma 值
 * @return float 当前 gamma 指数
 */
float BL_GetGamma(void)
{
    return bl_gamma;
}

/**
 * @brief 使能/关闭呼吸灯
 * @param enable 1 = 呼吸, 0 = 熄灭
 */
void BL_Enable(uint8_t enable)
{
    bl_enabled = enable;
    if (!enable)
    {
        /* 关闭 PWM 输出 */
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    }
}
