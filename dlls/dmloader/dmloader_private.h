/* DirectMusicLoader Private Include
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

#ifndef __WINE_DMLOADER_PRIVATE_H
#define __WINE_DMLOADER_PRIVATE_H

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
 * Auxiliary definitions
 */
/* cache entry */
typedef struct _DMUS_PRIVATE_CACHE_ENTRY {
	struct list entry; /* for listing elements */
	BOOL bIsFaultyDLS; /* my workaround for enabling caching of "faulty" dls collections */
    LPDIRECTMUSICOBJECT pObject; /* pointer to object */
} DMUS_PRIVATE_CACHE_ENTRY, *LPDMUS_PRIVATE_CACHE_ENTRY;

/* alias entry */
typedef struct _DMUS_PRIVATE_ALIAS_ENTRY {
	struct list entry; /* for listing elements */
	LPDMUS_OBJECTDESC pDesc; /* descriptor, containing info */
} DMUS_PRIVATE_ALIAS_ENTRY, *LPDMUS_PRIVATE_ALIAS_ENTRY;

/* contained object entry */
typedef struct _DMUS_PRIVATE_CONTAINED_OBJECT_ENTRY {
	struct list entry; /* for listing elements */
	WCHAR* wszAlias;
	LPDMUS_OBJECTDESC pDesc;
} DMUS_PRIVATE_CONTAINED_OBJECT_ENTRY, *LPDMUS_PRIVATE_CONTAINED_OBJECT_ENTRY;

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoader8Impl IDirectMusicLoader8Impl;
typedef struct IDirectMusicContainerImpl IDirectMusicContainerImpl;

typedef struct ILoaderStream ILoaderStream;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8_Vtbl;

extern ICOM_VTABLE(IUnknown) DirectMusicContainer_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Container_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject) DirectMusicContainer_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicContainer_PersistStream_Vtbl;

extern ICOM_VTABLE(IUnknown) LoaderStream_Unknown_Vtbl;
extern ICOM_VTABLE(IStream) LoaderStream_Stream_Vtbl;
extern ICOM_VTABLE(IDirectMusicGetLoader) LoaderStream_GetLoader_Vtbl;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);
extern HRESULT WINAPI DMUSIC_CreateDirectMusicContainerImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateLoaderStream (LPVOID *ppobj);

/*****************************************************************************
 * IDirectMusicLoader8Impl implementation structure
 */
struct IDirectMusicLoader8Impl {
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicLoader8);
  DWORD          ref;

  /* IDirectMusicLoaderImpl fields */
  WCHAR wzSearchPath[MAX_PATH];
	
  /* simple cache (linked list) */
  struct list CacheList;
  struct list AliasList;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicLoader8Impl_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicLoader8Impl_AddRef (LPDIRECTMUSICLOADER8 iface);
extern ULONG WINAPI   IDirectMusicLoader8Impl_Release (LPDIRECTMUSICLOADER8 iface);
/* IDirectMusicLoader: */
extern HRESULT WINAPI IDirectMusicLoader8Impl_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID*ppv);
extern HRESULT WINAPI IDirectMusicLoader8Impl_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicLoader8Impl_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName);
extern HRESULT WINAPI IDirectMusicLoader8Impl_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass);
extern HRESULT WINAPI IDirectMusicLoader8Impl_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicLoader8Impl_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc);
/* IDirectMusicLoader8: */
extern void    WINAPI IDirectMusicLoader8Impl_CollectGarbage (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject);
extern HRESULT WINAPI IDirectMusicLoader8Impl_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClassID, REFIID iidInterfaceID, WCHAR* pwzFilePath, void** ppObject);

/*****************************************************************************
 * IDirectMusicContainerImpl implementation structure
 */
struct IDirectMusicContainerImpl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicContainer) *ContainerVtbl;
  ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicContainerImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  DMUS_IO_CONTAINER_HEADER* pHeader;

  /* list of objects */
  struct list ObjectsList;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicContainerImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG   WINAPI IDirectMusicContainerImpl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG   WINAPI IDirectMusicContainerImpl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicContainer: */
