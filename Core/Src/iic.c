/**
 * @file        iic.c
 * @brief       软件 I2C 驱动 (GPIO 位带操作, 标准模式 ~100kHz)
 *
 * 引脚: PB6(SCL), PB7(SDA)
 *
 * 时序:
 *   - 起始信号: SCL=1 期间 SDA 产生下降沿
 *   - 停止信号: SCL=1 期间 SDA 产生上升沿
 *   - 发送数据: MSB first, SCL 上升沿锁存
 *   - 接收数据: MSB first, 主机控制 SCL
 *   - ACK/NACK: 第9个 SCL 周期
 *
 * 依赖 DWT 周期计数器做微秒延时 (与 onewire 共用)。
 */

#include "iic.h"
#include "cmsis_os2.h"

/*============================================================================*/
/* I2C 总线互斥锁                                                               */
/*============================================================================*/

static osMutexId_t iic_mutex = NULL;

/**
 * @brief  获取 I2C 总线锁 (阻塞直到可用)
 */
void IIC_MutexAcquire(void)
{
    if (iic_mutex != NULL)
        osMutexAcquire(iic_mutex, osWaitForever);
}

/**
 * @brief  释放 I2C 总线锁
 */
void IIC_MutexRelease(void)
{
    if (iic_mutex != NULL)
        osMutexRelease(iic_mutex);
}

/*============================================================================*/
/* 微秒延时 (DWT 周期计数器, 与 onewire.h 共用)                                  */
/*============================================================================*/

static void IIC_DelayUs(uint16_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

/*============================================================================*/
/* 初始化                                                                       */
/*============================================================================*/

/**
 * @brief  初始化软件 I2C GPIO
 *
 * PB6(SCL): 推挽输出
 * PB7(SDA): 开漏输出 (读取时释放总线, 允许从机拉低用于 ACK)
 */
void IIC_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* 确保 DWT 已初始化 (已初始化则无操作) */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* SCL: 推挽输出 */
    gpio.Pin   = IIC_SCL_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SCL_PORT, &gpio);

    /* SDA: 开漏输出 */
    gpio.Pin   = IIC_SDA_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(IIC_SDA_PORT, &gpio);

    /* 创建 I2C 总线互斥锁 (仅首次初始化时创建) */
    if (iic_mutex == NULL)
        iic_mutex = osMutexNew(NULL);
}

/*============================================================================*/
/* 起始信号                                                                     */
/*============================================================================*/

/**
 * @brief  产生 I2C 起始信号
 *
 * 时序: 总线空闲(SCL=1, SDA=1)时, SDA 先拉低, 然后 SCL 拉低
 */
void IIC_Start(void)
{
    IIC_SDA_H();
    IIC_SCL_H();
    IIC_DelayUs(4);
    IIC_SDA_L();
    IIC_DelayUs(4);
    IIC_SCL_L();
}

/*============================================================================*/
/* 停止信号                                                                     */
/*============================================================================*/

/**
 * @brief  产生 I2C 停止信号
 *
 * 时序: SCL=1 期间 SDA 产生上升沿
 */
void IIC_Stop(void)
{
    IIC_SDA_L();
    IIC_SCL_L();
    IIC_DelayUs(4);
    IIC_SCL_H();
    IIC_DelayUs(4);
    IIC_SDA_H();
    IIC_DelayUs(4);
}

/*============================================================================*/
/* 等待从机应答                                                                 */
/*============================================================================*/

/**
 * @brief  等待从机 ACK (SDA 被从机拉低)
 * @retval 0=收到 ACK, 1=未收到 ACK (NACK 或超时)
 */
IIC_StatusTypeDef IIC_Wait_Ack(void)
{
    uint16_t timeout = 0;

    IIC_SDA_H();          /* 主机释放 SDA */
    IIC_DelayUs(1);
    IIC_SCL_H();          /* SCL 上升沿: 从机输出 ACK */
    IIC_DelayUs(1);

    while (IIC_SDA_IN())  /* 等待 SDA 被从机拉低 */
    {
        if (++timeout > 250)
        {
            IIC_Stop();
            return IIC_TIMEOUT;     /* NACK / 超时 */
        }
    }

    IIC_SCL_L();
    return IIC_OK;             /* ACK */
}

