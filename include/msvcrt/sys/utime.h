/*
 * Path and directory definitions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
