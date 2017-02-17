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

typedef struct {
    struct wine_rb_entry entry;
    eventid_t event_id;
    IDispatch *handler_prop;
    DWORD handler_cnt;
    IDispatch **handlers;
} handler_vector_t;

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

static const WCHAR HTMLEventsW[] = {'H','T','M','L','E','v','e','n','t','s',0};
static const WCHAR KeyboardEventW[] = {'K','e','y','b','o','a','r','d','E','v','e','n','t',0};
static const WCHAR MouseEventW[] = {'M','o','u','s','e','E','v','e','n','t',0};

enum {
    EVENTT_NONE,
    EVENTT_HTML,
    EVENTT_KEY,
    EVENTT_MOUSE
};

static const WCHAR *event_types[] = {
    NULL,
    HTMLEventsW,
    KeyboardEventW,
    MouseEventW
};

typedef struct {
    const WCHAR *name;
    DWORD type;
    DISPID dispid;
    DWORD flags;
} event_info_t;

#define EVENT_DEFAULTLISTENER    0x0001
#define EVENT_BUBBLE             0x0002
#define EVENT_FORWARDBODY        0x0004
#define EVENT_BIND_TO_BODY       0x0008
#define EVENT_CANCELABLE         0x0010
#define EVENT_HASDEFAULTHANDLERS 0x0020
#define EVENT_FIXME              0x0040

