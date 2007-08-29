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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SHDOCVW_H
#define __WINE_SHDOCVW_H

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
 * Shell Instance Objects
 */
extern HRESULT SHDOCVW_GetShellInstanceObjectClassObject(REFCLSID rclsid, 
    REFIID riid, LPVOID *ppvClassObj);

/**********************************************************************
 * WebBrowser declaration for SHDOCVW.DLL
 */

typedef struct ConnectionPoint ConnectionPoint;

typedef struct {
    const IConnectionPointContainerVtbl *lpConnectionPointContainerVtbl;

    ConnectionPoint *wbe2;
    ConnectionPoint *wbe;
    ConnectionPoint *pns;

    IUnknown *impl;
} ConnectionPointContainer;

typedef struct {
    const IOleClientSiteVtbl      *lpOleClientSiteVtbl;
    const IOleInPlaceSiteVtbl     *lpOleInPlaceSiteVtbl;
    const IDocHostUIHandler2Vtbl  *lpDocHostUIHandlerVtbl;
    const IOleDocumentSiteVtbl    *lpOleDocumentSiteVtbl;
    const IOleCommandTargetVtbl   *lpOleCommandTargetVtbl;
    const IDispatchVtbl           *lpDispatchVtbl;
    const IServiceProviderVtbl    *lpServiceProviderVtbl;

    /* Interfaces of InPlaceFrame object */
    const IOleInPlaceFrameVtbl          *lpOleInPlaceFrameVtbl;

    IDispatch *disp;

    IDispatch *client_disp;
    IDocHostUIHandler *hostui;
    IOleInPlaceFrame *frame;

    IUnknown *document;
    IOleDocumentView *view;

    HWND hwnd;
    HWND frame_hwnd;

    LPOLESTR url;

    VARIANT_BOOL silent;
    VARIANT_BOOL offline;

    ConnectionPointContainer cps;
} DocHost;

struct WebBrowser {
    /* Interfaces available via WebBrowser object */

    const IWebBrowser2Vtbl              *lpWebBrowser2Vtbl;
    const IOleObjectVtbl                *lpOleObjectVtbl;
    const IOleInPlaceObjectVtbl         *lpOleInPlaceObjectVtbl;
    const IOleControlVtbl               *lpOleControlVtbl;
    const IPersistStorageVtbl           *lpPersistStorageVtbl;
    const IPersistStreamInitVtbl        *lpPersistStreamInitVtbl;
    const IProvideClassInfo2Vtbl        *lpProvideClassInfoVtbl;
    const IViewObject2Vtbl              *lpViewObjectVtbl;
    const IOleInPlaceActiveObjectVtbl   *lpOleInPlaceActiveObjectVtbl;
    const IOleCommandTargetVtbl         *lpOleCommandTargetVtbl;
    const IHlinkFrameVtbl               *lpHlinkFrameVtbl;

    LONG ref;

    INT version;

    IOleClientSite *client;
    IOleContainer *container;
    IOleInPlaceSite *inplace;

    /* window context */

    HWND frame_hwnd;
    IOleInPlaceUIWindow *uiwindow;
    RECT pos_rect;
    RECT clip_rect;
    OLEINPLACEFRAMEINFO frameinfo;
    SIZEL extent;

    HWND shell_embedding_hwnd;

    VARIANT_BOOL visible;
    VARIANT_BOOL menu_bar;
    VARIANT_BOOL address_bar;
    VARIANT_BOOL status_bar;
    VARIANT_BOOL tool_bar;

    DocHost doc_host;
};

struct InternetExplorer {
    const IWebBrowser2Vtbl *lpWebBrowser2Vtbl;

    LONG ref;

    HWND frame_hwnd;

    DocHost doc_host;
};

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
#define OLECMD(x)       ((IOleCommandTarget*)           &(x)->lpOleCommandTargetVtbl)
#define HLINKFRAME(x)   ((IHlinkFrame*)                 &(x)->lpHlinkFrameVtbl)

#define CLIENTSITE(x)   ((IOleClientSite*)              &(x)->lpOleClientSiteVtbl)
#define INPLACESITE(x)  ((IOleInPlaceSite*)             &(x)->lpOleInPlaceSiteVtbl)
#define DOCHOSTUI(x)    ((IDocHostUIHandler*)           &(x)->lpDocHostUIHandlerVtbl)
#define DOCHOSTUI2(x)   ((IDocHostUIHandler2*)          &(x)->lpDocHostUIHandlerVtbl)
#define DOCSITE(x)      ((IOleDocumentSite*)            &(x)->lpOleDocumentSiteVtbl)
#define CLDISP(x)       ((IDispatch*)                   &(x)->lpDispatchVtbl)
#define SERVPROV(x)     ((IServiceProvider*)            &(x)->lpServiceProviderVtbl)

#define INPLACEFRAME(x) ((IOleInPlaceFrame*)            &(x)->lpOleInPlaceFrameVtbl)

void WebBrowser_OleObject_Init(WebBrowser*);
void WebBrowser_ViewObject_Init(WebBrowser*);
void WebBrowser_Persist_Init(WebBrowser*);
void WebBrowser_ClassInfo_Init(WebBrowser*);
void WebBrowser_HlinkFrame_Init(WebBrowser*);

void WebBrowser_OleObject_Destroy(WebBrowser*);

void DocHost_Init(DocHost*,IDispatch*);
void DocHost_ClientSite_Init(DocHost*);
void DocHost_Frame_Init(DocHost*);

void DocHost_Release(DocHost*);
void DocHost_ClientSite_Release(DocHost*);

void ConnectionPointContainer_Init(ConnectionPointContainer*,IUnknown*);
void ConnectionPointContainer_Destroy(ConnectionPointContainer*);

HRESULT WebBrowserV1_Create(IUnknown*,REFIID,void**);
HRESULT WebBrowserV2_Create(IUnknown*,REFIID,void**);

void create_doc_view_hwnd(DocHost*);
void deactivate_document(DocHost*);
void call_sink(ConnectionPoint*,DISPID,DISPPARAMS*);
HRESULT navigate_url(DocHost*,BSTR,const VARIANT*,const VARIANT*,VARIANT*,VARIANT*);

HRESULT InternetExplorer_Create(IUnknown*,REFIID,void**);
void InternetExplorer_WebBrowser_Init(InternetExplorer*);

HRESULT CUrlHistory_Create(IUnknown*,REFIID,void**);

#define WB_WM_NAVIGATE2 (WM_USER+100)

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

/**********************************************************************
 * Dll lifetime tracking declaration for shdocvw.dll
 */
extern LONG SHDOCVW_refCount;
static inline void SHDOCVW_LockModule(void) { InterlockedIncrement( &SHDOCVW_refCount ); }
static inline void SHDOCVW_UnlockModule(void) { InterlockedDecrement( &SHDOCVW_refCount ); }

extern HINSTANCE shdocvw_hinstance;
extern void register_iewindow_class(void);
extern void unregister_iewindow_class(void);

HRESULT register_class_object(BOOL);
HRESULT get_typeinfo(ITypeInfo**);

/* memory allocation functions */

static inline void *shdocvw_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *shdocvw_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL shdocvw_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

#endif /* __WINE_SHDOCVW_H */
