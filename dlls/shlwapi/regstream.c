/*
 * SHLWAPI Registry Stream functions
 *
 * Copyright 1999 Juergen Schmied
 * Copyright 2002 Jon Griffiths
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
	IStream IStream_iface;
	LONG   ref;
	LPBYTE pbBuffer;
	DWORD  dwLength;
	DWORD  dwPos;
	DWORD  dwMode;
} ISHRegStream;

static inline ISHRegStream *impl_from_IStream(IStream *iface)
{
	return CONTAINING_RECORD(iface, ISHRegStream, IStream_iface);
}

/**************************************************************************
*  IStream_fnQueryInterface
*/
static HRESULT WINAPI IStream_fnQueryInterface(IStream *iface, REFIID riid, LPVOID *ppvObj)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

       if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IStream))
         *ppvObj = &This->IStream_iface;

	if(*ppvObj)
	{
	  IStream_AddRef((IStream*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IStream_fnAddRef
*/
static ULONG WINAPI IStream_fnAddRef(IStream *iface)
{
	ISHRegStream *This = impl_from_IStream(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);
	
	TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
	ISHRegStream *This = impl_from_IStream(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

	if (!refCount)
	{
	  free(This->pbBuffer);
	  free(This);
	  return 0;
	}

	return refCount;
}

/**************************************************************************
 * IStream_fnRead
 */
static HRESULT WINAPI IStream_fnRead (IStream * iface, void* pv, ULONG cb, ULONG* pcbRead)
{
	ISHRegStream *This = impl_from_IStream(iface);
	DWORD dwBytesToRead;

	TRACE("(%p)->(%p,0x%08lx,%p)\n",This, pv, cb, pcbRead);

	if (This->dwPos >= This->dwLength)
	  dwBytesToRead = 0;
        else
	  dwBytesToRead = This->dwLength - This->dwPos;

	dwBytesToRead = (cb > dwBytesToRead) ? dwBytesToRead : cb;
	if (dwBytesToRead != 0) /* not at end of buffer and we want to read something */
	{
	  memmove(pv, This->pbBuffer + This->dwPos, dwBytesToRead);
	  This->dwPos += dwBytesToRead; /* adjust pointer */
	}

	if (pcbRead)
	  *pcbRead = dwBytesToRead;

	return S_OK;
}

/**************************************************************************
 * IStream_fnWrite
 */
static HRESULT WINAPI IStream_fnWrite (IStream * iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
	ISHRegStream *This = impl_from_IStream(iface);
	DWORD newLen = This->dwPos + cb;

	TRACE("(%p, %p, %ld, %p)\n",This, pv, cb, pcbWritten);

	if (newLen < This->dwPos) /* overflow */
	  return STG_E_INSUFFICIENTMEMORY;

	if (newLen > This->dwLength)
	{
	  BYTE *newBuf = _recalloc(This->pbBuffer, 1, newLen);
	  if (!newBuf)
	    return STG_E_INSUFFICIENTMEMORY;

	  This->dwLength = newLen;
	  This->pbBuffer = newBuf;
	}
	memmove(This->pbBuffer + This->dwPos, pv, cb);
	This->dwPos += cb; /* adjust pointer */

	if (pcbWritten)
	  *pcbWritten = cb;

	return S_OK;
}

/**************************************************************************
 *  IStream_fnSeek
 */
static HRESULT WINAPI IStream_fnSeek (IStream * iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ISHRegStream *This = impl_from_IStream(iface);
	LARGE_INTEGER tmp;
	TRACE("(%p, %s, %ld %p)\n", This,
              wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);

	if (dwOrigin == STREAM_SEEK_SET)
	  tmp = dlibMove;
        else if (dwOrigin == STREAM_SEEK_CUR)
	  tmp.QuadPart = This->dwPos + dlibMove.QuadPart;
	else if (dwOrigin == STREAM_SEEK_END)
	  tmp.QuadPart = This->dwLength + dlibMove.QuadPart;
        else
	  return STG_E_INVALIDPARAMETER;

	if (tmp.QuadPart < 0)
	  return STG_E_INVALIDFUNCTION;

	/* we cut off the high part here */
	This->dwPos = tmp.u.LowPart;

	if (plibNewPosition)
	  plibNewPosition->QuadPart = This->dwPos;
	return S_OK;
}

/**************************************************************************
 * IStream_fnSetSize
 */
static HRESULT WINAPI IStream_fnSetSize (IStream * iface, ULARGE_INTEGER libNewSize)
{
	ISHRegStream *This = impl_from_IStream(iface);
	DWORD newLen;
	LPBYTE newBuf;

	TRACE("(%p, %s)\n", This, wine_dbgstr_longlong(libNewSize.QuadPart));

	/* we cut off the high part here */
	newLen = libNewSize.u.LowPart;
	newBuf = _recalloc(This->pbBuffer, 1, newLen);
	if (!newBuf)
	  return STG_E_INSUFFICIENTMEMORY;

	This->pbBuffer = newBuf;
	This->dwLength = newLen;

	return S_OK;
}

/**************************************************************************
 * IStream_fnCopyTo
 */
static HRESULT WINAPI IStream_fnCopyTo (IStream * iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);
	if (pcbRead)
	  pcbRead->QuadPart = 0;
	if (pcbWritten)
	  pcbWritten->QuadPart = 0;

	/* TODO implement */
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnCommit
 */
static HRESULT WINAPI IStream_fnCommit (IStream * iface, DWORD grfCommitFlags)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);

	/* commit not supported by this stream */
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnRevert
 */
static HRESULT WINAPI IStream_fnRevert (IStream * iface)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);

	/* revert not supported by this stream */
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnLockUnlockRegion
 */
static HRESULT WINAPI IStream_fnLockUnlockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);

	/* lock/unlock not supported by this stream */
	return E_NOTIMPL;
}

