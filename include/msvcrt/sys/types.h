/*
 * _stat() definitions
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
#ifndef __WINE_SYS_TYPES_H
#define __WINE_SYS_TYPES_H
#define __WINE_USE_MSVCRT


#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


typedef unsigned int   _dev_t;
typedef unsigned short _ino_t;
typedef int            _off_t;
typedef long           MSVCRT(time_t);


#ifndef USE_MSVCRT_PREFIX
#define dev_t _dev_t
#define ino_t _ino_t
#define off_t _off_t
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_TYPES_H */
