# key 按键驱动使用文档

## 概述
按键驱动封装 GPIO 输入消抖逻辑，支持 KEY0/KEY1/KEY_UP 三个按键，每个按键**按一次翻转一次**目标 LED。

## 硬件对应
| 按键 | GPIO | 有效电平 | 目标 |
|------|------|----------|------|
| KEY0 | PE4 | 低（上拉） | 翻转 LED0 (PB5) |
| KEY1 | PE3 | 低（上拉） | 翻转 LED1 (PE5) |
| KEY_UP | PA0 | 高（下拉） | 同时翻转 LED0 + LED1 |

## API

```c
#include "key.h"

void Key_Process(void);  // 轮询处理函数，需在任务中周期调用
```

## 参数说明
- **消抖时间**: `KEY_DEBOUNCE_MS` = 20ms（定义在 key.c）
- 检测到按下后在 `while` 中阻塞等待松开（每 5ms 检查一次），松开后才执行动作
- 防止按下一次触发多次翻转

## 使用示例
```c
// 在 FreeRTOS 任务中每 10ms 调用一次
void StartKeyTask(void *argument)
{
    for (;;)
    {
        Key_Process();
        osDelay(10);
    }
}
```

## 注意事项
- `Key_Process()` 内部使用 `osDelay(5)` 等待松键，仅在 FreeRTOS 环境下使用
- 依赖 `main.h` 中的 `LED0_Pin`、`LED1_Pin`、`LED1_GPIO_Port` 宏定义
