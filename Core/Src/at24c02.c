/**
 * @file        at24c02.c
 * @brief       24C02 EEPROM 驱动 (软件 I2C)
 *
 * 硬件连接: PB6(SCL), PB7(SDA)
 * 设备地址: 写 0xA0 / 读 0xA1 (7-bit addr 0x50)
 * 容量:     256 bytes (2Kbit), 8-byte page
 *
 * 24C02 写时序:
 *   START → DevAddr 0xA0(W) → ACK → MemAddr → ACK → Data → ACK → STOP
 *
 * 24C02 读时序 (复合时序):
 *   START → DevAddr 0xA0(W) → ACK → MemAddr → ACK →
 *   START → DevAddr 0xA1(R) → ACK → Data → NACK → STOP
 */

#include "at24c02.h"
#include "iic.h"

/*============================================================================*/
/* 初始化                                                                       */
/*============================================================================*/

/**
 * @brief  初始化 24C02 (软件 I2C GPIO)
 * @retval 始终返回 HAL_OK
 */
HAL_StatusTypeDef AT24C02_Init(void)
{
    IIC_Init();
    return HAL_OK;
}

/*============================================================================*/
/* 单字节写入                                                                   */
/*============================================================================*/

/**
 * @brief  向 24C02 指定地址写入 1 字节数据
 * @param  addr  EEPROM 内部地址 (0x00 ~ 0xFF)
 * @param  data  要写入的数据
 * @retval HAL_OK 成功, HAL_ERROR 失败 (从机 NACK)
 *
 * 时序: START → 0xA0(W) → ACK → MemAddr → ACK → Data → ACK → STOP
 * 写入后等待 10ms 内部编程周期。
 */
HAL_StatusTypeDef AT24C02_WriteByte(uint8_t addr, uint8_t data)
{
    IIC_Start();

    /* 发送设备地址 0xA0 (写) */
    IIC_Send_Byte(AT24C02_DEV_ADDR_WRITE);
    if (IIC_Wait_Ack())
        return HAL_ERROR;   /* 从机无应答 */

    /* 发送内存地址 */
    IIC_Send_Byte(addr);
    if (IIC_Wait_Ack())
        return HAL_ERROR;

    /* 发送数据 */
    IIC_Send_Byte(data);
    if (IIC_Wait_Ack())
        return HAL_ERROR;

    IIC_Stop();
    HAL_Delay(10);  /* 内部写入周期 (安全起见用 10ms) */

    return HAL_OK;
}

/*============================================================================*/
/* 单字节读取 (复合读时序)                                                       */
/*============================================================================*/

/**
 * @brief  从 24C02 指定地址读取 1 字节数据 (复合读时序)
 * @param  addr  EEPROM 内部地址 (0x00 ~ 0xFF)
 * @retval 读取到的数据 (失败返回 0xFF)
 *
 * 复合读时序:
 *   Phase 1 (Dummy Write):
 *     START → 0xA0(W) → ACK → MemAddr → ACK
 *   Phase 2 (Read):
 *     START → 0xA1(R) → ACK → Data → NACK → STOP
 */
uint8_t AT24C02_ReadByte(uint8_t addr)
{
    uint8_t data;

    /* Phase 1: Dummy Write — 发送内存地址 */
    IIC_Start();
    IIC_Send_Byte(AT24C02_DEV_ADDR_WRITE);   /* 0xA0, 写 */
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 0xFF;
    }
    IIC_Send_Byte(addr);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 0xFF;
    }

    /* Phase 2: 重新起始 + 读 */
    IIC_Start();
    IIC_Send_Byte(AT24C02_DEV_ADDR_READ);    /* 0xA1, 读 */
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 0xFF;
    }
    data = IIC_Read_Byte(0);   /* 读 1 字节, NACK 结束 */
    IIC_Stop();

    return data;
}

/*============================================================================*/
/* 检查 24C02 是否正常                                                           */
/*============================================================================*/

/**
 * @brief  检查 24C02 是否正常工作
 * @retval 0=正常, 1=异常
 *
 * 原理: 在末地址 255 写入 0x55 再读回, 比对确认。
 */
uint8_t AT24C02_Check(void)
{
    uint8_t temp;

    /* 先读末地址, 避免每次上电都写 */
    temp = AT24C02_ReadByte(255);
    if (temp == 0x55)
        return 0;   /* 之前写入过, 正常 */

    /* 首次上电: 写入测试 */
    if (AT24C02_WriteByte(255, 0x55) != HAL_OK)
        return 1;

    temp = AT24C02_ReadByte(255);
    if (temp == 0x55)
        return 0;

    return 1;
}

/*============================================================================*/
/* 页写入                                                                       */
/*============================================================================*/

/**
 * @brief  从指定地址开始写入多字节 (自动处理 8 字节页边界)
 * @param  addr  起始地址
 * @param  buf   数据缓冲区指针
 * @param  len   写入长度
 * @retval HAL status
 */
HAL_StatusTypeDef AT24C02_WritePage(uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t offset = 0;

    while (offset < len)
    {
        /* 当前页剩余空间 */
        uint8_t page_remain = AT24C02_PAGE_SIZE - (addr % AT24C02_PAGE_SIZE);
        uint8_t chunk = (len - offset) < page_remain ? (len - offset) : page_remain;

        IIC_Start();
        IIC_Send_Byte(AT24C02_DEV_ADDR_WRITE);
        if (IIC_Wait_Ack()) return HAL_ERROR;
        IIC_Send_Byte(addr);
        if (IIC_Wait_Ack()) return HAL_ERROR;

        for (uint8_t i = 0; i < chunk; i++)
        {
            IIC_Send_Byte(buf[offset + i]);
            if (IIC_Wait_Ack()) return HAL_ERROR;
        }

        IIC_Stop();
        HAL_Delay(10);

        addr    += chunk;
        offset  += chunk;
    }

    return HAL_OK;
}

/*============================================================================*/
/* 连续读取                                                                     */
/*============================================================================*/

/**
 * @brief  从指定地址开始连续读取多字节 (复合读时序)
 * @param  addr  起始地址
 * @param  buf   接收缓冲区指针
 * @param  len   读取长度
 * @retval HAL status
 */
HAL_StatusTypeDef AT24C02_ReadSeq(uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    /* Phase 1: Dummy Write — 发送起始地址 */
    IIC_Start();
    IIC_Send_Byte(AT24C02_DEV_ADDR_WRITE);
    if (IIC_Wait_Ack()) return HAL_ERROR;
    IIC_Send_Byte(addr);
    if (IIC_Wait_Ack()) return HAL_ERROR;

    /* Phase 2: 读 */
    IIC_Start();
    IIC_Send_Byte(AT24C02_DEV_ADDR_READ);
    if (IIC_Wait_Ack()) return HAL_ERROR;

    for (i = 0; i < len - 1; i++)
    {
        buf[i] = IIC_Read_Byte(1);  /* ACK: 继续读 */
    }
    buf[i] = IIC_Read_Byte(0);      /* NACK: 结束读取 */
    IIC_Stop();

    return HAL_OK;
}
