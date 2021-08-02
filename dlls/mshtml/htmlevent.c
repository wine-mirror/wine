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
    struct list listeners;
    WCHAR type[1];
} listener_container_t;

typedef enum {
    DISPATCH_BOTH,
    DISPATCH_STANDARD,
    DISPATCH_LEGACY
} dispatch_mode_t;

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
    L"Event",
    L"UIEvent",
    L"KeyboardEvent",
    L"MouseEvent",
    L"Event", /* FIXME */
    L"Event", /* FIXME */
    L"Event", /* FIXME */
    L"Event"  /* FIXME */
};

typedef struct {
    const WCHAR *name;
    event_type_t type;
    DISPID dispid;
    DWORD flags;
} event_info_t;

/* Use Gecko default listener (it's registered on window object for DOM nodes). */
#define EVENT_DEFAULTLISTENER    0x0001
/* Register Gecko listener on target itself (unlike EVENT_DEFAULTLISTENER). */
#define EVENT_BIND_TO_TARGET     0x0002
/* Event bubbles by default (unless explicitly specified otherwise). */
#define EVENT_BUBBLES            0x0004
/* Event is cancelable by default (unless explicitly specified otherwise). */
#define EVENT_CANCELABLE         0x0008
/* Event may have default handler (so we always have to register Gecko listener). */
#define EVENT_HASDEFAULTHANDLERS 0x0020
/* Ecent is not supported properly, print FIXME message when it's used. */
#define EVENT_FIXME              0x0040

/* mouse event flags for fromElement and toElement implementation */
#define EVENT_MOUSE_TO_RELATED   0x0100
#define EVENT_MOUSE_FROM_RELATED 0x0200

