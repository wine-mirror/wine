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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "file.h"
#include "winternl.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);

/* maximum time adjustment in seconds for SetLocalTime and SetSystemTime */
#define SETTIME_MAX_ADJUST 120
#define CALINFO_MAX_YEAR 2029


/***********************************************************************
 *              SetLocalTime            (KERNEL32.@)
 *
 *  Set the local time using current time zone and daylight
 *  savings settings.
 *
 * RETURNS
 *  Success: TRUE. The time was set
 *  Failure: FALSE, if the time was invalid or caller does not have
 *           permission to change the time.
 */
BOOL WINAPI SetLocalTime(
    const SYSTEMTIME *systime) /* [in] The desired local time. */
{
    FILETIME ft;
    LARGE_INTEGER st, st2;
    NTSTATUS status;

    SystemTimeToFileTime( systime, &ft );
    st.s.LowPart = ft.dwLowDateTime;
    st.s.HighPart = ft.dwHighDateTime;
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
 * RETURNS
 *  TRUE.
 *
 * BUGS
 *  Only the special case of disabled time adjustments is supported.
 */
BOOL WINAPI GetSystemTimeAdjustment(
    PDWORD lpTimeAdjustment,         /* [out] The clock adjustment per interupt in 100's of nanoseconds. */
    PDWORD lpTimeIncrement,          /* [out] The time between clock interupts in 100's of nanoseconds. */
    PBOOL  lpTimeAdjustmentDisabled) /* [out] The clock synchonisation has been disabled. */
{
    *lpTimeAdjustment = 0;
    *lpTimeIncrement = 0;
    *lpTimeAdjustmentDisabled = TRUE;
    return TRUE;
}


/***********************************************************************
 *              SetSystemTime            (KERNEL32.@)
 *
 *  Set the system time in utc.
 *
 * RETURNS
 *  Success: TRUE. The time was set
 *  Failure: FALSE, if the time was invalid or caller does not have
 *           permission to change the time.
 */
BOOL WINAPI SetSystemTime(
    const SYSTEMTIME *systime) /* [in] The desired system time. */
{
    FILETIME ft;
    LARGE_INTEGER t;
    NTSTATUS status;

    SystemTimeToFileTime( systime, &ft );
    t.s.LowPart = ft.dwLowDateTime;
    t.s.HighPart = ft.dwHighDateTime;
    if ((status = NtSetSystemTime(&t, NULL)))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *              SetSystemTimeAdjustment  (KERNEL32.@)
 *
 *  Enables or disables the timing adjustments to the system's clock.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI SetSystemTimeAdjustment(
    DWORD dwTimeAdjustment,
    BOOL bTimeAdjustmentDisabled)
{
    /* Fake function for now... */
    FIXME("(%08lx,%d): stub !\n", dwTimeAdjustment, bTimeAdjustmentDisabled);
    return TRUE;
}


/***********************************************************************
 *              GetTimeZoneInformation  (KERNEL32.@)
 *
 *  Get information about the current local time zone.
 *
 * RETURNS
 *  Success: TIME_ZONE_ID_STANDARD. tzinfo contains the time zone info.
 *  Failure: TIME_ZONE_ID_INVALID.
 */
DWORD WINAPI GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION tzinfo) /* [out] Destination for time zone information */
{
    NTSTATUS status;
    if ((status = RtlQueryTimeZoneInformation(tzinfo)))
        SetLastError( RtlNtStatusToDosError(status) );
    return TIME_ZONE_ID_STANDARD;
}


/***********************************************************************
 *              SetTimeZoneInformation  (KERNEL32.@)
 *
 *  Change the settings of the current local time zone.
 *
 * RETURNS
 *  Success: TRUE. The time zone was updated with the settings from tzinfo
 *  Failure: FALSE.
 */
