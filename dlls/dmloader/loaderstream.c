/* ILoaderStream Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);
WINE_DECLARE_DEBUG_CHANNEL(dmfileraw);

/*****************************************************************************
 * Custom functions:
 */
HRESULT WINAPI ILoaderStream_Attach (LPSTREAM iface, LPCWSTR wzFile, IDirectMusicLoader *pLoader) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
    TRACE("(%p, %s, %p)\n", This, debugstr_w(wzFile), pLoader);
    ILoaderStream_Detach (iface);
    This->hFile = CreateFileW (wzFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (This->hFile == INVALID_HANDLE_VALUE) {
        TRACE(": failed\n");
        return DMUS_E_LOADER_FAILEDOPEN;
    }
    /* create IDirectMusicGetLoader */
    (LPDIRECTMUSICLOADER) This->pLoader = pLoader;
    strncpyW (This->wzFileName, wzFile, MAX_PATH);
    TRACE(": succeeded\n");
    return S_OK;
}

void WINAPI ILoaderStream_Detach (LPSTREAM iface) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	if (This->hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(This->hFile);
    }
    This->wzFileName[0] = (L'\0');
}

/*****************************************************************************
 * ILoaderStream implementation
 */
/* ILoaderStream IUnknown part: */
HRESULT WINAPI ILoaderStream_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(ILoaderStream, UnknownVtbl, iface);
	
	TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPVOID)&This->UnknownVtbl;
		ILoaderStream_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IStream)) {
		*ppobj = (LPVOID)&This->StreamVtbl;
		ILoaderStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicGetLoader)) {
		*ppobj = (LPVOID)&This->GetLoaderVtbl;
		ILoaderStream_IDirectMusicGetLoader_AddRef ((LPDIRECTMUSICGETLOADER)&This->GetLoaderVtbl);		
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI ILoaderStream_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(ILoaderStream, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI ILoaderStream_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(ILoaderStream, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) LoaderStream_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ILoaderStream_IUnknown_QueryInterface,
	ILoaderStream_IUnknown_AddRef,
	ILoaderStream_IUnknown_Release
};

/* ILoaderStream IStream part: */
HRESULT WINAPI ILoaderStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	return ILoaderStream_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI ILoaderStream_IStream_AddRef (LPSTREAM iface) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	return ILoaderStream_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI ILoaderStream_IStream_Release (LPSTREAM iface) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	return ILoaderStream_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI ILoaderStream_IStream_Read (LPSTREAM iface, void* pv, ULONG cb, ULONG* pcbRead) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
    ULONG cbRead;
	TRACE_(dmfileraw)("(%p, %p, 0x%04lx, %p)\n", This, pv, cb, pcbRead);
    if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;
    if (pcbRead == NULL) pcbRead = &cbRead;
    if (!ReadFile (This->hFile, pv, cb, pcbRead, NULL) || *pcbRead != cb) return E_FAIL;
	TRACE_(dmfileraw)(": data (size = 0x%04lx): '%s'\n", *pcbRead, debugstr_an(pv, *pcbRead));
    return S_OK;
}

HRESULT WINAPI ILoaderStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
    LARGE_INTEGER liNewPos;
	
	TRACE_(dmfileraw)("(%p, 0x%04llx, %s, %p)\n", This, dlibMove.QuadPart, resolve_STREAM_SEEK(dwOrigin), plibNewPosition);

	if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;

    liNewPos.s.HighPart = dlibMove.s.HighPart;
    liNewPos.s.LowPart = SetFilePointer (This->hFile, dlibMove.s.LowPart, &liNewPos.s.HighPart, dwOrigin);

    if (liNewPos.s.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) return E_FAIL;
    if (plibNewPosition) plibNewPosition->QuadPart = liNewPos.QuadPart;
    
    return S_OK;
}

HRESULT WINAPI ILoaderStream_IStream_Clone (LPSTREAM iface, IStream** ppstm) {
	ICOM_THIS_MULTI(ILoaderStream, StreamVtbl, iface);
	LPSTREAM pOther = NULL;
	HRESULT result;

	TRACE("(%p, %p)\n", iface, ppstm);
	result = DMUSIC_CreateLoaderStream ((LPVOID*)&pOther);
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

HRESULT WINAPI ILoaderStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten) {
	ERR(": should not be needed\n");
	return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_Revert (LPSTREAM iface) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

HRESULT WINAPI ILoaderStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

ICOM_VTABLE(IStream) LoaderStream_Stream_Vtbl = {
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
 * ILoaderStream IDirectMusicGetLoader part:
 */
HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);
	return ILoaderStream_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI ILoaderStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);
	return ILoaderStream_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI ILoaderStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);
	return ILoaderStream_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader) {
	ICOM_THIS_MULTI(ILoaderStream, GetLoaderVtbl, iface);

	TRACE("(%p, %p)\n", This, ppLoader);
	*ppLoader = (LPDIRECTMUSICLOADER)This->pLoader;
	IDirectMusicLoader8_AddRef ((LPDIRECTMUSICLOADER8)*ppLoader);
	
	return S_OK;
}

ICOM_VTABLE(IDirectMusicGetLoader) LoaderStream_GetLoader_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ILoaderStream_IDirectMusicGetLoader_QueryInterface,
	ILoaderStream_IDirectMusicGetLoader_AddRef,
	ILoaderStream_IDirectMusicGetLoader_Release,
	ILoaderStream_IDirectMusicGetLoader_GetLoader
};

HRESULT WINAPI DMUSIC_CreateLoaderStream (LPVOID* ppobj) {
	ILoaderStream *pStream;

	TRACE("(%p)\n", ppobj);
	pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(ILoaderStream));
	if (NULL == pStream) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	pStream->UnknownVtbl = &LoaderStream_Unknown_Vtbl;
	pStream->StreamVtbl = &LoaderStream_Stream_Vtbl;
	pStream->GetLoaderVtbl = &LoaderStream_GetLoader_Vtbl;
	pStream->ref = 0; /* will be inited with QueryInterface */

	return ILoaderStream_IUnknown_QueryInterface ((LPUNKNOWN)&pStream->UnknownVtbl, &IID_IStream, ppobj);
}
