#include "nrf24l01.h"
#include "nrf24l01_reg.h"

#define SPI_CE(x) do { x?SPI_CS_HIGH(NRF_CE_PORT, NRF_CE_PIN):SPI_CS_LOW(NRF_CE_PORT, NRF_CE_PIN); } while(0)
#define SPI_CS(x) do { x?SPI_CS_HIGH(NRF_CS_PORT, NRF_CS_PIN):SPI_CS_LOW(NRF_CS_PORT, NRF_CS_PIN); } while(0)
#define NRF24L01_IRQ HAL_GPIO_ReadPin(NRF_IRQ_PORT, NRF_IRQ_PIN)

const uint8_t TX_ADDRESS[TX_ADR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};    /* 发送地址 */
const uint8_t RX_ADDRESS[RX_ADR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};    /* 接收地址 */

void nrf24l01_gpio_init(void)
{
    __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitTypeDef gpio;
    gpio.Pin = NRF_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(NRF_CS_PORT, &gpio);
    SPI_CS_HIGH(NRF_CS_PORT, NRF_CS_PIN);

    /* NRF CE     - PG8 */
    gpio.Pin = NRF_CE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(NRF_CE_PORT, &gpio);

    /* NRF IRQ    - PG6 (输入) */
    gpio.Pin = NRF_IRQ_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(NRF_IRQ_PORT, &gpio);
}

nrf24l01_init(void)
{
    SPI_DRV_Init();
    nrf24l01_gpio_init();
    SPI_CE(0);
    SPI_CS(1);
}



static uint8_t nrf24101_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t ret = 0;
    SPI_CS(0);
    ret = spi2_read_write_byte(reg);
    spi2_read_write_byte(value);
    SPI_CS(1);
    return ret;
}

static uint8_t nrf24101_read_reg(uint8_t reg)
{
    uint8_t ret = 0;
    SPI_CS(0);
    spi2_read_write_byte(reg);
    ret = spi2_read_write_byte(0xff);
    SPI_CS(1);
    return ret;
}

static uint8_t nrf24101_read_buf(uint8_t reg, uint8_t *pbuf, uint8_t len)
{
    uint8_t ret = 0, i = 0;
    SPI_CS(0);
    ret = spi2_read_write_byte(reg);
    for(i = 0; i < len; i++)
    {
        pbuf[i] = spi2_read_write_byte(0xff);
    }
    SPI_CS(1);
    return ret;
}

static uint8_t nrf24101_write_buf(uint8_t reg, uint8_t *pbuf, uint8_t len)
{
    uint8_t ret = 0, i = 0;
    SPI_CS(0);
    ret = spi2_read_write_byte(reg);
    for(i = 0; i < len; i++)
    {
        spi2_read_write_byte(pbuf[i]);
    }
    SPI_CS(1);
    return ret;
}

/**
 * @brief   设置NRF24L01进入接收模式
 *  @note   设置Rx地址，写RX数据宽度，设置RF频道，波特率
 *          当CE为高电平时，NRF24L01进入接收模式
 * @param   无
 * @return  无
 */
void nrf24l01_set_rx_mode(void)
{
    SPI_CE(0);
    nrf24101_write_buf(NRF_WRITE_REG + RX_ADDR_P0, (uint8_t *)RX_ADDRESS,
                       RX_ADR_WIDTH); /* 写RX节点地址 */
    
    nrf24101_write_reg(NRF_WRITE_REG + EN_AA, 0x01);     /* 使能通道0的自动应答 */
    nrf24101_write_reg(NRF_WRITE_REG + EN_RXADDR, 0x01); /* 使能通道0的接收地址 */
    nrf24101_write_reg(NRF_WRITE_REG + RF_CH, 40);       /* 设置RF通信频率 */
    /* 选择通道0的有效数据宽度 */
    nrf24101_write_reg(NRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH);
    /* 设置TX发射参数,0db增益,2Mbps,低噪声增益开启 */
    nrf24101_write_reg(NRF_WRITE_REG + RF_SETUP, 0x0f);
    /* 配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式 */
    nrf24101_write_reg(NRF_WRITE_REG + CONFIG, 0x0f);
    SPI_CE(1);
}

