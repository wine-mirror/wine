/* IDirectMusicChordMap Implementation
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

#include "dmcompos_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

/* IDirectMusicChordMap IUnknown parts follow: */
HRESULT WINAPI IDirectMusicChordMapImpl_QueryInterface (LPDIRECTMUSICCHORDMAP iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordMapImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicChordMap)) {
		IDirectMusicChordMapImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicChordMapImpl_AddRef (LPDIRECTMUSICCHORDMAP iface)
{
	ICOM_THIS(IDirectMusicChordMapImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicChordMapImpl_Release (LPDIRECTMUSICCHORDMAP iface)
{
	ICOM_THIS(IDirectMusicChordMapImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicChordMap Interface follow: */
HRESULT WINAPI IDirectMusicChordMapImpl_GetScale (LPDIRECTMUSICCHORDMAP iface, DWORD* pdwScale)
{
	ICOM_THIS(IDirectMusicChordMapImpl,iface);

	TRACE("(%p, %p)\n", This, pdwScale);
	*pdwScale = This->dwScale;

	return S_OK;
}

ICOM_VTABLE(IDirectMusicChordMap) DirectMusicChordMap_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordMapImpl_QueryInterface,
	IDirectMusicChordMapImpl_AddRef,
	IDirectMusicChordMapImpl_Release,
	IDirectMusicChordMapImpl_GetScale
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicChordMap (LPCGUID lpcGUID, LPDIRECTMUSICCHORDMAP* ppDMCM, LPUNKNOWN pUnkOuter)
{
	IDirectMusicChordMapImpl* dmchordmap;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicChordMap)) {
		dmchordmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordMapImpl));
		if (NULL == dmchordmap) {
			*ppDMCM = (LPDIRECTMUSICCHORDMAP) NULL;
			return E_OUTOFMEMORY;
		}
		dmchordmap->lpVtbl = &DirectMusicChordMap_Vtbl;
		dmchordmap->ref = 1;
		*ppDMCM = (LPDIRECTMUSICCHORDMAP) dmchordmap;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}

/*****************************************************************************
 * IDirectMusicChordMapObject implementation
 */
/* IDirectMusicChordMapObject IUnknown part: */
HRESULT WINAPI IDirectMusicChordMapObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordMapObject,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicObject)) {
		IDirectMusicChordMapObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IPersistStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = (LPPERSISTSTREAM)This->pStream;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicChordMap)) {
		IDirectMusicChordMap_AddRef ((LPDIRECTMUSICCHORDMAP)This->pChordMap);
		*ppobj = (LPDIRECTMUSICCHORDMAP)This->pChordMap;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicChordMapObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicChordMapObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicChordMapObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicChordMapObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicChordMapObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicChordMapObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicChordMapObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordMapObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicChordMapObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicChordMapObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicChordMapObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicChordMapObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordMapObject_QueryInterface,
	IDirectMusicChordMapObject_AddRef,
	IDirectMusicChordMapObject_Release,
	IDirectMusicChordMapObject_GetDescriptor,
	IDirectMusicChordMapObject_SetDescriptor,
	IDirectMusicChordMapObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicChordMapObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicChordMapObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordMapObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicChordMapObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicChordMapObjectStream));
		obj->pStream->lpVtbl = &DirectMusicChordMapObjectStream_Vtbl;
		obj->pStream->ref = 1;	
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusiChordMap */
		DMUSIC_CreateDirectMusicChordMap (&IID_IDirectMusicChordMap, (LPDIRECTMUSICCHORDMAP*)&obj->pChordMap, NULL);
		obj->pChordMap->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
	
/*****************************************************************************
 * IDirectMusicChordMapObjectStream implementation
 */
/* IDirectMusicChordMapObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicChordMapObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordMapObjectStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicChordMapObjectStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicChordMapObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicChordMapObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicChordMapObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicChordMapObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicChordMapObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicChordMapObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicChordMapObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicChordMapObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordMapObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicChordMapObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicChordMapObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicChordMapObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordMapObjectStream_QueryInterface,
	IDirectMusicChordMapObjectStream_AddRef,
	IDirectMusicChordMapObjectStream_Release,
	IDirectMusicChordMapObjectStream_GetClassID,
	IDirectMusicChordMapObjectStream_IsDirty,
	IDirectMusicChordMapObjectStream_Load,
	IDirectMusicChordMapObjectStream_Save,
	IDirectMusicChordMapObjectStream_GetSizeMax
};
