/*
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

#include "docobj.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "hlink.h"

#include "wine/list.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "nsiface.h"

#define GENERATE_MSHTML_NS_FAILURE(code) \
    ((nsresult) ((PRUint32)(1<<31) | ((PRUint32)(0x45+6)<<16) | (PRUint32)(code)))

#define NS_OK                     ((nsresult)0x00000000L)
#define NS_ERROR_FAILURE          ((nsresult)0x80004005L)
#define NS_NOINTERFACE            ((nsresult)0x80004002L)
#define NS_ERROR_NOT_IMPLEMENTED  ((nsresult)0x80004001L)
#define NS_ERROR_INVALID_ARG      ((nsresult)0x80070057L) 
#define NS_ERROR_UNEXPECTED       ((nsresult)0x8000ffffL)
#define NS_ERROR_UNKNOWN_PROTOCOL ((nsresult)0x804b0012L)

#define WINE_NS_LOAD_FROM_MONIKER GENERATE_MSHTML_NS_FAILURE(0)

#define NS_FAILED(res) ((res) & 0x80000000)
#define NS_SUCCEEDED(res) (!NS_FAILED(res))

#define NSAPI WINAPI

#define NS_ELEMENT_NODE   1
#define NS_DOCUMENT_NODE  9

typedef struct HTMLDOMNode HTMLDOMNode;
typedef struct ConnectionPoint ConnectionPoint;
typedef struct BSCallback BSCallback;

typedef struct {
    const IHTMLWindow2Vtbl *lpHTMLWindow2Vtbl;

    LONG ref;

    HTMLDocument *doc;
    nsIDOMWindow *nswindow;

    struct list entry;
} HTMLWindow;

typedef enum {
    UNKNOWN_USERMODE,
    BROWSEMODE,
    EDITMODE        
} USERMODE;

struct HTMLDocument {
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
    const IConnectionPointContainerVtbl   *lpConnectionPointContainerVtbl;
    const IPersistStreamInitVtbl          *lpPersistStreamInitVtbl;

    LONG ref;

    NSContainer *nscontainer;
    HTMLWindow *window;

    IOleClientSite *client;
    IDocHostUIHandler *hostui;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;

    BSCallback *bscallback;
    IMoniker *mon;

    HWND hwnd;
    HWND tooltips_hwnd;

    DOCHOSTUIINFO hostinfo;

    USERMODE usermode;
    READYSTATE readystate;
    BOOL in_place_active;
    BOOL ui_active;
    BOOL window_active;
    BOOL has_key_path;
    BOOL container_locked;

    DWORD update;

    ConnectionPoint *cp_htmldocevents;
    ConnectionPoint *cp_htmldocevents2;
    ConnectionPoint *cp_propnotif;

    HTMLDOMNode *nodes;
};

struct NSContainer {
    const nsIWebBrowserChromeVtbl       *lpWebBrowserChromeVtbl;
    const nsIContextMenuListenerVtbl    *lpContextMenuListenerVtbl;
    const nsIURIContentListenerVtbl     *lpURIContentListenerVtbl;
    const nsIEmbeddingSiteWindowVtbl    *lpEmbeddingSiteWindowVtbl;
    const nsITooltipListenerVtbl        *lpTooltipListenerVtbl;
    const nsIInterfaceRequestorVtbl     *lpInterfaceRequestorVtbl;
    const nsIWeakReferenceVtbl          *lpWeakReferenceVtbl;
    const nsISupportsWeakReferenceVtbl  *lpSupportsWeakReferenceVtbl;
    const nsIDOMEventListenerVtbl       *lpDOMEventListenerVtbl;

    nsIWebBrowser *webbrowser;
    nsIWebNavigation *navigation;
    nsIBaseWindow *window;
    nsIWebBrowserFocus *focus;

    nsIController *editor_controller;

    LONG ref;

    NSContainer *parent;
    HTMLDocument *doc;

    nsIURIContentListener *content_listener;

    HWND hwnd;

    BSCallback *bscallback; /* hack */
};

typedef struct {
    const nsIHttpChannelVtbl *lpHttpChannelVtbl;
    const nsIUploadChannelVtbl *lpUploadChannelVtbl;

    LONG ref;

    nsIChannel *channel;
    nsIHttpChannel *http_channel;
    nsIWineURI *uri;
    nsIInputStream *post_data_stream;
    nsILoadGroup *load_group;
    nsIInterfaceRequestor *notif_callback;
    nsLoadFlags load_flags;
    nsIURI *original_uri;
    char *content;
    char *charset;
} nsChannel;

typedef struct {
    const nsIInputStreamVtbl *lpInputStreamVtbl;

    LONG ref;

    char buf[1024];
    DWORD buf_size;
} nsProtocolStream;

struct BSCallback {
    const IBindStatusCallbackVtbl *lpBindStatusCallbackVtbl;
    const IServiceProviderVtbl    *lpServiceProviderVtbl;
    const IHttpNegotiate2Vtbl     *lpHttpNegotiate2Vtbl;
    const IInternetBindInfoVtbl   *lpInternetBindInfoVtbl;

