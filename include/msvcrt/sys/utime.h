/*
 * Path and directory definitions
 *
 * Copyright 2000 Francois Gouget.
 */
#ifndef __WINE_SYS_UTIME_H
#define __WINE_SYS_UTIME_H
#define __WINE_USE_MSVCRT

#include "winnt.h"
#include "msvcrt/sys/types.h"      /* For time_t */


struct _utimbuf
{
    MSVCRT(time_t) actime;
    MSVCRT(time_t) modtime;
};


#ifdef __cplusplus
extern "C" {
#endif

int         _futime(int,struct _utimbuf*);
int         _utime(const char*,struct _utimbuf*);

int         _wutime(const WCHAR*,struct _utimbuf*);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define utimbuf _utimbuf

#define utime _utime
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_UTIME_H */
