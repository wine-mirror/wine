/*
 * Win32 kernel time functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif
#ifdef HAVE_SYS_LIMITS_H
#include <sys/limits.h>
#elif defined(HAVE_MACHINE_LIMITS_H)
#include <machine/limits.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "kernel_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(time);

#define CALINFO_MAX_YEAR 2029

#define LL2FILETIME( ll, pft )\
    (pft)->dwLowDateTime = (UINT)(ll); \
    (pft)->dwHighDateTime = (UINT)((ll) >> 32);
#define FILETIME2LL( pft, ll) \
    ll = (((LONGLONG)((pft)->dwHighDateTime))<<32) + (pft)-> dwLowDateTime ;


static const int MonthLengths[2][12] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline BOOL IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0);
}

/***********************************************************************
 *  TIME_DayLightCompareDate
 *
 * Compares two dates without looking at the year.
 *
 * PARAMS
 *   date        [in] The local time to compare.
 *   compareDate [in] The daylight savings begin or end date.
 *
 * RETURNS
 *
 *  -1 if date < compareDate
 *   0 if date == compareDate
 *   1 if date > compareDate
 *  -2 if an error occurs
 */
static int TIME_DayLightCompareDate( const SYSTEMTIME *date,
    const SYSTEMTIME *compareDate )
{
    int limit_day, dayinsecs;

    if (date->wMonth < compareDate->wMonth)
        return -1; /* We are in a month before the date limit. */

    if (date->wMonth > compareDate->wMonth)
        return 1; /* We are in a month after the date limit. */

    /* if year is 0 then date is in day-of-week format, otherwise
     * it's absolute date.
     */
    if (compareDate->wYear == 0)
    {
        WORD First;
        /* compareDate->wDay is interpreted as number of the week in the month
         * 5 means: the last week in the month */
        int weekofmonth = compareDate->wDay;
          /* calculate the day of the first DayOfWeek in the month */
        First = ( 6 + compareDate->wDayOfWeek - date->wDayOfWeek + date->wDay 
               ) % 7 + 1;
        limit_day = First + 7 * (weekofmonth - 1);
        /* check needed for the 5th weekday of the month */
        if(limit_day > MonthLengths[date->wMonth==2 && IsLeapYear(date->wYear)]
                [date->wMonth - 1])
            limit_day -= 7;
    }
    else
    {
       limit_day = compareDate->wDay;
    }

    /* convert to seconds */
    limit_day = ((limit_day * 24  + compareDate->wHour) * 60 +
            compareDate->wMinute ) * 60;
    dayinsecs = ((date->wDay * 24  + date->wHour) * 60 +
            date->wMinute ) * 60 + date->wSecond;
    /* and compare */
    return dayinsecs < limit_day ? -1 :
           dayinsecs > limit_day ? 1 :
           0;   /* date is equal to the date limit. */
}

/***********************************************************************
 *  TIME_CompTimeZoneID
 *
 *  Computes the local time bias for a given time and time zone.
 *
 *  PARAMS
 *      pTZinfo     [in] The time zone data.
 *      lpFileTime  [in] The system or local time.
 *      islocal     [in] it is local time.
 *
 *  RETURNS
 *      TIME_ZONE_ID_INVALID    An error occurred
 *      TIME_ZONE_ID_UNKNOWN    There are no transition time known
 *      TIME_ZONE_ID_STANDARD   Current time is standard time
 *      TIME_ZONE_ID_DAYLIGHT   Current time is daylight savings time
 */
