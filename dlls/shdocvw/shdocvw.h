/*
 * Header includes for shdocvw.dll
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#ifndef __WINE_SHDOCVW_H
#define __WINE_SHDOCVW_H

#define COM_NO_WINDOWS_H
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "olectl.h"
#include "shlobj.h"
#include "exdisp.h"
#include "mshtmhst.h"
#include "hlink.h"

/**********************************************************************
 * IClassFactory declaration for SHDOCVW.DLL
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl *lpVtbl;
    LONG ref;
} IClassFactoryImpl;

extern IClassFactoryImpl SHDOCVW_ClassFactory;

/**********************************************************************
 * Shell Instance Objects
 */
extern HRESULT SHDOCVW_GetShellInstanceObjectClassObject(REFCLSID rclsid, 
    REFIID riid, LPVOID *ppvClassObj);

/**********************************************************************
 * WebBrowser declaration for SHDOCVW.DLL
 */

typedef struct ConnectionPoint ConnectionPoint;

typedef struct {
    /* Interfaces available via WebBrowser object */

    const IWebBrowser2Vtbl              *lpWebBrowser2Vtbl;
    const IOleObjectVtbl                *lpOleObjectVtbl;
    const IOleInPlaceObjectVtbl         *lpOleInPlaceObjectVtbl;
    const IOleControlVtbl               *lpOleControlVtbl;
    const IPersistStorageVtbl           *lpPersistStorageVtbl;
    const IPersistStreamInitVtbl        *lpPersistStreamInitVtbl;
    const IProvideClassInfo2Vtbl        *lpProvideClassInfoVtbl;
    const IConnectionPointContainerVtbl *lpConnectionPointContainerVtbl;
    const IViewObject2Vtbl              *lpViewObjectVtbl;
    const IOleInPlaceActiveObjectVtbl   *lpOleInPlaceActiveObjectVtbl;
    const IOleCommandTargetVtbl         *lpWBOleCommandTargetVtbl;
    const IHlinkFrameVtbl               *lpHlinkFrameVtbl;

    /* Interfaces available for embeded document */

    const IOleClientSiteVtbl            *lpOleClientSiteVtbl;
    const IOleInPlaceSiteVtbl           *lpOleInPlaceSiteVtbl;
    const IDocHostUIHandler2Vtbl        *lpDocHostUIHandlerVtbl;
    const IOleDocumentSiteVtbl          *lpOleDocumentSiteVtbl;
    const IOleCommandTargetVtbl         *lpClOleCommandTargetVtbl;
    const IDispatchVtbl                 *lpDispatchVtbl;
    const IServiceProviderVtbl          *lpClServiceProviderVtbl;

    /* Interfaces of InPlaceFrame object */

    const IOleInPlaceFrameVtbl          *lpOleInPlaceFrameVtbl;

    LONG ref;

    IUnknown *document;

    IOleClientSite *client;
    IOleContainer *container;
    IOleDocumentView *view;
    IDocHostUIHandler *hostui;

    LPOLESTR url;

    /* window context */

    HWND iphwnd;
    HWND frame_hwnd;
    IOleInPlaceFrame *frame;
    IOleInPlaceUIWindow *uiwindow;
    RECT pos_rect;
    RECT clip_rect;
    OLEINPLACEFRAMEINFO frameinfo;

    HWND doc_view_hwnd;
    HWND shell_embedding_hwnd;

    /* Connection points */

    ConnectionPoint *cp_wbe2;
    ConnectionPoint *cp_wbe;
    ConnectionPoint *cp_pns;
} WebBrowser;

