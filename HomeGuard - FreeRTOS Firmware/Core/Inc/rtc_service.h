
#ifndef RTC_SERVICE_H_
#define RTC_SERVICE_H_


#include <stddef.h>

#define RTC_MAGIC 0xA5A5

void RTC_Service_Init(void);
void RTC_GetTimestamp(char *out, size_t len);

#endif
