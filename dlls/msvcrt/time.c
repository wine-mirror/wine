/*
 * msvcrt.dll date/time functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 */

#include <time.h>
#include <sys/times.h>

#include "msvcrt.h"
#include "msvcrt/sys/timeb.h"
#include "msvcrt/time.h"


#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);


/* INTERNAL: Return formatted current time/date */
char* msvcrt_get_current_time(char* out, const char* format)
{
  static const MSVCRT_time_t bad_time = (MSVCRT_time_t)-1;
  time_t t;
  struct tm *_tm = NULL;
  char *retval = NULL;

  if (time(&t) != bad_time && (_tm = localtime(&t)) &&
      strftime(out,9,format,_tm) == 8)
    retval = out;
  return retval;
}

/**********************************************************************
 *		mktime (MSVCRT.@)
 */
MSVCRT_time_t MSVCRT_mktime(struct MSVCRT_tm *t)
{
  MSVCRT_time_t res;
  struct tm tm;

  tm.tm_sec = t->tm_sec;
  tm.tm_min = t->tm_min;
  tm.tm_hour = t->tm_hour;
  tm.tm_mday = t->tm_mday;
  tm.tm_mon = t->tm_mon;
  tm.tm_year = t->tm_year;
  tm.tm_isdst = t->tm_isdst;
  res = mktime(&tm);
  if (res != -1)
  {
      t->tm_sec = tm.tm_sec;
      t->tm_min = tm.tm_min;
      t->tm_hour = tm.tm_hour;
      t->tm_mday = tm.tm_mday;
      t->tm_mon = tm.tm_mon;
      t->tm_year = tm.tm_year;
      t->tm_isdst = tm.tm_isdst;
  }
  return res;
}

/**********************************************************************
 *		_strdate (MSVCRT.@)
 */
char* _strdate(char* date)
{
  return msvcrt_get_current_time(date,"%m/%d/%y");
}

/*********************************************************************
 *		_strtime (MSVCRT.@)
 */
char* _strtime(char* date)
{
  return msvcrt_get_current_time(date,"%H:%M:%S");
}

/*********************************************************************
 *		clock (MSVCRT.@)
 */
MSVCRT_clock_t MSVCRT_clock(void)
{
  struct tms alltimes;
  clock_t res;

  times(&alltimes);
  res = alltimes.tms_utime + alltimes.tms_stime +
        alltimes.tms_cutime + alltimes.tms_cstime;
  /* FIXME: We need some symbolic representation for CLOCKS_PER_SEC,
   *  10 holds only for Windows/Linux_i86)
   */
  return 10*res;
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
  MSVCRT_time_t curtime = time(NULL);
  return buf ? *buf = curtime : curtime;
}

/*********************************************************************
 *		_ftime (MSVCRT.@)
 */
void _ftime(struct _timeb *buf)
{
  buf->time = MSVCRT_time(NULL);
  buf->millitm = 0; /* FIXME */
  buf->timezone = 0;
  buf->dstflag = 0;
}
