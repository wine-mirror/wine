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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <time.h>
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif

#include "msvcrt.h"
#include "msvcrt/sys/timeb.h"
#include "msvcrt/time.h"

#include "winbase.h"

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

#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000
#define TICKSPERMSEC      10000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)

/* native uses a single static buffer for localtime/gmtime/mktime */
static struct MSVCRT_tm tm;

/**********************************************************************
 *		mktime (MSVCRT.@)
 */
MSVCRT_time_t MSVCRT_mktime(struct MSVCRT_tm *t)
{
  MSVCRT_time_t secs;

  SYSTEMTIME st;
  FILETIME lft, uft;
  ULONGLONG time;

  st.wMilliseconds = 0;
  st.wSecond = t->tm_sec;
  st.wMinute = t->tm_min;
  st.wHour   = t->tm_hour;
  st.wDay    = t->tm_mday;
  st.wMonth  = t->tm_mon + 1;
  st.wYear   = t->tm_year + 1900;

  SystemTimeToFileTime(&st, &lft);
  LocalFileTimeToFileTime(&lft, &uft);

  time = ((ULONGLONG)uft.dwHighDateTime << 32) | uft.dwLowDateTime;
  secs = time / TICKSPERSEC - SECS_1601_TO_1970;

  return secs; 
}

/*********************************************************************
 *      localtime (MSVCRT.@)
 */
struct MSVCRT_tm* MSVCRT_localtime(const MSVCRT_time_t* secs)
{
  int i;

  FILETIME ft, lft;
  SYSTEMTIME st;
  DWORD tzid;
  TIME_ZONE_INFORMATION tzinfo;

  ULONGLONG time = *secs * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;

  ft.dwHighDateTime = (UINT)(time >> 32);
  ft.dwLowDateTime  = (UINT)time;

  FileTimeToLocalFileTime(&ft, &lft);
  FileTimeToSystemTime(&lft, &st);

  if (st.wYear < 1970) return NULL;

  tm.tm_sec  = st.wSecond;
  tm.tm_min  = st.wMinute;
  tm.tm_hour = st.wHour;
  tm.tm_mday = st.wDay;
  tm.tm_year = st.wYear - 1900;
  tm.tm_mon  = st.wMonth  - 1;
  tm.tm_wday = st.wDayOfWeek;

  for (i = tm.tm_yday = 0; i < st.wMonth - 1; i++) {
    tm.tm_yday += MonthLengths[IsLeapYear(st.wYear)][i];
  }

  tm.tm_yday += st.wDay - 1;
 
  tzid = GetTimeZoneInformation(&tzinfo);

  if (tzid == TIME_ZONE_ID_UNKNOWN) {
    tm.tm_isdst = -1;
  } else {
    tm.tm_isdst = (tzid == TIME_ZONE_ID_DAYLIGHT?1:0);
  }

  return &tm;
}

struct MSVCRT_tm* MSVCRT_gmtime(const MSVCRT_time_t* secs)
{
  int i;

  FILETIME ft;
  SYSTEMTIME st;

  ULONGLONG time = *secs * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;

  ft.dwHighDateTime = (UINT)(time >> 32);
  ft.dwLowDateTime  = (UINT)time;

  FileTimeToSystemTime(&ft, &st);

  if (st.wYear < 1970) return NULL;

  tm.tm_sec  = st.wSecond;
  tm.tm_min  = st.wMinute;
  tm.tm_hour = st.wHour;
  tm.tm_mday = st.wDay;
  tm.tm_year = st.wYear - 1900;
  tm.tm_mon  = st.wMonth - 1;
  tm.tm_wday = st.wDayOfWeek;
  for (i = tm.tm_yday = 0; i < st.wMonth - 1; i++) {
    tm.tm_yday += MonthLengths[IsLeapYear(st.wYear)][i];
  }

  tm.tm_yday += st.wDay - 1;
  tm.tm_isdst = 0;

  return &tm;
}

/**********************************************************************
 *		_strdate (MSVCRT.@)
 */
char* _strdate(char* date)
{
  LPCSTR format = "MM'/'dd'/'yy";

  GetDateFormatA(LOCALE_NEUTRAL, 0, NULL, format, date, 9);

  return date;
}

/*********************************************************************
 *		_strtime (MSVCRT.@)
 */
char* _strtime(char* date)
{
  LPCSTR format = "HH':'mm':'ss";

  GetTimeFormatA(LOCALE_NEUTRAL, 0, NULL, format, date, 9); 

  return date;
}

/*********************************************************************
 *		clock (MSVCRT.@)
 */
MSVCRT_clock_t MSVCRT_clock(void)
{
  FILETIME ftc, fte, ftk, ftu;
  ULONGLONG utime, ktime;
 
  MSVCRT_clock_t clock;

  GetProcessTimes(GetCurrentProcess(), &ftc, &fte, &ftk, &ftu);

  ktime = ((ULONGLONG)ftk.dwHighDateTime << 32) | ftk.dwLowDateTime;
  utime = ((ULONGLONG)ftu.dwHighDateTime << 32) | ftu.dwLowDateTime;

  clock = ((utime + ktime) / TICKSPERSEC) * CLOCKS_PER_SEC;

  return clock;
}

/*********************************************************************
 *		difftime (MSVCRT.@)
 */
double MSVCRT_difftime(MSVCRT_time_t time1, MSVCRT_time_t time2)
{
  return (double)(time1 - time2);
}

/*********************************************************************
 *		time (MSVCRT.@)
 */
MSVCRT_time_t MSVCRT_time(MSVCRT_time_t* buf)
{
  MSVCRT_time_t curtime;
  struct _timeb tb;

  _ftime(&tb);

  curtime = tb.time;
  return buf ? *buf = curtime : curtime;
}

/*********************************************************************
 *		_ftime (MSVCRT.@)
 */
void _ftime(struct _timeb *buf)
{
  TIME_ZONE_INFORMATION tzinfo;
  FILETIME ft;
  ULONGLONG time;

  DWORD tzid = GetTimeZoneInformation(&tzinfo);
  GetSystemTimeAsFileTime(&ft);

  time = ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

  buf->time = time / TICKSPERSEC - SECS_1601_TO_1970;
  buf->millitm = (time % TICKSPERSEC) / TICKSPERMSEC;
  buf->timezone = tzinfo.Bias;
  buf->dstflag = (tzid == TIME_ZONE_ID_DAYLIGHT?1:0);
}

/*********************************************************************
 *		_daylight (MSVCRT.@)
 */
int MSVCRT___daylight = 1; /* FIXME: assume daylight */

/*********************************************************************
 *		__p_daylight (MSVCRT.@)
 */
void *MSVCRT___p__daylight(void)
{
	return &MSVCRT___daylight;
}
