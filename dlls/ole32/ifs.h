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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

/***********************************************************************
 * IMalloc16 interface
 */

typedef struct IMalloc16 IMalloc16, *LPMALLOC16;

#define INTERFACE IMalloc16
#define IMalloc16_METHODS \
    IUnknown_METHODS \
    STDMETHOD_(LPVOID,Alloc)(THIS_ DWORD   cb) PURE; \
    STDMETHOD_(LPVOID,Realloc)(THIS_ LPVOID  pv, DWORD  cb) PURE; \
    STDMETHOD_(void,Free)(THIS_ LPVOID  pv) PURE; \
    STDMETHOD_(DWORD,GetSize)(THIS_ LPVOID  pv) PURE; \
    STDMETHOD_(INT16,DidAlloc)(THIS_ LPVOID  pv) PURE; \
    STDMETHOD_(LPVOID,HeapMinimize)(THIS) PURE;
ICOM_DEFINE(IMalloc16,IUnknown)
#undef INTERFACE

/**********************************************************************/

extern LPMALLOC16 IMalloc16_Constructor();

/**********************************************************************/

typedef struct ILockBytes16 *LPLOCKBYTES16, ILockBytes16;

#define INTERFACE ILockBytes
#define ILockBytes16_METHODS \
	IUnknown_METHODS \
	STDMETHOD(ReadAt)(THIS_ ULARGE_INTEGER ulOffset, void *pv, ULONG  cb, ULONG *pcbRead) PURE; \
	STDMETHOD(WriteAt)(THIS_ ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten) PURE; \
	STDMETHOD(Flush)(THIS) PURE; \
	STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER cb) PURE; \
	STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER  cb, DWORD dwLockType) PURE; \
	STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER  cb, DWORD dwLockType) PURE; \
	STDMETHOD(Stat)(THIS_ STATSTG *pstatstg, DWORD grfStatFlag) PURE;
ICOM_DEFINE(ILockBytes16,IUnknown)
#undef INTERFACE


#endif /* __WINE_OLE_IFS_H */
