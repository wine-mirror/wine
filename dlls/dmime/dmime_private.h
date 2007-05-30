/* DirectMusicInteractiveEngine Private Include
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DMIME_PRIVATE_H
#define __WINE_DMIME_PRIVATE_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "winreg.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"
#include "dmusicc.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicPerformance8Impl IDirectMusicPerformance8Impl;
typedef struct IDirectMusicSegment8Impl IDirectMusicSegment8Impl;
typedef struct IDirectMusicSegmentState8Impl IDirectMusicSegmentState8Impl;
typedef struct IDirectMusicGraphImpl IDirectMusicGraphImpl;
typedef struct IDirectMusicAudioPathImpl IDirectMusicAudioPathImpl;
typedef struct IDirectMusicTool8Impl IDirectMusicTool8Impl;
typedef struct IDirectMusicPatternTrackImpl IDirectMusicPatternTrackImpl;

typedef struct IDirectMusicLyricsTrack IDirectMusicLyricsTrack;
typedef struct IDirectMusicMarkerTrack IDirectMusicMarkerTrack;
typedef struct IDirectMusicParamControlTrack IDirectMusicParamControlTrack;
typedef struct IDirectMusicSegTriggerTrack IDirectMusicSegTriggerTrack;
typedef struct IDirectMusicSeqTrack IDirectMusicSeqTrack;
typedef struct IDirectMusicSysExTrack IDirectMusicSysExTrack;
typedef struct IDirectMusicTempoTrack IDirectMusicTempoTrack;
typedef struct IDirectMusicTimeSigTrack IDirectMusicTimeSigTrack;
typedef struct IDirectMusicWaveTrack IDirectMusicWaveTrack;
	
/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPerformanceImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegmentStateImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicGraphImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicAudioPathImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicToolImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicPatternTrackImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicLyricsTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicMarkerTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicParamControlTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSegTriggerTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSeqTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSysExTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicTempoTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicTimeSigTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicWaveTrack (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);


/*****************************************************************************
 * Auxiliary definitions
 */
typedef struct _DMUS_PRIVATE_SEGMENT_TRACK {
  struct list entry; /* for listing elements */
  DWORD dwGroupBits;
  IDirectMusicTrack* pTrack;
} DMUS_PRIVATE_SEGMENT_TRACK, *LPDMUS_PRIVATE_SEGMENT_TRACK;

typedef struct _DMUS_PRIVATE_TEMPO_ITEM {
  struct list entry; /* for listing elements */
  DMUS_IO_TEMPO_ITEM item;
} DMUS_PRIVATE_TEMPO_ITEM, *LPDMUS_PRIVATE_TEMPO_ITEM;

typedef struct _DMUS_PRIVATE_SEGMENT_ITEM {
  struct list entry; /* for listing elements */
  DMUS_IO_SEGMENT_ITEM_HEADER header;
  IDirectMusicObject* pObject;
  WCHAR wszName[DMUS_MAX_NAME];
} DMUS_PRIVATE_SEGMENT_ITEM, *LPDMUS_PRIVATE_SEGMENT_ITEM;

typedef struct _DMUS_PRIVATE_GRAPH_TOOL {
  struct list entry; /* for listing elements */
  DWORD dwIndex;
  IDirectMusicTool* pTool;
} DMUS_PRIVATE_GRAPH_TOOL, *LPDMUS_PRIVATE_GRAPH_TOOL;

typedef struct _DMUS_PRIVATE_TEMPO_PLAY_STATE {
  DWORD dummy;
} DMUS_PRIVATE_TEMPO_PLAY_STATE, *LPDMUS_PRIVATE_TEMPO_PLAY_STATE;

/* some sort of aux. performance channel: as far as i can understand, these are 
   used to represent a particular midi channel in particular group at particular
   group; so all we need to do is to fill it with parent port, group and midi 
   channel ? */
typedef struct DMUSIC_PRIVATE_PCHANNEL_ {
	DWORD channel; /* map to this channel... */
	DWORD group; /* ... in this group ... */
	IDirectMusicPort *port; /* ... at this port */
} DMUSIC_PRIVATE_PCHANNEL, *LPDMUSIC_PRIVATE_PCHANNEL;

/*****************************************************************************
 * IDirectMusicPerformance8Impl implementation structure
 */
struct IDirectMusicPerformance8Impl {
  /* IUnknown fields */
  const IDirectMusicPerformance8Vtbl *lpVtbl;
  LONG                   ref;

  /* IDirectMusicPerformanceImpl fields */
  IDirectMusic8*         pDirectMusic;
  IDirectSound*          pDirectSound;
  IDirectMusicGraph*     pToolGraph;
  DMUS_AUDIOPARAMS       pParams;

  /* global parameters */
  BOOL  fAutoDownload;
  char  cMasterGrooveLevel;
  float fMasterTempo;
  long  lMasterVolume;
	
