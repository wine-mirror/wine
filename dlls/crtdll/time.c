/*
 * CRTDLL date/time functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * Implementation Notes:
 * MT Safe.
 */

#include "crtdll.h"
#include "process.h"
#include "options.h"
#include <stdlib.h>
#include <sys/times.h>


DEFAULT_DEBUG_CHANNEL(crtdll);


/* INTERNAL: Return formatted current time/date */
static LPSTR __CRTDLL__get_current_time(LPSTR out, const char * format);
static LPSTR __CRTDLL__get_current_time(LPSTR out, const char * format)
{
  time_t t;
  struct tm *_tm;

  if ((time(&t) != ((time_t)-1)) &&
      ((_tm = localtime(&t)) != 0) &&
      (strftime(out,9,format,_tm) == 8))
  {
    CRTDLL_free(_tm);
    return out;
  }

  return NULL;
}


/*********************************************************************
 *                  _ftime      (CRTDLL.112)
 *
 * Get current time.
 */
VOID __cdecl CRTDLL__ftime (struct _timeb* t)
{
  t->time = CRTDLL_time(NULL);
  t->millitm = 0; /* FIXME */
  t->timezone = 0;
  t->dstflag = 0;
}


/**********************************************************************
 *                  _strdate          (CRTDLL.283)
 *
 * Return the current date as MM/DD/YY - (American Format)
 */
LPSTR __cdecl CRTDLL__strdate (LPSTR date)
{
  return __CRTDLL__get_current_time(date,"%m/%d/%y");
}


/*********************************************************************
 *                  _strtime          (CRTDLL.299)
 *
 * Return the current time as HH:MM:SS
 */
LPSTR __cdecl CRTDLL__strtime (LPSTR date)
{
  return __CRTDLL__get_current_time(date,"%H:%M:%S");
}


/*********************************************************************
 *                  clock         (CRTDLL.350)
 */
clock_t __cdecl CRTDLL_clock(void)
{
    struct tms alltimes;
    clock_t res;

    times(&alltimes);
    res = alltimes.tms_utime + alltimes.tms_stime+
        alltimes.tms_cutime + alltimes.tms_cstime;
    /* Fixme: We need some symbolic representation
       for (Hostsystem_)CLOCKS_PER_SEC 
       and (Emulated_system_)CLOCKS_PER_SEC
       10 holds only for Windows/Linux_i86)
    */
    return 10*res;
}


/*********************************************************************
 *                  difftime      (CRTDLL.357)
 */
double __cdecl CRTDLL_difftime (time_t time1, time_t time2)
{
    double timediff;

    timediff = (double)(time1 - time2);
    return timediff;
}


/*********************************************************************
 *                  time          (CRTDLL.488)
 */
time_t __cdecl CRTDLL_time(time_t *timeptr)
{
    time_t curtime = time(NULL);

    if (timeptr)
        *timeptr = curtime;
    return curtime;
}
