/* ILoaderStream Implementation
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);

/*****************************************************************************
 * Custom functions:
 */
HRESULT WINAPI ILoaderStream_Attach (ILoaderStream* This, LPCWSTR wzFile, IDirectMusicLoader *pLoader)
{
	TRACE("(%p, %s, %p)\n", This, debugstr_w(wzFile), pLoader);
	ILoaderStream_Detach (This);
	This->hFile = CreateFileW (wzFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (This->hFile == INVALID_HANDLE_VALUE) {
		TRACE(": failed\n");
        return E_FAIL;
    }
	/* create IDirectMusicGetLoader */
    (LPDIRECTMUSICLOADER)This->pLoader = pLoader;
    IDirectMusicLoader8_AddRef ((LPDIRECTMUSICLOADER8)This->pLoader);
    strncpyW (This->wzFileName, wzFile, MAX_PATH);
	TRACE(": succeeded\n");
	
    return S_OK;
}

void WINAPI ILoaderStream_Detach (ILoaderStream* This)
{
	if (This->hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(This->hFile);
    }
    This->wzFileName[0] = (L'\0');
}

/*****************************************************************************
 * ILoaderStream IStream:
 */
HRESULT WINAPI ILoaderStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj)
{
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	
	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IStream)) {
		*ppobj = (LPVOID)&This->StreamVtbl;
		ILoaderStream_IStream_AddRef (iface);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicGetLoader)) {
		*ppobj = (LPVOID)&This->GetLoaderVtbl;
		ILoaderStream_IStream_AddRef (iface);		
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI ILoaderStream_IStream_AddRef (LPSTREAM iface)
{
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI ILoaderStream_IStream_Release (LPSTREAM iface)
{
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

HRESULT WINAPI ILoaderStream_IStream_Read (LPSTREAM iface, void* pv, ULONG cb, ULONG* pcbRead)
{
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
    ULONG cbRead;

    if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;
    if (pcbRead == NULL) pcbRead = &cbRead;
    if (!ReadFile (This->hFile, pv, cb, pcbRead, NULL) || *pcbRead != cb) return E_FAIL;

    return S_OK;
}

HRESULT WINAPI ILoaderStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
    LARGE_INTEGER liNewPos;

	if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;

    liNewPos.s.HighPart = dlibMove.s.HighPart;
    liNewPos.s.LowPart = SetFilePointer (This->hFile, dlibMove.s.LowPart, &liNewPos.s.HighPart, dwOrigin);

    if (liNewPos.s.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR) return E_FAIL;
    if (plibNewPosition) plibNewPosition->QuadPart = liNewPos.QuadPart;
    
    return S_OK;
}

HRESULT WINAPI ILoaderStream_IStream_Clone (LPSTREAM iface, IStream** ppstm)
{
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	ILoaderStream* pOther = NULL;
	HRESULT result;

	TRACE("(%p, %p)\n", iface, ppstm);
	result = DMUSIC_CreateLoaderStream ((LPSTREAM*)&pOther);
	if (FAILED(result)) return result;
	if (This->hFile != INVALID_HANDLE_VALUE) {
		ULARGE_INTEGER ullCurrentPosition;
		result = ILoaderStream_Attach (pOther, This->wzFileName, (LPDIRECTMUSICLOADER)This->pLoader);
		if (SUCCEEDED(result)) {
			LARGE_INTEGER liZero;
			liZero.QuadPart = 0;
			result = ILoaderStream_IStream_Seek (iface, liZero, STREAM_SEEK_CUR, &ullCurrentPosition); /* get current position in current stream */
        }
		if (SUCCEEDED(result)) {
			LARGE_INTEGER liNewPosition;
			liNewPosition.QuadPart = ullCurrentPosition.QuadPart;
			result = ILoaderStream_IStream_Seek ((LPSTREAM)pOther, liNewPosition, STREAM_SEEK_SET, &ullCurrentPosition);
		}
		if (FAILED(result)) {
			TRACE(": failed\n");
			ILoaderStream_IStream_Release ((LPSTREAM)pOther);
			return result;
		}
	}
	TRACE(": succeeded\n");
	*ppstm = (IStream*)pOther;
	return S_OK;
}

/* not needed*/
HRESULT WINAPI ILoaderStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_Revert (LPSTREAM iface)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

ICOM_VTABLE(IStream) LoaderStream_Stream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ILoaderStream_IStream_QueryInterface,
	ILoaderStream_IStream_AddRef,
	ILoaderStream_IStream_Release,
	ILoaderStream_IStream_Read,
	ILoaderStream_IStream_Write,
	ILoaderStream_IStream_Seek,
	ILoaderStream_IStream_SetSize,
	ILoaderStream_IStream_CopyTo,
	ILoaderStream_IStream_Commit,
	ILoaderStream_IStream_Revert,
	ILoaderStream_IStream_LockRegion,
	ILoaderStream_IStream_UnlockRegion,
	ILoaderStream_IStream_Stat,
	ILoaderStream_IStream_Clone
};

/*****************************************************************************
 * ILoaderStream IDirectMusicGetLoader:
 */
HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj)
{
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);
	return ILoaderStream_IStream_QueryInterface ((LPSTREAM)&This->StreamVtbl, riid, ppobj);
}

ULONG WINAPI ILoaderStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface)
{
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);
	return ILoaderStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
}

ULONG WINAPI ILoaderStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface)
{
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);
	return ILoaderStream_IStream_Release ((LPSTREAM)&This->StreamVtbl);
}

HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader)
{
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);

	TRACE("(%p, %p)\n", This, ppLoader);
	*ppLoader = (LPDIRECTMUSICLOADER)This->pLoader;

	return S_OK;
}

ICOM_VTABLE(IDirectMusicGetLoader) LoaderStream_GetLoader_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ILoaderStream_IDirectMusicGetLoader_QueryInterface,
	ILoaderStream_IDirectMusicGetLoader_AddRef,
	ILoaderStream_IDirectMusicGetLoader_Release,
	ILoaderStream_IDirectMusicGetLoader_GetLoader
};


HRESULT WINAPI DMUSIC_CreateLoaderStream (LPSTREAM* ppStream)
{
	ILoaderStream *pStream;

	TRACE("(%p)\n", ppStream);

	pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(ILoaderStream));
	if (NULL == pStream) {
		*ppStream = (LPSTREAM)NULL;
		return E_OUTOFMEMORY;
	}
	pStream->StreamVtbl = &LoaderStream_Stream_Vtbl;
	pStream->GetLoaderVtbl = &LoaderStream_GetLoader_Vtbl;
	pStream->ref = 1;
	
	*ppStream = (LPSTREAM)pStream;
	return S_OK;
}
