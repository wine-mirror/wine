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

#ifndef __WINE_WINE_OBJ_CONNECTION_H
#define __WINE_WINE_OBJ_CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */

typedef struct tagCONNECTDATA
{
				  IUnknown *pUnk;
					  DWORD dwCookie;
} CONNECTDATA, *LPCONNECTDATA;

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IConnectionPoint, 0xb196b286, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IConnectionPoint IConnectionPoint, *LPCONNECTIONPOINT;

DEFINE_GUID(IID_IConnectionPointContainer, 0xb196b284, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IConnectionPointContainer IConnectionPointContainer, *LPCONNECTIONPOINTCONTAINER;

DEFINE_GUID(IID_IEnumConnections, 0xb196b287, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IEnumConnections IEnumConnections, *LPENUMCONNECTIONS;

DEFINE_GUID(IID_IEnumConnectionPoints, 0xb196b285, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IEnumConnectionPoints IEnumConnectionPoints, *LPENUMCONNECTIONPOINTS;

/*****************************************************************************
 * IConnectionPoint interface
 */
#define INTERFACE IConnectionPoint
#define IConnectionPoint_METHODS \
	STDMETHOD(GetConnectionInterface)(THIS_ IID *pIID) PURE; \
	STDMETHOD(GetConnectionPointContainer)(THIS_ IConnectionPointContainer **ppCPC) PURE; \
	STDMETHOD(Advise)(THIS_ IUnknown *pUnkSink, DWORD *pdwCookie) PURE; \
	STDMETHOD(Unadvise)(THIS_ DWORD dwCookie) PURE; \
	STDMETHOD(EnumConnections)(THIS_ IEnumConnections **ppEnum) PURE;
#define IConnectionPoint_IMETHODS \
	IUnknown_IMETHODS \
	IConnectionPoint_METHODS
ICOM_DEFINE(IConnectionPoint,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IConnectionPoint_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IConnectionPoint_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IConnectionPoint_Release(p)                 (p)->lpVtbl->Release(p)
/*** IConnectionPointContainer methods ***/
#define IConnectionPoint_GetConnectionInterface(p,a)      (p)->lpVtbl->GetConnectionInterface(p,a)
#define IConnectionPoint_GetConnectionPointContainer(p,a) (p)->lpVtbl->GetConnectionPointContainer(p,a)
#define IConnectionPoint_Advise(p,a,b)                    (p)->lpVtbl->Advise(p,a,b)
#define IConnectionPoint_Unadvise(p,a)                    (p)->lpVtbl->Unadvise(p,a)
#define IConnectionPoint_EnumConnections(p,a)             (p)->lpVtbl->EnumConnections(p,a)
#endif


/*****************************************************************************
 * IConnectionPointContainer interface
 */
#define INTERFACE IConnectionPointContainer
#define IConnectionPointContainer_METHODS \
	STDMETHOD(EnumConnectionPoints)(THIS_ IEnumConnectionPoints **ppEnum) PURE; \
	STDMETHOD(FindConnectionPoint)(THIS_ REFIID riid, IConnectionPoint **ppCP) PURE;
#define IConnectionPointContainer_IMETHODS \
	IUnknown_IMETHODS \
	IConnectionPointContainer_METHODS
ICOM_DEFINE(IConnectionPointContainer,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IConnectionPointContainer_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IConnectionPointContainer_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IConnectionPointContainer_Release(p)                 (p)->lpVtbl->Release(p)
/*** IConnectionPointContainer methods ***/
#define IConnectionPointContainer_EnumConnectionPoints(p,a)  (p)->lpVtbl->EnumConnectionPoints(p,a)
#define IConnectionPointContainer_FindConnectionPoint(p,a,b) (p)->lpVtbl->FindConnectionPoint(p,a,b)
#endif


/*****************************************************************************
 * IEnumConnections interface
 */
#define INTERFACE IEnumConnections
#define IEnumConnections_METHODS \
	STDMETHOD(Next)(THIS_ ULONG cConnections, LPCONNECTDATA rgcd, ULONG *pcFectched) PURE; \
	STDMETHOD(Skip)(THIS_ ULONG cConnections) PURE; \
	STDMETHOD(Reset)(THIS) PURE; \
	STDMETHOD(Clone)(THIS_ IEnumConnections **ppEnum) PURE;
#define IEnumConnections_IMETHODS \
	IUnknown_IMETHODS \
	IEnumConnections_METHODS
ICOM_DEFINE(IEnumConnections,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumConnections_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumConnections_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IEnumConnections_Release(p)                 (p)->lpVtbl->Release(p)
/*** IConnectionPointContainer methods ***/
#define IEnumConnections_Next(p,a,b,c)              (p)->lpVtbl->Next(p,a,b,c)
#define IEnumConnections_Skip(p,a)                  (p)->lpVtbl->Skip(p,a)
#define IEnumConnections_Reset(p)                   (p)->lpVtbl->Reset(p)
#define IEnumConnections_Clone(p,a)                 (p)->lpVtbl->Clone(p,a)
#endif

/*****************************************************************************
 * IEnumConnectionPoints interface
 */
#define INTERFACE IEnumConnectionPoints
#define IEnumConnectionPoints_METHODS \
	STDMETHOD(Next)(THIS_ ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFectched) PURE; \
	STDMETHOD(Skip)(THIS_ ULONG cConnections) PURE; \
	STDMETHOD(Reset)(THIS) PURE; \
	STDMETHOD(Clone)(THIS_ IEnumConnections **ppEnum) PURE;
#define IEnumConnectionPoints_IMETHODS \
	IUnknown_IMETHODS \
	IEnumConnectionPoints_METHODS
ICOM_DEFINE(IEnumConnectionPoints,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumConnectionPoints_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumConnectionPoints_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IEnumConnectionPoints_Release(p)                 (p)->lpVtbl->Release(p)
/*** IConnectionPointContainer methods ***/
#define IEnumConnectionPoints_Next(p,a,b,c)              (p)->lpVtbl->Next(p,a,b,c)
#define IEnumConnectionPoints_Skip(p,a)                  (p)->lpVtbl->Skip(p,a)
#define IEnumConnectionPoints_Reset(p)                   (p)->lpVtbl->Reset(p)
#define IEnumConnectionPoints_Clone(p,a)                 (p)->lpVtbl->Clone(p,a)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_CONTROL_H */
