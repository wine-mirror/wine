/* IDirectMusicBuffer Implementation
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

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "dsound.h"

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicBuffer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicBufferImpl_QueryInterface (LPDIRECTMUSICBUFFER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicBuffer))
	{
		IDirectMusicBufferImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicBufferImpl_AddRef (LPDIRECTMUSICBUFFER iface)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicBufferImpl_Release (LPDIRECTMUSICBUFFER iface)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicBuffer Interface follow: */
HRESULT WINAPI IDirectMusicBufferImpl_Flush (LPDIRECTMUSICBUFFER iface)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p): stub\n", This);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_TotalTime (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prtTime)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);
	
	FIXME("(%p, %p): stub\n", This, prtTime);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_PackStructured (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD dwChannelMessage)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %lli, %ld, %ld): stub\n", This, rt, dwChannelGroup, dwChannelMessage);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_PackUnstructured (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD cb, LPBYTE lpb)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %lli, %ld, %ld, %p): stub\n", This, rt, dwChannelGroup, cb, lpb);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_ResetReadPtr (LPDIRECTMUSICBUFFER iface)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p): stub\n", This);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetNextEvent (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt, LPDWORD pdwChannelGroup, LPDWORD pdwLength, LPBYTE* ppData)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %p, %p, %p, %p): stub\n", This, prt, pdwChannelGroup, pdwLength, ppData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetRawBufferPtr (LPDIRECTMUSICBUFFER iface, LPBYTE* ppData)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %p): stub\n", This, ppData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetStartTime (LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %p): stub\n", This, prt);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetUsedBytes (LPDIRECTMUSICBUFFER iface, LPDWORD pcb)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %p): stub\n", This, pcb);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetMaxBytes (LPDIRECTMUSICBUFFER iface, LPDWORD pcb)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %p): stub\n", This, pcb);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_GetBufferFormat (LPDIRECTMUSICBUFFER iface, LPGUID pGuidFormat)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %p): stub\n", This, pGuidFormat);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_SetStartTime (LPDIRECTMUSICBUFFER iface, REFERENCE_TIME rt)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %lli): stub\n", This, rt);

	return S_OK;
}

HRESULT WINAPI IDirectMusicBufferImpl_SetUsedBytes (LPDIRECTMUSICBUFFER iface, DWORD cb)
{
	ICOM_THIS(IDirectMusicBufferImpl,iface);

	FIXME("(%p, %ld): stub\n", This, cb);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicBuffer) DirectMusicBuffer_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicBufferImpl_QueryInterface,
	IDirectMusicBufferImpl_AddRef,
	IDirectMusicBufferImpl_Release,
	IDirectMusicBufferImpl_Flush,
	IDirectMusicBufferImpl_TotalTime,
	IDirectMusicBufferImpl_PackStructured,
	IDirectMusicBufferImpl_PackUnstructured,
	IDirectMusicBufferImpl_ResetReadPtr,
	IDirectMusicBufferImpl_GetNextEvent,
	IDirectMusicBufferImpl_GetRawBufferPtr,
	IDirectMusicBufferImpl_GetStartTime,
	IDirectMusicBufferImpl_GetUsedBytes,
	IDirectMusicBufferImpl_GetMaxBytes,
	IDirectMusicBufferImpl_GetBufferFormat,
	IDirectMusicBufferImpl_SetStartTime,
	IDirectMusicBufferImpl_SetUsedBytes
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicBuffer (LPCGUID lpcGUID, LPDIRECTMUSICBUFFER* ppDMBuff, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicBuffer))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
