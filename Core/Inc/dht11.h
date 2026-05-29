#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

typedef struct {
    float temperature;
    float humidity;
} DHT11_Data;

uint8_t DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data *data);

#endif
