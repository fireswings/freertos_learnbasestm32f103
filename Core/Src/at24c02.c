/**
 * @file        at24c02.c
 * @brief       24C02 EEPROM 驱动 (硬件 I2C1)
 *
 * 硬件连接: PB6(SCL), PB7(SDA) — I2C1
 * 设备地址: 写 0xA0 / 读 0xA1 (7-bit addr 0x50)
 *
 * 24C02 写时序 (HAL_I2C_Mem_Write):
 *   START → 0xA0(W)→ACK → MemAddr→ACK → Data→ACK → STOP
 *
 * 24C02 读时序 (HAL_I2C_Mem_Read, 复合读写):
 *   START → 0xA0(W)→ACK → MemAddr→ACK →
 *   Repeated START → 0xA1(R)→ACK → Data → NACK → STOP
 */

#include "at24c02.h"

/*============================================================================*/
/* 私有变量                                                                    */
/*============================================================================*/

static I2C_HandleTypeDef hi2c1;

/*============================================================================*/
/* I2C MSP 初始化 (由 HAL_I2C_Init 回调)                                      */
/*============================================================================*/

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2c->Instance == AT24C02_I2C_INSTANCE)
    {
        /* 使能 I2C1 和 GPIOB 时钟 */
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PB6(SCL), PB7(SDA) — I2C 复用开漏输出 */
        gpio.Pin   = AT24C02_SCL_PIN | AT24C02_SDA_PIN;
        gpio.Mode  = GPIO_MODE_AF_OD;
        gpio.Pull  = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(AT24C02_SCL_PORT, &gpio);
    }
}

/*============================================================================*/
/* MSP 反初始化                                                                */
/*============================================================================*/

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == AT24C02_I2C_INSTANCE)
    {
        __HAL_RCC_I2C1_CLK_ENABLE();   /* 需要时钟来操作 deinit */
        HAL_GPIO_DeInit(AT24C02_SCL_PORT, AT24C02_SCL_PIN | AT24C02_SDA_PIN);
        __HAL_RCC_I2C1_CLK_DISABLE();
    }
}

/*============================================================================*/
/* 初始化                                                                      */
/*============================================================================*/

/**
 * @brief  初始化 24C02 (I2C1 硬件外设)
 * @retval HAL status
 *
 * I2C1 标准模式 100kHz, 7-bit 地址。
 * 上电后调用一次即可。
 */
HAL_StatusTypeDef AT24C02_Init(void)
{
    hi2c1.Instance             = AT24C02_I2C_INSTANCE;
    hi2c1.Init.ClockSpeed      = 100000;                 /* 100kHz 标准模式 */
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    return HAL_I2C_Init(&hi2c1);
}

/*============================================================================*/
/* 单字节写入                                                                  */
/*============================================================================*/

/**
 * @brief  向指定地址写入 1 字节
 * @param  addr  EEPROM 内部地址 (0x00 ~ 0xFF)
 * @param  data  要写入的数据
 * @retval HAL status
 *
 * 时序: START → 0xA0(W)→ACK → MemAddr→ACK → Data→ACK → STOP
 * 写入后等待 5ms 内部编程周期。
 *
 * @note   DevAddress: 0xA0 = (0x50 << 1) | 0 (W)
 */
HAL_StatusTypeDef AT24C02_WriteByte(uint8_t addr, uint8_t data)
{
    HAL_StatusTypeDef ret;

    /* HAL_I2C_Mem_Write 内部时序:
     *   START → DevAddr|W → ACK → MemAddr → ACK → Data → ACK → STOP
     * DevAddress = 0xA0 (左移后的 7 位地址, HAL 内部自行处理 R/W 位)
     */
    ret = HAL_I2C_Mem_Write(&hi2c1,
                            AT24C02_DEV_ADDR_WRITE,     /* 左移后的 7-bit 地址 */
                            addr,                        /* 内存地址 */
                            I2C_MEMADD_SIZE_8BIT,        /* 8 位地址 */
                            &data,                       /* 数据 */
                            1,                           /* 1 字节 */
                            AT24C02_I2C_TIMEOUT);        /* 超时 ms */

    /* EEPROM 内部写入周期 (5ms) */
    if (ret == HAL_OK)
        HAL_Delay(AT24C02_WRITE_DELAY_MS);

    return ret;
}

