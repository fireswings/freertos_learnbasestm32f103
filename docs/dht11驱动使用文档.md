# dht11 DHT11 温湿度传感器驱动使用文档

## 概述
基于 GPIO 位操作的 DHT11 数字温湿度传感器驱动（使用单总线专用协议，非 Dallas 1-Wire）。

## 硬件
| 信号 | 引脚 |
|------|------|
| DATA | PG11 (开漏输出，与 DS18B20 复用同一引脚) |

## API

```c
#include "dht11.h"

typedef struct {
    float temperature;   // 温度 °C
    float humidity;      // 湿度 %
} DHT11_Data;

uint8_t DHT11_Init(void);                // 初始化，等待传感器就绪
uint8_t DHT11_Read(DHT11_Data *data);    // 读取一次温湿度，返回 1=成功, 0=校验失败
```

## 参数说明
- `DHT11_Init()`：调用 `OW_Init()` 初始化 DWT，释放总线并等待 1s 稳定
- `DHT11_Read()`：发送起始信号 → 等待应答 → 读取 40 位数据 → 校验和验证
- 读取失败常见原因：传感器不在线、总线冲突（同时挂了 DS18B20）、时序被中断打断

## 使用示例
```c
DHT11_Data data;
if (DHT11_Read(&data))
{
    printf("Temp: %.1f C, Humidity: %.1f%%\r\n",
           data.temperature, data.humidity);
}
else
{
    printf("DHT11 read error\r\n");
}
```

## 时序说明
- 起始信号：拉低 20ms（使用 `osDelay(20)` 不阻塞其他任务）+ 拉高 30µs
- 数据阶段：每个位约 50µs，关键时序用 `__disable_irq()` 保护
- 每 2s 内在禁用中断的总时间约 2ms，对系统影响可忽略

## 注意事项
- 与 DS18B20 **不能同时连接**同一总线（协议不兼容）
- 依赖 `onewire.h` 的 `OW_DelayUs()` 及 GPIO 宏
- DHT11 读取间隔建议 ≥1s
