/*
 * msvcrt.dll date/time functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2004 Hans Leidekker
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

#define _POSIX_PTHREAD_SEMANTICS /* switch to a 2 arg style asctime_r on Solaris */
#include <time.h>
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif
#include <limits.h>

#include "msvcrt.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static const int MonthLengths[2][12] =
{
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline int IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0);
}

static inline void msvcrt_tm_to_unix( struct tm *dest, const struct MSVCRT_tm *src )
{
    memset( dest, 0, sizeof(*dest) );
    dest->tm_sec   = src->tm_sec;
    dest->tm_min   = src->tm_min;
    dest->tm_hour  = src->tm_hour;
    dest->tm_mday  = src->tm_mday;
    dest->tm_mon   = src->tm_mon;
    dest->tm_year  = src->tm_year;
    dest->tm_wday  = src->tm_wday;
    dest->tm_yday  = src->tm_yday;
    dest->tm_isdst = src->tm_isdst;
}

static inline void unix_tm_to_msvcrt( struct MSVCRT_tm *dest, const struct tm *src )
{
    memset( dest, 0, sizeof(*dest) );
    dest->tm_sec   = src->tm_sec;
    dest->tm_min   = src->tm_min;
    dest->tm_hour  = src->tm_hour;
    dest->tm_mday  = src->tm_mday;
    dest->tm_mon   = src->tm_mon;
    dest->tm_year  = src->tm_year;
    dest->tm_wday  = src->tm_wday;
    dest->tm_yday  = src->tm_yday;
    dest->tm_isdst = src->tm_isdst;
}

#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000
#define TICKSPERMSEC      10000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)

/**********************************************************************
 *		mktime (MSVCRT.@)
 */
MSVCRT_time_t CDECL MSVCRT_mktime(struct MSVCRT_tm *mstm)
{
    time_t secs;
    struct tm tm;

    msvcrt_tm_to_unix( &tm, mstm );
    secs = mktime( &tm );
    unix_tm_to_msvcrt( mstm, &tm );

    return secs < 0 ? -1 : secs;
}

/*********************************************************************
 *      localtime (MSVCRT.@)
 */
struct MSVCRT_tm* CDECL MSVCRT_localtime(const MSVCRT_time_t* secs)
{
    struct tm tm;
    thread_data_t *data = msvcrt_get_thread_data();
    time_t seconds = *secs;

    localtime_r( &seconds, &tm );
    unix_tm_to_msvcrt( &data->time_buffer, &tm );

    return &data->time_buffer;
}

/*********************************************************************
 *      gmtime (MSVCRT.@)
 */
struct MSVCRT_tm* CDECL MSVCRT_gmtime(const MSVCRT_time_t* secs)
{
  thread_data_t * const data = msvcrt_get_thread_data();
  int i;
  FILETIME ft;
  SYSTEMTIME st;

  ULONGLONG time = *secs * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;

  ft.dwHighDateTime = (UINT)(time >> 32);
  ft.dwLowDateTime  = (UINT)time;

  FileTimeToSystemTime(&ft, &st);

  if (st.wYear < 1970) return NULL;

  data->time_buffer.tm_sec  = st.wSecond;
  data->time_buffer.tm_min  = st.wMinute;
  data->time_buffer.tm_hour = st.wHour;
  data->time_buffer.tm_mday = st.wDay;
  data->time_buffer.tm_year = st.wYear - 1900;
  data->time_buffer.tm_mon  = st.wMonth - 1;
  data->time_buffer.tm_wday = st.wDayOfWeek;
  for (i = data->time_buffer.tm_yday = 0; i < st.wMonth - 1; i++) {
    data->time_buffer.tm_yday += MonthLengths[IsLeapYear(st.wYear)][i];
  }

  data->time_buffer.tm_yday += st.wDay - 1;
  data->time_buffer.tm_isdst = 0;

  return &data->time_buffer;
}

/**********************************************************************
 *		_strdate (MSVCRT.@)
 */
char* CDECL _strdate(char* date)
{
  static const char format[] = "MM'/'dd'/'yy";

  GetDateFormatA(LOCALE_NEUTRAL, 0, NULL, format, date, 9);

  return date;
}

/**********************************************************************
 *		_wstrdate (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL _wstrdate(MSVCRT_wchar_t* date)
{
  static const WCHAR format[] = { 'M','M','\'','/','\'','d','d','\'','/','\'','y','y',0 };

  GetDateFormatW(LOCALE_NEUTRAL, 0, NULL, format, (LPWSTR)date, 9);

  return date;
}

/*********************************************************************
 *		_strtime (MSVCRT.@)
 */
