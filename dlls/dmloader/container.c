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

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicContainer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicContainerImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicContainer))
	{
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
	if (ref == 0)
	{
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
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicContainer))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
