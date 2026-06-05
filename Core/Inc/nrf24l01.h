#ifndef __NRF24L01_H__
#define __NRF24L01_H__
#include "spi.h"
/* NRF24L01 无线模块控制引脚 */

#define NRF_CS_PIN          GPIO_PIN_7
#define NRF_CS_PORT         GPIOG
#define NRF_CE_PIN          GPIO_PIN_8
#define NRF_CE_PORT         GPIOG
#define NRF_IRQ_PIN         GPIO_PIN_6
#define NRF_IRQ_PORT         GPIOG

void nrf24l01_rx_mode(void);
void nrf24l01_tx_mode(void);
uint8_t nrf24l01_tx_packet(uint8_t *ptxbuf);
uint8_t nrf24l01_rx_packet(uint8_t *prxbuf);


#endif /* __NRF24L01_H__ */