BOOL WINAPI SetTimeZoneInformation(
    const LPTIME_ZONE_INFORMATION tzinfo) /* [in] The new time zone. */
{
    NTSTATUS status;
    if ((status = RtlSetTimeZoneInformation(tzinfo)))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *  _DayLightCompareDate
 *
 *  Compares two dates without looking at the year
 *
 * RETURNS
 *
 *  -1 if date < compareDate
 *   0 if date == compareDate
 *   1 if date > compareDate
 *  -2 if an error occures
 */

static int _DayLightCompareDate(
    const LPSYSTEMTIME date,                       /* [in] The date to compare. */
    const LPSYSTEMTIME compareDate)                /* [in] The daylight saving begin or end date */
{
    int limit_day;

    if (compareDate->wYear != 0)
    {
        if (date->wMonth < compareDate->wMonth)
            return -1; /* We are in a year before the date limit. */

        if (date->wMonth > compareDate->wMonth)
            return 1; /* We are in a year after the date limit. */
    }

    if (date->wMonth < compareDate->wMonth)
        return -1; /* We are in a month before the date limit. */

    if (date->wMonth > compareDate->wMonth)
        return 1; /* We are in a month after the date limit. */

    if (compareDate->wDayOfWeek <= 6)
    {
       SYSTEMTIME tmp;
       FILETIME tmp_ft;

       /* compareDate->wDay is interpreted as number of the week in the month. */
       /* 5 means: the last week in the month */
       int weekofmonth = compareDate->wDay;

         /* calculate day of week for the first day in the month */
        memcpy(&tmp, date, sizeof(SYSTEMTIME));
        tmp.wDay = 1;
        tmp.wDayOfWeek = -1;

        if (weekofmonth == 5)
        {
             /* Go to the beginning of the next month. */
            if (++tmp.wMonth > 12)
            {
                tmp.wMonth = 1;
                ++tmp.wYear;
            }
        }

        if (!SystemTimeToFileTime(&tmp, &tmp_ft))
            return -2;

        if (weekofmonth == 5)
        {
          LONGLONG t, one_day;

          t = tmp_ft.dwHighDateTime;
          t <<= 32;
          t += (UINT)tmp_ft.dwLowDateTime;

          /* subtract one day */
          one_day = 24*60*60;
          one_day *= 10000000;
          t -= one_day;

          tmp_ft.dwLowDateTime  = (UINT)t;
          tmp_ft.dwHighDateTime = (UINT)(t >> 32);
        }

        if (!FileTimeToSystemTime(&tmp_ft, &tmp))
            return -2;

       if (weekofmonth == 5)
       {
          /* calculate the last matching day of the week in this month */
          int dif = tmp.wDayOfWeek - compareDate->wDayOfWeek;
          if (dif < 0)
             dif += 7;

          limit_day = tmp.wDay - dif;
       }
       else
       {
          /* calculate the matching day of the week in the given week */
          int dif = compareDate->wDayOfWeek - tmp.wDayOfWeek;
          if (dif < 0)
             dif += 7;

          limit_day = tmp.wDay + 7*(weekofmonth-1) + dif;
       }
    }
    else
    {
       limit_day = compareDate->wDay;
    }

    if (date->wDay < limit_day)
        return -1;

    if (date->wDay > limit_day)
        return 1;

    return 0;   /* date is equal to the date limit. */
}


/***********************************************************************
 *  _GetTimezoneBias
 *
 *  Calculates the local time bias for a given time zone
 *
 * RETURNS
 *
 *  Returns TRUE when the time zone bias was calculated.
 */

static BOOL _GetTimezoneBias(
    const LPTIME_ZONE_INFORMATION lpTimeZoneInformation, /* [in] The time zone data.            */
    LPSYSTEMTIME                  lpSystemTime,          /* [in] The system time.               */
    LONG*                         pBias)                 /* [out] The calulated bias in minutes */
{
    int ret;
    BOOL beforeStandardDate, afterDaylightDate;
    BOOL daylightsaving = FALSE;
    LONG bias = lpTimeZoneInformation->Bias;

    if (lpTimeZoneInformation->DaylightDate.wMonth != 0)
    {
        if (lpTimeZoneInformation->StandardDate.wMonth == 0 ||
            lpTimeZoneInformation->StandardDate.wDay<1 ||
            lpTimeZoneInformation->StandardDate.wDay>5 ||
            lpTimeZoneInformation->DaylightDate.wDay<1 ||
            lpTimeZoneInformation->DaylightDate.wDay>5)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

         /* check for daylight saving */
        ret = _DayLightCompareDate(lpSystemTime, &lpTimeZoneInformation->StandardDate);
        if (ret == -2)
          return FALSE;

        beforeStandardDate = ret < 0;

        ret = _DayLightCompareDate(lpSystemTime, &lpTimeZoneInformation->DaylightDate);
        if (ret == -2)
          return FALSE;

        afterDaylightDate = ret >= 0;

        if (beforeStandardDate && afterDaylightDate)
            daylightsaving = TRUE;
    }

    if (daylightsaving)
        bias += lpTimeZoneInformation->DaylightBias;
    else if (lpTimeZoneInformation->StandardDate.wMonth != 0)
        bias += lpTimeZoneInformation->StandardBias;

    *pBias = bias;

    return TRUE;
}


/***********************************************************************
 *              SystemTimeToTzSpecificLocalTime  (KERNEL32.@)
 *
 *  Convert a utc system time to a local time in a given time zone.
 *
 * RETURNS
 *  Success: TRUE. lpLocalTime contains the converted time
 *  Failure: FALSE.
 */

BOOL WINAPI SystemTimeToTzSpecificLocalTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation, /* [in] The desired time zone. */
    LPSYSTEMTIME            lpUniversalTime,       /* [in] The utc time to base local time on. */
    LPSYSTEMTIME            lpLocalTime)           /* [out] The local time in the time zone. */
{
    FILETIME ft;
    LONG lBias;
    LONGLONG t, bias;
    TIME_ZONE_INFORMATION tzinfo;

    if (lpTimeZoneInformation != NULL)
    {
        memcpy(&tzinfo, lpTimeZoneInformation, sizeof(TIME_ZONE_INFORMATION));
    }
    else
    {
        if (GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID)
            return FALSE;
    }

    if (!SystemTimeToFileTime(lpUniversalTime, &ft))
        return FALSE;

    t = ft.dwHighDateTime;
    t <<= 32;
    t += (UINT)ft.dwLowDateTime;

    if (!_GetTimezoneBias(&tzinfo, lpUniversalTime, &lBias))
        return FALSE;

    bias = (LONGLONG)lBias * 600000000; /* 60 seconds per minute, 100000 [100-nanoseconds-ticks] per second */
    t -= bias;

    ft.dwLowDateTime  = (UINT)t;
    ft.dwHighDateTime = (UINT)(t >> 32);

    return FileTimeToSystemTime(&ft, lpLocalTime);
}


