/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *              GetLocalTime            (KERNEL32.228)
 */
VOID GetLocalTime(LPSYSTEMTIME systime)
{
    time_t local_time;
    struct tm *local_tm;
    struct timeval tv;

    time(&local_time);
    local_tm = localtime(&local_time);
    gettimeofday(&tv, NULL);

    systime->wYear = local_tm->tm_year + 1900;
    systime->wMonth = local_tm->tm_mon + 1;
    systime->wDayOfWeek = local_tm->tm_wday;
    systime->wDay = local_tm->tm_mday;
    systime->wHour = local_tm->tm_hour;
    systime->wMinute = local_tm->tm_min;
    systime->wSecond = local_tm->tm_sec;
    systime->wMilliseconds = (tv.tv_usec / 1000) % 1000;
}

/***********************************************************************
 *              GetSystemTime            (KERNEL32.285)
 */
VOID GetSystemTime(LPSYSTEMTIME systime)
{
    time_t local_time;
    struct tm *local_tm;
    struct timeval tv;

    time(&local_time);
    local_tm = gmtime(&local_time);
    gettimeofday(&tv, NULL);

    systime->wYear = local_tm->tm_year + 1900;
    systime->wMonth = local_tm->tm_mon + 1;
    systime->wDayOfWeek = local_tm->tm_wday;
    systime->wDay = local_tm->tm_mday;
    systime->wHour = local_tm->tm_hour;
    systime->wMinute = local_tm->tm_min;
    systime->wSecond = local_tm->tm_sec;
    systime->wMilliseconds = (tv.tv_usec / 1000) % 1000;
}

/***********************************************************************
 *              GetTimeZoneInformation  (KERNEL32.302)
 */
DWORD GetTimeZoneInformation(LPTIME_ZONE_INFORMATION tzinfo)
{
    time_t gmt, lt;

    memset(tzinfo, 0, sizeof(TIME_ZONE_INFORMATION));

    gmt = time(NULL);
    lt = mktime(localtime(&gmt));
    tzinfo->Bias = (gmt - lt) / 60;
    tzinfo->StandardBias = 0;
    tzinfo->DaylightBias = -60;

    return TIME_ZONE_ID_UNKNOWN;
}

/***********************************************************************
 *              Sleep  (KERNEL32.523)
 */
VOID Sleep(DWORD cMilliseconds)
{
    if(cMilliseconds == INFINITE)
        while(1) { /* Spin forever */ }
    usleep(cMilliseconds*1000);
}
