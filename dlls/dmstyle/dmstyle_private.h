/* DirectMusicStyle Private Include
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

#ifndef __WINE_DMSTYLE_PRIVATE_H
#define __WINE_DMSTYLE_PRIVATE_H

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicStyle8Impl IDirectMusicStyle8Impl;

typedef struct IDirectMusicStyleObject IDirectMusicStyleObject;
typedef struct IDirectMusicStyleObjectStream IDirectMusicStyleObjectStream;	

typedef struct IDirectMusicAuditionTrack IDirectMusicAuditionTrack;
typedef struct IDirectMusicAuditionTrackStream IDirectMusicAuditionTrackStream;
typedef struct IDirectMusicChordTrack IDirectMusicChordTrack;
typedef struct IDirectMusicChordTrackStream IDirectMusicChordTrackStream;
typedef struct IDirectMusicCommandTrack IDirectMusicCommandTrack;
typedef struct IDirectMusicCommandTrackStream IDirectMusicCommandTrackStream;
typedef struct IDirectMusicMelodyFormulationTrack IDirectMusicMelodyFormulationTrack;
typedef struct IDirectMusicMelodyFormulationTrackStream IDirectMusicMelodyFormulationTrackStream;
typedef struct IDirectMusicMotifTrack IDirectMusicMotifTrack;
typedef struct IDirectMusicMotifTrackStream IDirectMusicMotifTrackStream;
typedef struct IDirectMusicMuteTrack IDirectMusicMuteTrack;
typedef struct IDirectMusicMuteTrackStream IDirectMusicMuteTrackStream;
typedef struct IDirectMusicStyleTrack IDirectMusicStyleTrack;
typedef struct IDirectMusicStyleTrackStream IDirectMusicStyleTrackStream;
	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicStyle8) DirectMusicStyle8_Vtbl;

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicStyleObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicStyleObjectStream_Vtbl;

extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicAuditionTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicAuditionTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicChordTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicChordTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicCommandTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicCommandTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMelodyFormulationTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicMelodyFormulationTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMotifTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicMotifTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicMuteTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicMuteTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicStyleTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicStyleTrackStream_Vtbl;

/*****************************************************************************
 * ClassFactory
 *
 * can support IID_IDirectMusicStyle and IID_IDirectMusicStyle8
 * return always an IDirectMusicStyle8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicStyle (LPCGUID lpcGUID, LPDIRECTMUSICSTYLE8* ppDMStyle, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicStyleObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicAuditionTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicChordTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicCommandTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMelodyFormulationTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMotifTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMuteTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicStyleTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicStyle8Impl implementation structure
 */
