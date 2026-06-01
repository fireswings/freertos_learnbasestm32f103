#include "rtc_drv.h"

static RTC_HandleTypeDef hrtc;

/*
 * Initialize RTC with LSE (32.768kHz) as clock source.
 *
 * The LSE oscillator is connected to PC14/PC15 on this board.
 * RTC_AUTO_1_SECOND tells the HAL to automatically compute
 * the asynchronous prescaler so the timebase is exactly 1 Hz.
 *
 * Returns:
 *   HAL_OK — RTC already running (keeps existing time)
 *   1      — RTC was configured for the first time (backup domain was lost,
 *           caller should set current date/time)
 */
HAL_StatusTypeDef RTC_DRV_Init(void)
{
    HAL_StatusTypeDef status;

    /* Enable power clock and backup domain access */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    /* Check if RTC is already ticking — if so, don't reset the counter.
     * RCC_BDCR_RTCEN bit survives soft reset as long as backup domain is powered. */
    if ((RCC->BDCR & RCC_BDCR_RTCEN) && __HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY))
    {
        /* RTC already configured and running. Just init the software handle,
         * but do NOT call HAL_RTC_Init() — it would reset the prescaler and
         * wipe the counter. */
        hrtc.Instance = RTC;
        hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
        hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
        hrtc.State = HAL_RTC_STATE_READY;
        return HAL_OK;
    }

    /* First boot or backup domain lost: do full hardware init */

    /* Enable LSE oscillator */
    RCC_OscInitTypeDef osc_init = {0};
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    osc_init.LSEState = RCC_LSE_ON;
    if (HAL_RCC_OscConfig(&osc_init) != HAL_OK)
        Error_Handler();

    /* Select LSE as RTC clock source and enable RTC */
    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
    __HAL_RCC_RTC_ENABLE();

    /* Init RTC with auto 1-second prescaler (this resets the counter!) */
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;

    status = HAL_RTC_Init(&hrtc);
    if (status != HAL_OK)
        Error_Handler();

    return 1; /* Return 1 to indicate first init — caller should set time */
}

/*
 * Set RTC time (24-hour binary format).
 *
 * Note: according to STM32F1 HAL convention, SetDate should be called
 * before SetTime when setting both.
 */
HAL_StatusTypeDef RTC_DRV_SetTime(uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours = hour;
    sTime.Minutes = min;
    sTime.Seconds = sec;
    return HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

/*
 * Get current RTC time.
 *
 * IMPORTANT: GetTime must be called before GetDate on STM32F1,
 * because the date is shadowed when time is read.
 */
HAL_StatusTypeDef RTC_DRV_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    RTC_TimeTypeDef sTime = {0};
    HAL_StatusTypeDef status = HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    if (status == HAL_OK)
    {
        *hour = sTime.Hours;
        *min = sTime.Minutes;
        *sec = sTime.Seconds;
    }
    return status;
}

/*
 * Set RTC date (binary format).
 */
HAL_StatusTypeDef RTC_DRV_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t week_day)
{
    RTC_DateTypeDef sDate = {0};
    sDate.Year = year;
    sDate.Month = month;
    sDate.Date = day;
    sDate.WeekDay = week_day;
    return HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

/*
 * Get current RTC date.
 *
 * Must be called after RTC_DRV_GetTime().
 */
HAL_StatusTypeDef RTC_DRV_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *week_day)
{
    RTC_DateTypeDef sDate = {0};
    HAL_StatusTypeDef status = HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    if (status == HAL_OK)
    {
        *year = sDate.Year;
        *month = sDate.Month;
        *day = sDate.Date;
        *week_day = sDate.WeekDay;
    }
    return status;
}
