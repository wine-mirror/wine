/* DirectMusic Wave Private Include
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

#ifndef __WINE_DSWAVE_PRIVATE_H
#define __WINE_DSWAVE_PRIVATE_H

#include <stdarg.h>

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
 * ClassFactory
 */
extern HRESULT WINAPI create_dswave(REFIID lpcGUID, void **ret_iface) DECLSPEC_HIDDEN;

/**********************************************************************
 * Dll lifetime tracking declaration for dswave.dll
 */
extern LONG DSWAVE_refCount DECLSPEC_HIDDEN;
static inline void DSWAVE_LockModule(void) { InterlockedIncrement( &DSWAVE_refCount ); }
static inline void DSWAVE_UnlockModule(void) { InterlockedDecrement( &DSWAVE_refCount ); }

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

#define FE(x) { x, #x }	
#define GE(x) { &x, #x }

/* FOURCC to string conversion for debug messages */
extern const char *debugstr_fourcc (DWORD fourcc) DECLSPEC_HIDDEN;
/* returns name of given GUID */
extern const char *debugstr_dmguid (const GUID *id) DECLSPEC_HIDDEN;
/* dump whole DMUS_OBJECTDESC struct */
extern const char *debugstr_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) DECLSPEC_HIDDEN;

#endif	/* __WINE_DSWAVE_PRIVATE_H */