#define WEBBROWSER(x)   ((IWebBrowser*)                 &(x)->lpWebBrowser2Vtbl)
#define WEBBROWSER2(x)  ((IWebBrowser2*)                &(x)->lpWebBrowser2Vtbl)
#define OLEOBJ(x)       ((IOleObject*)                  &(x)->lpOleObjectVtbl)
#define INPLACEOBJ(x)   ((IOleInPlaceObject*)           &(x)->lpOleInPlaceObjectVtbl)
#define CONTROL(x)      ((IOleControl*)                 &(x)->lpOleControlVtbl)
#define PERSTORAGE(x)   ((IPersistStorage*)             &(x)->lpPersistStorageVtbl)
#define PERSTRINIT(x)   ((IPersistStreamInit*)          &(x)->lpPersistStreamInitVtbl)
#define CLASSINFO(x)    ((IProvideClassInfo2*)          &(x)->lpProvideClassInfoVtbl)
#define CONPTCONT(x)    ((IConnectionPointContainer*)   &(x)->lpConnectionPointContainerVtbl)
#define VIEWOBJ(x)      ((IViewObject*)                 &(x)->lpViewObjectVtbl);
#define VIEWOBJ2(x)     ((IViewObject2*)                &(x)->lpViewObjectVtbl);
#define ACTIVEOBJ(x)    ((IOleInPlaceActiveObject*)     &(x)->lpOleInPlaceActiveObjectVtbl)
#define WBOLECMD(x)     ((IOleCommandTarget*)           &(x)->lpWBOleCommandTargetVtbl)
#define HLINKFRAME(x)   ((IHlinkFrame*)                 &(x)->lpHlinkFrameVtbl)

#define CLIENTSITE(x)   ((IOleClientSite*)              &(x)->lpOleClientSiteVtbl)
#define INPLACESITE(x)  ((IOleInPlaceSite*)             &(x)->lpOleInPlaceSiteVtbl)
#define DOCHOSTUI(x)    ((IDocHostUIHandler*)           &(x)->lpDocHostUIHandlerVtbl)
#define DOCHOSTUI2(x)   ((IDocHostUIHandler2*)          &(x)->lpDocHostUIHandlerVtbl)
#define DOCSITE(x)      ((IOleDocumentSite*)            &(x)->lpOleDocumentSiteVtbl)
#define CLOLECMD(x)     ((IOleCommandTarget*)           &(x)->lpClOleCommandTargetVtbl)
#define CLDISP(x)       ((IDispatch*)                   &(x)->lpDispatchVtbl)
#define CLSERVPROV(x)   ((IServiceProvider*)            &(x)->lpClServiceProviderVtbl)

#define INPLACEFRAME(x) ((IOleInPlaceFrame*)            &(x)->lpOleInPlaceFrameVtbl)

void WebBrowser_OleObject_Init(WebBrowser*);
void WebBrowser_ViewObject_Init(WebBrowser*);
void WebBrowser_Persist_Init(WebBrowser*);
void WebBrowser_ClassInfo_Init(WebBrowser*);
void WebBrowser_Events_Init(WebBrowser*);
void WebBrowser_HlinkFrame_Init(WebBrowser*);

void WebBrowser_ClientSite_Init(WebBrowser*);
void WebBrowser_DocHost_Init(WebBrowser*);

void WebBrowser_Frame_Init(WebBrowser*);

void WebBrowser_OleObject_Destroy(WebBrowser*);
void WebBrowser_Events_Destroy(WebBrowser*);
void WebBrowser_ClientSite_Destroy(WebBrowser*);

HRESULT WebBrowser_Create(IUnknown*,REFIID,void**);

void create_doc_view_hwnd(WebBrowser *This);
void deactivate_document(WebBrowser*);
void call_sink(ConnectionPoint*,DISPID,DISPPARAMS*);
HRESULT navigate_url(WebBrowser*,LPCWSTR,PBYTE,ULONG,LPWSTR);

#define WB_WM_NAVIGATE2 (WM_USER+100)

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

/**********************************************************************
 * Dll lifetime tracking declaration for shdocvw.dll
 */
extern LONG SHDOCVW_refCount;
static inline void SHDOCVW_LockModule(void) { InterlockedIncrement( &SHDOCVW_refCount ); }
static inline void SHDOCVW_UnlockModule(void) { InterlockedDecrement( &SHDOCVW_refCount ); }

extern HINSTANCE shdocvw_hinstance;

#endif /* __WINE_SHDOCVW_H */
