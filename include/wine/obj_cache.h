/*
 * Defines the COM interfaces and APIs related to structured data storage.
 *
 * Depends on 'obj_base.h'.
 *
 * Copyright (C) 1999 Paul Quinn
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_OBJ_CACHE_H
#define __WINE_WINE_OBJ_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */


/*****************************************************************************
 * Predeclare the interfaces
 */

DEFINE_OLEGUID(IID_IOleCache,  0x0000011eL, 0, 0);
typedef struct IOleCache IOleCache, *LPOLECACHE;

DEFINE_OLEGUID(IID_IOleCache2,  0x00000128L, 0, 0);
typedef struct IOleCache2 IOleCache2, *LPOLECACHE2;

DEFINE_OLEGUID(IID_IOleCacheControl,  0x00000129L, 0, 0);
typedef struct IOleCacheControl IOleCacheControl, *LPOLECACHECONTROL;

/*****************************************************************************
 * IOleCache interface
 */
#define INTERFACE IOleCache
#define IOleCache_METHODS \
	STDMETHOD(Cache)(THIS_ FORMATETC *pformatetc, DWORD advf, DWORD * pdwConnection) PURE; \
	STDMETHOD(Uncache)(THIS_ DWORD dwConnection) PURE; \
	STDMETHOD(EnumCache)(THIS_ IEnumSTATDATA **ppenumSTATDATA) PURE; \
	STDMETHOD(InitCache)(THIS_ IDataObject *pDataObject) PURE; \
	STDMETHOD(SetData)(THIS_ FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) PURE;
#define IOleCache_IMETHODS \
	IUnknown_IMETHODS \
	IOleCache_METHODS
ICOM_DEFINE(IOleCache,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleCache_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleCache_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleCache_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleCache methods ***/
#define IOleCache_Cache(p,a,b,c)             (p)->lpVtbl->Cache(p,a,b,c)
#define IOleCache_Uncache(p,a)               (p)->lpVtbl->Uncache(p,a)
#define IOleCache_EnumCache(p,a)             (p)->lpVtbl->EnumCache(p,a)
#define IOleCache_InitCache(p,a)             (p)->lpVtbl->InitCache(p,a)
#define IOleCache_SetData(p,a,b,c)           (p)->lpVtbl->SetData(p,a,b,c)
#endif


/*****************************************************************************
 * IOleCache2 interface
 */
#define INTERFACE IOleCache2
#define IOleCache2_METHODS \
	STDMETHOD(UpdateCache)(THIS_ LPDATAOBJECT pDataObject, DWORD grfUpdf, LPVOID pReserved) PURE; \
	STDMETHOD(DiscardCache)(THIS_ DWORD dwDiscardOptions) PURE;
#define IOleCache2_IMETHODS \
	IOleCache_IMETHODS \
	IOleCache2_METHODS
ICOM_DEFINE(IOleCache2,IOleCache)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleCache2_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleCache2_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleCache2_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleCache methods ***/
#define IOleCache2_Cache(p,a,b,c)             (p)->lpVtbl->Cache(p,a,b,c)
#define IOleCache2_Uncache(p,a)               (p)->lpVtbl->Uncache(p,a)
#define IOleCache2_EnumCache(p,a)             (p)->lpVtbl->EnumCache(p,a)
#define IOleCache2_InitCache(p,a)             (p)->lpVtbl->InitCache(p,a)
#define IOleCache2_SetData(p,a,b,c)           (p)->lpVtbl->SetData(p,a,b,c)
/*** IOleCache2 methods ***/
#define IOleCache2_UpdateCache(p,a,b,c)       (p)->lpVtbl->UpdateCache(p,a,b,c)
#define IOleCache2_DiscardCache(p,a)          (p)->lpVtbl->DiscardCache(p,a)
#endif


/*****************************************************************************
 * IOleCacheControl interface
 */
#define INTERFACE IOleCacheControl
#define IOleCacheControl_METHODS \
	STDMETHOD(OnRun)(THIS_ LPDATAOBJECT pDataObject) PURE; \
	STDMETHOD(OnStop)(THIS) PURE;
#define IOleCacheControl_IMETHODS \
	IUnknown_IMETHODS \
	IOleCacheControl_METHODS
ICOM_DEFINE(IOleCacheControl,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleCacheControl_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleCacheControl_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleCacheControl_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleCacheControl methods ***/
#define IOleCacheControl_OnRun(p,a)                 (p)->lpVtbl->UpdateCache(p,a)
#define IOleCacheControl_OnStop(p)                  (p)->lpVtbl->OnStop(p)
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_CONTROL_H */
