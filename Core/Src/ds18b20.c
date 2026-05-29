#include "ds18b20.h"
#include "onewire.h"

#define DS18B20_SKIP_ROM   0xCC
#define DS18B20_CONVERT_T  0x44
#define DS18B20_READ_SCR   0xBE

/* 初始化并检测传感器 */
uint8_t DS18B20_Init(void)
{
    OW_Init();
    return OW_Reset();  /* 1 = 有设备, 0 = 无设备 */
}

/* 启动温度转换 */
void DS18B20_StartConversion(void)
{
    OW_Reset();
    OW_WriteByte(DS18B20_SKIP_ROM);
    OW_WriteByte(DS18B20_CONVERT_T);
}

/* 读取温度 (°C)，需在转换完成后调用（≥750ms） */
float DS18B20_ReadTemp(void)
{
    uint8_t lsb, msb;
    int16_t raw;

    OW_Reset();
    OW_WriteByte(DS18B20_SKIP_ROM);
    OW_WriteByte(DS18B20_READ_SCR);
    lsb = OW_ReadByte();
    msb = OW_ReadByte();

    raw = ((int16_t)msb << 8) | lsb;
    return raw * 0.0625f;
}
