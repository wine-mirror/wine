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

/**********************************************************************/

extern LPMALLOC16 IMalloc16_Constructor();

/**********************************************************************/

typedef struct ILockBytes16 *LPLOCKBYTES16, ILockBytes16;

#define ICOM_INTERFACE ILockBytes
#define ILockBytes16_METHODS \
	ICOM_METHOD4(HRESULT,ReadAt,       ULARGE_INTEGER,ulOffset, void*,pv, ULONG, cb, ULONG*,pcbRead) \
	ICOM_METHOD4(HRESULT,WriteAt,      ULARGE_INTEGER,ulOffset, const void*,pv, ULONG,cb, ULONG*,pcbWritten) \
	ICOM_METHOD (HRESULT,Flush) \
	ICOM_METHOD1(HRESULT,SetSize,      ULARGE_INTEGER,cb) \
	ICOM_METHOD3(HRESULT,LockRegion,   ULARGE_INTEGER,libOffset, ULARGE_INTEGER, cb, DWORD,dwLockType) \
	ICOM_METHOD3(HRESULT,UnlockRegion, ULARGE_INTEGER,libOffset, ULARGE_INTEGER, cb, DWORD,dwLockType) \
	ICOM_METHOD2(HRESULT,Stat,         STATSTG*,pstatstg, DWORD,grfStatFlag)

#define ILockBytes16_IMETHODS \
	IUnknown_IMETHODS \
	ILockBytes16_METHODS

ICOM_DEFINE(ILockBytes16,IUnknown)
#undef ICOM_INTERFACE


#endif /* __WINE_OLE_IFS_H */
