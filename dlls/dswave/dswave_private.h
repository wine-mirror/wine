/* DirectMusic Wave Private Include
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

#ifndef __WINE_DSWAVE_PRIVATE_H
#define __WINE_DSWAVE_PRIVATE_H

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
typedef struct IDirectMusicWaveImpl IDirectMusicWaveImpl;


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IUnknown)               DirectMusicWave_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject)     DirectMusicWave_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream)         DirectMusicWave_PersistStream_Vtbl;
extern ICOM_VTABLE(IDirectMusicSegment8)   DirectMusicWave_Segment_Vtbl;


/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicWaveImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);


/*****************************************************************************
 * IDirectMusicWaveImpl implementation structure
 */
struct IDirectMusicWaveImpl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicSegment8) *SegmentVtbl;
  ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicWaveImpl fields */
  LPDMUS_OBJECTDESC pDesc;

};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicWaveImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicWaveImpl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicSegment(8): */
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_QueryInterface (LPDIRECTMUSICSEGMENT8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicWaveImpl_IDirectMusicSegment8_AddRef (LPDIRECTMUSICSEGMENT8 iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IDirectMusicSegment8_Release (LPDIRECTMUSICSEGMENT8 iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtLength);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetLength (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtLength);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwRepeats);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetRepeats (LPDIRECTMUSICSEGMENT8 iface, DWORD dwRepeats);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD* pdwResolution);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetDefaultResolution (LPDIRECTMUSICSEGMENT8 iface, DWORD dwResolution);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetTrack (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, IDirectMusicTrack** ppTrack);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetTrackGroup (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD* pdwGroupBits);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_InsertTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack, DWORD dwGroupBits);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_RemoveTrack (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicTrack* pTrack);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_InitPlay (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicSegmentState** ppSegState, IDirectMusicPerformance* pPerformance, DWORD dwFlags);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph** ppGraph);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetGraph (LPDIRECTMUSICSEGMENT8 iface, IDirectMusicGraph* pGraph);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_AddNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_RemoveNotificationType (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidNotificationType);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, void* pParam);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetParam (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidType, DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void* pParam);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_Clone (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicSegment** ppSegment);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetStartPoint (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetLoopPoints (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME* pmtStart, MUSIC_TIME* pmtEnd);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetPChannelsUsed (LPDIRECTMUSICSEGMENT8 iface, DWORD dwNumPChannels, DWORD* paPChannels);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_SetTrackConfig (LPDIRECTMUSICSEGMENT8 iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_GetAudioPathConfig (LPDIRECTMUSICSEGMENT8 iface, IUnknown** ppAudioPathConfig);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_Compose (LPDIRECTMUSICSEGMENT8 iface, MUSIC_TIME mtTime, IDirectMusicSegment* pFromSegment, IDirectMusicSegment* pToSegment, IDirectMusicSegment** ppComposedSegment);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_Download (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicSegment8_Unload (LPDIRECTMUSICSEGMENT8 iface, IUnknown *pAudioPath);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicWaveImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicWaveImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


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

#endif	/* __WINE_DSWAVE_PRIVATE_H */
