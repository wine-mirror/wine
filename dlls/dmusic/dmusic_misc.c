/* IReferenceClock Implementation
 * IDirectMusicThru Implementation
 * IDirectMusicAudioPath Implementation
 * IDirectMusicBand Implementation
 * IDirectMusicSong Implementation
 * IDirectMusicChordMap Implementation
 * IDirectMusicComposer
 * IDirectMusicContainer
 * IDirectMusicGraph
 * IDirectMusicScript
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

/* IReferenceClock IUnknown parts follow: */
HRESULT WINAPI IReferenceClockImpl_QueryInterface (LPREFERENCECLOCK iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IReferenceClock))
	{
		IReferenceClockImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IReferenceClockImpl_AddRef (LPREFERENCECLOCK iface)
{
	ICOM_THIS(IReferenceClockImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IReferenceClockImpl_Release (LPREFERENCECLOCK iface)
{
	ICOM_THIS(IReferenceClockImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IReferenceClock Interface follow: */
HRESULT WINAPI IReferenceClockImpl_GetTime (LPREFERENCECLOCK iface, REFERENCE_TIME* pTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IReferenceClockImpl_AdviseTime (LPREFERENCECLOCK iface, REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD* pdwAdviseCookie)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic (LPREFERENCECLOCK iface, REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD* pdwAdviseCookie)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IReferenceClockImpl_Unadvise (LPREFERENCECLOCK iface, DWORD dwAdviseCookie)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IReferenceClock) ReferenceClock_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IReferenceClockImpl_QueryInterface,
	IReferenceClockImpl_AddRef,
	IReferenceClockImpl_Release,
	IReferenceClockImpl_GetTime,
	IReferenceClockImpl_AdviseTime,
	IReferenceClockImpl_AdvisePeriodic,
	IReferenceClockImpl_Unadvise
};


/* IDirectMusicThru IUnknown parts follow: */
HRESULT WINAPI IDirectMusicThruImpl_QueryInterface (LPDIRECTMUSICTHRU iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicThruImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicThru))
	{
		IDirectMusicThruImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
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
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicThru Interface follow: */
HRESULT WINAPI IDirectMusicThruImpl_ThruChannel (LPDIRECTMUSICTHRU iface, DWORD dwSourceChannelGroup, DWORD dwSourceChannel, DWORD dwDestinationChannelGroup, DWORD dwDestinationChannel, LPDIRECTMUSICPORT pDestinationPort)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicThru) DirectMusicThru_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicThruImpl_QueryInterface,
	IDirectMusicThruImpl_AddRef,
	IDirectMusicThruImpl_Release,
	IDirectMusicThruImpl_ThruChannel
};


/* IDirectMusicAudioPath IUnknown parts follow: */
HRESULT WINAPI IDirectMusicAudioPathImpl_QueryInterface (LPDIRECTMUSICAUDIOPATH iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicAudioPath))
	{
		IDirectMusicAudioPathImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicAudioPathImpl_AddRef (LPDIRECTMUSICAUDIOPATH iface)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicAudioPathImpl_Release (LPDIRECTMUSICAUDIOPATH iface)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicAudioPath Interface follow: */
HRESULT WINAPI IDirectMusicAudioPathImpl_GetObjectInPath (LPDIRECTMUSICAUDIOPATH iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, WORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicAudioPathImpl_Activate (LPDIRECTMUSICAUDIOPATH iface, BOOL fActivate)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicAudioPathImpl_SetVolume (LPDIRECTMUSICAUDIOPATH iface, long lVolume, DWORD dwDuration)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicAudioPathImpl_ConvertPChannel (LPDIRECTMUSICAUDIOPATH iface, DWORD dwPChannelIn, DWORD* pdwPChannelOut)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicAudioPath) DirectMusicAudioPath_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicAudioPathImpl_QueryInterface,
	IDirectMusicAudioPathImpl_AddRef,
	IDirectMusicAudioPathImpl_Release,

};


/* IDirectMusicBand IUnknown parts follow: */
HRESULT WINAPI IDirectMusicBandImpl_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicBandImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicBand))
	{
		IDirectMusicBandImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
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
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBandImpl_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicBandImpl_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance)
{
	FIXME("stub\n");
	return DS_OK;
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


/* IDirectMusicSong IUnknown parts follow: */
HRESULT WINAPI IDirectMusicSongImpl_QueryInterface (LPDIRECTMUSICSONG iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicSong))
	{
		IDirectMusicSongImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSongImpl_AddRef (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSongImpl_Release (LPDIRECTMUSICSONG iface)
{
	ICOM_THIS(IDirectMusicSongImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSong Interface follow: */
HRESULT WINAPI IDirectMusicSongImpl_Compose (LPDIRECTMUSICSONG iface)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetParam (LPDIRECTMUSICSONG iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetSegment (LPDIRECTMUSICSONG iface, WCHAR* pwzName, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_GetAudioPathConfig (LPDIRECTMUSICSONG iface, IUnknown** ppAudioPathConfig)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_Download (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_Unload (LPDIRECTMUSICSONG iface, IUnknown* pAudioPath)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicSongImpl_EnumSegment (LPDIRECTMUSICSONG iface, DWORD dwIndex, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicSong) DirectMusicSong_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSongImpl_QueryInterface,
	IDirectMusicSongImpl_AddRef,
	IDirectMusicSongImpl_Release,
	IDirectMusicSongImpl_Compose,
	IDirectMusicSongImpl_GetParam,
	IDirectMusicSongImpl_GetSegment,
	IDirectMusicSongImpl_GetAudioPathConfig,
	IDirectMusicSongImpl_Download,
	IDirectMusicSongImpl_Unload,
	IDirectMusicSongImpl_EnumSegment
};


/* IDirectMusicChordMap IUnknown parts follow: */
HRESULT WINAPI IDirectMusicChordMapImpl_QueryInterface (LPDIRECTMUSICCHORDMAP iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicChordMapImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicChordMap))
	{
		IDirectMusicChordMapImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
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
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicChordMap) DirectMusicChordMap_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicChordMapImpl_QueryInterface,
	IDirectMusicChordMapImpl_AddRef,
	IDirectMusicChordMapImpl_Release,
	IDirectMusicChordMapImpl_GetScale
};


/* IDirectMusicComposer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicComposerImpl_QueryInterface (LPDIRECTMUSICCOMPOSER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicComposer))
	{
		IDirectMusicComposerImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicComposerImpl_AddRef (LPDIRECTMUSICCOMPOSER iface)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicComposerImpl_Release (LPDIRECTMUSICCOMPOSER iface)
{
	ICOM_THIS(IDirectMusicComposerImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicComposer Interface follow: */
HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromTemplate (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, IDirectMusicSegment* pTemplate, WORD wActivity, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromShape (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, WORD wNumMeasures, WORD wShape, WORD wActivity, BOOL fIntro, BOOL fEnd, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ComposeTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pFromSeg, IDirectMusicSegment* pToSeg, MUSIC_TIME mtTime, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_AutoTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicPerformance* pPerformance, IDirectMusicSegment* pToSeg, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg, IDirectMusicSegmentState** ppToSegState, IDirectMusicSegmentState** ppTransSegState)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ComposeTemplateFromShape (LPDIRECTMUSICCOMPOSER iface, WORD wNumMeasures, WORD wShape, BOOL fIntro, BOOL fEnd, WORD wEndLength, IDirectMusicSegment** ppTemplate)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicComposerImpl_ChangeChordMap (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pSegment, BOOL fTrackScale, IDirectMusicChordMap* pChordMap)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicComposer) DirectMusicComposer_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicComposerImpl_QueryInterface,
	IDirectMusicComposerImpl_AddRef,
	IDirectMusicComposerImpl_Release,
	IDirectMusicComposerImpl_ComposeSegmentFromTemplate,
	IDirectMusicComposerImpl_ComposeSegmentFromShape,
	IDirectMusicComposerImpl_ComposeTransition,
	IDirectMusicComposerImpl_AutoTransition,
	IDirectMusicComposerImpl_ComposeTemplateFromShape,
	IDirectMusicComposerImpl_ChangeChordMap
};


/* IDirectMusicContainer IUnknown parts follow: */
HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicContainerImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicContainer))
	{
		IDirectMusicContainerImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
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
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicContainerImpl_QueryInterface,
	IDirectMusicContainerImpl_AddRef,
	IDirectMusicContainerImpl_Release,
	IDirectMusicContainerImpl_EnumObject
};


/* IDirectMusicGraph IUnknown parts follow: */
HRESULT WINAPI IDirectMusicGraphImpl_QueryInterface (LPDIRECTMUSICGRAPH iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicGraph))
	{
		IDirectMusicGraphImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicGraphImpl_AddRef (LPDIRECTMUSICGRAPH iface)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicGraphImpl_Release (LPDIRECTMUSICGRAPH iface)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicGraph Interface follow: */
HRESULT WINAPI IDirectMusicGraphImpl_StampPMsg (LPDIRECTMUSICGRAPH iface, DMUS_PMSG* pPMSG)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicGraphImpl_InsertTool (LPDIRECTMUSICGRAPH iface, IDirectMusicTool* pTool, DWORD* pdwPChannels, DWORD cPChannels, LONG lIndex)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicGraphImpl_GetTool (LPDIRECTMUSICGRAPH iface, DWORD dwIndex, IDirectMusicTool** ppTool)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicGraphImpl_RemoveTool (LPDIRECTMUSICGRAPH iface, IDirectMusicTool* pTool)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicGraph) DirectMusicGraph_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicGraphImpl_QueryInterface,
	IDirectMusicGraphImpl_AddRef,
	IDirectMusicGraphImpl_Release,
	IDirectMusicGraphImpl_StampPMsg,
	IDirectMusicGraphImpl_InsertTool,
	IDirectMusicGraphImpl_GetTool,
	IDirectMusicGraphImpl_RemoveTool
};


