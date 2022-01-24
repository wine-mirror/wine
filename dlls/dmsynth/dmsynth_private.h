/* DirectMusicSynthesizer Private Include
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

#ifndef __WINE_DMSYNTH_PRIVATE_H
#define __WINE_DMSYNTH_PRIVATE_H

#include <stdarg.h>

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
#include "dmksctrl.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicSynth8Impl IDirectMusicSynth8Impl;
typedef struct IDirectMusicSynthSinkImpl IDirectMusicSynthSinkImpl;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSynthImpl(REFIID riid, void **ppobj) DECLSPEC_HIDDEN;
extern HRESULT WINAPI DMUSIC_CreateDirectMusicSynthSinkImpl(REFIID riid, void **ppobj) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectMusicSynth8Impl implementation structure
 */
struct IDirectMusicSynth8Impl {
    /* IUnknown fields */
    IDirectMusicSynth8 IDirectMusicSynth8_iface;
    IKsControl IKsControl_iface;
    LONG ref;

    /* IDirectMusicSynth8 fields */
    DMUS_PORTCAPS caps;
    BOOL active;
    IReferenceClock *latency_clock;
    IDirectMusicSynthSink *sink;
};

/*****************************************************************************
 * IDirectMusicSynthSinkImpl implementation structure
 */
struct IDirectMusicSynthSinkImpl {
    IDirectMusicSynthSink IDirectMusicSynthSink_iface;
    IKsControl IKsControl_iface;
    LONG ref;
    IReferenceClock *latency_clock;
    IReferenceClock *master_clock;
    IDirectMusicSynth *synth;   /* No reference hold! */
    BOOL active;
};

/**********************************************************************
 * Dll lifetime tracking declaration for dmsynth.dll
 */
extern LONG DMSYNTH_refCount DECLSPEC_HIDDEN;
static inline void DMSYNTH_LockModule(void) { InterlockedIncrement( &DMSYNTH_refCount ); }
static inline void DMSYNTH_UnlockModule(void) { InterlockedDecrement( &DMSYNTH_refCount ); }

/*****************************************************************************
 * Misc.
 */
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
#define FE(x) { x, #x }	
#define GE(x) { &x, #x }

/* returns name of given GUID */
extern const char *debugstr_dmguid (const GUID *id) DECLSPEC_HIDDEN;

#endif	/* __WINE_DMSYNTH_PRIVATE_H */
