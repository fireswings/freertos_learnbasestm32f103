#include "app_lcd.h"
#include "lcd.h"
#include "cmsis_os.h"
#include <stdio.h>

void StartLcdTask(void *argument)
{
    lcd_init();

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

        /* 区域2: 显示温度 */
        sprintf(buf, "Temp: N/A  ", tick);
        lcd_show_string(10, y + 20, 200, 16, 16, buf, BLACK);

        /* 区域3: 显示按键提示 */
        lcd_show_string(10, y + 50, 220, 16, 12, "KEY0:Toggle LED0", GRAYBLUE);
        lcd_show_string(10, y + 64, 220, 16, 12, "KEY1:Toggle LED1", GRAYBLUE);
        lcd_show_string(10, y + 78, 220, 16, 12, "KEY_UP:Toggle ALL", GRAYBLUE);

        osDelay(1000);
    }
}