static const event_info_t event_info[] = {
    {abortW,             EVENTT_NONE,   DISPID_EVMETH_ONABORT,
        EVENT_BIND_TO_BODY},
    {beforeactivateW,    EVENTT_NONE,   DISPID_EVMETH_ONBEFOREACTIVATE,
        EVENT_FIXME},
    {beforeunloadW,      EVENTT_NONE,   DISPID_EVMETH_ONBEFOREUNLOAD,
        EVENT_DEFAULTLISTENER|EVENT_FORWARDBODY},
    {blurW,              EVENTT_HTML,   DISPID_EVMETH_ONBLUR,
        EVENT_DEFAULTLISTENER},
    {changeW,            EVENTT_HTML,   DISPID_EVMETH_ONCHANGE,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {clickW,             EVENTT_MOUSE,  DISPID_EVMETH_ONCLICK,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE|EVENT_CANCELABLE|EVENT_HASDEFAULTHANDLERS},
    {contextmenuW,       EVENTT_MOUSE,  DISPID_EVMETH_ONCONTEXTMENU,
        EVENT_BUBBLE|EVENT_CANCELABLE},
    {dataavailableW,     EVENTT_NONE,   DISPID_EVMETH_ONDATAAVAILABLE,
        EVENT_BUBBLE},
    {dblclickW,          EVENTT_MOUSE,  DISPID_EVMETH_ONDBLCLICK,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE|EVENT_CANCELABLE},
    {dragW,              EVENTT_MOUSE,  DISPID_EVMETH_ONDRAG,
        EVENT_FIXME|EVENT_CANCELABLE},
    {dragstartW,         EVENTT_MOUSE,  DISPID_EVMETH_ONDRAGSTART,
        EVENT_FIXME|EVENT_CANCELABLE},
    {errorW,             EVENTT_NONE,   DISPID_EVMETH_ONERROR,
        EVENT_BIND_TO_BODY},
    {focusW,             EVENTT_HTML,   DISPID_EVMETH_ONFOCUS,
        EVENT_DEFAULTLISTENER},
    {focusinW,           EVENTT_HTML,   DISPID_EVMETH_ONFOCUSIN,
        EVENT_BUBBLE},
    {focusoutW,          EVENTT_HTML,   DISPID_EVMETH_ONFOCUSOUT,
        EVENT_BUBBLE},
    {helpW,              EVENTT_KEY,    DISPID_EVMETH_ONHELP,
        EVENT_BUBBLE|EVENT_CANCELABLE},
    {keydownW,           EVENTT_KEY,    DISPID_EVMETH_ONKEYDOWN,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE|EVENT_HASDEFAULTHANDLERS},
    {keypressW,          EVENTT_KEY,    DISPID_EVMETH_ONKEYPRESS,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {keyupW,             EVENTT_KEY,    DISPID_EVMETH_ONKEYUP,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {loadW,              EVENTT_HTML,   DISPID_EVMETH_ONLOAD,
        EVENT_BIND_TO_BODY},
    {messageW,           EVENTT_NONE,   DISPID_EVMETH_ONMESSAGE,
        EVENT_FORWARDBODY /* FIXME: remove when we get the target right */ },
    {mousedownW,         EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEDOWN,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE|EVENT_CANCELABLE},
    {mousemoveW,         EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEMOVE,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseoutW,          EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEOUT,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseoverW,         EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEOVER,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseupW,           EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEUP,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mousewheelW,        EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEWHEEL,
        EVENT_FIXME},
    {pasteW,             EVENTT_NONE,   DISPID_EVMETH_ONPASTE,
        EVENT_CANCELABLE},
    {readystatechangeW,  EVENTT_NONE,   DISPID_EVMETH_ONREADYSTATECHANGE,
        0},
    {resizeW,            EVENTT_NONE,   DISPID_EVMETH_ONRESIZE,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {scrollW,            EVENTT_HTML,   DISPID_EVMETH_ONSCROLL,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {selectionchangeW,   EVENTT_NONE,   DISPID_EVMETH_ONSELECTIONCHANGE,
        EVENT_FIXME},
    {selectstartW,       EVENTT_MOUSE,  DISPID_EVMETH_ONSELECTSTART,
        EVENT_CANCELABLE},
    {submitW,            EVENTT_HTML,   DISPID_EVMETH_ONSUBMIT,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE|EVENT_CANCELABLE|EVENT_HASDEFAULTHANDLERS},
    {unloadW,            EVENTT_HTML,   DISPID_EVMETH_ONUNLOAD,
        EVENT_FIXME}
};

eventid_t str_to_eid(LPCWSTR str)
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
        if(!strcmpW(event_info[i].name, str+2))
            return i;
    }

    return EVENTID_LAST;
}

struct HTMLEventObj {
    DispatchEx dispex;
    IHTMLEventObj IHTMLEventObj_iface;

    LONG ref;

    HTMLDOMNode *target;
    const event_info_t *type;
    nsIDOMEvent *nsevent;
    VARIANT return_value;
    BOOL prevent_default;
    BOOL cancel_bubble;
};

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
        if(This->target)
            IHTMLDOMNode_Release(&This->target->IHTMLDOMNode_iface);
        if(This->nsevent)
            nsIDOMEvent_Release(This->nsevent);
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
    if(This->target)
        IHTMLDOMNode_QueryInterface(&This->target->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)p);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_altKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetAltKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetAltKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_ctrlKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetCtrlKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetCtrlKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_shiftKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetShiftKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetShiftKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
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
    if(!V_BOOL(&v))
        This->prevent_default = TRUE;
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

    This->cancel_bubble = !!v;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->cancel_bubble ? VARIANT_TRUE : VARIANT_FALSE;
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

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
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

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
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

    if(This->nsevent) {
        nsIDOMUIEvent *ui_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event);
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

    if(This->nsevent) {
        nsIDOMUIEvent *ui_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event);
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

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
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

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
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

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
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

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
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

static HTMLEventObj *create_event(void)
{
    HTMLEventObj *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return NULL;

    ret->IHTMLEventObj_iface.lpVtbl = &HTMLEventObjVtbl;
    ret->ref = 1;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLEventObj_iface, &HTMLEventObj_dispex);

    return ret;
}

static HRESULT set_event_info(HTMLEventObj *event, HTMLDOMNode *target, eventid_t eid, nsIDOMEvent *nsevent)
{
    event->type = event_info+eid;
    event->nsevent = nsevent;

    if(nsevent) {
        nsIDOMEvent_AddRef(nsevent);
    }else if(event_types[event_info[eid].type]) {
        nsAString type_str;
        nsresult nsres;

        nsAString_InitDepend(&type_str, event_types[event_info[eid].type]);
        nsres = nsIDOMHTMLDocument_CreateEvent(target->doc->nsdoc, &type_str, &event->nsevent);
        nsAString_Finish(&type_str);
        if(NS_FAILED(nsres)) {
            ERR("Could not create event: %08x\n", nsres);
            return E_FAIL;
        }
    }

    event->target = target;
    if(target)
        IHTMLDOMNode_AddRef(&target->IHTMLDOMNode_iface);
    return S_OK;
}

HRESULT create_event_obj(IHTMLEventObj **ret)
{
    HTMLEventObj *event;

    event = create_event();
    if(!event)
        return E_OUTOFMEMORY;

    *ret = &event->IHTMLEventObj_iface;
    return S_OK;
}

static handler_vector_t *get_handler_vector(EventTarget *event_target, eventid_t eid, BOOL alloc)
{
    const dispex_static_data_vtbl_t *vtbl;
    handler_vector_t *handler_vector;
    struct wine_rb_entry *entry;

    vtbl = dispex_get_vtbl(&event_target->dispex);
    if(vtbl->get_event_target)
        event_target = vtbl->get_event_target(&event_target->dispex);

    entry = wine_rb_get(&event_target->handler_map, (const void*)eid);
    if(entry)
        return WINE_RB_ENTRY_VALUE(entry, handler_vector_t, entry);
    if(!alloc)
        return NULL;

    handler_vector = heap_alloc_zero(sizeof(*handler_vector));
    if(!handler_vector)
        return NULL;

    handler_vector->event_id = eid;
    vtbl = dispex_get_vtbl(&event_target->dispex);
    if(vtbl->bind_event)
        vtbl->bind_event(&event_target->dispex, eid);
    else
        FIXME("Unsupported event binding on target %p\n", event_target);

    wine_rb_put(&event_target->handler_map, (const void*)eid, &handler_vector->entry);
    return handler_vector;
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

static HRESULT call_cp_func(IDispatch *disp, DISPID dispid, HTMLEventObj *event_obj, VARIANT *retv)
{
    DISPPARAMS dp = {NULL,NULL,0,0};
    VARIANT event_arg;
    ULONG argerr;
    EXCEPINFO ei;

    if(event_obj) {
        V_VT(&event_arg) = VT_DISPATCH;
        V_DISPATCH(&event_arg) = (IDispatch*)&event_obj->IHTMLEventObj_iface;
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

void call_event_handlers(HTMLDocumentNode *doc, HTMLEventObj *event_obj, EventTarget *event_target,
        ConnectionPointContainer *cp_container, eventid_t eid, IDispatch *this_obj)
{
    handler_vector_t *handler_vector = get_handler_vector(event_target, eid, FALSE);
    const BOOL cancelable = event_info[eid].flags & EVENT_CANCELABLE;
    VARIANT v;
    HRESULT hres;

    if(handler_vector && handler_vector->handler_prop) {
        DISPID named_arg = DISPID_THIS;
        VARIANTARG arg;
        DISPPARAMS dp = {&arg, &named_arg, 1, 1};

        V_VT(&arg) = VT_DISPATCH;
        V_DISPATCH(&arg) = this_obj;
        V_VT(&v) = VT_EMPTY;

        TRACE("%s >>>\n", debugstr_w(event_info[eid].name));
        hres = call_disp_func(handler_vector->handler_prop, &dp, &v);
        if(hres == S_OK) {
            TRACE("%s <<< %s\n", debugstr_w(event_info[eid].name), debugstr_variant(&v));

            if(cancelable) {
                if(V_VT(&v) == VT_BOOL) {
                    if(!V_BOOL(&v))
                        event_obj->prevent_default = TRUE;
                }else if(V_VT(&v) != VT_EMPTY) {
                    FIXME("unhandled result %s\n", debugstr_variant(&v));
                }
            }
            VariantClear(&v);
        }else {
            WARN("%s <<< %08x\n", debugstr_w(event_info[eid].name), hres);
        }
    }

    if(handler_vector && handler_vector->handler_cnt) {
        VARIANTARG arg;
        DISPPARAMS dp = {&arg, NULL, 1, 0};
        int i;

        V_VT(&arg) = VT_DISPATCH;
        V_DISPATCH(&arg) = (IDispatch*)&event_obj->dispex.IDispatchEx_iface;

        i = handler_vector->handler_cnt;
        while(i--) {
            if(handler_vector->handlers[i]) {
                V_VT(&v) = VT_EMPTY;

                TRACE("%s [%d] >>>\n", debugstr_w(event_info[eid].name), i);
                hres = call_disp_func(handler_vector->handlers[i], &dp, &v);
                if(hres == S_OK) {
                    TRACE("%s [%d] <<<\n", debugstr_w(event_info[eid].name), i);

                    if(cancelable) {
                        if(V_VT(&v) == VT_BOOL) {
                            if(!V_BOOL(&v))
                                event_obj->prevent_default = TRUE;
                        }else if(V_VT(&v) != VT_EMPTY) {
                            FIXME("unhandled result %s\n", debugstr_variant(&v));
                        }
                    }
                    VariantClear(&v);
                }else {
                    WARN("%s [%d] <<< %08x\n", debugstr_w(event_info[eid].name), i, hres);
                }
            }
        }
    }

    /*
     * NOTE: CP events may require doc_obj reference, which we don't own. We make sure that
     * it's safe to call event handler by checking nsevent_listener, which is NULL for
     * detached documents.
     */
    if(cp_container && cp_container->forward_container)
        cp_container = cp_container->forward_container;
    if(cp_container && cp_container->cps && doc->nsevent_listener) {
        ConnectionPoint *cp;
        unsigned i, j;

        for(j=0; cp_container->cp_entries[j].riid; j++) {
            cp = cp_container->cps + j;
            if(!cp->sinks_size || !is_cp_event(cp->data, event_info[eid].dispid))
                continue;

            for(i=0; doc->nsevent_listener && i < cp->sinks_size; i++) {
                if(!cp->sinks[i].disp)
                    continue;

                V_VT(&v) = VT_EMPTY;

                TRACE("cp %s [%u] >>>\n", debugstr_w(event_info[eid].name), i);
                hres = call_cp_func(cp->sinks[i].disp, event_info[eid].dispid,
                        cp->data->pass_event_arg ? event_obj : NULL, &v);
                if(hres == S_OK) {
                    TRACE("cp %s [%u] <<<\n", debugstr_w(event_info[eid].name), i);

                    if(cancelable) {
                        if(V_VT(&v) == VT_BOOL) {
                            if(!V_BOOL(&v))
                                event_obj->prevent_default = TRUE;
                        }else if(V_VT(&v) != VT_EMPTY) {
                            FIXME("unhandled result %s\n", debugstr_variant(&v));
                        }
                    }
                    VariantClear(&v);
                }else {
                    WARN("cp %s [%u] <<< %08x\n", debugstr_w(event_info[eid].name), i, hres);
                }
            }

            if(!doc->nsevent_listener)
                break;
        }
    }
}

static void fire_event_obj(HTMLDocumentNode *doc, eventid_t eid, HTMLEventObj *event_obj,
        HTMLDOMNode *target, IDispatch *script_this)
{
    IHTMLEventObj *prev_event;
    nsIDOMNode *parent, *nsnode;
    BOOL prevent_default = FALSE;
    HTMLInnerWindow *window;
    HTMLDOMNode *node;
    UINT16 node_type;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p) %s\n", doc, debugstr_w(event_info[eid].name));

    window = doc->window;
    if(!window) {
        WARN("NULL window\n");
        return;
    }

    htmldoc_addref(&doc->basedoc);

    prev_event = window->event;
    window->event = event_obj ? &event_obj->IHTMLEventObj_iface : NULL;

    nsIDOMNode_GetNodeType(target->nsnode, &node_type);
    nsnode = target->nsnode;
    nsIDOMNode_AddRef(nsnode);

    switch(node_type) {
    case ELEMENT_NODE:
        do {
            hres = get_node(doc, nsnode, FALSE, &node);
            if(SUCCEEDED(hres) && node) {
                call_event_handlers(doc, event_obj, &node->event_target, node->cp_container, eid,
                        script_this ? script_this : (IDispatch*)&node->IHTMLDOMNode_iface);
                node_release(node);
            }

            if(!(event_info[eid].flags & EVENT_BUBBLE) || (event_obj && event_obj->cancel_bubble))
                break;

            nsIDOMNode_GetParentNode(nsnode, &parent);
            nsIDOMNode_Release(nsnode);
            nsnode = parent;
            if(!nsnode)
                break;

            nsIDOMNode_GetNodeType(nsnode, &node_type);
        }while(node_type == ELEMENT_NODE);

        if(!(event_info[eid].flags & EVENT_BUBBLE) || (event_obj && event_obj->cancel_bubble))
            break;

    case DOCUMENT_NODE:
        if(event_info[eid].flags & EVENT_FORWARDBODY) {
            nsIDOMHTMLElement *nsbody;
            nsresult nsres;

            nsres = nsIDOMHTMLDocument_GetBody(doc->nsdoc, &nsbody);
            if(NS_SUCCEEDED(nsres) && nsbody) {
                hres = get_node(doc, (nsIDOMNode*)nsbody, FALSE, &node);
                if(SUCCEEDED(hres) && node) {
                    call_event_handlers(doc, event_obj, &node->event_target, node->cp_container, eid,
                            script_this ? script_this : (IDispatch*)&node->IHTMLDOMNode_iface);
                    node_release(node);
                }
                nsIDOMHTMLElement_Release(nsbody);
            }else {
                ERR("Could not get body: %08x\n", nsres);
            }
        }

        call_event_handlers(doc, event_obj, &doc->node.event_target, &doc->basedoc.cp_container, eid,
                script_this ? script_this : (IDispatch*)&doc->basedoc.IHTMLDocument2_iface);
        break;

    default:
        FIXME("unimplemented node type %d\n", node_type);
    }

    if(nsnode)
        nsIDOMNode_Release(nsnode);

    if(event_obj && event_obj->prevent_default)
        prevent_default = TRUE;
    window->event = prev_event;

    if(!prevent_default && (event_info[eid].flags & EVENT_HASDEFAULTHANDLERS)) {
        nsnode = target->nsnode;
        nsIDOMNode_AddRef(nsnode);

        do {
            hres = get_node(doc, nsnode, TRUE, &node);
            if(FAILED(hres))
                break;

            if(node) {
                if(node->vtbl->handle_event)
                    hres = node->vtbl->handle_event(node, eid, event_obj ? event_obj->nsevent : NULL, &prevent_default);
                node_release(node);
                if(FAILED(hres) || prevent_default || (event_obj && event_obj->cancel_bubble))
                    break;
            }

            nsres = nsIDOMNode_GetParentNode(nsnode, &parent);
            if(NS_FAILED(nsres))
                break;

            nsIDOMNode_Release(nsnode);
            nsnode = parent;
        } while(nsnode);

        if(nsnode)
            nsIDOMNode_Release(nsnode);
    }

    if(prevent_default && event_obj && event_obj->nsevent) {
        TRACE("calling PreventDefault\n");
        nsIDOMEvent_PreventDefault(event_obj->nsevent);
    }

    htmldoc_release(&doc->basedoc);
}

void fire_event(HTMLDocumentNode *doc, eventid_t eid, BOOL set_event, HTMLDOMNode *target, nsIDOMEvent *nsevent,
        IDispatch *script_this)
{
    HTMLEventObj *event_obj = NULL;
    HRESULT hres;

    if(set_event) {
        event_obj = create_event();
        if(!event_obj)
            return;

        hres = set_event_info(event_obj, target, eid, nsevent);
        if(FAILED(hres)) {
            IHTMLEventObj_Release(&event_obj->IHTMLEventObj_iface);
            return;
        }
    }

    fire_event_obj(doc, eid, event_obj, target, script_this);

    if(event_obj)
        IHTMLEventObj_Release(&event_obj->IHTMLEventObj_iface);
}

HRESULT dispatch_event(HTMLDOMNode *node, const WCHAR *event_name, VARIANT *event_var, VARIANT_BOOL *cancelled)
{
    HTMLEventObj *event_obj = NULL;
    eventid_t eid;
    HRESULT hres;

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

    if(event_obj) {
        hres = set_event_info(event_obj, node, eid, NULL);
        if(SUCCEEDED(hres))
            fire_event_obj(node->doc, eid, event_obj, node, NULL);

        IHTMLEventObj_Release(&event_obj->IHTMLEventObj_iface);
        if(FAILED(hres))
            return hres;
    }else {
        fire_event(node->doc, eid, TRUE, node, NULL, NULL);
    }

    *cancelled = VARIANT_TRUE; /* FIXME */
    return S_OK;
}

HRESULT call_fire_event(HTMLDOMNode *node, eventid_t eid)
{
    HRESULT hres;

    if(node->vtbl->fire_event) {
        BOOL handled = FALSE;

        hres = node->vtbl->fire_event(node, eid, &handled);
        if(handled)
            return hres;
    }

    fire_event(node->doc, eid, TRUE, node, NULL, NULL);
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

static void remove_event_handler(EventTarget *event_target, eventid_t eid)
{
    handler_vector_t *handler_vector;
    VARIANT *store;
    HRESULT hres;

    hres = get_event_dispex_ref(event_target, eid, FALSE, &store);
    if(SUCCEEDED(hres))
        VariantClear(store);

    handler_vector = get_handler_vector(event_target, eid, FALSE);
    if(handler_vector && handler_vector->handler_prop) {
        IDispatch_Release(handler_vector->handler_prop);
        handler_vector->handler_prop = NULL;
    }
}

static HRESULT set_event_handler_disp(EventTarget *event_target, eventid_t eid, IDispatch *disp)
{
    handler_vector_t *handler_vector;

    if(event_info[eid].flags & EVENT_FIXME)
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    remove_event_handler(event_target, eid);
    if(!disp)
        return S_OK;

    handler_vector = get_handler_vector(event_target, eid, TRUE);
    if(!handler_vector)
        return E_OUTOFMEMORY;

    if(handler_vector->handler_prop)
        IDispatch_Release(handler_vector->handler_prop);

    handler_vector->handler_prop = disp;
    IDispatch_AddRef(disp);
    return S_OK;
}

HRESULT set_event_handler(EventTarget *event_target, eventid_t eid, VARIANT *var)
{
    switch(V_VT(var)) {
    case VT_NULL:
        remove_event_handler(event_target, eid);
        return S_OK;

    case VT_DISPATCH:
        return set_event_handler_disp(event_target, eid, V_DISPATCH(var));

    case VT_BSTR: {
        VARIANT *v;
        HRESULT hres;

        /*
         * Setting event handler to string is a rare case and we don't want to
         * complicate nor increase memory of handler_vector_t for that. Instead,
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
        /* fall through */
    case VT_EMPTY:
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT get_event_handler(EventTarget *event_target, eventid_t eid, VARIANT *var)
{
    handler_vector_t *handler_vector;
    VARIANT *v;
    HRESULT hres;

    hres = get_event_dispex_ref(event_target, eid, FALSE, &v);
    if(SUCCEEDED(hres) && V_VT(v) != VT_EMPTY) {
        V_VT(var) = VT_EMPTY;
        return VariantCopy(var, v);
    }

    handler_vector = get_handler_vector(event_target, eid, FALSE);
    if(handler_vector && handler_vector->handler_prop) {
        V_VT(var) = VT_DISPATCH;
        V_DISPATCH(var) = handler_vector->handler_prop;
        IDispatch_AddRef(V_DISPATCH(var));
    }else {
        V_VT(var) = VT_NULL;
    }

    return S_OK;
}

HRESULT attach_event(EventTarget *event_target, BSTR name, IDispatch *disp, VARIANT_BOOL *res)
{
    handler_vector_t *handler_vector;
    eventid_t eid;
    DWORD i = 0;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        *res = VARIANT_TRUE;
        return S_OK;
    }

    if(event_info[eid].flags & EVENT_FIXME)
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    handler_vector = get_handler_vector(event_target, eid, TRUE);
    if(!handler_vector)
        return E_OUTOFMEMORY;

    while(i < handler_vector->handler_cnt && handler_vector->handlers[i])
        i++;
    if(i == handler_vector->handler_cnt) {
        if(i)
            handler_vector->handlers = heap_realloc_zero(handler_vector->handlers,
                                                         (i + 1) * sizeof(*handler_vector->handlers));
        else
            handler_vector->handlers = heap_alloc_zero(sizeof(*handler_vector->handlers));
        if(!handler_vector->handlers)
            return E_OUTOFMEMORY;
        handler_vector->handler_cnt++;
    }

    IDispatch_AddRef(disp);
    handler_vector->handlers[i] = disp;

    *res = VARIANT_TRUE;
    return S_OK;
}

HRESULT detach_event(EventTarget *event_target, BSTR name, IDispatch *disp)
{
    handler_vector_t *handler_vector;
    eventid_t eid;
    unsigned i;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        return S_OK;
    }

    handler_vector = get_handler_vector(event_target, eid, FALSE);
    if(!handler_vector)
        return S_OK;

    for(i = 0; i < handler_vector->handler_cnt; i++) {
        if(handler_vector->handlers[i] == disp) {
            IDispatch_Release(handler_vector->handlers[i]);
            handler_vector->handlers[i] = NULL;
        }
    }

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

        set_event_handler_disp(&node->event_target, eid, disp);
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

static int event_id_cmp(const void *key, const struct wine_rb_entry *entry)
{
    return (INT_PTR)key - WINE_RB_ENTRY_VALUE(entry, handler_vector_t, entry)->event_id;
}

void init_event_target(EventTarget *event_target)
{
    wine_rb_init(&event_target->handler_map, event_id_cmp);
}

void release_event_target(EventTarget *event_target)
{
    handler_vector_t *iter, *iter2;
    unsigned i;

    WINE_RB_FOR_EACH_ENTRY_DESTRUCTOR(iter, iter2, &event_target->handler_map, handler_vector_t, entry) {
        if(iter->handler_prop)
            IDispatch_Release(iter->handler_prop);
        for(i = 0; i < iter->handler_cnt; i++)
            if(iter->handlers[i])
                IDispatch_Release(iter->handlers[i]);
        heap_free(iter->handlers);
        heap_free(iter);
    }
}
