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

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

/* dmloader.dll global (for DllCanUnloadNow) */
extern DWORD dwDirectMusicLoader; /* number of DirectMusicLoader(CF) instances */
extern DWORD dwDirectMusicContainer; /* number of DirectMusicContainer(CF) instances */

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
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IClassFactory)         DirectMusicLoaderCF_Vtbl;
extern ICOM_VTABLE(IClassFactory)         DirectMusicContainerCF_Vtbl;

extern ICOM_VTABLE(IDirectMusicLoader8)   DirectMusicLoader_Loader_Vtbl;

extern ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Container_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject)    DirectMusicContainer_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream)        DirectMusicContainer_PersistStream_Vtbl;

extern ICOM_VTABLE(IStream)               DirectMusicLoaderFileStream_Stream_Vtbl;
extern ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicLoaderFileStream_GetLoader_Vtbl;

extern ICOM_VTABLE(IStream)               DirectMusicLoaderResourceStream_Stream_Vtbl;
extern ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicLoaderResourceStream_GetLoader_Vtbl;

extern ICOM_VTABLE(IStream)               DirectMusicLoaderGenericStream_Stream_Vtbl;
extern ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicLoaderGenericStream_GetLoader_Vtbl;

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
extern HRESULT WINAPI DMUSIC_DestroyDirectMusicLoaderFileStream (LPSTREAM iface);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderResourceStream (LPVOID *ppobj);
extern HRESULT WINAPI DMUSIC_DestroyDirectMusicLoaderResourceStream (LPSTREAM iface);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderGenericStream (LPVOID *ppobj);
extern HRESULT WINAPI DMUSIC_DestroyDirectMusicLoaderGenericStream (LPSTREAM iface);

/*****************************************************************************
 * IDirectMusicLoaderCF implementation structure
 */
struct IDirectMusicLoaderCF {
	/* IUnknown fields */
	ICOM_VFIELD(IClassFactory);
	DWORD dwRef;
};

/* IUnknown / IClassFactory: */
extern HRESULT WINAPI IDirectMusicLoaderCF_QueryInterface (LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj);
extern ULONG   WINAPI IDirectMusicLoaderCF_AddRef (LPCLASSFACTORY iface);
extern ULONG   WINAPI IDirectMusicLoaderCF_Release (LPCLASSFACTORY iface);
extern HRESULT WINAPI IDirectMusicLoaderCF_CreateInstance (LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT WINAPI IDirectMusicLoaderCF_LockServer (LPCLASSFACTORY iface,BOOL dolock);


/*****************************************************************************
 * IDirectMusicContainerCF implementation structure
 */
struct IDirectMusicContainerCF {
	/* IUnknown fields */
	ICOM_VFIELD(IClassFactory);
	DWORD dwRef;
};

/* IUnknown / IClassFactory: */
extern HRESULT WINAPI IDirectMusicContainerCF_QueryInterface (LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj);
extern ULONG   WINAPI IDirectMusicContainerCF_AddRef (LPCLASSFACTORY iface);
extern ULONG   WINAPI IDirectMusicContainerCF_Release (LPCLASSFACTORY iface);
extern HRESULT WINAPI IDirectMusicContainerCF_CreateInstance (LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT WINAPI IDirectMusicContainerCF_LockServer (LPCLASSFACTORY iface,BOOL dolock);


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
	ICOM_VTABLE(IDirectMusicLoader8) *LoaderVtbl;
	/* reference counter */
	DWORD dwRef;	
	/* simple cache (linked list) */
	struct list *pObjects;
	/* settings for certain object classes */
	struct list *pClassSettings;
	/* critical section */
	CRITICAL_SECTION CritSect;
};

/* IUnknown / IDirectMusicLoader(8): */
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj);
extern ULONG   WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_AddRef (LPDIRECTMUSICLOADER8 iface);
extern ULONG   WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_Release (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID*ppv);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc);
extern void    WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_CollectGarbage (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject);
extern HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClassID, REFIID iidInterfaceID, WCHAR* pwzFilePath, void** ppObject);

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
	ICOM_VTABLE(IDirectMusicContainer) *ContainerVtbl;
	ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
	ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
	/* reference counter */
	DWORD dwRef;
	/* stream */
	LPSTREAM pStream;
	/* header */
	DMUS_IO_CONTAINER_HEADER Header;
	/* data */
	struct list *pContainedObjects;	
	/* descriptor */
	DMUS_OBJECTDESC Desc;
};

/* IUnknown / IDirectMusicContainer: */
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
 * IDirectMusicLoaderFileStream implementation structure
 */
