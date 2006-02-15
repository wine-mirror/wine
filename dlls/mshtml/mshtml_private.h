/*
 * Copyright 2005 Jacek Caban
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

#include "docobj.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "hlink.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "nsiface.h"

#define NS_OK                     ((nsresult)0x00000000L)
#define NS_ERROR_FAILURE          ((nsresult)0x80004005L)
#define NS_NOINTERFACE            ((nsresult)0x80004002L)
#define NS_ERROR_NOT_IMPLEMENTED  ((nsresult)0x80004001L)
#define NS_ERROR_INVALID_ARG      ((nsresult)0x80070057L) 
#define NS_ERROR_UNEXPECTED       ((nsresult)0x8000ffffL)

#define NS_FAILED(res) ((res) & 0x80000000)
#define NS_SUCCEEDED(res) (!NS_FAILED(res))

#define NSAPI WINAPI

typedef struct BindStatusCallback BindStatusCallback;

typedef struct {
    const IHTMLDocument2Vtbl              *lpHTMLDocument2Vtbl;
    const IHTMLDocument3Vtbl              *lpHTMLDocument3Vtbl;
    const IPersistMonikerVtbl             *lpPersistMonikerVtbl;
    const IPersistFileVtbl                *lpPersistFileVtbl;
    const IMonikerPropVtbl                *lpMonikerPropVtbl;
    const IOleObjectVtbl                  *lpOleObjectVtbl;
    const IOleDocumentVtbl                *lpOleDocumentVtbl;
    const IOleDocumentViewVtbl            *lpOleDocumentViewVtbl;
    const IOleInPlaceActiveObjectVtbl     *lpOleInPlaceActiveObjectVtbl;
    const IViewObject2Vtbl                *lpViewObject2Vtbl;
    const IOleInPlaceObjectWindowlessVtbl *lpOleInPlaceObjectWindowlessVtbl;
    const IServiceProviderVtbl            *lpServiceProviderVtbl;
    const IOleCommandTargetVtbl           *lpOleCommandTargetVtbl;
    const IOleControlVtbl                 *lpOleControlVtbl;
    const IHlinkTargetVtbl                *lpHlinkTargetVtbl;

    LONG ref;

    NSContainer *nscontainer;

    IOleClientSite *client;
    IDocHostUIHandler *hostui;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;

    HWND hwnd;

    BOOL in_place_active;
    BOOL ui_active;
    BOOL window_active;
    BOOL has_key_path;
    BOOL container_locked;

    BindStatusCallback *status_callback;
} HTMLDocument;

struct NSContainer {
    const nsIWebBrowserChromeVtbl     *lpWebBrowserChromeVtbl;
    const nsIContextMenuListenerVtbl  *lpContextMenuListenerVtbl;
    const nsIURIContentListenerVtbl   *lpURIContentListenerVtbl;
    const nsIEmbeddingSiteWindowVtbl  *lpEmbeddingSiteWindowVtbl;

    nsIWebBrowser *webbrowser;
    nsIWebNavigation *navigation;
    nsIBaseWindow *window;
    nsIWebBrowserStream *stream;
    nsIWebBrowserFocus *focus;

    LONG ref;

    HTMLDocument *doc;

    HWND hwnd;

    BOOL load_call; /* hack */
};

