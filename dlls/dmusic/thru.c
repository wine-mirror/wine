/* IDirectMusicThru Implementation
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
 * MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

/* IDirectMusicThru IUnknown parts follow: */
HRESULT WINAPI IDirectMusicThruImpl_QueryInterface (LPDIRECTMUSICTHRU iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicThruImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicThru)) {
		IDirectMusicThruImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicThruImpl_AddRef (LPDIRECTMUSICTHRU iface)
{
	ICOM_THIS(IDirectMusicThruImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicThruImpl_Release (LPDIRECTMUSICTHRU iface)
{
	ICOM_THIS(IDirectMusicThruImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicThru Interface follow: */
HRESULT WINAPI IDirectMusicThruImpl_ThruChannel (LPDIRECTMUSICTHRU iface, DWORD dwSourceChannelGroup, DWORD dwSourceChannel, DWORD dwDestinationChannelGroup, DWORD dwDestinationChannel, LPDIRECTMUSICPORT pDestinationPort)
{
	ICOM_THIS(IDirectMusicThruImpl,iface);

	FIXME("(%p, %ld, %ld, %ld, %ld, %p): stub\n", This, dwSourceChannelGroup, dwSourceChannel, dwDestinationChannelGroup, dwDestinationChannel, pDestinationPort);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicThru) DirectMusicThru_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicThruImpl_QueryInterface,
	IDirectMusicThruImpl_AddRef,
	IDirectMusicThruImpl_Release,
	IDirectMusicThruImpl_ThruChannel
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicThru (LPCGUID lpcGUID, LPDIRECTMUSICTHRU* ppDMThru, LPUNKNOWN pUnkOuter)
{
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicThru))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;	
}
