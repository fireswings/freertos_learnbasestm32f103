#ifndef __SPI_H
#define __SPI_H

#include "main.h"

/* SPI2 引脚定义 (已在 main.h 中定义 CS 引脚, 此处补充说明) */
#define SPI2_SCK_PIN        GPIO_PIN_13
#define SPI2_SCK_PORT       GPIOB
#define SPI2_MISO_PIN       GPIO_PIN_14
#define SPI2_MISO_PORT      GPIOB
#define SPI2_MOSI_PIN       GPIO_PIN_15
#define SPI2_MOSI_PORT      GPIOB

/* Flash 片选 */
#define FLASH_CS_PIN        GPIO_PIN_12
#define FLASH_CS_PORT       GPIOB



/* 通用 CS 控制宏 — 外部模块通过片选切换 SPI 从设备 */
#define SPI_CS_LOW(port, pin)   HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define SPI_CS_HIGH(port, pin)  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)

HAL_StatusTypeDef SPI_DRV_Init(void);
uint8_t spi2_read_write_byte(uint8_t tx_byte);

extern SPI_HandleTypeDef hspi2;

#endif /* __SPI_H */
