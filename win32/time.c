/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "file.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32)

/***********************************************************************
 *              GetLocalTime            (KERNEL32.228)
 */
VOID WINAPI GetLocalTime(LPSYSTEMTIME systime)
{
    time_t local_time;
    struct tm *local_tm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    local_time = tv.tv_sec;
    local_tm = localtime(&local_time);

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
VOID WINAPI GetSystemTime(LPSYSTEMTIME systime)
{
    time_t local_time;
    struct tm *local_tm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    local_time = tv.tv_sec;
    local_tm = gmtime(&local_time);

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
 *              SetSystemTime            (KERNEL32.507)
 */
BOOL WINAPI SetSystemTime(const SYSTEMTIME *systime)
{
    struct timeval tv;
    struct timezone tz;
    struct tm t;
    time_t sec;

    /* call gettimeofday to get the current timezone */
    gettimeofday(&tv, &tz);

    /* get the number of seconds */
    t.tm_sec = systime->wSecond;
    t.tm_min = systime->wMinute;
    t.tm_hour = systime->wHour;
    t.tm_mday = systime->wDay;
    t.tm_mon = systime->wMonth;
    t.tm_year = systime->wYear;
    sec = mktime (&t);

    /* set the new time */
    tv.tv_sec = sec;
    tv.tv_usec = systime->wMilliseconds * 1000;
    if (settimeofday(&tv, &tz))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/***********************************************************************
 *              GetTimeZoneInformation  (KERNEL32.302)
 */
DWORD WINAPI GetTimeZoneInformation(LPTIME_ZONE_INFORMATION tzinfo)
{
    time_t gmt, lt;
    struct tm *ptm;
    int daylight;

    memset(tzinfo, 0, sizeof(TIME_ZONE_INFORMATION));

    gmt = time(NULL);
    ptm=localtime(&gmt);
    daylight=ptm->tm_isdst;

    ptm = gmtime(&gmt);
    ptm->tm_isdst=daylight;
    lt = mktime(ptm);
    
    tzinfo->Bias = (lt - gmt) / 60;
    tzinfo->StandardBias = 0;
    tzinfo->DaylightBias = -60;

    return TIME_ZONE_ID_UNKNOWN;
}


/***********************************************************************
 *              SetTimeZoneInformation  (KERNEL32.515)
 */
BOOL WINAPI SetTimeZoneInformation(const LPTIME_ZONE_INFORMATION tzinfo)
{
    struct timezone tz;

    tz.tz_minuteswest = tzinfo->Bias;
#ifdef DST_NONE
    tz.tz_dsttime = DST_NONE;
#else
    tz.tz_dsttime = 0;
#endif
    return !settimeofday(NULL, &tz);
}


/***********************************************************************
 *              GetSystemTimeAsFileTime  (KERNEL32)
 */
VOID WINAPI GetSystemTimeAsFileTime(LPFILETIME systemtimeAsfiletime) {
	DOSFS_UnixTimeToFileTime(time(NULL),systemtimeAsfiletime,0);
}

/***********************************************************************
 *              SystemTimeToTzSpecificLocalTime32  (KERNEL32.683)
 */
BOOL WINAPI SystemTimeToTzSpecificLocalTime(
  LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
  LPSYSTEMTIME lpUniversalTime,
  LPSYSTEMTIME lpLocalTime) {

  FIXME(":stub\n"); 
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
       return FALSE;
}
