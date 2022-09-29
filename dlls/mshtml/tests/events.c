/*
 * Copyright 2008-2011 Jacek Caban for CodeWeavers
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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "mshtml.h"
#include "mshtmdid.h"
#include "docobj.h"
#include "hlink.h"
#include "dispex.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
    trace(#func "\n"); \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(document_onclick);
DEFINE_EXPECT(body_onclick);
DEFINE_EXPECT(doc_onclick_attached);
DEFINE_EXPECT(div_onclick);
DEFINE_EXPECT(div_onclick_attached);
DEFINE_EXPECT(div_onclick_capture);
DEFINE_EXPECT(div_onclick_bubble);
DEFINE_EXPECT(timeout);
DEFINE_EXPECT(doccp_onclick);
DEFINE_EXPECT(doccp2_onclick);
DEFINE_EXPECT(doccp_onclick_cancel);
DEFINE_EXPECT(div_onclick_disp);
DEFINE_EXPECT(invoke_onclick);
DEFINE_EXPECT(iframe_onreadystatechange_loading);
DEFINE_EXPECT(iframe_onreadystatechange_interactive);
DEFINE_EXPECT(iframe_onreadystatechange_complete);
DEFINE_EXPECT(iframedoc_onreadystatechange);
DEFINE_EXPECT(img_onload);
DEFINE_EXPECT(img_onerror);
DEFINE_EXPECT(link_onload);
DEFINE_EXPECT(input_onfocus);
DEFINE_EXPECT(input_onblur);
DEFINE_EXPECT(div_onfocusin);
DEFINE_EXPECT(div_onfocusout);
DEFINE_EXPECT(form_onsubmit);
DEFINE_EXPECT(form_onclick);
DEFINE_EXPECT(submit_onclick);
DEFINE_EXPECT(submit_onclick_cancel);
DEFINE_EXPECT(submit_onclick_attached);
DEFINE_EXPECT(submit_onclick_attached_check_cancel);
DEFINE_EXPECT(submit_onclick_setret);
DEFINE_EXPECT(elem2_cp_onclick);
DEFINE_EXPECT(iframe_onload);
DEFINE_EXPECT(doc1_onstorage);
DEFINE_EXPECT(doc1_onstoragecommit);
DEFINE_EXPECT(window1_onstorage);
DEFINE_EXPECT(doc2_onstorage);
DEFINE_EXPECT(doc2_onstoragecommit);
DEFINE_EXPECT(window2_onstorage);

static HWND container_hwnd = NULL;
static IHTMLWindow2 *window;
static IOleDocumentView *view;
static BOOL is_ie9plus;
static int document_mode;
static unsigned in_fire_event;

typedef struct {
    LONG x;
    LONG y;
    LONG offset_x;
    LONG offset_y;
} xy_test_t;

static const xy_test_t no_xy = {-10,-10,-10,-10};

static const char empty_doc_str[] =
    "<html></html>";

static const char empty_doc_ie9_str[] =
    "<html>"
    "<head><meta http-equiv=\"x-ua-compatible\" content=\"IE=9\" /></head>"
    "</html>";

static const char readystate_doc_str[] =
    "<html><body><iframe id=\"iframe\"></iframe></body></html>";

static const char readystate_doc_ie9_str[] =
    "<html>"
    "<head><meta http-equiv=\"x-ua-compatible\" content=\"IE=9\" /></head>"
    "<body><iframe id=\"iframe\"></iframe></body></html>";

static const char img_doc_str[] =
    "<html><body><img id=\"imgid\"></img></body></html>";

static const char link_doc_str[] =
    "<html><body><link id=\"linkid\" rel=\"stylesheet\" type=\"text/css\"></link></body></html>";

static const char input_doc_str[] =
    "<html><body><div id=\"divid\"><input id=\"inputid\"></input></div></body></html>";

static const char iframe_doc_str[] =
    "<html><body><iframe id=\"ifr\">Testing</iframe></body></html>";

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;

    if(iface1 == iface2)
        return TRUE;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_Release(unk1);
    IUnknown_QueryInterface(iface2, &IID_IUnknown, (void**)&unk2);
    IUnknown_Release(unk2);

    return unk1 == unk2;
}

#define test_disp(u,id) _test_disp(__LINE__,u,id)
static void _test_disp(unsigned line, IUnknown *unk, const IID *diid)
{
    IDispatchEx *dispex;
    ITypeInfo *typeinfo;
    UINT ticnt;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IDispatch: %08lx\n", hres);
    if(FAILED(hres))
        return;

    ticnt = 0xdeadbeef;
    hres = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfoCount failed: %08lx\n", hres);
    ok_(__FILE__,line) (ticnt == 1, "ticnt=%u\n", ticnt);

    hres = IDispatchEx_GetTypeInfo(dispex, 0, 0, &typeinfo);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfo failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        TYPEATTR *type_attr;

        hres = ITypeInfo_GetTypeAttr(typeinfo, &type_attr);
        ok_(__FILE__,line) (hres == S_OK, "GetTypeAttr failed: %08lx\n", hres);
        if(diid)
            ok_(__FILE__,line) (IsEqualGUID(&type_attr->guid, diid), "unexpected guid %s\n",
                                wine_dbgstr_guid(&type_attr->guid));

        ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
        ITypeInfo_Release(typeinfo);
    }

    IDispatchEx_Release(dispex);
}

#define get_doc3_iface(u) _get_doc3_iface(__LINE__,u)
static IHTMLDocument3 *_get_doc3_iface(unsigned line, IUnknown *unk)
{
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument3, (void**)&doc3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDocument3 iface: %08lx\n", hres);

    return doc3;
}

#define get_elem_iface(u) _get_elem_iface(__LINE__,u)
static IHTMLElement *_get_elem_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement iface: %08lx\n", hres);

    return elem;
}

#define get_elem2_iface(u) _get_elem2_iface(__LINE__,u)
static IHTMLElement2 *_get_elem2_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem2;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement2, (void**)&elem2);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement2 iface: %08lx\n", hres);

    return elem2;
}

#define get_elem3_iface(u) _get_elem3_iface(__LINE__,u)
static IHTMLElement3 *_get_elem3_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement3 *elem3;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement3, (void**)&elem3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement3 iface: %08lx\n", hres);

    return elem3;
}

#define get_elem4_iface(u) _get_elem4_iface(__LINE__,u)
static IHTMLElement4 *_get_elem4_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement4 *elem4;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement4, (void**)&elem4);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement4 iface: %08lx\n", hres);

    return elem4;
}

#define get_iframe_iface(u) _get_iframe_iface(__LINE__,u)
static IHTMLIFrameElement *_get_iframe_iface(unsigned line, IUnknown *unk)
{
    IHTMLIFrameElement *iframe;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLIFrameElement, (void**)&iframe);
    ok_(__FILE__,line)(hres == S_OK, "QueryInterface(IID_IHTMLIFrameElement) failed: %08lx\n", hres);

    return iframe;
}

#define doc_get_body(d) _doc_get_body(__LINE__,d)
static IHTMLElement *_doc_get_body(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLElement *body = NULL;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &body);
    ok_(__FILE__,line) (hres == S_OK, "get_body failed: %08lx\n", hres);
    ok_(__FILE__,line) (body != NULL, "body == NULL\n");

    return body;
}

#define get_elem_id(d,i) _get_elem_id(__LINE__,d,i)
static IHTMLElement *_get_elem_id(unsigned line, IHTMLDocument2 *doc, const WCHAR *id)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, (IUnknown*)doc);
    IHTMLElement *elem;
    BSTR str;
    HRESULT hres;

    str = SysAllocString(id);
    hres = IHTMLDocument3_getElementById(doc3, str, &elem);
    SysFreeString(str);
    IHTMLDocument3_Release(doc3);
    ok_(__FILE__,line) (hres == S_OK, "getElementById failed: %08lx\n", hres);
    ok_(__FILE__,line) (elem != NULL, "elem == NULL\n");

    return elem;
}

#define test_elem_tag(u,n) _test_elem_tag(__LINE__,u,n)
static void _test_elem_tag(unsigned line, IUnknown *unk, const WCHAR *extag)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR tag;
    HRESULT hres;

    hres = IHTMLElement_get_tagName(elem, &tag);
    IHTMLElement_Release(elem);
    ok_(__FILE__, line) (hres == S_OK, "get_tagName failed: %08lx\n", hres);
    ok_(__FILE__, line) (!lstrcmpW(tag, extag), "got tag: %s, expected %s\n", wine_dbgstr_w(tag), wine_dbgstr_w(extag));

    SysFreeString(tag);
}

#define get_event_obj() _get_event_obj(__LINE__)
static IHTMLEventObj *_get_event_obj(unsigned line)
{
    IHTMLEventObj *event = NULL;
    HRESULT hres;

    hres = IHTMLWindow2_get_event(window, &event);
    ok_(__FILE__,line) (hres == S_OK, "get_event failed: %08lx\n", hres);
    ok_(__FILE__,line) (event != NULL, "event = NULL\n");

    /* Native IE uses an undocumented DIID in IE9+ mode */
    _test_disp(line, (IUnknown*)event, document_mode < 9 ? &DIID_DispCEventObj : NULL);

    return event;
}

#define set_elem_innerhtml(e,t) _set_elem_innerhtml(__LINE__,e,t)
static void _set_elem_innerhtml(unsigned line, IHTMLElement *elem, const WCHAR *inner_html)
{
    BSTR html;
    HRESULT hres;

    html = SysAllocString(inner_html);
    hres = IHTMLElement_put_innerHTML(elem, html);
    ok_(__FILE__,line)(hres == S_OK, "put_innerHTML failed: %08lx\n", hres);
    SysFreeString(html);
}

#define elem_fire_event(a,b,c) _elem_fire_event(__LINE__,a,b,c)
static void _elem_fire_event(unsigned line, IUnknown *unk, const WCHAR *event, VARIANT *evobj)
{
    IHTMLElement3 *elem3 = _get_elem3_iface(line, unk);
    VARIANT_BOOL b;
    BSTR str;
    HRESULT hres;

    b = 100;
    str = SysAllocString(event);
    in_fire_event++;
    hres = IHTMLElement3_fireEvent(elem3, str, evobj, &b);
    in_fire_event--;
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "fireEvent failed: %08lx\n", hres);
    ok_(__FILE__,line)(b == VARIANT_TRUE, "fireEvent returned %x\n", b);
}

