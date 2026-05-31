# STM32F103 FreeRTOS 学习项目

## 硬件平台
- 主控: STM32F103ZE (Cortex-M3)
- 开发板: 正点原子精英STM32F103开发板
- 引脚定义: docs/硬件描述文档.md

## 构建
- 工具链: arm-none-eabi-gcc
- 构建方式: make
- 输出: build/testfr.elf/hex/bin
- 烧录: make install (通过 OpenOCD + ST-Link)

## 项目结构
- Core/Inc/ — 头文件
- Core/Src/ — 源文件
- Drivers/ — STM32 HAL 驱动
- Middlewares/ — FreeRTOS + CMSIS-RTOS V2
- docs/  — 文档目录

## 已创建的任务 (freertos.c)
- defaultTask: LED0快闪指示 + 串口回显 + 看门狗喂狗
- led1Task: (待开发)
- keyTask: 按键处理(每10ms轮询)
- tempTask: DS18B20温度采集(每2s读取，通过串口输出)
- lcdTask: LCD显示(FSMC控制，显示运行信息)

## 外设
- 按键: KEY0(PE4), KEY1(PE3), KEY_UP(PA0)
- LED: LED0(PB5), LED1(PE5)
- USART1: PA9(TX), PA10(RX) — printf重定向, 中断接收(256字节线性缓冲)
- 1-Wire: PG11 — DS18B20/DHT11 温度传感器
- TFT: PG12(NE4/CS), PG0(A10/RS), PD5(WR), PD4(RD), PB0(BL) — FSMC 16位 8080接口
- CAN: PA11(RX), PA12(TX) — CAN通信，中断接收(16条环形缓冲)
- USART2: PA2/RX(485_RX), PA3/TX(485_TX)

## 驱动层模块
| 模块 | 文件 | 接口 |
|------|------|------|
| 按键 | key.h/c | `Key_Process()` |
| 1-Wire总线 | onewire.h/c | `DWT_InitUs()` (static inline), `OW_DelayUs()`, `OW_Init()`, `OW_Reset()`, `OW_WriteByte()`, `OW_ReadByte()` |
| DS18B20 | ds18b20.h/c | `DS18B20_Init()`, `DS18B20_StartConversion()`, `DS18B20_ReadTemp()` → float |
| DHT11 | dht11.h/c | `DHT11_Init()`, `DHT11_Read(DHT11_Data*)` → temp/humidity |
| CAN | can_drv.h/c | `CAN_DRV_Init(baudrate)`, `CAN_DRV_SendMsg()`, `CAN_DRV_RecvMsg()`, `CAN_DRV_AvailMsg()` |
| TFT LCD | lcd.h/c + lcd_ex.h/c | `lcd_init()`, 画点/画线/矩形/圆形/字符/字符串/数字，支持7种IC驱动 |
| 独立看门狗 | iwdg_drv.h/c | `IWDG_DRV_Init()` (4s超时), `IWDG_DRV_Feed()` |
| 串口 | usart.h/c | printf重定向, `uart1_data_ready()`, 回显 |

## 应用层模块
| 模块 | 文件 | 说明 |
|------|------|------|
| 温度采集 | app_temp.h/c | `StartTempTask` — 自动检测 DS18B20/DHT11，每2s读取并通过printf输出 |
| LCD显示 | app_lcd.h/c | `StartLcdTask` — 初始化LCD，每1s刷新显示运行信息 |

## 串口API (usart.h/c)
- printf() — 直接输出到USART1 (通过__io_putchar)
- USART1_StartRx() — 启动中断接收，重置缓冲区和标志位
- uart1_data_ready() — 检测接收完成标志(收到\n或缓冲区满时为1)
- uart1_rx_buf[] — 全局接收缓冲区(外部可访问)
- uart1_rx_flag — 接收完成标志(外部可访问)
- uart1_available() — 查询有无未读字节
- uart1_getchar() — 阻塞读取一个字符

## 规则
- 新功能新建独立的任务和 .c/.h 文件
- Makefile 中添加新源文件到 C_SOURCES
- 遵循 CubeMX USER CODE 注释块规范
- **手写代码不使用 CubeMX 的 `/* USER CODE BEGIN/END */` 注释标记**（仅 CubeMX 生成的已有文件中沿用已有标记）
- **严格遵循嵌入式分层架构**，各层职责如下：

### 分层架构
```
应用层 (Application)
       ↓
中间件层 (Middleware)
       ↓
驱动层 (Driver / BSP)
       ↓
硬件抽象层 (HAL)
```

| 层级 | 文件位置 | 职责 | 规则 |
|------|----------|------|------|
| 应用层 | `Core/Src/app_*.c` | 任务逻辑、业务流程 | 只调用驱动层/Middleware API，**禁止直接调 HAL** |
| 中间件层 | 维护在 `Middlewares/` 中 | FreeRTOS、协议栈 | 只调用 HAL 或驱动层 API |
| 驱动层 | `Core/Src/*.c` (如 key.c、led.c) | 外设驱动封装 | 封装 HAL，对外暴露业务接口，**禁止直接调 HAL 寄存器** |
| HAL | `Drivers/` | 硬件抽象 | CubeMX 生成 |

### 检查清单
- 应用层代码中**禁止出现** `HAL_GPIO_*`、`HAL_UART_*`、`HAL_*` 等直接 HAL 调用
- 外设操作必须通过驱动层封装函数（如 `LED_On()`、`Key_GetState()`）
- 每个驱动模块提供统一的 `.h` 接口，隐藏 HAL 实现细节
- 新加外设 = 新建驱动文件 + 在应用层任务中调用驱动接口
- **开发完驱动层模块后，必须在 `docs/` 目录写一份使用文档**（API说明、参数、示例代码）