struct IDirectMusicStyle8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicStyle8);
  DWORD          ref;

  /* IDirectMusicStyle8Impl fields */
  IDirectMusicStyleObject* pObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_QueryInterface (LPDIRECTMUSICSTYLE8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyle8Impl_AddRef (LPDIRECTMUSICSTYLE8 iface);
extern ULONG WINAPI   IDirectMusicStyle8Impl_Release (LPDIRECTMUSICSTYLE8 iface);
/* IDirectMusicStyle: */
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetBand (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyle8Impl_EnumBand (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultBand (LPDIRECTMUSICSTYLE8 iface, IDirectMusicBand** ppBand);
extern HRESULT WINAPI IDirectMusicStyle8Impl_EnumMotif (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetMotif (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultChordMap (LPDIRECTMUSICSTYLE8 iface, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyle8Impl_EnumChordMap (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetChordMap (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetTimeSignature (LPDIRECTMUSICSTYLE8 iface, DMUS_TIMESIGNATURE* pTimeSig);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetEmbellishmentLength (LPDIRECTMUSICSTYLE8 iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax);
extern HRESULT WINAPI IDirectMusicStyle8Impl_GetTempo (LPDIRECTMUSICSTYLE8 iface, double* pTempo);
/* IDirectMusicStyle8: */
extern HRESULT WINAPI IDirectMusicStyle8ImplEnumPattern (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, DWORD dwPatternType, WCHAR* pwszName);


/*****************************************************************************
 * IDirectMusicStyleObject implementation structure
 */
struct IDirectMusicStyleObject
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicStyleObjectStream* pStream;
  IDirectMusicStyle8Impl* pStyle;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyleObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyleObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicStyleObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicStyleObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicStyleObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicStyleObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicStyleObjectStream implementation structure
 */
struct IDirectMusicStyleObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicStyleObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyleObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicStyleObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicStyleObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicStyleObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicStyleObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicStyleObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicStyleObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicStyleObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicAuditionTrack implementation structure
 */
struct IDirectMusicAuditionTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicAuditionTrack fields */
  IDirectMusicAuditionTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicAuditionTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicAuditionTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicAuditionTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicAuditionTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicAuditionTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicAuditionTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicAuditionTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicAuditionTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicAuditionTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicAuditionTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicAuditionTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicAuditionTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicAuditionTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicAuditionTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicAuditionTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicAuditionTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicAuditionTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicAuditionTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicAuditionTrackStream implementation structure
 */
struct IDirectMusicAuditionTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicAuditionTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicAuditionTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicAuditionTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicAuditionTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicAuditionTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicAuditionTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicAuditionTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicAuditionTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicAuditionTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicChordTrack implementation structure
 */
struct IDirectMusicChordTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicChordTrack fields */
  IDirectMusicChordTrackStream* pStream;
  DWORD dwHeader;
  /* FIXME: scheme with HeapReAlloc doesn't work so.. */
/*
 *  DMUS_CHORD_KEY* pChordKeys;
 *  MUSIC_TIME* pChordKeysTime;
 */
  DMUS_CHORD_KEY pChordKeys[255];
  MUSIC_TIME pChordKeysTime[255];
  DWORD dwChordKeys;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicChordTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicChordTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicChordTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicChordTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicChordTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicChordTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicChordTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicChordTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicChordTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicChordTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicChordTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicChordTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicChordTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicChordTrackStream implementation structure
 */
struct IDirectMusicChordTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicChordTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicChordTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicChordTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicChordTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicChordTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicChordTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicChordTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicChordTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicChordTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicCommandTrack implementation structure
 */
struct IDirectMusicCommandTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicCommandTrack fields */
  IDirectMusicCommandTrackStream* pStream;
  /* track data */
  DMUS_IO_COMMAND* pCommands;
  DWORD dwCommands; /* nr. of DMUS_IO_COMMAND structures in pCommands */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicCommandTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicCommandTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicCommandTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicCommandTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicCommandTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicCommandTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicCommandTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicCommandTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicCommandTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicCommandTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicCommandTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicCommandTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicCommandTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicCommandTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicCommandTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicCommandTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicCommandTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicCommandTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicCommandTrackStream implementation structure
 */
struct IDirectMusicCommandTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicCommandTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicCommandTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicCommandTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicCommandTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicCommandTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicCommandTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicCommandTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicCommandTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicCommandTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicMelodyFormulationTrack implementation structure
 */
struct IDirectMusicMelodyFormulationTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicMelodyFormulationTrack fields */
  IDirectMusicMelodyFormulationTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicMelodyFormulationTrackStream implementation structure
 */
struct IDirectMusicMelodyFormulationTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicMelodyFormulationTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicMelodyFormulationTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicMelodyFormulationTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicMelodyFormulationTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicMotifTrack implementation structure
 */
struct IDirectMusicMotifTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicMotifTrack fields */
  IDirectMusicMotifTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMotifTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMotifTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMotifTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicMotifTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMotifTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMotifTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMotifTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMotifTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMotifTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMotifTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMotifTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMotifTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMotifTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicMotifTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMotifTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMotifTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMotifTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMotifTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicMotifTrackStream implementation structure
 */
struct IDirectMusicMotifTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicMotifTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicMotifTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicMotifTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicMotifTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicMotifTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMotifTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMotifTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMotifTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMotifTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicMuteTrack implementation structure
 */
struct IDirectMusicMuteTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicMuteTrack fields */
  IDirectMusicMuteTrackStream* pStream;
  DMUS_IO_MUTE* pMutes;
  DWORD dwMutes; /* nr. of DMUS_IO_MUTE structures in pMutes */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMuteTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicMuteTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicMuteTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicMuteTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicMuteTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMuteTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicMuteTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMuteTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicMuteTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicMuteTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicMuteTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMuteTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicMuteTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicMuteTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicMuteTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMuteTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicMuteTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicMuteTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicMuteTrackStream implementation structure
 */
struct IDirectMusicMuteTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicMuteTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicMuteTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicMuteTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicMuteTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicMuteTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicMuteTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicMuteTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicMuteTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicMuteTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicStyleTrack implementation structure
 */
struct IDirectMusicStyleTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicStyleTrack fields */
  IDirectMusicStyleTrackStream* pStream;
  /* FIXME: implement it with HeapReAlloc */
  DWORD pStampTimes[255]; /* stamp times for styles */
  IDirectMusicStyle8Impl* ppStyles[255];
  DWORD dwStyles; /* nr. of IDirectMusicStyle8Impl* and DWORD */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicStyleTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicStyleTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicStyleTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicStyleTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicStyleTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicStyleTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicStyleTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicStyleTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicStyleTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicStyleTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicStyleTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicStyleTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicStyleTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicStyleTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicStyleTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicStyleTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicStyleTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicStyleTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicStyleTrackStream implementation structure
 */
struct IDirectMusicStyleTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicStyleTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicStyleTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicStyleTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicStyleTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicStyleTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicStyleTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicStyleTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicStyleTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicStyleTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

static inline const char *debugstr_fourcc( DWORD fourcc )
{
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf( "\'%c%c%c%c\'",
                             (char)(fourcc), (char)(fourcc >> 8),
                             (char)(fourcc >> 16), (char)(fourcc >> 24) );
}

#endif	/* __WINE_DMSTYLE_PRIVATE_H */
