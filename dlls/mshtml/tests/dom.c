/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include "docobj.h"

static const char doc_str1[] = "<html><body>test</body></html>";
static const char doc_str2[] =
    "<html><body>test a<font size=\"2\">bc 123<br />it's </font>text<br /></body></html>";

static WCHAR characterW[] = {'c','h','a','r','a','c','t','e','r',0};
static WCHAR wordW[] = {'w','o','r','d',0};

static const char *dbgstr_w(LPCWSTR str)
{
    static char buf[512];
    if(!str)
        return "(null)";
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    WCHAR buf[512];
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, sizeof(buf)/sizeof(WCHAR));
    return lstrcmpW(strw, buf);
}

static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    return doc;
}

#define test_node_name(u,n) _test_node_name(__LINE__,u,n)
static void _test_node_name(unsigned line, IUnknown *unk, const char *exname)
{
    IHTMLDOMNode *node;
    BSTR name;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMNode, (void**)&node);
    ok_(__FILE__, line) (hres == S_OK, "QueryInterface(IID_IHTMLNode) failed: %08x\n", hres);

    hres = IHTMLDOMNode_get_nodeName(node, &name);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__, line) (hres == S_OK, "get_nodeName failed: %08x\n", hres);
    ok_(__FILE__, line) (!strcmp_wa(name, exname), "got name: %s, expected HTML\n", dbgstr_w(name));

    SysFreeString(name);
}

static void test_doc_elem(IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument3) failed: %08x\n", hres);

    hres = IHTMLDocument3_get_documentElement(doc3, &elem);
    IHTMLDocument3_Release(doc3);
    ok(hres == S_OK, "get_documentElement failed: %08x\n", hres);

    test_node_name((IUnknown*)elem, "HTML");

    IHTMLElement_Release(elem);
}

#define test_range_text(r,t) _test_range_text(__LINE__,r,t)
static void _test_range_text(unsigned line, IHTMLTxtRange *range, const char *extext)
{
    BSTR text;
    HRESULT hres;

    hres = IHTMLTxtRange_get_text(range, &text);
    ok_(__FILE__, line) (hres == S_OK, "get_text failed: %08x\n", hres);

    if(extext) {
        ok_(__FILE__, line) (text != NULL, "text == NULL\n");
        ok_(__FILE__, line) (!strcmp_wa(text, extext), "text=\"%s\", expected \"%s\"\n", dbgstr_w(text), extext);
    }else {
        ok_(__FILE__, line) (text == NULL, "text=\"%s\", expected NULL\n", dbgstr_w(text));
    }

    SysFreeString(text);

}

#define test_range_collapse(r,b) _test_range_collapse(__LINE__,r,b)
static void _test_range_collapse(unsigned line, IHTMLTxtRange *range, BOOL b)
{
    HRESULT hres;

    hres = IHTMLTxtRange_collapse(range, b);
    ok_(__FILE__, line) (hres == S_OK, "collapse failed: %08x\n", hres);
    _test_range_text(line, range, NULL);
}

