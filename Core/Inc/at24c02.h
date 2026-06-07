#ifndef __AT24C02_H
#define __AT24C02_H

#include "main.h"

/*============================================================================*/
/* 24C02 硬件定义                                                              */
/*============================================================================*/

/* I2C 总线 */
#define AT24C02_I2C_INSTANCE         I2C1
#define AT24C02_I2C_CLK_ENABLE()     __HAL_RCC_I2C1_CLK_ENABLE()

/* I2C GPIO: PB6(SCL), PB7(SDA) */
#define AT24C02_SCL_PORT             GPIOB
#define AT24C02_SCL_PIN              GPIO_PIN_6
#define AT24C02_SDA_PORT             GPIOB
#define AT24C02_SDA_PIN              GPIO_PIN_7
#define AT24C02_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()

/* 24C02 设备地址 */
#define AT24C02_DEV_ADDR_WRITE       0xA0  /* 7-bit addr 0x50, W bit = 0 */
#define AT24C02_DEV_ADDR_READ        0xA1  /* 7-bit addr 0x50, R bit = 1 */
#define AT24C02_PAGE_SIZE            8     /* 8-byte page */
#define AT24C02_MEM_SIZE             256   /* 2Kbit = 256 bytes */
#define AT24C02_WRITE_DELAY_MS       5     /* 内部写入周期 */

/* I2C 通信超时 */
#define AT24C02_I2C_TIMEOUT          100   /* ms */

/*============================================================================*/
/* API 声明                                                                    */
/*============================================================================*/

HAL_StatusTypeDef AT24C02_Init(void);
HAL_StatusTypeDef AT24C02_WriteByte(uint8_t addr, uint8_t data);
uint8_t           AT24C02_ReadByte(uint8_t addr);
HAL_StatusTypeDef AT24C02_WritePage(uint8_t addr, uint8_t *buf, uint8_t len);
HAL_StatusTypeDef AT24C02_ReadSeq(uint8_t addr, uint8_t *buf, uint8_t len);

#endif /* __AT24C02_H */
