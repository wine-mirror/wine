/* IDirectMusicBand Implementation
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

#include "dmband_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicBand IUnknown parts follow: */
HRESULT WINAPI IDirectMusicBandImpl_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicBand))
	{
		IDirectMusicBandImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBandImpl_AddRef (LPDIRECTMUSICBAND iface)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBandImpl_Release (LPDIRECTMUSICBAND iface)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBand Interface follow: */
HRESULT WINAPI IDirectMusicBandImpl_CreateSegment (LPDIRECTMUSICBAND iface, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandImpl_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPerformance);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBandImpl_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPerformance);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicBand) DirectMusicBand_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBandImpl_QueryInterface,
	IDirectMusicBandImpl_AddRef,
	IDirectMusicBandImpl_Release,
	IDirectMusicBandImpl_CreateSegment,
	IDirectMusicBandImpl_Download,
	IDirectMusicBandImpl_Unload
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicBand (LPCGUID lpcGUID, LPDIRECTMUSICBAND* ppDMBand, LPUNKNOWN pUnkOuter)
{
	IDirectMusicBandImpl* dmband;
	
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicBand))
	{
		dmband = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicBandImpl));
		if (NULL == dmband) {
			*ppDMBand = (LPDIRECTMUSICBAND) NULL;
			return E_OUTOFMEMORY;
		}
		dmband->lpVtbl = &DirectMusicBand_Vtbl;
		dmband->ref = 1;
		*ppDMBand = (LPDIRECTMUSICBAND) dmband;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