static DWORD TIME_CompTimeZoneID ( const TIME_ZONE_INFORMATION *pTZinfo,
    FILETIME *lpFileTime, BOOL islocal )
{
    int ret, year;
    BOOL beforeStandardDate, afterDaylightDate;
    DWORD retval = TIME_ZONE_ID_INVALID;
    LONGLONG llTime = 0; /* initialized to prevent gcc complaining */
    SYSTEMTIME SysTime;
    FILETIME ftTemp;

    if (pTZinfo->DaylightDate.wMonth != 0)
    {
        /* if year is 0 then date is in day-of-week format, otherwise
         * it's absolute date.
         */
        if (pTZinfo->StandardDate.wMonth == 0 ||
            (pTZinfo->StandardDate.wYear == 0 &&
            (pTZinfo->StandardDate.wDay<1 ||
            pTZinfo->StandardDate.wDay>5 ||
            pTZinfo->DaylightDate.wDay<1 ||
            pTZinfo->DaylightDate.wDay>5)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return TIME_ZONE_ID_INVALID;
        }

        if (!islocal) {
            FILETIME2LL( lpFileTime, llTime );
            llTime -= pTZinfo->Bias * (LONGLONG)600000000;
            LL2FILETIME( llTime, &ftTemp)
            lpFileTime = &ftTemp;
        }

        FileTimeToSystemTime(lpFileTime, &SysTime);
        year = SysTime.wYear;

        if (!islocal) {
            llTime -= pTZinfo->DaylightBias * (LONGLONG)600000000;
            LL2FILETIME( llTime, &ftTemp)
            FileTimeToSystemTime(lpFileTime, &SysTime);
        }

        /* check for daylight savings */
        if(year == SysTime.wYear) {
            ret = TIME_DayLightCompareDate( &SysTime, &pTZinfo->StandardDate);
            if (ret == -2)
                return TIME_ZONE_ID_INVALID;

            beforeStandardDate = ret < 0;
        } else
            beforeStandardDate = SysTime.wYear < year;

        if (!islocal) {
            llTime -= ( pTZinfo->StandardBias - pTZinfo->DaylightBias )
                * (LONGLONG)600000000;
            LL2FILETIME( llTime, &ftTemp)
            FileTimeToSystemTime(lpFileTime, &SysTime);
        }

        if(year == SysTime.wYear) {
            ret = TIME_DayLightCompareDate( &SysTime, &pTZinfo->DaylightDate);
            if (ret == -2)
                return TIME_ZONE_ID_INVALID;

            afterDaylightDate = ret >= 0;
        } else
            afterDaylightDate = SysTime.wYear > year;

        retval = TIME_ZONE_ID_STANDARD;
        if( pTZinfo->DaylightDate.wMonth <  pTZinfo->StandardDate.wMonth ) {
            /* Northern hemisphere */
            if( beforeStandardDate && afterDaylightDate )
                retval = TIME_ZONE_ID_DAYLIGHT;
        } else    /* Down south */
            if( beforeStandardDate || afterDaylightDate )
            retval = TIME_ZONE_ID_DAYLIGHT;
    } else 
        /* No transition date */
        retval = TIME_ZONE_ID_UNKNOWN;
        
    return retval;
}

/***********************************************************************
 *  TIME_TimeZoneID
 *
 *  Calculates whether daylight savings is on now.
 *
 *  PARAMS
 *      pTzi [in] Timezone info.
 *
 *  RETURNS
 *      TIME_ZONE_ID_INVALID    An error occurred
 *      TIME_ZONE_ID_UNKNOWN    There are no transition time known
 *      TIME_ZONE_ID_STANDARD   Current time is standard time
 *      TIME_ZONE_ID_DAYLIGHT   Current time is daylight savings time
 */
static DWORD TIME_ZoneID( const TIME_ZONE_INFORMATION *pTzi )
{
    FILETIME ftTime;
    GetSystemTimeAsFileTime( &ftTime);
    return TIME_CompTimeZoneID( pTzi, &ftTime, FALSE);
}

/***********************************************************************
 *  TIME_GetTimezoneBias
 *
 *  Calculates the local time bias for a given time zone.
 *
 * PARAMS
 *  pTZinfo    [in]  The time zone data.
 *  lpFileTime [in]  The system or local time.
 *  islocal    [in]  It is local time.
 *  pBias      [out] The calculated bias in minutes.
 *
 * RETURNS
 *  TRUE when the time zone bias was calculated.
 */
static BOOL TIME_GetTimezoneBias( const TIME_ZONE_INFORMATION *pTZinfo,
    FILETIME *lpFileTime, BOOL islocal, LONG *pBias )
{
    LONG bias = pTZinfo->Bias;
    DWORD tzid = TIME_CompTimeZoneID( pTZinfo, lpFileTime, islocal);

    if( tzid == TIME_ZONE_ID_INVALID)
        return FALSE;
    if (tzid == TIME_ZONE_ID_DAYLIGHT)
        bias += pTZinfo->DaylightBias;
    else if (tzid == TIME_ZONE_ID_STANDARD)
        bias += pTZinfo->StandardBias;
    *pBias = bias;
    return TRUE;
}


/***********************************************************************
 *              SetLocalTime            (KERNEL32.@)
 *
 *  Set the local time using current time zone and daylight
 *  savings settings.
 *
 * PARAMS
 *  systime [in] The desired local time.
 *
 * RETURNS
 *  Success: TRUE. The time was set.
 *  Failure: FALSE, if the time was invalid or caller does not have
 *           permission to change the time.
 */
BOOL WINAPI SetLocalTime( const SYSTEMTIME *systime )
{
    FILETIME ft;
    LARGE_INTEGER st, st2;
    NTSTATUS status;

    if( !SystemTimeToFileTime( systime, &ft ))
        return FALSE;
    st.u.LowPart = ft.dwLowDateTime;
    st.u.HighPart = ft.dwHighDateTime;
    RtlLocalTimeToSystemTime( &st, &st2 );

    if ((status = NtSetSystemTime(&st2, NULL)))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           GetSystemTimeAdjustment     (KERNEL32.@)
 *
 *  Get the period between clock interrupts and the amount the clock
 *  is adjusted each interrupt so as to keep it in sync with an external source.
 *
 * PARAMS
 *  lpTimeAdjustment [out] The clock adjustment per interrupt in 100's of nanoseconds.
 *  lpTimeIncrement  [out] The time between clock interrupts in 100's of nanoseconds.
 *  lpTimeAdjustmentDisabled [out] The clock synchronisation has been disabled.
 *
 * RETURNS
 *  TRUE.
 *
 * BUGS
 *  Only the special case of disabled time adjustments is supported.
 */
BOOL WINAPI GetSystemTimeAdjustment( PDWORD lpTimeAdjustment, PDWORD lpTimeIncrement,
    PBOOL  lpTimeAdjustmentDisabled )
{
    *lpTimeAdjustment = 0;
    *lpTimeIncrement = 10000000 / sysconf(_SC_CLK_TCK);
    *lpTimeAdjustmentDisabled = TRUE;
    return TRUE;
}


/***********************************************************************
 *              SetSystemTime            (KERNEL32.@)
 *
 *  Set the system time in utc.
 *
 * PARAMS
 *  systime [in] The desired system time.
 *
 * RETURNS
 *  Success: TRUE. The time was set.
 *  Failure: FALSE, if the time was invalid or caller does not have
 *           permission to change the time.
 */
BOOL WINAPI SetSystemTime( const SYSTEMTIME *systime )
{
    FILETIME ft;
    LARGE_INTEGER t;
    NTSTATUS status;

    if( !SystemTimeToFileTime( systime, &ft ))
        return FALSE;
    t.u.LowPart = ft.dwLowDateTime;
    t.u.HighPart = ft.dwHighDateTime;
    if ((status = NtSetSystemTime(&t, NULL)))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *              SetSystemTimeAdjustment  (KERNEL32.@)
 *
 *  Enables or disables the timing adjustments to the system's clock.
 *
 * PARAMS
 *  dwTimeAdjustment        [in] Number of units to add per clock interrupt.
 *  bTimeAdjustmentDisabled [in] Adjustment mode.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI SetSystemTimeAdjustment( DWORD dwTimeAdjustment, BOOL bTimeAdjustmentDisabled )
{
    /* Fake function for now... */
    FIXME("(%08x,%d): stub !\n", dwTimeAdjustment, bTimeAdjustmentDisabled);
    return TRUE;
}

/***********************************************************************
 *              GetTimeZoneInformation  (KERNEL32.@)
 *
 *  Get information about the current local time zone.
 *
 * PARAMS
 *  tzinfo [out] Destination for time zone information.
 *
 * RETURNS
 *  TIME_ZONE_ID_INVALID    An error occurred
 *  TIME_ZONE_ID_UNKNOWN    There are no transition time known
 *  TIME_ZONE_ID_STANDARD   Current time is standard time
 *  TIME_ZONE_ID_DAYLIGHT   Current time is daylight savings time
 */
DWORD WINAPI GetTimeZoneInformation( LPTIME_ZONE_INFORMATION tzinfo )
{
    NTSTATUS status;

    status = RtlQueryTimeZoneInformation( (RTL_TIME_ZONE_INFORMATION*)tzinfo );
    if ( status != STATUS_SUCCESS )
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return TIME_ZONE_ID_INVALID;
    }
    return TIME_ZoneID( tzinfo );
}

/***********************************************************************
 *              SetTimeZoneInformation  (KERNEL32.@)
 *
 *  Change the settings of the current local time zone.
 *
 * PARAMS
 *  tzinfo [in] The new time zone.
 *
 * RETURNS
 *  Success: TRUE. The time zone was updated with the settings from tzinfo.
 *  Failure: FALSE.
 */
BOOL WINAPI SetTimeZoneInformation( const TIME_ZONE_INFORMATION *tzinfo )
{
    NTSTATUS status;
    status = RtlSetTimeZoneInformation( (const RTL_TIME_ZONE_INFORMATION *)tzinfo );
    if ( status != STATUS_SUCCESS )
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *              SystemTimeToTzSpecificLocalTime  (KERNEL32.@)
 *
 *  Convert a utc system time to a local time in a given time zone.
 *
 * PARAMS
 *  lpTimeZoneInformation [in]  The desired time zone.
 *  lpUniversalTime       [in]  The utc time to base local time on.
 *  lpLocalTime           [out] The local time in the time zone.
 *
 * RETURNS
 *  Success: TRUE. lpLocalTime contains the converted time
 *  Failure: FALSE.
 */

BOOL WINAPI SystemTimeToTzSpecificLocalTime(
    const TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    const SYSTEMTIME *lpUniversalTime, LPSYSTEMTIME lpLocalTime )
{
    FILETIME ft;
    LONG lBias;
    LONGLONG llTime;
    TIME_ZONE_INFORMATION tzinfo;

    if (lpTimeZoneInformation != NULL)
    {
        tzinfo = *lpTimeZoneInformation;
    }
    else
    {
        if (GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID)
            return FALSE;
    }

    if (!SystemTimeToFileTime(lpUniversalTime, &ft))
        return FALSE;
    FILETIME2LL( &ft, llTime)
    if (!TIME_GetTimezoneBias(&tzinfo, &ft, FALSE, &lBias))
        return FALSE;
    /* convert minutes to 100-nanoseconds-ticks */
    llTime -= (LONGLONG)lBias * 600000000;
    LL2FILETIME( llTime, &ft)

    return FileTimeToSystemTime(&ft, lpLocalTime);
}


/***********************************************************************
 *              TzSpecificLocalTimeToSystemTime  (KERNEL32.@)
 *
 *  Converts a local time to a time in utc.
 *
 * PARAMS
 *  lpTimeZoneInformation [in]  The desired time zone.
 *  lpLocalTime           [in]  The local time.
 *  lpUniversalTime       [out] The calculated utc time.
 *
 * RETURNS
 *  Success: TRUE. lpUniversalTime contains the converted time.
 *  Failure: FALSE.
 */
BOOL WINAPI TzSpecificLocalTimeToSystemTime(
    const TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    const SYSTEMTIME *lpLocalTime, LPSYSTEMTIME lpUniversalTime)
{
    FILETIME ft;
    LONG lBias;
    LONGLONG t;
    TIME_ZONE_INFORMATION tzinfo;

    if (lpTimeZoneInformation != NULL)
    {
        tzinfo = *lpTimeZoneInformation;
    }
    else
    {
        if (GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID)
            return FALSE;
    }

    if (!SystemTimeToFileTime(lpLocalTime, &ft))
        return FALSE;
    FILETIME2LL( &ft, t)
    if (!TIME_GetTimezoneBias(&tzinfo, &ft, TRUE, &lBias))
        return FALSE;
    /* convert minutes to 100-nanoseconds-ticks */
    t += (LONGLONG)lBias * 600000000;
    LL2FILETIME( t, &ft)
    return FileTimeToSystemTime(&ft, lpUniversalTime);
}


/***********************************************************************
 *              GetSystemTimeAsFileTime  (KERNEL32.@)
 *
 *  Get the current time in utc format.
 *
 *  RETURNS
 *   Nothing.
 */
VOID WINAPI GetSystemTimeAsFileTime(
    LPFILETIME time) /* [out] Destination for the current utc time */
{
    LARGE_INTEGER t;
    NtQuerySystemTime( &t );
    time->dwLowDateTime = t.u.LowPart;
    time->dwHighDateTime = t.u.HighPart;
}


/*********************************************************************
 *      TIME_ClockTimeToFileTime    (olorin@fandra.org, 20-Sep-1998)
 *
 *  Used by GetProcessTimes to convert clock_t into FILETIME.
 *
 *      Differences to UnixTimeToFileTime:
 *          1) Divided by CLK_TCK
 *          2) Time is relative. There is no 'starting date', so there is
 *             no need for offset correction, like in UnixTimeToFileTime
 */
static void TIME_ClockTimeToFileTime(clock_t unix_time, LPFILETIME filetime)
{
    long clocksPerSec = sysconf(_SC_CLK_TCK);
    ULONGLONG secs = (ULONGLONG)unix_time * 10000000 / clocksPerSec;
    filetime->dwLowDateTime  = (DWORD)secs;
    filetime->dwHighDateTime = (DWORD)(secs >> 32);
}

/*********************************************************************
 *	GetProcessTimes				(KERNEL32.@)
 *
 *  Get the user and kernel execution times of a process,
 *  along with the creation and exit times if known.
 *
 * PARAMS
 *  hprocess       [in]  The process to be queried.
 *  lpCreationTime [out] The creation time of the process.
 *  lpExitTime     [out] The exit time of the process if exited.
 *  lpKernelTime   [out] The time spent in kernel routines in 100's of nanoseconds.
 *  lpUserTime     [out] The time spent in user routines in 100's of nanoseconds.
 *
 * RETURNS
 *  TRUE.
 *
 * NOTES
 *  olorin@fandra.org:
 *  Would be nice to subtract the cpu time used by Wine at startup.
 *  Also, there is a need to separate times used by different applications.
 *
 * BUGS
 *  KernelTime and UserTime are always for the current process
 */
BOOL WINAPI GetProcessTimes( HANDLE hprocess, LPFILETIME lpCreationTime,
    LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime )
{
    struct tms tms;
    KERNEL_USER_TIMES pti;

    times(&tms);
    TIME_ClockTimeToFileTime(tms.tms_utime,lpUserTime);
    TIME_ClockTimeToFileTime(tms.tms_stime,lpKernelTime);
    if (NtQueryInformationProcess( hprocess, ProcessTimes, &pti, sizeof(pti), NULL))
        return FALSE;
    LL2FILETIME( pti.CreateTime.QuadPart, lpCreationTime);
    LL2FILETIME( pti.ExitTime.QuadPart, lpExitTime);
    return TRUE;
}

/*********************************************************************
 *	GetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI GetCalendarInfoA(LCID lcid, CALID Calendar, CALTYPE CalType,
			    LPSTR lpCalData, int cchData, LPDWORD lpValue)
{
    int ret;
    LPWSTR lpCalDataW = NULL;

    if (NLS_IsUnicodeOnlyLcid(lcid))
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    if (cchData &&
        !(lpCalDataW = HeapAlloc(GetProcessHeap(), 0, cchData*sizeof(WCHAR))))
      return 0;

    ret = GetCalendarInfoW(lcid, Calendar, CalType, lpCalDataW, cchData, lpValue);
    if(ret && lpCalDataW && lpCalData)
      WideCharToMultiByte(CP_ACP, 0, lpCalDataW, cchData, lpCalData, cchData, NULL, NULL);
    else if (CalType & CAL_RETURN_NUMBER)
        ret *= sizeof(WCHAR);
    HeapFree(GetProcessHeap(), 0, lpCalDataW);

    return ret;
}

/*********************************************************************
 *	GetCalendarInfoW				(KERNEL32.@)
 *
 */
int WINAPI GetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType,
			    LPWSTR lpCalData, int cchData, LPDWORD lpValue)
{
    if (CalType & CAL_NOUSEROVERRIDE)
	FIXME("flag CAL_NOUSEROVERRIDE used, not fully implemented\n");
    if (CalType & CAL_USE_CP_ACP)
	FIXME("flag CAL_USE_CP_ACP used, not fully implemented\n");

    if (CalType & CAL_RETURN_NUMBER) {
        if (!lpValue)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
	if (lpCalData != NULL)
	    WARN("lpCalData not NULL (%p) when it should!\n", lpCalData);
	if (cchData != 0)
	    WARN("cchData not 0 (%d) when it should!\n", cchData);
    } else {
	if (lpValue != NULL)
	    WARN("lpValue not NULL (%p) when it should!\n", lpValue);
    }

    /* FIXME: No verification is made yet wrt Locale
     * for the CALTYPES not requiring GetLocaleInfoA */
    switch (CalType & ~(CAL_NOUSEROVERRIDE|CAL_RETURN_NUMBER|CAL_USE_CP_ACP)) {
	case CAL_ICALINTVALUE:
            if (CalType & CAL_RETURN_NUMBER)
                return GetLocaleInfoW(Locale, LOCALE_RETURN_NUMBER | LOCALE_ICALENDARTYPE,
                        (LPWSTR)lpValue, 2);
            return GetLocaleInfoW(Locale, LOCALE_ICALENDARTYPE, lpCalData, cchData);
	case CAL_SCALNAME:
            FIXME("Unimplemented caltype %d\n", CalType & 0xffff);
            if (lpCalData) *lpCalData = 0;
	    return 1;
	case CAL_IYEAROFFSETRANGE:
            FIXME("Unimplemented caltype %d\n", CalType & 0xffff);
	    return 0;
	case CAL_SERASTRING:
            FIXME("Unimplemented caltype %d\n", CalType & 0xffff);
	    return 0;
	case CAL_SSHORTDATE:
	    return GetLocaleInfoW(Locale, LOCALE_SSHORTDATE, lpCalData, cchData);
	case CAL_SLONGDATE:
	    return GetLocaleInfoW(Locale, LOCALE_SLONGDATE, lpCalData, cchData);
	case CAL_SDAYNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME1, lpCalData, cchData);
	case CAL_SDAYNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME2, lpCalData, cchData);
	case CAL_SDAYNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME3, lpCalData, cchData);
	case CAL_SDAYNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME4, lpCalData, cchData);
	case CAL_SDAYNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME5, lpCalData, cchData);
	case CAL_SDAYNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME6, lpCalData, cchData);
	case CAL_SDAYNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME7, lpCalData, cchData);
	case CAL_SABBREVDAYNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME1, lpCalData, cchData);
	case CAL_SABBREVDAYNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME2, lpCalData, cchData);
	case CAL_SABBREVDAYNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME3, lpCalData, cchData);
	case CAL_SABBREVDAYNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME4, lpCalData, cchData);
	case CAL_SABBREVDAYNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME5, lpCalData, cchData);
	case CAL_SABBREVDAYNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME6, lpCalData, cchData);
	case CAL_SABBREVDAYNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME7, lpCalData, cchData);
	case CAL_SMONTHNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME1, lpCalData, cchData);
	case CAL_SMONTHNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME2, lpCalData, cchData);
	case CAL_SMONTHNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME3, lpCalData, cchData);
	case CAL_SMONTHNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME4, lpCalData, cchData);
	case CAL_SMONTHNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME5, lpCalData, cchData);
	case CAL_SMONTHNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME6, lpCalData, cchData);
	case CAL_SMONTHNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME7, lpCalData, cchData);
	case CAL_SMONTHNAME8:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME8, lpCalData, cchData);
	case CAL_SMONTHNAME9:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME9, lpCalData, cchData);
	case CAL_SMONTHNAME10:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME10, lpCalData, cchData);
	case CAL_SMONTHNAME11:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME11, lpCalData, cchData);
	case CAL_SMONTHNAME12:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME12, lpCalData, cchData);
	case CAL_SMONTHNAME13:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME13, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME1, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME2, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME3, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME4, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME5, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME6, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME7, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME8:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME8, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME9:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME9, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME10:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME10, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME11:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME11, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME12:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME12, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME13:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME13, lpCalData, cchData);
	case CAL_SYEARMONTH:
	    return GetLocaleInfoW(Locale, LOCALE_SYEARMONTH, lpCalData, cchData);
	case CAL_ITWODIGITYEARMAX:
            if (CalType & CAL_RETURN_NUMBER)
            {
                *lpValue = CALINFO_MAX_YEAR;
                return sizeof(DWORD) / sizeof(WCHAR);
            }
            else
            {
                static const WCHAR fmtW[] = {'%','u',0};
                WCHAR buffer[10];
                int ret = snprintfW( buffer, 10, fmtW, CALINFO_MAX_YEAR  ) + 1;
                if (!lpCalData) return ret;
                if (ret <= cchData)
                {
                    strcpyW( lpCalData, buffer );
                    return ret;
                }
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }
	    break;
	default:
            FIXME("Unknown caltype %d\n",CalType & 0xffff);
            SetLastError(ERROR_INVALID_FLAGS);
            return 0;
    }
    return 0;
}

