/**
 * @file        iic.h
 * @brief       软件 I2C 驱动 (GPIO 位带操作, ~100kHz)
 *
 * 引脚: PB6(SCL), PB7(SDA)
 *
 * 时序参考 24C02 数据手册:
 *   - 起始: SCL=H 时 SDA 下降沿
 *   - 停止: SCL=H 时 SDA 上升沿
 *   - 数据: SCL=L 时 SDA 变化, SCL 上升沿锁存
 *   - ACK : 第9个 SCL 上升沿, SDA 被从机拉低
 *
 * 依赖 DWT 周期计数器 (由 DWT_InitUs 初始化, 参考 onewire.h)
 */

#ifndef __IIC_H
#define __IIC_H

#include "main.h"

/*============================================================================*/
/* 引脚宏                                                                       */
/*============================================================================*/

#define IIC_SCL_PORT       GPIOB
#define IIC_SCL_PIN        GPIO_PIN_6
#define IIC_SDA_PORT       GPIOB
#define IIC_SDA_PIN        GPIO_PIN_7

/* 直接寄存器操作 (与 onewire 一致) */
#define IIC_SCL_H()   (IIC_SCL_PORT->BSRR = IIC_SCL_PIN)
#define IIC_SCL_L()   (IIC_SCL_PORT->BRR  = IIC_SCL_PIN)
#define IIC_SDA_H()   (IIC_SDA_PORT->BSRR = IIC_SDA_PIN)
#define IIC_SDA_L()   (IIC_SDA_PORT->BRR  = IIC_SDA_PIN)
#define IIC_SDA_IN()  ((IIC_SDA_PORT->IDR & IIC_SDA_PIN) ? 1 : 0)

/*============================================================================*/
/* I2C 总线互斥锁 (保护软件 I2C 不被多任务并发访问)                               */
/*============================================================================*/

void IIC_Lock(void);
void IIC_Unlock(void);

/*============================================================================*/
/* API                                                                         */
/*============================================================================*/

void    IIC_Init(void);
void    IIC_Start(void);
void    IIC_Stop(void);
uint8_t IIC_Wait_Ack(void);
void    IIC_Ack(void);
void    IIC_NAck(void);
void    IIC_Send_Byte(uint8_t data);
uint8_t IIC_Read_Byte(uint8_t ack);

#endif /* __IIC_H */
