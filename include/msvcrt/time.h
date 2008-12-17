/*
 * Time definitions
 *
 * Copyright 2000 Francois Gouget.
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
#ifndef __WINE_TIME_H
#define __WINE_TIME_H

#include <crtdefs.h>

#include <pshpack8.h>

#ifndef _CLOCK_T_DEFINED
typedef long clock_t;
#define _CLOCK_T_DEFINED
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000
#endif

#ifndef _TM_DEFINED
#define _TM_DEFINED
struct tm {
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
#endif /* _TM_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __i386__
#define _daylight (*__p__daylight())
#define _dstbias (*__p__dstbias())
#define _timezone (*__p__timezone())
#define _tzname (__p__tzname())

int *   __cdecl __p__daylight(void);
long *  __cdecl __p__dstbias(void);
long *  __cdecl __p__timezone(void);
char ** __cdecl __p__tzname(void);
#else
extern int _daylight;
extern long _dstbias;
extern long _timezone;
extern char *_tzname;
#endif

unsigned    __cdecl _getsystime(struct tm*);
unsigned    __cdecl _setsystime(struct tm*,unsigned);
char*       __cdecl _strdate(char*);
char*       __cdecl _strtime(char*);
void        __cdecl _tzset(void);

char*       __cdecl asctime(const struct tm*);
clock_t     __cdecl clock(void);
char*       __cdecl ctime(const time_t*);
double      __cdecl difftime(time_t,time_t);
struct tm*  __cdecl gmtime(const time_t*);
struct tm*  __cdecl localtime(const time_t*);
time_t      __cdecl mktime(struct tm*);
size_t      __cdecl strftime(char*,size_t,const char*,const struct tm*);
time_t      __cdecl time(time_t*);

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED
wchar_t* __cdecl _wasctime(const struct tm*);
size_t   __cdecl wcsftime(wchar_t*,size_t,const wchar_t*,const struct tm*);
wchar_t* __cdecl _wctime(const time_t*);
wchar_t* __cdecl _wstrdate(wchar_t*);
wchar_t* __cdecl _wstrtime(wchar_t*);
#endif /* _WTIME_DEFINED */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_TIME_H */
