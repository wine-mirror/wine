/* IDirectMusicStyle8 Implementation
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

#include "dmstyle_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);

/* IDirectMusicStyle8 IUnknown part: */
HRESULT WINAPI IDirectMusicStyle8Impl_QueryInterface (LPDIRECTMUSICSTYLE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicStyle) ||
	    IsEqualIID (riid, &IID_IDirectMusicStyle8)) {
		IDirectMusicStyle8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyle8Impl_AddRef (LPDIRECTMUSICSTYLE8 iface)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyle8Impl_Release (LPDIRECTMUSICSTYLE8 iface)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyle8 IDirectMusicStyle part: */
HRESULT WINAPI IDirectMusicStyle8Impl_GetBand (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicBand** ppBand)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pwszName, ppBand);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_EnumBand (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultBand (LPDIRECTMUSICSTYLE8 iface, IDirectMusicBand** ppBand)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppBand);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_EnumMotif (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetMotif (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pwszName, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultChordMap (LPDIRECTMUSICSTYLE8 iface, IDirectMusicChordMap** ppChordMap)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppChordMap);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_EnumChordMap (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetChordMap (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pwszName, ppChordMap);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetTimeSignature (LPDIRECTMUSICSTYLE8 iface, DMUS_TIMESIGNATURE* pTimeSig)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pTimeSig);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetEmbellishmentLength (LPDIRECTMUSICSTYLE8 iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %ld, %ld, %p, %p): stub\n", This, dwType, dwLevel, pdwMin, pdwMax);

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetTempo (LPDIRECTMUSICSTYLE8 iface, double* pTempo)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pTempo);

	return S_OK;
}

/* IDirectMusicStyle8 IDirectMusicStyle8 part: */
HRESULT WINAPI IDirectMusicStyle8ImplEnumPattern (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, DWORD dwPatternType, WCHAR* pwszName)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, dwIndex, dwPatternType, pwszName);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicStyle8) DirectMusicStyle8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyle8Impl_QueryInterface,
	IDirectMusicStyle8Impl_AddRef,
	IDirectMusicStyle8Impl_Release,
	IDirectMusicStyle8Impl_GetBand,
	IDirectMusicStyle8Impl_EnumBand,
	IDirectMusicStyle8Impl_GetDefaultBand,
	IDirectMusicStyle8Impl_EnumMotif,
	IDirectMusicStyle8Impl_GetMotif,
	IDirectMusicStyle8Impl_GetDefaultChordMap,
	IDirectMusicStyle8Impl_EnumChordMap,
	IDirectMusicStyle8Impl_GetChordMap,
	IDirectMusicStyle8Impl_GetTimeSignature,
	IDirectMusicStyle8Impl_GetEmbellishmentLength,
	IDirectMusicStyle8Impl_GetTempo,
	IDirectMusicStyle8ImplEnumPattern
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicStyle (LPCGUID lpcGUID, LPDIRECTMUSICSTYLE8* ppDMStyle, LPUNKNOWN pUnkOuter)
{
	IDirectMusicStyle8Impl* dmstlye;
	
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicStyle)
		|| IsEqualIID (lpcGUID, &IID_IDirectMusicStyle8)) {
		dmstlye = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyle8Impl));
		if (NULL == dmstlye) {
			*ppDMStyle = (LPDIRECTMUSICSTYLE8) NULL;
			return E_OUTOFMEMORY;
		}
		dmstlye->lpVtbl = &DirectMusicStyle8_Vtbl;
		dmstlye->ref = 1;
		*ppDMStyle = (LPDIRECTMUSICSTYLE8) dmstlye;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}


/*****************************************************************************
 * IDirectMusicStyleObject implementation
 */
/* IDirectMusicStyleObject IUnknown part: */
HRESULT WINAPI IDirectMusicStyleObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyleObject,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicObject)) {
		IDirectMusicStyleObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IPersistStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = (LPPERSISTSTREAM)This->pStream;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicStyle)
		|| IsEqualIID (riid, &IID_IDirectMusicStyle8)) {
		IDirectMusicStyle8_AddRef ((LPDIRECTMUSICSTYLE8)This->pStyle);
		*ppobj = (LPDIRECTMUSICSTYLE8)This->pStyle;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyleObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicStyleObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyleObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicStyleObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyleObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicStyleObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicStyleObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicStyleObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicStyleObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicStyleObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleObject_QueryInterface,
	IDirectMusicStyleObject_AddRef,
	IDirectMusicStyleObject_Release,
	IDirectMusicStyleObject_GetDescriptor,
	IDirectMusicStyleObject_SetDescriptor,
	IDirectMusicStyleObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicStyleObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicStyleObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyleObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicStyleObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicStyleObjectStream));
		obj->pStream->lpVtbl = &DirectMusicStyleObjectStream_Vtbl;
		obj->pStream->ref = 1;	
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicStyle */
		DMUSIC_CreateDirectMusicStyle (&IID_IDirectMusicStyle8, (LPDIRECTMUSICSTYLE8*)&obj->pStyle, NULL);
		obj->pStyle->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
	
/*****************************************************************************
 * IDirectMusicStyleObjectStream implementation
 */
/* IDirectMusicStyleObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicStyleObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyleObjectStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicStyleObjectStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyleObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicStyleObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyleObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicStyleObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyleObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicStyleObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
	return E_NOTIMPL;
}

/* IDirectMusicStyleObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicStyleObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicStyleObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicStyleObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicStyleObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleObjectStream_QueryInterface,
	IDirectMusicStyleObjectStream_AddRef,
	IDirectMusicStyleObjectStream_Release,
	IDirectMusicStyleObjectStream_GetClassID,
	IDirectMusicStyleObjectStream_IsDirty,
	IDirectMusicStyleObjectStream_Load,
	IDirectMusicStyleObjectStream_Save,
	IDirectMusicStyleObjectStream_GetSizeMax
};
