/* DirectMusicLoader Private Include
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

#ifndef __WINE_DMLOADER_PRIVATE_H
#define __WINE_DMLOADER_PRIVATE_H

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
#include "dmobject.h"

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

/* dmloader.dll global (for DllCanUnloadNow) */
extern LONG module_ref;
static inline void lock_module(void) { InterlockedIncrement( &module_ref ); }
static inline void unlock_module(void) { InterlockedDecrement( &module_ref ); }

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoaderCF             IDirectMusicLoaderCF;
typedef struct IDirectMusicContainerCF          IDirectMusicContainerCF;

typedef struct IDirectMusicLoaderFileStream     IDirectMusicLoaderFileStream;
typedef struct IDirectMusicLoaderResourceStream IDirectMusicLoaderResourceStream;
typedef struct IDirectMusicLoaderGenericStream  IDirectMusicLoaderGenericStream;

/*****************************************************************************
 * Creation helpers
 */
extern HRESULT create_dmloader(REFIID riid, void **ret_iface);
extern HRESULT create_dmcontainer(REFIID riid, void **ret_iface);
extern HRESULT DMUSIC_CreateDirectMusicLoaderFileStream(void **ppobj);
extern HRESULT DMUSIC_CreateDirectMusicLoaderResourceStream(void **ppobj);
extern HRESULT DMUSIC_CreateDirectMusicLoaderGenericStream(void **ppobj);

/*****************************************************************************
 * IDirectMusicLoaderFileStream implementation structure
 */
struct IDirectMusicLoaderFileStream {
	/* VTABLEs */
	const IStreamVtbl *StreamVtbl;
	const IDirectMusicGetLoaderVtbl *GetLoaderVtbl;
	/* reference counter */
	LONG dwRef;
	/* file */
	WCHAR wzFileName[MAX_PATH]; /* for clone */
	HANDLE hFile;
	/* loader */
	LPDIRECTMUSICLOADER8 pLoader;
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderFileStream_Attach (LPSTREAM iface, LPCWSTR wzFile, LPDIRECTMUSICLOADER8 pLoader);

/*****************************************************************************
 * IDirectMusicLoaderResourceStream implementation structure
 */
struct IDirectMusicLoaderResourceStream {
	/* IUnknown fields */
	const IStreamVtbl *StreamVtbl;
	const IDirectMusicGetLoaderVtbl *GetLoaderVtbl;
	/* reference counter */
	LONG dwRef;
	/* data */
	LPBYTE pbMemData;
	LONGLONG llMemLength;
	/* current position */
	LONGLONG llPos;	
	/* loader */
	LPDIRECTMUSICLOADER8 pLoader;
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_Attach (LPSTREAM iface, LPBYTE pbMemData, LONGLONG llMemLength, LONGLONG llPos, LPDIRECTMUSICLOADER8 pLoader);

/*****************************************************************************
 * IDirectMusicLoaderGenericStream implementation structure
 */
struct IDirectMusicLoaderGenericStream {
	/* IUnknown fields */
	const IStreamVtbl *StreamVtbl;
	const IDirectMusicGetLoaderVtbl *GetLoaderVtbl;
	/* reference counter */
	LONG dwRef;
	/* stream */
	LPSTREAM pStream;
	/* loader */
	LPDIRECTMUSICLOADER8 pLoader;
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_Attach (LPSTREAM iface, LPSTREAM pStream, LPDIRECTMUSICLOADER8 pLoader);

#include "debug.h"

#endif	/* __WINE_DMLOADER_PRIVATE_H */
