#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#include "main.h"

/* 光敏传感器硬件定义 */
#define LS_ADC_PIN          GPIO_PIN_8
#define LS_ADC_PORT         GPIOF
#define LS_ADC_INSTANCE     ADC3
#define LS_ADC_CHANNEL      ADC_CHANNEL_6

HAL_StatusTypeDef LS_DRV_Init(void);
uint16_t LS_DRV_ReadRaw(void);
uint8_t LS_DRV_ReadPercent(void);

#endif /* __LIGHT_SENSOR_H */
