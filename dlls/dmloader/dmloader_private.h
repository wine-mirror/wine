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
extern LONG dwDirectMusicLoader; /* number of DirectMusicLoader(CF) instances */
extern LONG dwDirectMusicContainer; /* number of DirectMusicContainer(CF) instances */

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoaderCF             IDirectMusicLoaderCF;
typedef struct IDirectMusicContainerCF          IDirectMusicContainerCF;

typedef struct IDirectMusicLoaderImpl           IDirectMusicLoaderImpl;
typedef struct IDirectMusicContainerImpl        IDirectMusicContainerImpl;

typedef struct IDirectMusicLoaderFileStream     IDirectMusicLoaderFileStream;
typedef struct IDirectMusicLoaderResourceStream IDirectMusicLoaderResourceStream;
typedef struct IDirectMusicLoaderGenericStream  IDirectMusicLoaderGenericStream;

/*****************************************************************************
 * Creation helpers
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderCF (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicContainerCF (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_DestroyDirectMusicLoaderImpl (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicContainerImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_DestroyDirectMusicContainerImpl(LPDIRECTMUSICCONTAINER iface);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderFileStream (LPVOID *ppobj);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderResourceStream (LPVOID *ppobj);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderGenericStream (LPVOID *ppobj);

/*****************************************************************************
 * IDirectMusicLoaderCF implementation structure
 */
struct IDirectMusicLoaderCF {
	/* IUnknown fields */
	const IClassFactoryVtbl *lpVtbl;
	LONG dwRef;
};

/*****************************************************************************
 * IDirectMusicContainerCF implementation structure
 */
struct IDirectMusicContainerCF {
	/* IUnknown fields */
	const IClassFactoryVtbl *lpVtbl;
	LONG dwRef;
};

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
	/* VTABLEs */
	const IDirectMusicLoader8Vtbl *LoaderVtbl;
	/* reference counter */
	LONG dwRef;	
	/* simple cache (linked list) */
	struct list *pObjects;
	/* settings for certain object classes */
	struct list *pClassSettings;
	/* critical section */
	CRITICAL_SECTION CritSect;
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
 * IDirectMusicContainerImpl implementation structure
 */
struct IDirectMusicContainerImpl {
	/* VTABLEs */
	const IDirectMusicContainerVtbl *ContainerVtbl;
	const IDirectMusicObjectVtbl *ObjectVtbl;
	const IPersistStreamVtbl *PersistStreamVtbl;
	/* reference counter */
	LONG dwRef;
	/* stream */
	LPSTREAM pStream;
	/* header */
	DMUS_IO_CONTAINER_HEADER Header;
	/* data */
	struct list *pContainedObjects;	
	/* descriptor */
	DMUS_OBJECTDESC Desc;
};

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
extern void    WINAPI IDirectMusicLoaderFileStream_Detach (LPSTREAM iface);

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
extern void    WINAPI IDirectMusicLoaderResourceStream_Detach (LPSTREAM iface);

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
extern void    WINAPI IDirectMusicLoaderGenericStream_Detach (LPSTREAM iface);

/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _WINE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} WINE_CHUNK, *LPWINE_CHUNK;

extern HRESULT WINAPI DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH]);
extern HRESULT WINAPI DMUSIC_GetLoaderSettings (LPDIRECTMUSICLOADER8 iface, REFGUID pClassID, WCHAR* wszSearchPath, LPBOOL pbCache);
extern HRESULT WINAPI DMUSIC_InitLoaderSettings (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI DMUSIC_CopyDescriptor (LPDMUS_OBJECTDESC pDst, LPDMUS_OBJECTDESC pSrc);
extern BOOL WINAPI DMUSIC_IsValidLoadableClass (REFCLSID pClassID);

#include "debug.h"

#endif	/* __WINE_DMLOADER_PRIVATE_H */
