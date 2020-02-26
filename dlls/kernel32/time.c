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
