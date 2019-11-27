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
#ifdef __APPLE__
# include <mach/mach_time.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "kernel_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(time);

static inline void longlong_to_filetime( LONGLONG t, FILETIME *ft )
{
    ft->dwLowDateTime = (DWORD)t;
    ft->dwHighDateTime = (DWORD)(t >> 32);
}

static inline LONGLONG filetime_to_longlong( const FILETIME *ft )
{
    return (((LONGLONG)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
}

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000

/* return a monotonic time counter, in Win32 ticks */
static inline ULONGLONG monotonic_counter(void)
{
    LARGE_INTEGER counter;

#ifdef __APPLE__
    static mach_timebase_info_data_t timebase;

    if (!timebase.denom) mach_timebase_info( &timebase );
#ifdef HAVE_MACH_CONTINUOUS_TIME
    if (&mach_continuous_time != NULL)
        return mach_continuous_time() * timebase.numer / timebase.denom / 100;
#endif
    return mach_absolute_time() * timebase.numer / timebase.denom / 100;
#elif defined(HAVE_CLOCK_GETTIME)
    struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
    if (!clock_gettime( CLOCK_MONOTONIC_RAW, &ts ))
        return ts.tv_sec * (ULONGLONG)TICKSPERSEC + ts.tv_nsec / 100;
#endif
    if (!clock_gettime( CLOCK_MONOTONIC, &ts ))
        return ts.tv_sec * (ULONGLONG)TICKSPERSEC + ts.tv_nsec / 100;
#endif

    NtQueryPerformanceCounter( &counter, NULL );
    return counter.QuadPart;
}

static const WCHAR mui_stdW[] = { 'M','U','I','_','S','t','d',0 };
static const WCHAR mui_dltW[] = { 'M','U','I','_','D','l','t',0 };

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
 *      time        [in] The system or local time.
 *      islocal     [in] it is local time.
 *
 *  RETURNS
 *      TIME_ZONE_ID_INVALID    An error occurred
 *      TIME_ZONE_ID_UNKNOWN    There are no transition time known
 *      TIME_ZONE_ID_STANDARD   Current time is standard time
 *      TIME_ZONE_ID_DAYLIGHT   Current time is daylight savings time
 */
static DWORD TIME_CompTimeZoneID ( const TIME_ZONE_INFORMATION *pTZinfo,
                                   LONGLONG time, BOOL islocal )
{
    int ret, year;
    BOOL beforeStandardDate, afterDaylightDate;
    DWORD retval = TIME_ZONE_ID_INVALID;
    SYSTEMTIME SysTime;
    FILETIME ft;

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

        if (!islocal)
            time -= pTZinfo->Bias * (LONGLONG)600000000;

        longlong_to_filetime( time, &ft );
        FileTimeToSystemTime( &ft, &SysTime );
        year = SysTime.wYear;

        if (!islocal) {
            time -= pTZinfo->DaylightBias * (LONGLONG)600000000;
            longlong_to_filetime( time, &ft );
            FileTimeToSystemTime( &ft, &SysTime );
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
            time -= ( pTZinfo->StandardBias - pTZinfo->DaylightBias ) * (LONGLONG)600000000;
            longlong_to_filetime( time, &ft );
            FileTimeToSystemTime( &ft, &SysTime );
        }

        if(year == SysTime.wYear) {
            ret = TIME_DayLightCompareDate( &SysTime, &pTZinfo->DaylightDate );
            if (ret == -2)
                return TIME_ZONE_ID_INVALID;

            afterDaylightDate = ret >= 0;
        } else
            afterDaylightDate = SysTime.wYear > year;

        retval = TIME_ZONE_ID_STANDARD;
        if( pTZinfo->DaylightDate.wMonth < pTZinfo->StandardDate.wMonth ) {
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
    LARGE_INTEGER now;

    NtQuerySystemTime( &now );
    return TIME_CompTimeZoneID( pTzi, now.QuadPart, FALSE );
}

/***********************************************************************
 *  TIME_GetTimezoneBias
 *
 *  Calculates the local time bias for a given time zone.
 *
 * PARAMS
 *  pTZinfo    [in]  The time zone data.
 *  time       [in]  The system or local time.
 *  islocal    [in]  It is local time.
 *  pBias      [out] The calculated bias in minutes.
 *
 * RETURNS
 *  TRUE when the time zone bias was calculated.
 */
static BOOL TIME_GetTimezoneBias( const TIME_ZONE_INFORMATION *pTZinfo,
                                  LONGLONG time, BOOL islocal, LONG *pBias )
{
    LONG bias = pTZinfo->Bias;
    DWORD tzid = TIME_CompTimeZoneID( pTZinfo, time, islocal );

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
 *  TIME_GetSpecificTimeZoneKey
 *
 *  Opens the registry key for the time zone with the given name.
 *
 * PARAMS
 *  key_name   [in]  The time zone name.
 *  result     [out] The open registry key handle.
 *
 * RETURNS
 *  TRUE if successful.
 */
static BOOL TIME_GetSpecificTimeZoneKey( const WCHAR *key_name, HANDLE *result )
{
    static const WCHAR Time_ZonesW[] = { '\\','R','E','G','I','S','T','R','Y','\\',
        'M','a','c','h','i','n','e','\\',
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'T','i','m','e',' ','Z','o','n','e','s',0 };
    HANDLE time_zones_key;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    NTSTATUS status;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, Time_ZonesW );
    if (!set_ntstatus( NtOpenKey( &time_zones_key, KEY_READ, &attr ))) return FALSE;

    attr.RootDirectory = time_zones_key;
    RtlInitUnicodeString( &nameW, key_name );
    status = NtOpenKey( result, KEY_READ, &attr );

    NtClose( time_zones_key );
    return set_ntstatus( status );
}

static BOOL reg_query_value(HKEY hkey, LPCWSTR name, DWORD type, void *data, DWORD count)
{
    UNICODE_STRING nameW;
    char buf[256];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buf;

    if (count > sizeof(buf) - sizeof(KEY_VALUE_PARTIAL_INFORMATION))
        return FALSE;

    RtlInitUnicodeString(&nameW, name);

    if (!set_ntstatus( NtQueryValueKey(hkey, &nameW, KeyValuePartialInformation,
                                       buf, sizeof(buf), &count)))
        return FALSE;

    if (info->Type != type)
    {
        SetLastError( ERROR_DATATYPE_MISMATCH );
        return FALSE;
    }

    memcpy(data, info->Data, info->DataLength);
    return TRUE;
}

/***********************************************************************
 *  TIME_GetSpecificTimeZoneInfo
 *
 *  Returns time zone information for the given time zone and year.
 *
 * PARAMS
 *  key_name   [in]  The time zone name.
 *  year       [in]  The year, if Dynamic DST is used.
 *  dynamic    [in]  Whether to use Dynamic DST.
 *  result     [out] The time zone information.
 *
 * RETURNS
 *  TRUE if successful.
 */
static BOOL TIME_GetSpecificTimeZoneInfo( const WCHAR *key_name, WORD year,
    BOOL dynamic, DYNAMIC_TIME_ZONE_INFORMATION *tzinfo )
{
    static const WCHAR Dynamic_DstW[] = { 'D','y','n','a','m','i','c',' ','D','S','T',0 };
    static const WCHAR fmtW[] = { '%','d',0 };
    static const WCHAR stdW[] = { 'S','t','d',0 };
    static const WCHAR dltW[] = { 'D','l','t',0 };
    static const WCHAR tziW[] = { 'T','Z','I',0 };
    HANDLE time_zone_key, dynamic_dst_key;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR yearW[16];
    BOOL got_reg_data = FALSE;
    struct tz_reg_data
    {
        LONG bias;
        LONG std_bias;
        LONG dlt_bias;
        SYSTEMTIME std_date;
        SYSTEMTIME dlt_date;
    } tz_data;

    if (!TIME_GetSpecificTimeZoneKey( key_name, &time_zone_key ))
        return FALSE;

    if (RegLoadMUIStringW( time_zone_key, mui_stdW, tzinfo->StandardName, sizeof(tzinfo->StandardName), NULL, 0, DIR_System ) &&
        !reg_query_value( time_zone_key, stdW, REG_SZ, tzinfo->StandardName, sizeof(tzinfo->StandardName) ))
    {
        NtClose( time_zone_key );
        return FALSE;
    }

    if (RegLoadMUIStringW( time_zone_key, mui_dltW, tzinfo->DaylightName, sizeof(tzinfo->DaylightName), NULL, 0, DIR_System ) &&
        !reg_query_value( time_zone_key, dltW, REG_SZ, tzinfo->DaylightName, sizeof(tzinfo->DaylightName) ))
    {
        NtClose( time_zone_key );
        return FALSE;
    }

    lstrcpyW(tzinfo->TimeZoneKeyName, key_name);

    if (dynamic)
    {
        attr.Length = sizeof(attr);
        attr.RootDirectory = time_zone_key;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &nameW, Dynamic_DstW );
        if (!NtOpenKey( &dynamic_dst_key, KEY_READ, &attr ))
        {
            sprintfW( yearW, fmtW, year );
            got_reg_data = reg_query_value( dynamic_dst_key, yearW, REG_BINARY, &tz_data, sizeof(tz_data) );

            NtClose( dynamic_dst_key );
        }
    }

    if (!got_reg_data)
    {
        if (!reg_query_value( time_zone_key, tziW, REG_BINARY, &tz_data, sizeof(tz_data) ))
        {
            NtClose( time_zone_key );
            return FALSE;
        }
    }

    tzinfo->Bias = tz_data.bias;
    tzinfo->StandardBias = tz_data.std_bias;
    tzinfo->DaylightBias = tz_data.dlt_bias;
    tzinfo->StandardDate = tz_data.std_date;
    tzinfo->DaylightDate = tz_data.dlt_date;

    tzinfo->DynamicDaylightTimeDisabled = !dynamic;

    NtClose( time_zone_key );

    return TRUE;
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
DWORD WINAPI GetTimeZoneInformation( LPTIME_ZONE_INFORMATION ret )
{
    DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
    DWORD time_zone_id;

    TRACE("(%p)\n", ret);
    time_zone_id = GetDynamicTimeZoneInformation( &tzinfo );
    memcpy( ret, &tzinfo, sizeof(*ret) );
    return time_zone_id;
}

/***********************************************************************
 *              GetTimeZoneInformationForYear  (KERNEL32.@)
 */
BOOL WINAPI GetTimeZoneInformationForYear( USHORT wYear,
    PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi )
{
    DYNAMIC_TIME_ZONE_INFORMATION local_dtzi, result;

    TRACE("(%u,%p)\n", wYear, ptzi);
    if (!pdtzi)
    {
        if (GetDynamicTimeZoneInformation(&local_dtzi) == TIME_ZONE_ID_INVALID)
            return FALSE;
        pdtzi = &local_dtzi;
    }

    if (!TIME_GetSpecificTimeZoneInfo(pdtzi->TimeZoneKeyName, wYear,
            !pdtzi->DynamicDaylightTimeDisabled, &result))
        return FALSE;

    memcpy(ptzi, &result, sizeof(*ptzi));

    return TRUE;
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
    TRACE("(%p)\n", tzinfo);
    return set_ntstatus( RtlSetTimeZoneInformation( (const RTL_TIME_ZONE_INFORMATION *)tzinfo ));
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
        RtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    }

    if (!SystemTimeToFileTime(lpUniversalTime, &ft))
        return FALSE;
    llTime = filetime_to_longlong( &ft );
    if (!TIME_GetTimezoneBias(&tzinfo, llTime, FALSE, &lBias))
        return FALSE;
    /* convert minutes to 100-nanoseconds-ticks */
    llTime -= (LONGLONG)lBias * 600000000;
    longlong_to_filetime( llTime, &ft );

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
        RtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    }

    if (!SystemTimeToFileTime(lpLocalTime, &ft))
        return FALSE;
    t = filetime_to_longlong( &ft );
    if (!TIME_GetTimezoneBias(&tzinfo, t, TRUE, &lBias))
        return FALSE;
    /* convert minutes to 100-nanoseconds-ticks */
    t += (LONGLONG)lBias * 600000000;
    longlong_to_filetime( t, &ft );
    return FileTimeToSystemTime(&ft, lpUniversalTime);
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

/***********************************************************************
 *		TIMEZONE_InitRegistry
 *
 * Update registry contents on startup if the user timezone has changed.
 * This simulates the action of the Windows control panel.
 */
void TIMEZONE_InitRegistry(void)
{
    static const WCHAR timezoneInformationW[] = {
        'M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'T','i','m','e','Z','o','n','e','I','n','f','o','r','m','a','t','i','o','n','\0'
    };
    static const WCHAR standardNameW[] = {'S','t','a','n','d','a','r','d','N','a','m','e','\0'};
    static const WCHAR timezoneKeyNameW[] = {'T','i','m','e','Z','o','n','e','K','e','y','N','a','m','e','\0'};
    DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES attr;
    HANDLE hkey;
    DWORD tzid;

    tzid = GetDynamicTimeZoneInformation(&tzinfo);
    if (tzid == TIME_ZONE_ID_INVALID) return;

    RtlInitUnicodeString(&name, timezoneInformationW);
    InitializeObjectAttributes(&attr, &name, 0, 0, NULL);
    if (NtCreateKey(&hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL) != STATUS_SUCCESS) return;

    RtlInitUnicodeString(&name, standardNameW);
    NtSetValueKey(hkey, &name, 0, REG_SZ, tzinfo.StandardName,
                  (strlenW(tzinfo.StandardName) + 1) * sizeof(WCHAR));

    RtlInitUnicodeString(&name, timezoneKeyNameW);
    NtSetValueKey(hkey, &name, 0, REG_SZ, tzinfo.TimeZoneKeyName,
                  (strlenW(tzinfo.TimeZoneKeyName) + 1) * sizeof(WCHAR));

    NtClose( hkey );
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
    longlong_to_filetime( pti.CreateTime.QuadPart, lpCreationTime );
    longlong_to_filetime( pti.ExitTime.QuadPart, lpExitTime );
    return TRUE;
}

/*********************************************************************
 *	GetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI GetCalendarInfoA(LCID lcid, CALID Calendar, CALTYPE CalType,
			    LPSTR lpCalData, int cchData, LPDWORD lpValue)
{
    int ret, cchDataW = cchData;
    LPWSTR lpCalDataW = NULL;

    if (NLS_IsUnicodeOnlyLcid(lcid))
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    if (!cchData && !(CalType & CAL_RETURN_NUMBER))
        cchDataW = GetCalendarInfoW(lcid, Calendar, CalType, NULL, 0, NULL);
    if (!(lpCalDataW = HeapAlloc(GetProcessHeap(), 0, cchDataW*sizeof(WCHAR))))
        return 0;

    ret = GetCalendarInfoW(lcid, Calendar, CalType, lpCalDataW, cchDataW, lpValue);
    if(ret && lpCalDataW && lpCalData)
        ret = WideCharToMultiByte(CP_ACP, 0, lpCalDataW, -1, lpCalData, cchData, NULL, NULL);
    else if (CalType & CAL_RETURN_NUMBER)
        ret *= sizeof(WCHAR);
    HeapFree(GetProcessHeap(), 0, lpCalDataW);

    return ret;
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

    TRACE("()\n");
    RtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    return (TIME_ZoneID(&tzinfo) == TIME_ZONE_ID_DAYLIGHT);
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
    *fattime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
    *fatdate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5) + tm->tm_mday;
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
    LARGE_INTEGER idle_time, kernel_time, user_time;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *sppi;
    SYSTEM_BASIC_INFORMATION sbi;
    ULONG ret_size;
    int i;

    TRACE("(%p,%p,%p)\n", lpIdleTime, lpKernelTime, lpUserTime);

    if (!set_ntstatus( NtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), &ret_size )))
        return FALSE;

    sppi = HeapAlloc( GetProcessHeap(), 0,
                      sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * sbi.NumberOfProcessors);
    if (!sppi)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    if (!set_ntstatus( NtQuerySystemInformation( SystemProcessorPerformanceInformation, sppi,
                                                 sizeof(*sppi) * sbi.NumberOfProcessors, &ret_size )))
    {
        HeapFree( GetProcessHeap(), 0, sppi );
        return FALSE;
    }

    idle_time.QuadPart = 0;
    kernel_time.QuadPart = 0;
    user_time.QuadPart = 0;
    for (i = 0; i < sbi.NumberOfProcessors; i++)
    {
        idle_time.QuadPart += sppi[i].IdleTime.QuadPart;
        kernel_time.QuadPart += sppi[i].KernelTime.QuadPart;
        user_time.QuadPart += sppi[i].UserTime.QuadPart;
    }

    if (lpIdleTime)
    {
        lpIdleTime->dwLowDateTime = idle_time.u.LowPart;
        lpIdleTime->dwHighDateTime = idle_time.u.HighPart;
    }
    if (lpKernelTime)
    {
        lpKernelTime->dwLowDateTime = kernel_time.u.LowPart;
        lpKernelTime->dwHighDateTime = kernel_time.u.HighPart;
    }
    if (lpUserTime)
    {
        lpUserTime->dwLowDateTime = user_time.u.LowPart;
        lpUserTime->dwHighDateTime = user_time.u.HighPart;
    }

    HeapFree( GetProcessHeap(), 0, sppi );
    return TRUE;
}

/***********************************************************************
 *           GetDynamicTimeZoneInformation   (KERNEL32.@)
 */
DWORD WINAPI GetDynamicTimeZoneInformation(DYNAMIC_TIME_ZONE_INFORMATION *tzinfo)
{
    HANDLE time_zone_key;

    TRACE("(%p)\n", tzinfo);
    if (!set_ntstatus( RtlQueryDynamicTimeZoneInformation( (RTL_DYNAMIC_TIME_ZONE_INFORMATION*)tzinfo )))
        return TIME_ZONE_ID_INVALID;

    if (!TIME_GetSpecificTimeZoneKey( tzinfo->TimeZoneKeyName, &time_zone_key ))
        return TIME_ZONE_ID_INVALID;
    RegLoadMUIStringW( time_zone_key, mui_stdW, tzinfo->StandardName, sizeof(tzinfo->StandardName), NULL, 0, DIR_System );
    RegLoadMUIStringW( time_zone_key, mui_dltW, tzinfo->DaylightName, sizeof(tzinfo->DaylightName), NULL, 0, DIR_System );
    NtClose( time_zone_key );

    return TIME_ZoneID( (TIME_ZONE_INFORMATION*)tzinfo );
}

/***********************************************************************
 *           GetDynamicTimeZoneInformationEffectiveYears   (KERNEL32.@)
 */
DWORD WINAPI GetDynamicTimeZoneInformationEffectiveYears(DYNAMIC_TIME_ZONE_INFORMATION *tzinfo, DWORD *first_year, DWORD *last_year)
{
    FIXME("(%p, %p, %p): stub!\n", tzinfo, first_year, last_year);
    return ERROR_FILE_NOT_FOUND;
}

/***********************************************************************
 *           QueryProcessCycleTime   (KERNEL32.@)
 */
BOOL WINAPI QueryProcessCycleTime(HANDLE process, PULONG64 cycle)
{
    static int once;
    if (!once++)
        FIXME("(%p,%p): stub!\n", process, cycle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           QueryThreadCycleTime   (KERNEL32.@)
 */
BOOL WINAPI QueryThreadCycleTime(HANDLE thread, PULONG64 cycle)
{
    static int once;
    if (!once++)
        FIXME("(%p,%p): stub!\n", thread, cycle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *           GetTickCount64       (KERNEL32.@)
 */
ULONGLONG WINAPI DECLSPEC_HOTPATCH GetTickCount64(void)
{
    return monotonic_counter() / TICKSPERMSEC;
}

/***********************************************************************
 *           GetTickCount       (KERNEL32.@)
 *
 * Get the number of milliseconds the system has been running.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current tick count.
 *
 * NOTES
 *  The value returned will wrap around every 2^32 milliseconds.
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTickCount(void)
{
    return monotonic_counter() / TICKSPERMSEC;
}