/* IDirectMusicScript IUnknown parts follow: */
HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicScript))
	{
		IDirectMusicScriptImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicScriptImpl_AddRef (LPDIRECTMUSICSCRIPT iface)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicScriptImpl_Release (LPDIRECTMUSICSCRIPT iface)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicScript Interface follow: */
HRESULT WINAPI IDirectMusicScriptImpl_Init (LPDIRECTMUSICSCRIPT iface, IDirectMusicPerformance* pPerformance, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszRoutineName, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT* pvarValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG* plValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, IUnknown* punkValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, REFIID riid, LPVOID* ppv, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicScript) DirectMusicScript_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptImpl_QueryInterface,
	IDirectMusicScriptImpl_AddRef,
	IDirectMusicScriptImpl_Release,
	IDirectMusicScriptImpl_Init,
	IDirectMusicScriptImpl_CallRoutine,
	IDirectMusicScriptImpl_SetVariableVariant,
	IDirectMusicScriptImpl_GetVariableVariant,
	IDirectMusicScriptImpl_SetVariableNumber,
	IDirectMusicScriptImpl_GetVariableNumber,
	IDirectMusicScriptImpl_SetVariableObject,
	IDirectMusicScriptImpl_GetVariableObject,
	IDirectMusicScriptImpl_EnumRoutine,
	IDirectMusicScriptImpl_EnumVariable
};
