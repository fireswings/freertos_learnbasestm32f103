/**
 * @file        lcd.h
 * @brief       TFTLCD (MCU interface) driver header
 *
 * Supports LCD driver ICs: ILI9341/NT35310/NT35510/SSD1963/ST7789/ST7796/ILI9806
 * Platform: ALIENTEK STM32F103 development board
 * Original source: www.alientek.com
 */

#ifndef __LCD_H
#define __LCD_H

#include "stdlib.h"
#include "main.h"

/*============================================================================*/
/* FSMC hardware pin definitions                                             */
/*============================================================================*/

/* WR / RD / BL pins */
#define LCD_WR_GPIO_PORT                GPIOD
#define LCD_WR_GPIO_PIN                 GPIO_PIN_5
#define LCD_WR_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define LCD_RD_GPIO_PORT                GPIOD
#define LCD_RD_GPIO_PIN                 GPIO_PIN_4
#define LCD_RD_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define LCD_BL_GPIO_PORT                GPIOB
#define LCD_BL_GPIO_PIN                 GPIO_PIN_0
#define LCD_BL_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

/* CS (FSMC_NE4) and RS (FSMC_A10) pins */
#define LCD_CS_GPIO_PORT                GPIOG
#define LCD_CS_GPIO_PIN                 GPIO_PIN_12
#define LCD_CS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)

#define LCD_RS_GPIO_PORT                GPIOG
#define LCD_RS_GPIO_PIN                 GPIO_PIN_0
#define LCD_RS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)

/*
 * FSMC register macros.
 *
 * NEX: which chip select line drives LCD_CS (1~4).
 * AX : which address line drives LCD_RS (0~25).
 *
 * Modify LCD_FSMC_NEX → also update LCD_CS_GPIO pin.
 * Modify LCD_FSMC_AX  → also update LCD_RS_GPIO pin.
 */
#define LCD_FSMC_NEX         4               /* Use FSMC_NE4 for LCD_CS */
#define LCD_FSMC_AX          10              /* Use FSMC_A10 for LCD_RS */

#define LCD_FSMC_BCRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2]      /* BCR register */
#define LCD_FSMC_BTRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2 + 1]  /* BTR register */
#define LCD_FSMC_BWTRX       FSMC_Bank1E->BWTR[(LCD_FSMC_NEX - 1) * 2]     /* BWTR register */

/*============================================================================*/
/* LCD device parameters                                                     */
/*============================================================================*/

typedef struct
{
    uint16_t width;     /* LCD width */
    uint16_t height;    /* LCD height */
    uint16_t id;        /* LCD driver IC ID */
    uint8_t  dir;       /* Orientation: 0=portrait, 1=landscape */
    uint16_t wramcmd;   /* Write GRAM command */
    uint16_t setxcmd;   /* Set X coordinate command */
    uint16_t setycmd;   /* Set Y coordinate command */
} _lcd_dev;

extern _lcd_dev lcddev;
extern uint32_t  g_point_color;    /* Foreground color, default RED */
extern uint32_t  g_back_color;     /* Background color, default WHITE */

/* Backlight control */
#define LCD_BL(x)   do{ x ? \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET); \
                     }while(0)

/*============================================================================*/
/* FSMC memory-map (LCD register vs RAM)                                     */
/*============================================================================*/

typedef struct
{
    volatile uint16_t LCD_REG;   /* RS=0: register/index port */
    volatile uint16_t LCD_RAM;   /* RS=1: data/GRAM port */
} LCD_TypeDef;