/*************************************************************************
 * IStream_fnStat
 */
static HRESULT WINAPI IStream_fnStat (IStream * iface, STATSTG* pstatstg, DWORD grfStatFlag)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p, %p, %ld)\n",This,pstatstg,grfStatFlag);

	pstatstg->pwcsName = NULL;
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = This->dwLength;
	pstatstg->mtime.dwHighDateTime = 0;
	pstatstg->mtime.dwLowDateTime = 0;
	pstatstg->ctime.dwHighDateTime = 0;
	pstatstg->ctime.dwLowDateTime = 0;
	pstatstg->atime.dwHighDateTime = 0;
	pstatstg->atime.dwLowDateTime = 0;
	pstatstg->grfMode = This->dwMode;
	pstatstg->grfLocksSupported = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	pstatstg->reserved = 0;

	return S_OK;
}

/*************************************************************************
 * IStream_fnClone
 */
static HRESULT WINAPI IStream_fnClone (IStream * iface, IStream** ppstm)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);
	*ppstm = NULL;

	/* clone not supported by this stream */
	return E_NOTIMPL;
}

static const IStreamVtbl rstvt =
{
	IStream_fnQueryInterface,
	IStream_fnAddRef,
	IStream_fnRelease,
	IStream_fnRead,
	IStream_fnWrite,
	IStream_fnSeek,
	IStream_fnSetSize,
	IStream_fnCopyTo,
	IStream_fnCommit,
	IStream_fnRevert,
	IStream_fnLockUnlockRegion,
	IStream_fnLockUnlockRegion,
	IStream_fnStat,
	IStream_fnClone
};

/**************************************************************************
 * IStream_Create
 *
 * Internal helper: Create and initialise a new registry stream object.
 */
static ISHRegStream *IStream_Create(HKEY hKey, LPBYTE pbBuffer, DWORD dwLength)
{
 ISHRegStream* regStream;

 regStream = malloc(sizeof(ISHRegStream));

 if (regStream)
 {
   regStream->IStream_iface.lpVtbl = &rstvt;
   regStream->ref = 1;
   regStream->pbBuffer = pbBuffer;
   regStream->dwLength = dwLength;
   regStream->dwPos = 0;
   regStream->dwMode = STGM_READWRITE;
 }
 TRACE ("Returning %p\n", regStream);
 return regStream;
}

/*************************************************************************
 * SHCreateStreamWrapper   [SHLWAPI.@]
 *
 * Create an IStream object on a block of memory.
 *
 * PARAMS
 * lpbData    [I] Memory block to create the IStream object on
 * dwDataLen  [I] Length of data block
 * dwReserved [I] Reserved, Must be 0.
 * lppStream  [O] Destination for IStream object
 *
 * RETURNS
 * Success: S_OK. lppStream contains the new IStream object.
 * Failure: E_INVALIDARG, if any parameters are invalid,
 *          E_OUTOFMEMORY if memory allocation fails.
 *
 * NOTES
 *  The stream assumes ownership of the memory passed to it.
 */
HRESULT WINAPI SHCreateStreamWrapper(LPBYTE lpbData, DWORD dwDataLen,
                                     DWORD dwReserved, IStream **lppStream)
{
  ISHRegStream *strm;

  if (lppStream)
    *lppStream = NULL;

  if(dwReserved || !lppStream)
    return E_INVALIDARG;

  strm = IStream_Create(NULL, lpbData, dwDataLen);

  if(!strm)
    return E_OUTOFMEMORY;

  IStream_QueryInterface(&strm->IStream_iface, &IID_IStream, (void**)lppStream);
  IStream_Release(&strm->IStream_iface);
  return S_OK;
}
