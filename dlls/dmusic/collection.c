/* IDirectMusicCollection Implementation
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicCollection IUnknown parts follow: */
HRESULT WINAPI IDirectMusicCollectionImpl_QueryInterface (LPDIRECTMUSICCOLLECTION iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicCollection)) {
		IDirectMusicCollectionImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicCollectionImpl_AddRef (LPDIRECTMUSICCOLLECTION iface)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicCollectionImpl_Release (LPDIRECTMUSICCOLLECTION iface)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicCollection Interface follow: */
HRESULT WINAPI IDirectMusicCollectionImpl_GetInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwPatch, IDirectMusicInstrument** ppInstrument)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);
	int i;
	
	TRACE("(%p, %ld, %p)\n", This, dwPatch, ppInstrument);
	for (i = 0; i < This->nrofinstruments; i++) {
		if (This->ppInstruments[i]->dwPatch == dwPatch) {
			*ppInstrument = (LPDIRECTMUSICINSTRUMENT)This->ppInstruments[i];
			return S_OK;
		}
	}
	
	return DMUS_E_INVALIDPATCH;
}

HRESULT WINAPI IDirectMusicCollectionImpl_EnumInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwIndex, DWORD* pdwPatch, LPWSTR pwszName, DWORD dwNameLen)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);

	TRACE("(%p, %ld, %p, %p, %ld)\n", This, dwIndex, pdwPatch, pwszName, dwNameLen);
	if (dwIndex > This->nrofinstruments)
		return S_FALSE;
	*pdwPatch = This->ppInstruments[dwIndex]->dwPatch;
	if (pwszName != NULL) {
		/*
		 *pwszName = (LPWSTR)This->ppInstruments[dwIndex]->pwszName;
		 *dwNameLen = wcslen (This->ppInstruments[dwIndex]->pwszName);
		 */
	}
	
	return S_OK;
}

ICOM_VTABLE(IDirectMusicCollection) DirectMusicCollection_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicCollectionImpl_QueryInterface,
	IDirectMusicCollectionImpl_AddRef,
	IDirectMusicCollectionImpl_Release,
	IDirectMusicCollectionImpl_GetInstrument,
	IDirectMusicCollectionImpl_EnumInstrument
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicCollection (LPCGUID lpcGUID, LPDIRECTMUSICCOLLECTION* ppDMColl, LPUNKNOWN pUnkOuter)
{
	IDirectMusicCollectionImpl *collection;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppDMColl, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicCollection)) {
		collection = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicCollectionImpl));
		if (NULL == collection) {
			*ppDMColl = (LPDIRECTMUSICCOLLECTION) NULL;
			return E_OUTOFMEMORY;
		}
		collection->lpVtbl = &DirectMusicCollection_Vtbl;
		collection->ref = 1;
		*ppDMColl = (LPDIRECTMUSICCOLLECTION) collection;
		return S_OK;
	}

	WARN("No interface found\n");	
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicCollectionObject implementation
 */
/* IDirectMusicCollectionObject IUnknown part: */
HRESULT WINAPI IDirectMusicCollectionObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicCollectionObject,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicObject)) {
		IDirectMusicCollectionObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicCollectionObjectStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = This->pStream;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicCollection)) {
		IDirectMusicCollectionImpl_AddRef ((LPDIRECTMUSICCOLLECTION)This->pCollection);
		*ppobj = This->pCollection;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicCollectionObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicCollectionObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicCollectionObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicCollectionObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicCollectionObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicCollectionObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicCollectionObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicCollectionObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicCollectionObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicCollectionObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicCollectionObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicCollectionObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicCollectionObject_QueryInterface,
	IDirectMusicCollectionObject_AddRef,
	IDirectMusicCollectionObject_Release,
	IDirectMusicCollectionObject_GetDescriptor,
	IDirectMusicCollectionObject_SetDescriptor,
	IDirectMusicCollectionObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicCollectionObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicCollectionObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicCollectionObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicCollectionObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicCollectionObjectStream));
		obj->pStream->lpVtbl = &DirectMusicCollectionObjectStream_Vtbl;
		obj->pStream->ref = 1;
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicCollection */
		DMUSIC_CreateDirectMusicCollection (&IID_IDirectMusicCollection, (LPDIRECTMUSICCOLLECTION*)&obj->pCollection, NULL);
		obj->pCollection->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirectMusicCollectionObjectStream implementation
 */
/* IDirectMusicCollectionObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicCollectionObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicCollectionObjectStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicCollectionObjectStream_AddRef (iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicCollectionObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicCollectionObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicCollectionObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicCollectionObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicCollectionObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicCollectionObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicCollectionObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicCollectionObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicCollectionObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FOURCC chunkID;
	DWORD chunkSize;

	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
	
	if (chunkID == FOURCC_RIFF) {
		FIXME(": Loading not implemented yet\n");
	} else {
		WARN("Not a RIFF file\n");
		return E_FAIL;
	}
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicCollectionObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicCollectionObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicCollectionObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicCollectionObjectStream_QueryInterface,
	IDirectMusicCollectionObjectStream_AddRef,
	IDirectMusicCollectionObjectStream_Release,
	IDirectMusicCollectionObjectStream_GetClassID,
	IDirectMusicCollectionObjectStream_IsDirty,
	IDirectMusicCollectionObjectStream_Load,
	IDirectMusicCollectionObjectStream_Save,
	IDirectMusicCollectionObjectStream_GetSizeMax
};
