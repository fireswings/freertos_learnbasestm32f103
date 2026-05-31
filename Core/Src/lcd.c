/**
 * @file        lcd.c
 * @brief       TFTLCD driver - FSMC init, drawing primitives, text display
 *
 * Supports 7 driver ICs via lcd_ex.c register tables.
 * Original source: ALIENTEK www.alientek.com
 */

#include "stdlib.h"
#include "lcd.h"
#include "lcdfont.h"
#include "lcd_ex.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "onewire.h"

SRAM_HandleTypeDef g_sram_handle;

uint32_t g_point_color = 0XF800;
uint32_t g_back_color  = 0XFFFF;

_lcd_dev lcddev;

/* Write data to LCD GRAM (RS=1) */
void lcd_wr_data(volatile uint16_t data)
{
    data = data;
    LCD->LCD_RAM = data;
}

/* Write register index (RS=0) */
void lcd_wr_regno(volatile uint16_t regno)
{
    regno = regno;
    LCD->LCD_REG = regno;
}

/* Write register index + data */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD->LCD_REG = regno;
    LCD->LCD_RAM = data;
}

/* Busy-wait delay for LCD timing */
static void lcd_opt_delay(uint32_t i)
{
    while (i--);
}

/* Read 16-bit data from LCD GRAM */
static uint16_t lcd_rd_data(void)
{
    volatile uint16_t ram;
    lcd_opt_delay(2);
    ram = LCD->LCD_RAM;
    return ram;
}

/* Prepare to write GRAM (send wramcmd) */
void lcd_write_ram_prepare(void)
{
    LCD->LCD_REG = lcddev.wramcmd;
}

/* Read RGB565 color at pixel (x,y) */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    uint16_t r = 0, g = 0, b = 0;

    if (x >= lcddev.width || y >= lcddev.height) return 0;

    lcd_set_cursor(x, y);

    if (lcddev.id == 0X5510)
        lcd_wr_regno(0X2E00);   /* NT35510 read GRAM command */
    else
        lcd_wr_regno(0X2E);     /* Others: read GRAM command */

    r = lcd_rd_data();          /* Dummy read */

    if (lcddev.id == 0x1963)
        return r;               /* SSD1963: direct return */

    r = lcd_rd_data();          /* Actual color data */

    if (lcddev.id == 0x7796)    /* ST7796: single read returns full pixel */
        return r;

    /* ILI9341/NT35310/NT35510/ST7789/ILI9806: need 2 more reads */
    b = lcd_rd_data();
    g = r & 0XFF;               /* First read: R(high8) G(high8) */
    g <<= 8;
    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}

/* Turn display on */
void lcd_display_on(void)
{
    if (lcddev.id == 0X5510)
        lcd_wr_regno(0X2900);
    else
        lcd_wr_regno(0X29);
}

/* Turn display off */
void lcd_display_off(void)
{
    if (lcddev.id == 0X5510)
        lcd_wr_regno(0X2800);
    else
        lcd_wr_regno(0X28);
}

