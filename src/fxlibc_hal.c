/* Hardware Abstraction Layer (HAL) implementation for fxlibc. We can group
   this in a single file because LTO will prune away unused functions. */

#include <fxlibc/hal.h>
#include <gint/rtc.h>
#include <gint/kmalloc.h>
#include <errno.h>

void *fxlibc_hal_malloc(size_t size)
{
    return kmalloc(size, NULL);
}

void fxlibc_hal_free(void *ptr)
{
    return kfree(ptr);
}

void *fxlibc_hal_realloc(void *ptr, size_t size)
{
    return krealloc(ptr, size);
}

void fxlibc_hal_rawtime(struct tm *tm)
{
    rtc_time_t rtc;
    rtc_get_time(&rtc);
    tm->tm_sec   = rtc.seconds;
    tm->tm_min   = rtc.minutes;
    tm->tm_hour  = rtc.hours;
    tm->tm_mday  = rtc.month_day;
    tm->tm_mon   = rtc.month;
    tm->tm_year  = rtc.year - 1900;
    tm->tm_isdst = 0;
}

clock_t fxlibc_hal_clock(void)
{
    return (CLOCKS_PER_SEC * (uint64_t)rtc_ticks()) / 128;
}
