/* IDirectMusicSegmentState8 Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);


/* IDirectMusicSegmentState8 IUnknown part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_QueryInterface (LPDIRECTMUSICSEGMENTSTATE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicSegmentState) ||
	    IsEqualGUID(riid, &IID_IDirectMusicSegmentState8))
	{
		IDirectMusicSegmentState8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicSegmentState8Impl_AddRef (LPDIRECTMUSICSEGMENTSTATE8 iface)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicSegmentState8Impl_Release (LPDIRECTMUSICSEGMENTSTATE8 iface)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicSegmentState Interface part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_GetRepeats (LPDIRECTMUSICSEGMENTSTATE8 iface,  DWORD* pdwRepeats)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pdwRepeats);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSegment (LPDIRECTMUSICSEGMENTSTATE8 iface, IDirectMusicSegment** ppSegment)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, ppSegment);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartTime (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtStart);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetSeek (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtSeek)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtSeek);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetStartPoint (LPDIRECTMUSICSEGMENTSTATE8 iface, MUSIC_TIME* pmtStart)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pmtStart);

	return S_OK;
}

/* IDirectMusicSegmentState8 Interface part follow: */
HRESULT WINAPI IDirectMusicSegmentState8Impl_SetTrackConfig (LPDIRECTMUSICSEGMENTSTATE8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_guid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);

	return S_OK;
}

HRESULT WINAPI IDirectMusicSegmentState8Impl_GetObjectInPath (LPDIRECTMUSICSEGMENTSTATE8 iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	ICOM_THIS(IDirectMusicSegmentState8Impl,iface);

	FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_guid(guidObject), dwIndex, debugstr_guid(iidInterface), ppObject);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicSegmentState8) DirectMusicSegmentState8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicSegmentState8Impl_QueryInterface,
	IDirectMusicSegmentState8Impl_AddRef,
	IDirectMusicSegmentState8Impl_Release,
	IDirectMusicSegmentState8Impl_GetRepeats,
	IDirectMusicSegmentState8Impl_GetSegment,
	IDirectMusicSegmentState8Impl_GetStartTime,
	IDirectMusicSegmentState8Impl_GetSeek,
	IDirectMusicSegmentState8Impl_GetStartPoint,
	IDirectMusicSegmentState8Impl_SetTrackConfig,
	IDirectMusicSegmentState8Impl_GetObjectInPath
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentState (LPCGUID lpcGUID, LPDIRECTMUSICSEGMENTSTATE8 *ppDMSeg, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicComposer))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