/*============================================================================*/
/* 主机应答                                                                     */
/*============================================================================*/

/**
 * @brief  主机发送 ACK (SDA 拉低)
 */
void IIC_Ack(void)
{
    IIC_SDA_L();
    IIC_DelayUs(2);
    IIC_SCL_H();
    IIC_DelayUs(4);
    IIC_SCL_L();
    IIC_DelayUs(2);
    IIC_SDA_H();
}

/**
 * @brief  主机发送 NACK (SDA 释放)
 */
void IIC_NAck(void)
{
    IIC_SDA_H();
    IIC_DelayUs(2);
    IIC_SCL_H();
    IIC_DelayUs(4);
    IIC_SCL_L();
    IIC_DelayUs(2);
}

/*============================================================================*/
/* 发送一个字节                                                                 */
/*============================================================================*/

/**
 * @brief  发送 1 字节 (MSB first)
 * @param  data  要发送的字节
 *
 * SCL 低电平时 SDA 变化, SCL 上升沿从机锁存。
 * 发完 8 bit 后释放 SDA 准备接收 ACK。
 */
void IIC_Send_Byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
            IIC_SDA_H();
        else
            IIC_SDA_L();

        IIC_DelayUs(2);
        IIC_SCL_H();
        IIC_DelayUs(4);
        IIC_SCL_L();

        data <<= 1;
        IIC_DelayUs(2);
    }

    IIC_SDA_H();  /* 释放 SDA, 准备收 ACK */
}

/*============================================================================*/
/* 读取一个字节                                                                 */
/*============================================================================*/

/**
 * @brief  读取 1 字节 (MSB first)
 * @param  ack  1=读完后发送 ACK, 0=发送 NACK (结束读取)
 * @retval 读取到的字节
 */
uint8_t IIC_Read_Byte(uint8_t ack)
{
    uint8_t data = 0;

    IIC_SDA_H();  /* 释放 SDA, 让从机驱动 */

    for (uint8_t i = 0; i < 8; i++)
    {
        data <<= 1;

        IIC_DelayUs(2);
        IIC_SCL_H();
        IIC_DelayUs(2);

        if (IIC_SDA_IN())
            data |= 0x01;

        IIC_SCL_L();
        IIC_DelayUs(2);
    }

    /* 第9个 SCL: 发送 ACK/NACK */
    if (ack)
        IIC_Ack();
    else
        IIC_NAck();

    return data;
}

IIC_StatusTypeDef I2C_Write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    IIC_MutexAcquire();

    IIC_Start();
    IIC_Send_Byte(dev_addr);
    if (IIC_Wait_Ack()) goto fail;

    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack()) goto fail;

    for (int i = 0; i < len; i++) {
        IIC_Send_Byte(data[i]);
        if (IIC_Wait_Ack()) goto fail;
    }

    IIC_Stop();
    IIC_MutexRelease();
    return IIC_OK;

fail:
    IIC_Stop();
    IIC_MutexRelease();
    return IIC_TIMEOUT;
}

IIC_StatusTypeDef I2C_Read(uint8_t dev_addr, uint8_t reg,
                           uint8_t *data, uint16_t len)
{
    IIC_MutexAcquire();

    /* 写寄存器地址 */
    IIC_Start();
    IIC_Send_Byte(dev_addr & 0xFE);
    if (IIC_Wait_Ack() != IIC_OK) goto fail;

    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack() != IIC_OK) goto fail;

    /* 读数据 */
    IIC_Start();
    IIC_Send_Byte(dev_addr | 0x01);
    if (IIC_Wait_Ack() != IIC_OK) goto fail;

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1) {
            data[i] = IIC_Read_Byte(0); // 最后一个：NACK
        } else {
            data[i] = IIC_Read_Byte(1); // 其余：ACK
        }
    }

    IIC_Stop();
    IIC_MutexRelease();
    return IIC_OK;

fail:
    IIC_Stop();
    IIC_MutexRelease();
    return IIC_TIMEOUT;
}
