/* DirectMusicLoader Private Include
 *
 * Copyright (C) 2003 Rok Mandeljc
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
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"


typedef struct _DMUS_PRIVATE_CACHE_ENTRY
{
	GUID guidObject;
	WCHAR pwzFileName[MAX_PATH];
    IDirectMusicObject* pObject;
} DMUS_PRIVATE_CACHE_ENTRY, *LPDMUS_PRIVATE_CACHE_ENTRY;

typedef struct _DMUS_PRIVATE_OBJECT_REFERENCE DMUS_PRIVATE_OBJECT_REFERENCE;

struct _DMUS_PRIVATE_OBJECT_REFERENCE {
    DMUS_PRIVATE_OBJECT_REFERENCE* pNext;
	WCHAR pwsFileName[MAX_PATH];
    GUID guidObject;
    IDirectMusicObject* pObject;
};

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoader8Impl IDirectMusicLoader8Impl;
typedef struct IDirectMusicContainerImpl IDirectMusicContainerImpl;

typedef struct IDirectMusicContainerObject IDirectMusicContainerObject;
typedef struct IDirectMusicContainerObjectStream IDirectMusicContainerObjectStream;

typedef struct ILoaderStream ILoaderStream;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8_Vtbl;
extern ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Vtbl;

extern ICOM_VTABLE(IDirectMusicObject) DirectMusicContainerObject_Vtbl;
extern ICOM_VTABLE(IPersistStream) DirectMusicContainerObjectStream_Vtbl;

extern ICOM_VTABLE(IUnknown) LoaderStream_Unknown_Vtbl;
extern ICOM_VTABLE(IStream) LoaderStream_Stream_Vtbl;
extern ICOM_VTABLE(IDirectMusicGetLoader) LoaderStream_GetLoader_Vtbl;

/*****************************************************************************
 * ClassFactory
 */
/* can support IID_IDirectMusicLoader and IID_IDirectMusicLoader8
 * return always an IDirectMusicLoader8Impl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicLoader (LPCGUID lpcGUID, LPDIRECTMUSICLOADER8 *ppDMLoad, LPUNKNOWN pUnkOuter);
/* can support IID_IDirectMusicContainer
 * return always an IDirectMusicContainerImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicContainer (LPCGUID lpcGUID, LPDIRECTMUSICCONTAINER *ppDMCon, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateDirectMusicContainerObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter);

extern HRESULT WINAPI DMUSIC_CreateLoaderStream (LPSTREAM *ppStream);

/*****************************************************************************
 * IDirectMusicLoader8Impl implementation structure
 */
struct IDirectMusicLoader8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicLoader8);
  DWORD          ref;

  /* IDirectMusicLoaderImpl fields */
  WCHAR wzSearchPath[MAX_PATH];
	
  /* simple cache */
  LPDMUS_PRIVATE_CACHE_ENTRY pCache; /* cache entries */
  DWORD dwCacheSize; /* nr. of entries */
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
struct IDirectMusicContainerImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicContainer);
  DWORD          ref;

  /* IDirectMusicContainerImpl fields */
  IDirectMusicContainerObject* pObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicContainerImpl_AddRef (LPDIRECTMUSICCONTAINER iface);
extern ULONG WINAPI   IDirectMusicContainerImpl_Release (LPDIRECTMUSICCONTAINER iface);
/* IDirectMusicContainer: */
extern HRESULT WINAPI IDirectMusicContainerImpl_EnumObject (LPDIRECTMUSICCONTAINER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc, WCHAR* pwszAlias);


/*****************************************************************************
 * IDirectMusicContainerObject implementation structure
 */
struct IDirectMusicContainerObject
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicObject);
  DWORD          ref;

  /* IDirectMusicObjectImpl fields */
  LPDMUS_OBJECTDESC pDesc;
  IDirectMusicContainerObjectStream* pStream;
  IDirectMusicContainerImpl* pContainer;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicContainerObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicContainerObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicContainerObject_Release (LPDIRECTMUSICOBJECT iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicContainerObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicContainerObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicContainerObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

/*****************************************************************************
 * IDirectMusicContainerObjectStream implementation structure
 */
struct IDirectMusicContainerObjectStream
{
  /* IUnknown fields */
  ICOM_VFIELD (IPersistStream);
  DWORD          ref;

  /* IPersistStreamImpl fields */
  IDirectMusicContainerObject* pParentObject;
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicContainerObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI IDirectMusicContainerObjectStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI IDirectMusicContainerObjectStream_Release (LPPERSISTSTREAM iface);
/* IPersist: */
extern HRESULT WINAPI IDirectMusicContainerObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicContainerObjectStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicContainerObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicContainerObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicContainerObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * ILoaderStream implementation structure
 */
struct ILoaderStream
{
  /* IUnknown fields */
  ICOM_VTABLE(IStream) *StreamVtbl;
  ICOM_VTABLE(IDirectMusicGetLoader) *GetLoaderVtbl;
  DWORD          ref;

  /* ILoaderStream fields */
  IDirectMusicLoader8Impl* pLoader;
  HANDLE hFile;
  WCHAR wzFileName[MAX_PATH]; /* for clone */
};

/* Custom: */
extern HRESULT WINAPI ILoaderStream_Attach (ILoaderStream* iface, LPCWSTR wzFile, IDirectMusicLoader *pLoader);
extern void WINAPI ILoaderStream_Detach (ILoaderStream* iface);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, void** ppobj);
extern ULONG WINAPI ILoaderStream_IDirectMusicGetLoader_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG WINAPI ILoaderStream_IDirectMusicGetLoader_Release (LPDIRECTMUSICGETLOADER iface);
extern HRESULT WINAPI ILoaderStream_IDirectMusicGetLoader_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader **ppLoader);
/* IStream: */
extern HRESULT WINAPI ILoaderStream_IStream_QueryInterface (LPSTREAM iface, REFIID riid, void** ppobj);
extern ULONG WINAPI ILoaderStream_IStream_AddRef (LPSTREAM iface);
extern ULONG WINAPI ILoaderStream_IStream_Release (LPSTREAM iface);extern HRESULT WINAPI ILoaderStream_IStream_Read (IStream* iface, void* pv, ULONG cb, ULONG* pcbRead);
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

#endif	/* __WINE_DMLOADER_PRIVATE_H */
