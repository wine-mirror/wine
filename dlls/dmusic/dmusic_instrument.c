/* IDirectMusicInstrument Implementation
 * IDirectMusicDownloadedInstrument Implementation
 * IDirectMusicCollection Implementation
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

/* IDirectMusicInstrument IUnknown parts follow: */
HRESULT WINAPI IDirectMusicInstrumentImpl_QueryInterface (LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicInstrumentImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicInstrument))
	{
		IDirectMusicInstrumentImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicInstrumentImpl_AddRef (LPDIRECTMUSICINSTRUMENT iface)
{
	ICOM_THIS(IDirectMusicInstrumentImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicInstrumentImpl_Release (LPDIRECTMUSICINSTRUMENT iface)
{
	ICOM_THIS(IDirectMusicInstrumentImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicInstrument Interface follow: */
HRESULT WINAPI IDirectMusicInstrumentImpl_GetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicInstrumentImpl_SetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicInstrument) DirectMusicInstrument_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicInstrumentImpl_QueryInterface,
	IDirectMusicInstrumentImpl_AddRef,
	IDirectMusicInstrumentImpl_Release,
	IDirectMusicInstrumentImpl_GetPatch,
	IDirectMusicInstrumentImpl_SetPatch
};


/* IDirectMusicDownloadedInstrument IUnknown parts follow: */
HRESULT WINAPI IDirectMusicDownloadedInstrumentImpl_QueryInterface (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicDownloadedInstrumentImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicDownloadedInstrument))
	{
		IDirectMusicDownloadedInstrumentImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicDownloadedInstrumentImpl_AddRef (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface)
{
	ICOM_THIS(IDirectMusicDownloadedInstrumentImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicDownloadedInstrumentImpl_Release (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface)
{
	ICOM_THIS(IDirectMusicDownloadedInstrumentImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicDownloadedInstrument Interface follow: */
/* none at this time */

ICOM_VTABLE(IDirectMusicDownloadedInstrument) DirectMusicDownloadedInstrument_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicDownloadedInstrumentImpl_QueryInterface,
	IDirectMusicDownloadedInstrumentImpl_AddRef,
	IDirectMusicDownloadedInstrumentImpl_Release
};


/* IDirectMusicCollection IUnknown parts follow: */
HRESULT WINAPI IDirectMusicCollectionImpl_QueryInterface (LPDIRECTMUSICCOLLECTION iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicCollection))
	{
		IDirectMusicCollectionImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
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
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicCollection Interface follow: */
HRESULT WINAPI IDirectMusicCollectionImpl_GetInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwPatch, IDirectMusicInstrument** ppInstrument)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicCollectionImpl_EnumInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwIndex, DWORD* pdwPatch, LPWSTR pwszName, DWORD dwNameLen)
{
	FIXME("stub\n");
	return DS_OK;
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
