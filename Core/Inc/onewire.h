#ifndef __ONEWIRE_H
#define __ONEWIRE_H

#include "main.h"

/* GPIO bit-bang macros (direct register access for speed) */
#define OW_LOW()    (OW_GPIO_Port->BRR  = OW_Pin)
#define OW_HIGH()   (OW_GPIO_Port->BSRR = OW_Pin)
#define OW_READ()   ((OW_GPIO_Port->IDR & OW_Pin) ? 1 : 0)

/*
 * DWT cycle counter - shared microsecond delay for all drivers.
 * Call DWT_InitUs() once early in main() before any driver needs it.
 */
static inline void DWT_InitUs(void)
{
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

static inline void OW_DelayUs(uint16_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

void OW_Init(void);
uint8_t OW_Reset(void);
void OW_WriteByte(uint8_t data);
uint8_t OW_ReadByte(void);

#endif
