/*
 * Win95 Flat Thunk data structures
 *
 * Copyright 1998 Ulrich Weigand
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

#ifndef __WINE_FLATTHUNK_H
#define __WINE_FLATTHUNK_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

struct ThunkDataCommon
{
    char                   magic[4];         /* 00 */
    DWORD                  checksum;         /* 04 */
};

struct ThunkDataLS16
{
    struct ThunkDataCommon common;           /* 00 */
    SEGPTR                 targetTable;      /* 08 */
    DWORD                  firstTime;        /* 0C */
};

struct ThunkDataLS32
{
    struct ThunkDataCommon common;           /* 00 */
    DWORD *                targetTable;      /* 08 */
    char                   lateBinding[4];   /* 0C */
    DWORD                  flags;            /* 10 */
    DWORD                  reserved1;        /* 14 */
    DWORD                  reserved2;        /* 18 */
    DWORD                  offsetQTThunk;    /* 1C */
    DWORD                  offsetFTProlog;   /* 20 */
};

struct ThunkDataSL16
{
    struct ThunkDataCommon common;            /* 00 */
    DWORD                  flags1;            /* 08 */
    DWORD                  reserved1;         /* 0C */
    struct ThunkDataSL *   fpData;            /* 10 */
    SEGPTR                 spData;            /* 14 */
    DWORD                  reserved2;         /* 18 */
    char                   lateBinding[4];    /* 1C */
    DWORD                  flags2;            /* 20 */
    DWORD                  reserved3;         /* 20 */
    SEGPTR                 apiDatabase;       /* 28 */
};

struct ThunkDataSL32
{
    struct ThunkDataCommon common;            /* 00 */
    DWORD                  reserved1;         /* 08 */
    struct ThunkDataSL *   data;              /* 0C */
    char                   lateBinding[4];    /* 10 */
    DWORD                  flags;             /* 14 */
    DWORD                  reserved2;         /* 18 */
    DWORD                  reserved3;         /* 1C */
    DWORD                  offsetTargetTable; /* 20 */
};

struct ThunkDataSL
{
#if 0
    This structure differs from the Win95 original,
    but this should not matter since it is strictly internal to
    the thunk handling routines in KRNL386 / KERNEL32.

    For reference, here is the Win95 layout:

    struct ThunkDataCommon common;            /* 00 */
    DWORD                  flags1;            /* 08 */
    SEGPTR                 apiDatabase;       /* 0C */
    WORD                   exePtr;            /* 10 */
    WORD                   segMBA;            /* 12 */
    DWORD                  lenMBATotal;       /* 14 */
    DWORD                  lenMBAUsed;        /* 18 */
    DWORD                  flags2;            /* 1C */
    char                   pszDll16[256];     /* 20 */
    char                   pszDll32[256];     /*120 */

    We do it differently since all our thunk handling is done
    by 32-bit code. Therefore we do not need do provide
    easy access to this data, especially the process target
    table database, for 16-bit code.
#endif

    struct ThunkDataCommon common;
    DWORD                  flags1;
    struct SLApiDB *       apiDB;
    struct SLTargetDB *    targetDB;
    DWORD                  flags2;
    char                   pszDll16[256];
    char                   pszDll32[256];
};

struct SLTargetDB
{
     struct SLTargetDB *   next;
     DWORD                 process;
     DWORD *               targetTable;
};

struct SLApiDB
{
    DWORD                  nrArgBytes;
    DWORD                  errorReturnValue;
};


#endif /* __WINE_FLATTHUNK_H */

