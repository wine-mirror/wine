/*
 * Module definitions
 *
 * Copyright 1995 Alexandre Julliard
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

#ifndef __WINE_MODULE_H
#define __WINE_MODULE_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wine/windef16.h>
#include <wine/winbase16.h>
#include <winternl.h>

#include <pshpack1.h>

typedef struct {
    BYTE type;
    BYTE flags;
    BYTE segnum;
    WORD offs;
} ET_ENTRY;

typedef struct {
    WORD first; /* ordinal */
    WORD last; /* ordinal */
    WORD next; /* bundle */
} ET_BUNDLE;


  /* In-memory segment table */
typedef struct
{
    WORD      filepos;   /* Position in file, in sectors */
    WORD      size;      /* Segment size on disk */
    WORD      flags;     /* Segment flags */
    WORD      minsize;   /* Min. size of segment in memory */
    HANDLE16  hSeg;      /* Selector or handle (selector - 1) */
                         /* of segment in memory */
} SEGTABLEENTRY;

#include <poppack.h>

enum loadorder_type
{
    LOADORDER_INVALID = 0, /* Must be 0 */
    LOADORDER_DLL,         /* Native DLLs */
    LOADORDER_BI,          /* Built-in modules */
    LOADORDER_NTYPES
};

/* module.c */
extern NTSTATUS MODULE_DllThreadAttach( LPVOID lpReserved );

/* loadorder.c */
extern void MODULE_GetLoadOrderW( enum loadorder_type plo[], const WCHAR *app_name,
                                  const WCHAR *path );

#endif  /* __WINE_MODULE_H */
