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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicLoader IUnknown parts follow: */
HRESULT WINAPI IDirectMusicLoaderImpl_QueryInterface (LPDIRECTMUSICLOADER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicLoaderImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicLoader))
	{
		IDirectMusicLoaderImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicLoaderImpl_AddRef (LPDIRECTMUSICLOADER iface)
{
	ICOM_THIS(IDirectMusicLoaderImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicLoaderImpl_Release (LPDIRECTMUSICLOADER iface)
{
	ICOM_THIS(IDirectMusicLoaderImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicLoader Interface follow: */
HRESULT WINAPI IDirectMusicLoaderImpl_GetObject (LPDIRECTMUSICLOADER iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID*ppv)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_SetObject (LPDIRECTMUSICLOADER iface, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_SetSearchDirectory (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_ScanDirectory (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_CacheObject (LPDIRECTMUSICLOADER iface, IDirectMusicObject* pObject)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_ReleaseObject (LPDIRECTMUSICLOADER iface, IDirectMusicObject* pObject)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_ClearCache (LPDIRECTMUSICLOADER iface, REFGUID rguidClass)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_EnableCache (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoaderImpl_EnumObject (LPDIRECTMUSICLOADER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicLoader) DirectMusicLoader_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicLoaderImpl_QueryInterface,
	IDirectMusicLoaderImpl_AddRef,
	IDirectMusicLoaderImpl_Release,
	IDirectMusicLoaderImpl_GetObject,
	IDirectMusicLoaderImpl_SetObject,
	IDirectMusicLoaderImpl_SetSearchDirectory,
	IDirectMusicLoaderImpl_ScanDirectory,
	IDirectMusicLoaderImpl_CacheObject,
	IDirectMusicLoaderImpl_ReleaseObject,
	IDirectMusicLoaderImpl_ClearCache,
	IDirectMusicLoaderImpl_EnableCache,
	IDirectMusicLoaderImpl_EnumObject
};


/* IDirectMusicLoader8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicLoader8Impl_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicLoader8))
	{
		IDirectMusicLoader8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
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
HRESULT WINAPI IDirectMusicLoader8Impl_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID*ppv)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc)
{
	FIXME("stub\n");
	return DS_OK;
}

/* IDirectMusicLoader8 Interface part follow: */
void WINAPI IDirectMusicLoader8Impl_CollectGarbage (LPDIRECTMUSICLOADER8 iface)
{
	FIXME("stub\n");
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClassID, REFIID iidInterfaceID, WCHAR* pwzFilePath, void** ppObject)
{
	FIXME("stub\n");
	return DS_OK;
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


/* IDirectMusicGetLoader IUnknown parts follow: */
HRESULT WINAPI IDirectMusicGetLoaderImpl_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicGetLoaderImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicGetLoader))
	{
		IDirectMusicGetLoaderImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
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
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicGetLoader_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicGetLoaderImpl_QueryInterface,
	IDirectMusicGetLoaderImpl_AddRef,
	IDirectMusicGetLoaderImpl_Release,
	IDirectMusicGetLoaderImpl_GetLoader
};
