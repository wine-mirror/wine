/* IDirectMusicTool8 Implementation
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);


/* IDirectMusicTool8 IUnknown part: */
HRESULT WINAPI IDirectMusicTool8Impl_QueryInterface (LPDIRECTMUSICTOOL8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicTool) ||
	    IsEqualIID (riid, &IID_IDirectMusicTool8)) {
		IDirectMusicTool8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicTool8Impl_AddRef (LPDIRECTMUSICTOOL8 iface)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicTool8Impl_Release (LPDIRECTMUSICTOOL8 iface)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicTool8 IDirectMusicTool part: */
HRESULT WINAPI IDirectMusicTool8Impl_Init (LPDIRECTMUSICTOOL8 iface, IDirectMusicGraph* pGraph)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pGraph);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTool8Impl_GetMsgDeliveryType (LPDIRECTMUSICTOOL8 iface, DWORD* pdwDeliveryType)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwDeliveryType);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTool8Impl_GetMediaTypeArraySize (LPDIRECTMUSICTOOL8 iface, DWORD* pdwNumElements)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwNumElements);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTool8Impl_GetMediaTypes (LPDIRECTMUSICTOOL8 iface, DWORD** padwMediaTypes, DWORD dwNumElements)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p, %ld): stub\n", This, padwMediaTypes, dwNumElements);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTool8Impl_ProcessPMsg (LPDIRECTMUSICTOOL8 iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pPerf, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTool8Impl_Flush (LPDIRECTMUSICTOOL8 iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG, REFERENCE_TIME rtTime)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p, %p, %lli): stub\n", This, pPerf, pPMSG, rtTime);

	return S_OK;
}

/* IDirectMusicTool8 IDirectMusicTool8 part: */
HRESULT WINAPI IDirectMusicTool8Impl_Clone (LPDIRECTMUSICTOOL8 iface, IDirectMusicTool** ppTool)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppTool);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTool8) DirectMusicTool8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicTool8Impl_QueryInterface,
	IDirectMusicTool8Impl_AddRef,
	IDirectMusicTool8Impl_Release,
	IDirectMusicTool8Impl_Init,
	IDirectMusicTool8Impl_GetMsgDeliveryType,
	IDirectMusicTool8Impl_GetMediaTypeArraySize,
	IDirectMusicTool8Impl_GetMediaTypes,
	IDirectMusicTool8Impl_ProcessPMsg,
	IDirectMusicTool8Impl_Flush,
	IDirectMusicTool8Impl_Clone
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicTool (LPCGUID lpcGUID, LPDIRECTMUSICTOOL8 *ppDMTool, LPUNKNOWN pUnkOuter)
{
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicComposer)) {
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
