/* IDirectMusicInstrument Implementation
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

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IDirectMusicInstrument)) {
		IDirectMusicInstrumentImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
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
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicInstrument Interface follow: */
HRESULT WINAPI IDirectMusicInstrumentImpl_GetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch)
{
	ICOM_THIS(IDirectMusicInstrumentImpl,iface);

	TRACE("(%p, %p)\n", This, pdwPatch);
	*pdwPatch = This->dwPatch;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicInstrumentImpl_SetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch)
{
	ICOM_THIS(IDirectMusicInstrumentImpl,iface);

	TRACE("(%p, %ld)\n", This, dwPatch);
	This->dwPatch = dwPatch;

	return S_OK;
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicInstrument (LPCGUID lpcGUID, LPDIRECTMUSICINSTRUMENT* ppDMInstr, LPUNKNOWN pUnkOuter)
{
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicInstrument)) {
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}

	WARN("No interface found\n");
	return E_NOINTERFACE;	
}
