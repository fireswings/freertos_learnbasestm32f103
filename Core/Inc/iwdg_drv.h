#ifndef __IWDG_DRV_H
#define __IWDG_DRV_H

#include "main.h"

HAL_StatusTypeDef IWDG_DRV_Init(uint32_t timeout_s);
void IWDG_DRV_Feed(void);

#endif
