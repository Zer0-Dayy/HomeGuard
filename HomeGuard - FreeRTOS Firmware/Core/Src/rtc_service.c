#include "rtc_service.h"
#include "rtc.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

void RTC_Service_Init(void)
{
    HAL_PWR_EnableBkUpAccess();

    uint32_t marker = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
    if (marker == RTC_MAGIC)
    {
        return;
    }

    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    time.Hours   = 22;
    time.Minutes = 10;
    time.Seconds = 00;

    date.WeekDay = RTC_WEEKDAY_TUESDAY;
    date.Month   = RTC_MONTH_DECEMBER;
    date.Date    = 17;
    date.Year    = 25;

    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_MAGIC);
}


void RTC_GetTimestamp(char *out, size_t len)
{
    if (!out || len < 20)
    {
        return;
    }

    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    snprintf(out, len,
             "%04u-%02u-%02uT%02u:%02u:%02u",
             2000U + date.Year,
             date.Month,
             date.Date,
             time.Hours,
             time.Minutes,
             time.Seconds);
}
