# light_sensor 光敏传感器驱动使用文档

## 概述
基于 STM32F103 ADC3 的光敏传感器（LS1）驱动，通过 ADC 采样光敏电阻分压值，输出光照强度百分比。

## 硬件
| 信号 | GPIO | ADC | 说明 |
|------|------|-----|------|
| AIN | PF8 | ADC3_IN6 (Channel 6) | 光敏传感器 LS1 模拟输出 |

## API

```c
#include "light_sensor.h"

HAL_StatusTypeDef LS_DRV_Init(void);     // 初始化 ADC3 + PF8
uint16_t          LS_DRV_ReadRaw(void);   // 读取原始 ADC 值
uint8_t           LS_DRV_ReadPercent(void); // 读取光照百分比 (0–100)
```

## 参数说明

### LS_DRV_Init
- 初始化 ADC3：独立模式、单通道（CH6）、连续转换、软件触发
- 采样时间：239.5 周期（最长，保证光敏电阻这类高阻抗源的精度）
- PF8 配置为模拟输入（无上下拉）
- 返回：`HAL_OK` 成功，`HAL_ERROR` 失败（会调用 `Error_Handler()`）

### LS_DRV_ReadRaw
- 返回：uint16_t 原始 ADC 值（0–4095）
- 值越大表示光照越强（光敏电阻阻值越小，分压越高）
- 阻塞等待当前转换完成（`HAL_MAX_DELAY`）

### LS_DRV_ReadPercent
- 返回：uint8_t 0–100 百分比
- 内部 8 次采样取平均，降低噪声和波动
- 计算公式：`avg / 4095 × 100`

## 使用示例

### 基本用法
```c
#include "light_sensor.h"
#include <stdio.h>

// 1. 初始化
if (LS_DRV_Init() != HAL_OK)
{
    printf("Light sensor init failed\r\n");
}

// 2. 读取原始值
uint16_t raw = LS_DRV_ReadRaw();
printf("LS raw: %d\r\n", raw);          // LS raw: 2048

// 3. 读取百分比
uint8_t percent = LS_DRV_ReadPercent();
printf("Light: %d%%\r\n", percent);    // Light: 50%
```

### FreeRTOS 任务中定时读取
```c
void LightSensorTask(void *argument)
{
    LS_DRV_Init();

    for (;;)
    {
        uint8_t light = LS_DRV_ReadPercent();

        if (light < 20)
            printf("环境较暗 (%d%%)\r\n", light);
        else if (light > 80)
            printf("环境较亮 (%d%%)\r\n", light);

        osDelay(1000);  // 每 1s 读取一次
    }
}
```

### 光控 LED 示例
```c
void LightControlTask(void *argument)
{
    LS_DRV_Init();

    for (;;)
    {
        uint8_t light = LS_DRV_ReadPercent();

        if (light < 30)
        {
            HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);  // 亮灯
        }
        else
        {
            HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);    // 熄灯
        }

        osDelay(500);
    }
}
```

## 注意事项
- 依赖 HAL ADC 模块，需在 `stm32f1xx_hal_conf.h` 启用 `HAL_ADC_MODULE_ENABLED`
- ADC 连续转换模式：`LS_DRV_ReadRaw()` 读取的是最近一次转换结果，不需要每次都启动转换
- `LS_DRV_ReadPercent()` 内部调用 8 次 `ReadRaw`，单次调用约耗时 8 × 采样时间，适合低频（≥100ms）场景
- 如需要更高频率采样，直接使用 `LS_DRV_ReadRaw()` 并在应用层做滤波
- 光敏电阻在不同温度下特性会有漂移，百分比仅为近似值
