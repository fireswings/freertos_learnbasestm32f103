#include "onewire.h"

/* Release 1-Wire bus and pull high */
void OW_Init(void)
{
    OW_HIGH();
}

/* Reset bus and detect slave presence. Returns 1=device present, 0=no device */
uint8_t OW_Reset(void)
{
    uint8_t presence;

    __disable_irq();
    OW_LOW();
    OW_DelayUs(500);
    OW_HIGH();
    OW_DelayUs(60);
    presence = OW_READ() ? 0 : 1;
    __enable_irq();
    OW_DelayUs(420);

    return presence;
}

static void OW_WriteBit(uint8_t bit)
{
    __disable_irq();
    if (bit)
    {
        OW_LOW();
        OW_DelayUs(2);
        OW_HIGH();
        OW_DelayUs(60);
    }
    else
    {
        OW_LOW();
        OW_DelayUs(60);
        OW_HIGH();
        OW_DelayUs(2);
    }
    __enable_irq();
}

static uint8_t OW_ReadBit(void)
{
    uint8_t bit;

    __disable_irq();
    OW_LOW();
    OW_DelayUs(2);
    OW_HIGH();
    OW_DelayUs(12);
    bit = OW_READ();
    __enable_irq();
    OW_DelayUs(50);

    return bit;
}

void OW_WriteByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        OW_WriteBit(data & 0x01);
        data >>= 1;
    }
}

uint8_t OW_ReadByte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        data >>= 1;
        if (OW_ReadBit())
            data |= 0x80;
    }
    return data;
}
