#ifndef __RTC_DRV_H
#define __RTC_DRV_H

#include "main.h"

/*
 * Initialize RTC with LSE (32.768kHz) clock source.
 *
 * Enables PWR clock, backup domain access, LSE oscillator,
 * and configures the RTC with auto-prescaler for 1-second timebase.
 *
 * Must be called once before the RTOS scheduler starts.
 */
HAL_StatusTypeDef RTC_DRV_Init(void);

/*
 * Set RTC time (24-hour format, binary mode).
 *   hour: 0–23, min: 0–59, sec: 0–59
 */
HAL_StatusTypeDef RTC_DRV_SetTime(uint8_t hour, uint8_t min, uint8_t sec);

/*
 * Get current RTC time.
 *   hour: 0–23, min: 0–59, sec: 0–59
 */
HAL_StatusTypeDef RTC_DRV_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec);

/*
 * Set RTC date (binary mode).
 *   year: 0–99 (e.g., 26 for 2026)
 *   month: 1–12
 *   day: 1–31
 *   week_day: 0=Sunday, 1=Monday, ..., 6=Saturday
 */
HAL_StatusTypeDef RTC_DRV_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t week_day);

/*
 * Get current RTC date.
 */
HAL_StatusTypeDef RTC_DRV_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *week_day);

#endif /* __RTC_DRV_H */
