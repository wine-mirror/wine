/*
 * Defines IOleObject COM and other oleidl.h interfaces
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

#ifndef __WINE_WINE_OBJ_OLEOBJ_H
#define __WINE_WINE_OBJ_OLEOBJ_H

struct tagMSG;
struct tagLOGPALETTE;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */
typedef struct tagOBJECTDESCRIPTOR
{
	ULONG cbSize;
	CLSID clsid;
	DWORD dwDrawAspect;
	SIZEL sizel;
	POINTL pointl;
	DWORD dwStatus;
	DWORD dwFullUserTypeName;
	DWORD dwSrcOfCopy;
} OBJECTDESCRIPTOR, *LPOBJECTDESCRIPTOR;

typedef enum tagOLEMISC
{
	OLEMISC_RECOMPOSEONRESIZE = 0x1,
	OLEMISC_ONLYICONIC = 0x2,
	OLEMISC_INSERTNOTREPLACE = 0x4,
	OLEMISC_STATIC = 0x8,
	OLEMISC_CANTLINKINSIDE = 0x10,
	OLEMISC_CANLINKBYOLE1 = 0x20,
	OLEMISC_ISLINKOBJECT = 0x40,
	OLEMISC_INSIDEOUT = 0x80,
	OLEMISC_ACTIVATEWHENVISIBLE = 0x100,
	OLEMISC_RENDERINGISDEVICEINDEPENDENT = 0x200,
	OLEMISC_INVISIBLEATRUNTIME = 0x400,
	OLEMISC_ALWAYSRUN = 0x800,
	OLEMISC_ACTSLIKEBUTTON = 0x1000,
	OLEMISC_ACTSLIKELABEL = 0x2000,
	OLEMISC_NOUIACTIVATE = 0x4000,
	OLEMISC_ALIGNABLE = 0x8000,
	OLEMISC_SIMPLEFRAME = 0x10000,
	OLEMISC_SETCLIENTSITEFIRST = 0x20000,
	OLEMISC_IMEMODE = 0x40000,
	OLEMISC_IGNOREACTIVATEWHENVISIBLE = 0x80000,
	OLEMISC_WANTSTOMENUMERGE = 0x100000,
	OLEMISC_SUPPORTSMULTILEVELUNDO = 0x200000
} OLEMISC;

typedef enum tagOLEVERBATTRIB
{
	OLEVERBATTRIB_NEVERDIRTIES = 1,
	OLEVERBATTRIB_ONCONTAINERMENU = 2
} OLEVERBATTRIB;

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IOleObject,  0x00000112L, 0, 0);
typedef struct IOleObject IOleObject, *LPOLEOBJECT;

DEFINE_OLEGUID(IID_IOleAdviseHolder,  0x00000111L, 0, 0);
typedef struct IOleAdviseHolder IOleAdviseHolder, *LPOLEADVISEHOLDER;

DEFINE_OLEGUID(IID_IEnumOLEVERB,  0x00000104L, 0, 0);
typedef struct IEnumOLEVERB IEnumOLEVERB, *LPENUMOLEVERB;

/*****************************************************************************
 * IOleObject interface
 */
#define INTERFACE IOleObject
#define IOleObject_METHODS \
	IUnknown_METHODS \
	STDMETHOD(SetClientSite)(THIS_ IOleClientSite *pClientSite) PURE; \
	STDMETHOD(GetClientSite)(THIS_ IOleClientSite **ppClientSite) PURE; \
	STDMETHOD(SetHostNames)(THIS_ LPCOLESTR szContainerApp, LPCOLESTR szContainerObj) PURE; \
	STDMETHOD(Close)(THIS_ DWORD dwSaveOption) PURE; \
	STDMETHOD(SetMoniker)(THIS_ DWORD dwWhichMoniker, IMoniker *pmk) PURE; \
	STDMETHOD(GetMoniker)(THIS_ DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk) PURE; \
	STDMETHOD(InitFromData)(THIS_ IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved) PURE; \
	STDMETHOD(GetClipboardData)(THIS_ DWORD dwReserved, IDataObject **ppDataObject) PURE; \
	STDMETHOD(DoVerb)(THIS_ LONG iVerb, struct tagMSG *lpmsg, IOleClientSite *pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect) PURE; \
	STDMETHOD(EnumVerbs)(THIS_ IEnumOLEVERB **ppEnumOleVerb) PURE; \
	STDMETHOD(Update)(THIS) PURE; \
	STDMETHOD(IsUpToDate)(THIS) PURE; \
	STDMETHOD(GetUserClassID)(THIS_ CLSID *pClsid) PURE; \
	STDMETHOD(GetUserType)(THIS_ DWORD dwFormOfType, LPOLESTR *pszUserType) PURE; \
	STDMETHOD(SetExtent)(THIS_ DWORD dwDrawAspect, SIZEL *psizel) PURE; \
	STDMETHOD(GetExtent)(THIS_ DWORD dwDrawAspect, SIZEL *psizel) PURE; \
	STDMETHOD(Advise)(THIS_ IAdviseSink *pAdvSink, DWORD *pdwConnection) PURE; \
	STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) PURE; \
	STDMETHOD(EnumAdvise)(THIS_ IEnumSTATDATA **ppenumAdvise) PURE; \
	STDMETHOD(GetMiscStatus)(THIS_ DWORD dwAspect, DWORD *pdwStatus) PURE; \
	STDMETHOD(SetColorScheme)(THIS_ struct tagLOGPALETTE *pLogpal) PURE;
