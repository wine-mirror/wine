/* DirectMusicBand Private Include
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

#ifndef __WINE_DMBAND_PRIVATE_H
#define __WINE_DMBAND_PRIVATE_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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
typedef struct IDirectMusicBandImpl IDirectMusicBandImpl;
	
typedef struct IDirectMusicBandTrack IDirectMusicBandTrack;
	
/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IUnknown)           DirectMusicBand_Uknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicBand)   DirectMusicBand_Band_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicBand_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicBand_PeristStream_Vtbl;

extern ICOM_VTABLE(IUnknown)           DirectMusicBandTrack_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicTrack8) DirectMusicBandTrack_DirectMusicTrack_Vtbl;
extern ICOM_VTABLE(IPersistStream)     DirectMusicBandTrack_PersistStream_Vtbl;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicBandImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicBandTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);


/*****************************************************************************
 * Auxiliary definitions
 */
/* i don't like M$'s idea about two different band item headers, so behold: universal one */
typedef struct _DMUS_PRIVATE_BAND_ITEM_HEADER {
	DWORD dwVersion; /* 1 or 2 */
	/* v.1 */
	MUSIC_TIME lBandTime;
	/* v.2 */
	MUSIC_TIME lBandTimeLogical;
	MUSIC_TIME lBandTimePhysical;
} DMUS_PRIVATE_BAND_ITEM_HEADER;

typedef struct _DMUS_PRIVATE_INSTRUMENT {
	struct list entry; /* for listing elements */
	DMUS_IO_INSTRUMENT pInstrument;
	IDirectMusicCollection* ppReferenceCollection;
} DMUS_PRIVATE_INSTRUMENT, *LPDMUS_PRIVATE_INSTRUMENT;

typedef struct _DMUS_PRIVATE_BAND {
	struct list entry; /* for listing elements */
	DMUS_PRIVATE_BAND_ITEM_HEADER pBandHeader;
	IDirectMusicBandImpl* ppBand;
} DMUS_PRIVATE_BAND, *LPDMUS_PRIVATE_BAND;


/*****************************************************************************
 * IDirectMusicBandImpl implementation structure
 */
struct IDirectMusicBandImpl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicBand) *BandVtbl;
  ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicBandImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  /* data */
  struct list Instruments;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandImpl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicBandImpl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicBand: */
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_QueryInterface (LPDIRECTMUSICBAND iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandImpl_IDirectMusicBand_AddRef (LPDIRECTMUSICBAND iface);
extern ULONG WINAPI   IDirectMusicBandImpl_IDirectMusicBand_Release (LPDIRECTMUSICBAND iface);
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_CreateSegment (LPDIRECTMUSICBAND iface, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_Download (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance);
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicBand_Unload (LPDIRECTMUSICBAND iface, IDirectMusicPerformance* pPerformance);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicBandImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface);
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicBandImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicBandImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicBandImpl_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicBandImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * IDirectMusicBandTrack implementation structure
 */
struct IDirectMusicBandTrack {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicTrack8) *TrackVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicBandTrack fields */
  LPDMUS_OBJECTDESC pDesc;
  DMUS_IO_BAND_TRACK_HEADER* pHeader;
	
  /* data */
  struct list Bands;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicBandTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandTrack_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicBandTrack_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicTrack(8): */
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_QueryInterface (LPDIRECTMUSICTRACK8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicBandTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface);
extern ULONG WINAPI   IDirectMusicBandTrack_IDirectMusicTrack_Release (LPDIRECTMUSICTRACK8 iface);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Init (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegment* pSegment);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_InitPlay (LPDIRECTMUSICTRACK8 iface, IDirectMusicSegmentState* pSegmentState, IDirectMusicPerformance* pPerformance, void** ppStateData, DWORD dwVirtualTrackID, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_EndPlay (LPDIRECTMUSICTRACK8 iface, void* pStateData);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Play (LPDIRECTMUSICTRACK8 iface, void* pStateData, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_GetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_SetParam (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_IsParamSupported (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_AddNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_RemoveNotificationType (LPDIRECTMUSICTRACK8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Clone (LPDIRECTMUSICTRACK8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_PlayEx (LPDIRECTMUSICTRACK8 iface, void* pStateData, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags, IDirectMusicPerformance* pPerf, IDirectMusicSegmentState* pSegSt, DWORD dwVirtualID);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_GetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME* prtNext, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_SetParamEx (LPDIRECTMUSICTRACK8 iface, REFGUID rguidType, REFERENCE_TIME rtTime, void* pParam, void* pStateData, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Compose (LPDIRECTMUSICTRACK8 iface, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
extern HRESULT WINAPI IDirectMusicBandTrack_IDirectMusicTrack_Join (LPDIRECTMUSICTRACK8 iface, IDirectMusicTrack* pNewTrack, MUSIC_TIME mtJoin, IUnknown* pContext, DWORD dwTrackGroup, IDirectMusicTrack** ppResultTrack);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicBandTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicBandTrack_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicBandTrack_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _DMUS_PRIVATE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} DMUS_PRIVATE_CHUNK, *LPDMUS_PRIVATE_CHUNK;

/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

typedef struct {
    const GUID *guid;
    const char* name;
} guid_info;

/* used for initialising structs (primarily for DMUS_OBJECTDESC) */
#define DM_STRUCT_INIT(x) 				\
	do {								\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);		\
	} while (0)

#define FE(x) { x, #x }
#define GE(x) { &x, #x }

/* check whether the given DWORD is even (return 0) or odd (return 1) */
extern int even_or_odd (DWORD number);
/* FOURCC to string conversion for debug messages */
extern const char *debugstr_fourcc (DWORD fourcc);
/* DMUS_VERSION struct to string conversion for debug messages */
extern const char *debugstr_dmversion (LPDMUS_VERSION version);
/* returns name of given GUID */
extern const char *debugstr_dmguid (const GUID *id);
/* returns name of given error code */
extern const char *debugstr_dmreturn (DWORD code);
/* generic flags-dumping function */
extern const char *debugstr_flags (DWORD flags, const flag_info* names, size_t num_names);
extern const char *debugstr_DMUS_OBJ_FLAGS (DWORD flagmask);
/* dump whole DMUS_OBJECTDESC struct */
extern const char *debugstr_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc);

#endif	/* __WINE_DMBAND_PRIVATE_H */
