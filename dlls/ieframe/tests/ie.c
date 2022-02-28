/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "exdisp.h"
#include "exdispid.h"
#include "mshtml.h"
#include "initguid.h"
#include "ieautomation.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
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

DEFINE_EXPECT(Invoke_NAVIGATECOMPLETE2);

static BOOL navigate_complete;

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_NAVIGATECOMPLETE2:
        CHECK_EXPECT(Invoke_NAVIGATECOMPLETE2);
        navigate_complete = TRUE;
        return S_OK;
    }

    return E_NOTIMPL;
}

static IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static void advise_cp(IUnknown *unk, BOOL init)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    static DWORD cookie = 100;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DWebBrowserEvents2, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(init) {
        hres = IConnectionPoint_Advise(point, (IUnknown*)&Dispatch, &cookie);
        ok(hres == S_OK, "Advise failed: %08lx\n", hres);
    }else {
        hres = IConnectionPoint_Unadvise(point, cookie);
        ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);
    }

    IConnectionPoint_Release(point);

}

static void test_visible(IWebBrowser2 *wb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0x100;
    hres = IWebBrowser2_get_Visible(wb, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "Visible = %x\n", b);

    hres = IWebBrowser2_put_Visible(wb, VARIANT_TRUE);
    ok(hres == S_OK, "put_Visible failed: %08lx\n", hres);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(wb, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    hres = IWebBrowser2_put_Visible(wb, VARIANT_FALSE);
    ok(hres == S_OK, "put_Visible failed: %08lx\n", hres);
}

static void test_html_window(IWebBrowser2 *wb)
{
    IHTMLWindow2 *html_window;
    IServiceProvider *sp;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

    hres = IServiceProvider_QueryService(sp, &SID_SHTMLWindow, &IID_IHTMLWindow2, (void**)&html_window);
    IServiceProvider_Release(sp);
    ok(hres == S_OK, "Could not get SHTMLWindow service: %08lx\n", hres);

    IHTMLWindow2_Release(html_window);
}

static void test_window(IWebBrowser2 *wb)
{
    SHANDLE_PTR handle = 0;
    HWND hwnd = NULL;
    char buf[100];
    HRESULT hres;

    hres = IWebBrowser2_get_HWND(wb, &handle);
    ok(hres == S_OK, "get_HWND failed: %08lx\n", hres);
    ok(handle, "handle == 0\n");

    hwnd = (HWND)handle;
    GetClassNameA(hwnd, buf, sizeof(buf));
    ok(!strcmp(buf, "IEFrame"), "Unexpected class name %s\n", buf);
}

static void test_navigate(IWebBrowser2 *wb, const WCHAR *url)
{
    VARIANT urlv, emptyv;
    MSG msg;
    HRESULT hres;

    SET_EXPECT(Invoke_NAVIGATECOMPLETE2);

    V_VT(&urlv) = VT_BSTR;
    V_BSTR(&urlv) = SysAllocString(url);
    V_VT(&emptyv) = VT_EMPTY;
    hres = IWebBrowser2_Navigate2(wb, &urlv, &emptyv, &emptyv, &emptyv, &emptyv);
    ok(hres == S_OK, "Navigate2 failed: %08lx\n", hres);
    SysFreeString(V_BSTR(&urlv));

    while(!navigate_complete && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CHECK_CALLED(Invoke_NAVIGATECOMPLETE2);
}

static void test_busy(IWebBrowser2 *wb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0xdead;
    hres = IWebBrowser2_get_Busy(wb, &b);
    ok(hres == S_OK, "get_Busy failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "Busy = %x\n", b);
}

static void test_InternetExplorer(void)
{
    IWebBrowser2 *wb;
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_InternetExplorer, NULL, CLSCTX_SERVER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Could not create InternetExplorer instance: %08lx\n", hres);

    if(hres != S_OK)
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 interface: %08lx\n", hres);
    if (hres != S_OK) {
        IUnknown_Release(unk);
        return;
    }

    advise_cp(unk, TRUE);

    test_busy(wb);
    test_visible(wb);
    test_html_window(wb);
    test_window(wb);
    test_navigate(wb, L"http://test.winehq.org/tests/hello.html");

    advise_cp(unk, FALSE);

    IWebBrowser2_Release(wb);
    ref = IUnknown_Release(unk);
    ok(!ref, "object not destroyed, ref=%lu\n", ref);
}

static void test_InternetExplorerManager(void)
{
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_InternetExplorerManager, NULL, CLSCTX_LOCAL_SERVER,
            &IID_IInternetExplorerManager, (void**)&unk);
    ok(hres == S_OK || broken(hres == REGDB_E_CLASSNOTREG), "Could not create InternetExplorerManager instance: %08lx\n", hres);

    if(hres != S_OK)
    {
        win_skip("InternetExplorerManager not available\n");
        return;
    }

    ref = IUnknown_Release(unk);
    ok(!ref, "object not destroyed, ref=%lu\n", ref);
}

START_TEST(ie)
{
    CoInitialize(NULL);

    test_InternetExplorerManager();

    test_InternetExplorer();

    CoUninitialize();
}
