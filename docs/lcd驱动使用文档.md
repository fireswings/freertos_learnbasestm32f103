# lcd TFT LCD 驱动使用文档

## 概述
正点原子 TFTLCD 驱动，通过 STM32 FSMC 外设以 16 位 8080 接口驱动液晶屏，支持 7 种驱动 IC。

## 硬件连接

| 信号 | GPIO | FSMC 映射 | 说明 |
|------|------|-----------|------|
| CS (片选) | PG12 | FSMC_NE4 | Bank1-NE4 |
| RS (命令/数据) | PG0 | FSMC_A10 | 1=数据, 0=命令 |
| WR (写) | PD5 | FSMC_NWE | 写使能 |
| RD (读) | PD4 | FSMC_NOE | 读使能 |
| D0~D1 | PD14~PD15 | FSMC_D0~D1 | 16 位数据 |
| D2~D3 | PD0~PD1 | FSMC_D2~D3 | |
| D4~D12 | PE7~PE15 | FSMC_D4~D12 | |
| D13~D15 | PD8~PD10 | FSMC_D13~D15 | |
| BL (背光) | PB0 | GPIO | 高电平点亮 |

**FSMC Bank 地址映射：**

```
LCD->LCD_REG = 0x6C000000  (RS=0, 写寄存器地址)
LCD->LCD_RAM = 0x6C000800  (RS=1, 读/写GRAM数据)
```

## 支持的驱动 IC

| IC 型号 | 典型分辨率 | 实际验证 |
|---------|-----------|---------|
| ILI9341 | 240×320 | 已测试 |
| ST7789 | 240×320 | 已测试 |
| ST7796 | 320×480 | 已测试 |
| NT35310 | 320×480 | 已测试 |
| NT35510 | 480×800 | 已测试 |
| ILI9806 | 480×800 | 已测试 |
| SSD1963 | 480×800 | 已测试 |

`lcd_init()` 会自动通过读取 ID 寄存器识别 IC 型号并加载对应初始化序列。

## API

### 初始化与显示控制
```c
void lcd_init(void);                        // 自动识别IC并初始化(含FSMC配置)
void lcd_display_on(void);                  // 开显示
void lcd_display_off(void);                 // 关显示
void lcd_display_dir(uint8_t dir);          // 设置横屏(1) /竖屏(0)
void lcd_scan_dir(uint8_t dir);             // 设置扫描方向(0~7)
```

### 基本绘图
```c
void lcd_clear(uint16_t color);                                       // 全屏清屏
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);         // 画点
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);  // 画线
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color); // 画矩形
void lcd_draw_circle(uint16_t x, uint16_t y, uint8_t r, uint16_t color);    // 画圆
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);     // 填充矩形
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);              // 填充实心圆
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color); // 填充颜色数组
```

### 文字显示
```c
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);
```

- `size`: 字号 12 / 16 / 24 / 32
- `mode`: 字符显示模式，0=非叠加(背景填充)，1=叠加(仅画前景)
- 字体: ASCII 半角字符（空格 ~ `~`），4 种字号
- 中文字体需自行添加 FLASH 字库

### 预定义颜色
```c
WHITE    BLACK    RED      GREEN    BLUE     MAGENTA
YELLOW   CYAN     BROWN    BRRED    GRAY     DARKBLUE
LIGHTBLUE  GRAYBLUE  LIGHTGREEN  LGRAY  LGRAYBLUE  LBBLUE
```

## 使用示例

### 基本显示任务
```c
#include "lcd.h"
#include "cmsis_os.h"
#include <stdio.h>

void StartLcdTask(void *argument)
{
    lcd_init();     // 自动识别IC、初始化FSMC、配置扫描方向、开背光、清屏(白色)

    // 显示字符串
    lcd_show_string(10, 5, 220, 16, 16, "Hello STM32!", BLUE);

    // 显示数字
    lcd_show_num(10, 30, 12345, 5, 16, BLACK);

    // 画一些图形
    lcd_draw_rectangle(10, 60, 100, 100, RED);
    lcd_fill_circle(200, 80, 20, GREEN);

    char buf[32];
    for (;;)
    {
        sprintf(buf, "Tick: %lu", HAL_GetTick());
        lcd_show_string(10, 120, 200, 16, 16, buf, BLACK);
        osDelay(1000);
    }
}
```

### 全局变量
```c
extern _lcd_dev lcddev;          // LCD 设备参数
// lcddev.width  — 当前有效宽度
// lcddev.height — 当前有效高度
// lcddev.id    — 驱动IC ID
// lcddev.dir   — 当前方向(0=竖屏, 1=横屏)

extern uint32_t g_point_color;   // 画笔颜色(默认红色)
extern uint32_t g_back_color;    // 背景颜色(默认白色)
```

### 背光控制
```c
#define LCD_BL(x)  do { x ? HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET)
                           : HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET);
                    } while(0)

LCD_BL(1);  // 开背光
LCD_BL(0);  // 关背光
```

## 注意事项

- `lcd_init()` 必须在 FreeRTOS 调度器启动后在任务中调用（内部使用 `osDelay` 延时）
- `lcd_init()` 内部会调用 `printf` 输出 LCD ID 到串口
- lcdTask 栈建议 ≥ 2048 字节（`512 * 4`），`lcd_init()` 调用了 printf
- 所有绘图 API 不会自动刷新屏幕，对 GRAM 写入后立即生效（并口 8080 模式）
- FSMC 时序已针对 72MHz HCLK 优化：读 DATAST=16T(222ns)，写 DATAST=2T(28ns)
- LCD 数据线占用 PD0/PD1/PD8~PD10/PD14~PD15 和 PE7~PE15，注意不要与其他外设冲突
- 扫描方向设置后 `lcddev.width` 和 `lcddev.height` 会自动交换（横竖屏切换）
- 依赖 `stm32f1xx_hal_sram.c` 和 `stm32f1xx_ll_fsmc.c` 驱动