/* Set cursor to (x,y) - handles IC-specific register layouts */
void lcd_set_cursor(uint16_t x, uint16_t y)
{
    if (lcddev.id == 0X1963)    /* SSD1963: portrait mode needs x-flip */
    {
        if (lcddev.dir == 0)
        {
            x = lcddev.width - 1 - x;
            lcd_wr_regno(lcddev.setxcmd);
            lcd_wr_data(0); lcd_wr_data(0);
            lcd_wr_data(x >> 8); lcd_wr_data(x & 0XFF);
        }
        else
        {
            lcd_wr_regno(lcddev.setxcmd);
            lcd_wr_data(x >> 8); lcd_wr_data(x & 0XFF);
            lcd_wr_data((lcddev.width - 1) >> 8);
            lcd_wr_data((lcddev.width - 1) & 0XFF);
        }
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(y >> 8); lcd_wr_data(y & 0XFF);
        lcd_wr_data((lcddev.height - 1) >> 8);
        lcd_wr_data((lcddev.height - 1) & 0XFF);
    }
    else if (lcddev.id == 0X5510)  /* NT35510: 2-register XY */
    {
        lcd_wr_regno(lcddev.setxcmd);     lcd_wr_data(x >> 8);
        lcd_wr_regno(lcddev.setxcmd + 1); lcd_wr_data(x & 0XFF);
        lcd_wr_regno(lcddev.setycmd);     lcd_wr_data(y >> 8);
        lcd_wr_regno(lcddev.setycmd + 1); lcd_wr_data(y & 0XFF);
    }
    else    /* ILI9341/ST7789/ST7796/ILI9806: 1-register 2-byte XY */
    {
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(x >> 8); lcd_wr_data(x & 0XFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(y >> 8); lcd_wr_data(y & 0XFF);
    }
}

/*
 * Set scan direction (8 orientations)
 *
 * Note: orientation conversions compensate for IC-specific defaults.
 *   dir=1 (landscape) + non-1963  =>  apply transform
 *   dir=0 (portrait)  + 1963      =>  apply transform
 */
void lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0, dirreg = 0, temp;

    /* Orientation compensation */
    if ((lcddev.dir == 1 && lcddev.id != 0X1963) ||
        (lcddev.dir == 0 && lcddev.id == 0X1963))
    {
        switch (dir)
        {
            case 0: dir = 6; break;
            case 1: dir = 7; break;
            case 2: dir = 4; break;
            case 3: dir = 5; break;
            case 4: dir = 1; break;
            case 5: dir = 0; break;
            case 6: dir = 3; break;
            case 7: dir = 2; break;
        }
    }

    switch (dir)
    {
        case L2R_U2D: regval |= (0 << 7) | (0 << 6) | (0 << 5); break;
        case L2R_D2U: regval |= (1 << 7) | (0 << 6) | (0 << 5); break;
        case R2L_U2D: regval |= (0 << 7) | (1 << 6) | (0 << 5); break;
        case R2L_D2U: regval |= (1 << 7) | (1 << 6) | (0 << 5); break;
        case U2D_L2R: regval |= (0 << 7) | (0 << 6) | (1 << 5); break;
        case U2D_R2L: regval |= (0 << 7) | (1 << 6) | (1 << 5); break;
        case D2U_L2R: regval |= (1 << 7) | (0 << 6) | (1 << 5); break;
        case D2U_R2L: regval |= (1 << 7) | (1 << 6) | (1 << 5); break;
    }

    dirreg = (lcddev.id == 0X5510) ? 0X3600 : 0X36;

    /* ILI9341/ST7789/ST7796 need BGR bit */
    if (lcddev.id == 0X9341 || lcddev.id == 0X7789 || lcddev.id == 0x7796)
        regval |= 0X08;

    lcd_write_reg(dirreg, regval);

    /* Swap width/height if X-Y axes exchanged */
    if (lcddev.id != 0X1963)
    {
        if (regval & 0X20)
        {
            if (lcddev.width < lcddev.height)
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
        else
        {
            if (lcddev.width > lcddev.height)
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
    }

    /* Set display window size */
    if (lcddev.id == 0X5510)
    {
        lcd_wr_regno(lcddev.setxcmd); lcd_wr_data(0);
        lcd_wr_regno(lcddev.setxcmd + 1); lcd_wr_data(0);
        lcd_wr_regno(lcddev.setxcmd + 2); lcd_wr_data((lcddev.width - 1) >> 8);
        lcd_wr_regno(lcddev.setxcmd + 3); lcd_wr_data((lcddev.width - 1) & 0XFF);
        lcd_wr_regno(lcddev.setycmd); lcd_wr_data(0);
        lcd_wr_regno(lcddev.setycmd + 1); lcd_wr_data(0);
        lcd_wr_regno(lcddev.setycmd + 2); lcd_wr_data((lcddev.height - 1) >> 8);
        lcd_wr_regno(lcddev.setycmd + 3); lcd_wr_data((lcddev.height - 1) & 0XFF);
    }
    else
    {
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(0); lcd_wr_data(0);
        lcd_wr_data((lcddev.width - 1) >> 8); lcd_wr_data((lcddev.width - 1) & 0XFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(0); lcd_wr_data(0);
        lcd_wr_data((lcddev.height - 1) >> 8); lcd_wr_data((lcddev.height - 1) & 0XFF);
    }
}

/* Draw a single pixel at (x,y) */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    lcd_set_cursor(x, y);
    lcd_write_ram_prepare();
    LCD->LCD_RAM = color;
}

/* SSD1963 backlight PWM: 0~100 */
void lcd_ssd_backlight_set(uint8_t pwm)
{
    lcd_wr_regno(0xBE);
    lcd_wr_data(0x05);           /* PWM frequency */
    lcd_wr_data(pwm * 2.55);     /* Duty cycle */
    lcd_wr_data(0x01);           /* C */
    lcd_wr_data(0xFF);           /* D */
    lcd_wr_data(0x00);           /* E */
    lcd_wr_data(0x00);           /* F */
}

/* Set display orientation: dir=0 portrait, dir=1 landscape */
void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;

    if (dir == 0)    /* Portrait */
    {
        lcddev.width = 240; lcddev.height = 320;
        if (lcddev.id == 0x5510)
        {
            lcddev.wramcmd = 0X2C00; lcddev.setxcmd = 0X2A00;
            lcddev.setycmd = 0X2B00; lcddev.width = 480; lcddev.height = 800;
        }
        else if (lcddev.id == 0X1963)
        {
            lcddev.wramcmd = 0X2C; lcddev.setxcmd = 0X2B;
            lcddev.setycmd = 0X2A; lcddev.width = 480; lcddev.height = 800;
        }
        else
        {
            lcddev.wramcmd = 0X2C; lcddev.setxcmd = 0X2A; lcddev.setycmd = 0X2B;
        }
        if (lcddev.id == 0X5310 || lcddev.id == 0x7796)
            { lcddev.width = 320; lcddev.height = 480; }
        if (lcddev.id == 0X9806)
            { lcddev.width = 480; lcddev.height = 800; }
    }
    else             /* Landscape */
    {
        lcddev.width = 320; lcddev.height = 240;
        if (lcddev.id == 0x5510)
        {
            lcddev.wramcmd = 0X2C00; lcddev.setxcmd = 0X2A00;
            lcddev.setycmd = 0X2B00; lcddev.width = 800; lcddev.height = 480;
        }
        else if (lcddev.id == 0X1963 || lcddev.id == 0x9806)
        {
            lcddev.wramcmd = 0X2C; lcddev.setxcmd = 0X2A;
            lcddev.setycmd = 0X2B; lcddev.width = 800; lcddev.height = 480;
        }
        else
        {
            lcddev.wramcmd = 0X2C; lcddev.setxcmd = 0X2A; lcddev.setycmd = 0X2B;
        }
        if (lcddev.id == 0X5310 || lcddev.id == 0x7796)
            { lcddev.width = 480; lcddev.height = 320; }
    }

    lcd_scan_dir(DFT_SCAN_DIR);
}

/* Set window for burst write, auto-sets cursor to top-left (sx,sy) */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t twidth = sx + width - 1, theight = sy + height - 1;

    if (lcddev.id == 0X1963 && lcddev.dir != 1)    /* SSD1963 portrait */
    {
        sx = lcddev.width - width - sx;
        height = sy + height - 1;
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(sx >> 8); lcd_wr_data(sx & 0XFF);
        lcd_wr_data((sx + width - 1) >> 8); lcd_wr_data((sx + width - 1) & 0XFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(sy >> 8); lcd_wr_data(sy & 0XFF);
        lcd_wr_data(height >> 8); lcd_wr_data(height & 0XFF);
    }
    else if (lcddev.id == 0X5510)    /* NT35510 */
    {
        lcd_wr_regno(lcddev.setxcmd); lcd_wr_data(sx >> 8);
        lcd_wr_regno(lcddev.setxcmd + 1); lcd_wr_data(sx & 0XFF);
        lcd_wr_regno(lcddev.setxcmd + 2); lcd_wr_data(twidth >> 8);
        lcd_wr_regno(lcddev.setxcmd + 3); lcd_wr_data(twidth & 0XFF);
        lcd_wr_regno(lcddev.setycmd); lcd_wr_data(sy >> 8);
        lcd_wr_regno(lcddev.setycmd + 1); lcd_wr_data(sy & 0XFF);
        lcd_wr_regno(lcddev.setycmd + 2); lcd_wr_data(theight >> 8);
        lcd_wr_regno(lcddev.setycmd + 3); lcd_wr_data(theight & 0XFF);
    }
    else    /* ILI9341/ST7789/ST7796/ILI9806 */
    {
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(sx >> 8); lcd_wr_data(sx & 0XFF);
        lcd_wr_data(twidth >> 8); lcd_wr_data(twidth & 0XFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(sy >> 8); lcd_wr_data(sy & 0XFF);
        lcd_wr_data(theight >> 8); lcd_wr_data(theight & 0XFF);
    }
}

/* FSMC GPIO pin initialization (called by HAL_SRAM_Init) */
void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
{
    GPIO_InitTypeDef gpio_init_struct;

    __HAL_RCC_FSMC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* PD0,PD1,PD8,PD9,PD10,PD14,PD15 */
    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8
                         | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    /* PE7~PE15 */
    gpio_init_struct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10
                         | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio_init_struct);
}

/*
 * Initialize LCD: configure FSMC, auto-detect driver IC, load init sequence.
 * Must be called from a FreeRTOS task (uses osDelay for timing).
 */
void lcd_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    FSMC_NORSRAM_TimingTypeDef fsmc_read_handle;
    FSMC_NORSRAM_TimingTypeDef fsmc_write_handle;

    DWT_InitUs();  /* initialize DWT cycle counter for microsecond delays */

    LCD_CS_GPIO_CLK_ENABLE();
    LCD_WR_GPIO_CLK_ENABLE();
    LCD_RD_GPIO_CLK_ENABLE();
    LCD_RS_GPIO_CLK_ENABLE();
    LCD_BL_GPIO_CLK_ENABLE();

    /* CS pin */
    gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
    HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
    HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
    HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);

    /* FSMC configuration: Bank1-NE4, 16-bit data, Mode A, Extended mode */
    g_sram_handle.Instance = FSMC_NORSRAM_DEVICE;
    g_sram_handle.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    g_sram_handle.Init.NSBank = FSMC_NORSRAM_BANK4;
    g_sram_handle.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    g_sram_handle.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    g_sram_handle.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    g_sram_handle.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    g_sram_handle.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    g_sram_handle.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    g_sram_handle.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    g_sram_handle.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
    g_sram_handle.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    g_sram_handle.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;

    /* Read timing: ADDSET=0 -> 1HCLK, DATAST=15 -> 16HCLK (~222ns @ 72MHz) */
    fsmc_read_handle.AddressSetupTime = 0;
    fsmc_read_handle.AddressHoldTime = 0;
    fsmc_read_handle.DataSetupTime = 15;
    fsmc_read_handle.AccessMode = FSMC_ACCESS_MODE_A;

    /* Write timing: ADDSET=0 -> 1HCLK, DATAST=1 -> 2HCLK (~28ns @ 72MHz) */
    fsmc_write_handle.AddressSetupTime = 0;
    fsmc_write_handle.AddressHoldTime = 0;
    fsmc_write_handle.DataSetupTime = 1;
    fsmc_write_handle.AccessMode = FSMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&g_sram_handle, &fsmc_read_handle, &fsmc_write_handle);
    osDelay(50);    /* Wait for FSMC to stabilize */

    /* === Auto-detect LCD driver IC by reading ID registers === */

    /* Try ILI9341 (ID @ 0xD3) */
    lcd_wr_regno(0XD3);
    lcddev.id = lcd_rd_data();
    lcddev.id = lcd_rd_data();
    lcddev.id = lcd_rd_data();
    lcddev.id <<= 8;
    lcddev.id |= lcd_rd_data();

    if (lcddev.id != 0X9341)    /* Not ILI9341 -> try ST7789 (ID @ 0x04) */
    {
        lcd_wr_regno(0X04);
        lcddev.id = lcd_rd_data(); lcddev.id = lcd_rd_data();
        lcddev.id = lcd_rd_data(); lcddev.id <<= 8; lcddev.id |= lcd_rd_data();
        if (lcddev.id == 0X8552) lcddev.id = 0x7789;

        if (lcddev.id != 0x7789)    /* Not ST7789 -> try NT35310 (ID @ 0xD4) */
        {
            lcd_wr_regno(0xD4);
            lcddev.id = lcd_rd_data(); lcddev.id = lcd_rd_data();
            lcddev.id = lcd_rd_data(); lcddev.id <<= 8; lcddev.id |= lcd_rd_data();

            if (lcddev.id != 0x5310)    /* Not NT35310 -> try ST7796 (ID @ 0xD3) */
            {
                lcd_wr_regno(0XD3);
                lcddev.id = lcd_rd_data(); lcddev.id = lcd_rd_data();
                lcddev.id = lcd_rd_data(); lcddev.id <<= 8; lcddev.id |= lcd_rd_data();

                if (lcddev.id != 0x7796)    /* Not ST7796 -> try NT35510 */
                {
                    /* Unlock NT35510 extended registers */
                    lcd_write_reg(0xF000, 0x0055); lcd_write_reg(0xF001, 0x00AA);
                    lcd_write_reg(0xF002, 0x0052); lcd_write_reg(0xF003, 0x0008);
                    lcd_write_reg(0xF004, 0x0001);

                    lcd_wr_regno(0xC500); lcddev.id = lcd_rd_data(); lcddev.id <<= 8;
                    lcd_wr_regno(0xC501); lcddev.id |= lcd_rd_data();
                    osDelay(5);     /* 1963 needs reset delay after 0xC501 */

                    if (lcddev.id != 0x5510)    /* Not NT35510 -> try ILI9806 */
                    {
                        lcd_wr_regno(0XD3);
                        lcddev.id = lcd_rd_data(); lcddev.id = lcd_rd_data();
                        lcddev.id = lcd_rd_data(); lcddev.id <<= 8; lcddev.id |= lcd_rd_data();

                        if (lcddev.id != 0x9806)    /* Not ILI9806 -> try SSD1963 */
                        {
                            lcd_wr_regno(0xA1);
                            lcddev.id = lcd_rd_data(); lcddev.id = lcd_rd_data();
                            lcddev.id <<= 8; lcddev.id |= lcd_rd_data();
                            if (lcddev.id == 0x5761) lcddev.id = 0x1963;
                        }
                    }
                }
            }
        }
    }

    printf("LCD ID:%x\r\n", lcddev.id);
    /* Load IC-specific register initialization sequence */
    if (lcddev.id == 0X7789)      lcd_ex_st7789_reginit();
    else if (lcddev.id == 0X9341) lcd_ex_ili9341_reginit();
    else if (lcddev.id == 0x5310) lcd_ex_nt35310_reginit();
    else if (lcddev.id == 0x7796) lcd_ex_st7796_reginit();
    else if (lcddev.id == 0x5510) lcd_ex_nt35510_reginit();
    else if (lcddev.id == 0x9806) lcd_ex_ili9806_reginit();
    else if (lcddev.id == 0x1963) { lcd_ex_ssd1963_reginit(); lcd_ssd_backlight_set(100); }

    lcd_display_dir(0); /* Default: portrait */
    LCD_BL(1);          /* Backlight on */
    lcd_clear(WHITE);
}

