/* IDirectMusicLoaderFileStream
 * IDirectMusicLoaderResourceStream
 * IDirectMusicLoaderGenericStream
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


/* SIDE NOTES:
 * After extensive testing and structure dumping I came to a conclusion that
 * DirectMusic as in present state implements three types of streams:
 *  1. IDirectMusicLoaderFileStream: stream that was most obvious, since 
 *     it's used for loading from files; it is sort of wrapper around 
 *     CreateFile, ReadFile, WriteFile and SetFilePointer and it supports 
 *     both read and write
 *  2. IDirectMusicLoaderResourceStream: a stream that had to exist, since 
 *     according to MSDN, IDirectMusicLoader supports loading from resource 
 *     as well; in this case, data is represented as a big chunk of bytes, 
 *     from which we "read" (copy) data and keep the trace of our position; 
 *      it supports read only
 *  3. IDirectMusicLoaderGenericStream: this one was the most problematic, 
 *     since I thought it was URL-related; besides, there's no obvious need 
 *     for it, since input streams can simply be cloned, lest loading from 
 *     stream is requested; but if one really thinks about it, input stream 
 *     could be none of 1. or 2.; in this case, a wrapper that offers
 *     IDirectMusicGetLoader interface would be nice, and this is what this 
 *     stream is; as such, all functions are supported, as long as underlying 
 *     ("low-level") stream supports them
 *
 * - Rok Mandeljc; 24. April, 2004
*/

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);
WINE_DECLARE_DEBUG_CHANNEL(dmfileraw);

static ULONG WINAPI IDirectMusicLoaderFileStream_IStream_AddRef (LPSTREAM iface);
static ULONG WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
static ULONG WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
static ULONG WINAPI IDirectMusicLoaderResourceStream_IStream_AddRef (LPSTREAM iface);
static ULONG WINAPI IDirectMusicLoaderGenericStream_IStream_AddRef (LPSTREAM iface);
static ULONG WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);


/*****************************************************************************
 * IDirectMusicLoaderFileStream implementation
 */
/* Custom : */

static void IDirectMusicLoaderFileStream_Detach (LPSTREAM iface) {
    ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
    TRACE("(%p)\n", This);
    if (This->hFile != INVALID_HANDLE_VALUE) CloseHandle(This->hFile);
    This->wzFileName[0] = '\0';
}

HRESULT WINAPI IDirectMusicLoaderFileStream_Attach (LPSTREAM iface, LPCWSTR wzFile, LPDIRECTMUSICLOADER8 pLoader) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
    TRACE("(%p, %s, %p)\n", This, debugstr_w(wzFile), pLoader);
    IDirectMusicLoaderFileStream_Detach (iface);
    This->hFile = CreateFileW (wzFile, (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (This->hFile == INVALID_HANDLE_VALUE) {
        WARN(": failed\n");
        return DMUS_E_LOADER_FAILEDOPEN;
    }
    /* create IDirectMusicGetLoader */
    This->pLoader = pLoader;
    lstrcpynW (This->wzFileName, wzFile, MAX_PATH);
    TRACE(": succeeded\n");
    return S_OK;
}


/* IUnknown/IStream part: */
static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
	
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown) ||
		IsEqualIID (riid, &IID_IStream)) {
		*ppobj = &This->StreamVtbl;
		IDirectMusicLoaderFileStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicGetLoader)) {
		*ppobj = &This->GetLoaderVtbl;
		IDirectMusicLoaderFileStream_IDirectMusicGetLoader_AddRef ((LPDIRECTMUSICGETLOADER)&This->GetLoaderVtbl);		
		return S_OK;
	}

	WARN(": not found\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicLoaderFileStream_IStream_AddRef (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
	TRACE("(%p): AddRef from %ld\n", This, This->dwRef);
	return InterlockedIncrement (&This->dwRef);
}

