/*
 * Defines the COM interfaces and APIs related to structured data storage.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_CONTROL_H
#define __WINE_WINE_OBJ_CONTROL_H


#include "winbase.h"


/*****************************************************************************
 * Declare the structures
 */

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
	BOOL32 fRuntimeKeyAvail;
	BOOL32 fLicVerified;
} LICINFO, *LPLICINFO;

typedef struct tagCONTROLINFO
{
	ULONG cb;
	HACCEL32 hAccel;
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

/*****************************************************************************
 * IOleControl interface
 */
#define ICOM_INTERFACE IOleControl
#define IOleControl_METHODS \
	ICOM_METHOD1(HRESULT,GetControlInfo, CONTROLINFO*,pCI); \
	ICOM_METHOD1(HRESULT,OnMnemonic, MSG32*,pMsg); \
	ICOM_METHOD1(HRESULT,OnAmbientPropertyChange, DISPID,dispID); \
	ICOM_METHOD1(HRESULT,FreezeEvents, BOOL32,bFreeze);
#define IOleControl_IMETHODS \
	IUnknown_IMETHODS \
	IOleControl_METHODS
ICOM_DEFINE(IOleControl,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknwon methods ***/
#define IOleControl_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IOleControl_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IOleControl_Release(p)                   ICOM_CALL (Release,p)
/*** IOleControl methods ***/
#define IOleControl_GetControlInfo(p,a)          ICOM_CALL1(GetControlInfo,p,a)
#define IOleControl_OnMnemonic(p,a)              ICOM_CALL1(OnMnemonic,p,a)
#define IOleControl_OnAmbientPropertyChange(p,a) ICOM_CALL1(OnAmbientPropertyChange,p,a)
#define IOleControl_FreezeEvents(p,a)            ICOM_CALL1(FreezeEvents,p,a)
#endif
				

/*****************************************************************************
 * IOleControlSite interface
 */
#define ICOM_INTERFACE IOleControlSite 
#define IOleControlSite_METHODS \
	ICOM_METHOD (HRESULT,OnControlInfoChanged); \
	ICOM_METHOD1(HRESULT,LockInPlaceActive, BOOL32,fLock); \
	ICOM_METHOD3(HRESULT,TransformCoords, POINTL*,pPtlHimetric, POINTF*,pPtfContainer, DWORD,dwFlags); \
	ICOM_METHOD2(HRESULT,TranslateAccelerator, MSG32*,pMsg, DWORD,grfModifiers) ;\
	ICOM_METHOD1(HRESULT,OnFocus, BOOL32,fGotFocus); \
	ICOM_METHOD (HRESULT,ShowPropertyFrame);
#define IOleControlSite_IMETHODS \
	IUnknown_IMETHODS \
	IOleControlSite_METHODS
ICOM_DEFINE(IOleControlSite,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknwon methods ***/
#define IOleControlSite_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IOleControlSite_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IOleControlSite_Release(p)                   ICOM_CALL (Release,p)
/*** IOleControlSite methods ***/
#define IOleControlSite_OnControlInfoChanged(p)      ICOM_CALL1(OnControlInfoChanged,p)
#define IOleControlSite_LockInPlaceActive(p,a)       ICOM_CALL1(LockInPlaceActive,p,a)
#define IOleControlSite_TransformCoords(p,a,b,c)     ICOM_CALL1(TransformCoords,p,a,b,c)
#define IOleControlSite_TranslateAccelerator(p,a,b)  ICOM_CALL1(TranslateAccelerator,p,a,b)
#define IOleControlSite_OnFocus(p,a)                 ICOM_CALL1(OnFocus,p,a)
#define IOleControlSite_ShowPropertyFrame(p)         ICOM_CALL1(ShowPropertyFrame,p)
#endif
				
				
/*****************************************************************************
 * IOleInPlaceSiteEx interface
 */
#define ICOM_INTERFACE IOleInPlaceSiteEx
#define IOleInPlaceSiteEx_METHODS \
	ICOM_METHOD2(HRESULT,OnInPlaceActivateEx, BOOL32*,pfNoRedraw, DWORD,dwFlags); \
	ICOM_METHOD1(HRESULT,OnInPlaceDeactivateEx, BOOL32,fNoRedraw); \
	ICOM_METHOD (HRESULT,RequestUIActivate);
#define IOleInPlaceSiteEx_IMETHODS \
	IOleInPlaceSite_IMETHODS \
	IOleInPlaceSiteEx_METHODS
ICOM_DEFINE(IOleInPlaceSiteEx,IOleInPlaceSite)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
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
#endif
				 

/*****************************************************************************
 * IOleInPlaceSiteWindowless interface
 */
#define ICOM_INTERFACE IOleInPlaceSiteWindowless
#define IOleInPlaceSiteWindowless_METHODS \
	ICOM_METHOD (HRESULT,CanWindowlessActivate); \
	ICOM_METHOD (HRESULT,GetCapture); \
	ICOM_METHOD1(HRESULT,SetCapture, BOOL32,fCapture); \
	ICOM_METHOD (HRESULT,GetFocus); \
	ICOM_METHOD1(HRESULT,SetFocus, BOOL32,fFocus); \
	ICOM_METHOD3(HRESULT,GetDC, LPCRECT32,pRect, DWORD,grfFlags, HDC32*,phDC); \
	ICOM_METHOD1(HRESULT,ReleaseDC, HDC32,hDC); \
	ICOM_METHOD2(HRESULT,InvalidateRect, LPCRECT32,pRect, BOOL32,fErase); \
	ICOM_METHOD2(HRESULT,InvalidateRgn, HRGN32,hRgn, BOOL32,fErase); \
	ICOM_METHOD4(HRESULT,ScrollRect, INT32,dx, INT32,dy, LPCRECT32,pRectScroll, LPCRECT32,pRectClip); \
	ICOM_METHOD1(HRESULT,AdjustRect, LPRECT32,prc); \
	ICOM_METHOD4(HRESULT,OnDefWindowMessage, UINT32,msg, WPARAM32,wParam, LPARAM,lParam, LRESULT*,plResult);
#define IOleInPlaceSiteWindowless_IMETHODS \
	IOleInPlaceSite_IMETHODS \
	IOleInPlaceSiteWindowless_METHODS
ICOM_DEFINE(IOleInPlaceSiteWindowless,IOleInPlaceSite)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
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
#endif


/*****************************************************************************
 * IOleInPlaceObjectWindowless interface
 */
#define ICOM_INTERFACE IOleInPlaceObjectWindowless
#define IOleInPlaceObjectWindowless_METHODS \
	ICOM_METHOD4(HRESULT,OnWindowMessage, UINT32,msg, WPARAM32,wParam, LPARAM,lParam, LRESULT*,plResult); \
	ICOM_METHOD1(HRESULT,GetDropTarget, IDropTarget**,ppDropTarget);
#define IOleInPlaceObjectWindowless_IMETHODS \
	IOleInPlaceObject_IMETHODS \
	IOleInPlaceObjectWindowless_METHODS
ICOM_DEFINE(IOleInPlaceObjectWindowless,IOleInPlaceObject)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
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
#endif
				 

/*****************************************************************************
 * IClassFactory2 interface
 */
#define ICOM_INTERFACE IClassFactory2
#define IClassFactory2_METHODS \
	ICOM_METHOD1(HRESULT,GetLicInfo, LICINFO*,pLicInfo); \
	ICOM_METHOD2(HRESULT,RequestLicKey, DWORD,dwReserved, BSTR32*,pBstrKey); \
	ICOM_METHOD5(HRESULT,CreateInstanceLic, IUnknown*,pUnkOuter, IUnknown*,pUnkReserved, REFIID,riid, BSTR32,bstrKey, PVOID*,ppvObj);
#define IClassFactory2_IMETHODS \
	IClassFactory_IMETHODS \
	IClassFactory2_METHODS
ICOM_DEFINE(IClassFactory2,IClassFactory)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknwon methods ***/
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
#endif


#endif /* __WINE_WINE_OBJ_CONTROL_H */


