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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_SYS_UTIME_H
#define __WINE_SYS_UTIME_H
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

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef _UTIMBUF_DEFINED
#define _UTIMBUF_DEFINED
struct _utimbuf
{
    time_t actime;
    time_t modtime;
};
#endif /* _UTIMBUF_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

int         _futime(int,struct _utimbuf*);
int         _utime(const char*,struct _utimbuf*);

int         _wutime(const wchar_t*,struct _utimbuf*);

#ifdef __cplusplus
}
#endif


#define utimbuf _utimbuf

static inline int utime(const char* path, struct _utimbuf* buf) { return _utime(path, buf); }

#include <poppack.h>

#endif /* __WINE_SYS_UTIME_H */
