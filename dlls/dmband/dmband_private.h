/* DirectMusicBand Private Include
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

#ifndef __WINE_DMBAND_PRIVATE_H
#define __WINE_DMBAND_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicBandImpl IDirectMusicBandImpl;

typedef struct IDirectMusicBandObject IDirectMusicBandObject;
typedef struct IDirectMusicBandObjectStream IDirectMusicBandObjectStream;

typedef struct IDirectMusicBandTrack IDirectMusicBandTrack;
typedef struct IDirectMusicBandTrackStream IDirectMusicBandTrackStream;
	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicBand) DirectMusicBand_Vtbl;

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicBandObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicBandObjectStream_Vtbl;

extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicBandTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicBandTrackStream_Vtbl;

/*****************************************************************************
 * ClassFactory
 * can support IID_IDirectMusicBand
 * return always an IDirectMusicBandImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicBand (LPCGUID lpcGUID, LPDIRECTMUSICBAND* ppDMBand, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicBandObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicBandTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicBandImpl implementation structure
 */
struct IDirectMusicBandImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicBand);
  DWORD          ref;

  /* IDirectMusicBandImpl fields */
  IDirectMusicBandObject* pObject;
  GUID guidID; /* unique id */
  DMUS_IO_VERSION vVersion; /* version */
  /* info from UNFO list */
  WCHAR* wszName;
  WCHAR* wszArtist;
  WCHAR* wszCopyright;
  WCHAR* wszSubject;
  WCHAR* wszComment;
  /* data */
  DMUS_IO_INSTRUMENT pInstruments[255];
  IDirectMusicCollection* ppReferenceCollection[255];
  DWORD dwInstruments;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandImpl_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandImpl_AddRef (LPDIRECTMUSICBAND iface);
extern ULONG WINAPI   IDirectMusicBandImpl_Release (LPDIRECTMUSICBAND iface);
/* IDirectMusicBand: */
extern HRESULT WINAPI IDirectMusicBandImpl_CreateSegment (LPDIRECTMUSICBAND iface, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicBandImpl_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance);
extern HRESULT WINAPI IDirectMusicBandImpl_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance);


/*****************************************************************************
 * IDirectMusicBandObject implementation structure
 */
struct IDirectMusicBandObject
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicBandObjectStream* pStream;
  IDirectMusicBandImpl* pBand;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicBandObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicBandObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicBandObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicBandObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicBandObjectStream implementation structure
 */
struct IDirectMusicBandObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicBandObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicBandObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicBandObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicBandObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicBandObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicBandObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicBandObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicBandObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

/* i don't like M$'s idea about two different band item headers, so behold: universal one */
typedef struct _DMUS_PRIVATE_BAND_ITEM_HEADER {
	DWORD dwVersion; /* 1 or 2 */
	/* v.1 */
	MUSIC_TIME lBandTime;
	/* v.2 */
	MUSIC_TIME lBandTimeLogical;
	MUSIC_TIME lBandTimePhysical;
} DMUS_PRIVATE_BAND_ITEM_HEADER;

/*****************************************************************************
 * IDirectMusicBandTrack implementation structure
 */
struct IDirectMusicBandTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicBandTrack fields */
  IDirectMusicBandTrackStream* pStream;
  DMUS_IO_BAND_TRACK_HEADER btkHeader; /* header */
  GUID guidID; /* unique id */
  DMUS_IO_VERSION vVersion; /* version */
  /* info from UNFO list */
  WCHAR* wszName;
  WCHAR* wszArtist;
  WCHAR* wszCopyright;
  WCHAR* wszSubject;
  WCHAR* wszComment;
  /* data */
  DMUS_PRIVATE_BAND_ITEM_HEADER pBandHeaders[255]; /* band item headers for bands */
  IDirectMusicBandImpl* ppBands[255]; /* bands */
  DWORD dwBands; /* nr. of IDirectMusicBandImpl* and DMUS_PRIVATE_BAND_ITEM_HEADER */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicBandTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicBandTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicBandTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicBandTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicBandTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicBandTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicBandTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicBandTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicBandTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicBandTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicBandTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicBandTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicBandTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicBandTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicBandTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicBandTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicBandTrackStream implementation structure
 */
struct IDirectMusicBandTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicBandTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicBandTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicBandTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicBandTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicBandTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicBandTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicBandTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicBandTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicBandTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

static inline const char *debugstr_fourcc( DWORD fourcc )
{
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf( "\'%c%c%c%c\'",
                             (char)(fourcc), (char)(fourcc >> 8),
                             (char)(fourcc >> 16), (char)(fourcc >> 24) );
}

#endif	/* __WINE_DMBAND_PRIVATE_H */
