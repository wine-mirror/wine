/* IDirectMusic8 Implementation
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

/* IDirectMusic8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusic8Impl_QueryInterface (LPDIRECTMUSIC8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusic8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusic8))
	{
		IDirectMusic8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusic8Impl_AddRef (LPDIRECTMUSIC8 iface)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusic8Impl_Release (LPDIRECTMUSIC8 iface)
{
	ICOM_THIS(IDirectMusic8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusic8 Interface follow: */
HRESULT WINAPI IDirectMusic8Impl_EnumPort (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_PORTCAPS pPortCaps)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_CreateMusicBuffer (LPDIRECTMUSIC8 iface, LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER** ppBuffer, LPUNKNOWN pUnkOuter)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_CreatePort (LPDIRECTMUSIC8 iface, REFCLSID rclsidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT* ppPort, LPUNKNOWN pUnkOuter)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_EnumMasterClock (LPDIRECTMUSIC8 iface, DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_GetMasterClock (LPDIRECTMUSIC8 iface, LPGUID pguidClock, IReferenceClock** ppReferenceClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetMasterClock (LPDIRECTMUSIC8 iface, REFGUID rguidClock)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_Activate (LPDIRECTMUSIC8 iface, BOOL fEnable)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_GetDefaultPort (LPDIRECTMUSIC8 iface, LPGUID pguidPort)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetDirectSound (LPDIRECTMUSIC8 iface, LPDIRECTSOUND pDirectSound, HWND hWnd)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusic8Impl_SetExternalMasterClock (LPDIRECTMUSIC8 iface, IReferenceClock* pClock)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusic8) DirectMusic8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusic8Impl_QueryInterface,
	IDirectMusic8Impl_AddRef,
	IDirectMusic8Impl_Release,
	IDirectMusic8Impl_EnumPort,
	IDirectMusic8Impl_CreateMusicBuffer,
	IDirectMusic8Impl_CreatePort,
	IDirectMusic8Impl_EnumMasterClock,
	IDirectMusic8Impl_GetMasterClock,
	IDirectMusic8Impl_SetMasterClock,
	IDirectMusic8Impl_Activate,
	IDirectMusic8Impl_GetDefaultPort,
	IDirectMusic8Impl_SetDirectSound,
	IDirectMusic8Impl_SetExternalMasterClock
};
