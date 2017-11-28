/*
 * Copyright 2008-2009 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "htmlscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef enum {
    LISTENER_TYPE_CAPTURE,
    LISTENER_TYPE_BUBBLE,
    LISTENER_TYPE_ONEVENT,
    LISTENER_TYPE_ATTACHED
} listener_type_t;

typedef struct {
    struct list entry;
    listener_type_t type;
    IDispatch *function;
} event_listener_t;

typedef struct {
    struct wine_rb_entry entry;
    eventid_t event_id;
    struct list listeners;
} listener_container_t;

static const WCHAR abortW[] = {'a','b','o','r','t',0};
static const WCHAR beforeactivateW[] = {'b','e','f','o','r','e','a','c','t','i','v','a','t','e',0};
static const WCHAR beforeunloadW[] = {'b','e','f','o','r','e','u','n','l','o','a','d',0};
static const WCHAR blurW[] = {'b','l','u','r',0};
static const WCHAR changeW[] = {'c','h','a','n','g','e',0};
static const WCHAR clickW[] = {'c','l','i','c','k',0};
static const WCHAR contextmenuW[] = {'c','o','n','t','e','x','t','m','e','n','u',0};
static const WCHAR dataavailableW[] = {'d','a','t','a','a','v','a','i','l','a','b','l','e',0};
static const WCHAR dblclickW[] = {'d','b','l','c','l','i','c','k',0};
static const WCHAR dragW[] = {'d','r','a','g',0};
static const WCHAR dragstartW[] = {'d','r','a','g','s','t','a','r','t',0};
static const WCHAR errorW[] = {'e','r','r','o','r',0};
static const WCHAR focusW[] = {'f','o','c','u','s',0};
static const WCHAR focusinW[] = {'f','o','c','u','s','i','n',0};
static const WCHAR focusoutW[] = {'f','o','c','u','s','o','u','t',0};
static const WCHAR helpW[] = {'h','e','l','p',0};
static const WCHAR keydownW[] = {'k','e','y','d','o','w','n',0};
static const WCHAR keypressW[] = {'k','e','y','p','r','e','s','s',0};
static const WCHAR keyupW[] = {'k','e','y','u','p',0};
static const WCHAR loadW[] = {'l','o','a','d',0};
static const WCHAR messageW[] = {'m','e','s','s','a','g','e',0};
static const WCHAR mousedownW[] = {'m','o','u','s','e','d','o','w','n',0};
static const WCHAR mousemoveW[] = {'m','o','u','s','e','m','o','v','e',0};
static const WCHAR mouseoutW[] = {'m','o','u','s','e','o','u','t',0};
static const WCHAR mouseoverW[] = {'m','o','u','s','e','o','v','e','r',0};
static const WCHAR mouseupW[] = {'m','o','u','s','e','u','p',0};
static const WCHAR mousewheelW[] = {'m','o','u','s','e','w','h','e','e','l',0};
static const WCHAR pasteW[] = {'p','a','s','t','e',0};
static const WCHAR readystatechangeW[] = {'r','e','a','d','y','s','t','a','t','e','c','h','a','n','g','e',0};
static const WCHAR resizeW[] = {'r','e','s','i','z','e',0};
static const WCHAR scrollW[] = {'s','c','r','o','l','l',0};
static const WCHAR selectstartW[] = {'s','e','l','e','c','t','s','t','a','r','t',0};
static const WCHAR selectionchangeW[] = {'s','e','l','e','c','t','i','o','n','c','h','a','n','g','e',0};
static const WCHAR submitW[] = {'s','u','b','m','i','t',0};
static const WCHAR unloadW[] = {'u','n','l','o','a','d',0};
static const WCHAR DOMContentLoadedW[] = {'D','O','M','C','o','n','t','e','n','t','L','o','a','d','e','d',0};

static const WCHAR EventW[] = {'E','v','e','n','t',0};
static const WCHAR UIEventW[] = {'U','I','E','v','e','n','t',0};
static const WCHAR KeyboardEventW[] = {'K','e','y','b','o','a','r','d','E','v','e','n','t',0};
static const WCHAR MouseEventW[] = {'M','o','u','s','e','E','v','e','n','t',0};

typedef enum {
    EVENT_TYPE_EVENT,
    EVENT_TYPE_UIEVENT,
    EVENT_TYPE_KEYBOARD,
    EVENT_TYPE_MOUSE,
    EVENT_TYPE_FOCUS,
    EVENT_TYPE_DRAG,
    EVENT_TYPE_MESSAGE,
    EVENT_TYPE_CLIPBOARD
} event_type_t;

static const WCHAR *event_types[] = {
    EventW,
    UIEventW,
    KeyboardEventW,
    MouseEventW,
    EventW, /* FIXME */
    EventW, /* FIXME */
    EventW, /* FIXME */
    EventW  /* FIXME */
};

typedef struct {
    const WCHAR *name;
    event_type_t type;
    DISPID dispid;
    DWORD flags;
} event_info_t;

#define EVENT_DEFAULTLISTENER    0x0001
#define EVENT_BUBBLES            0x0002
#define EVENT_BIND_TO_BODY       0x0008
#define EVENT_CANCELABLE         0x0010
#define EVENT_HASDEFAULTHANDLERS 0x0020
#define EVENT_FIXME              0x0040

