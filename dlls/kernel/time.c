/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include "file.h"
#include "ntddk.h"
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
 *  Sets the local time using current time zone and daylight
 *  savings settings.
 *
 * RETURNS
 *
 *  True if the time was set, false if the time was invalid or the
 *  necessary permissions were not held.
 */
BOOL WINAPI SetLocalTime( const SYSTEMTIME *systime /* The desired local time. */ )
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
 *  Indicates the period between clock interrupt and the amount the clock
 *  is adjusted each interrupt so as to keep it insync with an external source.
 *
 * RETURNS
 *
 *  Always returns true.
 *
 * BUGS
 *
 *  Only the special case of disabled time adjustments is supported.
 *  (also the signature is wrong it should have a return type of BOOL)
 */
DWORD WINAPI GetSystemTimeAdjustment(
	 LPDWORD lpTimeAdjustment, /* The clock adjustment per interupt in 100's of nanoseconds. */
	 LPDWORD lpTimeIncrement, /* The time between clock interupts in 100's of nanoseconds. */
	 LPBOOL lpTimeAdjustmentDisabled /* The clock synchonisation has been disabled. */ )
{
    *lpTimeAdjustment = 0;
    *lpTimeIncrement = 0;
    *lpTimeAdjustmentDisabled = TRUE;
    return TRUE;
}


/***********************************************************************
 *              SetSystemTime            (KERNEL32.665)
 *
 *  Sets the system time (utc).
 *
 * RETURNS
 *
 *  True if the time was set, false if the time was invalid or the
 *  necessary permissions were not held.
 */
BOOL WINAPI SetSystemTime(
    const SYSTEMTIME *systime /* The desired system time. */)
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
 *              GetTimeZoneInformation  (KERNEL32.424)
 *
 *  Fills in the a time zone information structure with values based on
 *  the current local time.
 *
 * RETURNS
 *
 *  The daylight savings time standard or TIME_ZONE_ID_INVALID if the call failed.
 */
DWORD WINAPI GetTimeZoneInformation(
	LPTIME_ZONE_INFORMATION tzinfo /* The time zone structure to be filled in. */)
{
    time_t gmt;
    int bias, daylight;

    memset(tzinfo, 0, sizeof(TIME_ZONE_INFORMATION));

    gmt = time(NULL);
    bias=TIME_GetBias(gmt,&daylight);

    tzinfo->Bias = -bias / 60;
    tzinfo->StandardBias = 0;
    tzinfo->DaylightBias = -60;

    return TIME_ZONE_ID_STANDARD;
}


/***********************************************************************
 *              SetTimeZoneInformation  (KERNEL32.673)
 *
 *  Set the local time zone with values based on the time zone structure.
 *
 * RETURNS
 *
 *  True on successful setting of the time zone.
 *
 * BUGS
 *
 *  Use the obsolete unix timezone structure and tz_dsttime member.
 */
BOOL WINAPI SetTimeZoneInformation(
	const LPTIME_ZONE_INFORMATION tzinfo /* The new time zone. */)
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
 *
 *  Converts the system time (utc) to the local time in the specified time zone.
 *
 * RETURNS
 *
 *  Returns true when the local time was calculated.
 *
 * BUGS
 *
 *  Does not handle daylight savings time adjustments correctly.
 */
BOOL WINAPI SystemTimeToTzSpecificLocalTime(
	LPTIME_ZONE_INFORMATION lpTimeZoneInformation, /* The desired time zone. */
	LPSYSTEMTIME lpUniversalTime, /* The utc time to base local time on. */
	LPSYSTEMTIME lpLocalTime /* The local time in the time zone. */)
{
  FIXME(":stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}


/***********************************************************************
 *              GetSystemTimeAsFileTime  (KERNEL32.408)
 *
 *  Fills in a file time structure with the current time in UTC format.
 */
VOID WINAPI GetSystemTimeAsFileTime(
	LPFILETIME time /* The file time struct to be filled with the system time. */)
{
    NtQuerySystemTime( (LARGE_INTEGER *)time );
}


/*********************************************************************
 *      TIME_ClockTimeToFileTime    (olorin@fandra.org, 20-Sep-1998)
 *
 *  Used by GetProcessTimes to convert clock_t into FILETIME.
 *
 *      Differences to UnixTimeToFileTime:
 *          1) Divided by CLK_TCK
 *          2) Time is relative. There is no 'starting date', so there is 
 *             no need in offset correction, like in UnixTimeToFileTime
 */
static void TIME_ClockTimeToFileTime(clock_t unix_time, LPFILETIME filetime)
{
    LONGLONG secs = RtlEnlargedUnsignedMultiply( unix_time, 10000000 );
    ((LARGE_INTEGER *)filetime)->QuadPart = RtlExtendedLargeIntegerDivide( secs, CLK_TCK, NULL );
}

/*********************************************************************
 *	GetProcessTimes				(KERNEL32.378)
 *
 *  Returns the user and kernel execution times of a process,
 *  along with the creation and exit times if known.
 *
 *  olorin@fandra.org:
 *  Would be nice to subtract the cpu time, used by Wine at startup.
 *  Also, there is a need to separate times used by different applications.
 *
 * RETURNS
 *
 *  Always returns true.
 *
 * BUGS
 *
 *  lpCreationTime, lpExitTime are NOT INITIALIZED.
 */
BOOL WINAPI GetProcessTimes(
	HANDLE hprocess, /* The process to be queried (obtained from PROCESS_QUERY_INFORMATION). */
	LPFILETIME lpCreationTime, /* The creation time of the process. */
	LPFILETIME lpExitTime, /* The exit time of the process if exited. */
	LPFILETIME lpKernelTime, /* The time spent in kernal routines in 100's of nanoseconds. */
	LPFILETIME lpUserTime /* The time spent in user routines in 100's of nanoseconds. */)
{
    struct tms tms;

    times(&tms);
    TIME_ClockTimeToFileTime(tms.tms_utime,lpUserTime);
    TIME_ClockTimeToFileTime(tms.tms_stime,lpKernelTime);
    return TRUE;
}
