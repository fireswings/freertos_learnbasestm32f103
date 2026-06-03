#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include "cmsis_os.h"

#define KEY0 HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin)
#define KEY1 HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin)
#define KEY_UP HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin)

#define KEY0_PRESS 0
#define KEY1_PRESS 0
#define KEY_UP_PRESS 1
extern osThreadId_t lcdTaskHandle;

void Key_Process(void);

#endif