extern HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj);
extern ULONG   WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_AddRef (LPDIRECTMUSICCONTAINER iface);
extern ULONG   WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_Release (LPDIRECTMUSICCONTAINER iface);
extern HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicContainer_EnumObject (LPDIRECTMUSICCONTAINER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc, WCHAR* pwszAlias);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG   WINAPI IDirectMusicContainerImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG   WINAPI IDirectMusicContainerImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface);
extern HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicContainerImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG   WINAPI IDirectMusicContainerImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG   WINAPI IDirectMusicContainerImpl_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicContainerImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * ILoaderStream implementation structure
 */
struct ILoaderStream {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IStream) *StreamVtbl;
  ICOM_VTABLE(IDirectMusicGetLoader) *GetLoaderVtbl;
  DWORD          ref;

  /* ILoaderStream fields */
  IDirectMusicLoader8Impl* pLoader;
  HANDLE hFile;
  WCHAR wzFileName[MAX_PATH]; /* for clone */
};

/* Custom: */
extern HRESULT WINAPI ILoaderStream_Attach (LPSTREAM iface, LPCWSTR wzFile, IDirectMusicLoader *pLoader);
extern void    WINAPI ILoaderStream_Detach (LPSTREAM iface);
/* IUnknown: */
extern HRESULT WINAPI ILoaderStream_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI ILoaderStream_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG   WINAPI ILoaderStream_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI ILoaderStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG   WINAPI ILoaderStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface);
extern HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader);
/* IStream: */
extern HRESULT WINAPI ILoaderStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI ILoaderStream_IStream_AddRef (LPSTREAM iface);
extern ULONG   WINAPI ILoaderStream_IStream_Release (LPSTREAM iface);extern HRESULT WINAPI ILoaderStream_IStream_Read (IStream* iface, void* pv, ULONG cb, ULONG* pcbRead);
extern HRESULT WINAPI ILoaderStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten);
extern HRESULT WINAPI ILoaderStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition);
extern HRESULT WINAPI ILoaderStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize);
extern HRESULT WINAPI ILoaderStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten);
extern HRESULT WINAPI ILoaderStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags);
extern HRESULT WINAPI ILoaderStream_IStream_Revert (LPSTREAM iface);
extern HRESULT WINAPI ILoaderStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI ILoaderStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI ILoaderStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag);
extern HRESULT WINAPI ILoaderStream_IStream_Clone (LPSTREAM iface, IStream** ppstm);


/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _DMUS_PRIVATE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} DMUS_PRIVATE_CHUNK, *LPDMUS_PRIVATE_CHUNK;

/* check whether the given DWORD is even (return 0) or odd (return 1) */
static inline int even_or_odd (DWORD number) {
	return (number & 0x1); /* basically, check if bit 0 is set ;) */
}

/* translate STREAM_SEEK flag to string */
static inline const char *resolve_STREAM_SEEK (DWORD flag) {
	switch (flag) {
		case STREAM_SEEK_SET:
			return wine_dbg_sprintf ("STREAM_SEEK_SET");
		case STREAM_SEEK_CUR:
			return wine_dbg_sprintf ("STREAM_SEEK_CUR");
		case STREAM_SEEK_END:
			return wine_dbg_sprintf ("STREAM_SEEK_END");
		default:
			return wine_dbg_sprintf ("()");			
	}
}

/* FOURCC to string conversion for debug messages */
static inline const char *debugstr_fourcc (DWORD fourcc) {
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf ("\'%c%c%c%c\'",
		(char)(fourcc), (char)(fourcc >> 8),
        (char)(fourcc >> 16), (char)(fourcc >> 24));
}

/* DMUS_VERSION struct to string conversion for debug messages */
static inline const char *debugstr_dmversion (LPDMUS_VERSION version) {
	if (!version) return "'null'";
	return wine_dbg_sprintf ("\'%i,%i,%i,%i\'",
		(int)((version->dwVersionMS && 0xFFFF0000) >> 8), (int)(version->dwVersionMS && 0x0000FFFF), 
		(int)((version->dwVersionLS && 0xFFFF0000) >> 8), (int)(version->dwVersionLS && 0x0000FFFF));
}

/* used for initialising structs (primarily for DMUS_OBJECTDESC) */
#define DM_STRUCT_INIT(x) 				\
	do {								\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);		\
	} while (0)


/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }
#define DMUSIC_dump_flags(flags,names,num_names) DMUSIC_dump_flags_(flags, names, num_names, 1)

