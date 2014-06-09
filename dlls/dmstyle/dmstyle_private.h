/* DirectMusicStyle Private Include
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

#ifndef __WINE_DMSTYLE_PRIVATE_H
#define __WINE_DMSTYLE_PRIVATE_H

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

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicAuditionTrack IDirectMusicAuditionTrack;
typedef struct IDirectMusicChordTrack IDirectMusicChordTrack;
typedef struct IDirectMusicCommandTrack IDirectMusicCommandTrack;
typedef struct IDirectMusicMelodyFormulationTrack IDirectMusicMelodyFormulationTrack;
typedef struct IDirectMusicMotifTrack IDirectMusicMotifTrack;
typedef struct IDirectMusicMuteTrack IDirectMusicMuteTrack;
typedef struct IDirectMusicStyleTrack IDirectMusicStyleTrack;
	
/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI create_dmstyle(REFIID lpcGUID, LPVOID* ppobj) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmauditiontrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmchordtrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmcommandtrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmmotiftrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmmutetrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmstyletrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Auxiliary definitions
 */
typedef struct _DMUS_PRIVATE_STYLE_BAND {
  struct list entry; /* for listing elements */
  IDirectMusicBand* pBand;
} DMUS_PRIVATE_STYLE_BAND, *LPDMUS_PRIVATE_STYLE_BAND;

typedef struct _DMUS_PRIVATE_STYLE_PARTREF_ITEM {
  struct list entry; /* for listing elements */
  DMUS_OBJECTDESC desc;
  DMUS_IO_PARTREF part_ref;
} DMUS_PRIVATE_STYLE_PARTREF_ITEM, *LPDMUS_PRIVATE_STYLE_PARTREF_ITEM;

typedef struct _DMUS_PRIVATE_STYLE_MOTIF {
  struct list entry; /* for listing elements */
  DWORD dwRhythm;
  DMUS_IO_PATTERN pattern;
  DMUS_OBJECTDESC desc;
  /** optional for motifs */
  DMUS_IO_MOTIFSETTINGS settings;
  IDirectMusicBand* pBand;

  struct list Items;
} DMUS_PRIVATE_STYLE_MOTIF, *LPDMUS_PRIVATE_STYLE_MOTIF;

typedef struct _DMUS_PRIVATE_STYLE_ITEM {
  struct list entry; /* for listing elements */
  DWORD dwTimeStamp;
  IDirectMusicStyle8* pObject;
} DMUS_PRIVATE_STYLE_ITEM, *LPDMUS_PRIVATE_STYLE_ITEM;


/*****************************************************************************
 * IDirectMusicAuditionTrack implementation structure
 */
struct IDirectMusicAuditionTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicAuditionTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicChordTrack implementation structure
 */
struct IDirectMusicChordTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicChordTrack fields */
  LPDMUS_OBJECTDESC pDesc;
  DWORD dwScale;
};

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
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicCommandTrack fields */
  LPDMUS_OBJECTDESC pDesc;
  /* track data */
  struct list Commands;
};

/*****************************************************************************
 * IDirectMusicMelodyFormulationTrack implementation structure
 */
struct IDirectMusicMelodyFormulationTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicMelodyFormulationTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicMelodyFormulationTrack_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IUnknown_AddRef (LPUNKNOWN iface) DECLSPEC_HIDDEN;
/* IDirectMusicTrack(8): */
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IDirectMusicTrack_AddRef (LPDIRECTMUSICTRACK8 iface) DECLSPEC_HIDDEN;
/* IPersistStream: */
extern ULONG WINAPI   IDirectMusicMelodyFormulationTrack_IPersistStream_AddRef (LPPERSISTSTREAM iface) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectMusicMotifTrack implementation structure
 */
struct IDirectMusicMotifTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicMotifTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicMuteTrack implementation structure
 */
struct IDirectMusicMuteTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicMuteTrack fields */
  LPDMUS_OBJECTDESC pDesc;
};

/*****************************************************************************
 * IDirectMusicStyleTrack implementation structure
 */
struct IDirectMusicStyleTrack {
  /* IUnknown fields */
  const IUnknownVtbl *UnknownVtbl;
  const IDirectMusicTrack8Vtbl *TrackVtbl;
  const IPersistStreamVtbl *PersistStreamVtbl;
  LONG           ref;

  /* IDirectMusicStyleTrack fields */
  LPDMUS_OBJECTDESC pDesc;
  
  struct list Items;
};

/**********************************************************************
 * Dll lifetime tracking declaration for dmstyle.dll
 */
extern LONG DMSTYLE_refCount DECLSPEC_HIDDEN;
static inline void DMSTYLE_LockModule(void) { InterlockedIncrement( &DMSTYLE_refCount ); }
static inline void DMSTYLE_UnlockModule(void) { InterlockedDecrement( &DMSTYLE_refCount ); }

/*****************************************************************************
 * Misc.
 */
#include "dmutils.h"

#endif	/* __WINE_DMSTYLE_PRIVATE_H */
