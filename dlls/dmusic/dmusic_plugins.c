/* IDirectMusicTool Implementation
 * IDirectMusicTool8 Implementation
 * IDirectMusicTrack Implementation
 * IDirectMusicTrack8 Implementation
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

/* IDirectMusicTool IUnknown parts follow: */
HRESULT WINAPI IDirectMusicToolImpl_QueryInterface (LPDIRECTMUSICTOOL iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicTool))
	{
		IDirectMusicToolImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicToolImpl_AddRef (LPDIRECTMUSICTOOL iface)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicToolImpl_Release (LPDIRECTMUSICTOOL iface)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicTool Interface follow: */
HRESULT WINAPI IDirectMusicToolImpl_Init (LPDIRECTMUSICTOOL iface, IDirectMusicGraph* pGraph)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	FIXME("(%p, %p): stub\n", This, pGraph);

	return S_OK;
}

HRESULT WINAPI IDirectMusicToolImpl_GetMsgDeliveryType (LPDIRECTMUSICTOOL iface, DWORD* pdwDeliveryType)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	FIXME("(%p, %p): stub\n", This, pdwDeliveryType);

	return S_OK;
}

HRESULT WINAPI IDirectMusicToolImpl_GetMediaTypeArraySize (LPDIRECTMUSICTOOL iface, DWORD* pdwNumElements)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	FIXME("(%p, %p): stub\n", This, pdwNumElements);

	return S_OK;
}

HRESULT WINAPI IDirectMusicToolImpl_GetMediaTypes (LPDIRECTMUSICTOOL iface, DWORD** padwMediaTypes, DWORD dwNumElements)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	FIXME("(%p, %p, %ld): stub\n", This, padwMediaTypes, dwNumElements);

	return S_OK;
}

HRESULT WINAPI IDirectMusicToolImpl_ProcessPMsg (LPDIRECTMUSICTOOL iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	FIXME("(%p, %p, %p): stub\n", This, pPerf, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicToolImpl_Flush (LPDIRECTMUSICTOOL iface, IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG, REFERENCE_TIME rtTime)
{
	ICOM_THIS(IDirectMusicToolImpl, iface);

	FIXME("(%p, %p, %p, FIXME): stub\n", This, pPerf, pPMSG/*, rtTime*/);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTool) DirectMusicTool_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicToolImpl_QueryInterface,
	IDirectMusicToolImpl_AddRef,
	IDirectMusicToolImpl_Release,
	IDirectMusicToolImpl_Init,
	IDirectMusicToolImpl_GetMsgDeliveryType,
	IDirectMusicToolImpl_GetMediaTypeArraySize,
	IDirectMusicToolImpl_GetMediaTypes,
	IDirectMusicToolImpl_ProcessPMsg,
	IDirectMusicToolImpl_Flush
};


/* IDirectMusicTool8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicTool8Impl_QueryInterface (LPDIRECTMUSICTOOL8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicTool8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicTool8))
	{
		IDirectMusicTool8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
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
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicTool8 Interface follow: */
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

	FIXME("(%p, %p, %p, FIXME): stub\n", This, pPerf, pPMSG/*, rtTime*/);

	return S_OK;
}

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


/* IDirectMusicTrack IUnknown parts follow: */
HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface (LPDIRECTMUSICTRACK iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicTrack))
	{
		IDirectMusicTrackImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicTrackImpl_AddRef (LPDIRECTMUSICTRACK iface)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicTrackImpl_Release (LPDIRECTMUSICTRACK iface)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}
/* IDirectMusicTrack Interface follow: */
HRESULT WINAPI IDirectMusicTrackImpl_Init (LPDIRECTMUSICTRACK iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_InitPlay (LPDIRECTMUSICTRACK iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrackID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_EndPlay (LPDIRECTMUSICTRACK iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_Play (LPDIRECTMUSICTRACK iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_GetParam (LPDIRECTMUSICTRACK iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_SetParam (LPDIRECTMUSICTRACK iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_IsParamSupported (LPDIRECTMUSICTRACK iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_AddNotificationType (LPDIRECTMUSICTRACK iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_RemoveNotificationType (LPDIRECTMUSICTRACK iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrackImpl_Clone (LPDIRECTMUSICTRACK iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicTrackImpl,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack) DirectMusicTrack_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicTrackImpl_QueryInterface,
	IDirectMusicTrackImpl_AddRef,
	IDirectMusicTrackImpl_Release,
	IDirectMusicTrackImpl_Init,
	IDirectMusicTrackImpl_InitPlay,
	IDirectMusicTrackImpl_EndPlay,
	IDirectMusicTrackImpl_Play,
	IDirectMusicTrackImpl_GetParam,
	IDirectMusicTrackImpl_SetParam,
	IDirectMusicTrackImpl_IsParamSupported,
	IDirectMusicTrackImpl_AddNotificationType,
	IDirectMusicTrackImpl_RemoveNotificationType,
	IDirectMusicTrackImpl_Clone
};

/* IDirectMusicTrack8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicTrack8Impl_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicTrack8))
	{
		IDirectMusicTrack8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicTrack8Impl_AddRef (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicTrack8Impl_Release (LPDIRECTMUSICTRACK8 iface)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}
/* IDirectMusicTrack Interface part follow: */
HRESULT WINAPI IDirectMusicTrack8Impl_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pStateData);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s, %ld, %p, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pmtNext, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidType), mtTime, pParam);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidNotificationType));

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);

	return S_OK;
}

/* IDirectMusicTrack8 Interface part follow: */

HRESULT WINAPI IDirectMusicTrack8Impl_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p, FIXME, FIXME, FIXME, %ld, %p, %p, %ld): stub\n", This, pStateData/*, rtStart, rtEnd, rtOffset*/, dwFlags, pPerf, pSegSt, dwVirtualID);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s, FIXME, %p, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType)/*, rtTime*/, prtNext, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %s, FIXME, %p, %p, %ld): stub\n", This, debugstr_guid(rguidType)/*, rtTime*/, pParam, pStateData, dwFlags);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

HRESULT WINAPI IDirectMusicTrack8Impl_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack)
{
	ICOM_THIS(IDirectMusicTrack8Impl,iface);

	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicTrack8) DirectMusicTrack8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicTrack8Impl_QueryInterface,
	IDirectMusicTrack8Impl_AddRef,
	IDirectMusicTrack8Impl_Release,
	IDirectMusicTrack8Impl_Init,
	IDirectMusicTrack8Impl_InitPlay,
	IDirectMusicTrack8Impl_EndPlay,
	IDirectMusicTrack8Impl_Play,
	IDirectMusicTrack8Impl_GetParam,
	IDirectMusicTrack8Impl_SetParam,
	IDirectMusicTrack8Impl_IsParamSupported,
	IDirectMusicTrack8Impl_AddNotificationType,
	IDirectMusicTrack8Impl_RemoveNotificationType,
	IDirectMusicTrack8Impl_Clone,
	IDirectMusicTrack8Impl_PlayEx,
	IDirectMusicTrack8Impl_GetParamEx,
	IDirectMusicTrack8Impl_SetParamEx,
	IDirectMusicTrack8Impl_Compose,
	IDirectMusicTrack8Impl_Join
};