/*
 * FSMC Bank1 address calculation:
 *
 *   Bank1 consists of 4 sub-banks (NE1~NE4), each 64MB apart:
 *     FSMC_NE1: 0x60000000 ~ 0x63FFFFFF
 *     FSMC_NE2: 0x64000000 ~ 0x67FFFFFF
 *     FSMC_NE3: 0x68000000 ~ 0x6BFFFFFF
 *     FSMC_NE4: 0x6C000000 ~ 0x6FFFFFFF
 *
 *   Address offset for FSMC_Ay (16-bit data bus):  offset = (1 << y) * 2
 *   RS=1 (RAM) address = NE_base + offset_Ay
 *   RS=0 (REG) address = RAM - 2  (because 16-bit bus, LCD_REG at next lower addr)
 *
 *   With NE4 + A10:
 *     LCD_RAM = 0x6C000000 + (1<<10)*2 = 0x6C000800
 *     LCD_BASE = LCD_RAM - 2           = 0x6C0007FE
 *
 *   General formula:  LCD_BASE = (0x60000000 + 0x04000000*(x-1)) | ((1<<y)*2 - 2)
 */
#define LCD_BASE        (uint32_t)((0X60000000 + (0X4000000 * (LCD_FSMC_NEX - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

/*============================================================================*/
/* Scan directions                                                           */
/*============================================================================*/

#define L2R_U2D         0        /* Left→Right, Up→Down */
#define L2R_D2U         1        /* Left→Right, Down→Up */
#define R2L_U2D         2        /* Right→Left, Up→Down */
#define R2L_D2U         3        /* Right→Left, Down→Up */
#define U2D_L2R         4        /* Up→Down, Left→Right */
#define U2D_R2L         5        /* Up→Down, Right→Left */
#define D2U_L2R         6        /* Down→Up, Left→Right */
#define D2U_R2L         7        /* Down→Up, Right→Left */

#define DFT_SCAN_DIR    L2R_U2D  /* Default scan direction */

/*============================================================================*/
/* Basic colors (RGB565)                                                     */
/*============================================================================*/

#define WHITE           0xFFFF
#define BLACK           0x0000
#define RED             0xF800
#define GREEN           0x07E0
#define BLUE            0x001F
#define MAGENTA         0XF81F   /* BLUE + RED */
#define YELLOW          0XFFE0   /* GREEN + RED */
#define CYAN            0X07FF   /* GREEN + BLUE */

/* Extended colors */
#define BROWN           0XBC40
#define BRRED           0XFC07
#define GRAY            0X8430
#define DARKBLUE        0X01CF
#define LIGHTBLUE       0X7D7C
#define GRAYBLUE        0X5458
#define LIGHTGREEN      0X841F
#define LGRAY           0XC618
#define LGRAYBLUE       0XA651
#define LBBLUE          0X2B12

/*============================================================================*/
/* SSD1963 timing parameters (leave at defaults)                              */
/*============================================================================*/

#define SSD_HOR_RESOLUTION      800
#define SSD_VER_RESOLUTION      480
#define SSD_HOR_PULSE_WIDTH     1
#define SSD_HOR_BACK_PORCH      46
#define SSD_HOR_FRONT_PORCH     210
#define SSD_VER_PULSE_WIDTH     1
#define SSD_VER_BACK_PORCH      23
#define SSD_VER_FRONT_PORCH     22

#define SSD_HT          (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH)
#define SSD_HPS         (SSD_HOR_BACK_PORCH)
#define SSD_VT          (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH)
#define SSD_VPS         (SSD_VER_BACK_PORCH)

/*============================================================================*/
/* API declarations                                                          */
/*============================================================================*/

void lcd_wr_data(volatile uint16_t data);
void lcd_wr_regno(volatile uint16_t regno);
void lcd_write_reg(uint16_t regno, uint16_t data);

void lcd_init(void);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_scan_dir(uint8_t dir);
void lcd_display_dir(uint8_t dir);
void lcd_ssd_backlight_set(uint8_t pwm);

void lcd_write_ram_prepare(void);
void lcd_set_cursor(uint16_t x, uint16_t y);
uint32_t lcd_read_point(uint16_t x, uint16_t y);
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);

void lcd_clear(uint16_t color);
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);

#endif
