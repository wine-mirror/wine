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
 *  Sets the local time using current time zone and daylight
 *  savings settings.
 *
 * RETURNS
 *
 *  True if the time was set, false if the time was invalid or the
 *  necessary permissions were not held.
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
 *  Sets the system time (utc).
 *
 * RETURNS
 *
 *  True if the time was set, false if the time was invalid or the
 *  necessary permissions were not held.
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
 *              GetTimeZoneInformation  (KERNEL32.@)
 *
 *  Fills in the a time zone information structure with values based on
 *  the current local time.
 *
 * RETURNS
 *
 *  The daylight savings time standard or TIME_ZONE_ID_INVALID if the call failed.
 */
DWORD WINAPI GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION tzinfo) /* [out] The time zone structure to be filled in. */
{
    NTSTATUS status;
    if ((status = RtlQueryTimeZoneInformation(tzinfo)))
        SetLastError( RtlNtStatusToDosError(status) );
    return TIME_ZONE_ID_STANDARD;
}


/***********************************************************************
 *              SetTimeZoneInformation  (KERNEL32.@)
 *
 *  Set the local time zone with values based on the time zone structure.
 *
 * RETURNS
 *
 *  True on successful setting of the time zone.
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
 *              SystemTimeToTzSpecificLocalTime  (KERNEL32.@)
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
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation, /* [in] The desired time zone. */
    LPSYSTEMTIME            lpUniversalTime,       /* [in] The utc time to base local time on. */
    LPSYSTEMTIME            lpLocalTime)           /* [out] The local time in the time zone. */
{
  FIXME(":stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}


/***********************************************************************
 *              GetSystemTimeAsFileTime  (KERNEL32.@)
 *
 *  Fills in a file time structure with the current time in UTC format.
 */
VOID WINAPI GetSystemTimeAsFileTime(
    LPFILETIME time) /* [out] The file time struct to be filled with the system time. */
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
 *             no need in offset correction, like in UnixTimeToFileTime
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
 */
VOID WINAPI GetLocalTime(LPSYSTEMTIME systime)
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
 */
VOID WINAPI GetSystemTime(LPSYSTEMTIME systime)
{
    FILETIME ft;
    LARGE_INTEGER t;

    NtQuerySystemTime(&t);
    ft.dwLowDateTime = t.s.LowPart;
    ft.dwHighDateTime = t.s.HighPart;
    FileTimeToSystemTime(&ft, systime);
}