/**
 * @brief       NRF24L01进入发送模式
 *   @note      设置TX地址,写TX数据宽度,设置RX自动应答的地址,填充TX发送数据,选择RF频道,波特率和
 *              PWR_UP,CRC使能
 *              当CE变高后,即进入TX模式,并可以发送数据了, CE为高大于10us,则启动发送.
 * @param       无
 * @retval      无
 */
void nrf24l01_tx_mode(void)
{
    SPI_CE(0);
    nrf24101_write_buf(NRF_WRITE_REG + TX_ADDR, (uint8_t *)TX_ADDRESS, TX_ADR_WIDTH);       /* 写TX节点地址 */
    nrf24101_write_buf(NRF_WRITE_REG + RX_ADDR_P0, (uint8_t *)RX_ADDRESS, RX_ADR_WIDTH);    /* 设置RX节点地址,主要为了使能ACK */

    nrf24101_write_reg(NRF_WRITE_REG + EN_AA, 0x01);        /* 使能通道0的自动应答 */
    nrf24101_write_reg(NRF_WRITE_REG + EN_RXADDR, 0x01);    /* 使能通道0的接收地址 */
    nrf24101_write_reg(NRF_WRITE_REG + SETUP_RETR, 0x1a);   /* 设置自动重发间隔时间:500us + 86us;最大自动重发次数:10次 */
    nrf24101_write_reg(NRF_WRITE_REG + RF_CH, 40);          /* 设置RF通道为40 */
    nrf24101_write_reg(NRF_WRITE_REG + RF_SETUP, 0x0f);     /* 设置TX发射参数,0db增益,2Mbps */
    nrf24101_write_reg(NRF_WRITE_REG + CONFIG, 0x0e);       /* 配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,发送模式,开启所有中断 */
    SPI_CE(1); /* CE为高,10us后启动发送 */
}

/**
 * @brief       启动NRF24L01发送一次数据(数据长度 = TX_PLOAD_WIDTH)
 * @param       ptxbuf : 待发送数据首地址
 * @retval      发送完成状态
 *   @arg       0    : 发送成功
 *   @arg       1    : 达到最大发送次数,失败
 *   @arg       0XFF : 其他错误
 */
uint8_t nrf24l01_tx_packet(uint8_t *ptxbuf)
{
    uint8_t sta;
    uint8_t rval = 0XFF;
    
    NRF24L01_CE(0);
    nrf24l01_write_buf(WR_TX_PLOAD, ptxbuf, TX_PLOAD_WIDTH);    /* 写数据到TX BUF  TX_PLOAD_WIDTH个字节 */
    NRF24L01_CE(1); /* 启动发送 */

    while (NRF24L01_IRQ != 0);          /* 等待发送完成 */

    sta = nrf24l01_read_reg(STATUS);    /* 读取状态寄存器的值 */
    nrf24l01_write_reg(NRF_WRITE_REG + STATUS, sta);    /* 清除TX_DS或MAX_RT中断标志 */

    if (sta & MAX_TX)   /* 达到最大重发次数 */
    {
        nrf24l01_write_reg(FLUSH_TX, 0xff); /* 清除TX FIFO寄存器 */
        rval = 1;
    }

    if (sta & TX_OK)/* 发送完成 */
    {
        rval = 0;   /* 标记发送成功 */
    }

    return rval;    /* 返回结果 */
}

/**
 * @brief       启动NRF24L01接收一次数据(数据长度 = RX_PLOAD_WIDTH)
 * @param       prxbuf : 接收数据缓冲区首地址
 * @retval      接收完成状态
 *   @arg       0 : 接收成功
 *   @arg       1 : 失败
 */
uint8_t nrf24l01_rx_packet(uint8_t *prxbuf)
{
    uint8_t sta;
    uint8_t rval = 1;
    
    sta = nrf24l01_read_reg(STATUS); /* 读取状态寄存器的值 */
    nrf24l01_write_reg(NRF_WRITE_REG + STATUS, sta); /* 清除RX_OK中断标志 */

    if (sta & RX_OK)    /* 接收到数据 */
    {
        nrf24l01_read_buf(RD_RX_PLOAD, prxbuf, RX_PLOAD_WIDTH); /* 读取数据 */
        nrf24l01_write_reg(FLUSH_RX, 0xff); /* 清除RX FIFO寄存器 */
        rval = 0;       /* 标记接收完成 */
    }

    return rval;    /* 返回结果 */
}