/* generic dump function */
static inline void DMUSIC_dump_flags_ (DWORD flags, const flag_info* names, size_t num_names, int newline) {
	unsigned int i;
	
	for (i=0; i < num_names; i++) {
		if ((flags & names[i].val) ||      /* standard flag value */
		((!flags) && (!names[i].val))) /* zero value only */
	    	DPRINTF("%s ", names[i].name);
	}
	
    if (newline) DPRINTF("\n");
}

static inline void DMUSIC_dump_DMUS_OBJ_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_OBJ_OBJECT),
	    FE(DMUS_OBJ_CLASS),
	    FE(DMUS_OBJ_NAME),
	    FE(DMUS_OBJ_CATEGORY),
	    FE(DMUS_OBJ_FILENAME),
	    FE(DMUS_OBJ_FULLPATH),
	    FE(DMUS_OBJ_URL),
	    FE(DMUS_OBJ_VERSION),
	    FE(DMUS_OBJ_DATE),
	    FE(DMUS_OBJ_LOADED),
	    FE(DMUS_OBJ_MEMORY),
	    FE(DMUS_OBJ_STREAM)
	};
    DMUSIC_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

static inline void DMUSIC_dump_DMUS_CONTAINER_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_CONTAINER_NOLOADS)
	};
    DMUSIC_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

static inline void DMUSIC_dump_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) {
	if (pDesc) {
		DPRINTF("DMUS_OBJECTDESC (%p)\n", pDesc);
		DPRINTF("  - dwSize = %ld\n", pDesc->dwSize);
		DPRINTF("  - dwValidData = ");
		DMUSIC_dump_DMUS_OBJ_FLAGS (pDesc->dwValidData);
		if (pDesc->dwValidData & DMUS_OBJ_CLASS) DPRINTF("  - guidClass = %s\n", debugstr_guid(&pDesc->guidClass));
		if (pDesc->dwValidData & DMUS_OBJ_OBJECT) DPRINTF("  - guidObject = %s\n", debugstr_guid(&pDesc->guidObject));
		if (pDesc->dwValidData & DMUS_OBJ_DATE) DPRINTF("  - ftDate = FIXME\n");
		if (pDesc->dwValidData & DMUS_OBJ_VERSION) DPRINTF("  - vVersion = %s\n", debugstr_dmversion(&pDesc->vVersion));
		if (pDesc->dwValidData & DMUS_OBJ_NAME) DPRINTF("  - wszName = %s\n", debugstr_w(pDesc->wszName));
		if (pDesc->dwValidData & DMUS_OBJ_CATEGORY) DPRINTF("  - wszCategory = %s\n", debugstr_w(pDesc->wszCategory));
		if (pDesc->dwValidData & DMUS_OBJ_FILENAME) DPRINTF("  - wszFileName = %s\n", debugstr_w(pDesc->wszFileName));
		if (pDesc->dwValidData & DMUS_OBJ_MEMORY) DPRINTF("  - llMemLength = %lli\n  - pbMemData = %p\n", pDesc->llMemLength, pDesc->pbMemData);
		if (pDesc->dwValidData & DMUS_OBJ_STREAM) DPRINTF("  - pStream = %p\n", pDesc->pStream);		
	} else {
		DPRINTF("(NULL)\n");
	}
}

/* check whether chunkID is valid dmobject form chunk */
static inline BOOL IS_VALID_DMFORM(chunkID) {
	if ((chunkID == DMUS_FOURCC_AUDIOPATH_FORM) || (chunkID == DMUS_FOURCC_BAND_FORM) || (chunkID == DMUS_FOURCC_CHORDMAP_FORM)
		|| (chunkID == DMUS_FOURCC_CONTAINER_FORM) || (chunkID == FOURCC_DLS) || (chunkID == DMUS_FOURCC_SCRIPT_FORM)
		|| (chunkID == DMUS_FOURCC_SEGMENT_FORM) || (chunkID == DMUS_FOURCC_STYLE_FORM) || (chunkID == DMUS_FOURCC_TOOLGRAPH_FORM)
	 	|| (chunkID == DMUS_FOURCC_TRACK_FORM) || (chunkID == mmioFOURCC('W','A','V','E')))  return TRUE;
	else return FALSE;
}

#endif	/* __WINE_DMLOADER_PRIVATE_H */