struct IDirectMusicLoaderFileStream {
	/* VTABLEs */
	ICOM_VTABLE(IStream) *StreamVtbl;
	ICOM_VTABLE(IDirectMusicGetLoader) *GetLoaderVtbl;
	/* reference counter */
	DWORD dwRef;
	/* file */
	WCHAR wzFileName[MAX_PATH]; /* for clone */
	HANDLE hFile;
	/* loader */
	LPDIRECTMUSICLOADER8 pLoader;
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderFileStream_Attach (LPSTREAM iface, LPCWSTR wzFile, LPDIRECTMUSICLOADER pLoader);
extern void    WINAPI IDirectMusicLoaderFileStream_Detach (LPSTREAM iface);
/* IUnknown/IStream: */
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI IDirectMusicLoaderFileStream_IStream_AddRef (LPSTREAM iface);
extern ULONG   WINAPI IDirectMusicLoaderFileStream_IStream_Release (LPSTREAM iface);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Read (IStream* iface, void* pv, ULONG cb, ULONG* pcbRead);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Revert (LPSTREAM iface);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IStream_Clone (LPSTREAM iface, IStream** ppstm);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG   WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface);
extern HRESULT WINAPI IDirectMusicLoaderFileStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader);


/*****************************************************************************
 * IDirectMusicLoaderResourceStream implementation structure
 */
struct IDirectMusicLoaderResourceStream {
	/* IUnknown fields */
	ICOM_VTABLE(IStream) *StreamVtbl;
	ICOM_VTABLE(IDirectMusicGetLoader) *GetLoaderVtbl;
	/* reference counter */
	DWORD dwRef;
	/* data */
	LPBYTE pbMemData;
	LONGLONG llMemLength;
	/* current position */
	LONGLONG llPos;	
	/* loader */
	LPDIRECTMUSICLOADER8 pLoader;
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_Attach (LPSTREAM iface, LPBYTE pbMemData, LONGLONG llMemLength, LONGLONG llPos, LPDIRECTMUSICLOADER pLoader);
extern void    WINAPI IDirectMusicLoaderResourceStream_Detach (LPSTREAM iface);
/* IUnknown/IStream: */
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI IDirectMusicLoaderResourceStream_IStream_AddRef (LPSTREAM iface);
extern ULONG   WINAPI IDirectMusicLoaderResourceStream_IStream_Release (LPSTREAM iface);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Read (IStream* iface, void* pv, ULONG cb, ULONG* pcbRead);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Revert (LPSTREAM iface);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IStream_Clone (LPSTREAM iface, IStream** ppstm);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG   WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface);
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader);


/*****************************************************************************
 * IDirectMusicLoaderGenericStream implementation structure
 */
struct IDirectMusicLoaderGenericStream {
	/* IUnknown fields */
	ICOM_VTABLE(IStream) *StreamVtbl;
	ICOM_VTABLE(IDirectMusicGetLoader) *GetLoaderVtbl;
	/* reference counter */
	DWORD dwRef;
	/* stream */
	LPSTREAM pStream;
	/* loader */
	LPDIRECTMUSICLOADER8 pLoader;
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_Attach (LPSTREAM iface, LPSTREAM pStream, LPDIRECTMUSICLOADER pLoader);
extern void    WINAPI IDirectMusicLoaderGenericStream_Detach (LPSTREAM iface);
/* IUnknown/IStream: */
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI IDirectMusicLoaderGenericStream_IStream_AddRef (LPSTREAM iface);
extern ULONG   WINAPI IDirectMusicLoaderGenericStream_IStream_Release (LPSTREAM iface);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Read (IStream* iface, void* pv, ULONG cb, ULONG* pcbRead);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Write (LPSTREAM iface, const void* pv, ULONG cb, ULONG* pcbWritten);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Seek (LPSTREAM iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_SetSize (LPSTREAM iface, ULARGE_INTEGER libNewSize);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_CopyTo (LPSTREAM iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Commit (LPSTREAM iface, DWORD grfCommitFlags);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Revert (LPSTREAM iface);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_LockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_UnlockRegion (LPSTREAM iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Stat (LPSTREAM iface, STATSTG* pstatstg, DWORD grfStatFlag);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IStream_Clone (LPSTREAM iface, IStream** ppstm);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj);
extern ULONG   WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG   WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface);
extern HRESULT WINAPI IDirectMusicLoaderGenericStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader);


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
extern HRESULT WINAPI DMUSIC_SetLoaderSettings (LPDIRECTMUSICLOADER8 iface, REFGUID pClassID, WCHAR* wszSearchPath, LPBOOL pbCache);
extern HRESULT WINAPI DMUSIC_InitLoaderSettings (LPDIRECTMUSICLOADER8 iface);
extern HRESULT WINAPI DMUSIC_CopyDescriptor (LPDMUS_OBJECTDESC pDst, LPDMUS_OBJECTDESC pSrc);
extern BOOL WINAPI DMUSIC_IsValidLoadableClass (REFCLSID pClassID);

#include "debug.h"

#endif	/* __WINE_DMLOADER_PRIVATE_H */