/*============================================================================*/
/* 单字节读取 (复合读时序)                                                      */
/*============================================================================*/

/**
 * @brief  从指定地址读取 1 字节 (复合读时序)
 * @param  addr  EEPROM 内部地址 (0x00 ~ 0xFF)
 * @retval 读取到的数据 (失败返回 0xFF)
 *
 * 复合读时序 (HAL_I2C_Mem_Read 内部处理):
 *   Phase 1 — Dummy Write:
 *     START → 0xA0(W) → ACK → MemAddr → ACK
 *   Phase 2 — Repeated START + Read:
 *     Repeated START → 0xA1(R) → ACK → Data → NACK → STOP
 *
 * HAL 的 I2C_RequestMemoryRead (line 6931) 在发送 MemAddr 后
 * 通过 CR1 START 位产生 Repeated START (而不是 STOP + new START),
 * 然后发送 DevAddr|R (line 7024: I2C_7BIT_ADD_READ)。
 * 最后在 HAL_I2C_Mem_Read line 2741/2750 禁 ACK + 设 STOP。
 */
uint8_t AT24C02_ReadByte(uint8_t addr)
{
    uint8_t data = 0xFF;

    if (HAL_I2C_Mem_Read(&hi2c1,
                         AT24C02_DEV_ADDR_WRITE,         /* 左移后的 7-bit 地址 */
                         addr,
                         I2C_MEMADD_SIZE_8BIT,
                         &data,
                         1,
                         AT24C02_I2C_TIMEOUT) != HAL_OK)
    {
        return 0xFF;    /* 读取失败 */
    }

    return data;
}

/*============================================================================*/
/* 页写入                                                                      */
/*============================================================================*/

/**
 * @brief  从指定地址开始写入多字节 (自动处理页边界)
 * @param  addr  起始地址
 * @param  buf   数据缓冲区指针
 * @param  len   写入长度 (字节)
 * @retval HAL status
 *
 * @note   自动处理 8 字节页边界，跨页时分多次写入。
 */
HAL_StatusTypeDef AT24C02_WritePage(uint8_t addr, uint8_t *buf, uint8_t len)
{
    HAL_StatusTypeDef ret = HAL_OK;
    uint8_t remaining = len;
    uint8_t offset = 0;

    while (remaining > 0)
    {
        /* 当前页剩余空间 */
        uint8_t page_remain = AT24C02_PAGE_SIZE - (addr % AT24C02_PAGE_SIZE);
        uint8_t chunk = (remaining < page_remain) ? remaining : page_remain;

        ret = HAL_I2C_Mem_Write(&hi2c1,
                                AT24C02_DEV_ADDR_WRITE,
                                addr,
                                I2C_MEMADD_SIZE_8BIT,
                                buf + offset,
                                chunk,
                                AT24C02_I2C_TIMEOUT);

        if (ret != HAL_OK)
            return ret;

        /* 等待内部写入周期 */
        HAL_Delay(AT24C02_WRITE_DELAY_MS);

        addr      += chunk;
        offset    += chunk;
        remaining -= chunk;
    }

    return ret;
}

/*============================================================================*/
/* 连续读取                                                                    */
/*============================================================================*/

/**
 * @brief  从指定地址开始连续读取多字节 (复合读时序)
 * @param  addr  起始地址
 * @param  buf   接收缓冲区指针
 * @param  len   读取长度 (字节)
 * @retval HAL status
 */
HAL_StatusTypeDef AT24C02_ReadSeq(uint8_t addr, uint8_t *buf, uint8_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                            AT24C02_DEV_ADDR_WRITE,
                            addr,
                            I2C_MEMADD_SIZE_8BIT,
                            buf,
                            len,
                            AT24C02_I2C_TIMEOUT);
}
