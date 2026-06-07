#ifndef __IIC_H
#define __IIC_H

#include "main.h"

/*============================================================================*/
/* I2C 引脚宏 (PB6=SCL, PB7=SDA)                                               */
/*============================================================================*/

#define IIC_SCL_PORT         GPIOB
#define IIC_SCL_PIN          GPIO_PIN_6
#define IIC_SDA_PORT         GPIOB
#define IIC_SDA_PIN          GPIO_PIN_7

/* GPIO 操作宏 */
#define IIC_SCL_H()   HAL_GPIO_WritePin(IIC_SCL_PORT, IIC_SCL_PIN, GPIO_PIN_SET)
#define IIC_SCL_L()   HAL_GPIO_WritePin(IIC_SCL_PORT, IIC_SCL_PIN, GPIO_PIN_RESET)
#define IIC_SDA_H()   HAL_GPIO_WritePin(IIC_SDA_PORT, IIC_SDA_PIN, GPIO_PIN_SET)
#define IIC_SDA_L()   HAL_GPIO_WritePin(IIC_SDA_PORT, IIC_SDA_PIN, GPIO_PIN_RESET)
#define IIC_SDA_READ()  HAL_GPIO_ReadPin(IIC_SDA_PORT, IIC_SDA_PIN)

/*============================================================================*/
/* API                                                                         */
/*============================================================================*/

void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
uint8_t IIC_Wait_Ack(void);
void IIC_Send_Byte(uint8_t data);
uint8_t IIC_Read_Byte(uint8_t ack);   /* ack=0 → NACK after read, ack=1 → ACK */
void IIC_Delay_us(uint16_t us);

#endif /* __IIC_H */
