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
#include "winreg.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"
#include "dmusicc.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicGraphImpl IDirectMusicGraphImpl;
typedef struct IDirectMusicAudioPathImpl IDirectMusicAudioPathImpl;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT create_dmperformance(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmsegment(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmsegmentstate(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmgraph(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmaudiopath(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;

extern HRESULT create_dmlyricstrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmmarkertrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmparamcontroltrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmsegtriggertrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmseqtrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmsysextrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmtempotrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmtimesigtrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmwavetrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;

extern void set_audiopath_perf_pointer(IDirectMusicAudioPath*,IDirectMusicPerformance8*) DECLSPEC_HIDDEN;
extern void set_audiopath_dsound_buffer(IDirectMusicAudioPath*,IDirectSoundBuffer*) DECLSPEC_HIDDEN;
extern void set_audiopath_primary_dsound_buffer(IDirectMusicAudioPath*,IDirectSoundBuffer*) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Auxiliary definitions
 */
typedef struct _DMUS_PRIVATE_SEGMENT_TRACK {
  struct list entry; /* for listing elements */
  DWORD dwGroupBits;
  DWORD flags;
  IDirectMusicTrack* pTrack;
} DMUS_PRIVATE_SEGMENT_TRACK, *LPDMUS_PRIVATE_SEGMENT_TRACK;

typedef struct _DMUS_PRIVATE_TEMPO_ITEM {
  struct list entry; /* for listing elements */
  DMUS_IO_TEMPO_ITEM item;
} DMUS_PRIVATE_TEMPO_ITEM, *LPDMUS_PRIVATE_TEMPO_ITEM;

typedef struct _DMUS_PRIVATE_GRAPH_TOOL {
  struct list entry; /* for listing elements */
  DWORD dwIndex;
  IDirectMusicTool* pTool;
} DMUS_PRIVATE_GRAPH_TOOL, *LPDMUS_PRIVATE_GRAPH_TOOL;

typedef struct _DMUS_PRIVATE_TEMPO_PLAY_STATE {
  DWORD dummy;
} DMUS_PRIVATE_TEMPO_PLAY_STATE, *LPDMUS_PRIVATE_TEMPO_PLAY_STATE;

/**********************************************************************
 * Dll lifetime tracking declaration for dmime.dll
 */
extern LONG DMIME_refCount DECLSPEC_HIDDEN;
static inline void DMIME_LockModule(void) { InterlockedIncrement( &DMIME_refCount ); }
static inline void DMIME_UnlockModule(void) { InterlockedDecrement( &DMIME_refCount ); }

/*****************************************************************************
 * Misc.
 */

#endif	/* __WINE_DMIME_PRIVATE_H */
