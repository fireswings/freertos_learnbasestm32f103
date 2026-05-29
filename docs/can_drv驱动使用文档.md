# can_drv CAN 通信驱动使用文档

## 概述
STM32F103 CAN1 通信驱动，支持标准帧/扩展帧，中断接收环形缓冲，阻塞发送。

## 硬件
| 信号 | GPIO | 说明 |
|------|------|------|
| CAN_TX | PA12 | CAN 发送 (AF_PP) |
| CAN_RX | PA11 | CAN 接收 (INPUT, 上拉) |

## API

```c
#include "can_drv.h"

typedef struct {
    uint32_t id;         // 帧 ID (标准=11bit, 扩展=29bit)
    uint8_t  ide;        // CAN_ID_STD 或 CAN_ID_EXT
    uint8_t  rtr;        // CAN_RTR_DATA 或 CAN_RTR_REMOTE
    uint8_t  dlc;        // 数据长度 0-8
    uint8_t  data[8];    // 数据字节
} CAN_Msg_t;

uint8_t CAN_DRV_Init(uint32_t baudrate);                   // 初始化 CAN1
uint8_t CAN_DRV_SendMsg(CAN_Msg_t *msg);                   // 发送一帧
uint8_t CAN_DRV_RecvMsg(CAN_Msg_t *msg, uint32_t timeout_ms); // 接收一帧（阻塞带超时）
uint8_t CAN_DRV_AvailMsg(void);                            // 查询环形缓冲区是否有数据
```

## 参数说明

### CAN_DRV_Init
- `baudrate`：支持 1000000, 500000, 250000, 125000
- 返回：1=成功, 0=失败（不支持的波特率）
- 内部自动配置：过滤器（接收所有消息）、NVIC 中断优先级 5、RX FIFO0 中断

### CAN_DRV_SendMsg
- 返回：1=成功（获取到空闲邮箱）, 0=失败（三个邮箱都满）
- 阻塞式发送，不使用 DMA

### CAN_DRV_RecvMsg
- `timeout_ms`：超时时间(ms)，设置为 `portMAX_DELAY` 可无限等待
- 返回：1=成功读取到消息, 0=超时
- 从环形缓冲区（16 条深度）中取出一条消息

### CAN_DRV_AvailMsg
- 返回：1=有消息待读取, 0=缓冲区空

## 使用示例

```c
// 初始化 CAN1@500kbps
if (!CAN_DRV_Init(500000))
{
    printf("CAN init failed\r\n");
}

// 发送一帧标准数据帧
CAN_Msg_t tx_msg = {
    .id   = 0x123,
    .ide  = CAN_ID_STD,
    .rtr  = CAN_RTR_DATA,
    .dlc  = 3,
    .data = {0xAA, 0xBB, 0xCC}
};
CAN_DRV_SendMsg(&tx_msg);

// 阻塞接收（100ms 超时）
CAN_Msg_t rx_msg;
if (CAN_DRV_RecvMsg(&rx_msg, 100))
{
    printf("CAN msg: ID=0x%03X, DLC=%d\r\n", rx_msg.id, rx_msg.dlc);
}

// 非阻塞轮询
if (CAN_DRV_AvailMsg())
{
    CAN_DRV_RecvMsg(&rx_msg, 0);  // timeout=0 立即返回
    // 处理消息...
}
```

## 过滤器配置
- 当前配置：单滤波器（Bank 0），ID/Mask 模式，32bit 位宽
- ID=0x0000, Mask=0x0000 → **接收所有消息**
- 如需按 ID 过滤，修改 `can_drv.c` 中 `CAN_DRV_Init()` 的 `can_filter` 配置

## 注意事项
- CAN 收发器芯片（如 TJA1050）需要外部硬件支持
- 中断使用 `USB_LP_CAN1_RX0_IRQn`（与 USB 低优先级中断共享，USB 未启用）
- 依赖 `stm32f1xx_hal_can.c` 驱动，需在 `stm32f1xx_hal_conf.h` 启用 `HAL_CAN_MODULE_ENABLED`
