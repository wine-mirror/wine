/* IDirectMusicSong Implementation
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

/* IDirectMusicSong IUnknown part: */
HRESULT WINAPI IDirectMusicSongImpl_QueryInterface (LPDIRECTMUSICSONG iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicSong)) {
		IDirectMusicSongImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSongImpl_AddRef (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSongImpl_Release (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSong IDirectMusicSong part: */
HRESULT WINAPI IDirectMusicSongImpl_Compose (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p): stub\n", This);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetParam (LPDIRECTMUSICSONG iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetSegment (LPDIRECTMUSICSONG iface, WCHAR* pwzName, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pwzName, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetAudioPathConfig (LPDIRECTMUSICSONG iface, IUnknown** ppAudioPathConfig)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppAudioPathConfig);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_Download (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p): stub\n", This, pAudioPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_Unload (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %p): stub\n", This, pAudioPath);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_EnumSegment (LPDIRECTMUSICSONG iface, DWORD dwIndex, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, ppSegment);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicSong) DirectMusicSong_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSongImpl_QueryInterface,
	IDirectMusicSongImpl_AddRef,
	IDirectMusicSongImpl_Release,
	IDirectMusicSongImpl_Compose,
	IDirectMusicSongImpl_GetParam,
	IDirectMusicSongImpl_GetSegment,
	IDirectMusicSongImpl_GetAudioPathConfig,
	IDirectMusicSongImpl_Download,
	IDirectMusicSongImpl_Unload,
	IDirectMusicSongImpl_EnumSegment
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSong (LPCGUID lpcGUID, LPDIRECTMUSICSONG *ppDMSng, LPUNKNOWN pUnkOuter)
{
	IDirectMusicSongImpl* dmsong;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicSong)) {
		dmsong = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSongImpl));
		if (NULL == dmsong) {
			*ppDMSng = (LPDIRECTMUSICSONG) NULL;
			return E_OUTOFMEMORY;
		}
		dmsong->lpVtbl = &DirectMusicSong_Vtbl;
		dmsong->ref = 1;
		*ppDMSng = (LPDIRECTMUSICSONG) dmsong;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirectMusicSongObject implementation
 */
/* IDirectMusicSongObject IUnknown part: */
HRESULT WINAPI IDirectMusicSongObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSongObject,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicObject)) {
		IDirectMusicSongObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IPersistStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = (LPPERSISTSTREAM)This->pStream;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicSong)) {
		IDirectMusicSong_AddRef ((LPDIRECTMUSICSONG)This->pSong);
		*ppobj = (LPDIRECTMUSICSONG)This->pSong;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSongObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicSongObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSongObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicSongObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSongObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicSongObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicSongObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicSongObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicSongObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicSongObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicSongObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicSongObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSongObject_QueryInterface,
	IDirectMusicSongObject_AddRef,
	IDirectMusicSongObject_Release,
	IDirectMusicSongObject_GetDescriptor,
	IDirectMusicSongObject_SetDescriptor,
	IDirectMusicSongObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSongObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicSongObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSongObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicSongObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicSongObjectStream));
		obj->pStream->lpVtbl = &DirectMusicSongObjectStream_Vtbl;
		obj->pStream->ref = 1;	
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicSong */
		DMUSIC_CreateDirectMusicSong (&IID_IDirectMusicSong, (LPDIRECTMUSICSONG*)&obj->pSong, NULL);
		obj->pSong->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
	
/*****************************************************************************
 * IDirectMusicSongObjectStream implementation
 */
/* IDirectMusicSongObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicSongObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSongObjectStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicSongObjectStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSongObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicSongObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSongObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicSongObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSongObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicSongObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicSongObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicSongObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSongObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicSongObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicSongObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicSongObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSongObjectStream_QueryInterface,
	IDirectMusicSongObjectStream_AddRef,
	IDirectMusicSongObjectStream_Release,
	IDirectMusicSongObjectStream_GetClassID,
	IDirectMusicSongObjectStream_IsDirty,
	IDirectMusicSongObjectStream_Load,
	IDirectMusicSongObjectStream_Save,
	IDirectMusicSongObjectStream_GetSizeMax
};