#define test_range_expand(r,u,b,t) _test_range_expand(__LINE__,r,u,b,t)
static void _test_range_expand(unsigned line, IHTMLTxtRange *range, LPWSTR unit,
        VARIANT_BOOL exb, const char *extext)
{
    VARIANT_BOOL b = 0xe0e0;
    HRESULT hres;

    hres = IHTMLTxtRange_expand(range, unit, &b);
    ok_(__FILE__,line) (hres == S_OK, "expand failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "b=%x, expected %x\n", b, exb);
    _test_range_text(line, range, extext);
}

#define test_range_move(r,u,c,e) _test_range_move(__LINE__,r,u,c,e)
static void _test_range_move(unsigned line, IHTMLTxtRange *range, LPWSTR unit, long cnt, long excnt)
{
    long c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_move(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%ld, expected %ld\n", c, excnt);
    _test_range_text(line, range, NULL);
}

#define test_range_inrange(r1,r2,b) _test_range_inrange(__LINE__,r1,r2,b)
static void _test_range_inrange(unsigned line, IHTMLTxtRange *range1, IHTMLTxtRange *range2, VARIANT_BOOL exb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0xe0e0;
    hres = IHTMLTxtRange_inRange(range1, range2, &b);
    ok_(__FILE__,line) (hres == S_OK, "(1->2) isEqual failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "(1->2) b=%x, expected %x\n", b, exb);
}

#define test_range_isequal(r1,r2,b) _test_range_isequal(__LINE__,r1,r2,b)
static void _test_range_isequal(unsigned line, IHTMLTxtRange *range1, IHTMLTxtRange *range2, VARIANT_BOOL exb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0xe0e0;
    hres = IHTMLTxtRange_isEqual(range1, range2, &b);
    ok_(__FILE__,line) (hres == S_OK, "(1->2) isEqual failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "(1->2) b=%x, expected %x\n", b, exb);

    b = 0xe0e0;
    hres = IHTMLTxtRange_isEqual(range2, range1, &b);
    ok_(__FILE__,line) (hres == S_OK, "(2->1) isEqual failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "(2->1) b=%x, expected %x\n", b, exb);

    if(exb) {
        test_range_inrange(range1, range2, VARIANT_TRUE);
        test_range_inrange(range2, range1, VARIANT_TRUE);
    }
}

static void test_txtrange(IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    IHTMLBodyElement *body;
    IHTMLTxtRange *body_range, *range, *range2;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLBodyElement, (void**)&body);
    IHTMLElement_Release(elem);

    hres = IHTMLBodyElement_createTextRange(body, &body_range);
    IHTMLBodyElement_Release(body);
    ok(hres == S_OK, "createTextRange failed: %08x\n", hres);

    test_range_text(body_range, "test abc 123\r\nit's text");

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    hres = IHTMLTxtRange_duplicate(body_range, &range2);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);
    test_range_isequal(range, range2, VARIANT_TRUE);

    test_range_text(range, "test abc 123\r\nit's text");
    test_range_text(body_range, "test abc 123\r\nit's text");

    test_range_collapse(range, TRUE);
    test_range_isequal(range, range2, VARIANT_FALSE);
    test_range_inrange(range, range2, VARIANT_FALSE);
    test_range_inrange(range2, range, VARIANT_TRUE);
    IHTMLTxtRange_Release(range2);

    test_range_expand(range, wordW, VARIANT_TRUE, "test ");
    test_range_expand(range, wordW, VARIANT_FALSE, "test ");
    test_range_move(range, characterW, 2, 2);
    test_range_expand(range, wordW, VARIANT_TRUE, "test ");

    test_range_collapse(range, FALSE);
    test_range_expand(range, wordW, VARIANT_TRUE, "abc ");

    test_range_collapse(range, FALSE);
    test_range_expand(range, wordW, VARIANT_TRUE, "123");
    test_range_expand(range, wordW, VARIANT_FALSE, "123");
    test_range_move(range, characterW, 2, 2);
    test_range_expand(range, wordW, VARIANT_TRUE, "123");

    IHTMLTxtRange_Release(range);

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    test_range_text(range, "test abc 123\r\nit's text");
    test_range_move(range, characterW, 3, 3);

    IHTMLTxtRange_Release(range);
    IHTMLTxtRange_Release(body_range);
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

        static const WCHAR completeW[] = {'c','o','m','p','l','e','t','e',0};

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);

        if(!lstrcmpW(state, completeW))
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

static IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

static IHTMLDocument2 *create_doc_with_string(const char *str)
{
    IPersistStreamInit *init;
    IStream *stream;
    IHTMLDocument2 *doc;
    HGLOBAL mem;
    SIZE_T len;

    notif_doc = doc = create_document();
    if(!doc)
        return NULL;

    doc_complete = FALSE;
    len = strlen(str);
    mem = GlobalAlloc(0, len);
    memcpy(mem, str, len);
    CreateStreamOnHGlobal(mem, TRUE, &stream);

    IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);

    IPersistStreamInit_Load(init, stream);
    IPersistStreamInit_Release(init);
    IStream_Release(stream);

    return doc;
}

static void do_advise(IUnknown *unk, REFIID riid, IUnknown *unk_advise)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    hres = IConnectionPoint_Advise(cp, unk_advise, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);
}

typedef void (*domtest_t)(IHTMLDocument2*);

static void run_domtest(const char *str, domtest_t test)
{
    IHTMLDocument2 *doc;
    ULONG ref;
    MSG msg;

    doc = create_doc_with_string(str);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    test(doc);

    ref = IHTMLDocument2_Release(doc);
    ok(!ref, "ref = %d\n", ref);
}

static void gecko_installer_workaround(BOOL disable)
{
    HKEY hkey;
    DWORD res;

    static BOOL has_url = FALSE;
    static char url[2048];

    if(!disable && !has_url)
        return;

    res = RegOpenKey(HKEY_CURRENT_USER, "Software\\Wine\\MSHTML", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    if(disable) {
        DWORD type, size = sizeof(url);

        res = RegQueryValueEx(hkey, "GeckoUrl", NULL, &type, (PVOID)url, &size);
        if(res == ERROR_SUCCESS && type == REG_SZ)
            has_url = TRUE;

        RegDeleteValue(hkey, "GeckoUrl");
    }else {
        RegSetValueEx(hkey, "GeckoUrl", 0, REG_SZ, (PVOID)url, lstrlenA(url)+1);
    }

    RegCloseKey(hkey);
}

START_TEST(dom)
{
    gecko_installer_workaround(TRUE);
    CoInitialize(NULL);

    run_domtest(doc_str1, test_doc_elem);
    run_domtest(doc_str2, test_txtrange);

    CoUninitialize();
    gecko_installer_workaround(FALSE);
}