/*********************************************************************
 *	GetCalendarInfoEx				(KERNEL32.@)
 */
int WINAPI GetCalendarInfoEx(LPCWSTR locale, CALID calendar, LPCWSTR lpReserved, CALTYPE caltype,
    LPWSTR data, int len, DWORD *value)
{
    LCID lcid = LocaleNameToLCID(locale, 0);
    FIXME("(%s, %d, %p, 0x%08x, %p, %d, %p): semi-stub\n", debugstr_w(locale), calendar, lpReserved, caltype,
        data, len, value);
    return GetCalendarInfoW(lcid, calendar, caltype, data, len, value);
}

/*********************************************************************
 *	SetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI	SetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPCSTR lpCalData)
{
    FIXME("(%08x,%08x,%08x,%s): stub\n",
	  Locale, Calendar, CalType, debugstr_a(lpCalData));
    return 0;
}

/*********************************************************************
 *	SetCalendarInfoW				(KERNEL32.@)
 *
 *
 */
int WINAPI	SetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType, LPCWSTR lpCalData)
{
    FIXME("(%08x,%08x,%08x,%s): stub\n",
	  Locale, Calendar, CalType, debugstr_w(lpCalData));
    return 0;
}

/*********************************************************************
 *      LocalFileTimeToFileTime                         (KERNEL32.@)
 */