static ULONG WINAPI IDirectMusicLoaderFileStream_IStream_Release (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
	
	DWORD dwRef = InterlockedDecrement (&This->dwRef);
	TRACE("(%p): ReleaseRef to %ld\n", This, dwRef);
	if (dwRef == 0) {
		if (This->hFile)
			IDirectMusicLoaderFileStream_Detach (iface);
		HeapFree (GetProcessHeap(), 0, This);
	}
	
	return dwRef;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Read (LPSTREAM iface, void* pv, ULONG cb, ULONG* pcbRead) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
    ULONG cbRead;
	
	TRACE_(dmfileraw)("(%p, %p, %#lx, %p)\n", This, pv, cb, pcbRead);
    if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;
    if (pcbRead == NULL) pcbRead = &cbRead;
    if (!ReadFile (This->hFile, pv, cb, pcbRead, NULL) || *pcbRead != cb) return E_FAIL;
	
	TRACE_(dmfileraw)(": data (size = %#lx): %s\n", *pcbRead, debugstr_an(pv, *pcbRead));
    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
    LARGE_INTEGER liNewPos;
	
    TRACE_(dmfileraw)("(%p, %s, %s, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), resolve_STREAM_SEEK(dwOrigin), plibNewPosition);

    if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;

    liNewPos.HighPart = dlibMove.HighPart;
    liNewPos.LowPart = SetFilePointer (This->hFile, dlibMove.LowPart, &liNewPos.HighPart, dwOrigin);

    if (liNewPos.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) return E_FAIL;
    if (plibNewPosition) plibNewPosition->QuadPart = liNewPos.QuadPart;
    
    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Clone (LPSTREAM iface, IStream** ppstm) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
	LPSTREAM pOther = NULL;
	HRESULT result;

	TRACE("(%p, %p)\n", iface, ppstm);
	result = DMUSIC_CreateDirectMusicLoaderFileStream ((LPVOID*)&pOther);
	if (FAILED(result)) return result;
	if (This->hFile != INVALID_HANDLE_VALUE) {
		ULARGE_INTEGER ullCurrentPosition;
		result = IDirectMusicLoaderFileStream_Attach (pOther, This->wzFileName, This->pLoader);
		if (SUCCEEDED(result)) {
			LARGE_INTEGER liZero;
			liZero.QuadPart = 0;
			result = IDirectMusicLoaderFileStream_IStream_Seek (iface, liZero, STREAM_SEEK_CUR, &ullCurrentPosition); /* get current position in current stream */
        }
		if (SUCCEEDED(result)) {
			LARGE_INTEGER liNewPosition;
			liNewPosition.QuadPart = ullCurrentPosition.QuadPart;
			result = IDirectMusicLoaderFileStream_IStream_Seek (pOther, liNewPosition, STREAM_SEEK_SET, &ullCurrentPosition);
		}
		if (FAILED(result)) {
			TRACE(": failed\n");
			IDirectMusicLoaderFileStream_IStream_Release (pOther);
			return result;
		}
	}
	TRACE(": succeeded\n");
	*ppstm = pOther;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, StreamVtbl, iface);
    ULONG cbWrite;
	
	TRACE_(dmfileraw)("(%p, %p, %#lx, %p)\n", This, pv, cb, pcbWritten);
    if (This->hFile == INVALID_HANDLE_VALUE) return E_FAIL;
    if (pcbWritten == NULL) pcbWritten = &cbWrite;
    if (!WriteFile (This->hFile, pv, cb, pcbWritten, NULL) || *pcbWritten != cb) return E_FAIL;
	
	TRACE_(dmfileraw)(": data (size = %#lx): %s\n", *pcbWritten, debugstr_an(pv, *pcbWritten));
    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Revert (LPSTREAM iface) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static const IStreamVtbl DirectMusicLoaderFileStream_Stream_Vtbl = {
	IDirectMusicLoaderFileStream_IStream_QueryInterface,
	IDirectMusicLoaderFileStream_IStream_AddRef,
	IDirectMusicLoaderFileStream_IStream_Release,
	IDirectMusicLoaderFileStream_IStream_Read,
	IDirectMusicLoaderFileStream_IStream_Write,
	IDirectMusicLoaderFileStream_IStream_Seek,
	IDirectMusicLoaderFileStream_IStream_SetSize,
	IDirectMusicLoaderFileStream_IStream_CopyTo,
	IDirectMusicLoaderFileStream_IStream_Commit,
	IDirectMusicLoaderFileStream_IStream_Revert,
	IDirectMusicLoaderFileStream_IStream_LockRegion,
	IDirectMusicLoaderFileStream_IStream_UnlockRegion,
	IDirectMusicLoaderFileStream_IStream_Stat,
	IDirectMusicLoaderFileStream_IStream_Clone
};

/* IDirectMusicGetLoader part: */
static HRESULT WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderFileStream_IStream_QueryInterface ((LPSTREAM)&This->StreamVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderFileStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
}

static ULONG WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderFileStream_IStream_Release ((LPSTREAM)&This->StreamVtbl);
}

