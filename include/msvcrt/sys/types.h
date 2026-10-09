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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_SYS_TYPES_H
#define __WINE_SYS_TYPES_H

#include <corecrt.h>

#ifndef _DEV_T_DEFINED
# ifdef _CRTDLL
typedef unsigned short _dev_t;
# else
typedef unsigned int   _dev_t;
# endif
#define _DEV_T_DEFINED
#endif

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;
#define _INO_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED
typedef __msvcrt_long _off_t;
#define _OFF_T_DEFINED
#endif

typedef _dev_t dev_t;
typedef _ino_t ino_t;
typedef _off_t off_t;

#endif /* __WINE_SYS_TYPES_H */
