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

#ifndef __WINE_WINE_OBJ_CONTROL_H
#define __WINE_WINE_OBJ_CONTROL_H

struct tagMSG;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */
typedef enum tagGUIDKIND
{
		GUIDKIND_DEFAULT_SOURCE_DISP_IID = 1
} GUIDKIND;

typedef enum tagREADYSTATE
{
	READYSTATE_UNINITIALIZED  = 0,
	READYSTATE_LOADING  = 1,
	READYSTATE_LOADED = 2,
	READYSTATE_INTERACTIVE  = 3,
	READYSTATE_COMPLETE = 4
} READYSTATE;

typedef struct tagExtentInfo
{
	ULONG cb;
	DWORD dwExtentMode;
	SIZEL sizelProposed;
} DVEXTENTINFO;

typedef struct tagVARIANT_BLOB
{
	DWORD clSize;
	DWORD rpcReserved;
	ULONGLONG ahData[1];
} wireVARIANT_BLOB;

typedef struct tagUserVARIANT
{
	wireVARIANT_BLOB pVarBlob;
} UserVARIANT;

typedef struct tagLICINFO
{
	LONG cbLicInfo;
	BOOL fRuntimeKeyAvail;
	BOOL fLicVerified;
} LICINFO, *LPLICINFO;

typedef struct tagCONTROLINFO
{
	ULONG cb;
	HACCEL hAccel;
	USHORT cAccel;
	DWORD dwFlags;
} CONTROLINFO, *LPCONTROLINFO;

typedef enum tagCTRLINFO
{
	CTRLINFO_EATS_RETURN = 1,
	CTRLINFO_EATS_ESCAPE = 2
} CTRLINFO;

typedef struct tagPOINTF
{
	FLOAT x;
	FLOAT y;
} POINTF, *LPPOINTF;

typedef enum tagXFORMCOORDS
{
	XFORMCOORDS_POSITION = 0x1,
	XFORMCOORDS_SIZE = 0x2,
	XFORMCOORDS_HIMETRICTOCONTAINER = 0x4,
	XFORMCOORDS_CONTAINERTOHIMETRIC = 0x8
} XFORMCOORDS;

typedef enum tagACTIVATEFLAGS
{
	ACTIVATE_WINDOWLESS = 1
} ACTIVATE_FLAGS;

typedef enum tagOLEDCFLAGS
{
	OLEDC_NODRAW = 0x1,
	OLEDC_PAINTBKGND = 0x2,
	OLEDC_OFFSCREEN = 0x4
} OLEDCFLAGS;

typedef enum tagDVASPECT2
{
	DVASPECT_OPAQUE = 16,
	DVASPECT_TRANSPARENT = 32
} DVASPECT2;

typedef enum tagHITRESULT
{
	HITRESULT_OUTSIDE = 0,
	HITRESULT_TRANSPARENT = 1,
	HITRESULT_CLOSE = 2,
	HITRESULT_HIT = 3
} HITRESULT;

typedef enum tagAspectInfoFlag
{
	DVASPECTINFOFLAG_CANOPTIMIZE = 1
} DVASPECTINFOFLAG;

typedef struct tagAspectInfo
{
	ULONG cb;
	DWORD dwFlags;
} DVASPECTINFO;

typedef enum tagVIEWSTATUS
{
	VIEWSTATUS_OPAQUE = 1,
	VIEWSTATUS_SOLIDBKGND = 2,
	VIEWSTATUS_DVASPECTOPAQUE = 4,
	VIEWSTATUS_DVASPECTTRANSPARENT = 8
} VIEWSTATUS;

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IOleControl, 0xb196b288, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IOleControl IOleControl, *LPOLECONTROL;

