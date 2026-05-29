#include "app_temp.h"
#include "ds18b20.h"
#include "dht11.h"
#include "cmsis_os.h"
#include <stdio.h>

/* 传感器类型 */
typedef enum { SENSOR_NONE, SENSOR_DS18B20, SENSOR_DHT11 } SensorType;

void StartTempTask(void *argument)
{
    SensorType sensor = SENSOR_NONE;
    DHT11_Data dht11_data;

    OW_Init();

    /* 自动检测传感器类型 */
    if (OW_Reset())
    {
        sensor = SENSOR_DS18B20;
        printf("DS18B20 detected.\r\n");
    }
    else
    {
        DHT11_Init();
        if (DHT11_Read(&dht11_data))
        {
            sensor = SENSOR_DHT11;
            printf("DHT11 detected.\r\n");
        }
    }

    if (sensor == SENSOR_NONE)
    {
        printf("No sensor found on 1-Wire bus!\r\n");
        while (1)
            osDelay(1000);
    }

    for (;;)
    {
        if (sensor == SENSOR_DS18B20)
        {
            DS18B20_StartConversion();
            osDelay(750);
            float temp = DS18B20_ReadTemp();
            printf("Temperature: %.1f C\r\n", temp);
        }
        else
        {
            if (DHT11_Read(&dht11_data))
            {
                printf("Temperature: %.1f C  Humidity: %.1f%%\r\n",
                       dht11_data.temperature, dht11_data.humidity);
            }
            else
            {
                printf("DHT11 read error\r\n");
            }
        }
        osDelay(2000);
    }
}
