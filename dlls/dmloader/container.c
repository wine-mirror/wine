/* IDirectMusicContainer
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

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);

/* IDirectMusicContainer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicContainerImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicContainer)) {
		IDirectMusicContainerImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicContainerImpl_AddRef (LPDIRECTMUSICCONTAINER iface)
{
	ICOM_THIS(IDirectMusicContainerImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicContainerImpl_Release (LPDIRECTMUSICCONTAINER iface)
{
	ICOM_THIS(IDirectMusicContainerImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicContainer Interface follow: */
HRESULT WINAPI IDirectMusicContainerImpl_EnumObject (LPDIRECTMUSICCONTAINER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc, WCHAR* pwszAlias)
{
	ICOM_THIS(IDirectMusicContainerImpl,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidClass), dwIndex, pDesc, pwszAlias);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerImpl_QueryInterface,
	IDirectMusicContainerImpl_AddRef,
	IDirectMusicContainerImpl_Release,
	IDirectMusicContainerImpl_EnumObject
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicContainer (LPCGUID lpcGUID, LPDIRECTMUSICCONTAINER *ppDMCon, LPUNKNOWN pUnkOuter)
{
	IDirectMusicContainerImpl* dmcon;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicBand)) {
		dmcon = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicContainerImpl));
		if (NULL == dmcon) {
			*ppDMCon = (LPDIRECTMUSICCONTAINER) NULL;
			return E_OUTOFMEMORY;
		}
		dmcon->lpVtbl = &DirectMusicContainer_Vtbl;
		dmcon->ref = 1;
		*ppDMCon = (LPDIRECTMUSICCONTAINER) dmcon;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}

/*****************************************************************************
 * IDirectMusicContainerObject implementation
 */
/* IDirectMusicContainerObject IUnknown part: */
HRESULT WINAPI IDirectMusicContainerObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicContainerObject,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicObject)) {
		IDirectMusicContainerObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IPersistStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = (LPPERSISTSTREAM)This->pStream;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicContainer)) {
		IDirectMusicContainer_AddRef ((LPDIRECTMUSICCONTAINER)This->pContainer);
		*ppobj = (LPDIRECTMUSICCONTAINER)This->pContainer;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicContainerObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicContainerObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicContainerObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicContainerObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicContainerObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicContainerObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicContainerObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicContainerObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicContainerObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicContainerObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicContainerObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicContainerObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerObject_QueryInterface,
	IDirectMusicContainerObject_AddRef,
	IDirectMusicContainerObject_Release,
	IDirectMusicContainerObject_GetDescriptor,
	IDirectMusicContainerObject_SetDescriptor,
	IDirectMusicContainerObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicContainerObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicContainerObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicContainerObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicContainerObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicContainerObjectStream));
		obj->pStream->lpVtbl = &DirectMusicContainerObjectStream_Vtbl;
		obj->pStream->ref = 1;	
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicContainer */
		DMUSIC_CreateDirectMusicContainer (&IID_IDirectMusicContainer, (LPDIRECTMUSICCONTAINER*)&obj->pContainer, NULL);
		obj->pContainer->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
	
/*****************************************************************************
 * IDirectMusicContainerObjectStream implementation
 */
/* IDirectMusicContainerObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicContainerObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicContainerObjectStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicContainerObjectStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicContainerObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicContainerObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicContainerObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicContainerObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicContainerObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicContainerObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicContainerObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicContainerObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicContainerObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicContainerObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicContainerObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicContainerObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerObjectStream_QueryInterface,
	IDirectMusicContainerObjectStream_AddRef,
	IDirectMusicContainerObjectStream_Release,
	IDirectMusicContainerObjectStream_GetClassID,
	IDirectMusicContainerObjectStream_IsDirty,
	IDirectMusicContainerObjectStream_Load,
	IDirectMusicContainerObjectStream_Save,
	IDirectMusicContainerObjectStream_GetSizeMax
};