/* Clear entire screen with single color */
void lcd_clear(uint16_t color)
{
    uint32_t index = 0;
    uint32_t totalpoint = lcddev.width;
    totalpoint *= lcddev.height;
    lcd_set_cursor(0x00, 0x0000);
    lcd_write_ram_prepare();
    for (index = 0; index < totalpoint; index++)
        LCD->LCD_RAM = color;
}

/* Fill rectangle from (sx,sy) to (ex,ey) with solid color */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t i, j;
    uint16_t xlen = ex - sx + 1;
    for (i = sy; i <= ey; i++)
    {
        lcd_set_cursor(sx, i);
        lcd_write_ram_prepare();
        for (j = 0; j < xlen; j++)
            LCD->LCD_RAM = color;
    }
}

/* Fill rectangle from color array (size: width*height) */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint16_t height = ey - sy + 1, width = ex - sx + 1;
    for (uint16_t i = 0; i < height; i++)
    {
        lcd_set_cursor(sx, sy + i);
        lcd_write_ram_prepare();
        for (uint16_t j = 0; j < width; j++)
            LCD->LCD_RAM = color[i * width + j];
    }
}

/* Draw line using Bresenham's algorithm */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;

    delta_x = x2 - x1; delta_y = y2 - y1;
    row = x1; col = y1;

    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }

    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }

    distance = (delta_x > delta_y) ? delta_x : delta_y;

    for (t = 0; t <= distance + 1; t++)
    {
        lcd_draw_point(row, col, color);
        xerr += delta_x; yerr += delta_y;
        if (xerr > distance) { xerr -= distance; row += incx; }
        if (yerr > distance) { yerr -= distance; col += incy; }
    }
}