static const event_info_t event_info[] = {
    {L"abort",             EVENT_TYPE_EVENT,     DISPID_EVMETH_ONABORT,
        EVENT_BIND_TO_TARGET},
    {L"animationend",      EVENT_TYPE_EVENT,     DISPID_EVPROP_ONANIMATIONEND,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"animationstart",    EVENT_TYPE_EVENT,     DISPID_EVPROP_ONANIMATIONSTART,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"beforeactivate",    EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREACTIVATE,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"beforeunload",      EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREUNLOAD,
        EVENT_DEFAULTLISTENER | EVENT_CANCELABLE },
    {L"blur",              EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONBLUR,
        EVENT_DEFAULTLISTENER},
    {L"change",            EVENT_TYPE_EVENT,     DISPID_EVMETH_ONCHANGE,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"click",             EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONCLICK,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {L"contextmenu",       EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONCONTEXTMENU,
        EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"dataavailable",     EVENT_TYPE_EVENT,     DISPID_EVMETH_ONDATAAVAILABLE,
        EVENT_FIXME | EVENT_BUBBLES},
    {L"dblclick",          EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONDBLCLICK,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"DOMContentLoaded",  EVENT_TYPE_EVENT,     0,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"drag",              EVENT_TYPE_DRAG,      DISPID_EVMETH_ONDRAG,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"dragstart",         EVENT_TYPE_DRAG,      DISPID_EVMETH_ONDRAGSTART,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"error",             EVENT_TYPE_EVENT,     DISPID_EVMETH_ONERROR,
        EVENT_BIND_TO_TARGET},
    {L"focus",             EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUS,
        EVENT_DEFAULTLISTENER},
    {L"focusin",           EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUSIN,
        EVENT_BUBBLES},
    {L"focusout",          EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUSOUT,
        EVENT_BUBBLES},
    {L"help",              EVENT_TYPE_EVENT,     DISPID_EVMETH_ONHELP,
        EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"input",             EVENT_TYPE_EVENT,     DISPID_UNKNOWN,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"keydown",           EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYDOWN,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {L"keypress",          EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYPRESS,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"keyup",             EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYUP,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"load",              EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONLOAD,
        EVENT_BIND_TO_TARGET},
    {L"message",           EVENT_TYPE_MESSAGE,   DISPID_EVMETH_ONMESSAGE,
        0},
    {L"mousedown",         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEDOWN,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"mousemove",         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEMOVE,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE | EVENT_MOUSE_FROM_RELATED},
    {L"mouseout",          EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEOUT,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE | EVENT_MOUSE_TO_RELATED},
    {L"mouseover",         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEOVER,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE | EVENT_MOUSE_FROM_RELATED},
    {L"mouseup",           EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEUP,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"mousewheel",        EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEWHEEL,
        EVENT_FIXME},
    {L"msthumbnailclick",  EVENT_TYPE_MOUSE,     DISPID_EVPROP_ONMSTHUMBNAILCLICK,
        EVENT_FIXME},
    {L"paste",             EVENT_TYPE_CLIPBOARD, DISPID_EVMETH_ONPASTE,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"readystatechange",  EVENT_TYPE_EVENT,     DISPID_EVMETH_ONREADYSTATECHANGE,
        0},
    {L"resize",            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONRESIZE,
        EVENT_DEFAULTLISTENER},
    {L"scroll",            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONSCROLL,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES /* FIXME: not for elements */},
    {L"selectionchange",   EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSELECTIONCHANGE,
        EVENT_FIXME},
    {L"selectstart",       EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSELECTSTART,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"submit",            EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSUBMIT,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"unload",            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONUNLOAD,
        EVENT_FIXME}
};

C_ASSERT(ARRAY_SIZE(event_info) == EVENTID_LAST);

static eventid_t str_to_eid(const WCHAR *str)
{
    int i;

    for(i=0; i < ARRAY_SIZE(event_info); i++) {
        if(!wcscmp(event_info[i].name, str))
            return i;
    }

    return EVENTID_LAST;
}

static eventid_t attr_to_eid(const WCHAR *str)
{
    int i;

    if((str[0] != 'o' && str[0] != 'O') || (str[1] != 'n' && str[1] != 'N'))
        return EVENTID_LAST;

    for(i=0; i < ARRAY_SIZE(event_info); i++) {
        if(!wcscmp(event_info[i].name, str+2) && event_info[i].dispid)
            return i;
    }

    return EVENTID_LAST;
}

static listener_container_t *get_listener_container(EventTarget *event_target, const WCHAR *type, BOOL alloc)
{
    const event_target_vtbl_t *vtbl;
    listener_container_t *container;
    struct wine_rb_entry *entry;
    size_t type_len;
    eventid_t eid;

    entry = wine_rb_get(&event_target->handler_map, type);
    if(entry)
        return WINE_RB_ENTRY_VALUE(entry, listener_container_t, entry);
    if(!alloc)
        return NULL;

    eid = str_to_eid(type);
    if(eid != EVENTID_LAST && (event_info[eid].flags & EVENT_FIXME))
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    type_len = lstrlenW(type);
    container = heap_alloc(FIELD_OFFSET(listener_container_t, type[type_len+1]));
    if(!container)
        return NULL;
    memcpy(container->type, type, (type_len + 1) * sizeof(WCHAR));
    list_init(&container->listeners);
    vtbl = dispex_get_vtbl(&event_target->dispex);
    if(vtbl->bind_event)
        vtbl->bind_event(&event_target->dispex, eid);
    else
        FIXME("Unsupported event binding on target %p\n", event_target);

    wine_rb_put(&event_target->handler_map, container->type, &container->entry);
    return container;
}

static void remove_event_listener(EventTarget *event_target, const WCHAR *type_name, listener_type_t type, IDispatch *function)
{
    listener_container_t *container;
    event_listener_t *listener;

    container = get_listener_container(event_target, type_name, FALSE);
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

static HRESULT get_gecko_target(IEventTarget*,nsIDOMEventTarget**);

typedef struct {
    DispatchEx dispex;
    IHTMLEventObj IHTMLEventObj_iface;

    LONG ref;

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

    if(!This->event) {
        *p = NULL;
        return S_OK;
    }

    return IDOMEvent_get_srcElement(&This->event->IDOMEvent_iface, p);
}

static HRESULT WINAPI HTMLEventObj_get_altKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_altKey(&This->event->IDOMMouseEvent_iface, p);

    if(This->event && This->event->keyboard_event)
        return IDOMKeyboardEvent_get_altKey(&This->event->IDOMKeyboardEvent_iface, p);

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_ctrlKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_ctrlKey(&This->event->IDOMMouseEvent_iface, p);

    if(This->event && This->event->keyboard_event)
        return IDOMKeyboardEvent_get_ctrlKey(&This->event->IDOMKeyboardEvent_iface, p);

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_shiftKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_shiftKey(&This->event->IDOMMouseEvent_iface, p);

    if(This->event && This->event->keyboard_event)
        return IDOMKeyboardEvent_get_shiftKey(&This->event->IDOMKeyboardEvent_iface, p);

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

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_fromElement(&This->event->IDOMMouseEvent_iface, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_toElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_toElement(&This->event->IDOMMouseEvent_iface, p);

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

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->keyboard_event)
        return IDOMKeyboardEvent_get_keyCode(&This->event->IDOMKeyboardEvent_iface, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_button(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    USHORT button = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event) {
        HRESULT hres;
        hres = IDOMMouseEvent_get_button(&This->event->IDOMMouseEvent_iface, &button);
        if(FAILED(hres))
            return hres;
    }

    *p = button;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_type(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event) {
        *p = NULL;
        return S_OK;
    }

    return IDOMEvent_get_type(&This->event->IDOMEvent_iface, p);
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

    if(This->event && This->event->ui_event) {
        nsresult nsres;

        /* NOTE: pageX is not exactly right here. */
        nsres = nsIDOMUIEvent_GetPageX(This->event->ui_event, &x);
        assert(nsres == NS_OK);
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_y(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->ui_event) {
        nsresult nsres;

        /* NOTE: pageY is not exactly right here. */
        nsres = nsIDOMUIEvent_GetPageY(This->event->ui_event, &y);
        assert(nsres == NS_OK);
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_clientX(&This->event->IDOMMouseEvent_iface, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_clientY(&This->event->IDOMMouseEvent_iface, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_offsetX(&This->event->IDOMMouseEvent_iface, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_offsetY(&This->event->IDOMMouseEvent_iface, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_screenX(&This->event->IDOMMouseEvent_iface, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && This->event->mouse_event)
        return IDOMMouseEvent_get_screenY(&This->event->IDOMMouseEvent_iface, p);

    *p = 0;
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

static HTMLEventObj *alloc_event_obj(DOMEvent *event, compat_mode_t compat_mode)
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

    init_dispatch(&event_obj->dispex, (IUnknown*)&event_obj->IHTMLEventObj_iface, &HTMLEventObj_dispex, compat_mode);
    return event_obj;
}

HRESULT create_event_obj(compat_mode_t compat_mode, IHTMLEventObj **ret)
{
    HTMLEventObj *event_obj;

    event_obj = alloc_event_obj(NULL, compat_mode);
    if(!event_obj)
        return E_OUTOFMEMORY;

    *ret = &event_obj->IHTMLEventObj_iface;
    return S_OK;
}

static inline DOMEvent *impl_from_IDOMEvent(IDOMEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, IDOMEvent_iface);
}

static const IDOMEventVtbl DOMEventVtbl;

static inline DOMEvent *unsafe_impl_from_IDOMEvent(IDOMEvent *iface)
{
    return iface && iface->lpVtbl == &DOMEventVtbl ? impl_from_IDOMEvent(iface) : NULL;
}

static HRESULT WINAPI DOMEvent_QueryInterface(IDOMEvent *iface, REFIID riid, void **ppv)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = &This->IDOMEvent_iface;
    else if(IsEqualGUID(&IID_IDOMEvent, riid))
        *ppv = &This->IDOMEvent_iface;
    else if(This->ui_event && IsEqualGUID(&IID_IDOMUIEvent, riid))
        *ppv = &This->IDOMUIEvent_iface;
    else if(This->mouse_event && IsEqualGUID(&IID_IDOMMouseEvent, riid))
        *ppv = &This->IDOMMouseEvent_iface;
    else if(This->keyboard_event && IsEqualGUID(&IID_IDOMKeyboardEvent, riid))
        *ppv = &This->IDOMKeyboardEvent_iface;
    else if(dispex_query_interface(&This->dispex, riid, ppv))
        return *ppv ? S_OK : E_NOINTERFACE;
    else if(!This->query_interface || !(*ppv = This->query_interface(This, riid))) {
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
        if(This->destroy)
            This->destroy(This);
        if(This->ui_event)
            nsIDOMUIEvent_Release(This->ui_event);
        if(This->mouse_event)
            nsIDOMMouseEvent_Release(This->mouse_event);
        if(This->keyboard_event)
            nsIDOMKeyEvent_Release(This->keyboard_event);
        if(This->target)
            IEventTarget_Release(&This->target->IEventTarget_iface);
        nsIDOMEvent_Release(This->nsevent);
        release_dispex(&This->dispex);
        heap_free(This->type);
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

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->bubbles);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_cancelable(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->cancelable);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_currentTarget(IDOMEvent *iface, IEventTarget **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->current_target)
        IEventTarget_AddRef(*p = &This->current_target->IEventTarget_iface);
    else
        *p = NULL;
    return S_OK;
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

    TRACE("(%p)->(%p)\n", This, p);

    if(This->target)
        IEventTarget_AddRef(*p = &This->target->IEventTarget_iface);
    else
        *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_timeStamp(IDOMEvent *iface, ULONGLONG *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->time_stamp;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_type(IDOMEvent *iface, BSTR *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->type) {
        *p = SysAllocString(This->type);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }
    return S_OK;
}

#ifdef __i386__
#define nsIDOMEvent_InitEvent(_this,type,bubbles,cancelable) \
    ((void (WINAPI*)(void*,nsIDOMEvent*,const nsAString*,cpp_bool,cpp_bool)) \
     &call_thiscall_func)((_this)->lpVtbl->InitEvent,_this,type,bubbles,cancelable)

#endif

static HRESULT WINAPI DOMEvent_initEvent(IDOMEvent *iface, BSTR type, VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    nsAString nsstr;

    TRACE("(%p)->(%s %x %x)\n", This, debugstr_w(type), can_bubble, cancelable);

    if(This->target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    heap_free(This->type);
    This->type = heap_strdupW(type);
    if(!This->type)
        return E_OUTOFMEMORY;
    This->event_id = str_to_eid(type);

    This->bubbles = !!can_bubble;
    This->cancelable = !!cancelable;

    nsAString_InitDepend(&nsstr, type);
    nsIDOMEvent_InitEvent(This->nsevent, &nsstr, This->bubbles, This->cancelable);
    nsAString_Finish(&nsstr);

    return S_OK;
}

static HRESULT WINAPI DOMEvent_preventDefault(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    if(This->current_target && This->cancelable) {
        This->prevent_default = TRUE;
        nsIDOMEvent_PreventDefault(This->nsevent);
    }
    return S_OK;
}

static HRESULT WINAPI DOMEvent_stopPropagation(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    This->stop_propagation = TRUE;
    nsIDOMEvent_StopPropagation(This->nsevent);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_stopImmediatePropagation(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    This->stop_immediate_propagation = This->stop_propagation = TRUE;
    nsIDOMEvent_StopImmediatePropagation(This->nsevent);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_isTrusted(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->trusted);
    return S_OK;
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

    TRACE("(%p)->(%p)\n", This, p);

    if(This->target)
        IDispatchEx_QueryInterface(&This->target->dispex.IDispatchEx_iface, &IID_IHTMLElement, (void**)p);
    else
        *p = NULL;
    return S_OK;
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

static inline DOMEvent *impl_from_IDOMUIEvent(IDOMUIEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, IDOMUIEvent_iface);
}

static HRESULT WINAPI DOMUIEvent_QueryInterface(IDOMUIEvent *iface, REFIID riid, void **ppv)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDOMEvent_QueryInterface(&This->IDOMEvent_iface, riid, ppv);
}

static ULONG WINAPI DOMUIEvent_AddRef(IDOMUIEvent *iface)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDOMEvent_AddRef(&This->IDOMEvent_iface);
}

static ULONG WINAPI DOMUIEvent_Release(IDOMUIEvent *iface)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDOMEvent_Release(&This->IDOMEvent_iface);
}

static HRESULT WINAPI DOMUIEvent_GetTypeInfoCount(IDOMUIEvent *iface, UINT *pctinfo)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DOMUIEvent_GetTypeInfo(IDOMUIEvent *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DOMUIEvent_GetIDsOfNames(IDOMUIEvent *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI DOMUIEvent_Invoke(IDOMUIEvent *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DOMUIEvent_get_view(IDOMUIEvent *iface, IHTMLWindow2 **p)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    mozIDOMWindowProxy *moz_window;
    HTMLOuterWindow *view = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMUIEvent_GetView(This->ui_event, &moz_window);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(moz_window) {
        view = mozwindow_to_window(moz_window);
        mozIDOMWindowProxy_Release(moz_window);
    }
    if(view)
        IHTMLWindow2_AddRef((*p = &view->base.inner_window->base.IHTMLWindow2_iface));
    else
        *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMUIEvent_get_detail(IDOMUIEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    INT32 detail;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMUIEvent_GetDetail(This->ui_event, &detail);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = detail;
    return S_OK;
}

static HRESULT WINAPI DOMUIEvent_initUIEvent(IDOMUIEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
        VARIANT_BOOL cancelable, IHTMLWindow2 *view, LONG detail)
{
    DOMEvent *This = impl_from_IDOMUIEvent(iface);
    nsAString type_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %p %x)\n", This, debugstr_w(type), can_bubble, cancelable, view, detail);

    if(This->target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    if(view)
        FIXME("view argument is not supported\n");

    hres = IDOMEvent_initEvent(&This->IDOMEvent_iface, type, can_bubble, cancelable);
    if(FAILED(hres))
        return hres;

    nsAString_InitDepend(&type_str, type);
    nsres = nsIDOMUIEvent_InitUIEvent(This->ui_event, &type_str, !!can_bubble, !!cancelable,
                                      NULL /* FIXME */, detail);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres)) {
        FIXME("InitUIEvent failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static const IDOMUIEventVtbl DOMUIEventVtbl = {
    DOMUIEvent_QueryInterface,
    DOMUIEvent_AddRef,
    DOMUIEvent_Release,
    DOMUIEvent_GetTypeInfoCount,
    DOMUIEvent_GetTypeInfo,
    DOMUIEvent_GetIDsOfNames,
    DOMUIEvent_Invoke,
    DOMUIEvent_get_view,
    DOMUIEvent_get_detail,
    DOMUIEvent_initUIEvent
};

static inline DOMEvent *impl_from_IDOMMouseEvent(IDOMMouseEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, IDOMMouseEvent_iface);
}

static HRESULT WINAPI DOMMouseEvent_QueryInterface(IDOMMouseEvent *iface, REFIID riid, void **ppv)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDOMEvent_QueryInterface(&This->IDOMEvent_iface, riid, ppv);
}

static ULONG WINAPI DOMMouseEvent_AddRef(IDOMMouseEvent *iface)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDOMEvent_AddRef(&This->IDOMEvent_iface);
}

static ULONG WINAPI DOMMouseEvent_Release(IDOMMouseEvent *iface)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDOMEvent_Release(&This->IDOMEvent_iface);
}

static HRESULT WINAPI DOMMouseEvent_GetTypeInfoCount(IDOMMouseEvent *iface, UINT *pctinfo)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DOMMouseEvent_GetTypeInfo(IDOMMouseEvent *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DOMMouseEvent_GetIDsOfNames(IDOMMouseEvent *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI DOMMouseEvent_Invoke(IDOMMouseEvent *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DOMMouseEvent_get_screenX(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT32 screen_x;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetScreenX(This->mouse_event, &screen_x);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = screen_x;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_screenY(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT32 screen_y;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetScreenY(This->mouse_event, &screen_y);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = screen_y;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_clientX(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT32 client_x;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetClientX(This->mouse_event, &client_x);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = client_x;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_clientY(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT32 client_y;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetClientY(This->mouse_event, &client_y);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = client_y;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_ctrlKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetCtrlKey(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_shiftKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetShiftKey(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_altKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetAltKey(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_metaKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetMetaKey(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_button(IDOMMouseEvent *iface, USHORT *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT16 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetButton(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_relatedTarget(IDOMMouseEvent *iface, IEventTarget **p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    nsIDOMEventTarget *related_target;
    nsIDOMNode *target_node;
    HTMLDOMNode *node;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetRelatedTarget(This->mouse_event, &related_target);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(!related_target) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMEventTarget_QueryInterface(related_target, &IID_nsIDOMNode, (void**)&target_node);
    nsIDOMEventTarget_Release(related_target);
    if(NS_FAILED(nsres)) {
        FIXME("Only node targets supported\n");
        return E_NOTIMPL;
    }

    hres = get_node(target_node, TRUE, &node);
    nsIDOMNode_Release(target_node);
    if(FAILED(hres))
        return hres;

    *p = &node->event_target.IEventTarget_iface;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_initMouseEvent(IDOMMouseEvent *iface, BSTR type,
        VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable, IHTMLWindow2 *view, LONG detail,
        LONG screen_x, LONG screen_y, LONG client_x, LONG client_y, VARIANT_BOOL ctrl_key,
        VARIANT_BOOL alt_key, VARIANT_BOOL shift_key, VARIANT_BOOL meta_key, USHORT button,
        IEventTarget *related_target)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    nsIDOMEventTarget *nstarget = NULL;
    nsAString type_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %p %d %d %d %d %d %x %x %x %x %u %p)\n", This, debugstr_w(type),
          can_bubble, cancelable, view, detail, screen_x, screen_y, client_x, client_y,
          ctrl_key, alt_key, shift_key, meta_key, button, related_target);

    if(This->target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    if(view)
        FIXME("view argument is not supported\n");

    if(related_target) {
        hres = get_gecko_target(related_target, &nstarget);
        if(FAILED(hres))
            return hres;
    }

    hres = IDOMEvent_initEvent(&This->IDOMEvent_iface, type, can_bubble, cancelable);
    if(SUCCEEDED(hres)) {
        nsAString_InitDepend(&type_str, type);
        nsres = nsIDOMMouseEvent_InitMouseEvent(This->mouse_event, &type_str, can_bubble, cancelable,
                                                NULL /* FIXME */, detail, screen_x, screen_y,
                                                client_x, client_y, !!ctrl_key, !!alt_key, !!shift_key,
                                                !!meta_key, button, nstarget);
        nsAString_Finish(&type_str);
        if(NS_FAILED(nsres)) {
            FIXME("InitMouseEvent failed: %08x\n", nsres);
            return E_FAIL;
        }
    }

    if(nstarget)
        nsIDOMEventTarget_Release(nstarget);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_getModifierState(IDOMMouseEvent *iface, BSTR key,
        VARIANT_BOOL *activated)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(key), activated);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_buttons(IDOMMouseEvent *iface, USHORT *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    UINT16 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetButtons(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_fromElement(IDOMMouseEvent *iface, IHTMLElement **p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    IEventTarget  *related_target = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event_id != EVENTID_LAST) {
        HRESULT hres = S_OK;
        if(event_info[This->event_id].flags & EVENT_MOUSE_FROM_RELATED)
            hres = IDOMMouseEvent_get_relatedTarget(&This->IDOMMouseEvent_iface, &related_target);
        else if(event_info[This->event_id].flags & EVENT_MOUSE_TO_RELATED)
            hres = IDOMEvent_get_target(&This->IDOMEvent_iface, &related_target);
        if(FAILED(hres))
            return hres;
    }

    if(!related_target) {
        *p = NULL;
        return S_OK;
    }

    IEventTarget_QueryInterface(related_target, &IID_IHTMLElement, (void**)p);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_toElement(IDOMMouseEvent *iface, IHTMLElement **p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    IEventTarget  *related_target = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event_id != EVENTID_LAST) {
        HRESULT hres = S_OK;
        if(event_info[This->event_id].flags & EVENT_MOUSE_TO_RELATED)
            hres = IDOMMouseEvent_get_relatedTarget(&This->IDOMMouseEvent_iface, &related_target);
        else if(event_info[This->event_id].flags & EVENT_MOUSE_FROM_RELATED)
            hres = IDOMEvent_get_target(&This->IDOMEvent_iface, &related_target);
        if(FAILED(hres))
            return hres;
    }

    if(!related_target) {
        *p = NULL;
        return S_OK;
    }

    IEventTarget_QueryInterface(related_target, &IID_IHTMLElement, (void**)p);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_x(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_y(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_offsetX(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);

    FIXME("(%p)->(%p) returning 0\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_offsetY(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);

    FIXME("(%p)->(%p) returning 0\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_pageX(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetPageX(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_pageY(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    INT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetPageY(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_layerX(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_layerY(IDOMMouseEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_which(IDOMMouseEvent *iface, USHORT *p)
{
    DOMEvent *This = impl_from_IDOMMouseEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetWhich(This->mouse_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static const IDOMMouseEventVtbl DOMMouseEventVtbl = {
    DOMMouseEvent_QueryInterface,
    DOMMouseEvent_AddRef,
    DOMMouseEvent_Release,
    DOMMouseEvent_GetTypeInfoCount,
    DOMMouseEvent_GetTypeInfo,
    DOMMouseEvent_GetIDsOfNames,
    DOMMouseEvent_Invoke,
    DOMMouseEvent_get_screenX,
    DOMMouseEvent_get_screenY,
    DOMMouseEvent_get_clientX,
    DOMMouseEvent_get_clientY,
    DOMMouseEvent_get_ctrlKey,
    DOMMouseEvent_get_shiftKey,
    DOMMouseEvent_get_altKey,
    DOMMouseEvent_get_metaKey,
    DOMMouseEvent_get_button,
    DOMMouseEvent_get_relatedTarget,
    DOMMouseEvent_initMouseEvent,
    DOMMouseEvent_getModifierState,
    DOMMouseEvent_get_buttons,
    DOMMouseEvent_get_fromElement,
    DOMMouseEvent_get_toElement,
    DOMMouseEvent_get_x,
    DOMMouseEvent_get_y,
    DOMMouseEvent_get_offsetX,
    DOMMouseEvent_get_offsetY,
    DOMMouseEvent_get_pageX,
    DOMMouseEvent_get_pageY,
    DOMMouseEvent_get_layerX,
    DOMMouseEvent_get_layerY,
    DOMMouseEvent_get_which
};

static inline DOMEvent *impl_from_IDOMKeyboardEvent(IDOMKeyboardEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, IDOMKeyboardEvent_iface);
}

static HRESULT WINAPI DOMKeyboardEvent_QueryInterface(IDOMKeyboardEvent *iface, REFIID riid, void **ppv)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDOMEvent_QueryInterface(&This->IDOMEvent_iface, riid, ppv);
}

static ULONG WINAPI DOMKeyboardEvent_AddRef(IDOMKeyboardEvent *iface)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDOMEvent_AddRef(&This->IDOMEvent_iface);
}

static ULONG WINAPI DOMKeyboardEvent_Release(IDOMKeyboardEvent *iface)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDOMEvent_Release(&This->IDOMEvent_iface);
}

static HRESULT WINAPI DOMKeyboardEvent_GetTypeInfoCount(IDOMKeyboardEvent *iface, UINT *pctinfo)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DOMKeyboardEvent_GetTypeInfo(IDOMKeyboardEvent *iface, UINT iTInfo,
                                                   LCID lcid, ITypeInfo **ppTInfo)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DOMKeyboardEvent_GetIDsOfNames(IDOMKeyboardEvent *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI DOMKeyboardEvent_Invoke(IDOMKeyboardEvent *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DOMKeyboardEvent_get_key(IDOMKeyboardEvent *iface, BSTR *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    nsAString key_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);


    nsAString_Init(&key_str, NULL);
    nsres = nsIDOMKeyEvent_GetKey(This->keyboard_event, &key_str);
    return return_nsstr(nsres, &key_str, p);
}

static HRESULT WINAPI DOMKeyboardEvent_get_location(IDOMKeyboardEvent *iface, ULONG *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetLocation(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_ctrlKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetCtrlKey(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_shiftKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetShiftKey(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_altKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetAltKey(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_metaKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetMetaKey(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_repeat(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetRepeat(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_getModifierState(IDOMKeyboardEvent *iface, BSTR key,
        VARIANT_BOOL *state)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(key), state);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMKeyboardEvent_initKeyboardEvent(IDOMKeyboardEvent *iface, BSTR type,
        VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable, IHTMLWindow2 *view, BSTR key,
        ULONG location, BSTR modifiers_list, VARIANT_BOOL repeat, BSTR locale)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%s %x %x %p %s %u %s %x %s)\n", This, debugstr_w(type), can_bubble,
          cancelable, view, debugstr_w(key), location, debugstr_w(modifiers_list),
          repeat, debugstr_w(locale));
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMKeyboardEvent_get_keyCode(IDOMKeyboardEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetKeyCode(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_charCode(IDOMKeyboardEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetKeyCode(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_which(IDOMKeyboardEvent *iface, LONG *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetWhich(This->keyboard_event, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_char(IDOMKeyboardEvent *iface, VARIANT *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMKeyboardEvent_get_locale(IDOMKeyboardEvent *iface, BSTR *p)
{
    DOMEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IDOMKeyboardEventVtbl DOMKeyboardEventVtbl = {
    DOMKeyboardEvent_QueryInterface,
    DOMKeyboardEvent_AddRef,
    DOMKeyboardEvent_Release,
    DOMKeyboardEvent_GetTypeInfoCount,
    DOMKeyboardEvent_GetTypeInfo,
    DOMKeyboardEvent_GetIDsOfNames,
    DOMKeyboardEvent_Invoke,
    DOMKeyboardEvent_get_key,
    DOMKeyboardEvent_get_location,
    DOMKeyboardEvent_get_ctrlKey,
    DOMKeyboardEvent_get_shiftKey,
    DOMKeyboardEvent_get_altKey,
    DOMKeyboardEvent_get_metaKey,
    DOMKeyboardEvent_get_repeat,
    DOMKeyboardEvent_getModifierState,
    DOMKeyboardEvent_initKeyboardEvent,
    DOMKeyboardEvent_get_keyCode,
    DOMKeyboardEvent_get_charCode,
    DOMKeyboardEvent_get_which,
    DOMKeyboardEvent_get_char,
    DOMKeyboardEvent_get_locale
};

typedef struct {
    DOMEvent event;
    IDOMCustomEvent IDOMCustomEvent_iface;
    VARIANT detail;
} DOMCustomEvent;

static inline DOMCustomEvent *impl_from_IDOMCustomEvent(IDOMCustomEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMCustomEvent, IDOMCustomEvent_iface);
}

static HRESULT WINAPI DOMCustomEvent_QueryInterface(IDOMCustomEvent *iface, REFIID riid, void **ppv)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDOMEvent_QueryInterface(&This->event.IDOMEvent_iface, riid, ppv);
}

static ULONG WINAPI DOMCustomEvent_AddRef(IDOMCustomEvent *iface)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDOMEvent_AddRef(&This->event.IDOMEvent_iface);
}

static ULONG WINAPI DOMCustomEvent_Release(IDOMCustomEvent *iface)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDOMEvent_Release(&This->event.IDOMEvent_iface);
}

static HRESULT WINAPI DOMCustomEvent_GetTypeInfoCount(IDOMCustomEvent *iface, UINT *pctinfo)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDispatchEx_GetTypeInfoCount(&This->event.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DOMCustomEvent_GetTypeInfo(IDOMCustomEvent *iface, UINT iTInfo,
                                                   LCID lcid, ITypeInfo **ppTInfo)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDispatchEx_GetTypeInfo(&This->event.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DOMCustomEvent_GetIDsOfNames(IDOMCustomEvent *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDispatchEx_GetIDsOfNames(&This->event.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI DOMCustomEvent_Invoke(IDOMCustomEvent *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    return IDispatchEx_Invoke(&This->event.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DOMCustomEvent_get_detail(IDOMCustomEvent *iface, VARIANT *p)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_EMPTY;
    return VariantCopy(p, &This->detail);
}

static HRESULT WINAPI DOMCustomEvent_initCustomEvent(IDOMCustomEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
                                                     VARIANT_BOOL cancelable, VARIANT *detail)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %s)\n", This, debugstr_w(type), can_bubble, cancelable, debugstr_variant(detail));

    hres = IDOMEvent_initEvent(&This->event.IDOMEvent_iface, type, can_bubble, cancelable);
    if(FAILED(hres))
        return hres;

    return VariantCopy(&This->detail, detail);
}

static const IDOMCustomEventVtbl DOMCustomEventVtbl = {
    DOMCustomEvent_QueryInterface,
    DOMCustomEvent_AddRef,
    DOMCustomEvent_Release,
    DOMCustomEvent_GetTypeInfoCount,
    DOMCustomEvent_GetTypeInfo,
    DOMCustomEvent_GetIDsOfNames,
    DOMCustomEvent_Invoke,
    DOMCustomEvent_get_detail,
    DOMCustomEvent_initCustomEvent
};

static DOMCustomEvent *DOMCustomEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMCustomEvent, event);
}

static void *DOMCustomEvent_query_interface(DOMEvent *event, REFIID riid)
{
    DOMCustomEvent *custom_event = DOMCustomEvent_from_DOMEvent(event);
    if(IsEqualGUID(&IID_IDOMCustomEvent, riid))
        return &custom_event->IDOMCustomEvent_iface;
    return NULL;
}

static void DOMCustomEvent_destroy(DOMEvent *event)
{
    DOMCustomEvent *custom_event = DOMCustomEvent_from_DOMEvent(event);
    VariantClear(&custom_event->detail);
}


static const tid_t DOMEvent_iface_tids[] = {
    IDOMEvent_tid,
    0
};

static dispex_static_data_t DOMEvent_dispex = {
    NULL,
    DispDOMEvent_tid,
    DOMEvent_iface_tids
};

static const tid_t DOMUIEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMUIEvent_tid,
    0
};

static dispex_static_data_t DOMUIEvent_dispex = {
    NULL,
    DispDOMUIEvent_tid,
    DOMUIEvent_iface_tids
};

static const tid_t DOMMouseEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMUIEvent_tid,
    IDOMMouseEvent_tid,
    0
};

static dispex_static_data_t DOMMouseEvent_dispex = {
    NULL,
    DispDOMMouseEvent_tid,
    DOMMouseEvent_iface_tids
};

static const tid_t DOMKeyboardEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMUIEvent_tid,
    IDOMKeyboardEvent_tid,
    0
};

static dispex_static_data_t DOMKeyboardEvent_dispex = {
    NULL,
    DispDOMKeyboardEvent_tid,
    DOMKeyboardEvent_iface_tids
};

static const tid_t DOMCustomEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMCustomEvent_tid,
    0
};

static dispex_static_data_t DOMCustomEvent_dispex = {
    NULL,
    DispDOMCustomEvent_tid,
    DOMCustomEvent_iface_tids
};

static BOOL check_event_iface(nsIDOMEvent *event, REFIID riid)
{
    nsISupports *iface;
    nsresult nsres;

    nsres = nsIDOMEvent_QueryInterface(event, riid, (void**)&iface);
    if(NS_FAILED(nsres))
        return FALSE;

    nsISupports_Release(iface);
    return TRUE;
}

static DOMEvent *alloc_event(nsIDOMEvent *nsevent, compat_mode_t compat_mode, eventid_t event_id)
{
    dispex_static_data_t *dispex_data = &DOMEvent_dispex;
    DOMEvent *event = NULL;
    nsresult nsres;

    if(check_event_iface(nsevent, &IID_nsIDOMCustomEvent)) {
        DOMCustomEvent *custom_event = heap_alloc_zero(sizeof(*custom_event));
        if(!custom_event)
            return NULL;

        custom_event->IDOMCustomEvent_iface.lpVtbl = &DOMCustomEventVtbl;
        custom_event->event.query_interface = DOMCustomEvent_query_interface;
        custom_event->event.destroy = DOMCustomEvent_destroy;
        event = &custom_event->event;
        dispex_data = &DOMCustomEvent_dispex;
    }else {
        event = heap_alloc_zero(sizeof(*event));
        if(!event)
            return NULL;
    }

    event->IDOMEvent_iface.lpVtbl = &DOMEventVtbl;
    event->IDOMUIEvent_iface.lpVtbl = &DOMUIEventVtbl;
    event->IDOMMouseEvent_iface.lpVtbl = &DOMMouseEventVtbl;
    event->IDOMKeyboardEvent_iface.lpVtbl = &DOMKeyboardEventVtbl;
    event->ref = 1;
    event->event_id = event_id;
    if(event_id != EVENTID_LAST) {
        event->type = heap_strdupW(event_info[event_id].name);
        if(!event->type) {
            heap_free(event);
            return NULL;
        }
        event->bubbles = (event_info[event_id].flags & EVENT_BUBBLES) != 0;
        event->cancelable = (event_info[event_id].flags & EVENT_CANCELABLE) != 0;
    }
    nsIDOMEvent_AddRef(event->nsevent = nsevent);

    event->time_stamp = get_time_stamp();

    nsres = nsIDOMEvent_QueryInterface(nsevent, &IID_nsIDOMUIEvent, (void**)&event->ui_event);
    if(NS_SUCCEEDED(nsres))
        dispex_data = &DOMUIEvent_dispex;
    else
        event->ui_event = NULL;

    nsres = nsIDOMEvent_QueryInterface(nsevent, &IID_nsIDOMMouseEvent, (void**)&event->mouse_event);
    if(NS_SUCCEEDED(nsres))
        dispex_data = &DOMMouseEvent_dispex;
    else
        event->mouse_event = NULL;

    nsres = nsIDOMEvent_QueryInterface(nsevent, &IID_nsIDOMKeyEvent, (void**)&event->keyboard_event);
    if(NS_SUCCEEDED(nsres))
        dispex_data = &DOMKeyboardEvent_dispex;
    else
        event->keyboard_event = NULL;

    init_dispatch(&event->dispex, (IUnknown*)&event->IDOMEvent_iface, dispex_data, compat_mode);
    return event;
}

HRESULT create_event_from_nsevent(nsIDOMEvent *nsevent, compat_mode_t compat_mode, DOMEvent **ret_event)
{
    eventid_t event_id = EVENTID_LAST;
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMEvent_GetType(nsevent, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const WCHAR *type;
        nsAString_GetData(&nsstr, &type);
        event_id = str_to_eid(type);
        if(event_id == EVENTID_LAST)
            FIXME("unknown event type %s\n", debugstr_w(type));
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }
    nsAString_Finish(&nsstr);

    event = alloc_event(nsevent, compat_mode, event_id);
    if(!event)
        return E_OUTOFMEMORY;

    event->trusted = TRUE;
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

    event = alloc_event(nsevent, dispex_compat_mode(&doc->node.event_target.dispex), EVENTID_LAST);
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

    event = alloc_event(nsevent, doc->document_mode, event_id);
    if(!event)
        return E_OUTOFMEMORY;

    event->event_id = event_id;
    event->trusted = TRUE;
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

    TRACE("%p,%d,%p,%p\n", disp, dispid, event_obj, retv);

    if(event_obj) {
        V_VT(&event_arg) = VT_DISPATCH;
        V_DISPATCH(&event_arg) = (IDispatch*)event_obj;
        dp.rgvarg = &event_arg;
        dp.cArgs = 1;
    }

    memset(&ei, 0, sizeof(ei));
    return IDispatch_Invoke(disp, dispid, &IID_NULL, 0, DISPATCH_METHOD, &dp, retv, &ei, &argerr);
}

static BOOL use_event_quirks(EventTarget *event_target)
{
    return dispex_compat_mode(&event_target->dispex) < COMPAT_MODE_IE9;
}

static BOOL is_cp_event(cp_static_data_t *data, DISPID dispid)
{
    int min, max, i;
    HRESULT hres;

    if(!data || dispid == DISPID_UNKNOWN)
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

static void call_event_handlers(EventTarget *event_target, DOMEvent *event, dispatch_mode_t dispatch_mode)
{
    const listener_container_t *container = get_listener_container(event_target, event->type, FALSE);
    event_listener_t *listener, listeners_buf[8], *listeners = listeners_buf;
    unsigned listeners_cnt, listeners_size;
    ConnectionPointContainer *cp_container = NULL;
    const event_target_vtbl_t *vtbl = NULL;
    BOOL skip_onevent_listener = FALSE;
    VARIANT v;
    HRESULT hres;

    assert(!event->current_target);
    event->current_target = event_target;

    if(container && !list_empty(&container->listeners) && event->phase != DEP_CAPTURING_PHASE) {
        listener = LIST_ENTRY(list_tail(&container->listeners), event_listener_t, entry);
        if(listener && listener->function && listener->type == LISTENER_TYPE_ONEVENT
                && use_event_quirks(event_target)) {
            DISPID named_arg = DISPID_THIS;
            VARIANTARG arg;
            DISPPARAMS dp = {&arg, &named_arg, 1, 1};

            skip_onevent_listener = TRUE;

            V_VT(&arg) = VT_DISPATCH;
            V_DISPATCH(&arg) = (IDispatch*)&event_target->dispex.IDispatchEx_iface;
            V_VT(&v) = VT_EMPTY;

            TRACE("%p %s >>>\n", event_target, debugstr_w(event->type));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%p %s <<< %s\n", event_target, debugstr_w(event->type), debugstr_variant(&v));

                if(event->cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%p %s <<< %08x\n", event_target, debugstr_w(event->type), hres);
            }
        }
    }

    listeners_cnt = 0;
    listeners_size = ARRAY_SIZE(listeners_buf);

    if(container) {
        LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
            if(!listener->function)
                continue;
            switch(listener->type) {
            case LISTENER_TYPE_ONEVENT:
                if(skip_onevent_listener || event->phase == DEP_CAPTURING_PHASE)
                    continue;
                break;
            case LISTENER_TYPE_CAPTURE:
                if(event->phase == DEP_BUBBLING_PHASE || dispatch_mode == DISPATCH_LEGACY)
                    continue;
                break;
            case LISTENER_TYPE_BUBBLE:
                if(event->phase == DEP_CAPTURING_PHASE || dispatch_mode == DISPATCH_LEGACY)
                    continue;
                break;
            case LISTENER_TYPE_ATTACHED:
                if(event->phase == DEP_CAPTURING_PHASE || dispatch_mode == DISPATCH_STANDARD)
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

    for(listener = listeners; !event->stop_immediate_propagation
            && listener < listeners + listeners_cnt; listener++) {
        if(listener->type != LISTENER_TYPE_ATTACHED) {
            DISPID named_arg = DISPID_THIS;
            VARIANTARG args[2];
            DISPPARAMS dp = {args, &named_arg, 2, 1};

            V_VT(args) = VT_DISPATCH;
            V_DISPATCH(args) = (IDispatch*)&event_target->dispex.IDispatchEx_iface;
            V_VT(args+1) = VT_DISPATCH;
            V_DISPATCH(args+1) = dispatch_mode == DISPATCH_LEGACY
                ? (IDispatch*)event->event_obj : (IDispatch*)&event->IDOMEvent_iface;
            V_VT(&v) = VT_EMPTY;

            TRACE("%p %s >>>\n", event_target, debugstr_w(event->type));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%p %s <<< %s\n", event_target, debugstr_w(event->type),
                      debugstr_variant(&v));

                if(event->cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%p %s <<< %08x\n", event_target, debugstr_w(event->type), hres);
            }
        }else {
            VARIANTARG arg;
            DISPPARAMS dp = {&arg, NULL, 1, 0};

            V_VT(&arg) = VT_DISPATCH;
            V_DISPATCH(&arg) = (IDispatch*)event->event_obj;
            V_VT(&v) = VT_EMPTY;

            TRACE("%p %s attached >>>\n", event_target, debugstr_w(event->type));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%p %s attached <<<\n", event_target, debugstr_w(event->type));

                if(event->cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%p %s attached <<< %08x\n", event_target, debugstr_w(event->type), hres);
            }
        }
    }

    for(listener = listeners; listener < listeners + listeners_cnt; listener++)
        IDispatch_Release(listener->function);
    if(listeners != listeners_buf)
        heap_free(listeners);

    if(event->phase != DEP_CAPTURING_PHASE && event->event_id != EVENTID_LAST
       && event_info[event->event_id].dispid && (vtbl = dispex_get_vtbl(&event_target->dispex))
       && vtbl->get_cp_container)
        cp_container = vtbl->get_cp_container(&event_target->dispex);
    if(cp_container) {
        if(cp_container->cps) {
            ConnectionPoint *cp;
            unsigned i, j;

            for(j=0; cp_container->cp_entries[j].riid; j++) {
                cp = cp_container->cps + j;
                if(!cp->sinks_size || !is_cp_event(cp->data, event_info[event->event_id].dispid))
                    continue;

                for(i=0; i < cp->sinks_size; i++) {
                    if(!cp->sinks[i].disp)
                        continue;

                    V_VT(&v) = VT_EMPTY;

                    TRACE("%p cp %s [%u] >>>\n", event_target, debugstr_w(event->type), i);
                    hres = call_cp_func(cp->sinks[i].disp, event_info[event->event_id].dispid,
                            cp->data->pass_event_arg ? event->event_obj : NULL, &v);
                    if(hres == S_OK) {
                        TRACE("%p cp %s [%u] <<<\n", event_target, debugstr_w(event->type), i);

                        if(event->cancelable) {
                            if(V_VT(&v) == VT_BOOL) {
                                if(!V_BOOL(&v))
                                    IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                            }else if(V_VT(&v) != VT_EMPTY) {
                                FIXME("unhandled result %s\n", debugstr_variant(&v));
                            }
                        }
                        VariantClear(&v);
                    }else {
                        WARN("%p cp %s [%u] <<< %08x\n", event_target, debugstr_w(event->type), i, hres);
                    }
                }
            }
        }
        IConnectionPointContainer_Release(&cp_container->IConnectionPointContainer_iface);
    }

    event->current_target = NULL;
}

static HRESULT dispatch_event_object(EventTarget *event_target, DOMEvent *event,
                                     dispatch_mode_t dispatch_mode, VARIANT_BOOL *r)
{
    EventTarget *target_chain_buf[8], **target_chain = target_chain_buf;
    unsigned chain_cnt, chain_buf_size, i;
    const event_target_vtbl_t *vtbl, *target_vtbl;
    HTMLEventObj *event_obj_ref = NULL;
    IHTMLEventObj *prev_event = NULL;
    EventTarget *iter;
    HRESULT hres;

    TRACE("(%p) %s\n", event_target, debugstr_w(event->type));

    if(!event->type) {
        FIXME("Uninitialized event.\n");
        return E_FAIL;
    }

    if(event->current_target) {
        FIXME("event is being dispatched.\n");
        return E_FAIL;
    }

    iter = event_target;
    IEventTarget_AddRef(&event_target->IEventTarget_iface);

    chain_cnt = 0;
    chain_buf_size = ARRAY_SIZE(target_chain_buf);

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
        event_obj_ref = alloc_event_obj(event, dispex_compat_mode(&event->dispex));
        if(event_obj_ref)
            event->event_obj = &event_obj_ref->IHTMLEventObj_iface;
    }

    target_vtbl = dispex_get_vtbl(&event_target->dispex);
    if(target_vtbl && target_vtbl->set_current_event)
        prev_event = target_vtbl->set_current_event(&event_target->dispex, event->event_obj);

    if(event->target)
        IEventTarget_Release(&event->target->IEventTarget_iface);
    event->target = event_target;
    IEventTarget_AddRef(&event_target->IEventTarget_iface);

    event->phase = DEP_CAPTURING_PHASE;
    i = chain_cnt-1;
    while(!event->stop_propagation && i)
        call_event_handlers(target_chain[i--], event, dispatch_mode);

    if(!event->stop_propagation) {
        event->phase = DEP_AT_TARGET;
        call_event_handlers(target_chain[0], event, dispatch_mode);
    }

    if(event->bubbles) {
        event->phase = DEP_BUBBLING_PHASE;
        for(i = 1; !event->stop_propagation && i < chain_cnt; i++)
            call_event_handlers(target_chain[i], event, dispatch_mode);
    }

    if(r)
        *r = variant_bool(!event->prevent_default);

    if(target_vtbl && target_vtbl->set_current_event) {
        prev_event = target_vtbl->set_current_event(&event_target->dispex, prev_event);
        if(prev_event)
            IHTMLEventObj_Release(prev_event);
    }

    if(event->event_id != EVENTID_LAST && (event_info[event->event_id].flags & EVENT_HASDEFAULTHANDLERS)) {
        BOOL prevent_default = event->prevent_default;
        for(i = 0; !prevent_default && i < chain_cnt; i++) {
            vtbl = dispex_get_vtbl(&target_chain[i]->dispex);
            if(!vtbl || !vtbl->handle_event_default)
                continue;
            hres = vtbl->handle_event_default(&event_target->dispex, event->event_id,
                    event->nsevent, &prevent_default);
            if(FAILED(hres) || event->stop_propagation)
                break;
            if(prevent_default)
                nsIDOMEvent_PreventDefault(event->nsevent);
        }
    }

    event->prevent_default = FALSE;
    if(event_obj_ref) {
        event->event_obj = NULL;
        IHTMLEventObj_Release(&event_obj_ref->IHTMLEventObj_iface);
    }

    for(i = 0; i < chain_cnt; i++)
        IEventTarget_Release(&target_chain[i]->IEventTarget_iface);
    if(target_chain != target_chain_buf)
        heap_free(target_chain);

    return S_OK;
}

void dispatch_event(EventTarget *event_target, DOMEvent *event)
{
    dispatch_event_object(event_target, event, DISPATCH_BOTH, NULL);

    /*
     * We may have registered multiple Gecko listeners for the same event type,
     * but we already dispatched event to all relevant targets. Stop event
     * propagation here to avoid events being dispatched multiple times.
     */
    if(event->event_id != EVENTID_LAST && (event_info[event->event_id].flags & EVENT_BIND_TO_TARGET))
        nsIDOMEvent_StopPropagation(event->nsevent);
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
        event_obj = alloc_event_obj(NULL, dispex_compat_mode(&node->event_target.dispex));
        if(!event_obj)
            return E_OUTOFMEMORY;
    }

    if(!event_obj->event)
        hres = create_document_event(node->doc, eid, &event_obj->event);

    if(SUCCEEDED(hres)) {
        event_obj->event->event_obj = &event_obj->IHTMLEventObj_iface;
        dispatch_event_object(&node->event_target, event_obj->event, DISPATCH_LEGACY, NULL);
        event_obj->event->event_obj = NULL;
    }

    IHTMLEventObj_Release(&event_obj->IHTMLEventObj_iface);
    if(FAILED(hres))
        return hres;

    *cancelled = VARIANT_TRUE; /* FIXME */
    return S_OK;
}

HRESULT ensure_doc_nsevent_handler(HTMLDocumentNode *doc, nsIDOMNode *nsnode, eventid_t eid)
{
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
    case EVENTID_LAST:
        return S_OK;
    default:
        break;
    }

    if(event_info[eid].flags & EVENT_DEFAULTLISTENER) {
        nsnode = NULL;
    }else if(event_info[eid].flags & EVENT_BIND_TO_TARGET) {
        if(!nsnode)
            nsnode = doc->node.nsnode;
    }else {
        return S_OK;
    }

    if(!nsnode || nsnode == doc->node.nsnode) {
        if(doc->event_vector[eid])
            return S_OK;
        doc->event_vector[eid] = TRUE;
    }

    add_nsevent_listener(doc, nsnode, event_info[eid].name);
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
    lstrcpyW(buf+2, event_info[eid].name);
    return dispex_get_dprop_ref(&event_target->dispex, buf, alloc, ret);
}

static event_listener_t *get_onevent_listener(EventTarget *event_target, eventid_t eid, BOOL alloc)
{
    listener_container_t *container;
    event_listener_t *listener;

    container = get_listener_container(event_target, event_info[eid].name, alloc);
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

    container = get_listener_container(event_target, event_info[eid].name, TRUE);
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

    remove_event_listener(event_target, event_info[eid].name, LISTENER_TYPE_ATTACHED, disp);
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
            ensure_doc_nsevent_handler(doc, NULL, i);
    }
}

void check_event_attr(HTMLDocumentNode *doc, nsIDOMElement *nselem)
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

    nsres = nsIDOMElement_HasAttributes(nselem, &has_attrs);
    if(NS_FAILED(nsres) || !has_attrs)
        return;

    nsres = nsIDOMElement_GetAttributes(nselem, &attr_map);
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
            hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
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
            hres = ensure_doc_nsevent_handler(doc, NULL, i);
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

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(type), function, capture);

    container = get_listener_container(This, type, TRUE);
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

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(type), listener, capture);

    remove_event_listener(This, type, capture ? LISTENER_TYPE_CAPTURE : LISTENER_TYPE_BUBBLE, listener);
    return S_OK;
}

static HRESULT WINAPI EventTarget_dispatchEvent(IEventTarget *iface, IDOMEvent *event_iface, VARIANT_BOOL *result)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    DOMEvent *event = unsafe_impl_from_IDOMEvent(event_iface);

    TRACE("(%p)->(%p %p)\n", This, event, result);

    if(!event) {
        WARN("Invalid event\n");
        return E_INVALIDARG;
    }

    return dispatch_event_object(This, event, DISPATCH_STANDARD, result);
}

static HRESULT IEventTarget_addEventListener_hook(DispatchEx *dispex, LCID lcid, WORD flags,
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

static HRESULT IEventTarget_removeEventListener_hook(DispatchEx *dispex, LCID lcid, WORD flags,
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

static EventTarget *unsafe_impl_from_IEventTarget(IEventTarget *iface)
{
    return iface && iface->lpVtbl == &EventTargetVtbl ? impl_from_IEventTarget(iface) : NULL;
}

static HRESULT get_gecko_target(IEventTarget *target, nsIDOMEventTarget **ret)
{
    EventTarget *event_target = unsafe_impl_from_IEventTarget(target);
    const event_target_vtbl_t *vtbl;
    nsresult nsres;

    if(!event_target) {
        WARN("Not our IEventTarget implementation\n");
        return E_INVALIDARG;
    }

    vtbl = (const event_target_vtbl_t*)dispex_get_vtbl(&event_target->dispex);
    nsres = nsISupports_QueryInterface(vtbl->get_gecko_target(&event_target->dispex),
                                       &IID_nsIDOMEventTarget, (void**)ret);
    assert(nsres == NS_OK);
    return S_OK;
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
    return wcscmp(key, WINE_RB_ENTRY_VALUE(entry, listener_container_t, entry)->type);
}

void EventTarget_Init(EventTarget *event_target, IUnknown *outer, dispex_static_data_t *dispex_data,
                      compat_mode_t compat_mode)
{
    init_dispatch(&event_target->dispex, outer, dispex_data, compat_mode);
    event_target->IEventTarget_iface.lpVtbl = &EventTargetVtbl;
    wine_rb_init(&event_target->handler_map, event_id_cmp);
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
