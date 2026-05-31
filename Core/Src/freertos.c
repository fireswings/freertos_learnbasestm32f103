/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "key.h"
#include "usart.h"
#include "app_temp.h"
#include "app_lcd.h"
#include "iwdg_drv.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Definitions for keyTask */
osThreadId_t keyTaskHandle;
const osThreadAttr_t keyTask_attributes = {
    .name = "keyTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for tempTask */
osThreadId_t tempTaskHandle;
const osThreadAttr_t tempTask_attributes = {
    .name = "tempTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for lcdTask */
osThreadId_t lcdTaskHandle;
const osThreadAttr_t lcdTask_attributes = {
    .name = "lcdTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartKeyTask(void *argument);
void StartTempTask(void *argument);
void StartLcdTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of keyTask */
  keyTaskHandle = osThreadNew(StartKeyTask, NULL, &keyTask_attributes);

  /* creation of tempTask */
  tempTaskHandle = osThreadNew(StartTempTask, NULL, &tempTask_attributes);

  /* creation of lcdTask */
  lcdTaskHandle = osThreadNew(StartLcdTask, NULL, &lcdTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  IWDG_DRV_Feed(); /* feed as early as possible */
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("System initialized, FreeRTOS running!\r\n");

  uint32_t led_last = osKernelGetTickCount();
  uint32_t wdg_last = osKernelGetTickCount();
  uint32_t now;

  for (;;)
  {
    now = osKernelGetTickCount();

    if (now - led_last >= 500)
    {
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      led_last = now;
    }

    if (now - wdg_last >= 500)
    {
      IWDG_DRV_Feed();
      wdg_last = now;
    }

    if (uart1_data_ready())
    {
      printf("%s", (char *)uart1_rx_buf);
      USART1_StartRx();
    }

    osDelay(1); /* yield to scheduler */
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartKeyTask */
/**
 * @brief  Function implementing the keyTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartKeyTask */
void StartKeyTask(void *argument)
{
  /* USER CODE BEGIN StartKeyTask */
  /* Infinite loop */
  for (;;)
  {
    Key_Process();
    osDelay(10);
  }
  /* USER CODE END StartKeyTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