/***********************************************************************
 *              TzSpecificLocalTimeToSystemTime  (KERNEL32.@)
 *
 *  Converts a local time to a time in utc.
 *
 * RETURNS
 *  Success: TRUE. lpUniversalTime contains the converted time
 *  Failure: FALSE.
 */
BOOL WINAPI TzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation, /* [in] The desired time zone. */
    LPSYSTEMTIME            lpLocalTime,           /* [in] The local time. */
    LPSYSTEMTIME            lpUniversalTime)       /* [out] The calculated utc time. */
{
    FILETIME ft;
    LONG lBias;
    LONGLONG t, bias;
    TIME_ZONE_INFORMATION tzinfo;

    if (lpTimeZoneInformation != NULL)
    {
        memcpy(&tzinfo, lpTimeZoneInformation, sizeof(TIME_ZONE_INFORMATION));
    }
    else
    {
        if (GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID)
            return FALSE;
    }

    if (!SystemTimeToFileTime(lpLocalTime, &ft))
        return FALSE;

    t = ft.dwHighDateTime;
    t <<= 32;
    t += (UINT)ft.dwLowDateTime;

    if (!_GetTimezoneBias(&tzinfo, lpLocalTime, &lBias))
        return FALSE;

    bias = (LONGLONG)lBias * 600000000; /* 60 seconds per minute, 100000 [100-nanoseconds-ticks] per second */
    t += bias;

    ft.dwLowDateTime  = (UINT)t;
    ft.dwHighDateTime = (UINT)(t >> 32);

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
    time->dwLowDateTime = t.s.LowPart;
    time->dwHighDateTime = t.s.HighPart;
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
    ULONGLONG secs = RtlEnlargedUnsignedMultiply( unix_time, 10000000 );
    secs = RtlExtendedLargeIntegerDivide( secs, CLK_TCK, NULL );
    filetime->dwLowDateTime  = (DWORD)secs;
    filetime->dwHighDateTime = (DWORD)(secs >> 32);
}

/*********************************************************************
 *	GetProcessTimes				(KERNEL32.@)
 *
 *  Get the user and kernel execution times of a process,
 *  along with the creation and exit times if known.
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
 *  lpCreationTime and lpExitTime are not initialised in the Wine implementation.
 */
BOOL WINAPI GetProcessTimes(
    HANDLE     hprocess,       /* [in] The process to be queried (obtained from PROCESS_QUERY_INFORMATION). */
    LPFILETIME lpCreationTime, /* [out] The creation time of the process. */
    LPFILETIME lpExitTime,     /* [out] The exit time of the process if exited. */
    LPFILETIME lpKernelTime,   /* [out] The time spent in kernal routines in 100's of nanoseconds. */
    LPFILETIME lpUserTime)     /* [out] The time spent in user routines in 100's of nanoseconds. */
{
    struct tms tms;

    times(&tms);
    TIME_ClockTimeToFileTime(tms.tms_utime,lpUserTime);
    TIME_ClockTimeToFileTime(tms.tms_stime,lpKernelTime);
    return TRUE;
}