ICOM_DEFINE(IOleObject,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleObject_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleObject_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleObject_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleObject methods ***/
#define IOleObject_SetClientSite(p,a)       (p)->lpVtbl->SetClientSite(p,a)
#define IOleObject_GetClientSite(p,a,b)     (p)->lpVtbl->GetClientSite(p,a)
#define IOleObject_SetHostNames(p,a,b)      (p)->lpVtbl->SetHostNames(p,a,b)
#define IOleObject_Close(p,a,b)             (p)->lpVtbl->Close(p,a)
#define IOleObject_SetMoniker(p,a,b)        (p)->lpVtbl->SetMoniker(p,a,b)
#define IOleObject_GetMoniker(p,a,b)        (p)->lpVtbl->GetMoniker(p,a,b,c)
#define IOleObject_InitFromData(p,a,b)      (p)->lpVtbl->InitFromData(p,a,b,c)
#define IOleObject_GetClipboardData(p,a,b)  (p)->lpVtbl->GetClipboardData(p,a,b)
#define IOleObject_DoVerb(p,a,b)            (p)->lpVtbl->DoVerb(p,a,b,c,d,e,f)
#define IOleObject_EnumVerbs(p,a,b)         (p)->lpVtbl->EnumVerbs(p,a)
#define IOleObject_Update(p,a,b)            (p)->lpVtbl->Update(p)
#define IOleObject_IsUpToDate(p,a,b)        (p)->lpVtbl->IsUpToDate(p)
#define IOleObject_GetUserClassID(p,a,b)    (p)->lpVtbl->GetUserClassID(p,a)
#define IOleObject_GetUserType(p,a,b)       (p)->lpVtbl->GetUserType(p,a,b)
#define IOleObject_SetExtent(p,a,b)         (p)->lpVtbl->SetExtent(p,a,b)
#define IOleObject_GetExtent(p,a,b)         (p)->lpVtbl->GetExtent(p,a,b)
#define IOleObject_Advise(p,a,b)            (p)->lpVtbl->Advise(p,a,b)
#define IOleObject_Unadvise(p,a,b)          (p)->lpVtbl->Unadvise(p,a)
#define IOleObject_EnumAdvise(p,a,b)        (p)->lpVtbl->EnumAdvise(p,a)
#define IOleObject_GetMiscStatus(p,a,b)     (p)->lpVtbl->GetMiscStatus(p,a,b)
#define IOleObject_SetColorScheme(p,a,b)    (p)->lpVtbl->SetColorScheme(p,a)
#endif


/*****************************************************************************
 * IOleAdviseHolder interface
 */
#define INTERFACE IOleAdviseHolder
#define IOleAdviseHolder_METHODS \
	IUnknown_METHODS \
	STDMETHOD(Advise)(THIS_ IAdviseSink *pAdvise, DWORD *pdwConnection) PURE; \
	STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) PURE; \
	STDMETHOD(EnumAdvise)(THIS_ IEnumSTATDATA **ppenumAdvise) PURE; \
	STDMETHOD(SendOnRename)(THIS_ IMoniker *pmk) PURE; \
	STDMETHOD(SendOnSave)(THIS) PURE; \
	STDMETHOD(SendOnClose)(THIS) PURE;
ICOM_DEFINE(IOleAdviseHolder,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleAdviseHolder_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleAdviseHolder_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleAdviseHolder_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleAdviseHolder methods ***/
#define IOleAdviseHolder_Advise(p,a,b)              (p)->lpVtbl->Advise(p,a,b)
#define IOleAdviseHolder_Unadvise(p,a)              (p)->lpVtbl->Unadvise(p,a)
#define IOleAdviseHolder_EnumAdvise(p,a)            (p)->lpVtbl->EnumAdvise(p,a)
#define IOleAdviseHolder_SendOnRename(p,a)          (p)->lpVtbl->SendOnRename(p,a)
#define IOleAdviseHolder_SendOnSave(p)              (p)->lpVtbl->SendOnSave(p)
#define IOleAdviseHolder_SendOnClose(p)             (p)->lpVtbl->SendOnClose(p)
#endif


/*****************************************************************************
 *  IEnumOLEVERB interface
 */
#define INTERFACE IEnumOLEVERB
#define IEnumOLEVERB_METHODS \
	IUnknown_METHODS \
	STDMETHOD(Next)(THIS_ ULONG celt, LPOLEVERB rgelt, ULONG *pceltFetched) PURE; \
	STDMETHOD(Skip)(THIS_ ULONG celt) PURE; \
	STDMETHOD(Reset)(THIS) PURE; \
	STDMETHOD(Clone)(THIS_ IEnumOLEVERB **ppenum) PURE;
ICOM_DEFINE(IEnumOLEVERB,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumOLEVERB_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumOLEVERB_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IEnumOLEVERB_Release(p)             (p)->lpVtbl->Release(p)
/*** IEnumOLEVERB methods ***/
#define IEnumOLEVERB_Next(p,a,b,c)          (p)->lpVtbl->Next(p,a,b,c)
#define IEnumOLEVERB_Skip(p,a)              (p)->lpVtbl->Skip(p,a)
#define IEnumOLEVERB_Reset(p,a)             (p)->lpVtbl->Reset(p)
#define IEnumOLEVERB_Clone(p,a)             (p)->lpVtbl->Clone(p,a)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_OLEOBJ_H */
