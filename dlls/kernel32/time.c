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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "kernel_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(time);

static const struct _KUSER_SHARED_DATA *user_shared_data = (struct _KUSER_SHARED_DATA *)0x7ffe0000;


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

/******************************************************************************
 *           GetTickCount64       (KERNEL32.@)
 */
ULONGLONG WINAPI DECLSPEC_HOTPATCH GetTickCount64(void)
{
    ULONG high, low;

    do
    {
        high = user_shared_data->u.TickCount.High1Time;
        low = user_shared_data->u.TickCount.LowPart;
    }
    while (high != user_shared_data->u.TickCount.High2Time);
    /* note: we ignore TickCountMultiplier */
    return (ULONGLONG)high << 32 | low;
}

/***********************************************************************
 *           GetTickCount       (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTickCount(void)
{
    /* note: we ignore TickCountMultiplier */
    return user_shared_data->u.TickCount.LowPart;
}
