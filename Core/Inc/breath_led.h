#ifndef __BREATH_LED_H
#define __BREATH_LED_H

#include "main.h"

#define BL_PWM_PERIOD       999     /* ARR, 1000 级占空比分辨率 */
#define BL_PWM_MAX_DUTY     999
#define BL_LUT_SIZE         (BL_PWM_MAX_DUTY + 1)

/*
 * Gamma 说明:
 *   - γ < 1: 低亮区被拉伸，高亮区被压缩，LED 会更早亮起
 *   - γ = 1: 线性，无校正
 *   - γ = 2.2: 标准 sRGB gamma，人眼感知最接近线性
 *   - γ > 2.2: 低亮区更"集中"，高亮区跳变更快
 */
#define BL_GAMMA_DEFAULT    2.2f

void BL_Init(void);
void BL_Process(void);
void BL_SetPeriod_ms(uint16_t period_ms);
void BL_SetGamma(float gamma);
float BL_GetGamma(void);
void BL_Enable(uint8_t enable);

extern TIM_HandleTypeDef htim3;

#endif /* __BREATH_LED_H */
