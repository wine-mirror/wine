/* DirectMusicComposer Private Include
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

#ifndef __WINE_DMCOMPOS_PRIVATE_H
#define __WINE_DMCOMPOS_PRIVATE_H

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
typedef struct IDirectMusicChordMapImpl IDirectMusicChordMapImpl;
typedef struct IDirectMusicComposerImpl IDirectMusicComposerImpl;

typedef struct IDirectMusicChordMapObject IDirectMusicChordMapObject;
typedef struct IDirectMusicChordMapObjectStream IDirectMusicChordMapObjectStream;

typedef struct IDirectMusicChordMapTrack IDirectMusicChordMapTrack;
typedef struct IDirectMusicChordMapTrackStream IDirectMusicChordMapTrackStream;
typedef struct IDirectMusicSignPostTrack IDirectMusicSignPostTrack;
typedef struct IDirectMusicSignPostTrackStream IDirectMusicSignPostTrackStream;
	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicChordMap) DirectMusicChordMap_Vtbl;
extern ICOM_VTABLE(IDirectMusicComposer) DirectMusicComposer_Vtbl;

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicChordMapObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicChordMapObjectStream_Vtbl;

extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicChordMapTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicChordMapTrackStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicSignPostTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicSignPostTrackStream_Vtbl;

/*****************************************************************************
 * ClassFactory
 */

/* can support IID_IDirectMusicChordMap
 * return always an IDirectMusicChordMapImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicChordMap (LPCGUID lpcGUID, LPDIRECTMUSICCHORDMAP* ppDMCM, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicComposer
 * return always an IDirectMusicComposerImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicComposer (LPCGUID lpcGUID, LPDIRECTMUSICCOMPOSER* ppDMCP, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicChordMapObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicChordMapTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSignPostTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicChordMapImpl implementation structure
 */
struct IDirectMusicChordMapImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicChordMap);
  DWORD ref;

  /* IDirectMusicChordMapImpl fields */
  IDirectMusicChordMapObject* pObject;
  DWORD dwScale;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordMapImpl_QueryInterface (LPDIRECTMUSICCHORDMAP iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordMapImpl_AddRef (LPDIRECTMUSICCHORDMAP iface);
extern ULONG WINAPI   IDirectMusicChordMapImpl_Release (LPDIRECTMUSICCHORDMAP iface);
/* IDirectMusicChordMap: */
extern HRESULT WINAPI IDirectMusicChordMapImpl_GetScale (LPDIRECTMUSICCHORDMAP iface, DWORD* pdwScale);

/*****************************************************************************
 * IDirectMusicComposerImpl implementation structure
 */
struct IDirectMusicComposerImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicComposer);
  DWORD ref;

  /* IDirectMusicComposerImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicComposerImpl_QueryInterface (LPDIRECTMUSICCOMPOSER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicComposerImpl_AddRef (LPDIRECTMUSICCOMPOSER iface);
extern ULONG WINAPI   IDirectMusicComposerImpl_Release (LPDIRECTMUSICCOMPOSER iface);
/* IDirectMusicComposer: */
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromTemplate (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, IDirectMusicSegment* pTemplate, WORD wActivity, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromShape (LPDIRECTMUSICCOMPOSER iface, IDirectMusicStyle* pStyle, WORD wNumMeasures, WORD wShape, WORD wActivity, BOOL fIntro, BOOL fEnd, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pFromSeg, IDirectMusicSegment* pToSeg, MUSIC_TIME mtTime, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg);
extern HRESULT WINAPI IDirectMusicComposerImpl_AutoTransition (LPDIRECTMUSICCOMPOSER iface, IDirectMusicPerformance* pPerformance, IDirectMusicSegment* pToSeg, WORD wCommand, DWORD dwFlags, IDirectMusicChordMap* pChordMap, IDirectMusicSegment** ppTransSeg, IDirectMusicSegmentState** ppToSegState, IDirectMusicSegmentState** ppTransSegState);
extern HRESULT WINAPI IDirectMusicComposerImpl_ComposeTemplateFromShape (LPDIRECTMUSICCOMPOSER iface, WORD wNumMeasures, WORD wShape, BOOL fIntro, BOOL fEnd, WORD wEndLength, IDirectMusicSegment** ppTemplate);
extern HRESULT WINAPI IDirectMusicComposerImpl_ChangeChordMap (LPDIRECTMUSICCOMPOSER iface, IDirectMusicSegment* pSegment, BOOL fTrackScale, IDirectMusicChordMap* pChordMap);


/*****************************************************************************
 * IDirectMusicChordMapObject implementation structure
 */
struct IDirectMusicChordMapObject
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicChordMapObjectStream* pStream;
  IDirectMusicChordMapImpl* pChordMap;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordMapObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordMapObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicChordMapObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicChordMapObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicChordMapObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicChordMapObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicChordMapObjectStream implementation structure
 */
struct IDirectMusicChordMapObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicChordMapObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordMapObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicChordMapObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicChordMapObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicChordMapObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicChordMapObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicChordMapObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicChordMapObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicChordMapObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicChordMapTrack implementation structure
 */
struct IDirectMusicChordMapTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicChordMapTrack fields */
  IDirectMusicChordMapTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicChordMapTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicChordMapTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicChordMapTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicChordMapTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicChordMapTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordMapTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicChordMapTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicChordMapTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicChordMapTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicChordMapTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicChordMapTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicChordMapTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicChordMapTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicChordMapTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicChordMapTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordMapTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicChordMapTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicChordMapTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicChordMapTrackStream implementation structure
 */
struct IDirectMusicChordMapTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicChordMapTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicChordMapTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicChordMapTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicChordMapTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicChordMapTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicChordMapTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicChordMapTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicChordMapTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicChordMapTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicSignPostTrack implementation structure
 */
struct IDirectMusicSignPostTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicSignPostTrack fields */
  IDirectMusicSignPostTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicSignPostTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicSignPostTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicSignPostTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicSignPostTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicSignPostTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSignPostTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicSignPostTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSignPostTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicSignPostTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicSignPostTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicSignPostTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSignPostTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicSignPostTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicSignPostTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicSignPostTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSignPostTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicSignPostTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicSignPostTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicSignPostTrackStream implementation structure
 */
struct IDirectMusicSignPostTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicSignPostTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicSignPostTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicSignPostTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicSignPostTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicSignPostTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicSignPostTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicSignPostTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicSignPostTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicSignPostTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

#endif	/* __WINE_DMCOMPOS_PRIVATE_H */