  /* performance channels */
  DMUSIC_PRIVATE_PCHANNEL PChannel[32];

   /* IDirectMusicPerformance8Impl fields */
  IDirectMusicAudioPath* pDefaultPath;
  HANDLE hNotification;
  REFERENCE_TIME rtMinimum;

  REFERENCE_TIME rtLatencyTime;
  DWORD dwBumperLength;
  DWORD dwPrepareTime;
  /** Message Processing */
  HANDLE         procThread;
  DWORD          procThreadId;
  REFERENCE_TIME procThreadStartTime;
  BOOL           procThreadTicStarted;
  CRITICAL_SECTION safe;
  struct DMUS_PMSGItem* head; 
  struct DMUS_PMSGItem* imm_head; 
};

/*****************************************************************************
 * IDirectMusicSegment8Impl implementation structure
 */
struct IDirectMusicSegment8Impl {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicSegment8Vtbl *SegmentVtbl;
  const IDirectMusicObjectVtbl *ObjectVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicSegment8Impl fields */
  LPDMUS_OBJECTDESC      pDesc;
  DMUS_IO_SEGMENT_HEADER header;
  IDirectMusicGraph*     pGraph; 
  struct list Tracks;
};

/*****************************************************************************
 * IDirectMusicSegmentState8Impl implementation structure
 */
struct IDirectMusicSegmentState8Impl {
  /* IUnknown fields */
  const IDirectMusicSegmentState8Vtbl *lpVtbl;
  LONG           ref;

  /* IDirectMusicSegmentState8Impl fields */
};

/*****************************************************************************
 * IDirectMusicGraphImpl implementation structure
 */
struct IDirectMusicGraphImpl {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicGraphVtbl *GraphVtbl;
  const IDirectMusicObjectVtbl *ObjectVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicGraphImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  WORD              num_tools;
  struct list       Tools;
};

/*****************************************************************************
 * IDirectMusicAudioPathImpl implementation structure
 */
struct IDirectMusicAudioPathImpl {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicAudioPathVtbl *AudioPathVtbl;
  const IDirectMusicObjectVtbl *ObjectVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicAudioPathImpl fields */
  LPDMUS_OBJECTDESC pDesc;
	
  IDirectMusicPerformance8* pPerf;
  IDirectMusicGraph*        pToolGraph;
  IDirectSoundBuffer*       pDSBuffer;
  IDirectSoundBuffer*       pPrimary;

  BOOL fActive;
};

/*****************************************************************************
 * IDirectMusicTool8Impl implementation structure
 */
struct IDirectMusicTool8Impl {
  /* IUnknown fields */
  const IDirectMusicTool8Vtbl *lpVtbl;
  LONG           ref;

  /* IDirectMusicTool8Impl fields */
};

/*****************************************************************************
 * IDirectMusicPatternTrackImpl implementation structure
 */
struct IDirectMusicPatternTrackImpl {
  /* IUnknown fields */
  const IDirectMusicPatternTrackVtbl *lpVtbl;
  LONG           ref;

  /* IDirectMusicPatternTrackImpl fields */
};

/*****************************************************************************
 * IDirectMusicLyricsTrack implementation structure
 */
struct IDirectMusicLyricsTrack
{
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicLyricsTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicMarkerTrack implementation structure
 */
struct IDirectMusicMarkerTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicMarkerTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicParamControlTrack implementation structure
 */
struct IDirectMusicParamControlTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicParamControlTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicSegTriggerTrack implementation structure
 */
struct IDirectMusicSegTriggerTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicSegTriggerTrack fields */
  LPDMUS_OBJECTDESC pDesc;

  struct list Items;
};

/*****************************************************************************
 * IDirectMusicSeqTrack implementation structure
 */
struct IDirectMusicSeqTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicSeqTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicSysExTrack implementation structure
 */
struct IDirectMusicSysExTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicSysExTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicTempoTrack implementation structure
 */
struct IDirectMusicTempoTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicTempoTrack fields */
  LPDMUS_OBJECTDESC pDesc;
  BOOL enabled;
  struct list Items;
};

/*****************************************************************************
 * IDirectMusicTimeSigTrack implementation structure
 */
struct IDirectMusicTimeSigTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicTimeSigTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicWaveTrack implementation structure
 */
struct IDirectMusicWaveTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicWaveTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/**********************************************************************
 * Dll lifetime tracking declaration for dmime.dll
 */
extern LONG DMIME_refCount;
static inline void DMIME_LockModule(void) { InterlockedIncrement( &DMIME_refCount ); }
static inline void DMIME_UnlockModule(void) { InterlockedDecrement( &DMIME_refCount ); }

/*****************************************************************************
 * Misc.
 */

#include "dmutils.h"

#endif	/* __WINE_DMIME_PRIVATE_H */