DEFINE_GUID(IID_IOleControlSite, 0xb196b289, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IOleControlSite IOleControlSite, *LPOLECONTROLSITE;

DEFINE_GUID(IID_IOleInPlaceSiteEx, 0x9c2cad80L, 0x3424, 0x11cf, 0xb6, 0x70, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8);
typedef struct IOleInPlaceSiteEx IOleInPlaceSiteEx, *LPOLEINPLACESITEEX;

DEFINE_OLEGUID(IID_IOleInPlaceSiteWindowless,  0x00000000L, 0, 0); /* FIXME - NEED GUID */
typedef struct IOleInPlaceSiteWindowless IOleInPlaceSiteWindowless, *LPOLEINPLACESITEWINDOWLESS;

DEFINE_OLEGUID(IID_IOleInPlaceObjectWindowless,  0x00000000L, 0, 0); /* FIXME - NEED GUID */
typedef struct IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless, *LPOLEINPLACEOBJECTWINDOWLESS;

DEFINE_GUID(IID_IClassFactory2, 0xb196b28f, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IClassFactory2 IClassFactory2, *LPCLASSFACTORY2;

DEFINE_GUID(IID_IViewObjectEx, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); /* FIXME need GUID */
typedef struct IViewObjectEx IViewObjectEx, *LPVIEWOBJECTEX;

DEFINE_GUID(IID_IProvideClassInfo, 0xb196b283, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IProvideClassInfo IProvideClassInfo, *LPPROVIDECLASSINFO;

DEFINE_GUID(IID_IProvideClassInfo2, 0xa6bc3ac0, 0xdbaa, 0x11ce, 0x9d, 0xe3, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IProvideClassInfo2 IProvideClassInfo2, *LPPROVIDECLASSINFO2;

/*****************************************************************************
 * IOleControl interface
 */
#define INTERFACE IOleControl
#define IOleControl_METHODS \
	STDMETHOD(GetControlInfo)(THIS_ CONTROLINFO *pCI) PURE; \
	STDMETHOD(OnMnemonic)(THIS_ struct tagMSG *pMsg) PURE; \
	STDMETHOD(OnAmbientPropertyChange)(THIS_ DISPID dispID) PURE; \
	STDMETHOD(FreezeEvents)(THIS_ BOOL bFreeze) PURE;
#define IOleControl_IMETHODS \
	IUnknown_IMETHODS \
	IOleControl_METHODS
ICOM_DEFINE(IOleControl,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IOleControl_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IOleControl_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IOleControl_Release(p)                   ICOM_CALL (Release,p)
/*** IOleControl methods ***/
#define IOleControl_GetControlInfo(p,a)          ICOM_CALL1(GetControlInfo,p,a)
#define IOleControl_OnMnemonic(p,a)              ICOM_CALL1(OnMnemonic,p,a)
#define IOleControl_OnAmbientPropertyChange(p,a) ICOM_CALL1(OnAmbientPropertyChange,p,a)
#define IOleControl_FreezeEvents(p,a)            ICOM_CALL1(FreezeEvents,p,a)


/*****************************************************************************
 * IOleControlSite interface
 */
#define INTERFACE IOleControlSite
#define IOleControlSite_METHODS \
	STDMETHOD(OnControlInfoChanged)(THIS) PURE; \
	STDMETHOD(LockInPlaceActive)(THIS_ BOOL fLock) PURE; \
	STDMETHOD(GetExtendedControl)(THIS_ IDispatch **ppDisp) PURE; \
	STDMETHOD(TransformCoords)(THIS_ POINTL *pPtlHimetric, POINTF *pPtfContainer, DWORD dwFlags) PURE; \
	STDMETHOD(TranslateAccelerator)(THIS_ struct tagMSG *pMsg, DWORD grfModifiers) PURE; \
	STDMETHOD(OnFocus)(THIS_ BOOL fGotFocus) PURE; \
	STDMETHOD(ShowPropertyFrame)(THIS) PURE;
#define IOleControlSite_IMETHODS \
	IUnknown_IMETHODS \
	IOleControlSite_METHODS
ICOM_DEFINE(IOleControlSite,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IOleControlSite_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IOleControlSite_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IOleControlSite_Release(p)                   ICOM_CALL (Release,p)
/*** IOleControlSite methods ***/
#define IOleControlSite_OnControlInfoChanged(p)      ICOM_CALL1(OnControlInfoChanged,p)
#define IOleControlSite_LockInPlaceActive(p,a)       ICOM_CALL1(LockInPlaceActive,p,a)
#define IOleControlSite_GetExtendedControl(p,a)      ICOM_CALL1(GetExtendedControl,p,a)
#define IOleControlSite_TransformCoords(p,a,b,c)     ICOM_CALL1(TransformCoords,p,a,b,c)
#define IOleControlSite_TranslateAccelerator(p,a,b)  ICOM_CALL1(TranslateAccelerator,p,a,b)
#define IOleControlSite_OnFocus(p,a)                 ICOM_CALL1(OnFocus,p,a)
#define IOleControlSite_ShowPropertyFrame(p)         ICOM_CALL1(ShowPropertyFrame,p)


/*****************************************************************************
 * IOleInPlaceSiteEx interface
 */
#define INTERFACE IOleInPlaceSiteEx
#define IOleInPlaceSiteEx_METHODS \
	STDMETHOD(OnInPlaceActivateEx)(THIS_ BOOL *pfNoRedraw, DWORD dwFlags) PURE; \
	STDMETHOD(OnInPlaceDeactivateEx)(THIS_ BOOL fNoRedraw) PURE; \
	STDMETHOD(RequestUIActivate)(THIS) PURE;
#define IOleInPlaceSiteEx_IMETHODS \
	IOleInPlaceSite_IMETHODS \
	IOleInPlaceSiteEx_METHODS
ICOM_DEFINE(IOleInPlaceSiteEx,IOleInPlaceSite)
#undef INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceSiteEx_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceSiteEx_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleInPlaceSiteEx_Release(p)                 ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceSiteEx_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceSiteEx_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceSite methods ***/
#define IOleInPlaceSiteEx_CanInPlaceActivate(p)      ICOM_CALL (CanInPlaceActivate,p)
#define IOleInPlaceSiteEx_OnInPlaceActivate(p)       ICOM_CALL (OnInPlaceActivate,p)
#define IOleInPlaceSiteEx_OnUIActivate(p)            ICOM_CALL (OnUIActivate,p)
#define IOleInPlaceSiteEx_GetWindowContext(p,a,b,c,d,e) ICOM_CALL5(GetWindowContext,p,a,b,c,d,e)
#define IOleInPlaceSiteEx_Scroll(p,a)                ICOM_CALL1(Scroll,p,a)
#define IOleInPlaceSiteEx_OnUIDeactivate(p,a)        ICOM_CALL1(OnUIDeactivate,p,a)
#define IOleInPlaceSiteEx_OnInPlaceDeactivate(p)     ICOM_CALL (OnInPlaceDeactivate,p)
#define IOleInPlaceSiteEx_DiscardUndoState(p)        ICOM_CALL (DiscardUndoState,p)
#define IOleInPlaceSiteEx_DeactivateAndUndo(p)       ICOM_CALL (DeactivateAndUndo,p)
#define IOleInPlaceSiteEx_OnPosRectChange(p,a)       ICOM_CALL1(OnPosRectChange,p,a)
/*** IOleInPlaceSiteEx methods ***/
#define IOleInPlaceSiteEx_OnInPlaceActivateEx(p,a,b) ICOM_CALL2(OnInPlaceActivateEx,p,a,b)
#define IOleInPlaceSiteEx_OnInPlaceDeactivateEx(p,a) ICOM_CALL1(OnInPlaceDeactivateEx,p,a)
#define IOleInPlaceSiteEx_RequestUIActivate(p)       ICOM_CALL (RequestUIActivate,p)


/*****************************************************************************
 * IOleInPlaceSiteWindowless interface
 */
#define INTERFACE IOleInPlaceSiteWindowless
#define IOleInPlaceSiteWindowless_METHODS \
	STDMETHOD(CanWindowlessActivate)(THIS) PURE; \
	STDMETHOD(GetCapture)(THIS) PURE; \
	STDMETHOD(SetCapture)(THIS_ BOOL fCapture) PURE; \
	STDMETHOD(GetFocus)(THIS) PURE; \
	STDMETHOD(SetFocus)(THIS_ BOOL fFocus) PURE; \
	STDMETHOD(GetDC)(THIS_ LPCRECT pRect, DWORD grfFlags, HDC *phDC) PURE; \
	STDMETHOD(ReleaseDC)(THIS_ HDC hDC) PURE; \
	STDMETHOD(InvalidateRect)(THIS_ LPCRECT pRect, BOOL fErase) PURE; \
	STDMETHOD(InvalidateRgn)(THIS_ HRGN hRgn, BOOL fErase) PURE; \
	STDMETHOD(ScrollRect)(THIS_ INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip) PURE; \
	STDMETHOD(AdjustRect)(THIS_ LPRECT prc) PURE; \
	STDMETHOD(OnDefWindowMessage)(THIS_ UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) PURE;
#define IOleInPlaceSiteWindowless_IMETHODS \
	IOleInPlaceSite_IMETHODS \
	IOleInPlaceSiteWindowless_METHODS
ICOM_DEFINE(IOleInPlaceSiteWindowless,IOleInPlaceSite)
#undef INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceSiteWindowless_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceSiteWindowless_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleInPlaceSiteWindowless_Release(p)                 ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceSiteWindowless_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceSiteWindowless_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceSitemethods ***/
#define IOleInPlaceSiteWindowless_CanInPlaceActivate(p)      ICOM_CALL (CanInPlaceActivate,p)
#define IOleInPlaceSiteWindowless_OnInPlaceActivate(p)       ICOM_CALL (OnInPlaceActivate,p)
#define IOleInPlaceSiteWindowless_OnUIActivate(p)            ICOM_CALL (OnUIActivate,p)
#define IOleInPlaceSiteWindowless_GetWindowContext(p,a,b,c,d,e) ICOM_CALL5(GetWindowContext,p,a,b,c,d,e)
#define IOleInPlaceSiteWindowless_Scroll(p,a)                ICOM_CALL1(Scroll,p,a)
#define IOleInPlaceSiteWindowless_OnUIDeactivate(p,a)        ICOM_CALL1(OnUIDeactivate,p,a)
#define IOleInPlaceSiteWindowless_OnInPlaceDeactivate(p)     ICOM_CALL (OnInPlaceDeactivate,p)
#define IOleInPlaceSiteWindowless_DiscardUndoState(p)        ICOM_CALL (DiscardUndoState,p)
#define IOleInPlaceSiteWindowless_DeactivateAndUndo(p)       ICOM_CALL (DeactivateAndUndo,p)
#define IOleInPlaceSiteWindowless_OnPosRectChange(p,a)       ICOM_CALL1(OnPosRectChange,p,a)
/*** IOleInPlaceSitemethods ***/
#define IOleInPlaceSiteWindowless_CanWindowlessActivate(p) ICOM_CALL (CanInPlaceActivate,p)
#define IOleInPlaceSiteWindowless_GetCapture(p)            ICOM_CALL (OnInPlaceActivate,p)
#define IOleInPlaceSiteWindowless_SetCapture(p,a)          ICOM_CALL1(OnUIActivate,p,a)
#define IOleInPlaceSiteWindowless_GetFocus(p)              ICOM_CALL (GetWindowContext,p)
#define IOleInPlaceSiteWindowless_SetFocus(p,a)            ICOM_CALL1(Scroll,p,a)
#define IOleInPlaceSiteWindowless_GetDC(p,a,b,c)           ICOM_CALL3(OnUIDeactivate,p,a,b,c)
#define IOleInPlaceSiteWindowless_ReleaseDC(p,a)           ICOM_CALL1(OnInPlaceDeactivate,p,a)
#define IOleInPlaceSiteWindowless_InvalidateRect(p,a,b)    ICOM_CALL2(DiscardUndoState,p,a,b)
#define IOleInPlaceSiteWindowless_InvalidateRgn(p,a,b)     ICOM_CALL2(DeactivateAndUndo,p,a,b)
#define IOleInPlaceSiteWindowless_ScrollRect(p,a,b,c,d)    ICOM_CALL4(OnPosRectChange,p,a,b,c,d)
#define IOleInPlaceSiteWindowless_AdjustRect(p,a)          ICOM_CALL1(OnPosRectChange,p,a)
#define IOleInPlaceSiteWindowless_OnDefWindowMessage(p,a,b,c,d) ICOM_CALL4(OnPosRectChange,p,a,b,c,d)


/*****************************************************************************
 * IOleInPlaceObjectWindowless interface
 */
#define INTERFACE IOleInPlaceObjectWindowless
#define IOleInPlaceObjectWindowless_METHODS \
	STDMETHOD(OnWindowMessage)(THIS_ UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) PURE; \
	STDMETHOD(GetDropTarget)(THIS_ IDropTarget **ppDropTarget) PURE;
#define IOleInPlaceObjectWindowless_IMETHODS \
	IOleInPlaceObject_IMETHODS \
	IOleInPlaceObjectWindowless_METHODS
ICOM_DEFINE(IOleInPlaceObjectWindowless,IOleInPlaceObject)
#undef INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceObjectWindowless_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceObjectWindowless_AddRef(p)             ICOM_CALL (AddRef,p)
#define IOleInPlaceObjectWindowless_Release(p)            ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceObjectWindowless_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceObjectWindowless_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceObject methods ***/
#define IOleInPlaceObjectWindowless_InPlaceDeactivate(p)       ICOM_CALL (InPlaceDeactivate,p)
#define IOleInPlaceObjectWindowless_UIDeactivate(p)            ICOM_CALL (UIDeactivate,p)
#define IOleInPlaceObjectWindowless_SetObjectRects(p,a,b)      ICOM_CALL2(SetObjectRects,p,a,b)
#define IOleInPlaceObjectWindowless_ReactivateAndUndo(p)       ICOM_CALL (ReactivateAndUndo,p)
/*** IOleInPlaceObjectWindowless methods ***/
#define IOleInPlaceObjectWindowless_OnWindowMessage(p,a,b,c,d) ICOM_CALL4(OnWindowMessage,p,a,b,c,d)
#define IOleInPlaceObjectWindowless_GetDropTarget(p,a)         ICOM_CALL1(GetDropTarget,p,a)


/*****************************************************************************
 * IClassFactory2 interface
 */
#define INTERFACE IClassFactory2
#define IClassFactory2_METHODS \
	STDMETHOD(GetLicInfo)(THIS_ LICINFO *pLicInfo) PURE; \
	STDMETHOD(RequestLicKey)(THIS_ DWORD dwReserved, BSTR *pBstrKey) PURE; \
	STDMETHOD(CreateInstanceLic)(THIS_ IUnknown *pUnkOuter, IUnknown *pUnkReserved, REFIID riid, BSTR bstrKey, PVOID *ppvObj) PURE;
#define IClassFactory2_IMETHODS \
	IClassFactory_IMETHODS \
	IClassFactory2_METHODS
ICOM_DEFINE(IClassFactory2,IClassFactory)
#undef INTERFACE

/*** IUnknown methods ***/
#define IClassFactory2_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IClassFactory2_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IClassFactory2_Release(p)                 ICOM_CALL (Release,p)
/*** IClassFactory methods ***/
#define IClassFactory2_CreateInstance(p,a,b,c)    ICOM_CALL3(CreateInstance,p,a,b,c)
#define IClassFactory2_LockServer(p,a)            ICOM_CALL1(LockServer,p,a)
/*** IClassFactory2 methods ***/
#define IClassFactory2_GetLicInfo(p,a)            ICOM_CALL1(GetLicInfo,p,a)
#define IClassFactory2_RequestLicKey(p,a,b)       ICOM_CALL2(RequestLicKey,p,a,b)
#define IClassFactory2_CreateInstanceLic(p,a,b,c,d,e) ICOM_CALL5(CreateInstanceLic,p,a,b,c,d,e)


/*****************************************************************************
 * IViewObject interface
 */
#define INTERFACE IViewObjectEx
#define IViewObjectEx_METHODS \
	STDMETHOD(GetRect)(THIS_ DWORD dwAspect, LPRECTL pRect) PURE; \
	STDMETHOD(GetViewStatus)(THIS_ DWORD *pdwStatus) PURE; \
	STDMETHOD(QueryHitPoint)(THIS_ DWORD dwAspect, LPCRECT pRectBounds, POINT ptlLoc, LONG lCloseHint, DWORD *pHitResult) PURE; \
	STDMETHOD(QueryHitRect)(THIS_ DWORD dwAspect, LPCRECT pRectBounds, LPCRECT pRectLoc, LONG lCloseHint, DWORD *pHitResult) PURE; \
	STDMETHOD(GetNaturalExtent)(THIS_ DWORD dwAspect, LONG lindex, DVTARGETDEVICE *ptd, HDC hicTargetDev, DVEXTENTINFO *pExtentInfo, LPSIZEL pSizel) PURE;
#define IViewObjectEx_IMETHODS \
	IViewObject2_IMETHODS \
	IViewObjectEx_METHODS
ICOM_DEFINE(IViewObjectEx,IViewObject2)
#undef INTERFACE

/*** IUnknown methods ***/
#define IViewObjectEx_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IViewObjectEx_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IViewObjectEx_Release(p)                   ICOM_CALL (Release,p)
/*** IViewObject methods ***/
#define IViewObjectEx_Draw(p,a,b,c,d,e,f,g,h,i,j)  ICOM_CALL10(Draw,p,a,b,c,d,e,f,g,h,i,j)
#define IViewObjectEx_GetColorSet(p,a,b,c,d,e,f)   ICOM_CALL6(GetColorSet,p,a,b,c,d,e,f)
#define IViewObjectEx_Freeze(p,a,b,c,d)            ICOM_CALL4(Freeze,p,a,b,c,d)
#define IViewObjectEx_Unfreeze(p,a)                ICOM_CALL1(Unfreeze,p,a)
#define IViewObjectEx_SetAdvise(p,a,b,c)           ICOM_CALL3(SetAdvise,p,a,b,c)
#define IViewObjectEx_GetAdvise(p,a,b,c)           ICOM_CALL3(GetAdvise,p,a,b,c)
/*** IViewObject2 methods ***/
#define IViewObjectEx_GetExtent(p,a,b,c,d)         ICOM_CALL4(GetExtent,p,a,b,c,d)
/*** IViewObjectEx methods ***/
#define IViewObjectEx_GetRect(p,a,b)                  ICOM_CALL2(GetRect,p,a,b)
#define IViewObjectEx_GetViewStatus(p,a)              ICOM_CALL1(GetViewStatus,p,a)
#define IViewObjectEx_QueryHitPoint(p,a,b,c,d,e)      ICOM_CALL5(QueryHitPoint,p,a,b,c,d,e)
#define IViewObjectEx_QueryHitRect(p,a,b,c,d,e)       ICOM_CALL5(QueryHitRect,p,a,b,c,d,e)
#define IViewObjectEx_GetNaturalExtent(p,a,b,c,d,e,f) ICOM_CALL6(GetNaturalExtent,p,a,b,c,d,e,f)


/*****************************************************************************
 * IProvideClassInfo interface
 */
#ifdef __WINESRC__
#undef GetClassInfo
#endif

#define INTERFACE IProvideClassInfo
#define IProvideClassInfo_METHODS \
	STDMETHOD(GetClassInfo)(THIS_ ITypeInfo **ppTI) PURE;
#define IProvideClassInfo_IMETHODS \
	IUnknown_IMETHODS \
	IProvideClassInfo_METHODS
ICOM_DEFINE(IProvideClassInfo,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IProvideClassInfo_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IProvideClassInfo_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IProvideClassInfo_Release(p)                   ICOM_CALL (Release,p)
/*** IProvideClassInfo methods ***/
#define IProvideClassInfo_GetClassInfo(p,a)            ICOM_CALL1(GetClassInfo,p,a)



/*****************************************************************************
 * IProvideClassInfo2 interface
 */
#define INTERFACE IProvideClassInfo2
#define IProvideClassInfo2_METHODS \
	STDMETHOD(GetGUID)(THIS_ DWORD dwGuidKind, GUID *pGUID) PURE;
#define IProvideClassInfo2_IMETHODS \
	IProvideClassInfo_IMETHODS \
	IProvideClassInfo2_METHODS
ICOM_DEFINE(IProvideClassInfo2,IProvideClassInfo)
#undef INTERFACE

/*** IUnknown methods ***/
#define IProvideClassInfo2_QueryInterface(p,a,b)   ICOM_CALL2(QueryInterface,p,a,b)
#define IProvideClassInfo2_AddRef(p)               ICOM_CALL (AddRef,p)
#define IProvideClassInfo2_Release(p)              ICOM_CALL (Release,p)
/*** IProvideClassInfo methods ***/
#define IProvideClassInfo2_GetClassInfo(p,a)       ICOM_CALL1(GetClassInfo,p,a)
/*** IProvideClassInfo2 methods ***/
#define IProvideClassInfo2_GetGUID(p,a,b)          ICOM_CALL2(GetGUID,p,a,b)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_CONTROL_H */
