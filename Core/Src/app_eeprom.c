/**
 * @file        app_eeprom.c
 * @brief       24C02 EEPROM 应用层任务
 *
 * 通过队列接收按键命令:
 *   - 0x01 (KEY1) → 写入自增计数器到 24C02 地址 0x00
 *   - 0x00 (KEY0) → 读取 24C02 地址 0x00 的数据
 *
 * 操作结果通过共享变量传递给 LCD 任务显示。
 */

#include "app_eeprom.h"
#include "at24c02.h"
#include "cmsis_os2.h"
#include <stdio.h>

/*============================================================================*/
/* 命令定义                                                                    */
/*============================================================================*/

#define EEPROM_CMD_READ   0x00
#define EEPROM_CMD_WRITE  0x01

/*============================================================================*/
/* 共享变量 (外部可访问)                                                       */
/*============================================================================*/

uint8_t eeprom_last_data = 0;    /* 最后一次读/写的数据 */
uint8_t eeprom_op_result = 0;    /* 操作结果: 0=空闲, 1=写成功, 2=读成功, 0xFF=失败 */

/*============================================================================*/
/* EEPROM 任务                                                                 */
/*============================================================================*/

/**
 * @brief  EEPROM 任务 — 处理 24C02 读写命令
 * @param  argument  未使用
 * @retval None
 */
void StartEepromTask(void *argument)
{
    (void)argument;

    /* 初始化 24C02 */
    if (AT24C02_Init() != HAL_OK)
    {
        printf("[EEPROM] AT24C02 init failed!\r\n");
        eeprom_op_result = 0xFF;
    }
    else
    {
        printf("[EEPROM] AT24C02 init OK\r\n");
    }

    uint8_t  cmd;
    uint8_t  write_counter = 0;
    uint8_t  read_data;
    uint8_t  status;

    for (;;)
    {
        /* 等待队列消息 */
        status = osMessageQueueGet(eepromQueueHandle, &cmd, NULL, osWaitForever);
        if (status != osOK)
            continue;

        if (cmd == EEPROM_CMD_WRITE)
        {
            /* KEY1: 写入递增计数器 */
            write_counter++;
            if (AT24C02_WriteByte(0x00, write_counter) == HAL_OK)
            {
                eeprom_last_data = write_counter;
                eeprom_op_result = 1;  /* 写成功 */
                printf("[EEPROM] Write OK: addr=0x00 data=%d\r\n", write_counter);
            }
            else
            {
                eeprom_op_result = 0xFF; /* 写失败 */
                printf("[EEPROM] Write FAIL!\r\n");
            }
        }
        else if (cmd == EEPROM_CMD_READ)
        {
            /* KEY0: 读取数据 */
            read_data = AT24C02_ReadByte(0x00);
            eeprom_last_data = read_data;
            eeprom_op_result = 2;  /* 读成功 */
            printf("[EEPROM] Read  OK: addr=0x00 data=%d\r\n", read_data);
        }
    }
}
