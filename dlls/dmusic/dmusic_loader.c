/* IDirectMusicLoader Implementation
 * IDirectMusicLoader8 Implementation
 * IDirectMusicGetLoader Implementation
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
#include "wine/unicode.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicLoader8 IUnknown part follow: */
HRESULT WINAPI IDirectMusicLoader8Impl_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicLoader) ||
	    IsEqualGUID(riid, &IID_IDirectMusicLoader8))
	{
		IDirectMusicLoader8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicLoader8Impl_AddRef (LPDIRECTMUSICLOADER8 iface)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicLoader8Impl_Release (LPDIRECTMUSICLOADER8 iface)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicLoader Interface part follow: */
HRESULT WINAPI IDirectMusicLoader8Impl_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID* ppv)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	
	FIXME("(%p, %p, %s, %p): stub\n", This, pDesc, debugstr_guid(riid), ppv);

	if (IsEqualGUID(riid, &IID_IDirectMusicScript)) {
	  IDirectMusicScriptImpl* script;
	  script = (IDirectMusicScriptImpl*) HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectMusicScriptImpl));
	  script->lpVtbl = &DirectMusicScript_Vtbl;
	  script->ref = 1;
	  *ppv = script;
	}

	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pDesc);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %p, %d): to check\n", This, debugstr_guid(rguidClass), pwzPath, fClear);

        if (0 == strncmpW(This->searchPath, pwzPath, MAX_PATH)) {
	  return S_FALSE;
	} 
	strncpyW(This->searchPath, pwzPath, MAX_PATH);
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_guid(rguidClass), pwzFileExtension, pwzScanFileName);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pObject);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pObject);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidClass));

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %d): stub\n", This, debugstr_guid(rguidClass), fEnable);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidClass), dwIndex, pDesc);

	return S_OK;
}

/* IDirectMusicLoader8 Interface part follow: */
void WINAPI IDirectMusicLoader8Impl_CollectGarbage (LPDIRECTMUSICLOADER8 iface)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p): stub\n", This);
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pObject);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, 
							   REFGUID rguidClassID, 
							   REFIID iidInterfaceID, 
							   WCHAR* pwzFilePath, 
							   void** ppObject)
{
	HANDLE fd;

	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %s, %s, %p): stub\n", This, debugstr_guid(rguidClassID), debugstr_guid(iidInterfaceID), debugstr_w(pwzFilePath), ppObject);
	
	fd = CreateFileW (pwzFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!fd) {
		WARN ("could not load file\n");
		return DMUS_E_LOADER_FAILEDOPEN;
	}
	
	if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicAudioPathConfig)) {
		FIXME("wanted 'aud'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicBand)) {
		FIXME("wanted 'bnd'\n");
		DMUSIC_FillBandFromFileHandle (NULL, fd);
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicContainer)) {
		FIXME("wanted 'con'\n");
		DMUSIC_FillContainerFromFileHandle (NULL, fd);		
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicCollection)) {
		FIXME("wanted 'dls'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicChordMap)) {
		FIXME("wanted 'cdm'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicSegment)) {
		FIXME("wanted 'sgt'\n");
		DMUSIC_FillSegmentFromFileHandle (NULL, fd);
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicScript)) {
		FIXME("wanted 'spt'\n");
		DMUSIC_FillScriptFromFileHandle (NULL, fd);
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicSong)) {
		FIXME("wanted 'sng'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicStyle)) {
		FIXME("wanted 'sty'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicSegment)) {
		FIXME("wanted 'tpl'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectMusicGraph)) {
		FIXME("wanted 'tgr'\n");
	} else if (IsEqualGUID(rguidClassID, &CLSID_DirectSoundWave)) {
		FIXME("wanted 'wav'\n");
	}

	CloseHandle (fd);
	
	if (IsEqualGUID(iidInterfaceID, &IID_IDirectMusicSegment) || 
	    IsEqualGUID(iidInterfaceID, &IID_IDirectMusicSegment8)) {
	  IDirectMusicSegment8Impl* segment;
	  segment = (IDirectMusicSegment8Impl*) HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectMusicSegment8Impl));
	  segment->lpVtbl = &DirectMusicSegment8_Vtbl;
	  segment->ref = 1;
	  *ppObject = segment;
	  return S_OK;
	} else if (IsEqualGUID(iidInterfaceID, &IID_IDirectMusicContainer)) {
	  IDirectMusicContainerImpl* container;
	  container = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectMusicContainerImpl));
	  container->lpVtbl = &DirectMusicContainer_Vtbl;
	  container->ref = 1;
	  *ppObject = container;
	  return S_OK;
	} else if (IsEqualGUID(iidInterfaceID, &IID_IDirectMusicScript)) {
	  IDirectMusicScriptImpl* script;
	  script = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectMusicScriptImpl));
	  script->lpVtbl = &DirectMusicScript_Vtbl;
	  script->ref = 1;
	  *ppObject = script;
	  return S_OK;		
	} else {
	  FIXME("bad iid\n");
	}
	
	/** for now alway return not supported for avoiding futur crash */
	return DMUS_E_LOADER_FORMATNOTSUPPORTED;
}

ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicLoader8Impl_QueryInterface,
	IDirectMusicLoader8Impl_AddRef,
	IDirectMusicLoader8Impl_Release,
	IDirectMusicLoader8Impl_GetObject,
	IDirectMusicLoader8Impl_SetObject,
	IDirectMusicLoader8Impl_SetSearchDirectory,
	IDirectMusicLoader8Impl_ScanDirectory,
	IDirectMusicLoader8Impl_CacheObject,
	IDirectMusicLoader8Impl_ReleaseObject,
	IDirectMusicLoader8Impl_ClearCache,
	IDirectMusicLoader8Impl_EnableCache,
	IDirectMusicLoader8Impl_EnumObject,
	IDirectMusicLoader8Impl_CollectGarbage,
	IDirectMusicLoader8Impl_ReleaseObjectByUnknown,
	IDirectMusicLoader8Impl_LoadObjectFromFile
};

