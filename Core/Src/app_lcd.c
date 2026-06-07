#include "app_lcd.h"
#include "lcd.h"
#include "cmsis_os.h"
#include "rtc_drv.h"
#include "light_sensor.h"
#include <stdio.h>

/* EEPROM 共享变量 (在 freertos.c 中定义) */
extern uint8_t eeprom_last_data;
extern uint8_t eeprom_op_result;

void StartLcdTask(void *argument)
{
    lcd_init();
    LS_DRV_Init();

    printf("LCD ID: %04X, %dx%d\r\n", lcddev.id, lcddev.width, lcddev.height);

    lcd_show_string(10, 5, 220, 16, 16, "STM32F103 FreeRTOS", BLUE);
    lcd_show_string(10, 25, 220, 16, 16, "LCD Test OK!", RED);

    char buf[64];
    uint16_t y = 50;
    uint32_t tick = 0;

    for (;;)
    {
        /* 区域1: 显示运行时间 */
        sprintf(buf, "Uptime: %lus", tick);
        lcd_show_string(10, y, 200, 16, 16, buf, BLACK);
        tick++;

        /* 区域2: 显示RTC时间 */
        uint8_t h, m, s;
        RTC_DRV_GetTime(&h, &m, &s);
        sprintf(buf, "Time: %02d:%02d:%02d", h, m, s);
        lcd_show_string(10, y + 20, 200, 16, 16, buf, BLACK);

        /* 区域3: 显示按键提示 */
        lcd_show_string(10, y + 50, 220, 16, 12, "KEY0:Read 24C02", GRAYBLUE);
        lcd_show_string(10, y + 64, 220, 16, 12, "KEY1:Write 24C02", GRAYBLUE);
        lcd_show_string(10, y + 78, 220, 16, 12, "KEY_UP:Toggle LCD", GRAYBLUE);

        /* 区域4: 显示光敏电阻值 */
        sprintf(buf, "Light: %d", LS_DRV_ReadPercent());
        lcd_show_string(10, y + 92, 220, 16, 12, buf, BLACK);

        /* 区域5: 显示 24C02 EEPROM 操作状态 */
        if (eeprom_op_result == 1)
        {
            sprintf(buf, "24C02 Wrote: %d", eeprom_last_data);
            lcd_show_string(10, y + 106, 220, 16, 12, buf, BLUE);
        }
        else if (eeprom_op_result == 2)
        {
            sprintf(buf, "24C02 Read: %d", eeprom_last_data);
            lcd_show_string(10, y + 106, 220, 16, 12, buf, GREEN);
        }
        else if (eeprom_op_result == 0xFF)
        {
            lcd_show_string(10, y + 106, 220, 16, 12, "24C02 Error!", RED);
        }
        else
        {
            lcd_show_string(10, y + 106, 220, 16, 12, "24C02 Ready", GRAY);
        }

        osDelay(1000);
    }
}