static HRESULT WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader) {
	ICOM_THIS_MULTI(IDirectMusicLoaderFileStream, GetLoaderVtbl, iface);

	TRACE("(%p, %p)\n", This, ppLoader);
	*ppLoader = (LPDIRECTMUSICLOADER)This->pLoader;
	IDirectMusicLoader8_AddRef ((LPDIRECTMUSICLOADER8)*ppLoader);
	
	return S_OK;
}

static const IDirectMusicGetLoaderVtbl DirectMusicLoaderFileStream_GetLoader_Vtbl = {
	IDirectMusicLoaderFileStream_IDirectMusicGetLoader_QueryInterface,
	IDirectMusicLoaderFileStream_IDirectMusicGetLoader_AddRef,
	IDirectMusicLoaderFileStream_IDirectMusicGetLoader_Release,
	IDirectMusicLoaderFileStream_IDirectMusicGetLoader_GetLoader
};

HRESULT DMUSIC_CreateDirectMusicLoaderFileStream (void** ppobj) {
	IDirectMusicLoaderFileStream *obj;

	TRACE("(%p)\n", ppobj);
	obj = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoaderFileStream));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->StreamVtbl = &DirectMusicLoaderFileStream_Stream_Vtbl;
	obj->GetLoaderVtbl = &DirectMusicLoaderFileStream_GetLoader_Vtbl;
	obj->dwRef = 0; /* will be inited with QueryInterface */

	return IDirectMusicLoaderFileStream_IStream_QueryInterface ((LPSTREAM)&obj->StreamVtbl, &IID_IStream, ppobj);
}


/*****************************************************************************
 * IDirectMusicLoaderResourceStream implementation
 */
/* Custom : */

static void IDirectMusicLoaderResourceStream_Detach (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
	TRACE("(%p)\n", This);

	This->pbMemData = NULL;
	This->llMemLength = 0;
}

HRESULT WINAPI IDirectMusicLoaderResourceStream_Attach (LPSTREAM iface, LPBYTE pbMemData, LONGLONG llMemLength, LONGLONG llPos, LPDIRECTMUSICLOADER8 pLoader) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
    
	TRACE("(%p, %p, %s, %s, %p)\n", This, pbMemData, wine_dbgstr_longlong(llMemLength), wine_dbgstr_longlong(llPos), pLoader);
	if (!pbMemData || !llMemLength) {
		WARN(": invalid pbMemData or llMemLength\n");
		return E_FAIL;
	}
	IDirectMusicLoaderResourceStream_Detach (iface);
	This->pbMemData = pbMemData;
	This->llMemLength = llMemLength;
	This->llPos = llPos;
	This->pLoader = pLoader;
	
    return S_OK;
}


/* IUnknown/IStream part: */
static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
	
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown) ||
		IsEqualIID (riid, &IID_IStream)) {
		*ppobj = &This->StreamVtbl;
		IDirectMusicLoaderResourceStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicGetLoader)) {
		*ppobj = &This->GetLoaderVtbl;
		IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_AddRef ((LPDIRECTMUSICGETLOADER)&This->GetLoaderVtbl);		
		return S_OK;
	}

	WARN(": not found\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicLoaderResourceStream_IStream_AddRef (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
	TRACE("(%p): AddRef from %ld\n", This, This->dwRef);
	return InterlockedIncrement (&This->dwRef);
}

