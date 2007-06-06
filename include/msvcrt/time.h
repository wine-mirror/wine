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
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

#if defined(__x86_64__) && !defined(_WIN64)
#define _WIN64
#endif

#if !defined(_MSC_VER) && !defined(__int64)
# ifdef _WIN64
#   define __int64 long
# else
#   define __int64 long long
# endif
#endif

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

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

#define _daylight (*__p__daylight())
#define _dstbias (*__p__dstbias())
#define _timezone (*__p__timezone())
#define _tzname (__p__tzname())

int *__p__daylight(void);
long *__p__dstbias(void);
long *__p__timezone(void);
char **__p__tzname(void);

unsigned    _getsystime(struct tm*);
unsigned    _setsystime(struct tm*,unsigned);
char*       _strdate(char*);
char*       _strtime(char*);
void        _tzset(void);

char*       asctime(const struct tm*);
clock_t clock(void);
char*       ctime(const time_t*);
double      difftime(time_t,time_t);
struct tm* gmtime(const time_t*);
struct tm* localtime(const time_t*);
time_t mktime(struct tm*);
size_t      strftime(char*,size_t,const char*,const struct tm*);
time_t time(time_t*);

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED
wchar_t* _wasctime(const struct tm*);
size_t  wcsftime(wchar_t*,size_t,const wchar_t*,const struct tm*);
wchar_t*_wctime(const time_t*);
wchar_t*_wstrdate(wchar_t*);
wchar_t*_wstrtime(wchar_t*);
#endif /* _WTIME_DEFINED */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_TIME_H */