static const event_info_t event_info[] = {
    {abortW,             EVENT_TYPE_EVENT,     DISPID_EVMETH_ONABORT,
        EVENT_BIND_TO_BODY},
    {beforeactivateW,    EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREACTIVATE,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {beforeunloadW,      EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREUNLOAD,
        EVENT_DEFAULTLISTENER | EVENT_CANCELABLE },
    {blurW,              EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONBLUR,
        EVENT_DEFAULTLISTENER},
    {changeW,            EVENT_TYPE_EVENT,     DISPID_EVMETH_ONCHANGE,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {clickW,             EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONCLICK,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {contextmenuW,       EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONCONTEXTMENU,
        EVENT_BUBBLES | EVENT_CANCELABLE},
    {dataavailableW,     EVENT_TYPE_EVENT,     DISPID_EVMETH_ONDATAAVAILABLE,
        EVENT_FIXME | EVENT_BUBBLES},
    {dblclickW,          EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONDBLCLICK,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {DOMContentLoadedW,  EVENT_TYPE_EVENT,     0,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {dragW,              EVENT_TYPE_DRAG,      DISPID_EVMETH_ONDRAG,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {dragstartW,         EVENT_TYPE_DRAG,      DISPID_EVMETH_ONDRAGSTART,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {errorW,             EVENT_TYPE_EVENT,     DISPID_EVMETH_ONERROR,
        EVENT_BIND_TO_BODY},
    {focusW,             EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUS,
        EVENT_DEFAULTLISTENER},
    {focusinW,           EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUSIN,
        EVENT_BUBBLES},
    {focusoutW,          EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUSOUT,
        EVENT_BUBBLES},
    {helpW,              EVENT_TYPE_EVENT,     DISPID_EVMETH_ONHELP,
        EVENT_BUBBLES | EVENT_CANCELABLE},
    {keydownW,           EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYDOWN,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {keypressW,          EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYPRESS,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {keyupW,             EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYUP,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {loadW,              EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONLOAD,
        EVENT_BIND_TO_BODY},
    {messageW,           EVENT_TYPE_MESSAGE,   DISPID_EVMETH_ONMESSAGE,
        0},
    {mousedownW,         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEDOWN,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {mousemoveW,         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEMOVE,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {mouseoutW,          EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEOUT,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {mouseoverW,         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEOVER,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {mouseupW,           EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEUP,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {mousewheelW,        EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEWHEEL,
        EVENT_FIXME},
    {pasteW,             EVENT_TYPE_CLIPBOARD, DISPID_EVMETH_ONPASTE,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {readystatechangeW,  EVENT_TYPE_EVENT,     DISPID_EVMETH_ONREADYSTATECHANGE,
        0},
    {resizeW,            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONRESIZE,
        EVENT_DEFAULTLISTENER},
    {scrollW,            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONSCROLL,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES /* FIXME: not for elements */},
    {selectionchangeW,   EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSELECTIONCHANGE,
        EVENT_FIXME},
    {selectstartW,       EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSELECTSTART,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {submitW,            EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSUBMIT,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE},
    {unloadW,            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONUNLOAD,
        EVENT_FIXME}
};

static BOOL use_event_quirks(EventTarget*);

static eventid_t str_to_eid(const WCHAR *str)
{
    int i;

    for(i=0; i < sizeof(event_info)/sizeof(event_info[0]); i++) {
        if(!strcmpW(event_info[i].name, str))
            return i;
    }

    ERR("unknown type %s\n", debugstr_w(str));
    return EVENTID_LAST;
}

static eventid_t attr_to_eid(const WCHAR *str)
{
    int i;

    if((str[0] != 'o' && str[0] != 'O') || (str[1] != 'n' && str[1] != 'N'))
        return EVENTID_LAST;

    for(i=0; i < sizeof(event_info)/sizeof(event_info[0]); i++) {
        if(!strcmpW(event_info[i].name, str+2) && event_info[i].dispid)
            return i;
    }

    return EVENTID_LAST;
}

static listener_container_t *get_listener_container(EventTarget *event_target, eventid_t eid, BOOL alloc)
{
    const event_target_vtbl_t *vtbl;
    listener_container_t *container;
    struct wine_rb_entry *entry;

    entry = wine_rb_get(&event_target->handler_map, (const void*)eid);
    if(entry)
        return WINE_RB_ENTRY_VALUE(entry, listener_container_t, entry);
    if(!alloc)
        return NULL;

    if(event_info[eid].flags & EVENT_FIXME)
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    container = heap_alloc(sizeof(*container));
    if(!container)
        return NULL;

    container->event_id = eid;
    list_init(&container->listeners);
    vtbl = dispex_get_vtbl(&event_target->dispex);
    if(vtbl->bind_event)
        vtbl->bind_event(&event_target->dispex, eid);
    else
        FIXME("Unsupported event binding on target %p\n", event_target);

    wine_rb_put(&event_target->handler_map, (const void*)eid, &container->entry);
    return container;
}

static void remove_event_listener(EventTarget *event_target, eventid_t eid, listener_type_t type, IDispatch *function)
{
    listener_container_t *container;
    event_listener_t *listener;

    container = get_listener_container(event_target, eid, FALSE);
    if(!container)
        return;

    LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
        if(listener->function == function && listener->type == type) {
            IDispatch_Release(listener->function);
            list_remove(&listener->entry);
            heap_free(listener);
            break;
        }
    }
}

typedef struct {
    DispatchEx dispex;
    IHTMLEventObj IHTMLEventObj_iface;

    LONG ref;

    const event_info_t *type;
    DOMEvent *event;
    VARIANT return_value;
} HTMLEventObj;

static inline HTMLEventObj *impl_from_IHTMLEventObj(IHTMLEventObj *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj_iface);
}

static HRESULT WINAPI HTMLEventObj_QueryInterface(IHTMLEventObj *iface, REFIID riid, void **ppv)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLEventObj_iface;
    }else if(IsEqualGUID(&IID_IHTMLEventObj, riid)) {
        *ppv = &This->IHTMLEventObj_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLEventObj_AddRef(IHTMLEventObj *iface)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLEventObj_Release(IHTMLEventObj *iface)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->event)
            IDOMEvent_Release(&This->event->IDOMEvent_iface);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLEventObj_GetTypeInfoCount(IHTMLEventObj *iface, UINT *pctinfo)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLEventObj_GetTypeInfo(IHTMLEventObj *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLEventObj_GetIDsOfNames(IHTMLEventObj *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLEventObj_Invoke(IHTMLEventObj *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLEventObj_get_srcElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    if(This->event && This->event->target)
        IDispatchEx_QueryInterface(&This->event->target->dispex.IDispatchEx_iface, &IID_IHTMLElement, (void**)p);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_altKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetAltKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetAltKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_ctrlKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetCtrlKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetCtrlKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_shiftKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetShiftKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetShiftKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_returnValue(IHTMLEventObj *iface, VARIANT v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BOOL) {
        FIXME("unsupported value %s\n", debugstr_variant(&v));
        return DISP_E_BADVARTYPE;
    }

    This->return_value = v;
    if(!V_BOOL(&v) && This->event)
        IDOMEvent_preventDefault(&This->event->IDOMEvent_iface);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_returnValue(IHTMLEventObj *iface, VARIANT *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_EMPTY;
    return VariantCopy(p, &This->return_value);
}

static HRESULT WINAPI HTMLEventObj_put_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%x)\n", This, v);

    if(This->event)
        IDOMEvent_stopPropagation(&This->event->IDOMEvent_iface);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->event && This->event->stop_propagation);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_fromElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_toElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_keyCode(IHTMLEventObj *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_keyCode(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    UINT32 key_code = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetKeyCode(key_event, &key_code);
            nsIDOMKeyEvent_Release(key_event);
        }
    }

    *p = key_code;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_button(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    INT16 button = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetButton(mouse_event, &button);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = button;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_type(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->type) {
        *p = NULL;
        return S_OK;
    }

    *p = SysAllocString(This->type->name);
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLEventObj_get_qualifier(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_reason(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_x(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG x = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMUIEvent *ui_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event);
        if(NS_SUCCEEDED(nsres)) {
            /* NOTE: pageX is not exactly right here. */
            nsres = nsIDOMUIEvent_GetPageX(ui_event, &x);
            assert(nsres == NS_OK);
            nsIDOMUIEvent_Release(ui_event);
        }
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_y(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMUIEvent *ui_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event);
        if(NS_SUCCEEDED(nsres)) {
            /* NOTE: pageY is not exactly right here. */
            nsres = nsIDOMUIEvent_GetPageY(ui_event, &y);
            assert(nsres == NS_OK);
            nsIDOMUIEvent_Release(ui_event);
        }
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG x = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetClientX(mouse_event, &x);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetClientY(mouse_event, &y);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG x = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetScreenX(mouse_event, &x);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetScreenY(mouse_event, &y);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_srcFilter(IHTMLEventObj *iface, IDispatch **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static const IHTMLEventObjVtbl HTMLEventObjVtbl = {
    HTMLEventObj_QueryInterface,
    HTMLEventObj_AddRef,
    HTMLEventObj_Release,
    HTMLEventObj_GetTypeInfoCount,
    HTMLEventObj_GetTypeInfo,
    HTMLEventObj_GetIDsOfNames,
    HTMLEventObj_Invoke,
    HTMLEventObj_get_srcElement,
    HTMLEventObj_get_altKey,
    HTMLEventObj_get_ctrlKey,
    HTMLEventObj_get_shiftKey,
    HTMLEventObj_put_returnValue,
    HTMLEventObj_get_returnValue,
    HTMLEventObj_put_cancelBubble,
    HTMLEventObj_get_cancelBubble,
    HTMLEventObj_get_fromElement,
    HTMLEventObj_get_toElement,
    HTMLEventObj_put_keyCode,
    HTMLEventObj_get_keyCode,
    HTMLEventObj_get_button,
    HTMLEventObj_get_type,
    HTMLEventObj_get_qualifier,
    HTMLEventObj_get_reason,
    HTMLEventObj_get_x,
    HTMLEventObj_get_y,
    HTMLEventObj_get_clientX,
    HTMLEventObj_get_clientY,
    HTMLEventObj_get_offsetX,
    HTMLEventObj_get_offsetY,
    HTMLEventObj_get_screenX,
    HTMLEventObj_get_screenY,
    HTMLEventObj_get_srcFilter
};

static inline HTMLEventObj *unsafe_impl_from_IHTMLEventObj(IHTMLEventObj *iface)
{
    return iface->lpVtbl == &HTMLEventObjVtbl ? impl_from_IHTMLEventObj(iface) : NULL;
}

static const tid_t HTMLEventObj_iface_tids[] = {
    IHTMLEventObj_tid,
    0
};

static dispex_static_data_t HTMLEventObj_dispex = {
    NULL,
    DispCEventObj_tid,
    HTMLEventObj_iface_tids
};

static HTMLEventObj *alloc_event_obj(DOMEvent *event)
{
    HTMLEventObj *event_obj;

    event_obj = heap_alloc_zero(sizeof(*event_obj));
    if(!event_obj)
        return NULL;

    event_obj->IHTMLEventObj_iface.lpVtbl = &HTMLEventObjVtbl;
    event_obj->ref = 1;
    event_obj->event = event;
    if(event)
        IDOMEvent_AddRef(&event->IDOMEvent_iface);

    init_dispex(&event_obj->dispex, (IUnknown*)&event_obj->IHTMLEventObj_iface, &HTMLEventObj_dispex);
    return event_obj;
}

HRESULT create_event_obj(IHTMLEventObj **ret)
{
    HTMLEventObj *event_obj;

    event_obj = alloc_event_obj(NULL);
    if(!event_obj)
        return E_OUTOFMEMORY;

    *ret = &event_obj->IHTMLEventObj_iface;
    return S_OK;
}

static inline DOMEvent *impl_from_IDOMEvent(IDOMEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, IDOMEvent_iface);
}

static HRESULT WINAPI DOMEvent_QueryInterface(IDOMEvent *iface, REFIID riid, void **ppv)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = &This->IDOMEvent_iface;
    else if(IsEqualGUID(&IID_IDOMEvent, riid))
        *ppv = &This->IDOMEvent_iface;
    else if(dispex_query_interface(&This->dispex, riid, ppv))
        return *ppv ? S_OK : E_NOINTERFACE;
    else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DOMEvent_AddRef(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI DOMEvent_Release(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if(!ref) {
        if(This->target)
            IDispatchEx_Release(&This->target->dispex.IDispatchEx_iface);
        nsIDOMEvent_Release(This->nsevent);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI DOMEvent_GetTypeInfoCount(IDOMEvent *iface, UINT *pctinfo)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DOMEvent_GetTypeInfo(IDOMEvent *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DOMEvent_GetIDsOfNames(IDOMEvent *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI DOMEvent_Invoke(IDOMEvent *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DOMEvent_get_bubbles(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_cancelable(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_currentTarget(IDOMEvent *iface, IEventTarget **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_defaultPrevented(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->prevent_default);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_eventPhase(IDOMEvent *iface, USHORT *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->phase;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_target(IDOMEvent *iface, IEventTarget **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_timeStamp(IDOMEvent *iface, ULONGLONG *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_type(IDOMEvent *iface, BSTR *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_initEvent(IDOMEvent *iface, BSTR type, VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_preventDefault(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    This->prevent_default = TRUE;
    nsIDOMEvent_PreventDefault(This->nsevent);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_stopPropagation(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    This->stop_propagation = TRUE;
    nsIDOMEvent_StopPropagation(This->nsevent);
    IDOMEvent_preventDefault(&This->IDOMEvent_iface);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_stopImmediatePropagation(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_isTrusted(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_put_cancelBubble(IDOMEvent *iface, VARIANT_BOOL v)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_cancelBubble(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMEvent_get_srcElement(IDOMEvent *iface, IHTMLElement **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IDOMEventVtbl DOMEventVtbl = {
    DOMEvent_QueryInterface,
    DOMEvent_AddRef,
    DOMEvent_Release,
    DOMEvent_GetTypeInfoCount,
    DOMEvent_GetTypeInfo,
    DOMEvent_GetIDsOfNames,
    DOMEvent_Invoke,
    DOMEvent_get_bubbles,
    DOMEvent_get_cancelable,
    DOMEvent_get_currentTarget,
    DOMEvent_get_defaultPrevented,
    DOMEvent_get_eventPhase,
    DOMEvent_get_target,
    DOMEvent_get_timeStamp,
    DOMEvent_get_type,
    DOMEvent_initEvent,
    DOMEvent_preventDefault,
    DOMEvent_stopPropagation,
    DOMEvent_stopImmediatePropagation,
    DOMEvent_get_isTrusted,
    DOMEvent_put_cancelBubble,
    DOMEvent_get_cancelBubble,
    DOMEvent_get_srcElement
};

static const tid_t DOMEvent_iface_tids[] = {
    IDOMEvent_tid,
    0
};

static dispex_static_data_t DOMEvent_dispex = {
    NULL,
    IDOMEvent_tid,
    DOMEvent_iface_tids
};

static DOMEvent *alloc_event(nsIDOMEvent *nsevent)
{
    DOMEvent *event;

    event = heap_alloc_zero(sizeof(*event));
    if(!event)
        return NULL;

    init_dispex(&event->dispex, (IUnknown*)&event->IDOMEvent_iface, &DOMEvent_dispex);
    event->IDOMEvent_iface.lpVtbl = &DOMEventVtbl;
    event->ref = 1;
    nsIDOMEvent_AddRef(event->nsevent = nsevent);
    event->event_id = EVENTID_LAST;
    return event;
}

HRESULT create_event_from_nsevent(nsIDOMEvent *nsevent, DOMEvent **ret_event)
{
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;

    event = alloc_event(nsevent);
    if(!event)
        return E_OUTOFMEMORY;

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMEvent_GetType(event->nsevent, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const WCHAR *type;
        nsAString_GetData(&nsstr, &type);
        event->event_id = str_to_eid(type);
        if(event->event_id == EVENTID_LAST)
            FIXME("unknown event type %s\n", debugstr_w(type));
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }
    nsAString_Finish(&nsstr);

    *ret_event = event;
    return S_OK;
}

HRESULT create_document_event_str(HTMLDocumentNode *doc, const WCHAR *type, IDOMEvent **ret_event)
{
    nsIDOMEvent *nsevent;
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;

    nsAString_InitDepend(&nsstr, type);
    nsres = nsIDOMHTMLDocument_CreateEvent(doc->nsdoc, &nsstr, &nsevent);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        FIXME("CreateEvent(%s) failed: %08x\n", debugstr_w(type), nsres);
        return E_FAIL;
    }

    event = alloc_event(nsevent);
    nsIDOMEvent_Release(nsevent);
    if(!event)
        return E_OUTOFMEMORY;

    *ret_event = &event->IDOMEvent_iface;
    return S_OK;
}

HRESULT create_document_event(HTMLDocumentNode *doc, eventid_t event_id, DOMEvent **ret_event)
{
    nsIDOMEvent *nsevent;
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;

    nsAString_InitDepend(&nsstr, event_types[event_info[event_id].type]);
    nsres = nsIDOMHTMLDocument_CreateEvent(doc->nsdoc, &nsstr, &nsevent);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        FIXME("CreateEvent(%s) failed: %08x\n", debugstr_w(event_types[event_info[event_id].type]), nsres);
        return E_FAIL;
    }

    event = alloc_event(nsevent);
    if(!event)
        return E_OUTOFMEMORY;

    event->event_id = event_id;
    *ret_event = event;
    return S_OK;
}

static HRESULT call_disp_func(IDispatch *disp, DISPPARAMS *dp, VARIANT *retv)
{
    IDispatchEx *dispex;
    EXCEPINFO ei;
    HRESULT hres;

    memset(&ei, 0, sizeof(ei));

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, 0, GetUserDefaultLCID(), DISPATCH_METHOD, dp, retv, &ei, NULL);
        IDispatchEx_Release(dispex);
    }else {
        TRACE("Could not get IDispatchEx interface: %08x\n", hres);
        hres = IDispatch_Invoke(disp, 0, &IID_NULL, GetUserDefaultLCID(), DISPATCH_METHOD,
                dp, retv, &ei, NULL);
    }

    return hres;
}

static HRESULT call_cp_func(IDispatch *disp, DISPID dispid, IHTMLEventObj *event_obj, VARIANT *retv)
{
    DISPPARAMS dp = {NULL,NULL,0,0};
    VARIANT event_arg;
    ULONG argerr;
    EXCEPINFO ei;

    if(event_obj) {
        V_VT(&event_arg) = VT_DISPATCH;
        V_DISPATCH(&event_arg) = (IDispatch*)event_obj;
        dp.rgvarg = &event_arg;
        dp.cArgs = 1;
    }

    memset(&ei, 0, sizeof(ei));
    return IDispatch_Invoke(disp, dispid, &IID_NULL, 0, DISPATCH_METHOD, &dp, retv, &ei, &argerr);
}

static BOOL is_cp_event(cp_static_data_t *data, DISPID dispid)
{
    int min, max, i;
    HRESULT hres;

    if(!data)
        return FALSE;

    if(!data->ids) {
        hres = get_dispids(data->tid, &data->id_cnt, &data->ids);
        if(FAILED(hres))
            return FALSE;
    }

    min = 0;
    max = data->id_cnt-1;
    while(min <= max) {
        i = (min+max)/2;
        if(data->ids[i] == dispid)
            return TRUE;

        if(data->ids[i] < dispid)
            min = i+1;
        else
            max = i-1;
    }

    return FALSE;
}

static void call_event_handlers(EventTarget *event_target, DOMEvent *event)
{
    const eventid_t eid = event->event_id;
    const listener_container_t *container = get_listener_container(event_target, eid, FALSE);
    const BOOL cancelable = event_info[eid].flags & EVENT_CANCELABLE;
    const BOOL use_quirks = use_event_quirks(event_target);
    event_listener_t *listener, listeners_buf[8], *listeners = listeners_buf;
    unsigned listeners_cnt, listeners_size;
    ConnectionPointContainer *cp_container = NULL;
    const event_target_vtbl_t *vtbl = NULL;
    VARIANT v;
    HRESULT hres;

    if(use_quirks && container && !list_empty(&container->listeners)
       && event->phase != DEP_CAPTURING_PHASE) {
        listener = LIST_ENTRY(list_tail(&container->listeners), event_listener_t, entry);
        if(listener && listener->function && listener->type == LISTENER_TYPE_ONEVENT) {
            DISPID named_arg = DISPID_THIS;
            VARIANTARG arg;
            DISPPARAMS dp = {&arg, &named_arg, 1, 1};

            V_VT(&arg) = VT_DISPATCH;
            V_DISPATCH(&arg) = (IDispatch*)&event_target->dispex.IDispatchEx_iface;
            V_VT(&v) = VT_EMPTY;

            TRACE("%s >>>\n", debugstr_w(event_info[eid].name));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%s <<< %s\n", debugstr_w(event_info[eid].name), debugstr_variant(&v));

                if(cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%s <<< %08x\n", debugstr_w(event_info[eid].name), hres);
            }
        }
    }

    listeners_cnt = 0;
    listeners_size = sizeof(listeners_buf)/sizeof(*listeners_buf);

    if(container) {
        LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
            if(!listener->function)
                continue;
            switch(listener->type) {
            case LISTENER_TYPE_ONEVENT:
                if(use_quirks || event->phase == DEP_CAPTURING_PHASE)
                    continue;
                break;
            case LISTENER_TYPE_CAPTURE:
                if(event->phase == DEP_BUBBLING_PHASE || event->in_fire_event)
                    continue;
                break;
            case LISTENER_TYPE_BUBBLE:
                if(event->in_fire_event)
                    continue;
                /* fallthrough */
            case LISTENER_TYPE_ATTACHED:
                if(event->phase == DEP_CAPTURING_PHASE)
                    continue;
                break;
            }

            if(listeners_cnt == listeners_size) {
                event_listener_t *new_listeners;
                if(listeners == listeners_buf) {
                    new_listeners = heap_alloc(listeners_size * 2 * sizeof(*new_listeners));
                    if(!new_listeners)
                        break;
                    memcpy(new_listeners, listeners, listeners_cnt * sizeof(*listeners));
                }else {
                    new_listeners = heap_realloc(listeners, listeners_size * 2 * sizeof(*new_listeners));
                }
                listeners = new_listeners;
                listeners_size *= 2;
            }

            listeners[listeners_cnt].type = listener->type;
            IDispatch_AddRef(listeners[listeners_cnt].function = listener->function);
            listeners_cnt++;
        }
    }

    for(listener = listeners; listener < listeners + listeners_cnt; listener++) {
        if(listener->type != LISTENER_TYPE_ATTACHED) {
            DISPID named_arg = DISPID_THIS;
            VARIANTARG args[2];
            DISPPARAMS dp = {args, &named_arg, 2, 1};

            V_VT(args) = VT_DISPATCH;
            V_DISPATCH(args) = (IDispatch*)&event_target->dispex.IDispatchEx_iface;
            V_VT(args+1) = VT_DISPATCH;
            V_DISPATCH(args+1) = event->in_fire_event
                ? (IDispatch*)event->event_obj : (IDispatch*)&event->IDOMEvent_iface;
            V_VT(&v) = VT_EMPTY;

            TRACE("%s >>>\n", debugstr_w(event_info[event->event_id].name));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%s <<< %s\n", debugstr_w(event_info[event->event_id].name),
                      debugstr_variant(&v));

                if(cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%s <<< %08x\n", debugstr_w(event_info[event->event_id].name), hres);
            }
        }else {
            VARIANTARG arg;
            DISPPARAMS dp = {&arg, NULL, 1, 0};

            V_VT(&arg) = VT_DISPATCH;
            V_DISPATCH(&arg) = (IDispatch*)event->event_obj;
            V_VT(&v) = VT_EMPTY;

            TRACE("%s attached >>>\n", debugstr_w(event_info[eid].name));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%s attached <<<\n", debugstr_w(event_info[eid].name));

                if(cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%s attached <<< %08x\n", debugstr_w(event_info[eid].name), hres);
            }
        }
    }

    for(listener = listeners; listener < listeners + listeners_cnt; listener++)
        IDispatch_Release(listener->function);
    if(listeners != listeners_buf)
        heap_free(listeners);
    if(event->phase == DEP_CAPTURING_PHASE)
        return;

    if(event_info[eid].dispid && (vtbl = dispex_get_vtbl(&event_target->dispex))
       && vtbl->get_cp_container)
        cp_container = vtbl->get_cp_container(&event_target->dispex);
    if(cp_container) {
        if(cp_container->cps) {
            ConnectionPoint *cp;
            unsigned i, j;

            for(j=0; cp_container->cp_entries[j].riid; j++) {
                cp = cp_container->cps + j;
                if(!cp->sinks_size || !is_cp_event(cp->data, event_info[eid].dispid))
                    continue;

                for(i=0; i < cp->sinks_size; i++) {
                    if(!cp->sinks[i].disp)
                        continue;

                    V_VT(&v) = VT_EMPTY;

                    TRACE("cp %s [%u] >>>\n", debugstr_w(event_info[eid].name), i);
                    hres = call_cp_func(cp->sinks[i].disp, event_info[eid].dispid,
                            cp->data->pass_event_arg ? event->event_obj : NULL, &v);
                    if(hres == S_OK) {
                        TRACE("cp %s [%u] <<<\n", debugstr_w(event_info[eid].name), i);

                        if(cancelable) {
                            if(V_VT(&v) == VT_BOOL) {
                                if(!V_BOOL(&v))
                                    IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                            }else if(V_VT(&v) != VT_EMPTY) {
                                FIXME("unhandled result %s\n", debugstr_variant(&v));
                            }
                        }
                        VariantClear(&v);
                    }else {
                        WARN("cp %s [%u] <<< %08x\n", debugstr_w(event_info[eid].name), i, hres);
                    }
                }
            }
        }
        IConnectionPointContainer_Release(&cp_container->IConnectionPointContainer_iface);
    }
}

void dispatch_event(EventTarget *event_target, DOMEvent *event)
{
    EventTarget *target_chain_buf[8], **target_chain = target_chain_buf;
    unsigned chain_cnt, chain_buf_size, i;
    const event_target_vtbl_t *vtbl, *target_vtbl;
    HTMLEventObj *event_obj_ref = NULL;
    IHTMLEventObj *prev_event = NULL;
    EventTarget *iter;
    DWORD event_flags;
    HRESULT hres;

    if(event->event_id == EVENTID_LAST) {
        FIXME("Unsupported on unknown events\n");
        return;
    }

    TRACE("(%p) %s\n", event_target, debugstr_w(event_info[event->event_id].name));

    event_flags = event_info[event->event_id].flags;
    iter = event_target;
    IDispatchEx_AddRef(&event_target->dispex.IDispatchEx_iface);

    chain_cnt = 0;
    chain_buf_size = sizeof(target_chain_buf)/sizeof(*target_chain_buf);

    do {
        if(chain_cnt == chain_buf_size) {
            EventTarget **new_chain;
            if(target_chain == target_chain_buf) {
                new_chain = heap_alloc(chain_buf_size * 2 * sizeof(*new_chain));
                if(!new_chain)
                    break;
                memcpy(new_chain, target_chain, chain_buf_size * sizeof(*new_chain));
            }else {
                new_chain = heap_realloc(target_chain, chain_buf_size * 2 * sizeof(*new_chain));
                if(!new_chain)
                    break;
            }
            chain_buf_size *= 2;
            target_chain = new_chain;
        }

        target_chain[chain_cnt++] = iter;

        if(!(vtbl = dispex_get_vtbl(&iter->dispex)) || !vtbl->get_parent_event_target)
            break;
        iter = vtbl->get_parent_event_target(&iter->dispex);
    } while(iter);

    if(!event->event_obj && !event->no_event_obj) {
        event_obj_ref = alloc_event_obj(event);
        if(event_obj_ref) {
            event_obj_ref->type = event_info + event->event_id;
            event->event_obj = &event_obj_ref->IHTMLEventObj_iface;
        }
    }

    target_vtbl = dispex_get_vtbl(&event_target->dispex);
    if(target_vtbl && target_vtbl->set_current_event)
        prev_event = target_vtbl->set_current_event(&event_target->dispex, event->event_obj);

    event->target = event_target;
    IDispatchEx_AddRef(&event_target->dispex.IDispatchEx_iface);

    event->phase = DEP_CAPTURING_PHASE;
    i = chain_cnt-1;
    while(!event->stop_propagation && i)
        call_event_handlers(target_chain[i--], event);

    if(!event->stop_propagation) {
        event->phase = DEP_AT_TARGET;
        call_event_handlers(target_chain[0], event);
    }

    if(event_flags & EVENT_BUBBLES) {
        event->phase = DEP_BUBBLING_PHASE;
        for(i = 1; !event->stop_propagation && i < chain_cnt; i++)
            call_event_handlers(target_chain[i], event);
    }

    if(target_vtbl && target_vtbl->set_current_event) {
        prev_event = target_vtbl->set_current_event(&event_target->dispex, prev_event);
        if(prev_event)
            IHTMLEventObj_Release(prev_event);
    }

    if(event_flags & EVENT_HASDEFAULTHANDLERS) {
        for(i = 0; !event->prevent_default && i < chain_cnt; i++) {
            BOOL prevent_default = FALSE;
            vtbl = dispex_get_vtbl(&target_chain[i]->dispex);
            if(!vtbl || !vtbl->handle_event_default)
                continue;
            hres = vtbl->handle_event_default(&event_target->dispex, event->event_id,
                    event->nsevent, &prevent_default);
            if(FAILED(hres) || event->stop_propagation)
                break;
            if(prevent_default)
                IDOMEvent_preventDefault(&event->IDOMEvent_iface);
        }
    }

    if(event_obj_ref) {
        event->event_obj = NULL;
        IHTMLEventObj_Release(&event_obj_ref->IHTMLEventObj_iface);
    }

    for(i = 0; i < chain_cnt; i++)
        IDispatchEx_Release(&target_chain[i]->dispex.IDispatchEx_iface);
    if(target_chain != target_chain_buf)
        heap_free(target_chain);
}

HRESULT fire_event(HTMLDOMNode *node, const WCHAR *event_name, VARIANT *event_var, VARIANT_BOOL *cancelled)
{
    HTMLEventObj *event_obj = NULL;
    eventid_t eid;
    HRESULT hres = S_OK;

    eid = attr_to_eid(event_name);
    if(eid == EVENTID_LAST) {
        WARN("unknown event %s\n", debugstr_w(event_name));
        return E_INVALIDARG;
    }

    if(event_var && V_VT(event_var) != VT_EMPTY && V_VT(event_var) != VT_ERROR) {
        if(V_VT(event_var) != VT_DISPATCH) {
            FIXME("event_var %s not supported\n", debugstr_variant(event_var));
            return E_NOTIMPL;
        }

        if(V_DISPATCH(event_var)) {
            IHTMLEventObj *event_iface;

            hres = IDispatch_QueryInterface(V_DISPATCH(event_var), &IID_IHTMLEventObj, (void**)&event_iface);
            if(FAILED(hres)) {
                FIXME("No IHTMLEventObj iface\n");
                return hres;
            }

            event_obj = unsafe_impl_from_IHTMLEventObj(event_iface);
            if(!event_obj) {
                ERR("Not our IHTMLEventObj?\n");
                IHTMLEventObj_Release(event_iface);
                return E_FAIL;
            }
        }
    }

    if(!event_obj) {
        event_obj = alloc_event_obj(NULL);
        if(!event_obj)
            return E_OUTOFMEMORY;
    }

    event_obj->type = event_info + eid;
    if(!event_obj->event)
        hres = create_document_event(node->doc, eid, &event_obj->event);

    if(SUCCEEDED(hres)) {
        event_obj->event->event_obj = &event_obj->IHTMLEventObj_iface;
        event_obj->event->in_fire_event++;
        dispatch_event(&node->event_target, event_obj->event);
        event_obj->event->in_fire_event--;
        event_obj->event->event_obj = NULL;
    }

    IHTMLEventObj_Release(&event_obj->IHTMLEventObj_iface);
    if(FAILED(hres))
        return hres;

    *cancelled = VARIANT_TRUE; /* FIXME */
    return S_OK;
}

HRESULT ensure_doc_nsevent_handler(HTMLDocumentNode *doc, eventid_t eid)
{
    nsIDOMNode *nsnode = NULL;

    TRACE("%s\n", debugstr_w(event_info[eid].name));

    if(!doc->nsdoc)
        return S_OK;

    switch(eid) {
    case EVENTID_FOCUSIN:
        doc->event_vector[eid] = TRUE;
        eid = EVENTID_FOCUS;
        break;
    case EVENTID_FOCUSOUT:
        doc->event_vector[eid] = TRUE;
        eid = EVENTID_BLUR;
        break;
    default:
        break;
    }

    if(doc->event_vector[eid] || !(event_info[eid].flags & (EVENT_DEFAULTLISTENER|EVENT_BIND_TO_BODY)))
        return S_OK;

    if(event_info[eid].flags & EVENT_BIND_TO_BODY) {
        nsnode = doc->node.nsnode;
        nsIDOMNode_AddRef(nsnode);
    }

    doc->event_vector[eid] = TRUE;
    add_nsevent_listener(doc, nsnode, event_info[eid].name);

    if(nsnode)
        nsIDOMNode_Release(nsnode);
    return S_OK;
}

void detach_events(HTMLDocumentNode *doc)
{
    if(doc->event_vector) {
        int i;

        for(i=0; i < EVENTID_LAST; i++) {
            if(doc->event_vector[i]) {
                detach_nsevent(doc, event_info[i].name);
                doc->event_vector[i] = FALSE;
            }
        }
    }

    release_nsevents(doc);
}

static HRESULT get_event_dispex_ref(EventTarget *event_target, eventid_t eid, BOOL alloc, VARIANT **ret)
{
    WCHAR buf[64];
    buf[0] = 'o';
    buf[1] = 'n';
    strcpyW(buf+2, event_info[eid].name);
    return dispex_get_dprop_ref(&event_target->dispex, buf, alloc, ret);
}

static event_listener_t *get_onevent_listener(EventTarget *event_target, eventid_t eid, BOOL alloc)
{
    listener_container_t *container;
    event_listener_t *listener;

    container = get_listener_container(event_target, eid, alloc);
    if(!container)
        return NULL;

    LIST_FOR_EACH_ENTRY_REV(listener, &container->listeners, event_listener_t, entry) {
        if(listener->type == LISTENER_TYPE_ONEVENT)
            return listener;
    }

    if(!alloc)
        return NULL;

    listener = heap_alloc(sizeof(*listener));
    if(!listener)
        return NULL;

    listener->type = LISTENER_TYPE_ONEVENT;
    listener->function = NULL;
    list_add_tail(&container->listeners, &listener->entry);
    return listener;
}

static void remove_event_handler(EventTarget *event_target, eventid_t eid)
{
    event_listener_t *listener;
    VARIANT *store;
    HRESULT hres;

    hres = get_event_dispex_ref(event_target, eid, FALSE, &store);
    if(SUCCEEDED(hres))
        VariantClear(store);

    listener = get_onevent_listener(event_target, eid, FALSE);
    if(listener && listener->function) {
        IDispatch_Release(listener->function);
        listener->function = NULL;
    }
}

static HRESULT set_event_handler_disp(EventTarget *event_target, eventid_t eid, IDispatch *disp)
{
    event_listener_t *listener;

    if(event_info[eid].flags & EVENT_FIXME)
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    remove_event_handler(event_target, eid);
    if(!disp)
        return S_OK;

    listener = get_onevent_listener(event_target, eid, TRUE);
    if(!listener)
        return E_OUTOFMEMORY;

    if(listener->function)
        IDispatch_Release(listener->function);

    IDispatch_AddRef(listener->function = disp);
    return S_OK;
}

HRESULT set_event_handler(EventTarget *event_target, eventid_t eid, VARIANT *var)
{
    switch(V_VT(var)) {
    case VT_EMPTY:
        if(use_event_quirks(event_target)) {
            WARN("attempt to set to VT_EMPTY in quirks mode\n");
            return E_NOTIMPL;
        }
        /* fall through */
    case VT_NULL:
        remove_event_handler(event_target, eid);
        return S_OK;

    case VT_DISPATCH:
        return set_event_handler_disp(event_target, eid, V_DISPATCH(var));

    case VT_BSTR: {
        VARIANT *v;
        HRESULT hres;

        if(!use_event_quirks(event_target))
            FIXME("Setting to string %s not supported\n", debugstr_w(V_BSTR(var)));

        /*
         * Setting event handler to string is a rare case and we don't want to
         * complicate nor increase memory of listener_container_t for that. Instead,
         * we store the value in DispatchEx, which can already handle custom
         * properties.
         */
        remove_event_handler(event_target, eid);

        hres = get_event_dispex_ref(event_target, eid, TRUE, &v);
        if(FAILED(hres))
            return hres;

        V_BSTR(v) = SysAllocString(V_BSTR(var));
        if(!V_BSTR(v))
            return E_OUTOFMEMORY;
        V_VT(v) = VT_BSTR;
        return S_OK;
    }

    default:
        FIXME("not handler %s\n", debugstr_variant(var));
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT get_event_handler(EventTarget *event_target, eventid_t eid, VARIANT *var)
{
    event_listener_t *listener;
    VARIANT *v;
    HRESULT hres;

    hres = get_event_dispex_ref(event_target, eid, FALSE, &v);
    if(SUCCEEDED(hres) && V_VT(v) != VT_EMPTY) {
        V_VT(var) = VT_EMPTY;
        return VariantCopy(var, v);
    }

    listener = get_onevent_listener(event_target, eid, FALSE);
    if(listener && listener->function) {
        V_VT(var) = VT_DISPATCH;
        V_DISPATCH(var) = listener->function;
        IDispatch_AddRef(V_DISPATCH(var));
    }else {
        V_VT(var) = VT_NULL;
    }

    return S_OK;
}

HRESULT attach_event(EventTarget *event_target, BSTR name, IDispatch *disp, VARIANT_BOOL *res)
{
    listener_container_t *container;
    event_listener_t *listener;
    eventid_t eid;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        *res = VARIANT_TRUE;
        return S_OK;
    }

    container = get_listener_container(event_target, eid, TRUE);
    if(!container)
        return E_OUTOFMEMORY;

    listener = heap_alloc(sizeof(*listener));
    if(!listener)
        return E_OUTOFMEMORY;

    listener->type = LISTENER_TYPE_ATTACHED;
    IDispatch_AddRef(listener->function = disp);
    if(use_event_quirks(event_target))
        list_add_head(&container->listeners, &listener->entry);
    else
        list_add_tail(&container->listeners, &listener->entry);

    *res = VARIANT_TRUE;
    return S_OK;
}

HRESULT detach_event(EventTarget *event_target, BSTR name, IDispatch *disp)
{
    eventid_t eid;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        return S_OK;
    }

    remove_event_listener(event_target, eid, LISTENER_TYPE_ATTACHED, disp);
    return S_OK;
}

void bind_target_event(HTMLDocumentNode *doc, EventTarget *event_target, const WCHAR *event, IDispatch *disp)
{
    eventid_t eid;

    TRACE("(%p %p %s %p)\n", doc, event_target, debugstr_w(event), disp);

    eid = attr_to_eid(event);
    if(eid == EVENTID_LAST) {
        WARN("Unsupported event %s\n", debugstr_w(event));
        return;
    }

    set_event_handler_disp(event_target, eid, disp);
}

void update_doc_cp_events(HTMLDocumentNode *doc, cp_static_data_t *cp)
{
    int i;

    for(i=0; i < EVENTID_LAST; i++) {
        if((event_info[i].flags & EVENT_DEFAULTLISTENER) && is_cp_event(cp, event_info[i].dispid))
            ensure_doc_nsevent_handler(doc, i);
    }
}

void check_event_attr(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    nsIDOMMozNamedAttrMap *attr_map;
    const PRUnichar *name, *value;
    nsAString name_str, value_str;
    HTMLDOMNode *node = NULL;
    cpp_bool has_attrs;
    nsIDOMAttr *attr;
    IDispatch *disp;
    UINT32 length, i;
    eventid_t eid;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLElement_HasAttributes(nselem, &has_attrs);
    if(NS_FAILED(nsres) || !has_attrs)
        return;

    nsres = nsIDOMHTMLElement_GetAttributes(nselem, &attr_map);
    if(NS_FAILED(nsres))
        return;

    nsres = nsIDOMMozNamedAttrMap_GetLength(attr_map, &length);
    assert(nsres == NS_OK);

    nsAString_Init(&name_str, NULL);
    nsAString_Init(&value_str, NULL);

    for(i = 0; i < length; i++) {
        nsres = nsIDOMMozNamedAttrMap_Item(attr_map, i, &attr);
        if(NS_FAILED(nsres))
            continue;

        nsres = nsIDOMAttr_GetName(attr, &name_str);
        if(NS_FAILED(nsres)) {
            nsIDOMAttr_Release(attr);
            continue;
        }

        nsAString_GetData(&name_str, &name);
        eid = attr_to_eid(name);
        if(eid == EVENTID_LAST) {
            nsIDOMAttr_Release(attr);
            continue;
        }

        nsres = nsIDOMAttr_GetValue(attr, &value_str);
        nsIDOMAttr_Release(attr);
        if(NS_FAILED(nsres))
            continue;

        nsAString_GetData(&value_str, &value);
        if(!*value)
            continue;

        TRACE("%p.%s = %s\n", nselem, debugstr_w(name), debugstr_w(value));

        disp = script_parse_event(doc->window, value);
        if(!disp)
            continue;

        if(!node) {
            hres = get_node(doc, (nsIDOMNode*)nselem, TRUE, &node);
            if(FAILED(hres)) {
                IDispatch_Release(disp);
                break;
            }
        }

        set_event_handler_disp(get_node_event_prop_target(node, eid), eid, disp);
        IDispatch_Release(disp);
    }

    if(node)
        node_release(node);
    nsAString_Finish(&name_str);
    nsAString_Finish(&value_str);
    nsIDOMMozNamedAttrMap_Release(attr_map);
}

HRESULT doc_init_events(HTMLDocumentNode *doc)
{
    unsigned i;
    HRESULT hres;

    doc->event_vector = heap_alloc_zero(EVENTID_LAST*sizeof(BOOL));
    if(!doc->event_vector)
        return E_OUTOFMEMORY;

    init_nsevents(doc);

    for(i=0; i < EVENTID_LAST; i++) {
        if(event_info[i].flags & EVENT_HASDEFAULTHANDLERS) {
            hres = ensure_doc_nsevent_handler(doc, i);
            if(FAILED(hres))
                return hres;
        }
    }

    return S_OK;
}

static inline EventTarget *impl_from_IEventTarget(IEventTarget *iface)
{
    return CONTAINING_RECORD(iface, EventTarget, IEventTarget_iface);
}

static inline EventTarget *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, EventTarget, dispex);
}

static HRESULT WINAPI EventTarget_QueryInterface(IEventTarget *iface, REFIID riid, void **ppv)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_QueryInterface(&This->dispex.IDispatchEx_iface, riid, ppv);
}

static ULONG WINAPI EventTarget_AddRef(IEventTarget *iface)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_AddRef(&This->dispex.IDispatchEx_iface);
}

static ULONG WINAPI EventTarget_Release(IEventTarget *iface)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_Release(&This->dispex.IDispatchEx_iface);
}

static HRESULT WINAPI EventTarget_GetTypeInfoCount(IEventTarget *iface, UINT *pctinfo)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI EventTarget_GetTypeInfo(IEventTarget *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI EventTarget_GetIDsOfNames(IEventTarget *iface, REFIID riid, LPOLESTR *rgszNames,
                                                UINT cNames, LCID lcid, DISPID *rgDispId)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI EventTarget_Invoke(IEventTarget *iface, DISPID dispIdMember,
                                         REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                                         VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI EventTarget_addEventListener(IEventTarget *iface, BSTR type,
                                                   IDispatch *function, VARIANT_BOOL capture)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    listener_type_t listener_type = capture ? LISTENER_TYPE_CAPTURE : LISTENER_TYPE_BUBBLE;
    listener_container_t *container;
    event_listener_t *listener;
    eventid_t eid;

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(type), function, capture);

    eid = str_to_eid(type);
    if(eid == EVENTID_LAST) {
        FIXME("Unsupported on event %s\n", debugstr_w(type));
        return E_NOTIMPL;
    }


    container = get_listener_container(This, eid, TRUE);
    if(!container)
        return E_OUTOFMEMORY;

    /* check for duplicates */
    LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
        if(listener->type == listener_type && listener->function == function)
            return S_OK;
    }

    listener = heap_alloc(sizeof(*listener));
    if(!listener)
        return E_OUTOFMEMORY;

    listener->type = listener_type;
    IDispatch_AddRef(listener->function = function);
    list_add_tail(&container->listeners, &listener->entry);
    return S_OK;
}

static HRESULT WINAPI EventTarget_removeEventListener(IEventTarget *iface, BSTR type,
                                                      IDispatch *listener, VARIANT_BOOL capture)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    eventid_t eid;

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(type), listener, capture);

    eid = str_to_eid(type);
    if(eid == EVENTID_LAST) {
        FIXME("Unsupported on event %s\n", debugstr_w(type));
        return E_NOTIMPL;
    }

    remove_event_listener(This, eid, capture ? LISTENER_TYPE_CAPTURE : LISTENER_TYPE_BUBBLE, listener);
    return S_OK;
}

static HRESULT WINAPI EventTarget_dispatchEvent(IEventTarget *iface, IDOMEvent *event, VARIANT_BOOL *result)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    FIXME("(%p)->(%p %p)\n", This, event, result);
    return E_NOTIMPL;
}

HRESULT IEventTarget_addEventListener_hook(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    /* If only two arguments were given, implicitly set capture to false */
    if((flags & DISPATCH_METHOD) && dp->cArgs == 2 && !dp->cNamedArgs) {
        VARIANT args[3];
        DISPPARAMS new_dp = {args, NULL, 3, 0};
        V_VT(args) = VT_BOOL;
        V_BOOL(args) = VARIANT_FALSE;
        args[1] = dp->rgvarg[0];
        args[2] = dp->rgvarg[1];

        TRACE("implicit capture\n");

        return IDispatchEx_InvokeEx(&dispex->IDispatchEx_iface, DISPID_IEVENTTARGET_ADDEVENTLISTENER,
                                    lcid, flags, &new_dp, res, ei, caller);
    }

    return S_FALSE; /* fallback to default */
}

HRESULT IEventTarget_removeEventListener_hook(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    /* If only two arguments were given, implicitly set capture to false */
    if((flags & DISPATCH_METHOD) && dp->cArgs == 2 && !dp->cNamedArgs) {
        VARIANT args[3];
        DISPPARAMS new_dp = {args, NULL, 3, 0};
        V_VT(args) = VT_BOOL;
        V_BOOL(args) = VARIANT_FALSE;
        args[1] = dp->rgvarg[0];
        args[2] = dp->rgvarg[1];

        TRACE("implicit capture\n");

        return IDispatchEx_InvokeEx(&dispex->IDispatchEx_iface, DISPID_IEVENTTARGET_REMOVEEVENTLISTENER,
                                    lcid, flags, &new_dp, res, ei, caller);
    }

    return S_FALSE; /* fallback to default */
}

static const IEventTargetVtbl EventTargetVtbl = {
    EventTarget_QueryInterface,
    EventTarget_AddRef,
    EventTarget_Release,
    EventTarget_GetTypeInfoCount,
    EventTarget_GetTypeInfo,
    EventTarget_GetIDsOfNames,
    EventTarget_Invoke,
    EventTarget_addEventListener,
    EventTarget_removeEventListener,
    EventTarget_dispatchEvent
};

#define DELAY_INIT_VTBL ((const IEventTargetVtbl*)1)

static BOOL use_event_quirks(EventTarget *event_target)
{
    if(event_target->IEventTarget_iface.lpVtbl == DELAY_INIT_VTBL) {
        event_target->IEventTarget_iface.lpVtbl =
            dispex_compat_mode(&event_target->dispex) >= COMPAT_MODE_IE9
            ? &EventTargetVtbl : NULL;
    }
    return !event_target->IEventTarget_iface.lpVtbl;
}

HRESULT EventTarget_QI(EventTarget *event_target, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IEventTarget)) {
        if(use_event_quirks(event_target)) {
            WARN("IEventTarget queried, but not supported by in document mode\n");
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        IEventTarget_AddRef(&event_target->IEventTarget_iface);
        *ppv = &event_target->IEventTarget_iface;
        return S_OK;
    }

    if(dispex_query_interface(&event_target->dispex, riid, ppv))
        return *ppv ? S_OK : E_NOINTERFACE;

    WARN("(%p)->(%s %p)\n", event_target, debugstr_mshtml_guid(riid), ppv);
    *ppv = NULL;
    return E_NOINTERFACE;
}

void EventTarget_init_dispex_info(dispex_data_t *dispex_info, compat_mode_t compat_mode)
{
    static const dispex_hook_t IEventTarget_hooks[] = {
        {DISPID_IEVENTTARGET_ADDEVENTLISTENER, IEventTarget_addEventListener_hook},
        {DISPID_IEVENTTARGET_REMOVEEVENTLISTENER, IEventTarget_removeEventListener_hook},
        {DISPID_UNKNOWN}
    };

    if(compat_mode >= COMPAT_MODE_IE9)
        dispex_info_add_interface(dispex_info, IEventTarget_tid, IEventTarget_hooks);
}

static int event_id_cmp(const void *key, const struct wine_rb_entry *entry)
{
    return (INT_PTR)key - WINE_RB_ENTRY_VALUE(entry, listener_container_t, entry)->event_id;
}

void EventTarget_Init(EventTarget *event_target, IUnknown *outer, dispex_static_data_t *dispex_data,
                      compat_mode_t compat_mode)
{
    init_dispex_with_compat_mode(&event_target->dispex, outer, dispex_data, compat_mode);
    wine_rb_init(&event_target->handler_map, event_id_cmp);

    /*
     * IEventTarget is supported by the object or not depending on compatibility mode.
     * We use NULL vtbl for objects in compatibility mode not supporting the interface.
     * For targets that don't know compatibility mode at creation time, we set vtbl
     * to special DELAY_INIT_VTBL value so that vtbl will be set to proper value
     * when it's needed.
     */
    if(compat_mode == COMPAT_MODE_QUIRKS && dispex_data->vtbl && dispex_data->vtbl->get_compat_mode)
        event_target->IEventTarget_iface.lpVtbl = DELAY_INIT_VTBL;
    else if(compat_mode < COMPAT_MODE_IE9)
        event_target->IEventTarget_iface.lpVtbl = NULL;
    else
        event_target->IEventTarget_iface.lpVtbl = &EventTargetVtbl;
}

void release_event_target(EventTarget *event_target)
{
    listener_container_t *iter, *iter2;

    WINE_RB_FOR_EACH_ENTRY_DESTRUCTOR(iter, iter2, &event_target->handler_map, listener_container_t, entry) {
        while(!list_empty(&iter->listeners)) {
            event_listener_t *listener = LIST_ENTRY(list_head(&iter->listeners), event_listener_t, entry);
            if(listener->function)
                IDispatch_Release(listener->function);
            list_remove(&listener->entry);
            heap_free(listener);
        }
        heap_free(iter);
    }
}