static ULONG WINAPI IDirectMusicLoaderResourceStream_IStream_Release (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
	
	DWORD dwRef = InterlockedDecrement (&This->dwRef);
	TRACE("(%p): ReleaseRef to %ld\n", This, dwRef);
	if (dwRef == 0) {
		IDirectMusicLoaderResourceStream_Detach (iface);
		HeapFree (GetProcessHeap(), 0, This);
	}
	
	return dwRef;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Read (LPSTREAM iface, void* pv, ULONG cb, ULONG* pcbRead) {
	LPBYTE pByte;
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
	
	TRACE_(dmfileraw)("(%p, %p, %#lx, %p)\n", This, pv, cb, pcbRead);
	if ((This->llPos + cb) > This->llMemLength) {
		WARN_(dmfileraw)(": requested size out of range\n");
		return E_FAIL;
	}
	
	pByte = &This->pbMemData[This->llPos];
	memcpy (pv, pByte, cb);
	This->llPos += cb; /* move pointer */
	/* FIXME: error checking would be nice */
	if (pcbRead) *pcbRead = cb;
	
	TRACE_(dmfileraw)(": data (size = %#lx): %s\n", cb, debugstr_an(pv, cb));
    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);	
	TRACE_(dmfileraw)("(%p, %s, %s, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), resolve_STREAM_SEEK(dwOrigin), plibNewPosition);
	
	switch (dwOrigin) {
		case STREAM_SEEK_CUR: {
			if ((This->llPos + dlibMove.QuadPart) > This->llMemLength) {
				WARN_(dmfileraw)(": requested offset out of range\n");
				return E_FAIL;
			}
			break;
		}
		case STREAM_SEEK_SET: {
			if (dlibMove.QuadPart > This->llMemLength) {
				WARN_(dmfileraw)(": requested offset out of range\n");
				return E_FAIL;
			}
			/* set to the beginning of the stream */
			This->llPos = 0;
			break;
		}
		case STREAM_SEEK_END: {
			/* TODO: check if this is true... I do think offset should be negative in this case */
			if (dlibMove.QuadPart > 0) {
				WARN_(dmfileraw)(": requested offset out of range\n");
				return E_FAIL;
			}
			/* set to the end of the stream */
			This->llPos = This->llMemLength;
			break;
		}
		default: {
			ERR_(dmfileraw)(": invalid dwOrigin\n");
			return E_FAIL;
		}
	}
	/* now simply add */
	This->llPos += dlibMove.QuadPart;

	if (plibNewPosition) plibNewPosition->QuadPart = This->llPos;
    	
    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Clone (LPSTREAM iface, IStream** ppstm) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, StreamVtbl, iface);
	LPSTREAM pOther = NULL;
	HRESULT result;

	TRACE("(%p, %p)\n", iface, ppstm);
	result = DMUSIC_CreateDirectMusicLoaderResourceStream ((LPVOID*)&pOther);
	if (FAILED(result)) return result;
	
	IDirectMusicLoaderResourceStream_Attach (pOther, This->pbMemData, This->llMemLength, This->llPos, This->pLoader);

	TRACE(": succeeded\n");
	*ppstm = pOther;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Revert (LPSTREAM iface) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag) {
	ERR(": should not be needed\n");
    return E_NOTIMPL;
}

static const IStreamVtbl DirectMusicLoaderResourceStream_Stream_Vtbl = {
	IDirectMusicLoaderResourceStream_IStream_QueryInterface,
	IDirectMusicLoaderResourceStream_IStream_AddRef,
	IDirectMusicLoaderResourceStream_IStream_Release,
	IDirectMusicLoaderResourceStream_IStream_Read,
	IDirectMusicLoaderResourceStream_IStream_Write,
	IDirectMusicLoaderResourceStream_IStream_Seek,
	IDirectMusicLoaderResourceStream_IStream_SetSize,
	IDirectMusicLoaderResourceStream_IStream_CopyTo,
	IDirectMusicLoaderResourceStream_IStream_Commit,
	IDirectMusicLoaderResourceStream_IStream_Revert,
	IDirectMusicLoaderResourceStream_IStream_LockRegion,
	IDirectMusicLoaderResourceStream_IStream_UnlockRegion,
	IDirectMusicLoaderResourceStream_IStream_Stat,
	IDirectMusicLoaderResourceStream_IStream_Clone
};

/* IDirectMusicGetLoader part: */
static HRESULT WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderResourceStream_IStream_QueryInterface ((LPSTREAM)&This->StreamVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderResourceStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
}

static ULONG WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderResourceStream_IStream_Release ((LPSTREAM)&This->StreamVtbl);
}

static HRESULT WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader) {
	ICOM_THIS_MULTI(IDirectMusicLoaderResourceStream, GetLoaderVtbl, iface);

	TRACE("(%p, %p)\n", This, ppLoader);
	*ppLoader = (LPDIRECTMUSICLOADER)This->pLoader;
	IDirectMusicLoader8_AddRef ((LPDIRECTMUSICLOADER8)*ppLoader);
	
	return S_OK;
}

static const IDirectMusicGetLoaderVtbl DirectMusicLoaderResourceStream_GetLoader_Vtbl = {
	IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_QueryInterface,
	IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_AddRef,
	IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_Release,
	IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_GetLoader
};