/* Draw horizontal line (uses lcd_fill for efficiency) */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcddev.width) || (y > lcddev.height)) return;
    lcd_fill(x, y, x + len - 1, y, color);
}

/* Draw rectangle outline */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/* Draw circle outline using Bresenham's circle algorithm */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a = 0, b = r, di = 3 - (r << 1);
    while (a <= b)
    {
        lcd_draw_point(x0 + a, y0 - b, color);
        lcd_draw_point(x0 + b, y0 - a, color);
        lcd_draw_point(x0 + b, y0 + a, color);
        lcd_draw_point(x0 + a, y0 + b, color);
        lcd_draw_point(x0 - a, y0 + b, color);
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color);
        lcd_draw_point(x0 - b, y0 - a, color);
        a++;
        if (di < 0) di += 4 * a + 6;
        else { di += 10 + 4 * (a - b); b--; }
    }
}

/* Draw filled circle */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    uint32_t xr = r;

    lcd_draw_hline(x - r, y, 2 * r, color);
    for (i = 1; i <= imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            if (xr > imax)
            {
                lcd_draw_hline(x - i + 1, y + xr, 2 * (i - 1), color);
                lcd_draw_hline(x - i + 1, y - xr, 2 * (i - 1), color);
            }
            xr--;
        }
        lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