HRESULT WINAPI DMUSIC_CreateDirectMusicLoader8 (LPCGUID lpcGUID, LPDIRECTMUSICLOADER8 *ppDMLoad8, LPUNKNOWN pUnkOuter)
{
	IDirectMusicLoader8Impl *dmloader8;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppDMLoad8, pUnkOuter);
	if (IsEqualGUID(lpcGUID, &IID_IDirectMusicLoader) || 
	    IsEqualGUID(lpcGUID, &IID_IDirectMusicLoader8))
	{
		dmloader8 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoader8Impl));
		if (NULL == dmloader8) {
			*ppDMLoad8 = (LPDIRECTMUSICLOADER8)NULL;
			return E_OUTOFMEMORY;
		}
		dmloader8->lpVtbl = &DirectMusicLoader8_Vtbl;
		dmloader8->ref = 1;
		*ppDMLoad8 = (LPDIRECTMUSICLOADER8)dmloader8;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}


/* IDirectMusicGetLoader IUnknown parts follow: */
HRESULT WINAPI IDirectMusicGetLoaderImpl_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicGetLoaderImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicGetLoader))
	{
		IDirectMusicGetLoaderImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicGetLoaderImpl_AddRef (LPDIRECTMUSICGETLOADER iface)
{
	ICOM_THIS(IDirectMusicGetLoaderImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicGetLoaderImpl_Release (LPDIRECTMUSICGETLOADER iface)
{
	ICOM_THIS(IDirectMusicGetLoaderImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicGetLoader Interface follow: */
HRESULT WINAPI IDirectMusicGetLoaderImpl_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader** ppLoader)
{
	ICOM_THIS(IDirectMusicGetLoaderImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppLoader);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicGetLoader_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicGetLoaderImpl_QueryInterface,
	IDirectMusicGetLoaderImpl_AddRef,
	IDirectMusicGetLoaderImpl_Release,
	IDirectMusicGetLoaderImpl_GetLoader
};