    LONG ref;

    LPWSTR headers;
    HGLOBAL post_data;
    ULONG post_data_len;
    ULONG readed;

    nsChannel *nschannel;
    nsIStreamListener *nslistener;
    nsISupports *nscontext;

    IMoniker *mon;
    IBinding *binding;

    HTMLDocument *doc;

    nsProtocolStream *nsstream;
};

struct HTMLDOMNode {
    const IHTMLDOMNodeVtbl *lpHTMLDOMNodeVtbl;

    void (*destructor)(IUnknown*);

    enum {
        NT_UNKNOWN,
        NT_HTMLELEM
    } node_type;

    union {
        IUnknown *unk;
        IHTMLElement *elem;
    } impl;

    nsIDOMNode *nsnode;
    HTMLDocument *doc;

    HTMLDOMNode *next;
};

typedef struct {
    const IHTMLElementVtbl *lpHTMLElementVtbl;
    const IHTMLElement2Vtbl *lpHTMLElement2Vtbl;

    void (*destructor)(IUnknown*);

    nsIDOMHTMLElement *nselem;
    HTMLDOMNode *node;

    IUnknown *impl;
} HTMLElement;

typedef struct {
    const IHTMLTextContainerVtbl *lpHTMLTextContainerVtbl;

    HTMLElement *element;
} HTMLTextContainer;

#define HTMLWINDOW2(x)   ((IHTMLWindow2*)                 &(x)->lpHTMLWindow2Vtbl)

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
#define HLNKTARGET(x)    ((IHlinkTarget*)                 &(x)->lpHlinkTargetVtbl)
#define CONPTCONT(x)     ((IConnectionPointContainer*)    &(x)->lpConnectionPointContainerVtbl)
#define PERSTRINIT(x)    ((IPersistStreamInit*)           &(x)->lpPersistStreamInitVtbl)

#define NSWBCHROME(x)    ((nsIWebBrowserChrome*)          &(x)->lpWebBrowserChromeVtbl)
#define NSCML(x)         ((nsIContextMenuListener*)       &(x)->lpContextMenuListenerVtbl)
#define NSURICL(x)       ((nsIURIContentListener*)        &(x)->lpURIContentListenerVtbl)
#define NSEMBWNDS(x)     ((nsIEmbeddingSiteWindow*)       &(x)->lpEmbeddingSiteWindowVtbl)
#define NSIFACEREQ(x)    ((nsIInterfaceRequestor*)        &(x)->lpInterfaceRequestorVtbl)
#define NSTOOLTIP(x)     ((nsITooltipListener*)           &(x)->lpTooltipListenerVtbl)
#define NSEVENTLIST(x)   ((nsIDOMEventListener*)          &(x)->lpDOMEventListenerVtbl)
#define NSWEAKREF(x)     ((nsIWeakReference*)             &(x)->lpWeakReferenceVtbl)
#define NSSUPWEAKREF(x)  ((nsISupportsWeakReference*)     &(x)->lpSupportsWeakReferenceVtbl)

#define NSCHANNEL(x)     ((nsIChannel*)        &(x)->lpHttpChannelVtbl)
#define NSHTTPCHANNEL(x) ((nsIHttpChannel*)    &(x)->lpHttpChannelVtbl)
#define NSUPCHANNEL(x)   ((nsIUploadChannel*)  &(x)->lpUploadChannelVtbl)

#define HTTPNEG(x)       ((IHttpNegotiate2*)              &(x)->lpHttpNegotiate2Vtbl)
#define STATUSCLB(x)     ((IBindStatusCallback*)          &(x)->lpBindStatusCallbackVtbl)
#define BINDINFO(x)      ((IInternetBindInfo*)            &(x)->lpInternetBindInfoVtbl);

#define HTMLELEM(x)      ((IHTMLElement*)                 &(x)->lpHTMLElementVtbl)
#define HTMLELEM2(x)     ((IHTMLElement2*)                &(x)->lpHTMLElement2Vtbl)
#define HTMLDOMNODE(x)   ((IHTMLDOMNode*)                 &(x)->lpHTMLDOMNodeVtbl)

#define HTMLTEXTCONT(x)  ((IHTMLTextContainer*)           &(x)->lpHTMLTextContainerVtbl)

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);
HRESULT HTMLLoadOptions_Create(IUnknown*,REFIID,void**);

HTMLWindow *HTMLWindow_Create(HTMLDocument*);
HTMLWindow *nswindow_to_window(const nsIDOMWindow*);

void HTMLDocument_HTMLDocument3_Init(HTMLDocument*);
void HTMLDocument_Persist_Init(HTMLDocument*);
void HTMLDocument_OleCmd_Init(HTMLDocument*);
void HTMLDocument_OleObj_Init(HTMLDocument*);
void HTMLDocument_View_Init(HTMLDocument*);
void HTMLDocument_Window_Init(HTMLDocument*);
void HTMLDocument_Service_Init(HTMLDocument*);
void HTMLDocument_Hlink_Init(HTMLDocument*);
void HTMLDocument_ConnectionPoints_Init(HTMLDocument*);

