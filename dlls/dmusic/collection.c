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

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicCollection))
	{
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
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicCollection Interface follow: */
HRESULT WINAPI IDirectMusicCollectionImpl_GetInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwPatch, IDirectMusicInstrument** ppInstrument)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwPatch, ppInstrument);

	return S_OK;
}

HRESULT WINAPI IDirectMusicCollectionImpl_EnumInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwIndex, DWORD* pdwPatch, LPWSTR pwszName, DWORD dwNameLen)
{
	ICOM_THIS(IDirectMusicCollectionImpl,iface);

	FIXME("(%p, %ld, %p, %p, %ld): stub\n", This, dwIndex, pdwPatch, pwszName, dwNameLen);

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
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicCollection))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