HRESULT DMUSIC_CreateDirectMusicLoaderResourceStream (void** ppobj) {
	IDirectMusicLoaderResourceStream *obj;

	TRACE("(%p)\n", ppobj);
	obj = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoaderResourceStream));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->StreamVtbl = &DirectMusicLoaderResourceStream_Stream_Vtbl;
	obj->GetLoaderVtbl = &DirectMusicLoaderResourceStream_GetLoader_Vtbl;
	obj->dwRef = 0; /* will be inited with QueryInterface */

	return IDirectMusicLoaderResourceStream_IStream_QueryInterface ((LPSTREAM)&obj->StreamVtbl, &IID_IStream, ppobj);
}


/*****************************************************************************
 * IDirectMusicLoaderGenericStream implementation
 */
/* Custom : */

static void IDirectMusicLoaderGenericStream_Detach (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);

	if (This->pStream)
		IStream_Release (This->pStream);
	This->pStream = NULL;
}

HRESULT WINAPI IDirectMusicLoaderGenericStream_Attach (LPSTREAM iface, LPSTREAM pStream, LPDIRECTMUSICLOADER8 pLoader) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);
    
	TRACE("(%p, %p, %p)\n", This, pStream, pLoader);
	if (!pStream) {
		WARN(": invalid pStream\n");
		return E_FAIL;
	}
	if (!pLoader) {
		WARN(": invalid pLoader\n");
		return E_FAIL;
	}	
	
	IDirectMusicLoaderGenericStream_Detach (iface);
	IStream_Clone (pStream, &This->pStream);
	This->pLoader = pLoader;
	
    return S_OK;
}


/* IUnknown/IStream part: */
static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);
	
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown) ||
		IsEqualIID (riid, &IID_IStream)) {
		*ppobj = &This->StreamVtbl;
		IDirectMusicLoaderGenericStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicGetLoader)) {
		*ppobj = &This->GetLoaderVtbl;
		IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_AddRef ((LPDIRECTMUSICGETLOADER)&This->GetLoaderVtbl);		
		return S_OK;
	}

	WARN(": not found\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicLoaderGenericStream_IStream_AddRef (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);
	TRACE("(%p): AddRef from %ld\n", This, This->dwRef);
	return InterlockedIncrement (&This->dwRef);
}

static ULONG WINAPI IDirectMusicLoaderGenericStream_IStream_Release (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);
	
	DWORD dwRef = InterlockedDecrement (&This->dwRef);
	TRACE("(%p): ReleaseRef to %ld\n", This, dwRef);
	if (dwRef == 0) {
		IDirectMusicLoaderGenericStream_Detach (iface);
		HeapFree (GetProcessHeap(), 0, This);
	}
	
	return dwRef;
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Read (LPSTREAM iface, void* pv, ULONG cb, ULONG* pcbRead) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);

	TRACE_(dmfileraw)("(%p, %p, %#lx, %p): redirecting to low-level stream\n", This, pv, cb, pcbRead);
	if (!This->pStream)
		return E_FAIL;

	return IStream_Read (This->pStream, pv, cb, pcbRead);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE_(dmfileraw)("(%p, %s, %s, %p): redirecting to low-level stream\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), resolve_STREAM_SEEK(dwOrigin), plibNewPosition);
	if (!This->pStream)
		return E_FAIL;

	return IStream_Seek (This->pStream, dlibMove, dwOrigin, plibNewPosition);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Clone (LPSTREAM iface, IStream** ppstm) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);
	LPSTREAM pOther = NULL;
	LPSTREAM pLowLevel = NULL;
	HRESULT result;

	TRACE("(%p, %p)\n", iface, ppstm);
	result = DMUSIC_CreateDirectMusicLoaderGenericStream ((LPVOID*)&pOther);
	if (FAILED(result)) return result;

	if (FAILED(IStream_Clone (This->pStream, &pLowLevel))) {
		IStream_Release(pOther);
		return E_FAIL;
	}
	IDirectMusicLoaderGenericStream_Attach (pOther, pLowLevel, This->pLoader);

	TRACE(": succeeded\n");
	*ppstm = pOther;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE_(dmfileraw)("(%p, %p, %#lx, %p): redirecting to low-level stream\n", This, pv, cb, pcbWritten);
	if (!This->pStream)
		return E_FAIL;

	return IStream_Write (This->pStream, pv, cb, pcbWritten);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p, %s): redirecting to low-level stream\n", This, wine_dbgstr_longlong(libNewSize.QuadPart));
	if (!This->pStream)
		return E_FAIL;

	return IStream_SetSize (This->pStream, libNewSize);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p, %p, %s, %p, %p): redirecting to low-level stream\n", This, pstm, wine_dbgstr_longlong(cb.QuadPart), pcbRead, pcbWritten);
	if (!This->pStream)
		return E_FAIL;

	return IStream_CopyTo (This->pStream, pstm, cb, pcbRead, pcbWritten);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p, %#lx): redirecting to low-level stream\n", This, grfCommitFlags);
	if (!This->pStream)
		return E_FAIL;

	return IStream_Commit (This->pStream, grfCommitFlags);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Revert (LPSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p): redirecting to low-level stream\n", This);
	if (!This->pStream)
		return E_FAIL;

	return IStream_Revert (This->pStream);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p, %s, %s, %#lx): redirecting to low-level stream\n", This, wine_dbgstr_longlong(libOffset.QuadPart), wine_dbgstr_longlong(cb.QuadPart), dwLockType);
	if (!This->pStream)
		return E_FAIL;

	return IStream_LockRegion (This->pStream, libOffset, cb, dwLockType);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p, %s, %s, %#lx): redirecting to low-level stream\n", This, wine_dbgstr_longlong(libOffset.QuadPart), wine_dbgstr_longlong(cb.QuadPart), dwLockType);
	if (!This->pStream)
		return E_FAIL;

	return IStream_UnlockRegion (This->pStream, libOffset, cb, dwLockType);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, StreamVtbl, iface);	
	TRACE("(%p, %p, %#lx): redirecting to low-level stream\n", This, pstatstg, grfStatFlag);
	if (!This->pStream)
		return E_FAIL;

	return IStream_Stat (This->pStream, pstatstg, grfStatFlag);
}