char* CDECL _strtime(char* time)
{
  static const char format[] = "HH':'mm':'ss";

  GetTimeFormatA(LOCALE_NEUTRAL, 0, NULL, format, time, 9); 

  return time;
}

/*********************************************************************
 *		_wstrtime (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL _wstrtime(MSVCRT_wchar_t* time)
{
  static const WCHAR format[] = { 'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0 };

  GetTimeFormatW(LOCALE_NEUTRAL, 0, NULL, format, (LPWSTR)time, 9);

  return time;
}

/*********************************************************************
 *		clock (MSVCRT.@)
 */
MSVCRT_clock_t CDECL MSVCRT_clock(void)
{
  FILETIME ftc, fte, ftk, ftu;
  ULONGLONG utime, ktime;
 
  MSVCRT_clock_t clock;

  GetProcessTimes(GetCurrentProcess(), &ftc, &fte, &ftk, &ftu);

  ktime = ((ULONGLONG)ftk.dwHighDateTime << 32) | ftk.dwLowDateTime;
  utime = ((ULONGLONG)ftu.dwHighDateTime << 32) | ftu.dwLowDateTime;

  clock = (utime + ktime) / (TICKSPERSEC / MSVCRT_CLOCKS_PER_SEC);

  return clock;
}

/*********************************************************************
 *		difftime (MSVCRT.@)
 */
double CDECL MSVCRT_difftime(MSVCRT_time_t time1, MSVCRT_time_t time2)
{
  return (double)(time1 - time2);
}

/*********************************************************************
 *		_ftime (MSVCRT.@)
 */
void CDECL _ftime(struct MSVCRT__timeb *buf)
{
  TIME_ZONE_INFORMATION tzinfo;
  FILETIME ft;
  ULONGLONG time;

  DWORD tzid = GetTimeZoneInformation(&tzinfo);
  GetSystemTimeAsFileTime(&ft);

  time = ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

  buf->time = time / TICKSPERSEC - SECS_1601_TO_1970;
  buf->millitm = (time % TICKSPERSEC) / TICKSPERMSEC;
  buf->timezone = tzinfo.Bias +
      ( tzid == TIME_ZONE_ID_STANDARD ? tzinfo.StandardBias :
      ( tzid == TIME_ZONE_ID_DAYLIGHT ? tzinfo.DaylightBias : 0 ));
  buf->dstflag = (tzid == TIME_ZONE_ID_DAYLIGHT?1:0);
}

/*********************************************************************
 *		time (MSVCRT.@)
 */
MSVCRT_time_t CDECL MSVCRT_time(MSVCRT_time_t* buf)
{
  MSVCRT_time_t curtime;
  struct MSVCRT__timeb tb;

  _ftime(&tb);

  curtime = tb.time;
  return buf ? *buf = curtime : curtime;
}

/*********************************************************************
 *		_daylight (MSVCRT.@)
 */
int MSVCRT___daylight = 0;

/*********************************************************************
 *		__p_daylight (MSVCRT.@)
 */
int * CDECL MSVCRT___p__daylight(void)
{
	return &MSVCRT___daylight;
}

/*********************************************************************
 *		_dstbias (MSVCRT.@)
 */
int MSVCRT__dstbias = 0;

/*********************************************************************
 *		__p_dstbias (MSVCRT.@)
 */
int * CDECL __p__dstbias(void)
{
    return &MSVCRT__dstbias;
}

/*********************************************************************
 *		_timezone (MSVCRT.@)
 */
long MSVCRT___timezone = 0;

/*********************************************************************
 *		__p_timezone (MSVCRT.@)
 */
long * CDECL MSVCRT___p__timezone(void)
{
	return &MSVCRT___timezone;
}

/*********************************************************************
 *		_tzname (MSVCRT.@)
 * NOTES
 *  Some apps (notably Mozilla) insist on writing to these, so the buffer
 *  must be large enough.  The size is picked based on observation of
 *  Windows XP.
 */
static char tzname_std[64] = "";
static char tzname_dst[64] = "";
char *MSVCRT__tzname[2] = { tzname_std, tzname_dst };

/*********************************************************************
 *		__p_tzname (MSVCRT.@)
 */
char ** CDECL __p__tzname(void)
{
	return MSVCRT__tzname;
}

/*********************************************************************
 *		_tzset (MSVCRT.@)
 */