/*
 * Display a single ASCII character at (x,y).
 *   size: font size 12/16/24/32
 *   mode: 0 = fill background, 1 = transparent background
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
    uint8_t *pfont = 0;

    chr = chr - ' ';    /* Offset from space (ASCII 32) */

    switch (size)
    {
        case 12: pfont = (uint8_t *)asc2_1206[chr]; break;
        case 16: pfont = (uint8_t *)asc2_1608[chr]; break;
        case 24: pfont = (uint8_t *)asc2_2412[chr]; break;
        case 32: pfont = (uint8_t *)asc2_3216[chr]; break;
        default: return;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                lcd_draw_point(x, y, color);
            else if (mode == 0)
                lcd_draw_point(x, y, g_back_color);

            temp <<= 1;
            y++;
            if (y >= lcddev.height) return;
            if ((y - y0) == size)
            {
                y = y0; x++;
                if (x >= lcddev.width) return;
                break;
            }
        }
    }
}

/* Power function: return m^n */
static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--) result *= m;
    return result;
}

/* Display unsigned integer, 'len' digits, leading zeros hidden */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp, enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2) * t, y, ' ', size, 0, color);
                continue;
            }
            else enshow = 1;
        }
        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, 0, color);
    }
}

/*
 * Extended number display with mode flags:
 *   bit[7]: 1 = show leading zeros, 0 = hide (space)
 *   bit[0]: 1 = transparent background, 0 = fill background
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp, enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2) * t, y,
                    (mode & 0X80) ? '0' : ' ', size, mode & 0X01, color);
                continue;
            }
            else enshow = 1;
        }
        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, mode & 0X01, color);
    }
}

/*
 * Display string within bounding box (width x height).
 * Auto-wraps to next line when exceeding width.
 * Only printable ASCII characters (' '..'~') are displayed.
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x; height += y;
    while ((*p <= '~') && (*p >= ' '))
    {
        if (x >= width) { x = x0; y += size; }
        if (y >= height) break;
        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}
