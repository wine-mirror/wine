/*
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Alex Korobka
 * Copyright 1999 Noel Borthwick
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

#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "windef.h"
#include "wine/windef16.h"

struct tagWND;

typedef struct tagWINE_CLIPFORMAT {
    UINT	wFormatID;
    UINT	wRefCount;
    BOOL	wDataPresent;
    LPSTR	Name;
    HANDLE16    hData16;
    HANDLE	hDataSrc32;
    HANDLE	hData32;
    ULONG       drvData;
    struct tagWINE_CLIPFORMAT *PrevFormat;
    struct tagWINE_CLIPFORMAT *NextFormat;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

extern LPWINE_CLIPFORMAT CLIPBOARD_LookupFormat( WORD wID );
extern BOOL CLIPBOARD_IsCacheRendered();
extern void CLIPBOARD_DeleteRecord(LPWINE_CLIPFORMAT lpFormat, BOOL bChange);
extern void CLIPBOARD_EmptyCache( BOOL bChange );
extern BOOL CLIPBOARD_IsPresent(WORD wFormat);
extern char * CLIPBOARD_GetFormatName(UINT wFormat, LPSTR buf, INT size);
extern void CLIPBOARD_ReleaseOwner();

#endif /* __WINE_CLIPBOARD_H */
