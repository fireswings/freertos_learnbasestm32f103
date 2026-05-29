#ifndef __ONEWIRE_H
#define __ONEWIRE_H

#include "main.h"

/* 位操作宏（直接寄存器操作保证速度） */
#define OW_LOW()    (OW_GPIO_Port->BRR  = OW_Pin)
#define OW_HIGH()   (OW_GPIO_Port->BSRR = OW_Pin)
#define OW_READ()   ((OW_GPIO_Port->IDR & OW_Pin) ? 1 : 0)

void OW_Init(void);
void OW_DelayUs(uint16_t us);
uint8_t OW_Reset(void);
void OW_WriteByte(uint8_t data);
uint8_t OW_ReadByte(void);

#endif
