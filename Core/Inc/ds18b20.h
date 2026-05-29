#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

uint8_t DS18B20_Init(void);
void DS18B20_StartConversion(void);
float DS18B20_ReadTemp(void);

#endif
