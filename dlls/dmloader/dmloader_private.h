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

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoader8Impl IDirectMusicLoader8Impl;
typedef struct IDirectMusicContainerImpl IDirectMusicContainerImpl;
typedef struct IDirectMusicGetLoaderImpl IDirectMusicGetLoaderImpl;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8_Vtbl;
extern ICOM_VTABLE(IDirectMusicContainer) DirectMusicContainer_Vtbl;
extern ICOM_VTABLE(IDirectMusicGetLoader) DirectMusicGetLoader_Vtbl;

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
/* can support IID_IDirectMusicGetLoader
 * return always an IDirectMusicGetLoaderImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicGetLoader (LPCGUID lpcGUID, LPDIRECTMUSICGETLOADER *ppDMGetLoad, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicLoader8Impl implementation structure
 */
struct IDirectMusicLoader8Impl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicLoader8);
  DWORD          ref;

  /* IDirectMusicLoaderImpl fields */
  WCHAR          wzSearchPath[MAX_PATH];
  /* IDirectMusicLoader8Impl fields */ 
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
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface (LPDIRECTMUSICCONTAINER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicContainerImpl_AddRef (LPDIRECTMUSICCONTAINER iface);
extern ULONG WINAPI   IDirectMusicContainerImpl_Release (LPDIRECTMUSICCONTAINER iface);
/* IDirectMusicContainer: */
extern HRESULT WINAPI IDirectMusicContainerImpl_EnumObject (LPDIRECTMUSICCONTAINER iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc, WCHAR* pwszAlias);

/*****************************************************************************
 * IDirectMusicGetLoaderImpl implementation structure
 */
struct IDirectMusicGetLoaderImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicGetLoader);
  DWORD          ref;

  /* IDirectMusicGetLoaderImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicGetLoaderImpl_QueryInterface (LPDIRECTMUSICGETLOADER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicGetLoaderImpl_AddRef (LPDIRECTMUSICGETLOADER iface);
extern ULONG WINAPI   IDirectMusicGetLoaderImpl_Release (LPDIRECTMUSICGETLOADER iface);
/* IDirectMusicGetLoader: */
extern HRESULT WINAPI IDirectMusicGetLoaderImpl_GetLoader (LPDIRECTMUSICGETLOADER iface, IDirectMusicLoader** ppLoader);

#endif	/* __WINE_DMLOADER_PRIVATE_H */
