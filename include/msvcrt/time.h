/*
 * Time definitions
 *
 * Copyright 2000 Francois Gouget.
 */
#ifndef __WINE_TIME_H
#define __WINE_TIME_H
#define __WINE_USE_MSVCRT

#include "winnt.h"
#include "msvcrt/sys/types.h"      /* For time_t */


#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

typedef long MSVCRT(clock_t);

struct MSVCRT(tm) {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};


#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: Must do something for _daylight, _dstbias, _timezone, _tzname */


unsigned    _getsystime(struct MSVCRT(tm)*);
unsigned    _setsystime(struct MSVCRT(tm)*,unsigned);
char*       _strdate(char*);
char*       _strtime(char*);
void        _tzset(void);

char*       MSVCRT(asctime)(const struct MSVCRT(tm)*);
MSVCRT(clock_t) MSVCRT(clock)(void);
char*       MSVCRT(ctime)(const MSVCRT(time_t)*);
double      MSVCRT(difftime)(MSVCRT(time_t),MSVCRT(time_t));
struct MSVCRT(tm)* MSVCRT(gmtime)(const MSVCRT(time_t)*);
struct MSVCRT(tm)* MSVCRT(localtime)(const MSVCRT(time_t)*);
MSVCRT(time_t) MSVCRT(mktime)(struct MSVCRT(tm)*);
size_t      MSVCRT(strftime)(char*,size_t,const char*,const struct MSVCRT(tm)*);
MSVCRT(time_t) MSVCRT(time)(MSVCRT(time_t)*);

WCHAR*      _wasctime(const struct MSVCRT(tm)*);
MSVCRT(size_t) wcsftime(WCHAR*,MSVCRT(size_t),const WCHAR*,const struct MSVCRT(tm)*);
WCHAR*      _wctime(const MSVCRT(time_t)*);
WCHAR*      _wstrdate(WCHAR*);
WCHAR*      _wstrtime(WCHAR*);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_TIME_H */
