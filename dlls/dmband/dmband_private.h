/* DirectMusicBand Private Include
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

#ifndef __WINE_DMBAND_PRIVATE_H
#define __WINE_DMBAND_PRIVATE_H

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

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT create_dmband(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT create_dmbandtrack(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;


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
	DMUS_PRIVATE_BAND_ITEM_HEADER BandHeader;
	IDirectMusicBand *band;
} DMUS_PRIVATE_BAND, *LPDMUS_PRIVATE_BAND;


/**********************************************************************
 * Dll lifetime tracking declaration for dmband.dll
 */
extern LONG DMBAND_refCount DECLSPEC_HIDDEN;
static inline void DMBAND_LockModule(void) { InterlockedIncrement( &DMBAND_refCount ); }
static inline void DMBAND_UnlockModule(void) { InterlockedDecrement( &DMBAND_refCount ); }

/*****************************************************************************
 * Misc.
 */

#include "dmutils.h"

#endif	/* __WINE_DMBAND_PRIVATE_H */