BOOL WINAPI LocalFileTimeToFileTime( const FILETIME *localft, LPFILETIME utcft )
{
    NTSTATUS status;
    LARGE_INTEGER local, utc;

    local.u.LowPart = localft->dwLowDateTime;
    local.u.HighPart = localft->dwHighDateTime;
    if (!(status = RtlLocalTimeToSystemTime( &local, &utc )))
    {
        utcft->dwLowDateTime = utc.u.LowPart;
        utcft->dwHighDateTime = utc.u.HighPart;
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    return !status;
}

/*********************************************************************
 *      FileTimeToLocalFileTime                         (KERNEL32.@)
 */
BOOL WINAPI FileTimeToLocalFileTime( const FILETIME *utcft, LPFILETIME localft )
{
    NTSTATUS status;
    LARGE_INTEGER local, utc;

    utc.u.LowPart = utcft->dwLowDateTime;
    utc.u.HighPart = utcft->dwHighDateTime;
    if (!(status = RtlSystemTimeToLocalTime( &utc, &local )))
    {
        localft->dwLowDateTime = local.u.LowPart;
        localft->dwHighDateTime = local.u.HighPart;
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    return !status;
}

/*********************************************************************
 *      FileTimeToSystemTime                            (KERNEL32.@)
 */
BOOL WINAPI FileTimeToSystemTime( const FILETIME *ft, LPSYSTEMTIME syst )
{
    TIME_FIELDS tf;
    LARGE_INTEGER t;

    t.u.LowPart = ft->dwLowDateTime;
    t.u.HighPart = ft->dwHighDateTime;
    RtlTimeToTimeFields(&t, &tf);

    syst->wYear = tf.Year;
    syst->wMonth = tf.Month;
    syst->wDay = tf.Day;
    syst->wHour = tf.Hour;
    syst->wMinute = tf.Minute;
    syst->wSecond = tf.Second;
    syst->wMilliseconds = tf.Milliseconds;
    syst->wDayOfWeek = tf.Weekday;
    return TRUE;
}

/*********************************************************************
 *      SystemTimeToFileTime                            (KERNEL32.@)
 */
BOOL WINAPI SystemTimeToFileTime( const SYSTEMTIME *syst, LPFILETIME ft )
{
    TIME_FIELDS tf;
    LARGE_INTEGER t;

    tf.Year = syst->wYear;
    tf.Month = syst->wMonth;
    tf.Day = syst->wDay;
    tf.Hour = syst->wHour;
    tf.Minute = syst->wMinute;
    tf.Second = syst->wSecond;
    tf.Milliseconds = syst->wMilliseconds;

    if( !RtlTimeFieldsToTime(&tf, &t)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ft->dwLowDateTime = t.u.LowPart;
    ft->dwHighDateTime = t.u.HighPart;
    return TRUE;
}

/*********************************************************************
 *      CompareFileTime                                 (KERNEL32.@)
 *
 * Compare two FILETIME's to each other.
 *
 * PARAMS
 *  x [I] First time
 *  y [I] time to compare to x
 *
 * RETURNS
 *  -1, 0, or 1 indicating that x is less than, equal to, or greater
 *  than y respectively.
 */
INT WINAPI CompareFileTime( const FILETIME *x, const FILETIME *y )
{
    if (!x || !y) return -1;

    if (x->dwHighDateTime > y->dwHighDateTime)
        return 1;
    if (x->dwHighDateTime < y->dwHighDateTime)
        return -1;
    if (x->dwLowDateTime > y->dwLowDateTime)
        return 1;
    if (x->dwLowDateTime < y->dwLowDateTime)
        return -1;
    return 0;
}

/*********************************************************************
 *      GetLocalTime                                    (KERNEL32.@)
 *
 * Get the current local time.
 *
 * PARAMS
 *  systime [O] Destination for current time.
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI GetLocalTime(LPSYSTEMTIME systime)
{
    FILETIME lft;
    LARGE_INTEGER ft, ft2;

    NtQuerySystemTime(&ft);
    RtlSystemTimeToLocalTime(&ft, &ft2);
    lft.dwLowDateTime = ft2.u.LowPart;
    lft.dwHighDateTime = ft2.u.HighPart;
    FileTimeToSystemTime(&lft, systime);
}

/*********************************************************************
 *      GetSystemTime                                   (KERNEL32.@)
 *
 * Get the current system time.
 *
 * PARAMS
 *  systime [O] Destination for current time.
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI GetSystemTime(LPSYSTEMTIME systime)
{
    FILETIME ft;
    LARGE_INTEGER t;

    NtQuerySystemTime(&t);
    ft.dwLowDateTime = t.u.LowPart;
    ft.dwHighDateTime = t.u.HighPart;
    FileTimeToSystemTime(&ft, systime);
}

/*********************************************************************
 *      GetDaylightFlag                                   (KERNEL32.@)
 *
 *  Specifies if daylight savings time is in operation.
 *
 * NOTES
 *  This function is called from the Win98's control applet timedate.cpl.
 *
 * RETURNS
 *  TRUE if daylight savings time is in operation.
 *  FALSE otherwise.
 */
BOOL WINAPI GetDaylightFlag(void)
{
    TIME_ZONE_INFORMATION tzinfo;
    return GetTimeZoneInformation( &tzinfo) == TIME_ZONE_ID_DAYLIGHT;
}

/***********************************************************************
 *           DosDateTimeToFileTime   (KERNEL32.@)
 */
BOOL WINAPI DosDateTimeToFileTime( WORD fatdate, WORD fattime, LPFILETIME ft)
{
    struct tm newtm;
#ifndef HAVE_TIMEGM
    struct tm *gtm;
    time_t time1, time2;
#endif

    newtm.tm_sec  = (fattime & 0x1f) * 2;
    newtm.tm_min  = (fattime >> 5) & 0x3f;
    newtm.tm_hour = (fattime >> 11);
    newtm.tm_mday = (fatdate & 0x1f);
    newtm.tm_mon  = ((fatdate >> 5) & 0x0f) - 1;
    newtm.tm_year = (fatdate >> 9) + 80;
    newtm.tm_isdst = -1;
#ifdef HAVE_TIMEGM
    RtlSecondsSince1970ToTime( timegm(&newtm), (LARGE_INTEGER *)ft );
#else
    time1 = mktime(&newtm);
    gtm = gmtime(&time1);
    time2 = mktime(gtm);
    RtlSecondsSince1970ToTime( 2*time1-time2, (LARGE_INTEGER *)ft );
#endif
    return TRUE;
}


/***********************************************************************
 *           FileTimeToDosDateTime   (KERNEL32.@)
 */
BOOL WINAPI FileTimeToDosDateTime( const FILETIME *ft, LPWORD fatdate,
                                     LPWORD fattime )
{
    LARGE_INTEGER       li;
    ULONG               t;
    time_t              unixtime;
    struct tm*          tm;

    if (!fatdate || !fattime)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    li.u.LowPart = ft->dwLowDateTime;
    li.u.HighPart = ft->dwHighDateTime;
    if (!RtlTimeToSecondsSince1970( &li, &t ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    unixtime = t;
    tm = gmtime( &unixtime );
    if (fattime)
        *fattime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
    if (fatdate)
        *fatdate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5)
                   + tm->tm_mday;
    return TRUE;
}

/*********************************************************************
 *      GetSystemTimes                                  (KERNEL32.@)
 *
 * Retrieves system timing information
 *
 * PARAMS
 *  lpIdleTime [O] Destination for idle time.
 *  lpKernelTime [O] Destination for kernel time.
 *  lpUserTime [O] Destination for user time.
 *
 * RETURNS
 *  TRUE if success, FALSE otherwise.
 */
BOOL WINAPI GetSystemTimes(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
    FIXME("(%p,%p,%p): Stub!\n", lpIdleTime, lpKernelTime, lpUserTime);

    return FALSE;
}

/***********************************************************************
 *           GetDynamicTimeZoneInformation   (KERNEL32.@)
 */
DWORD WINAPI GetDynamicTimeZoneInformation(PDYNAMIC_TIME_ZONE_INFORMATION info)
{
    FIXME("(%p) stub!\n", info);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TIME_ZONE_ID_INVALID;
}

/***********************************************************************
 *           QueryUnbiasedInterruptTime   (KERNEL32.@)
 */
BOOL WINAPI QueryUnbiasedInterruptTime(ULONGLONG *time)
{
    TRACE("(%p)\n", time);
    if (!time) return FALSE;
    RtlQueryUnbiasedInterruptTime(time);
    return TRUE;
}
