/*
 * Path and directory definitions
 *
 * Copyright 2000 Francois Gouget.
 */
#ifndef __WINE_SYS_TIMEB_H
#define __WINE_SYS_TIMEB_H
#define __WINE_USE_MSVCRT

#include "msvcrt/sys/types.h"      /* For time_t */


struct _timeb
{
    MSVCRT(time_t) time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};


#ifdef __cplusplus
extern "C" {
#endif

void        _ftime(struct _timeb*);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define timeb _timeb

#define ftime _ftime
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_TIMEB_H */
