/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include "file.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);

/* maximum time adjustment in seconds for SetLocalTime and SetSystemTime */
#define SETTIME_MAX_ADJUST 120

/* TIME_GetBias: helper function calculates delta local time from UTC */
static int TIME_GetBias( time_t utc, int *pdaylight)
{
    struct tm *ptm = localtime(&utc);
    *pdaylight = ptm->tm_isdst; /* daylight for local timezone */
    ptm = gmtime(&utc);
    ptm->tm_isdst = *pdaylight; /* use local daylight, not that of Greenwich */
    return (int)(utc-mktime(ptm));
}


/***********************************************************************
 *              SetLocalTime            (KERNEL32.655)
 *
 * FIXME: correct ? Is the timezone param of settimeofday() needed ?
 * I don't have any docu about SetLocal/SystemTime(), argl...
 */
BOOL WINAPI SetLocalTime(const SYSTEMTIME *systime)
{
    struct timeval tv;
    struct tm t;
    time_t sec;
    time_t oldsec=time(NULL);
    int err;

    /* get the number of seconds */
    t.tm_sec = systime->wSecond;
    t.tm_min = systime->wMinute;
    t.tm_hour = systime->wHour;
    t.tm_mday = systime->wDay;
    t.tm_mon = systime->wMonth - 1;
    t.tm_year = systime->wYear - 1900;
    t.tm_isdst = -1;
    sec = mktime (&t);

    /* set the new time */
    tv.tv_sec = sec;
    tv.tv_usec = systime->wMilliseconds * 1000;

    /* error and sanity check*/
    if( sec == (time_t)-1 || abs((int)(sec-oldsec)) > SETTIME_MAX_ADJUST ){
        err = 1;
        SetLastError(ERROR_INVALID_PARAMETER);
    } else {
        err=settimeofday(&tv, NULL); /* 0 is OK, -1 is error */
        if(err == 0)
            return TRUE;
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
    }
    ERR("Cannot set time to %d/%d/%d %d:%d:%d Time adjustment %ld %s\n",
            systime->wYear, systime->wMonth, systime->wDay, systime->wHour,
            systime->wMinute, systime->wSecond,
            sec-oldsec, err == -1 ? "No Permission" : 
                sec==(time_t)-1 ? "" : "is too large." );
    return FALSE;
}


/***********************************************************************
 *           GetSystemTimeAdjustment     (KERNEL32.407)
 *
 */
DWORD WINAPI GetSystemTimeAdjustment( LPDWORD lpTimeAdjustment,
                                      LPDWORD lpTimeIncrement,
                                      LPBOOL lpTimeAdjustmentDisabled )
{
    *lpTimeAdjustment = 0;
    *lpTimeIncrement = 0;
    *lpTimeAdjustmentDisabled = TRUE;
    return TRUE;
}


/***********************************************************************
 *              SetSystemTime            (KERNEL32.507)
 */
BOOL WINAPI SetSystemTime(const SYSTEMTIME *systime)
{
    struct timeval tv;
    struct timezone tz;
    struct tm t;
    time_t sec, oldsec;
    int dst, bias;
    int err;

    /* call gettimeofday to get the current timezone */
    gettimeofday(&tv, &tz);
    oldsec=tv.tv_sec;
    /* get delta local time from utc */
    bias=TIME_GetBias(oldsec,&dst);

    /* get the number of seconds */
    t.tm_sec = systime->wSecond;
    t.tm_min = systime->wMinute;
    t.tm_hour = systime->wHour;
    t.tm_mday = systime->wDay;
    t.tm_mon = systime->wMonth - 1;
    t.tm_year = systime->wYear - 1900;
    t.tm_isdst = dst;
    sec = mktime (&t);
    /* correct for timezone and daylight */
    sec += bias;

    /* set the new time */
    tv.tv_sec = sec;
    tv.tv_usec = systime->wMilliseconds * 1000;

    /* error and sanity check*/
    if( sec == (time_t)-1 || abs((int)(sec-oldsec)) > SETTIME_MAX_ADJUST ){
        err = 1;
        SetLastError(ERROR_INVALID_PARAMETER);
    } else {
        err=settimeofday(&tv, NULL); /* 0 is OK, -1 is error */
        if(err == 0)
            return TRUE;
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
    }
    ERR("Cannot set time to %d/%d/%d %d:%d:%d Time adjustment %ld %s\n",
            systime->wYear, systime->wMonth, systime->wDay, systime->wHour,
            systime->wMinute, systime->wSecond,
            sec-oldsec, err == -1 ? "No Permission" :
                sec==(time_t)-1 ? "" : "is too large." );
    return FALSE;
}


/***********************************************************************
 *              GetTimeZoneInformation  (KERNEL32.302)
 */
DWORD WINAPI GetTimeZoneInformation(LPTIME_ZONE_INFORMATION tzinfo)
{
    time_t gmt;
    int bias, daylight;

    memset(tzinfo, 0, sizeof(TIME_ZONE_INFORMATION));

    gmt = time(NULL);
    bias=TIME_GetBias(gmt,&daylight);

    tzinfo->Bias = -bias / 60;
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
 *              SystemTimeToTzSpecificLocalTime  (KERNEL32.683)
 */
BOOL WINAPI SystemTimeToTzSpecificLocalTime(
  LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
  LPSYSTEMTIME lpUniversalTime,
  LPSYSTEMTIME lpLocalTime)
{
  FIXME(":stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/*********************************************************************
 *      TIME_ClockTimeToFileTime
 *      (olorin@fandra.org, 20-Sep-1998)
 *      Converts clock_t into FILETIME.
 *      Used by GetProcessTime.
 *      Differences to UnixTimeToFileTime:
 *          1) Divided by CLK_TCK
 *          2) Time is relative. There is no 'starting date', so there is 
 *             no need in offset correction, like in UnixTimeToFileTime
 *      FIXME: This function should be moved to a more appropriate .c file
 *      FIXME: On floating point operations, it is assumed that
 *             floating values are truncated on conversion to integer.
 */
static void TIME_ClockTimeToFileTime(clock_t unix_time, LPFILETIME filetime)
{
    double td = (unix_time*10000000.0)/CLK_TCK;
    /* Yes, double, because long int might overflow here. */
#if SIZEOF_LONG_LONG >= 8
    unsigned long long t = td;
    filetime->dwLowDateTime  = (UINT) t;
    filetime->dwHighDateTime = (UINT) (t >> 32);
#else
    double divider = 1. * (1 << 16) * (1 << 16);
    filetime->dwHighDateTime = (UINT) (td / divider);
    filetime->dwLowDateTime  = (UINT) (td - filetime->dwHighDateTime*divider);
    /* using floor() produces wierd results, better leave this as it is 
     * ( with (UINT32) convertion )
     */
#endif
}

/*********************************************************************
 *	GetProcessTimes				[KERNEL32.262]
 *
 * FIXME: lpCreationTime, lpExitTime are NOT INITIALIZED.
 * olorin@fandra.org: Would be nice to substract the cpu time,
 *                    used by Wine at startup.
 *                    Also, there is a need to separate times
 *                    used by different applications.
 */
BOOL WINAPI GetProcessTimes( HANDLE hprocess,LPFILETIME lpCreationTime,LPFILETIME lpExitTime,
                             LPFILETIME lpKernelTime, LPFILETIME lpUserTime )
{
    struct tms tms;

    times(&tms);
    TIME_ClockTimeToFileTime(tms.tms_utime,lpUserTime);
    TIME_ClockTimeToFileTime(tms.tms_stime,lpKernelTime);
    return TRUE;
}
