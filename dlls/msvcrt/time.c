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

DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct __MSVCRT_timeb
{
  time_t  time;
  unsigned short  millitm;
  short   timezone;
  short   dstflag;
} MSVCRT_timeb;


/* INTERNAL: Return formatted current time/date */
char* msvcrt_get_current_time(char* out, const char* format)
{
  static const time_t bad_time = (time_t)-1;
  time_t t;
  struct tm *_tm = NULL;
  char *retval = NULL;

  if (time(&t) != bad_time && (_tm = localtime(&t)) &&
      strftime(out,9,format,_tm) == 8)
    retval = out;
  if (_tm)
    MSVCRT_free(_tm);
  return retval;
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
clock_t MSVCRT_clock(void)
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
double MSVCRT_difftime(time_t time1, time_t time2)
{
    return (double)(time1 - time2);
}

/*********************************************************************
 *		time (MSVCRT.@)
 */
time_t MSVCRT_time(time_t* buf)
{
  time_t curtime = time(NULL);
  return buf ? *buf = curtime : curtime;
}

/*********************************************************************
 *		_ftime (MSVCRT.@)
 */
void _ftime(MSVCRT_timeb* buf)
{
  buf->time = MSVCRT_time(NULL);
  buf->millitm = 0; /* FIXME */
  buf->timezone = 0;
  buf->dstflag = 0;
}
