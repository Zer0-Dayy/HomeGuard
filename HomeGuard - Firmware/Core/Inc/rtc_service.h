
#ifndef RTC_SERVICE_H_
#define RTC_SERVICE_H_


#include <stddef.h>
#include "rtc.h"

#define RTC_MAGIC 0xA5A5

#define START_HOUR 23U
#define START_MINUTE 25U
#define START_SECOND 00U
#define START_DAY RTC_WEEKDAY_WEDNESDAY
#define START_MONTH RTC_MONTH_DECEMBER
#define START_DATE 17U
#define START_YEAR 25U

void RTC_Service_Init(void);
void RTC_GetTimestamp(char *out, size_t len);

#endif