#define HTMLDOC(x)       ((IHTMLDocument2*)               &(x)->lpHTMLDocument2Vtbl)
#define HTMLDOC3(x)      ((IHTMLDocument3*)               &(x)->lpHTMLDocument3Vtbl)
#define PERSIST(x)       ((IPersist*)                     &(x)->lpPersistFileVtbl)
#define PERSISTMON(x)    ((IPersistMoniker*)              &(x)->lpPersistMonikerVtbl)
#define PERSISTFILE(x)   ((IPersistFile*)                 &(x)->lpPersistFileVtbl)
#define MONPROP(x)       ((IMonikerProp*)                 &(x)->lpMonikerPropVtbl)
#define OLEOBJ(x)        ((IOleObject*)                   &(x)->lpOleObjectVtbl)
#define OLEDOC(x)        ((IOleDocument*)                 &(x)->lpOleDocumentVtbl)
#define DOCVIEW(x)       ((IOleDocumentView*)             &(x)->lpOleDocumentViewVtbl)
#define OLEWIN(x)        ((IOleWindow*)                   &(x)->lpOleInPlaceActiveObjectVtbl)
#define ACTOBJ(x)        ((IOleInPlaceActiveObject*)      &(x)->lpOleInPlaceActiveObjectVtbl)
#define VIEWOBJ(x)       ((IViewObject*)                  &(x)->lpViewObject2Vtbl)
#define VIEWOBJ2(x)      ((IViewObject2*)                 &(x)->lpViewObject2Vtbl)
#define INPLACEOBJ(x)    ((IOleInPlaceObject*)            &(x)->lpOleInPlaceObjectWindowlessVtbl)
#define INPLACEWIN(x)    ((IOleInPlaceObjectWindowless*)  &(x)->lpOleInPlaceObjectWindowlessVtbl)
#define SERVPROV(x)      ((IServiceProvider*)             &(x)->lpServiceProviderVtbl)
#define CMDTARGET(x)     ((IOleCommandTarget*)            &(x)->lpOleCommandTargetVtbl)
#define CONTROL(x)       ((IOleControl*)                  &(x)->lpOleControlVtbl)
#define STATUSCLB(x)     ((IBindStatusCallback*)          &(x)->lpBindStatusCallbackVtbl)
#define HLNKTARGET(x)    ((IHlinkTarget*)                 &(x)->lpHlinkTargetVtbl)

#define NSWBCHROME(x)    ((nsIWebBrowserChrome*)          &(x)->lpWebBrowserChromeVtbl)
#define NSCML(x)         ((nsIContextMenuListener*)       &(x)->lpContextMenuListenerVtbl)
#define NSURICL(x)       ((nsIURIContentListener*)        &(x)->lpURIContentListenerVtbl)
#define NSEMBWNDS(x)     ((nsIEmbeddingSiteWindow*)       &(x)->lpEmbeddingSiteWindowVtbl)

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);

void HTMLDocument_HTMLDocument3_Init(HTMLDocument*);
void HTMLDocument_Persist_Init(HTMLDocument*);
void HTMLDocument_OleObj_Init(HTMLDocument*);
void HTMLDocument_View_Init(HTMLDocument*);
void HTMLDocument_Window_Init(HTMLDocument*);
void HTMLDocument_Service_Init(HTMLDocument*);
void HTMLDocument_Hlink_Init(HTMLDocument*);

void NSContainer_Create(HTMLDocument *doc);
void NSContainer_Release(NSContainer*);

void HTMLDocument_LockContainer(HTMLDocument*,BOOL);
void HTMLDocument_ShowContextMenu(HTMLDocument*,DWORD,POINT*);

HRESULT ProtocolFactory_Create(REFCLSID,REFIID,void**);

void close_gecko(void);
void register_nsservice(nsIComponentRegistrar*);
void init_nsio(nsIComponentManager*,nsIComponentRegistrar*);

void hlink_frame_navigate(NSContainer*,IHlinkFrame*,LPCWSTR,nsIInputStream*);

nsIURI *get_nsIURI(LPCWSTR);

nsACString *nsACString_Create(void);
PRUint32 nsACString_GetData(const nsACString*,const char**,PRBool*);
void nsACString_SetData(nsACString*,const char*);
void nsACString_Destroy(nsACString*);

nsIInputStream *create_nsstream(const char*,PRInt32);

IHlink *Hlink_Create(void);

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);

extern LONG module_ref;
#define LOCK_MODULE()   InterlockedIncrement(&module_ref)
#define UNLOCK_MODULE() InterlockedDecrement(&module_ref)

extern HINSTANCE hInst;
