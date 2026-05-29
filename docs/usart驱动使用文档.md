# usart 串口驱动使用文档

## 概述
USART1 串口驱动，包含 printf 重定向、中断接收行缓冲、回显功能。

## 硬件
| 信号 | GPIO | 说明 |
|------|------|------|
| TX | PA9 | AF_PP，高驱动能力 |
| RX | PA10 | INPUT，浮空 |
| 波特率 | — | 115200-8N1 |

## API

```c
#include "usart.h"

/* 接收相关 */
#define USART1_RX_BUF_SIZE  256
extern volatile uint8_t uart1_rx_buf[USART1_RX_BUF_SIZE];  // 接收缓冲区
extern volatile uint8_t uart1_rx_flag;                      // 接收完成标志
void    USART1_StartRx(void);                               // 启动/重置中断接收
uint8_t uart1_data_ready(void);                             // 检测接收完成标志
int     uart1_available(void);                              // 查询是否有未读字节
int     uart1_getchar(void);                                // 阻塞读取一个字符

/* printf 重定向 */
// printf() — 直接输出到 USART1（通过 _write() → HAL_UART_Transmit）
```

## 接收机制

```
RX 中断 → 逐字节存入 uart1_rx_buf[]
         → 收到 '\n' 或缓冲区满 → 置位 uart1_rx_flag = 1
         → 停止写入（需调用 USART1_StartRx() 重置才能继续接收）
```

## 使用示例

### printf 输出
```c
printf("System initialized, FreeRTOS running!\r\n");
printf("Temperature: %.1f C\r\n", temp);
```

### 行接收模式（检测完整一行）
```c
if (uart1_data_ready())
{
    printf("Received: %s", (char *)uart1_rx_buf);
    USART1_StartRx();  // 重置，开始接收下一行
}
```

### 逐字符读取模式
```c
if (uart1_available())
{
    int c = uart1_getchar();
    printf("Char: %c\r\n", c);
}
```

## 回显实现
回显逻辑在 `freertos.c` 的 `StartDefaultTask` 中，每 200ms 检查一次：
```c
if (uart1_data_ready())
{
    printf("%s", (char *)uart1_rx_buf);
    USART1_StartRx();
}
```

## printf 实现原理
- `printf` → `_write`（强定义在 usart.c） → `HAL_UART_Transmit` 整帧发送
- 启动时调用 `setvbuf(stdout, NULL, _IONBF, 0)` 禁用缓冲
- 同样支持 `scanf`/`gets`（通过 `_read` → `__io_getchar`）

## 注意事项
- `_write` 和 `_read` 在 usart.c 中直接定义（**强符号**），覆盖 newlib 默认弱实现
- CubeMX 的 `syscalls.c` 未被编译进项目，所有 syscall 由 usart.c 提供
- 接收缓冲区满或收到 `\n` 后自动停止写入，避免缓冲区溢出