#define test_event_args(a,b,c,d,e,f,g) _test_event_args(__LINE__,a,b,c,d,e,f,g)
static void _test_event_args(unsigned line, const IID *dispiid, DISPID id, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok_(__FILE__,line) (id == DISPID_VALUE, "id = %ld\n", id);
    ok_(__FILE__,line) (wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok_(__FILE__,line) (pdp != NULL, "pdp == NULL\n");
    ok_(__FILE__,line) (pdp->cArgs == (document_mode < 9 ? 1 : 2), "pdp->cArgs = %d\n", pdp->cArgs);
    ok_(__FILE__,line) (pdp->cNamedArgs == 1, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok_(__FILE__,line) (pdp->rgdispidNamedArgs[0] == DISPID_THIS, "pdp->rgdispidNamedArgs[0] = %ld\n",
                        pdp->rgdispidNamedArgs[0]);
    ok_(__FILE__,line) (V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
    if(pdp->cArgs > 1)
        ok_(__FILE__,line) (V_VT(pdp->rgvarg+1) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg+1));
    ok_(__FILE__,line) (pvarRes != NULL, "pvarRes == NULL\n");
    ok_(__FILE__,line) (pei != NULL, "pei == NULL\n");
    ok_(__FILE__,line) (!pspCaller, "pspCaller != NULL\n");

    if(dispiid)
        _test_disp(line, (IUnknown*)V_DISPATCH(pdp->rgvarg), dispiid);

    if(pdp->cArgs > 1) {
        IHTMLEventObj *window_event, *event_obj;
        IDOMEvent *event;
        HRESULT hres;

        hres = IDispatch_QueryInterface(V_DISPATCH(pdp->rgvarg+1), &IID_IDOMEvent, (void**)&event);
        if(in_fire_event)
            ok(hres == E_NOINTERFACE, "QI(IID_IDOMEvent) returned %08lx\n", hres);
        else
            ok(hres == S_OK, "Could not get IDOMEvent iface: %08lx\n", hres);

        hres = IDispatch_QueryInterface(V_DISPATCH(pdp->rgvarg+1), &IID_IHTMLEventObj, (void**)&event_obj);
        if(in_fire_event)
            ok(hres == S_OK, "Could not get IDOMEventObj iface: %08lx\n", hres);
        else
            ok(hres == E_NOINTERFACE, "QI(IID_IHTMLEventObj) returned %08lx\n", hres);

        if(event)
            IDOMEvent_Release(event);
        if(event_obj)
            IHTMLEventObj_Release(event_obj);

        hres = IHTMLWindow2_get_event(window, &window_event);
        ok(hres == S_OK, "get_event failed: %08lx\n", hres);
        if(window_event) {
            todo_wine_if(in_fire_event)
            ok(!iface_cmp((IUnknown*)V_DISPATCH(pdp->rgvarg+1), (IUnknown*)window_event),
               "window_event != event arg\n");
            IHTMLEventObj_Release(window_event);
        }
    }
}

#define test_attached_event_args(a,b,c,d,e) _test_attached_event_args(__LINE__,a,b,c,d,e)
static void _test_attached_event_args(unsigned line, DISPID id, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei)
{
    IHTMLEventObj *event;

    ok(id == DISPID_VALUE, "id = %ld\n", id);
    ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok(pdp != NULL, "pDispParams == NULL\n");
    ok(pdp->cArgs == 1, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(!pdp->cNamedArgs, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok(!pdp->rgdispidNamedArgs, "pdp->rgdispidNamedArgs = %p\n", pdp->rgdispidNamedArgs);
    ok(pdp->rgvarg != NULL, "rgvarg = NULL\n");
    ok(pvarRes != NULL, "pvarRes = NULL\n");
    ok(pei != NULL, "pei = NULL\n");
    ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pVarRes) = %d\n", V_VT(pvarRes));
    ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
    ok(V_DISPATCH(pdp->rgvarg) != NULL, "V_DISPATCH(pdp->rgvarg) = %p\n", V_DISPATCH(pdp->rgvarg));

    event = _get_event_obj(line);
    todo_wine
    ok(!iface_cmp((IUnknown*)event, (IUnknown*)V_DISPATCH(pdp->rgvarg)), "event != arg0\n");
    IHTMLEventObj_Release(event);
}

#define get_event_src() _get_event_src(__LINE__)
static IHTMLElement *_get_event_src(unsigned line)
{
    IHTMLEventObj *event = _get_event_obj(line);
    IHTMLElement *src_elem = NULL;
    HRESULT hres;

    hres = IHTMLEventObj_get_srcElement(event, &src_elem);
    IHTMLEventObj_Release(event);
    ok_(__FILE__,line) (hres == S_OK, "get_srcElement failed: %08lx\n", hres);

    return src_elem;
}

#define test_event_src(t) _test_event_src(__LINE__,t)
static void _test_event_src(unsigned line, const WCHAR *src_tag)
{
    IHTMLElement *src_elem = _get_event_src(line);

    if(src_tag) {
        ok_(__FILE__,line) (src_elem != NULL, "src_elem = NULL\n");
        _test_elem_tag(line, (IUnknown*)src_elem, src_tag);
        IHTMLElement_Release(src_elem);
    }else {
        ok_(__FILE__,line) (!src_elem, "src_elem != NULL\n");
    }
}

static void _test_event_altkey(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_altKey(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_altKey failed: %08lx\n", hres);
    ok_(__FILE__,line)(b == exval, "altKey = %x, expected %x\n", b, exval);
}

static void _test_event_ctrlkey(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_ctrlKey(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_ctrlKey failed: %08lx\n", hres);
    ok_(__FILE__,line)(b == exval, "ctrlKey = %x, expected %x\n", b, exval);
}

static void _test_event_shiftkey(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_shiftKey(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_shiftKey failed: %08lx\n", hres);
    ok_(__FILE__,line)(b == exval, "shiftKey = %x, expected %x\n", b, exval);
}

#define test_event_cancelbubble(a,b) _test_event_cancelbubble(__LINE__,a,b)
static void _test_event_cancelbubble(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_cancelBubble(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_cancelBubble failed: %08lx\n", hres);
    ok_(__FILE__,line)(b == exval, "cancelBubble = %x, expected %x\n", b, exval);
}

static void _test_event_fromelem(unsigned line, IHTMLEventObj *event, const WCHAR *from_tag)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLEventObj_get_fromElement(event, &elem);
    ok_(__FILE__,line)(hres == S_OK, "get_fromElement failed: %08lx\n", hres);
    if(from_tag)
        _test_elem_tag(line, (IUnknown*)elem, from_tag);
    else
        ok_(__FILE__,line)(elem == NULL, "fromElement != NULL\n");
    if(elem)
        IHTMLElement_Release(elem);
}

static void _test_event_toelem(unsigned line, IHTMLEventObj *event, const WCHAR *to_tag)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLEventObj_get_toElement(event, &elem);
    ok_(__FILE__,line)(hres == S_OK, "get_toElement failed: %08lx\n", hres);
    if(to_tag)
        _test_elem_tag(line, (IUnknown*)elem, to_tag);
    else
        ok_(__FILE__,line)(elem == NULL, "toElement != NULL\n");
    if(elem)
        IHTMLElement_Release(elem);
}

static void _test_event_keycode(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_keyCode(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_keyCode failed: %08lx\n", hres);
    ok_(__FILE__,line)(l == exl, "keyCode = %lx, expected %lx\n", l, exl);
}

static void _test_event_button(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_button(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_button failed: %08lx\n", hres);
    ok_(__FILE__,line)(l == exl, "button = %lx, expected %lx\n", l, exl);
}

static void _test_event_reason(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_reason(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_reason failed: %08lx\n", hres);
    ok_(__FILE__,line)(l == exl, "reason = %lx, expected %lx\n", l, exl);
}

static void _test_event_x(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_x(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_x failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "x = %ld, expected %ld\n", l, exl);
}

static void _test_event_y(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_y(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_y failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "y = %ld, expected %ld\n", l, exl);
}

static void _test_event_clientx(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_clientX(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_clientX failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "clientX = %ld, expected %ld\n", l, exl);
}

static void _test_event_clienty(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_clientY(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_clientY failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "clientY = %ld, expected %ld\n", l, exl);
}

static void _test_event_offsetx(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_offsetX(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_offsetX failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "offsetX = %ld, expected %ld\n", l, exl);
}

static void _test_event_offsety(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_offsetY(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_offsetY failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "offsetY = %ld, expected %ld\n", l, exl);
}

static void _test_event_screenx(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_screenX(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_screenX failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "screenX = %ld, expected %ld\n", l, exl);
}

static void _test_event_screeny(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_screenY(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_screenY failed: %08lx\n", hres);
    if(exl != -10) /* don't test the exact value for -10 */
        ok_(__FILE__,line)(l == exl, "screenY = %ld, expected %ld\n", l, exl);
}

static void _test_event_type(unsigned line, IHTMLEventObj *event, const WCHAR *exstr)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLEventObj_get_type(event, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_type failed: %08lx\n", hres);
    ok_(__FILE__,line)(!lstrcmpW(str, exstr), "type = %s, expected %s\n", wine_dbgstr_w(str), wine_dbgstr_w(exstr));
}

static void _test_event_qualifier(unsigned line, IHTMLEventObj *event, const WCHAR *exstr)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLEventObj_get_qualifier(event, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_qualifier failed: %08lx\n", hres);
    if(exstr)
        ok_(__FILE__,line)(!lstrcmpW(str, exstr), "qualifier = %s, expected %s\n", wine_dbgstr_w(str),
                           wine_dbgstr_w(exstr));
    else
        ok_(__FILE__,line)(!str, "qualifier != NULL\n");
}

static void _test_event_srcfilter(unsigned line, IHTMLEventObj *event)
{
    IDispatch *disp;
    HRESULT hres;

    hres = IHTMLEventObj_get_srcFilter(event, &disp);
    ok_(__FILE__,line)(hres == S_OK, "get_srcFilter failed: %08lx\n", hres);
    ok_(__FILE__,line)(!disp, "srcFilter != NULL\n");
}

#define test_event_obj(t,x) _test_event_obj(__LINE__,t,x)
static void _test_event_obj(unsigned line, const WCHAR *type, const xy_test_t *xy)
{
    IHTMLEventObj *event = _get_event_obj(line);
    IDOMEvent *dom_event;
    VARIANT v;
    HRESULT hres;

    _test_event_altkey(line, event, VARIANT_FALSE);
    _test_event_ctrlkey(line, event, VARIANT_FALSE);
    _test_event_shiftkey(line, event, VARIANT_FALSE);
    _test_event_cancelbubble(line, event, VARIANT_FALSE);
    _test_event_fromelem(line, event, NULL);
    _test_event_toelem(line, event, NULL);
    _test_event_keycode(line, event, 0);
    _test_event_button(line, event, 0);
    _test_event_type(line, event, type);
    _test_event_qualifier(line, event, NULL);
    _test_event_reason(line, event, 0);
    _test_event_srcfilter(line, event);
    _test_event_x(line, event, xy->x);
    _test_event_y(line, event, xy->y);
    _test_event_clientx(line, event, -10);
    _test_event_clienty(line, event, -10);
    _test_event_offsetx(line, event, xy->offset_x);
    _test_event_offsety(line, event, xy->offset_y);
    _test_event_screenx(line, event, -10);
    _test_event_screeny(line, event, -10);

    V_VT(&v) = VT_NULL;
    hres = IHTMLEventObj_get_returnValue(event, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_returnValue failed: %08lx\n", hres);
    /* Depending on source of event, returnValue may be true bool in IE9+ mode */
    ok_(__FILE__,line)(V_VT(&v) == VT_EMPTY || (document_mode >= 9 && V_VT(&v) == VT_BOOL),
                       "V_VT(returnValue) = %d\n", V_VT(&v));
    if(V_VT(&v) == VT_BOOL)
        ok_(__FILE__,line)(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(returnValue) = %x\n", V_BOOL(&v));

    hres = IHTMLEventObj_QueryInterface(event, &IID_IDOMEvent, (void**)&dom_event);
    ok(hres == E_NOINTERFACE, "Could not get IDOMEvent iface: %08lx\n", hres);

    IHTMLEventObj_Release(event);
}

#define elem_attach_event(a,b,c) _elem_attach_event(__LINE__,a,b,c)
static void _elem_attach_event(unsigned line, IUnknown *unk, const WCHAR *namew, IDispatch *disp)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    VARIANT_BOOL res;
    BSTR name;
    HRESULT hres;

    name = SysAllocString(namew);
    hres = IHTMLElement2_attachEvent(elem, name, disp, &res);
    IHTMLElement2_Release(elem);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "attachEvent failed: %08lx\n", hres);
    ok_(__FILE__,line)(res == VARIANT_TRUE, "attachEvent returned %x\n", res);
}

#define add_event_listener(a,b,c,d) _add_event_listener(__LINE__,a,b,c,d)
static void _add_event_listener(unsigned line, IUnknown *unk, const WCHAR *type, IDispatch *listener, VARIANT_BOOL use_capture)
{
    IEventTarget *event_target;
    BSTR str;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IEventTarget, (void**)&event_target);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IEventTarget iface: %08lx\n", hres);

    str = SysAllocString(type);
    hres = IEventTarget_addEventListener(event_target, str, listener, use_capture);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "addEventListener failed: %08lx\n", hres);

    IEventTarget_Release(event_target);
}

#define remove_event_listener(a,b,c,d) _remove_event_listener(__LINE__,a,b,c,d)
static void _remove_event_listener(unsigned line, IUnknown *unk, const WCHAR *type, IDispatch *listener, VARIANT_BOOL use_capture)
{
    IEventTarget *event_target;
    BSTR str;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IEventTarget, (void**)&event_target);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IEventTarget iface: %08lx\n", hres);

    str = SysAllocString(type);
    hres = IEventTarget_removeEventListener(event_target, str, listener, use_capture);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "removeEventListener failed: %08lx\n", hres);

    IEventTarget_Release(event_target);
}

#define elem_detach_event(a,b,c) _elem_detach_event(__LINE__,a,b,c)
static void _elem_detach_event(unsigned line, IUnknown *unk, const WCHAR *namew, IDispatch *disp)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    BSTR name;
    HRESULT hres;

    name = SysAllocString(namew);
    hres = IHTMLElement2_detachEvent(elem, name, disp);
    IHTMLElement2_Release(elem);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "detachEvent failed: %08lx\n", hres);
}

#define doc_attach_event(a,b,c) _doc_attach_event(__LINE__,a,b,c)
static void _doc_attach_event(unsigned line, IHTMLDocument2 *doc, const WCHAR *namew, IDispatch *disp)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, (IUnknown*)doc);
    VARIANT_BOOL res;
    BSTR name;
    HRESULT hres;

    name = SysAllocString(namew);
    hres = IHTMLDocument3_attachEvent(doc3, name, disp, &res);
    IHTMLDocument3_Release(doc3);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "attachEvent failed: %08lx\n", hres);
    ok_(__FILE__,line)(res == VARIANT_TRUE, "attachEvent returned %x\n", res);
}

#define doc_detach_event(a,b,c) _doc_detach_event(__LINE__,a,b,c)
static void _doc_detach_event(unsigned line, IHTMLDocument2 *doc, const WCHAR *namew, IDispatch *disp)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, (IUnknown*)doc);
    BSTR name;
    HRESULT hres;

    name = SysAllocString(namew);
    hres = IHTMLDocument3_detachEvent(doc3, name, disp);
    IHTMLDocument3_Release(doc3);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "detachEvent failed: %08lx\n", hres);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static HRESULT WINAPI Dispatch_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
     if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = iface;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call %s %lx\n", wine_dbgstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

#define EVENT_HANDLER_FUNC_OBJ(event) \
    static IDispatchExVtbl event ## FuncVtbl = { \
        DispatchEx_QueryInterface, \
        DispatchEx_AddRef, \
        DispatchEx_Release, \
        DispatchEx_GetTypeInfoCount, \
        DispatchEx_GetTypeInfo, \
        DispatchEx_GetIDsOfNames, \
        DispatchEx_Invoke, \
        DispatchEx_GetDispID, \
        event, \
        DispatchEx_DeleteMemberByName, \
        DispatchEx_DeleteMemberByDispID, \
        DispatchEx_GetMemberProperties, \
        DispatchEx_GetMemberName, \
        DispatchEx_GetNextDispID, \
        DispatchEx_GetNameSpaceParent \
    }; \
    static IDispatchEx event ## _obj = { &event ## FuncVtbl };

static HRESULT WINAPI document_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLDocument3 *doc3;
    CHECK_EXPECT(document_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    doc3 = get_doc3_iface((IUnknown*)V_DISPATCH(pdp->rgvarg));
    IHTMLDocument3_Release(doc3);
    test_event_src(L"DIV");
    test_event_obj(L"click", &no_xy);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(document_onclick);

static HRESULT WINAPI div_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"DIV");
    test_event_obj(L"click", &no_xy);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onclick);

