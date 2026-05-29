# onewire 1-Wire 总线驱动使用文档

## 概述
通过 GPIO 位操作实现 Dallas 1-Wire 总线协议，用于 DS18B20 等从设备通信。使用 DWT 周期计数器实现微秒级精准延时。

## 硬件
| 引脚 | GPIO |
|------|------|
| DQ | PG11 (开漏输出，上拉) |

## API

```c
#include "onewire.h"

void    OW_Init(void);                         // 初始化 DWT 延时 + 释放总线
void    OW_DelayUs(uint16_t us);               // 微秒延时（供其他驱动复用）
uint8_t OW_Reset(void);                        // 复位总线，返回 1=有设备, 0=无设备
void    OW_WriteByte(uint8_t data);            // 写入一个字节
uint8_t OW_ReadByte(void);                     // 读取一个字节
```

## 时序保护
- 所有位操作（读/写/复位）内部使用 `__disable_irq()` / `__enable_irq()` 保护关键时序段
- 单次禁用中断最长时间：写一个字节约 500µs，读取一个字节约 112µs，复位约 540µs

## 使用示例
```c
OW_Init();

// 复位检测设备
if (OW_Reset())
{
    // 有设备存在
    OW_WriteByte(0xCC);  // Skip ROM
    OW_WriteByte(0x44);  // Convert T (启动转换)
}
```

## 注意事项
- 依赖 `main.h` 中的 `OW_Pin`、`OW_GPIO_Port` 宏定义
- 依赖 `SystemCoreClock`（72MHz）计算微秒延时
- 本驱动仅提供原始字节收发，具体传感器协议由上层驱动实现
