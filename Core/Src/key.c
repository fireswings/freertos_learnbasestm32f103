/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    key.c
  * @brief   Key processing functions
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "key.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "lcd.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY_DEBOUNCE_MS 20
/* USER CODE END PD */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * @brief  Key processing function, called periodically from FreeRTOS task
  * @param  None
  * @retval None
  */
void Key_Process(void)
{
  /* USER CODE BEGIN Key_Process */
  static uint8_t key0_last = 1, key1_last = 1, key_up_last = 0;
  static uint8_t lcd_state = 1;
  static uint32_t key0_tick = 0, key1_tick = 0, key_up_tick = 0;
  static uint8_t eeprom_cmd;

  /* KEY0 — 按下触发 24C02 读取 */
  if (KEY0 != key0_last)
  {
    key0_tick = HAL_GetTick();
    key0_last = KEY0;
  }
  else if (KEY0 == KEY0_PRESS && (HAL_GetTick() - key0_tick) > KEY_DEBOUNCE_MS)
  {
    while (KEY0 == KEY0_PRESS)
      osDelay(5);
    eeprom_cmd = 0x00;  /* 读命令 */
    osMessageQueuePut(eepromQueueHandle, &eeprom_cmd, 0, 0);
    key0_tick = HAL_GetTick();
  }

  /* KEY1 — 按下触发 24C02 写入 */
  if (KEY1 != key1_last)
  {
    key1_tick = HAL_GetTick();
    key1_last = KEY1;
  }
  else if (KEY1 == KEY1_PRESS && (HAL_GetTick() - key1_tick) > KEY_DEBOUNCE_MS)
  {
    while (KEY1 == KEY1_PRESS)
      osDelay(5);
    eeprom_cmd = 0x01;  /* 写命令 */
    osMessageQueuePut(eepromQueueHandle, &eeprom_cmd, 0, 0);
    key1_tick = HAL_GetTick();
  }

  /* KEY_UP — 按下翻转LED0和LED1 */
  if (KEY_UP != key_up_last)
  {
    key_up_tick = HAL_GetTick();
    key_up_last = KEY_UP;
  }
  else if (KEY_UP == KEY_UP_PRESS && (HAL_GetTick() - key_up_tick) > KEY_DEBOUNCE_MS)
  {
    while (KEY_UP == KEY_UP_PRESS)
      osDelay(5);
    if(lcd_state)
    {
      LCD_BL(0);
      osThreadSuspend(lcdTaskHandle);
      lcd_display_off();
      lcd_state = 0;
    }
    else
    {
      LCD_BL(1);
      lcd_display_on();
      osThreadResume(lcdTaskHandle);
      lcd_state = 1;
    }
    key_up_tick = HAL_GetTick();
  }
  /* USER CODE END Key_Process */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
