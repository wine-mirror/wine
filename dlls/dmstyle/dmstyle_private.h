/* DirectMusicStyle Private Include
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#ifndef __WINE_DMSTYLE_PRIVATE_H
#define __WINE_DMSTYLE_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "winreg.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicStyle8Impl IDirectMusicStyle8Impl;

typedef struct IDirectMusicAuditionTrack IDirectMusicAuditionTrack;
typedef struct IDirectMusicChordTrack IDirectMusicChordTrack;
typedef struct IDirectMusicCommandTrack IDirectMusicCommandTrack;
typedef struct IDirectMusicMelodyFormulationTrack IDirectMusicMelodyFormulationTrack;
typedef struct IDirectMusicMotifTrack IDirectMusicMotifTrack;
typedef struct IDirectMusicMuteTrack IDirectMusicMuteTrack;
typedef struct IDirectMusicStyleTrack IDirectMusicStyleTrack;
	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IUnknown)           DirectMusicStyle8_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicStyle8) DirectMusicStyle8_Style_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicStyle8_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicStyle8_IPersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicAuditionTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicAuditionTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicAuditionTrack_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicChordTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicChordTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicChordTrack_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicCommandTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicCommandTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicCommandTrack_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicMelodyFormulationTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMelodyFormulationTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicMelodyFormulationTrack_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicMotifTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMotifTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicMotifTrack_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicMuteTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMuteTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicMuteTrack_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicStyleTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicStyleTrack_Track_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicStyleTrack_PersistStream_Vtbl;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicStyleImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicAuditionTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicChordTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicCommandTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMelodyFormulationTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMotifTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMuteTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicStyleTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicStyle8Impl implementation structure
 */