void CDECL MSVCRT__tzset(void)
{
    tzset();
#if defined(HAVE_TIMEZONE) && defined(HAVE_DAYLIGHT)
    MSVCRT___daylight = daylight;
    MSVCRT___timezone = timezone;
#else
    {
        static const time_t seconds_in_year = (365 * 24 + 6) * 3600;
        time_t t;
        struct tm *tmp;
        long zone_january, zone_july;

        t = (time((time_t *)0) / seconds_in_year) * seconds_in_year;
        tmp = localtime(&t);
        zone_january = -tmp->tm_gmtoff;
        t += seconds_in_year / 2;
        tmp = localtime(&t);
        zone_july = -tmp->tm_gmtoff;
        MSVCRT___daylight = (zone_january != zone_july);
        MSVCRT___timezone = max(zone_january, zone_july);
    }
#endif
    lstrcpynA(tzname_std, tzname[0], sizeof(tzname_std));
    tzname_std[sizeof(tzname_std) - 1] = '\0';
    lstrcpynA(tzname_dst, tzname[1], sizeof(tzname_dst));
    tzname_dst[sizeof(tzname_dst) - 1] = '\0';
}

/*********************************************************************
 *		strftime (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_strftime( char *str, MSVCRT_size_t max, const char *format,
                                     const struct MSVCRT_tm *mstm )
{
    struct tm tm;

    msvcrt_tm_to_unix( &tm, mstm );
    return strftime( str, max, format, &tm );
}

/*********************************************************************
 *		wcsftime (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcsftime( MSVCRT_wchar_t *str, MSVCRT_size_t max,
                                     const MSVCRT_wchar_t *format, const struct MSVCRT_tm *mstm )
{
    char *s, *fmt;
    MSVCRT_size_t len;

    TRACE("%p %d %s %p\n", str, max, debugstr_w(format), mstm );

    len = WideCharToMultiByte( CP_UNIXCP, 0, format, -1, NULL, 0, NULL, NULL );
    if (!(fmt = MSVCRT_malloc( len ))) return 0;
    WideCharToMultiByte( CP_UNIXCP, 0, format, -1, fmt, len, NULL, NULL );

    if ((s = MSVCRT_malloc( max*4 )))
    {
        struct tm tm;
        msvcrt_tm_to_unix( &tm, mstm );
        if (!strftime( s, max*4, fmt, &tm )) s[0] = 0;
        len = MultiByteToWideChar( CP_UNIXCP, 0, s, -1, str, max );
        if (len) len--;
        MSVCRT_free( s );
    }
    else len = 0;

    MSVCRT_free( fmt );
    return len;
}

/*********************************************************************
 *		asctime (MSVCRT.@)
 */
char * CDECL MSVCRT_asctime(const struct MSVCRT_tm *mstm)
{
    thread_data_t *data = msvcrt_get_thread_data();
    struct tm tm;

    msvcrt_tm_to_unix( &tm, mstm );

    if (!data->asctime_buffer)
        data->asctime_buffer = MSVCRT_malloc( 30 ); /* ought to be enough */

    /* FIXME: may want to map from Unix codepage to CP_ACP */
#ifdef HAVE_ASCTIME_R
    asctime_r( &tm, data->asctime_buffer );
#else
    strcpy( data->asctime_buffer, asctime(&tm) );
#endif
    return data->asctime_buffer;
}

/*********************************************************************
 *		_wasctime (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT__wasctime(const struct MSVCRT_tm *mstm)
{
    thread_data_t *data = msvcrt_get_thread_data();
    struct tm tm;
    char buffer[30];

    msvcrt_tm_to_unix( &tm, mstm );

    if (!data->wasctime_buffer)
        data->wasctime_buffer = MSVCRT_malloc( 30*sizeof(MSVCRT_wchar_t) ); /* ought to be enough */
#ifdef HAVE_ASCTIME_R
    asctime_r( &tm, buffer );
#else
    strcpy( buffer, asctime(&tm) );
#endif
    MultiByteToWideChar( CP_UNIXCP, 0, buffer, -1, data->wasctime_buffer, 30 );
    return data->wasctime_buffer;
}

/*********************************************************************
 *		ctime (MSVCRT.@)
 */
char * CDECL MSVCRT_ctime(const MSVCRT_time_t *time)
{
    return MSVCRT_asctime( MSVCRT_localtime(time) );
}

/*********************************************************************
 *		_wctime (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT__wctime(const MSVCRT_time_t *time)
{
    return MSVCRT__wasctime( MSVCRT_localtime(time) );
}