static HRESULT WINAPI div_onclick_attached(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onclick_attached);

    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src(L"DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onclick_attached);

static HRESULT WINAPI doc_onclick_attached(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(doc_onclick_attached);

    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src(L"DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(doc_onclick_attached);

static HRESULT WINAPI body_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(body_onclick);
    /* Native IE returns undocumented DIID in IE9+ mode */
    test_event_args(document_mode < 9 ? &DIID_DispHTMLBody : NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(body_onclick);

static HRESULT WINAPI div_onclick_capture(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onclick_capture);

    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onclick_capture);

static HRESULT WINAPI div_onclick_bubble(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onclick_bubble);

    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onclick_bubble);

static HRESULT WINAPI img_onload(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(img_onload);
    test_event_args(&DIID_DispHTMLImg, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"IMG");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(img_onload);

static HRESULT WINAPI link_onload(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(link_onload);
    test_event_args(&DIID_DispHTMLLinkElement, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"LINK");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(link_onload);

static HRESULT WINAPI unattached_img_onload(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLElement *event_src;

    CHECK_EXPECT(img_onload);

    test_event_args(&DIID_DispHTMLImg, id, wFlags, pdp, pvarRes, pei, pspCaller);
    event_src = get_event_src();
    todo_wine
        ok(!event_src, "event_src != NULL\n");
    if(event_src)
        IHTMLElement_Release(event_src);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(unattached_img_onload);

static HRESULT WINAPI img_onerror(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(img_onerror);
    test_event_args(&DIID_DispHTMLImg, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"IMG");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(img_onerror);

static HRESULT WINAPI input_onfocus(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(input_onfocus);
    test_event_args(&DIID_DispHTMLInputElement, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(input_onfocus);

static HRESULT WINAPI div_onfocusin(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onfocusin);
    test_event_args(NULL /* FIXME: &DIID_DispHTMLDivElement */, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onfocusin);

static HRESULT WINAPI div_onfocusout(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onfocusout);
    test_event_args(NULL /* FIXME: &DIID_DispHTMLDivElement */, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onfocusout);

static HRESULT WINAPI input_onblur(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(input_onblur);
    test_event_args(&DIID_DispHTMLInputElement, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(input_onblur);

static HRESULT WINAPI form_onsubmit(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(form_onsubmit);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"FORM");

    V_VT(pvarRes) = VT_BOOL;
    V_BOOL(pvarRes) = VARIANT_FALSE;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(form_onsubmit);

static HRESULT WINAPI form_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(form_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);

    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(form_onclick);

static HRESULT WINAPI submit_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(submit_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");

    V_VT(pvarRes) = VT_BOOL;
    V_BOOL(pvarRes) = VARIANT_FALSE;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick);

static HRESULT WINAPI iframe_onload(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(iframe_onload);
    test_event_args(&DIID_DispHTMLIFrame, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"IFRAME");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(iframe_onload);

static HRESULT WINAPI submit_onclick_attached(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(submit_onclick_attached);
    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src(L"INPUT");

    V_VT(pvarRes) = VT_BOOL;
    V_BOOL(pvarRes) = VARIANT_FALSE;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_attached);

static HRESULT WINAPI submit_onclick_attached_check_cancel(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event;
    HRESULT hres;

    CHECK_EXPECT(submit_onclick_attached_check_cancel);
    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src(L"INPUT");

    event = NULL;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08lx\n", hres);
    ok(event != NULL, "event == NULL\n");

    test_event_cancelbubble(event, VARIANT_TRUE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_attached_check_cancel);

static VARIANT onclick_retval, onclick_event_retval;

static HRESULT WINAPI submit_onclick_setret(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event;
    VARIANT v;
    HRESULT hres;

    CHECK_EXPECT(submit_onclick_setret);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");

    event = NULL;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08lx\n", hres);
    ok(event != NULL, "event == NULL\n");

    V_VT(&v) = VT_ERROR;
    hres = IHTMLEventObj_get_returnValue(event, &v);
    ok(hres == S_OK, "get_returnValue failed: %08lx\n", hres);
    if(document_mode < 9) {
        ok(V_VT(&v) == VT_EMPTY, "V_VT(returnValue) = %d\n", V_VT(&v));
    }else todo_wine {
        ok(V_VT(&v) == VT_BOOL, "V_VT(returnValue) = %d\n", V_VT(&v));
        ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(returnValue) = %x\n", V_BOOL(&v));
    }

    hres = IHTMLEventObj_put_returnValue(event, onclick_event_retval);
    ok(hres == S_OK, "put_returnValue failed: %08lx\n", hres);

    V_VT(&v) = VT_ERROR;
    hres = IHTMLEventObj_get_returnValue(event, &v);
    ok(hres == S_OK, "get_returnValue failed: %08lx\n", hres);
    ok(VarCmp(&v, &onclick_event_retval, 0, 0) == VARCMP_EQ, "unexpected returnValue\n");

    IHTMLEventObj_Release(event);
    *pvarRes = onclick_retval;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_setret);

static HRESULT WINAPI submit_onclick_cancel(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event;
    HRESULT hres;

    CHECK_EXPECT(submit_onclick_cancel);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"INPUT");

    event = NULL;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08lx\n", hres);
    ok(event != NULL, "event == NULL\n");

    test_event_cancelbubble(event, VARIANT_FALSE);

    hres = IHTMLEventObj_put_cancelBubble(event, VARIANT_TRUE);
    ok(hres == S_OK, "put_returnValue failed: %08lx\n", hres);

    test_event_cancelbubble(event, VARIANT_TRUE);
    IHTMLEventObj_Release(event);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_cancel);

static HRESULT WINAPI iframedoc_onreadystatechange(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event = NULL;
    HRESULT hres;

    CHECK_EXPECT2(iframedoc_onreadystatechange);
    test_event_args(document_mode < 9 ? &DIID_DispHTMLDocument : NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);

    event = (void*)0xdeadbeef;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08lx\n", hres);
    ok(!event, "event = %p\n", event);

    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(iframedoc_onreadystatechange);

static HRESULT WINAPI iframe_onreadystatechange(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLWindow2 *iframe_window;
    IHTMLDocument2 *iframe_doc;
    IHTMLFrameBase2 *iframe;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str, str2;
    HRESULT hres;

    test_event_args(document_mode < 9 ? &DIID_DispHTMLIFrame : NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src(L"IFRAME");

    elem = get_event_src();
    elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_readyState(elem2, &v);
    ok(hres == S_OK, "get_readyState failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(readyState) = %d\n", V_VT(&v));

    hres = IHTMLElement2_QueryInterface(elem2, &IID_IHTMLFrameBase2, (void**)&iframe);
    IHTMLElement2_Release(elem2);
    ok(hres == S_OK, "Could not get IHTMLFrameBase2 iface: %08lx\n", hres);

    str = NULL;
    hres = IHTMLFrameBase2_get_readyState(iframe, &str);
    ok(hres == S_OK, "get_readyState failed: %08lx\n", hres);
    ok(str != NULL, "readyState == NULL\n");
    ok(!lstrcmpW(str, V_BSTR(&v)), "ready states differ\n");
    VariantClear(&v);

    hres = IHTMLFrameBase2_get_contentWindow(iframe, &iframe_window);
    ok(hres == S_OK, "get_contentDocument failed: %08lx\n", hres);

    hres = IHTMLWindow2_get_document(iframe_window, &iframe_doc);
    IHTMLWindow2_Release(iframe_window);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);

    hres = IHTMLDocument2_get_readyState(iframe_doc, &str2);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);
    ok(!lstrcmpW(str, str2), "unexpected document readyState %s\n", wine_dbgstr_w(str2));
    SysFreeString(str2);

    if(!lstrcmpW(str, L"loading")) {
        CHECK_EXPECT(iframe_onreadystatechange_loading);

        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = (IDispatch*)&iframedoc_onreadystatechange_obj;
        hres = IHTMLDocument2_put_onreadystatechange(iframe_doc, v);
        ok(hres == S_OK, "put_onreadystatechange: %08lx\n", hres);
    }else if(!lstrcmpW(str, L"interactive"))
        CHECK_EXPECT(iframe_onreadystatechange_interactive);
    else if(!lstrcmpW(str, L"complete"))
        CHECK_EXPECT(iframe_onreadystatechange_complete);
    else
        ok(0, "unexpected state %s\n", wine_dbgstr_w(str));

    SysFreeString(str);
    IHTMLDocument2_Release(iframe_doc);
    IHTMLFrameBase2_Release(iframe);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(iframe_onreadystatechange);

static HRESULT WINAPI nocall(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(nocall);

#define CONNECTION_POINT_OBJ(cpname, diid) \
    static HRESULT WINAPI cpname ## _QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv) \
    { \
        *ppv = NULL; \
        if(IsEqualGUID(riid, &IID_IUnknown) \
           || IsEqualGUID(riid, &IID_IDispatch) \
           || IsEqualGUID(riid, &diid)) \
            *ppv = iface; \
        else { \
            ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid)); \
            return E_NOINTERFACE; \
        } \
        return S_OK; \
    } \
    static IDispatchExVtbl cpname ## Vtbl = { \
        cpname ## _QueryInterface, \
        DispatchEx_AddRef,  \
        DispatchEx_Release, \
        DispatchEx_GetTypeInfoCount, \
        DispatchEx_GetTypeInfo, \
        DispatchEx_GetIDsOfNames, \
        cpname, \
        DispatchEx_GetDispID, \
        DispatchEx_InvokeEx, \
        DispatchEx_DeleteMemberByName, \
        DispatchEx_DeleteMemberByDispID, \
        DispatchEx_GetMemberProperties, \
        DispatchEx_GetMemberName, \
        DispatchEx_GetNextDispID, \
        DispatchEx_GetNameSpaceParent \
    }; \
    static IDispatchEx cpname ## _obj = { &cpname ## Vtbl }

#define test_cp_args(a,b,c,d,e,f,g) _test_cp_args(__LINE__,a,b,c,d,e,f,g)
static void _test_cp_args(unsigned line, REFIID riid, WORD flags, DISPPARAMS *dp, VARIANT *vres, EXCEPINFO *ei, UINT *argerr, BOOL use_events2)
{
    ok_(__FILE__,line)(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok_(__FILE__,line)(flags == DISPATCH_METHOD, "flags = %x\n", flags);
    ok_(__FILE__,line)(dp != NULL, "dp == NULL\n");
    if (use_events2)
    {
        IHTMLEventObj *event;
        HRESULT hr;
        ok_(__FILE__,line)(dp->cArgs == 1, "dp->cArgs = %d\n", dp->cArgs);
        ok_(__FILE__,line)(dp->rgvarg != NULL, "dp->rgvarg = %p\n", dp->rgvarg);
        ok_(__FILE__,line)(V_VT(&dp->rgvarg[0]) == VT_DISPATCH, "vt = %d\n", V_VT(&dp->rgvarg[0]));
        hr = IDispatch_QueryInterface(V_DISPATCH(&dp->rgvarg[0]), &IID_IHTMLEventObj, (void **)&event);
        ok_(__FILE__,line)(hr == S_OK, "Could not get IHTMLEventObj iface: %08lx\n", hr);
        IHTMLEventObj_Release(event);
    }
    else
    {
        ok_(__FILE__,line)(!dp->cArgs, "dp->cArgs = %d\n", dp->cArgs);
        ok_(__FILE__,line)(!dp->rgvarg, "dp->rgvarg = %p\n", dp->rgvarg);
    }
    ok_(__FILE__,line)(!dp->cNamedArgs, "dp->cNamedArgs = %d\n", dp->cNamedArgs);
    ok_(__FILE__,line)(!dp->rgdispidNamedArgs, "dp->rgdispidNamedArgs = %p\n", dp->rgdispidNamedArgs);
    ok_(__FILE__,line)(vres != NULL, "vres == NULL\n");
    ok_(__FILE__,line)(V_VT(vres) == VT_EMPTY, "V_VT(vres) = %d\n", V_VT(vres));
    ok_(__FILE__,line)(ei != NULL, "ei == NULL\n");
    ok_(__FILE__,line)(argerr != NULL, "argerr == NULL\n");
}

#define test_cp_eventarg(a,b,c,d,e,f) _test_cp_eventarg(__LINE__,a,b,c,d,e,f)
static void _test_cp_eventarg(unsigned line, REFIID riid, WORD flags, DISPPARAMS *dp, VARIANT *vres, EXCEPINFO *ei, UINT *argerr)
{
    IHTMLEventObj *event;

    ok_(__FILE__,line)(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok_(__FILE__,line)(flags == DISPATCH_METHOD, "flags = %x\n", flags);
    ok_(__FILE__,line)(dp != NULL, "dp == NULL\n");
    ok_(__FILE__,line)(dp->cArgs == 1, "dp->cArgs = %d\n", dp->cArgs);
    ok_(__FILE__,line)(dp->rgvarg != NULL, "dp->rgvarg = %p\n", dp->rgvarg);
    ok_(__FILE__,line)(!dp->cNamedArgs, "dp->cNamedArgs = %d\n", dp->cNamedArgs);
    ok_(__FILE__,line)(!dp->rgdispidNamedArgs, "dp->rgdispidNamedArgs = %p\n", dp->rgdispidNamedArgs);
    ok_(__FILE__,line)(vres != NULL, "vres == NULL\n");
    ok_(__FILE__,line)(V_VT(vres) == VT_EMPTY, "V_VT(vres) = %d\n", V_VT(vres));
    ok_(__FILE__,line)(ei != NULL, "ei == NULL\n");
    ok_(__FILE__,line)(argerr != NULL, "argerr == NULL\n");

    ok(V_VT(dp->rgvarg) == VT_DISPATCH, "V_VT(dp->rgvarg) = %d\n", V_VT(dp->rgvarg));
    ok(V_DISPATCH(dp->rgvarg) != NULL, "V_DISPATCH(dp->rgvarg) = %p\n", V_DISPATCH(dp->rgvarg));

    event = _get_event_obj(line);
    todo_wine
    ok(!iface_cmp((IUnknown*)event, (IUnknown*)V_DISPATCH(dp->rgvarg)), "event != arg0\n");
    IHTMLEventObj_Release(event);
}

static HRESULT WINAPI doccp(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                            VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(doccp_onclick);
        test_cp_args(riid, wFlags, pdp, pVarResult, pei, puArgErr, FALSE);
        break;
    default:
        ok(0, "unexpected call %ld\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(doccp, DIID_HTMLDocumentEvents);

static HRESULT WINAPI doccp2(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                            VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(doccp2_onclick);
        test_cp_args(riid, wFlags, pdp, pVarResult, pei, puArgErr, TRUE);
        break;
    default:
        ok(0, "unexpected call %ld\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(doccp2, DIID_HTMLDocumentEvents2);

static HRESULT WINAPI doccp_onclick_cancel(IDispatchEx *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(doccp_onclick_cancel);
        test_cp_args(riid, wFlags, pdp, pVarResult, pei, puArgErr, FALSE);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        break;
    default:
        ok(0, "unexpected call %ld\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(doccp_onclick_cancel, DIID_HTMLDocumentEvents);

static HRESULT WINAPI elem2_cp(IDispatchEx *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(elem2_cp_onclick);
        test_cp_eventarg(riid, wFlags, pdp, pVarResult, pei, puArgErr);
        break;
    default:
        ok(0, "unexpected call %ld\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(elem2_cp, DIID_HTMLElementEvents2);

static HRESULT WINAPI timeoutFunc_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    CHECK_EXPECT(timeout);

    ok(dispIdMember == DISPID_VALUE, "dispIdMember = %ld\n", dispIdMember);
    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok(!lcid, "lcid = %lx\n", lcid);
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(!pDispParams->cArgs, "pdp->cArgs = %d\n", pDispParams->cArgs);
    ok(!pDispParams->cNamedArgs, "pdp->cNamedArgs = %d\n", pDispParams->cNamedArgs);
    ok(!pDispParams->rgdispidNamedArgs, "pdp->rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
    ok(!pDispParams->rgvarg, "rgvarg = %p\n", pDispParams->rgvarg);
    ok(pVarResult != NULL, "pVarResult = NULL\n");
    ok(pExcepInfo != NULL, "pExcepInfo = NULL\n");
    ok(!puArgErr, "puArgErr = %p\n", puArgErr);
    ok(V_VT(pVarResult) == VT_EMPTY, "V_VT(pVarResult) = %d\n", V_VT(pVarResult));

    return S_OK;
}

static const IDispatchExVtbl timeoutFuncVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    timeoutFunc_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx timeoutFunc = { &timeoutFuncVtbl };

static HRESULT WINAPI timeoutFunc2_Invoke(IDispatchEx *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_FAIL;
}

static const IDispatchExVtbl timeoutFunc2Vtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    timeoutFunc2_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx timeoutFunc2 = { &timeoutFunc2Vtbl };

static HRESULT WINAPI div_onclick_disp_Invoke(IDispatchEx *iface, DISPID id,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, UINT *puArgErr)
{
    CHECK_EXPECT(div_onclick_disp);

    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);

    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(!puArgErr, "puArgErr = %p\n", puArgErr);

    return S_OK;
}

static const IDispatchExVtbl div_onclick_dispVtbl = {
    Dispatch_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    div_onclick_disp_Invoke,
};

static IDispatchEx div_onclick_disp = { &div_onclick_dispVtbl };

static void pump_msgs(BOOL *b)
{
    MSG msg;

    if(b) {
        while(!*b && GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }else {
        while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

static IConnectionPoint *get_cp(IUnknown *unk, REFIID riid)
{
    IConnectionPointContainer *cp_container;
    IConnectionPoint *cp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&cp_container);
    ok(hres == S_OK, "Could not get IConnectionPointContainer: %08lx\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(cp_container, riid, &cp);
    IConnectionPointContainer_Release(cp_container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);

    return cp;
}

static DWORD register_cp(IUnknown *unk, REFIID riid, IUnknown *sink)
{
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    cp = get_cp(unk, riid);
    hres = IConnectionPoint_Advise(cp, sink, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);

    return cookie;
}

static void unregister_cp(IUnknown *unk, REFIID riid, DWORD cookie)
{
    IConnectionPoint *cp;
    HRESULT hres;

    cp = get_cp(unk, riid);
    hres = IConnectionPoint_Unadvise(cp, cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);
}

static HRESULT WINAPI EventDispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "Unexpected call\n");
    return E_NOINTERFACE;
}

static DWORD WINAPI EventDispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static DWORD WINAPI EventDispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI EventDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EventDispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EventDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EventDispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo != NULL, "pExcepInfo == NULL\n");
    ok(puArgErr != NULL, "puArgErr == NULL\n");
    ok(V_VT(pVarResult) == 0, "V_VT(pVarResult) = %d\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_METHOD, "wFlags = %d\n", wFlags);

    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT2(invoke_onclick);
        break;
    case DISPID_HTMLDOCUMENTEVENTS2_ONPROPERTYCHANGE:
    case DISPID_HTMLDOCUMENTEVENTS2_ONREADYSTATECHANGE:
    case 1027:
    case 1034:
    case 1037:
    case 1044:
    case 1045:
    case 1047:
    case 1048:
    case 1049:
        break; /* TODO */
    default:
        ok(0, "Unexpected call: %ld\n", dispIdMember);
    }

    return S_OK;
}

static const IDispatchVtbl EventDispatchVtbl = {
    EventDispatch_QueryInterface,
    EventDispatch_AddRef,
    EventDispatch_Release,
    EventDispatch_GetTypeInfoCount,
    EventDispatch_GetTypeInfo,
    EventDispatch_GetIDsOfNames,
    EventDispatch_Invoke
};

static IDispatch EventDispatch = { &EventDispatchVtbl };

static void set_body_html(IHTMLDocument2 *doc, const WCHAR *html)
{
    IHTMLElement *body = doc_get_body(doc);
    set_elem_innerhtml(body, html);
    IHTMLElement_Release(body);
}

static void test_onclick(IHTMLDocument2 *doc)
{
    DWORD events_cp_cookie, events2_cp_cookie, elem2_cp_cookie;
    IHTMLElement *div, *body;
    VARIANT v;
    HRESULT hres;

    trace("onclick tests in document mode %d\n", document_mode);

    set_body_html(doc, L"<div id=\"clickdiv\""
                       " style=\"text-align: center; background: red; font-size: 32\">"
                       "click here"
                       "</div>");

    register_cp((IUnknown*)doc, &IID_IDispatch, (IUnknown*)&EventDispatch);

    div = get_elem_id(doc, L"clickdiv");

    elem_attach_event((IUnknown*)div, L"abcde", (IDispatch*)&nocall_obj);
    elem_attach_event((IUnknown*)div, L"onclick", (IDispatch*)&div_onclick_attached_obj);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onclick) = %d\n", V_VT(&v));

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == (document_mode < 9 ? E_NOTIMPL : S_OK), "put_onclick failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onclick) = %d\n", V_VT(&v));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(L"function();");
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);
    SysFreeString(V_BSTR(&v));

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    if(document_mode < 9) {
        ok(V_VT(&v) == VT_BSTR, "V_VT(onclick) = %d\n", V_VT(&v));
        ok(!lstrcmpW(V_BSTR(&v), L"function();"), "V_BSTR(onclick) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    }else {
        todo_wine
        ok(V_VT(&v) == VT_NULL, "V_VT(onclick) = %d\n", V_VT(&v));
    }
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&div_onclick_obj;
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLElement_put_ondblclick(div, v);
    ok(hres == S_OK, "put_ondblclick failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onclick) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&div_onclick_obj, "V_DISPATCH(onclick) != onclickFunc\n");
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLDocument2_get_onclick(doc, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onclick) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&document_onclick_obj;
    hres = IHTMLDocument2_put_onclick(doc, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLDocument2_get_onclick(doc, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onclick) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&document_onclick_obj, "V_DISPATCH(onclick) != onclickFunc\n");
    VariantClear(&v);

    if(document_mode >= 9) {
        add_event_listener((IUnknown*)div, L"click", (IDispatch*)&div_onclick_capture_obj, VARIANT_TRUE);
        add_event_listener((IUnknown*)div, L"click", (IDispatch*)&div_onclick_bubble_obj, VARIANT_FALSE);
    }

    body = doc_get_body(doc);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&body_onclick_obj;
    hres = IHTMLElement_put_onclick(body, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    if(winetest_interactive && document_mode < 9) {
        SET_EXPECT(div_onclick);
        SET_EXPECT(div_onclick_attached);
        SET_EXPECT(body_onclick);
        SET_EXPECT(document_onclick);
        SET_EXPECT(invoke_onclick);
        pump_msgs(&called_document_onclick);
        CHECK_CALLED(div_onclick);
        CHECK_CALLED(div_onclick_attached);
        CHECK_CALLED(body_onclick);
        CHECK_CALLED(document_onclick);
        CHECK_CALLED(invoke_onclick);
    }

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_attached);
    if(document_mode >= 9) {
        SET_EXPECT(div_onclick_capture);
        SET_EXPECT(div_onclick_bubble);
    }
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(invoke_onclick);

    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08lx\n", hres);

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_attached);
    if(document_mode >= 9) {
        CHECK_CALLED(div_onclick_capture);
        CHECK_CALLED(div_onclick_bubble);
    }
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(invoke_onclick);

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(invoke_onclick);

    V_VT(&v) = VT_EMPTY;
    elem_fire_event((IUnknown*)div, L"onclick", &v);

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(invoke_onclick);

    events_cp_cookie = register_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, (IUnknown*)&doccp_obj);
    events2_cp_cookie = register_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents2, (IUnknown*)&doccp2_obj);
    elem_attach_event((IUnknown*)div, L"onclick", (IDispatch*)&div_onclick_disp);
    doc_attach_event(doc, L"onclick", (IDispatch*)&doc_onclick_attached_obj);

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_disp);
    SET_EXPECT(div_onclick_attached);
    if(document_mode >= 9) {
        SET_EXPECT(div_onclick_capture);
        SET_EXPECT(div_onclick_bubble);
    }
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(doc_onclick_attached);
    SET_EXPECT(doccp_onclick);
    SET_EXPECT(doccp2_onclick);
    SET_EXPECT(invoke_onclick);

    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08lx\n", hres);

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_disp);
    CHECK_CALLED(div_onclick_attached);
    if(document_mode >= 9) {
        CHECK_CALLED(div_onclick_capture);
        CHECK_CALLED(div_onclick_bubble);
    }
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(doc_onclick_attached);
    CHECK_CALLED(doccp_onclick);
    CHECK_CALLED(doccp2_onclick);
    CHECK_CALLED(invoke_onclick);

    elem2_cp_cookie = register_cp((IUnknown*)div, &DIID_HTMLElementEvents2, (IUnknown*)&elem2_cp_obj);

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_disp);
    SET_EXPECT(div_onclick_attached);
    if(document_mode >= 9) {
        SET_EXPECT(div_onclick_capture);
        SET_EXPECT(div_onclick_bubble);
    }
    SET_EXPECT(elem2_cp_onclick);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(doc_onclick_attached);
    SET_EXPECT(doccp_onclick);
    SET_EXPECT(doccp2_onclick);
    SET_EXPECT(invoke_onclick);

    trace("click >>>\n");
    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    trace("click <<<\n");

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_disp);
    CHECK_CALLED(div_onclick_attached);
    if(document_mode >= 9) {
        CHECK_CALLED(div_onclick_capture);
        CHECK_CALLED(div_onclick_bubble);
    }
    CHECK_CALLED(elem2_cp_onclick);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(doc_onclick_attached);
    CHECK_CALLED(doccp_onclick);
    CHECK_CALLED(doccp2_onclick);
    CHECK_CALLED(invoke_onclick);

    unregister_cp((IUnknown*)div, &DIID_HTMLElementEvents2, elem2_cp_cookie);
    unregister_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, events_cp_cookie);
    unregister_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents2, events2_cp_cookie);

    V_VT(&v) = VT_NULL;
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "get_onclick returned vt %d\n", V_VT(&v));

    elem_detach_event((IUnknown*)div, L"onclick", (IDispatch*)&div_onclick_disp);
    elem_detach_event((IUnknown*)div, L"onclick", (IDispatch*)&div_onclick_disp);
    elem_detach_event((IUnknown*)div, L"test", (IDispatch*)&div_onclick_disp);
    doc_detach_event(doc, L"onclick", (IDispatch*)&doc_onclick_attached_obj);

    if(document_mode >= 9) {
        remove_event_listener((IUnknown*)div, L"click", (IDispatch*)&div_onclick_capture_obj, VARIANT_TRUE);
        remove_event_listener((IUnknown*)div, L"click", (IDispatch*)&div_onclick_bubble_obj, VARIANT_FALSE);
    }

    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(invoke_onclick);

    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08lx\n", hres);

    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(invoke_onclick);

    IHTMLElement_Release(div);
    IHTMLElement_Release(body);
}

static void test_onreadystatechange(IHTMLDocument2 *doc)
{
    IHTMLFrameBase *iframe;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    elem = get_elem_id(doc, L"iframe");
    elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onreadystatechange(elem2, &v);
    ok(hres == S_OK, "get_onreadystatechange failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onreadystatechange) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&iframe_onreadystatechange_obj;
    hres = IHTMLElement2_put_onreadystatechange(elem2, v);
    ok(hres == S_OK, "put_onreadystatechange failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onreadystatechange(elem2, &v);
    ok(hres == S_OK, "get_onreadystatechange failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onreadystatechange) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&iframe_onreadystatechange_obj, "unexpected onreadystatechange value\n");

    hres = IHTMLElement2_QueryInterface(elem2, &IID_IHTMLFrameBase, (void**)&iframe);
    IHTMLElement2_Release(elem2);
    ok(hres == S_OK, "Could not get IHTMLFrameBase iface: %08lx\n", hres);

    hres = IHTMLFrameBase_put_src(iframe, (str = SysAllocString(L"about:blank")));
    SysFreeString(str);
    ok(hres == S_OK, "put_src failed: %08lx\n", hres);

    SET_EXPECT(iframe_onreadystatechange_loading);
    SET_EXPECT(iframedoc_onreadystatechange);
    SET_EXPECT(iframe_onreadystatechange_interactive);
    SET_EXPECT(iframe_onreadystatechange_complete);
    pump_msgs(&called_iframe_onreadystatechange_complete);
    CHECK_CALLED(iframe_onreadystatechange_loading);
    CHECK_CALLED(iframedoc_onreadystatechange);
    CHECK_CALLED(iframe_onreadystatechange_interactive);
    CHECK_CALLED(iframe_onreadystatechange_complete);

    IHTMLFrameBase_Release(iframe);
}

static void test_imgload(IHTMLDocument2 *doc)
{
    IHTMLImgElement *img;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    elem = get_elem_id(doc, L"imgid");
    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLImgElement, (void**)&img);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLImgElement iface: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onload(img, &v);
    ok(hres == S_OK, "get_onload failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onload) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&img_onload_obj;
    hres = IHTMLImgElement_put_onload(img, v);
    ok(hres == S_OK, "put_onload failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onload(img, &v);
    ok(hres == S_OK, "get_onload failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onload) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&img_onload_obj, "V_DISPATCH(onload) != onloadkFunc\n");
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&img_onerror_obj;
    hres = IHTMLImgElement_put_onerror(img, v);
    ok(hres == S_OK, "put_onerror failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onerror(img, &v);
    ok(hres == S_OK, "get_onerror failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onerror) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&img_onerror_obj, "V_DISPATCH(onerror) != onerrorFunc\n");
    VariantClear(&v);

    str = SysAllocString(L"http://test.winehq.org/tests/winehq_snapshot/index_files/winehq_logo_text.png");
    hres = IHTMLImgElement_put_src(img, str);
    ok(hres == S_OK, "put_src failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(img_onload);
    pump_msgs(&called_img_onload);
    CHECK_CALLED(img_onload);

    SET_EXPECT(img_onerror);

    str = SysAllocString(L"about:blank");
    hres = IHTMLImgElement_put_src(img, str);
    ok(hres == S_OK, "put_src failed: %08lx\n", hres);
    SysFreeString(str);

    pump_msgs(&called_img_onerror); /* FIXME: should not be needed */

    CHECK_CALLED(img_onerror);

    IHTMLImgElement_Release(img);

    /* test onload on unattached image */
    hres = IHTMLDocument2_createElement(doc, (str = SysAllocString(L"img")), &elem);
    SysFreeString(str);
    ok(hres == S_OK, "createElement(img) failed: %08lx\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLImgElement, (void**)&img);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLImgElement iface: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onload(img, &v);
    ok(hres == S_OK, "get_onload failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onload) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&unattached_img_onload_obj;
    hres = IHTMLImgElement_put_onload(img, v);
    ok(hres == S_OK, "put_onload failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onload(img, &v);
    ok(hres == S_OK, "get_onload failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onload) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&unattached_img_onload_obj, "incorrect V_DISPATCH(onload)\n");
    VariantClear(&v);

    str = SysAllocString(L"http://test.winehq.org/tests/winehq_snapshot/index_files/winehq_logo_text.png?v=1");
    hres = IHTMLImgElement_put_src(img, str);
    ok(hres == S_OK, "put_src failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(img_onload);
    pump_msgs(&called_img_onload);
    CHECK_CALLED(img_onload);

    IHTMLImgElement_Release(img);
}

static void test_link_load(IHTMLDocument2 *doc)
{
    IHTMLLinkElement *link;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    elem = get_elem_id(doc, L"linkid");
    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLLinkElement, (void**)&link);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLLinkElement iface: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLLinkElement_get_onload(link, &v);
    ok(hres == S_OK, "get_onload failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onload) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&link_onload_obj;
    hres = IHTMLLinkElement_put_onload(link, v);
    ok(hres == S_OK, "put_onload failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLLinkElement_get_onload(link, &v);
    ok(hres == S_OK, "get_onload failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onload) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&link_onload_obj, "V_DISPATCH(onload) != onloadkFunc\n");
    VariantClear(&v);

    str = SysAllocString(L"http://test.winehq.org/tests/winehq_snapshot/index_files/styles.css");
    hres = IHTMLLinkElement_put_href(link, str);
    ok(hres == S_OK, "put_src failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(link_onload);
    pump_msgs(&called_link_onload);
    CHECK_CALLED(link_onload);

    IHTMLLinkElement_Release(link);
}

static void test_focus(IHTMLDocument2 *doc)
{
    IHTMLElement2 *elem2;
    IHTMLElement4 *div;
    IHTMLElement *elem;
    VARIANT v;
    HRESULT hres;

    elem = get_elem_id(doc, L"inputid");
    elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    elem = get_elem_id(doc, L"divid");
    div = get_elem4_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onfocus(elem2, &v);
    ok(hres == S_OK, "get_onfocus failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onfocus) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&input_onfocus_obj;
    hres = IHTMLElement2_put_onfocus(elem2, v);
    ok(hres == S_OK, "put_onfocus failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onfocus(elem2, &v);
    ok(hres == S_OK, "get_onfocus failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onfocus) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&input_onfocus_obj, "V_DISPATCH(onfocus) != onfocusFunc\n");
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&div_onfocusin_obj;
    hres = IHTMLElement4_put_onfocusin(div, v);
    ok(hres == S_OK, "put_onfocusin failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement4_get_onfocusin(div, &v);
    ok(hres == S_OK, "get_onfocusin failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onfocusin) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&div_onfocusin_obj, "V_DISPATCH(onfocus) != onfocusFunc\n");
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onfocus(elem2, &v);
    ok(hres == S_OK, "get_onfocus failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onfocus) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&input_onfocus_obj, "V_DISPATCH(onfocus) != onfocusFunc\n");
    VariantClear(&v);

    if(!winetest_interactive)
        ShowWindow(container_hwnd, SW_SHOW);

    SetFocus(NULL);
    ok(!IsChild(container_hwnd, GetFocus()), "focus belongs to document window\n");

    hres = IHTMLWindow2_focus(window);
    ok(hres == S_OK, "focus failed: %08lx\n", hres);

    ok(IsChild(container_hwnd, GetFocus()), "focus does not belong to document window\n");
    pump_msgs(NULL);

    SET_EXPECT(div_onfocusin);
    SET_EXPECT(input_onfocus);
    hres = IHTMLElement2_focus(elem2);
    ok(hres == S_OK, "focus failed: %08lx\n", hres);
    pump_msgs(NULL);
    CHECK_CALLED(div_onfocusin);
    CHECK_CALLED(input_onfocus);

    SET_EXPECT(div_onfocusin);
    V_VT(&v) = VT_EMPTY;
    elem_fire_event((IUnknown*)elem2, L"onfocusin", &v);
    CHECK_CALLED(div_onfocusin);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&input_onblur_obj;
    hres = IHTMLElement2_put_onblur(elem2, v);
    ok(hres == S_OK, "put_onblur failed: %08lx\n", hres);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&div_onfocusout_obj;
    hres = IHTMLElement4_put_onfocusout(div, v);
    ok(hres == S_OK, "put_onfocusout failed: %08lx\n", hres);

    SET_EXPECT(div_onfocusout);
    SET_EXPECT(input_onblur);
    hres = IHTMLElement2_blur(elem2);
    pump_msgs(NULL);
    CHECK_CALLED(input_onblur);
    CHECK_CALLED(div_onfocusout);
    ok(hres == S_OK, "blur failed: %08lx\n", hres);

    if(!winetest_interactive)
        ShowWindow(container_hwnd, SW_HIDE);

    IHTMLElement2_Release(elem2);
    IHTMLElement4_Release(div);
}

static void test_submit(IHTMLDocument2 *doc)
{
    IHTMLElement *elem, *submit;
    IHTMLFormElement *form;
    VARIANT v;
    DWORD cp_cookie;
    HRESULT hres;

    set_body_html(doc,
                  L"<form id=\"formid\" method=\"post\" action=\"about:blank\">"
                  "<input type=\"text\" value=\"test\" name=\"i\"/>"
                  "<input type=\"submit\" id=\"submitid\" />"
                  "</form>");

    elem = get_elem_id(doc, L"formid");

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&form_onclick_obj;
    hres = IHTMLElement_put_onclick(elem, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLFormElement, (void**)&form);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLFormElement iface: %08lx\n", hres);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&form_onsubmit_obj;
    hres = IHTMLFormElement_put_onsubmit(form, v);
    ok(hres == S_OK, "put_onsubmit failed: %08lx\n", hres);

    IHTMLFormElement_Release(form);

    submit = get_elem_id(doc, L"submitid");

    SET_EXPECT(form_onclick);
    SET_EXPECT(form_onsubmit);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(form_onsubmit);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&submit_onclick_obj;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    SET_EXPECT(form_onclick);
    SET_EXPECT(submit_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(submit_onclick);

    elem_attach_event((IUnknown*)submit, L"onclick", (IDispatch*)&submit_onclick_attached_obj);

    SET_EXPECT(form_onclick);
    SET_EXPECT(submit_onclick);
    SET_EXPECT(submit_onclick_attached);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(submit_onclick);
    CHECK_CALLED(submit_onclick_attached);

    V_VT(&v) = VT_NULL;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    SET_EXPECT(form_onclick);
    SET_EXPECT(submit_onclick_attached);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(submit_onclick_attached);

    elem_detach_event((IUnknown*)submit, L"onclick", (IDispatch*)&submit_onclick_attached_obj);

    cp_cookie = register_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, (IUnknown*)&doccp_onclick_cancel_obj);

    SET_EXPECT(form_onclick);
    SET_EXPECT(doccp_onclick_cancel);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(doccp_onclick_cancel);

    unregister_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, cp_cookie);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&submit_onclick_setret_obj;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    V_VT(&onclick_retval) = VT_BOOL;
    V_BOOL(&onclick_retval) = VARIANT_TRUE;
    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_TRUE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    SET_EXPECT(form_onsubmit);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(form_onsubmit);

    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_FALSE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);

    V_VT(&onclick_retval) = VT_BOOL;
    V_BOOL(&onclick_retval) = VARIANT_FALSE;
    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_TRUE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);

    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_FALSE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);

    elem_attach_event((IUnknown*)submit, L"onclick", (IDispatch*)&submit_onclick_attached_obj);
    elem_attach_event((IUnknown*)submit, L"onclick", (IDispatch*)&submit_onclick_attached_check_cancel_obj);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&submit_onclick_cancel_obj;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08lx\n", hres);

    SET_EXPECT(submit_onclick_cancel);
    SET_EXPECT(submit_onclick_attached_check_cancel);
    SET_EXPECT(submit_onclick_attached);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08lx\n", hres);
    CHECK_CALLED(submit_onclick_cancel);
    CHECK_CALLED(submit_onclick_attached_check_cancel);
    CHECK_CALLED(submit_onclick_attached);

    if(1)pump_msgs(NULL);

    IHTMLElement_Release(submit);
}

static void test_timeout(IHTMLDocument2 *doc)
{
    VARIANT expr, var, args[2];
    DISPPARAMS dp = { args, NULL, 2, 0 };
    IHTMLWindow3 *win3;
    IDispatch *disp;
    UINT argerr;
    LONG id;
    HRESULT hres;

    /* First try the IHTMLWindow2 DISPIDs via IDispatch, since they're not exposed */
    hres = IHTMLWindow2_QueryInterface(window, &IID_IDispatch, (void**)&disp);
    ok(hres == S_OK, "Could not get IDispatch iface: %08lx\n", hres);

    V_VT(&args[1]) = VT_BSTR;
    V_BSTR(&args[1]) = SysAllocString(L"");
    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 1;
    V_VT(&var) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_IHTMLWINDOW2_SETINTERVAL, &IID_NULL, LOCALE_USER_DEFAULT,
                            DISPATCH_METHOD, &dp, &var, NULL, &argerr);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke(DISPID_IHTMLWINDOW2_SETINTERVAL) returned: %08lx\n", hres);

    hres = IDispatch_Invoke(disp, DISPID_IHTMLWINDOW2_SETTIMEOUT, &IID_NULL, LOCALE_USER_DEFAULT,
                            DISPATCH_METHOD, &dp, &var, NULL, &argerr);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke(DISPID_IHTMLWINDOW2_SETTIMEOUT) returned: %08lx\n", hres);
    SysFreeString(V_BSTR(&args[1]));
    IDispatch_Release(disp);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLWindow3, (void**)&win3);
    ok(hres == S_OK, "Could not get IHTMLWindow3 iface: %08lx\n", hres);

    V_VT(&expr) = VT_DISPATCH;
    V_DISPATCH(&expr) = (IDispatch*)&timeoutFunc2;
    V_VT(&var) = VT_EMPTY;
    id = 0;
    hres = IHTMLWindow3_setInterval(win3, &expr, 1, &var, &id);
    ok(hres == S_OK, "setInterval failed: %08lx\n", hres);
    ok(id, "id = 0\n");

    hres = IHTMLWindow2_clearTimeout(window, id);
    ok(hres == S_OK, "clearTimeout failer: %08lx\n", hres);

    V_VT(&expr) = VT_DISPATCH;
    V_DISPATCH(&expr) = (IDispatch*)&timeoutFunc;
    V_VT(&var) = VT_EMPTY;
    id = 0;
    hres = IHTMLWindow3_setTimeout(win3, &expr, 0, &var, &id);
    ok(hres == S_OK, "setTimeout failed: %08lx\n", hres);
    ok(id, "id = 0\n");

    hres = IHTMLWindow2_clearTimeout(window, 0);
    ok(hres == S_OK, "clearTimeout failed: %08lx\n", hres);

    SET_EXPECT(timeout);
    pump_msgs(&called_timeout);
    CHECK_CALLED(timeout);

    V_VT(&expr) = VT_DISPATCH;
    V_DISPATCH(&expr) = (IDispatch*)&timeoutFunc;
    V_VT(&var) = VT_EMPTY;
    id = 0;
    hres = IHTMLWindow3_setTimeout(win3, &expr, 0, &var, &id);
    ok(hres == S_OK, "setTimeout failed: %08lx\n", hres);
    ok(id, "id = 0\n");

    hres = IHTMLWindow2_clearTimeout(window, id);
    ok(hres == S_OK, "clearTimeout failed: %08lx\n", hres);

    V_VT(&expr) = VT_DISPATCH;
    V_DISPATCH(&expr) = (IDispatch*)&timeoutFunc;
    V_VT(&var) = VT_EMPTY;
    id = 0;
    hres = IHTMLWindow3_setInterval(win3, &expr, 1, &var, &id);
    ok(hres == S_OK, "setInterval failed: %08lx\n", hres);
    ok(id, "id = 0\n");

    SET_EXPECT(timeout);
    pump_msgs(&called_timeout);
    CHECK_CALLED(timeout);

    SET_EXPECT(timeout);
    pump_msgs(&called_timeout);
    CHECK_CALLED(timeout);

    hres = IHTMLWindow2_clearInterval(window, 0);
    ok(hres == S_OK, "clearInterval failed: %08lx\n", hres);

    SET_EXPECT(timeout);
    pump_msgs(&called_timeout);
    CHECK_CALLED(timeout);

    hres = IHTMLWindow2_clearInterval(window, id);
    ok(hres == S_OK, "clearTimeout failer: %08lx\n", hres);

    IHTMLWindow3_Release(win3);
}

static IHTMLWindow2 *get_iframe_window(IHTMLIFrameElement *iframe)
{
    IHTMLWindow2 *window;
    IHTMLFrameBase2 *base;
    HRESULT hres;

    hres = IHTMLIFrameElement_QueryInterface(iframe, &IID_IHTMLFrameBase2, (void**)&base);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLFrameBase2) failed: %08lx\n", hres);

    hres = IHTMLFrameBase2_get_contentWindow(base, &window);
    ok(hres == S_OK, "get_contentWindow failed: %08lx\n", hres);
    ok(window != NULL, "window == NULL\n");

    IHTMLFrameBase2_Release(base);
    return window;
}

static IHTMLDocument2 *get_iframe_doc(IHTMLIFrameElement *iframe)
{
    IHTMLDocument2 *result = NULL;
    IHTMLWindow2 *window;
    HRESULT hres;

    window = get_iframe_window(iframe);

    hres = IHTMLWindow2_get_document(window, &result);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);
    ok(result != NULL, "result == NULL\n");

    IHTMLWindow2_Release(window);
    return result;
}

static void test_iframe_connections(IHTMLDocument2 *doc)
{
    IHTMLIFrameElement *iframe;
    IHTMLDocument2 *iframes_doc;
    DWORD cookie;
    IConnectionPoint *cp;
    IHTMLElement *element;
    BSTR str;
    HRESULT hres;

    trace("iframe tests...\n");

    element = get_elem_id(doc, L"ifr");
    iframe = get_iframe_iface((IUnknown*)element);
    IHTMLElement_Release(element);

    iframes_doc = get_iframe_doc(iframe);
    IHTMLIFrameElement_Release(iframe);

    cookie = register_cp((IUnknown*)iframes_doc, &IID_IDispatch, (IUnknown*)&div_onclick_disp);

    cp = get_cp((IUnknown*)doc, &IID_IDispatch);
    hres = IConnectionPoint_Unadvise(cp, cookie);
    IConnectionPoint_Release(cp);
    ok(hres == CONNECT_E_NOCONNECTION, "Unadvise returned %08lx, expected CONNECT_E_NOCONNECTION\n", hres);

    unregister_cp((IUnknown*)iframes_doc, &IID_IDispatch, cookie);

    if(is_ie9plus) {
        IHTMLFrameBase2 *frame_base2;
        VARIANT v;

        hres = IHTMLIFrameElement_QueryInterface(iframe, &IID_IHTMLFrameBase2, (void**)&frame_base2);
        ok(hres == S_OK, "Could not get IHTMLFrameBase2 iface: %08lx\n", hres);

        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = (IDispatch*)&iframe_onload_obj;
        hres = IHTMLFrameBase2_put_onload(frame_base2, v);
        ok(hres == S_OK, "put_onload failed: %08lx\n", hres);

        IHTMLFrameBase2_Release(frame_base2);

        str = SysAllocString(L"about:blank");
        hres = IHTMLDocument2_put_URL(iframes_doc, str);
        ok(hres == S_OK, "put_URL failed: %08lx\n", hres);
        SysFreeString(str);

        SET_EXPECT(iframe_onload);
        pump_msgs(&called_iframe_onload);
        CHECK_CALLED(iframe_onload);

        str = SysAllocString(L"about:test");
        hres = IHTMLDocument2_put_URL(iframes_doc, str);
        ok(hres == S_OK, "put_URL failed: %08lx\n", hres);
        SysFreeString(str);

        SET_EXPECT(iframe_onload);
        pump_msgs(&called_iframe_onload);
        CHECK_CALLED(iframe_onload);
    }else {
        win_skip("Skipping iframe onload tests on IE older than 9.\n");
    }

    IHTMLDocument2_Release(iframes_doc);
}

static void test_create_event(IHTMLDocument2 *doc)
{
    IDOMKeyboardEvent *keyboard_event;
    IDOMCustomEvent *custom_event;
    IDOMMouseEvent *mouse_event;
    IDocumentEvent *doc_event;
    IEventTarget *event_target;
    IDOMUIEvent *ui_event;
    IHTMLElement *elem;
    IDOMEvent *event;
    VARIANT_BOOL b;
    USHORT phase;
    BSTR str;
    HRESULT hres;

    trace("createEvent tests...\n");

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IDocumentEvent, (void**)&doc_event);
    ok(hres == S_OK, "Could not get IDocumentEvent iface: %08lx\n", hres);

    str = SysAllocString(L"Event");
    hres = IDocumentEvent_createEvent(doc_event, str, &event);
    SysFreeString(str);
    ok(hres == S_OK, "createEvent failed: %08lx\n", hres);

    phase = 0xdead;
    hres = IDOMEvent_get_eventPhase(event, &phase);
    ok(hres == S_OK, "get_eventPhase failed: %08lx\n", hres);
    ok(!phase, "phase = %u\n", phase);

    hres = IDOMEvent_preventDefault(event);
    ok(hres == S_OK, "preventDefault failed: %08lx\n", hres);

    hres = IDOMEvent_stopPropagation(event);
    ok(hres == S_OK, "stopPropagation failed: %08lx\n", hres);

    str = (void*)0xdeadbeef;
    hres = IDOMEvent_get_type(event, &str);
    ok(hres == S_OK, "get_type failed: %08lx\n", hres);
    ok(!str, "type = %s\n", wine_dbgstr_w(str));

    b = 0xdead;
    hres = IDOMEvent_get_bubbles(event, &b);
    ok(hres == S_OK, "get_bubbles failed: %08lx\n", hres);
    ok(!b, "bubbles = %x\n", b);

    b = 0xdead;
    hres = IDOMEvent_get_cancelable(event, &b);
    ok(hres == S_OK, "get_cancelable failed: %08lx\n", hres);
    ok(!b, "cancelable = %x\n", b);

    elem = doc_get_body(doc);
    hres = IHTMLElement_QueryInterface(elem, &IID_IEventTarget, (void**)&event_target);
    IHTMLElement_Release(elem);

    hres = IEventTarget_dispatchEvent(event_target, NULL, &b);
    ok(hres == E_INVALIDARG, "dispatchEvent failed: %08lx\n", hres);

    IEventTarget_Release(event_target);

    hres = IDOMEvent_QueryInterface(event, &IID_IDOMUIEvent, (void**)&ui_event);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IDOMUIEvent returned %08lx\n", hres);
    hres = IDOMEvent_QueryInterface(event, &IID_IDOMMouseEvent, (void**)&mouse_event);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IDOMMouseEvent returned %08lx\n", hres);

    IDOMEvent_Release(event);

    str = SysAllocString(L"MouseEvent");
    hres = IDocumentEvent_createEvent(doc_event, str, &event);
    SysFreeString(str);
    ok(hres == S_OK, "createEvent failed: %08lx\n", hres);

    hres = IDOMEvent_QueryInterface(event, &IID_IDOMUIEvent, (void**)&ui_event);
    ok(hres == S_OK, "QueryInterface(IID_IDOMUIEvent returned %08lx\n", hres);
    IDOMUIEvent_Release(ui_event);
    hres = IDOMEvent_QueryInterface(event, &IID_IDOMMouseEvent, (void**)&mouse_event);
    ok(hres == S_OK, "QueryInterface(IID_IDOMMouseEvent returned %08lx\n", hres);
    IDOMMouseEvent_Release(mouse_event);

    IDOMEvent_Release(event);

    str = SysAllocString(L"UIEvent");
    hres = IDocumentEvent_createEvent(doc_event, str, &event);
    SysFreeString(str);
    ok(hres == S_OK, "createEvent failed: %08lx\n", hres);

    hres = IDOMEvent_QueryInterface(event, &IID_IDOMUIEvent, (void**)&ui_event);
    ok(hres == S_OK, "QueryInterface(IID_IDOMUIEvent returned %08lx\n", hres);
    IDOMUIEvent_Release(ui_event);
    hres = IDOMEvent_QueryInterface(event, &IID_IDOMMouseEvent, (void**)&mouse_event);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IDOMMouseEvent returned %08lx\n", hres);

    IDOMEvent_Release(event);

    str = SysAllocString(L"KeyboardEvent");
    hres = IDocumentEvent_createEvent(doc_event, str, &event);
    SysFreeString(str);
    ok(hres == S_OK, "createEvent failed: %08lx\n", hres);

    hres = IDOMEvent_QueryInterface(event, &IID_IDOMUIEvent, (void**)&ui_event);
    ok(hres == S_OK, "QueryInterface(IID_IDOMUIEvent returned %08lx\n", hres);
    IDOMUIEvent_Release(ui_event);
    hres = IDOMEvent_QueryInterface(event, &IID_IDOMKeyboardEvent, (void**)&keyboard_event);
    ok(hres == S_OK, "QueryInterface(IID_IDOMKeyboardEvent returned %08lx\n", hres);
    IDOMKeyboardEvent_Release(keyboard_event);

    IDOMEvent_Release(event);

    str = SysAllocString(L"CustomEvent");
    hres = IDocumentEvent_createEvent(doc_event, str, &event);
    SysFreeString(str);
    ok(hres == S_OK, "createEvent failed: %08lx\n", hres);

    hres = IDOMEvent_QueryInterface(event, &IID_IDOMCustomEvent, (void**)&custom_event);
    ok(hres == S_OK, "QueryInterface(IID_IDOMCustomEvent returned %08lx\n", hres);
    IDOMCustomEvent_Release(custom_event);

    IDOMEvent_Release(event);

    IDocumentEvent_Release(doc_event);
}

static unsigned onstorage_expect_line;
static const WCHAR *onstorage_expect_key, *onstorage_expect_old_value, *onstorage_expect_new_value;

#define set_onstorage_expect(a,b,c) _set_onstorage_expect(__LINE__,a,b,c)
static void _set_onstorage_expect(unsigned line, const WCHAR *key, const WCHAR *old_value, const WCHAR *new_value)
{
    onstorage_expect_line = line;
    onstorage_expect_key = key;
    onstorage_expect_old_value = old_value;
    onstorage_expect_new_value = new_value;
}

static void test_storage_event(DISPPARAMS *params, BOOL doc_onstorage)
{
    const WCHAR *expect_key = onstorage_expect_key, *expect_old_value = onstorage_expect_old_value, *expect_new_value = onstorage_expect_new_value;
    unsigned line = onstorage_expect_line;
    IHTMLEventObj *event_obj;
    IDOMStorageEvent *event;
    IDispatchEx *dispex;
    HRESULT hres;
    unsigned i;
    DISPID id;
    BSTR bstr;

    if(document_mode < 9) {
        ok_(__FILE__,line)(params->cArgs == 1, "cArgs = %u\n", params->cArgs);
        ok_(__FILE__,line)(V_VT(&params->rgvarg[0]) == VT_DISPATCH, "V_VT(this) = %d\n", V_VT(&params->rgvarg[0]));
        return;
    }

    ok_(__FILE__,line)(params->cArgs == 2, "cArgs = %u\n", params->cArgs);
    ok_(__FILE__,line)(V_VT(&params->rgvarg[1]) == VT_DISPATCH, "V_VT(event) = %d\n", V_VT(&params->rgvarg[1]));
    hres = IDispatch_QueryInterface(V_DISPATCH(&params->rgvarg[1]), &IID_IDispatchEx, (void**)&dispex);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IDispatchEx: %08lx\n", hres);

    hres = IDispatchEx_QueryInterface(dispex, &IID_IDOMStorageEvent, (void**)&event);
    if(doc_onstorage) {
        static const WCHAR *props[] = { L"key", L"oldValue", L"newValue", L"storageArea" };
        ok_(__FILE__,line)(hres != S_OK, "Got IDOMStorageEvent in document.onstorage handler\n");

        hres = IDispatchEx_QueryInterface(dispex, &IID_IHTMLEventObj, (void**)&event_obj);
        ok_(__FILE__,line)(hres == S_OK, "Could not get IHTMLEventObj: %08lx\n", hres);
        IHTMLEventObj_Release(event_obj);

        for(i = 0; i < ARRAY_SIZE(props); i++) {
            bstr = SysAllocString(props[i]);
            hres = IDispatchEx_GetDispID(dispex, bstr, 0, &id);
            ok_(__FILE__,line)(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) failed: %08lx\n", wine_dbgstr_w(bstr), hres);
            SysFreeString(bstr);
        }

        IDispatchEx_Release(dispex);
        return;
    }

    ok_(__FILE__,line)(hres == S_OK, "Could not get IDOMStorageEvent: %08lx\n", hres);
    IDispatchEx_Release(dispex);

    hres = IDOMStorageEvent_get_key(event, &bstr);
    ok_(__FILE__,line)(hres == S_OK, "get_key failed: %08lx\n", hres);
    ok_(__FILE__,line)((!bstr || !expect_key) ? (bstr == expect_key) : !wcscmp(bstr, expect_key),
                       "key = %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hres = IDOMStorageEvent_get_oldValue(event, &bstr);
    ok_(__FILE__,line)(hres == S_OK, "get_oldValue failed: %08lx\n", hres);
    ok_(__FILE__,line)((!bstr || !expect_old_value) ? (bstr == expect_old_value) : !wcscmp(bstr, expect_old_value),
                       "oldValue = %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hres = IDOMStorageEvent_get_newValue(event, &bstr);
    ok_(__FILE__,line)(hres == S_OK, "get_newValue failed: %08lx\n", hres);
    ok_(__FILE__,line)((!bstr || !expect_new_value) ? (bstr == expect_new_value) : !wcscmp(bstr, expect_new_value),
                       "newValue = %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hres = IDOMStorageEvent_get_url(event, &bstr);
    ok_(__FILE__,line)(hres == S_OK, "get_url failed: %08lx\n", hres);
    ok_(__FILE__,line)(!wcscmp(bstr, L"http://winetest.example.org/"), "url = %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    IDOMStorageEvent_Release(event);
}

static HRESULT WINAPI doc1_onstorage(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(doc1_onstorage);
    test_storage_event(pdp, TRUE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(doc1_onstorage);

static HRESULT WINAPI doc1_onstoragecommit(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(doc1_onstoragecommit);
    test_storage_event(pdp, FALSE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(doc1_onstoragecommit);

static HRESULT WINAPI window1_onstorage(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(window1_onstorage);
    test_storage_event(pdp, FALSE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(window1_onstorage);

static HRESULT WINAPI doc2_onstorage(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(doc2_onstorage);
    test_storage_event(pdp, TRUE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(doc2_onstorage);

static HRESULT WINAPI doc2_onstoragecommit(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(doc2_onstoragecommit);
    test_storage_event(pdp, FALSE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(doc2_onstoragecommit);

static HRESULT WINAPI window2_onstorage(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(window2_onstorage);
    test_storage_event(pdp, FALSE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(window2_onstorage);

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceFrame InPlaceUIWindow = { &InPlaceUIWindowVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT rect = {0,0,300,300};

    *ppFrame = &InPlaceFrame;
    *ppDoc = (IOleInPlaceUIWindow*)&InPlaceUIWindow;
    *lprcPosRect = rect;
    *lprcClipRect = rect;

    ok(lpFrameInfo->cb == sizeof(*lpFrameInfo), "lpFrameInfo->cb = %u, expected %u\n", lpFrameInfo->cb, (unsigned)sizeof(*lpFrameInfo));
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = container_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtant)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}

static const IOleInPlaceSiteVtbl InPlaceSiteVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange,
};

static IOleInPlaceSite InPlaceSite = { &InPlaceSiteVtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI DocumentSite_QueryInterface(IOleDocumentSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocumentSite_AddRef(IOleDocumentSite *iface)
{
    return 2;
}

static ULONG WINAPI DocumentSite_Release(IOleDocumentSite *iface)
{
    return 1;
}

static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    RECT rect = {0,0,300,300};
    IOleDocument *document;
    HRESULT hres;

    hres = IOleDocumentView_QueryInterface(pViewToActivate, &IID_IOleDocument, (void**)&document);
    ok(hres == S_OK, "could not get IOleDocument: %08lx\n", hres);

    hres = IOleDocument_CreateView(document, &InPlaceSite, NULL, 0, &view);
    IOleDocument_Release(document);
    ok(hres == S_OK, "CreateView failed: %08lx\n", hres);

    hres = IOleDocumentView_SetInPlaceSite(view, &InPlaceSite);
    ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);

    hres = IOleDocumentView_UIActivate(view, TRUE);
    ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

    hres = IOleDocumentView_SetRect(view, &rect);
    ok(hres == S_OK, "SetRect failed: %08lx\n", hres);

    hres = IOleDocumentView_Show(view, TRUE);
    ok(hres == S_OK, "Show failed: %08lx\n", hres);

    return S_OK;
}

static const IOleDocumentSiteVtbl DocumentSiteVtbl = {
    DocumentSite_QueryInterface,
    DocumentSite_AddRef,
    DocumentSite_Release,
    DocumentSite_ActivateMe
};

static IOleDocumentSite DocumentSite = { &DocumentSiteVtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IOleWindow, riid) || IsEqualGUID(&IID_IOleInPlaceSite, riid))
        *ppv = &InPlaceSite;

    return *ppv ? S_OK : E_NOINTERFACE;
}

static IHTMLDocument2 *notif_doc;
static BOOL doc_complete;

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    return 2;
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    return 1;
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    if(dispID == DISPID_READYSTATE){
        BSTR state;
        HRESULT hres;

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08lx\n", hres);

        if(!lstrcmpW(state, L"complete"))
            doc_complete = TRUE;

        SysFreeString(state);
    }

    return S_OK;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

typedef struct {
    IInternetProtocolEx IInternetProtocolEx_iface;
    IWinInetHttpInfo IWinInetHttpInfo_iface;

    LONG ref;

    IInternetProtocolSink *sink;
    IUri *uri;

    ULONG size;
    const char *data;
    const char *ptr;
} ProtocolHandler;

static DWORD WINAPI async_switch_proc(void *arg)
{
    PROTOCOLDATA protocol_data = { PI_FORCE_ASYNC };
    ProtocolHandler *protocol_handler = arg;
    IInternetProtocolSink_Switch(protocol_handler->sink, &protocol_data);
    return 0;
}

static inline ProtocolHandler *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, ProtocolHandler, IInternetProtocolEx_iface);
}

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid) || IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IWinInetInfo, riid) || IsEqualGUID(&IID_IWinInetHttpInfo, riid)) {
        *ppv = &This->IWinInetHttpInfo_iface;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Protocol_AddRef(IInternetProtocolEx *iface)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Protocol_Release(IInternetProtocolEx *iface)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        if(This->sink)
            IInternetProtocolSink_Release(This->sink);
        if(This->uri)
            IUri_Release(This->uri);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink,
        IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    IServiceProvider *service_provider;
    IHttpNegotiate *http_negotiate;
    WCHAR *addl_headers = NULL;
    HRESULT hres;
    BSTR bstr;

    hres = IInternetProtocolSink_QueryInterface(This->sink, &IID_IServiceProvider, (void**)&service_provider);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

    hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate, &IID_IHttpNegotiate, (void**)&http_negotiate);
    IServiceProvider_Release(service_provider);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08lx\n", hres);

    hres = IUri_GetDisplayUri(This->uri, &bstr);
    ok(hres == S_OK, "Failed to get display uri: %08lx\n", hres);
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, bstr, L"", 0, &addl_headers);
    ok(hres == S_OK, "BeginningTransaction failed: %08lx\n", hres);
    CoTaskMemFree(addl_headers);
    SysFreeString(bstr);

    bstr = SysAllocString(L"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, bstr, NULL, NULL);
    ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);
    IHttpNegotiate_Release(http_negotiate);
    SysFreeString(bstr);

    hres = IInternetProtocolSink_ReportData(This->sink, BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION,
                                            This->size, This->size);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

    hres = IInternetProtocolSink_ReportResult(This->sink, S_OK, 0, NULL);
    ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);

    IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
    return S_OK;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason, DWORD dwOptions)
{
    trace("Abort(%08lx %lx)\n", hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    return S_OK;
}

static HRESULT WINAPI Protocol_Suspend(IInternetProtocolEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Resume(IInternetProtocolEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Read(IInternetProtocolEx *iface, void *pv, ULONG cb, ULONG *pcbRead)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    ULONG read;

    read = This->size - (This->ptr - This->data);
    if(read > cb)
        read = cb;

    if(read) {
        memcpy(pv, This->ptr, read);
        This->ptr += read;
    }

    *pcbRead = read;
    return (This->ptr != This->data + This->size) ? S_OK : S_FALSE;
}

static HRESULT WINAPI Protocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove, DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    return S_OK;
}

static HRESULT WINAPI Protocol_UnlockRequest(IInternetProtocolEx *iface)
{
    return S_OK;
}

static const char *protocol_doc_str;

static HRESULT WINAPI ProtocolEx_StartEx(IInternetProtocolEx *iface, IUri *uri, IInternetProtocolSink *pOIProtSink,
        IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE *dwReserved)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);

    This->data = protocol_doc_str;
    This->size = strlen(This->data);

    IInternetProtocolSink_AddRef(This->sink = pOIProtSink);
    IUri_AddRef(This->uri = uri);
    This->ptr = This->data;

    IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
    QueueUserWorkItem(async_switch_proc, This, 0);
    return E_PENDING;
}

static const IInternetProtocolExVtbl ProtocolExVtbl = {
    Protocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release,
    Protocol_Start,
    Protocol_Continue,
    Protocol_Abort,
    Protocol_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    Protocol_Read,
    Protocol_Seek,
    Protocol_LockRequest,
    Protocol_UnlockRequest,
    ProtocolEx_StartEx
};

static inline ProtocolHandler *impl_from_IWinInetHttpInfo(IWinInetHttpInfo *iface)
{
    return CONTAINING_RECORD(iface, ProtocolHandler, IWinInetHttpInfo_iface);
}

static HRESULT WINAPI HttpInfo_QueryInterface(IWinInetHttpInfo *iface, REFIID riid, void **ppv)
{
    ProtocolHandler *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI HttpInfo_AddRef(IWinInetHttpInfo *iface)
{
    ProtocolHandler *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI HttpInfo_Release(IWinInetHttpInfo *iface)
{
    ProtocolHandler *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI HttpInfo_QueryOption(IWinInetHttpInfo *iface, DWORD dwOption, void *pBuffer, DWORD *pcbBuffer)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpInfo_QueryInfo(IWinInetHttpInfo *iface, DWORD dwOption, void *pBuffer, DWORD *pcbBuffer,
        DWORD *pdwFlags, DWORD *pdwReserved)
{
    return E_NOTIMPL;
}

static const IWinInetHttpInfoVtbl WinInetHttpInfoVtbl = {
    HttpInfo_QueryInterface,
    HttpInfo_AddRef,
    HttpInfo_Release,
    HttpInfo_QueryOption,
    HttpInfo_QueryInfo
};

static HRESULT WINAPI ProtocolCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolCF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ProtocolCF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ProtocolCF_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    ProtocolHandler *protocol;
    HRESULT hres;

    protocol = calloc(1, sizeof(*protocol));
    protocol->IInternetProtocolEx_iface.lpVtbl = &ProtocolExVtbl;
    protocol->IWinInetHttpInfo_iface.lpVtbl = &WinInetHttpInfoVtbl;
    protocol->ref = 1;

    hres = IInternetProtocolEx_QueryInterface(&protocol->IInternetProtocolEx_iface, riid, ppv);
    IInternetProtocolEx_Release(&protocol->IInternetProtocolEx_iface);
    return hres;
}

static HRESULT WINAPI ProtocolCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ProtocolCFVtbl = {
    ProtocolCF_QueryInterface,
    ProtocolCF_AddRef,
    ProtocolCF_Release,
    ProtocolCF_CreateInstance,
    ProtocolCF_LockServer
};

static IClassFactory protocol_cf = { &ProtocolCFVtbl };

static void doc_load_string(IHTMLDocument2 *doc, const char *str)
{
    IInternetSession *internet_session;
    IPersistStreamInit *init;
    IStream *stream;
    HRESULT hres;
    HGLOBAL mem;
    SIZE_T len;

    if(protocol_doc_str) {
        hres = CoInternetGetSession(0, &internet_session, 0);
        ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);

        hres = IInternetSession_UnregisterNameSpace(internet_session, &protocol_cf, L"http");
        ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

        IInternetSession_Release(internet_session);
        protocol_doc_str = NULL;
    }

    notif_doc = doc;

    doc_complete = FALSE;
    len = strlen(str);
    mem = GlobalAlloc(0, len);
    memcpy(mem, str, len);
    hres = CreateStreamOnHGlobal(mem, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream, hr %#lx.\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "Failed to get IPersistStreamInit, hr %#lx.\n", hres);

    IPersistStreamInit_Load(init, stream);
    IPersistStreamInit_Release(init);
    IStream_Release(stream);
}

static void do_advise(IUnknown *unk, REFIID riid, IUnknown *unk_advise)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);

    hres = IConnectionPoint_Advise(cp, unk_advise, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);
}

static void set_client_site(IHTMLDocument2 *doc, BOOL set)
{
    IOleObject *oleobj;
    HRESULT hres;

    if(!set && view) {
        IOleDocumentView_Show(view, FALSE);
        IOleDocumentView_CloseView(view, 0);
        IOleDocumentView_SetInPlaceSite(view, NULL);
        IOleDocumentView_Release(view);
        view = NULL;
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not et IOleObject: %08lx\n", hres);

    hres = IOleObject_SetClientSite(oleobj, set ? &ClientSite : NULL);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

    if(set) {
        IHlinkTarget *hlink;

        hres = IOleObject_QueryInterface(oleobj, &IID_IHlinkTarget, (void**)&hlink);
        ok(hres == S_OK, "Could not get IHlinkTarget iface: %08lx\n", hres);

        hres = IHlinkTarget_Navigate(hlink, 0, NULL);
        ok(hres == S_OK, "Navgate failed: %08lx\n", hres);

        IHlinkTarget_Release(hlink);
    }

    IOleObject_Release(oleobj);
}
static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
#if !defined(__i386__) && !defined(__x86_64__)
    todo_wine
#endif
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    return SUCCEEDED(hres) ? doc : NULL;
}

static IHTMLDocument2 *create_document_with_origin(const char *str)
{
    IInternetSession *internet_session;
    IPersistMoniker *persist;
    IHTMLDocument2 *doc;
    IMoniker *mon;
    HRESULT hres;
    BSTR url;
    MSG msg;

    doc = create_document();
    if(!doc)
        return NULL;

    if(!protocol_doc_str) {
        hres = CoInternetGetSession(0, &internet_session, 0);
        ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);

        hres = IInternetSession_RegisterNameSpace(internet_session, &protocol_cf, &CLSID_HttpProtocol, L"http", 0, NULL, 0);
        ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

        IInternetSession_Release(internet_session);
    }
    protocol_doc_str = str;

    notif_doc = doc;
    doc_complete = FALSE;
    set_client_site(doc, TRUE);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    url = SysAllocString(L"http://winetest.example.org");
    hres = CreateURLMoniker(NULL, url, &mon);
    SysFreeString(url);
    ok(hres == S_OK, "CreateUrlMoniker failed: %08lx\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker, (void**)&persist);
    ok(hres == S_OK, "Could not get IPersistMoniker iface: %08lx\n", hres);

    hres = IPersistMoniker_Load(persist, FALSE, mon, NULL, 0);
    ok(hres == S_OK, "Load failed: %08lx\n", hres);
    IPersistMoniker_Release(persist);
    IMoniker_Release(mon);

    while(!doc_complete && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return doc;
}


typedef void (*testfunc_t)(IHTMLDocument2*);

static void run_test(const char *str, testfunc_t test)
{
    IHTMLDocument2 *doc;
    IHTMLElement *body = NULL;
    MSG msg;
    HRESULT hres;

    doc = create_document();
    if (!doc)
        return;
    set_client_site(doc, TRUE);
    doc_load_string(doc, str);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08lx\n", hres);

    if(body) {
        IHTMLDocument6 *doc6;

        IHTMLElement_Release(body);

        hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument6, (void**)&doc6);
        if(SUCCEEDED(hres)) {
            VARIANT v;
            hres = IHTMLDocument6_get_documentMode(doc6, &v);
            ok(hres == S_OK, "get_documentMode failed: %08lx\n", hres);
            ok(V_VT(&v) == VT_R4, "V_VT(documentMode) = %u\n", V_VT(&v));
            document_mode = V_R4(&v);
            IHTMLDocument6_Release(doc6);
        }else {
            document_mode = 0;
        }

        hres = IHTMLDocument2_get_parentWindow(doc, &window);
        ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);
        ok(window != NULL, "window == NULL\n");

        test(doc);

        IHTMLWindow2_Release(window);
        window = NULL;
    }else {
        skip("Could not get document body. Assuming no Gecko installed.\n");
    }

    set_client_site(doc, FALSE);
    IHTMLDocument2_Release(doc);
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static const CHAR szHTMLDocumentTest[] = "HTMLDocumentTest";
    static WNDCLASSEXA wndclass = {
        sizeof(WNDCLASSEXA),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        szHTMLDocumentTest,
        NULL
    };

    RegisterClassExA(&wndclass);
    return CreateWindowA(szHTMLDocumentTest, szHTMLDocumentTest,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            300, 300, NULL, NULL, NULL, NULL);
}

static void test_empty_document(void)
{
    HRESULT hres;
    IHTMLWindow2 *window;
    IHTMLDocument2 *windows_doc, *doc;
    IConnectionPoint *cp;
    DWORD cookie;

    doc = create_document();
    if(!doc)
        return;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);

    hres = IHTMLWindow2_get_document(window, &windows_doc);
    IHTMLWindow2_Release(window);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);

    cookie = register_cp((IUnknown*)windows_doc, &IID_IDispatch, (IUnknown*)&div_onclick_disp);

    cp = get_cp((IUnknown*)doc, &IID_IDispatch);
    hres = IConnectionPoint_Unadvise(cp, cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    IHTMLDocument2_Release(windows_doc);
    IHTMLDocument2_Release(doc);
}

static void test_storage_events(const char *doc_str)
{
    static struct  {
        IDispatchEx *onstorage;
        IDispatchEx *onstoragecommit;
        IDispatchEx *window_onstorage;
    } doc_onstorage_handlers[] = {
        { &doc1_onstorage_obj, &doc1_onstoragecommit_obj, &window1_onstorage_obj },
        { &doc2_onstorage_obj, &doc2_onstoragecommit_obj, &window2_onstorage_obj },
    };
    IHTMLStorage *session_storage[2], *local_storage[2];
    IHTMLDocument2 *doc[2];
    IHTMLDocument6 *doc6;
    BSTR key, value;
    HRESULT hres;
    VARIANT var;
    LONG length;
    unsigned i;

    for(i = 0; i < ARRAY_SIZE(doc); i++)
        doc[i] = create_document_with_origin(doc_str);

    document_mode = 0;
    for(i = 0; i < ARRAY_SIZE(doc); i++) {
        IHTMLWindow6 *window6;
        IHTMLWindow7 *window7;
        IHTMLWindow2 *window;

        hres = IHTMLDocument2_get_parentWindow(doc[i], &window);
        ok(hres == S_OK, "get_parentWindow[%u] failed: %08lx\n", i, hres);
        ok(window != NULL, "window[%u] == NULL\n", i);

        hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLWindow6, (void**)&window6);
        ok(hres == S_OK, "Could not get IHTMLWindow6: %08lx\n", hres);
        IHTMLWindow2_Release(window);

        hres = IHTMLWindow6_get_sessionStorage(window6, &session_storage[i]);
        ok(hres == S_OK, "get_sessionStorage[%u] failed: %08lx\n", i, hres);
        ok(session_storage[i] != NULL, "session_storage[%u] == NULL\n", i);
        hres = IHTMLWindow6_get_localStorage(window6, &local_storage[i]);
        ok(hres == S_OK, "get_localStorage[%u] failed: %08lx\n", i, hres);
        ok(local_storage[i] != NULL, "local_storage[%u] == NULL\n", i);

        hres = IHTMLDocument2_QueryInterface(doc[i], &IID_IHTMLDocument6, (void**)&doc6);
        if(SUCCEEDED(hres)) {
            if(i == 0) {
                hres = IHTMLDocument6_get_documentMode(doc6, &var);
                ok(hres == S_OK, "get_documentMode failed: %08lx\n", hres);
                ok(V_VT(&var) == VT_R4, "V_VT(documentMode) = %u\n", V_VT(&var));
                document_mode = V_R4(&var);
            }
            V_VT(&var) = VT_DISPATCH;
            V_DISPATCH(&var) = (IDispatch*)doc_onstorage_handlers[i].onstorage;
            hres = IHTMLDocument6_put_onstorage(doc6, var);
            ok(hres == S_OK, "put_onstorage[%u] failed: %08lx\n", i, hres);

            V_DISPATCH(&var) = (IDispatch*)doc_onstorage_handlers[i].onstoragecommit;
            hres = IHTMLDocument6_put_onstoragecommit(doc6, var);
            ok(hres == S_OK, "put_onstoragecommit[%u] failed: %08lx\n", i, hres);
            IHTMLDocument6_Release(doc6);
        }

        hres = IHTMLWindow6_QueryInterface(window6, &IID_IHTMLWindow7, (void**)&window7);
        IHTMLWindow6_Release(window6);
        if(SUCCEEDED(hres)) {
            V_VT(&var) = VT_DISPATCH;
            V_DISPATCH(&var) = (IDispatch*)doc_onstorage_handlers[i].window_onstorage;
            hres = IHTMLWindow7_put_onstorage(window7, var);
            ok(hres == S_OK, "put_onstorage[%u] failed: %08lx\n", i, hres);
            IHTMLWindow7_Release(window7);
        }
    }

    /* local storage */
    for(;;) {
        hres = IHTMLStorage_get_length(local_storage[0], &length);
        ok(hres == S_OK, "get_length failed %08lx\n", hres);
        if(length == 0) break;

        hres = IHTMLStorage_clear(local_storage[0]);
        ok(hres == S_OK, "clear failed: %08lx\n", hres);
        SET_EXPECT(doc1_onstoragecommit);
        set_onstorage_expect(NULL, NULL, NULL);
        pump_msgs(&called_doc1_onstoragecommit);
        CHECK_CALLED(doc1_onstoragecommit);
    }

    key = SysAllocString(L"test");
    hres = IHTMLStorage_removeItem(local_storage[0], key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);

    value = SysAllocString(L"WINE");
    hres = IHTMLStorage_setItem(local_storage[0], key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    SysFreeString(value);
    SET_EXPECT(doc1_onstoragecommit);
    set_onstorage_expect(NULL, NULL, NULL);
    pump_msgs(&called_doc1_onstoragecommit);
    CHECK_CALLED(doc1_onstoragecommit);

    value = SysAllocString(L"wine");
    hres = IHTMLStorage_setItem(local_storage[1], key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    SysFreeString(value);
    SET_EXPECT(doc1_onstoragecommit);  /* Native sometimes calls this here for some reason */
    SET_EXPECT(doc2_onstoragecommit);
    set_onstorage_expect(NULL, NULL, NULL);
    pump_msgs(&called_doc2_onstoragecommit);
    CHECK_CALLED(doc2_onstoragecommit);
    CLEAR_CALLED(doc1_onstoragecommit);

    hres = IHTMLStorage_removeItem(local_storage[0], key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);
    SysFreeString(key);
    SET_EXPECT(doc1_onstoragecommit);
    set_onstorage_expect(NULL, NULL, NULL);
    pump_msgs(&called_doc1_onstoragecommit);
    CHECK_CALLED(doc1_onstoragecommit);

    key = SysAllocString(L"winetest");
    value = SysAllocString(L"WineTest");
    hres = IHTMLStorage_setItem(local_storage[1], key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    SysFreeString(value);
    SysFreeString(key);
    SET_EXPECT(doc2_onstoragecommit);
    set_onstorage_expect(NULL, NULL, NULL);
    pump_msgs(&called_doc2_onstoragecommit);
    CHECK_CALLED(doc2_onstoragecommit);

    hres = IHTMLStorage_clear(local_storage[0]);
    ok(hres == S_OK, "clear failed: %08lx\n", hres);
    SET_EXPECT(doc1_onstoragecommit);
    set_onstorage_expect(NULL, NULL, NULL);
    pump_msgs(&called_doc1_onstoragecommit);
    CHECK_CALLED(doc1_onstoragecommit);

    /* session storage */
    key = SysAllocString(L"foobar");
    hres = IHTMLStorage_removeItem(session_storage[0], key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);

    value = SysAllocString(L"BarFoo");
    hres = IHTMLStorage_setItem(session_storage[0], key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    SysFreeString(value);
    if(document_mode >= 9) SET_EXPECT(window1_onstorage);
    SET_EXPECT(doc1_onstorage);
    set_onstorage_expect(L"foobar", NULL, L"BarFoo");
    pump_msgs(&called_doc1_onstorage);
    CHECK_CALLED(doc1_onstorage);
    if(document_mode >= 9) CHECK_CALLED(window1_onstorage);

    value = SysAllocString(L"barfoo");
    hres = IHTMLStorage_setItem(session_storage[1], key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    SysFreeString(value);
    if(document_mode >= 9) SET_EXPECT(window2_onstorage);
    SET_EXPECT(doc2_onstorage);
    set_onstorage_expect(L"foobar", L"BarFoo", L"barfoo");
    pump_msgs(&called_doc2_onstorage);
    CHECK_CALLED(doc2_onstorage);
    if(document_mode >= 9) CHECK_CALLED(window2_onstorage);

    hres = IHTMLStorage_removeItem(session_storage[0], key);
    ok(hres == S_OK, "removeItem failed: %08lx\n", hres);
    SysFreeString(key);
    if(document_mode >= 9) SET_EXPECT(window1_onstorage);
    SET_EXPECT(doc1_onstorage);
    set_onstorage_expect(L"foobar", L"barfoo", NULL);
    pump_msgs(&called_doc1_onstorage);
    CHECK_CALLED(doc1_onstorage);
    if(document_mode >= 9) CHECK_CALLED(window1_onstorage);

    key = SysAllocString(L"winetest");
    value = SysAllocString(L"WineTest");
    hres = IHTMLStorage_setItem(session_storage[1], key, value);
    ok(hres == S_OK, "setItem failed: %08lx\n", hres);
    SysFreeString(value);
    SysFreeString(key);
    if(document_mode >= 9) SET_EXPECT(window2_onstorage);
    SET_EXPECT(doc2_onstorage);
    set_onstorage_expect(L"winetest", NULL, L"WineTest");
    pump_msgs(&called_doc2_onstorage);
    CHECK_CALLED(doc2_onstorage);
    if(document_mode >= 9) CHECK_CALLED(window2_onstorage);

    hres = IHTMLStorage_clear(session_storage[0]);
    ok(hres == S_OK, "clear failed: %08lx\n", hres);
    if(document_mode >= 9) SET_EXPECT(window1_onstorage);
    SET_EXPECT(doc1_onstorage);
    set_onstorage_expect(NULL, NULL, NULL);
    pump_msgs(&called_doc1_onstorage);
    CHECK_CALLED(doc1_onstorage);
    if(document_mode >= 9) CHECK_CALLED(window1_onstorage);

    set_client_site(doc[0], FALSE);
    set_client_site(doc[1], FALSE);
    IHTMLDocument2_Release(doc[0]);
    IHTMLDocument2_Release(doc[1]);
}

static BOOL check_ie(void)
{
    IHTMLDocument2 *doc;
    IHTMLDocument5 *doc5;
    IHTMLDocument7 *doc7;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return FALSE;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument7, (void**)&doc7);
    if(SUCCEEDED(hres)) {
        is_ie9plus = TRUE;
        IHTMLDocument7_Release(doc7);
    }

    trace("is_ie9plus %x\n", is_ie9plus);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument5, (void**)&doc5);
    if(SUCCEEDED(hres))
        IHTMLDocument5_Release(doc5);

    IHTMLDocument2_Release(doc);
    return SUCCEEDED(hres);
}

START_TEST(events)
{
    CoInitialize(NULL);

    if(check_ie()) {
        container_hwnd = create_container_window();

        if(winetest_interactive)
            ShowWindow(container_hwnd, SW_SHOW);

        run_test(empty_doc_str, test_timeout);
        run_test(empty_doc_str, test_onclick);
        run_test(empty_doc_ie9_str, test_onclick);
        run_test(readystate_doc_str, test_onreadystatechange);
        run_test(readystate_doc_ie9_str, test_onreadystatechange);
        run_test(img_doc_str, test_imgload);
        run_test(link_doc_str, test_link_load);
        run_test(input_doc_str, test_focus);
        run_test(empty_doc_str, test_submit);
        run_test(empty_doc_ie9_str, test_submit);
        run_test(iframe_doc_str, test_iframe_connections);
        if(is_ie9plus)
            run_test(empty_doc_ie9_str, test_create_event);

        test_empty_document();
        test_storage_events(empty_doc_str);
        if(is_ie9plus)
            test_storage_events(empty_doc_ie9_str);

        DestroyWindow(container_hwnd);
    }else {
#if !defined(__i386__) && !defined(__x86_64__)
        todo_wine
#endif
        win_skip("Too old IE\n");
    }

    CoUninitialize();
}
