/* DirectMusicScript Private Include
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

#ifndef __WINE_DMSCRIPT_PRIVATE_H
#define __WINE_DMSCRIPT_PRIVATE_H

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
typedef struct IDirectMusicScriptImpl IDirectMusicScriptImpl;

typedef struct IDirectMusicScriptObject IDirectMusicScriptObject;
typedef struct IDirectMusicScriptObjectStream IDirectMusicScriptObjectStream;

typedef struct IDirectMusicScriptTrack IDirectMusicScriptTrack;
typedef struct IDirectMusicScriptTrackStream IDirectMusicScriptTrackStream;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicScript) DirectMusicScript_Vtbl;

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicScriptObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicScriptObjectStream_Vtbl;

extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicScriptTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicScriptTrackStream_Vtbl;

/*****************************************************************************
 * ClassFactory
 *
 * can support IID_IDirectMusicScript
 * return always an IDirectMusicScriptImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicScript (LPCGUID lpcGUID, LPDIRECTMUSICSCRIPT* ppDMScript, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicScriptObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicScriptTrack (LPCGUID lpcGUID, LPDIRECTMUSICTRACK8* ppTrack, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicScriptImpl implementation structure
 */
struct IDirectMusicScriptImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicScript);
  DWORD          ref;

  /* IDirectMusicScriptImpl fields */
  IDirectMusicScriptObject* pObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicScriptImpl_AddRef (LPDIRECTMUSICSCRIPT iface);
extern ULONG WINAPI   IDirectMusicScriptImpl_Release (LPDIRECTMUSICSCRIPT iface);
/* IDirectMusicScript: */
extern HRESULT WINAPI IDirectMusicScriptImpl_Init (LPDIRECTMUSICSCRIPT iface, IDirectMusicPerformance* pPerformance, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszRoutineName, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT* pvarValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG* plValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, IUnknown* punkValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, REFIID riid, LPVOID* ppv, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName);


/*****************************************************************************
 * IDirectMusicScriptObject implementation structure
 */
struct IDirectMusicScriptObject
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicScriptObjectStream* pStream;
  IDirectMusicScriptImpl* pScript;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicScriptObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicScriptObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicScriptObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicScriptObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicScriptObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicScriptObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicScriptObjectStream implementation structure
 */
struct IDirectMusicScriptObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicScriptObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicScriptObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicScriptObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicScriptObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicScriptObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicScriptObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicScriptObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicScriptObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicScriptObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicScriptTrack implementation structure
 */
struct IDirectMusicScriptTrack
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicTrack8);
  DWORD          ref;

  /* IDirectMusicScriptTrack fields */
  IDirectMusicScriptTrackStream* pStream;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicScriptTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicScriptTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicScriptTrack_Release (LPDIRECTMUSICTRACK8 iface);
/* IDirectMusicTrack: */
extern HRESULT WINAPI IDirectMusicScriptTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicScriptTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicScriptTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicScriptTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicScriptTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicScriptTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicScriptTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicScriptTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicScriptTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicScriptTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
/* IDirectMusicTrack8: */
extern HRESULT WINAPI IDirectMusicScriptTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicScriptTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicScriptTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicScriptTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicScriptTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);

/*****************************************************************************
 * IDirectMusicScriptTrackStream implementation structure
 */
struct IDirectMusicScriptTrackStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicScriptTrack* pParentTrack;
};

/* IUnknown: */
extern HRESULT WINAPI  IDirectMusicScriptTrackStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicScriptTrackStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicScriptTrackStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicScriptTrackStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicScriptTrackStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicScriptTrackStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicScriptTrackStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicScriptTrackStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);

#endif	/* __WINE_DMSCRIPT_PRIVATE_H */
