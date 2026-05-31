#include "dht11.h"
#include "onewire.h"
#include "cmsis_os.h"

static uint8_t dht11_wait_low(uint16_t us)
{
    uint16_t t = 0;
    while (OW_READ() && t < us) { OW_DelayUs(1); t++; }
    return (t < us);
}

static uint8_t dht11_wait_high(uint16_t us)
{
    uint16_t t = 0;
    while (!OW_READ() && t < us) { OW_DelayUs(1); t++; }
    return (t < us);
}

uint8_t DHT11_Init(void)
{
    DWT_InitUs();  /* ensure DWT cycle counter is running */
    OW_HIGH();     /* release shared bus line */
    osDelay(1000);
    return 1;
}

uint8_t DHT11_Read(DHT11_Data *data)
{
    uint8_t buf[5] = {0};

    /* 1. 发送起始信号：拉低 20ms */
    OW_LOW();
    osDelay(20);
    OW_HIGH();

    /* 2. 等待 DHT11 响应 */
    __disable_irq();
    if (!dht11_wait_low(100)) { __enable_irq(); return 0; }   /* DHT11 拉低 80µs */
    if (!dht11_wait_high(100)) { __enable_irq(); return 0; }  /* DHT11 拉高 80µs */

    /* 3. 读取 40 位数据 */
    for (uint8_t j = 0; j < 5; j++)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (!dht11_wait_low(80)) { __enable_irq(); return 0; }
            OW_DelayUs(35);
            buf[j] <<= 1;
            if (OW_READ()) buf[j] |= 1;
            dht11_wait_high(80);
        }
    }
    __enable_irq();

    /* 4. 校验 */
    if (buf[0] + buf[1] + buf[2] + buf[3] != buf[4])
        return 0;

    data->humidity    = buf[0] + buf[1] * 0.1f;
    data->temperature = buf[2] + buf[3] * 0.1f;
    return 1;
}
