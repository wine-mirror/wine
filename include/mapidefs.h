/*
 * Copyright (C) 1998 Justin Bradford
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

#ifndef __WINE_MAPIDEFS_H
#define __WINE_MAPIDEFS_H

#include <windows.h>
#include <winerror.h>
#ifndef _OBJBASE_H_
#include <objbase.h>
#endif

/* Some types */

typedef unsigned char*          LPBYTE;
#ifndef __LHANDLE
#define __LHANDLE
typedef unsigned long           LHANDLE, *LPLHANDLE;
#endif

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED
typedef union tagCY
{
   struct {
#ifdef WORDS_BIGENDIAN
       LONG Hi;
       ULONG Lo;
#else
       ULONG Lo;
       LONG Hi;
#endif
   } u;
   LONGLONG int64;
} CY;
#endif /* _tagCY_DEFINED */

/* MAPI Recipient types */
#ifndef MAPI_ORIG
#define MAPI_ORIG        0          /* Message originator   */
#define MAPI_TO          1          /* Primary recipient    */
#define MAPI_CC          2          /* Copy recipient       */
#define MAPI_BCC         3          /* Blind copy recipient */
#define MAPI_P1          0x10000000 /* P1 resend recipient  */
#define MAPI_SUBMITTED   0x80000000 /* Already processed    */
#endif

/* Bit definitions for abFlags[0] */
#define MAPI_SHORTTERM   0x80
#define MAPI_NOTRECIP    0x40
#define MAPI_THISSESSION 0x20
#define MAPI_NOW         0x10
#define MAPI_NOTRESERVED 0x08

/* Bit definitions for abFlags[1]  */
#define MAPI_COMPOUND    0x80

#endif /*__WINE_MAPIDEFS_H*/