static const IStreamVtbl DirectMusicLoaderGenericStream_Stream_Vtbl = {
	IDirectMusicLoaderGenericStream_IStream_QueryInterface,
	IDirectMusicLoaderGenericStream_IStream_AddRef,
	IDirectMusicLoaderGenericStream_IStream_Release,
	IDirectMusicLoaderGenericStream_IStream_Read,
	IDirectMusicLoaderGenericStream_IStream_Write,
	IDirectMusicLoaderGenericStream_IStream_Seek,
	IDirectMusicLoaderGenericStream_IStream_SetSize,
	IDirectMusicLoaderGenericStream_IStream_CopyTo,
	IDirectMusicLoaderGenericStream_IStream_Commit,
	IDirectMusicLoaderGenericStream_IStream_Revert,
	IDirectMusicLoaderGenericStream_IStream_LockRegion,
	IDirectMusicLoaderGenericStream_IStream_UnlockRegion,
	IDirectMusicLoaderGenericStream_IStream_Stat,
	IDirectMusicLoaderGenericStream_IStream_Clone
};

/* IDirectMusicGetLoader part: */
static HRESULT WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderGenericStream_IStream_QueryInterface ((LPSTREAM)&This->StreamVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderGenericStream_IStream_AddRef ((LPSTREAM)&This->StreamVtbl);
}

static ULONG WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, GetLoaderVtbl, iface);
	return IDirectMusicLoaderGenericStream_IStream_Release ((LPSTREAM)&This->StreamVtbl);
}

static HRESULT WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader) {
	ICOM_THIS_MULTI(IDirectMusicLoaderGenericStream, GetLoaderVtbl, iface);

	TRACE("(%p, %p)\n", This, ppLoader);
	*ppLoader = (LPDIRECTMUSICLOADER)This->pLoader;
	IDirectMusicLoader8_AddRef ((LPDIRECTMUSICLOADER8)*ppLoader);
	
	return S_OK;
}

static const IDirectMusicGetLoaderVtbl DirectMusicLoaderGenericStream_GetLoader_Vtbl = {
	IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_QueryInterface,
	IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_AddRef,
	IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_Release,
	IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_GetLoader
};

HRESULT DMUSIC_CreateDirectMusicLoaderGenericStream (void** ppobj) {
	IDirectMusicLoaderGenericStream *obj;

	TRACE("(%p)\n", ppobj);
	obj = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoaderGenericStream));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->StreamVtbl = &DirectMusicLoaderGenericStream_Stream_Vtbl;
	obj->GetLoaderVtbl = &DirectMusicLoaderGenericStream_GetLoader_Vtbl;
	obj->dwRef = 0; /* will be inited with QueryInterface */

	return IDirectMusicLoaderGenericStream_IStream_QueryInterface ((LPSTREAM)&obj->StreamVtbl, &IID_IStream, ppobj);
}
