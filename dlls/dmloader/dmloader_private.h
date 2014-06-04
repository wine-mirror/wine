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
#include "wine/unicode.h"
#include "winreg.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

/* dmloader.dll global (for DllCanUnloadNow) */
extern LONG module_ref DECLSPEC_HIDDEN;
static inline void lock_module(void) { InterlockedIncrement( &module_ref ); }
static inline void unlock_module(void) { InterlockedDecrement( &module_ref ); }

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoaderCF             IDirectMusicLoaderCF;
typedef struct IDirectMusicContainerCF          IDirectMusicContainerCF;

typedef struct IDirectMusicLoaderImpl           IDirectMusicLoaderImpl;

typedef struct IDirectMusicLoaderFileStream     IDirectMusicLoaderFileStream;
typedef struct IDirectMusicLoaderResourceStream IDirectMusicLoaderResourceStream;
typedef struct IDirectMusicLoaderGenericStream  IDirectMusicLoaderGenericStream;

/*****************************************************************************
 * Creation helpers
 */
extern HRESULT WINAPI create_dmloader(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI create_dmcontainer(REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderFileStream (LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderResourceStream (LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderGenericStream (LPVOID *ppobj) DECLSPEC_HIDDEN;

/* cache/alias entry */
typedef struct _WINE_LOADER_ENTRY {
	struct list entry; /* for listing elements */
	DMUS_OBJECTDESC Desc;
    LPDIRECTMUSICOBJECT pObject; /* pointer to object */
	BOOL bInvalidDefaultDLS; /* my workaround for enabling caching of "faulty" default dls collection */
} WINE_LOADER_ENTRY, *LPWINE_LOADER_ENTRY;

/* cache options, search paths for specific types of objects */
typedef struct _WINE_LOADER_OPTION {
	struct list entry; /* for listing elements */
	GUID guidClass; /* ID of object type */
	WCHAR wszSearchPath[MAX_PATH]; /* look for objects of certain type in here */
	BOOL bCache; /* cache objects of certain type */
} WINE_LOADER_OPTION, *LPWINE_LOADER_OPTION;

/*****************************************************************************
 * IDirectMusicLoaderImpl implementation structure
 */
struct IDirectMusicLoaderImpl {
    IDirectMusicLoader8 IDirectMusicLoader8_iface;
    LONG ref;
    /* simple cache (linked list) */
    struct list *pObjects;
    /* settings for certain object classes */
    struct list *pClassSettings;
};

/* contained object entry */
typedef struct _WINE_CONTAINER_ENTRY {
	struct list entry; /* for listing elements */
	DMUS_OBJECTDESC Desc;
	BOOL bIsRIFF;
	DWORD dwFlags; /* DMUS_CONTAINED_OBJF_KEEP: keep object in loader's cache, even when container is released */
	WCHAR* wszAlias;
	LPDIRECTMUSICOBJECT pObject; /* needed when releasing from loader's cache on container release */
} WINE_CONTAINER_ENTRY, *LPWINE_CONTAINER_ENTRY;

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
extern HRESULT WINAPI IDirectMusicLoaderFileStream_Attach (LPSTREAM iface, LPCWSTR wzFile, LPDIRECTMUSICLOADER8 pLoader) DECLSPEC_HIDDEN;

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
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_Attach (LPSTREAM iface, LPBYTE pbMemData, LONGLONG llMemLength, LONGLONG llPos, LPDIRECTMUSICLOADER8 pLoader) DECLSPEC_HIDDEN;

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
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_Attach (LPSTREAM iface, LPSTREAM pStream, LPDIRECTMUSICLOADER8 pLoader) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _WINE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} WINE_CHUNK, *LPWINE_CHUNK;

#include "debug.h"

#endif	/* __WINE_DMLOADER_PRIVATE_H */