/*********************************************************************
 *	GetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI GetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType,
			    LPSTR lpCalData, int cchData, LPDWORD lpValue)
{
    int ret;
    LPWSTR lpCalDataW = NULL;

    FIXME("(%08lx,%08lx,%08lx,%p,%d,%p): quarter-stub\n",
	  Locale, Calendar, CalType, lpCalData, cchData, lpValue);
    /* FIXME: Should verify if Locale is allowable in ANSI, as per MSDN */

    if(cchData)
      if(!(lpCalDataW = HeapAlloc(GetProcessHeap(), 0, cchData*sizeof(WCHAR)))) return 0;

    ret = GetCalendarInfoW(Locale, Calendar, CalType, lpCalDataW, cchData, lpValue);
    if(ret && lpCalDataW && lpCalData)
      WideCharToMultiByte(CP_ACP, 0, lpCalDataW, cchData, lpCalData, cchData, NULL, NULL);
    if(lpCalDataW)
      HeapFree(GetProcessHeap(), 0, lpCalDataW);

    return ret;
}

/*********************************************************************
 *	GetCalendarInfoW				(KERNEL32.@)
 *
 * See GetCalendarInfoA.
 */
int WINAPI GetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType,
			    LPWSTR lpCalData, int cchData, LPDWORD lpValue)
{
    FIXME("(%08lx,%08lx,%08lx,%p,%d,%p): quarter-stub\n",
	  Locale, Calendar, CalType, lpCalData, cchData, lpValue);

    if (CalType & CAL_NOUSEROVERRIDE)
	FIXME("flag CAL_NOUSEROVERRIDE used, not fully implemented\n");
    if (CalType & CAL_USE_CP_ACP)
	FIXME("flag CAL_USE_CP_ACP used, not fully implemented\n");

    if (CalType & CAL_RETURN_NUMBER) {
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
	    FIXME("Unimplemented caltype %ld\n", CalType & 0xffff);
	    return E_FAIL;
	case CAL_SCALNAME:
	    FIXME("Unimplemented caltype %ld\n", CalType & 0xffff);
	    return E_FAIL;
	case CAL_IYEAROFFSETRANGE:
	    FIXME("Unimplemented caltype %ld\n", CalType & 0xffff);
	    return E_FAIL;
	case CAL_SERASTRING:
	    FIXME("Unimplemented caltype %ld\n", CalType & 0xffff);
	    return E_FAIL;
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
	    if (lpValue) *lpValue = CALINFO_MAX_YEAR;
	    break;
	default: MESSAGE("Unknown caltype %ld\n",CalType & 0xffff);
		 return E_FAIL;
    }
    return 0;
}

/*********************************************************************
 *	SetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI	SetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPCSTR lpCalData)
{
    FIXME("(%08lx,%08lx,%08lx,%s): stub\n",
	  Locale, Calendar, CalType, debugstr_a(lpCalData));
    return 0;
}

/*********************************************************************
 *	SetCalendarInfoW				(KERNEL32.@)
 *
 * See SetCalendarInfoA.
 */
int WINAPI	SetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType, LPCWSTR lpCalData)
{
    FIXME("(%08lx,%08lx,%08lx,%s): stub\n",
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

    local.s.LowPart = localft->dwLowDateTime;
    local.s.HighPart = localft->dwHighDateTime;
    if (!(status = RtlLocalTimeToSystemTime( &local, &utc )))
    {
        utcft->dwLowDateTime = utc.s.LowPart;
        utcft->dwHighDateTime = utc.s.HighPart;
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

    utc.s.LowPart = utcft->dwLowDateTime;
    utc.s.HighPart = utcft->dwHighDateTime;
    if (!(status = RtlSystemTimeToLocalTime( &utc, &local )))
    {
        localft->dwLowDateTime = local.s.LowPart;
        localft->dwHighDateTime = local.s.HighPart;
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

    t.s.LowPart = ft->dwLowDateTime;
    t.s.HighPart = ft->dwHighDateTime;
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

    RtlTimeFieldsToTime(&tf, &t);
    ft->dwLowDateTime = t.s.LowPart;
    ft->dwHighDateTime = t.s.HighPart;
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
 * RETURNS
 *  Nothing.
 */
VOID WINAPI GetLocalTime(LPSYSTEMTIME systime) /* [O] Destination for current time */
{
    FILETIME lft;
    LARGE_INTEGER ft, ft2;

    NtQuerySystemTime(&ft);
    RtlSystemTimeToLocalTime(&ft, &ft2);
    lft.dwLowDateTime = ft2.s.LowPart;
    lft.dwHighDateTime = ft2.s.HighPart;
    FileTimeToSystemTime(&lft, systime);
}

/*********************************************************************
 *      GetSystemTime                                   (KERNEL32.@)
 *
 * Get the current system time.
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI GetSystemTime(LPSYSTEMTIME systime) /* [O] Destination for current time */
{
    FILETIME ft;
    LARGE_INTEGER t;

    NtQuerySystemTime(&t);
    ft.dwLowDateTime = t.s.LowPart;
    ft.dwHighDateTime = t.s.HighPart;
    FileTimeToSystemTime(&ft, systime);
}