struct IDirectMusicStyle8Impl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicStyle8) *StyleVtbl;
  ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicStyle8Impl fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicStyle: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_QueryInterface (LPDIRECTMUSICSTYLE8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IDirectMusicStyle8_AddRef (LPDIRECTMUSICSTYLE8 iface);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IDirectMusicStyle8_Release (LPDIRECTMUSICSTYLE8 iface);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetBand (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_EnumBand (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetDefaultBand (LPDIRECTMUSICSTYLE8 iface, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_EnumMotif (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetMotif (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetDefaultChordMap (LPDIRECTMUSICSTYLE8 iface, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_EnumChordMap (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetChordMap (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetTimeSignature (LPDIRECTMUSICSTYLE8 iface, DMUS_TIMESIGNATURE* pTimeSig);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetEmbellishmentLength (LPDIRECTMUSICSTYLE8 iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_GetTempo (LPDIRECTMUSICSTYLE8 iface, double* pTempo);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicStyle8_EnumPattern (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, DWORD dwPatternType, WCHAR* pwszName);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicStyle8Impl_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicStyle8Impl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicAuditionTrack implementation structure
 */
struct IDirectMusicAuditionTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicAuditionTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicAuditionTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicAuditionTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicAuditionTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicAuditionTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicAuditionTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicAuditionTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicAuditionTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicAuditionTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicChordTrack implementation structure
 */
struct IDirectMusicChordTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicChordTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicChordTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicChordTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicChordTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicChordTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicChordTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicChordTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

typedef struct _DMUS_PRIVATE_COMMAND {
	struct list entry; /* for listing elements */
	DMUS_IO_COMMAND pCommand;
	IDirectMusicCollection* ppReferenceCollection;
} DMUS_PRIVATE_COMMAND, *LPDMUS_PRIVATE_COMMAND;

/*****************************************************************************
 * IDirectMusicCommandTrack implementation structure
 */
struct IDirectMusicCommandTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicCommandTrack fields */
  LPDMUS_OBJECTDESC pDesc;
  /* track data */
  struct list Commands;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicCommandTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCommandTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicCommandTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCommandTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicCommandTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicCommandTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicCommandTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicCommandTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicCommandTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicMelodyFormulationTrack implementation structure
 */
struct IDirectMusicMelodyFormulationTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicMelodyFormulationTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicMotifTrack implementation structure
 */
struct IDirectMusicMotifTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicMotifTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMotifTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMotifTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicMotifTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMotifTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMotifTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMotifTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMotifTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicMotifTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicMotifTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMotifTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicMotifTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMotifTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMotifTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMotifTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicMuteTrack implementation structure
 */
struct IDirectMusicMuteTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicMuteTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMuteTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMuteTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicMuteTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMuteTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMuteTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMuteTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG   WINAPI IDirectMusicMuteTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG   WINAPI IDirectMusicMuteTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMuteTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicStyleTrack implementation structure
 */
struct IDirectMusicStyleTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicStyleTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyleTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyleTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicStyleTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyleTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicStyleTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicStyleTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicStyleTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicStyleTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicStyleTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _DMUS_PRIVATE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} DMUS_PRIVATE_CHUNK, *LPDMUS_PRIVATE_CHUNK;

/* check whether the given DWORD is even (return 0) or odd (return 1) */
static inline int even_or_odd (DWORD number) {
	return (number & 0x1); /* basically, check if bit 0 is set ;) */
}

/* FOURCC to string conversion for debug messages */
static inline const char *debugstr_fourcc (DWORD fourcc) {
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf ("\'%c%c%c%c\'",
		(char)(fourcc), (char)(fourcc >> 8),
        (char)(fourcc >> 16), (char)(fourcc >> 24));
}

/* DMUS_VERSION struct to string conversion for debug messages */
static inline const char *debugstr_dmversion (LPDMUS_VERSION version) {
	if (!version) return "'null'";
	return wine_dbg_sprintf ("\'%i,%i,%i,%i\'",
		(int)((version->dwVersionMS && 0xFFFF0000) >> 8), (int)(version->dwVersionMS && 0x0000FFFF), 
		(int)((version->dwVersionLS && 0xFFFF0000) >> 8), (int)(version->dwVersionLS && 0x0000FFFF));
}

/* used for initialising structs (primarily for DMUS_OBJECTDESC) */
#define DM_STRUCT_INIT(x) 				\
	do {								\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);		\
	} while (0)


/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }
#define DMUSIC_dump_flags(flags,names,num_names) DMUSIC_dump_flags_(flags, names, num_names, 1)

/* generic dump function */
static inline void DMUSIC_dump_flags_ (DWORD flags, const flag_info* names, size_t num_names, int newline) {
	unsigned int i;
	
	for (i=0; i < num_names; i++) {
		if ((flags & names[i].val) ||      /* standard flag value */
		((!flags) && (!names[i].val))) /* zero value only */
	    	DPRINTF("%s ", names[i].name);
	}
	
    if (newline) DPRINTF("\n");
}

static inline void DMUSIC_dump_DMUS_OBJ_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_OBJ_OBJECT),
	    FE(DMUS_OBJ_CLASS),
	    FE(DMUS_OBJ_NAME),
	    FE(DMUS_OBJ_CATEGORY),
	    FE(DMUS_OBJ_FILENAME),
	    FE(DMUS_OBJ_FULLPATH),
	    FE(DMUS_OBJ_URL),
	    FE(DMUS_OBJ_VERSION),
	    FE(DMUS_OBJ_DATE),
	    FE(DMUS_OBJ_LOADED),
	    FE(DMUS_OBJ_MEMORY),
	    FE(DMUS_OBJ_STREAM)
	};
    DMUSIC_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

static inline void DMUSIC_dump_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) {
	if (pDesc) {
		DPRINTF("DMUS_OBJECTDESC (%p)\n", pDesc);
		DPRINTF("  - dwSize = %ld\n", pDesc->dwSize);
		DPRINTF("  - dwValidData = ");
		DMUSIC_dump_DMUS_OBJ_FLAGS (pDesc->dwValidData);
		if (pDesc->dwValidData & DMUS_OBJ_CLASS) DPRINTF("  - guidClass = %s\n", debugstr_guid(&pDesc->guidClass));
		if (pDesc->dwValidData & DMUS_OBJ_OBJECT) DPRINTF("  - guidObject = %s\n", debugstr_guid(&pDesc->guidObject));
		if (pDesc->dwValidData & DMUS_OBJ_DATE) DPRINTF("  - ftDate = FIXME\n");
		if (pDesc->dwValidData & DMUS_OBJ_VERSION) DPRINTF("  - vVersion = %s\n", debugstr_dmversion(&pDesc->vVersion));
		if (pDesc->dwValidData & DMUS_OBJ_NAME) DPRINTF("  - wszName = %s\n", debugstr_w(pDesc->wszName));
		if (pDesc->dwValidData & DMUS_OBJ_CATEGORY) DPRINTF("  - wszCategory = %s\n", debugstr_w(pDesc->wszCategory));
		if (pDesc->dwValidData & DMUS_OBJ_FILENAME) DPRINTF("  - wszFileName = %s\n", debugstr_w(pDesc->wszFileName));
		if (pDesc->dwValidData & DMUS_OBJ_MEMORY) DPRINTF("  - llMemLength = %lli\n  - pbMemData = %p\n", pDesc->llMemLength, pDesc->pbMemData);
		if (pDesc->dwValidData & DMUS_OBJ_STREAM) DPRINTF("  - pStream = %p\n", pDesc->pStream);		
	} else {
		DPRINTF("(NULL)\n");
	}
}

#endif	/* __WINE_DMSTYLE_PRIVATE_H */