void HTMLDocument_ConnectionPoints_Destroy(HTMLDocument*);

NSContainer *NSContainer_Create(HTMLDocument*,NSContainer*);
void NSContainer_Release(NSContainer*);

void HTMLDocument_LockContainer(HTMLDocument*,BOOL);
void show_context_menu(HTMLDocument*,DWORD,POINT*);

void show_tooltip(HTMLDocument*,DWORD,DWORD,LPCWSTR);
void hide_tooltip(HTMLDocument*);
HRESULT get_client_disp_property(IOleClientSite*,DISPID,VARIANT*);

HRESULT ProtocolFactory_Create(REFCLSID,REFIID,void**);

void close_gecko(void);
void register_nsservice(nsIComponentRegistrar*,nsIServiceManager*);
void init_nsio(nsIComponentManager*,nsIComponentRegistrar*);

void hlink_frame_navigate(HTMLDocument*,IHlinkFrame*,LPCWSTR,nsIInputStream*,DWORD);

void call_property_onchanged(ConnectionPoint*,DISPID);

void *nsalloc(size_t);
void nsfree(void*);

void nsACString_Init(nsACString*,const char*);
void nsACString_SetData(nsACString*,const char*);
PRUint32 nsACString_GetData(const nsACString*,const char**,PRBool*);
void nsACString_Finish(nsACString*);

void nsAString_Init(nsAString*,const PRUnichar*);
PRUint32 nsAString_GetData(const nsAString*,const PRUnichar**,PRBool*);
void nsAString_Finish(nsAString*);

nsIInputStream *create_nsstream(const char*,PRInt32);
nsICommandParams *create_nscommand_params(void);
void nsnode_to_nsstring(nsIDOMNode*,nsAString*);

BSCallback *create_bscallback(IMoniker*);
HRESULT start_binding(BSCallback*);
HRESULT load_stream(BSCallback*,IStream*);
void set_document_bscallback(HTMLDocument*,BSCallback*);
void set_current_mon(HTMLDocument*,IMoniker*);

IHlink *Hlink_Create(void);
IHTMLSelectionObject *HTMLSelectionObject_Create(nsISelection*);
IHTMLTxtRange *HTMLTxtRange_Create(nsIDOMRange*);
IHTMLStyle *HTMLStyle_Create(nsIDOMCSSStyleDeclaration*);
IHTMLStyleSheet *HTMLStyleSheet_Create(void);

void HTMLElement_Create(HTMLDOMNode*);
void HTMLBodyElement_Create(HTMLElement*);
void HTMLInputElement_Create(HTMLElement*);
void HTMLSelectElement_Create(HTMLElement*);
void HTMLTextAreaElement_Create(HTMLElement*);

void HTMLElement2_Init(HTMLElement*);

void HTMLTextContainer_Init(HTMLTextContainer*,HTMLElement*);

HRESULT HTMLDOMNode_QI(HTMLDOMNode*,REFIID,void**);
HRESULT HTMLElement_QI(HTMLElement*,REFIID,void**);

HTMLDOMNode *get_node(HTMLDocument*,nsIDOMNode*);
void release_nodes(HTMLDocument*);

BOOL install_wine_gecko(void);

/* commands */
typedef struct {
    DWORD id;
    HRESULT (*query)(HTMLDocument*,OLECMD*);
    HRESULT (*exec)(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
} cmdtable_t;

extern const cmdtable_t editmode_cmds[];

/* timer */
#define UPDATE_UI       0x0001
#define UPDATE_TITLE    0x0002

void update_doc(HTMLDocument *This, DWORD flags);
void update_title(HTMLDocument*);

/* editor */
void init_editor(HTMLDocument*);
void set_ns_editmode(NSContainer*);
void handle_edit_event(HTMLDocument*,nsIDOMEvent*);
HRESULT editor_exec_copy(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_cut(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_paste(HTMLDocument*,DWORD,VARIANT*,VARIANT*);

extern DWORD mshtml_tls;

typedef struct task_t {
    HTMLDocument *doc;

    enum {
        TASK_SETDOWNLOADSTATE,
        TASK_PARSECOMPLETE,
        TASK_SETPROGRESS,
        TASK_START_BINDING
    } task_id;

    BSCallback *bscallback;

    struct task_t *next;
} task_t;

typedef struct {
    HWND thread_hwnd;
    task_t *task_queue_head;
    task_t *task_queue_tail;
} thread_data_t;

thread_data_t *get_thread_data(BOOL);
HWND get_thread_hwnd(void);
void push_task(task_t*);
void remove_doc_tasks(const HTMLDocument*);

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);

DEFINE_GUID(CLSID_CMarkup,0x3050f4fb,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);

extern LONG module_ref;
#define LOCK_MODULE()   InterlockedIncrement(&module_ref)
#define UNLOCK_MODULE() InterlockedDecrement(&module_ref)

/* memory allocation functions */

static inline void *mshtml_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *mshtml_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *mshtml_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL mshtml_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

HINSTANCE get_shdoclc(void);

extern HINSTANCE hInst;
