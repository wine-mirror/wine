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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmcompos_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicChordMap IUnknown parts follow: */
HRESULT WINAPI IDirectMusicChordMapImpl_QueryInterface (LPDIRECTMUSICCHORDMAP iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordMapImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicChordMap))
	{
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
	if (ref == 0)
	{
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
	
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicChordMap))
	{
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
