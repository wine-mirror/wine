/*
 * Copyright 1997 Marcus Meissner
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

#ifndef __WINE_OLE_IFS_H
#define __WINE_OLE_IFS_H

#include "objbase.h"

/***********************************************************************
 * IMalloc16 interface
 */

typedef struct IMalloc16 IMalloc16, *LPMALLOC16;

#define ICOM_INTERFACE IMalloc16
#define IMalloc16_METHODS \
    ICOM_METHOD1 (LPVOID, Alloc,        DWORD,  cb) \
    ICOM_METHOD2 (LPVOID, Realloc,      LPVOID, pv, DWORD, cb) \
    ICOM_VMETHOD1(        Free,         LPVOID, pv) \
    ICOM_METHOD1 (DWORD,  GetSize,      LPVOID, pv) \
    ICOM_METHOD1 (INT16,  DidAlloc,     LPVOID, pv) \
    ICOM_METHOD  (LPVOID, HeapMinimize)
#define IMalloc16_IMETHODS \
    IUnknown_IMETHODS \
    IMalloc16_METHODS
ICOM_DEFINE(IMalloc16,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IMalloc16_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IMalloc16_AddRef(p)             ICOM_CALL (AddRef,p)
#define IMalloc16_Release(p)            ICOM_CALL (Release,p)
/*** IMalloc16 methods ***/
#define IMalloc16_Alloc(p,a)      ICOM_CALL1(Alloc,p,a)
#define IMalloc16_Realloc(p,a,b)  ICOM_CALL2(Realloc,p,a,b)
#define IMalloc16_Free(p,a)       ICOM_CALL1(Free,p,a)
#define IMalloc16_GetSize(p,a)    ICOM_CALL1(GetSize,p,a)
#define IMalloc16_DidAlloc(p,a)   ICOM_CALL1(DidAlloc,p,a)
#define IMalloc16_HeapMinimize(p) ICOM_CALL (HeapMinimize,p)

/**********************************************************************/

extern LPMALLOC16 IMalloc16_Constructor();

#endif /* __WINE_OLE_IFS_H */
