/*
 * Component Object Tests
 *
 * Copyright 2005 Robert Shearman
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlguid.h"

#include "wine/test.h"

/* functions that are not present on all versions of Windows */
HRESULT (WINAPI * pCoInitializeEx)(LPVOID lpReserved, DWORD dwCoInit);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08lx\n", hr)

static const CLSID CLSID_non_existent =   { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const CLSID CLSID_CDeviceMoniker = { 0x4315d437, 0x5b8c, 0x11d0, { 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86 } };
static const WCHAR devicedotone[] = {'d','e','v','i','c','e','.','1',0};
static const WCHAR wszCLSID_CDeviceMoniker[] =
{
    '{',
    '4','3','1','5','d','4','3','7','-',
    '5','b','8','c','-',
    '1','1','d','0','-',
    'b','d','3','b','-',
    '0','0','a','0','c','9','1','1','c','e','8','6',
    '}',0
};

static void test_ProgIDFromCLSID(void)
{
    LPWSTR progid;
    HRESULT hr = ProgIDFromCLSID(&CLSID_CDeviceMoniker, &progid);
    ok(hr == S_OK, "ProgIDFromCLSID failed with error 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpiW(progid, devicedotone), "Didn't get expected prog ID\n");
        CoTaskMemFree(progid);
    }

    progid = (LPWSTR)0xdeadbeef;
    hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
    ok(hr == REGDB_E_CLASSNOTREG, "ProgIDFromCLSID returned %08lx\n", hr);
    ok(progid == NULL, "ProgIDFromCLSID returns with progid %p\n", progid);
}

static void test_CLSIDFromProgID(void)
{
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(devicedotone, &clsid);
    ok(hr == S_OK, "CLSIDFromProgID failed with error 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");

    hr = CLSIDFromString((LPOLESTR)devicedotone, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");
}

static void test_CLSIDFromString(void)
{
    CLSID clsid;
    HRESULT hr = CLSIDFromString((LPOLESTR)wszCLSID_CDeviceMoniker, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");
}

static void test_CoCreateInstance(void)
{
    REFCLSID rclsid = &CLSID_MyComputer;
    IUnknown *pUnk = (IUnknown *)0xdeadbeef;
    HRESULT hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);
    ok(pUnk == NULL, "CoCreateInstance should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    OleInitialize(NULL);
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok_ole_success(hr, "CoCreateInstance");
    IUnknown_Release(pUnk);
    OleUninitialize();

    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);
}

static void test_CoGetClassObject(void)
{
    IUnknown *pUnk = (IUnknown *)0xdeadbeef;
    HRESULT hr = CoGetClassObject(&CLSID_MyComputer, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetClassObject should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);
    ok(pUnk == NULL, "CoGetClassObject should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    hr = CoGetClassObject(&CLSID_MyComputer, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG, "CoGetClassObject should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
}

static ATOM register_dummy_class(void)
{
    WNDCLASS wc =
    {
        0,
        DefWindowProc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        TEXT("WineOleTestClass"),
    };
    
    return RegisterClass(&wc);
}

static void test_ole_menu(void)
{
	HWND hwndFrame;
	HRESULT hr;

	hwndFrame = CreateWindow(MAKEINTATOM(register_dummy_class()), "Test", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	hr = OleSetMenuDescriptor(NULL, hwndFrame, NULL, NULL, NULL);
	todo_wine ok_ole_success(hr, "OleSetMenuDescriptor");

	DestroyWindow(hwndFrame);
}


static HRESULT WINAPI MessageFilter_QueryInterface(IMessageFilter *iface, REFIID riid, void ** ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = (LPVOID)iface;
        IMessageFilter_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI MessageFilter_AddRef(IMessageFilter *iface)
{
    return 2; /* non-heap object */
}

static ULONG WINAPI MessageFilter_Release(IMessageFilter *iface)
{
    return 1; /* non-heap object */
}

static DWORD WINAPI MessageFilter_HandleInComingCall(
  IMessageFilter *iface,
  DWORD dwCallType,
  HTASK threadIDCaller,
  DWORD dwTickCount,
  LPINTERFACEINFO lpInterfaceInfo)
{
    trace("HandleInComingCall\n");
    return SERVERCALL_ISHANDLED;
}

static DWORD WINAPI MessageFilter_RetryRejectedCall(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwRejectType)
{
    trace("RetryRejectedCall\n");
    return 0;
}

static DWORD WINAPI MessageFilter_MessagePending(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwPendingType)
{
    trace("MessagePending\n");
    return PENDINGMSG_WAITNOPROCESS;
}

static const IMessageFilterVtbl MessageFilter_Vtbl =
{
    MessageFilter_QueryInterface,
    MessageFilter_AddRef,
    MessageFilter_Release,
    MessageFilter_HandleInComingCall,
    MessageFilter_RetryRejectedCall,
    MessageFilter_MessagePending
};

static IMessageFilter MessageFilter = { &MessageFilter_Vtbl };

static void test_CoRegisterMessageFilter(void)
{
    HRESULT hr;
    IMessageFilter *prev_filter;

#if 0 /* crashes without an apartment! */
    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
#endif

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);
    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok(hr == CO_E_NOT_SUPPORTED,
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08lx\n",
        hr);
    ok(prev_filter == (IMessageFilter *)0xdeadbeef,
        "prev_filter should have been set to %p\n", prev_filter);
    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, "CoRegisterMessageFilter");

    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(NULL, &prev_filter);
    ok_ole_success(hr, "CoRegisterMessageFilter");
    ok(prev_filter == NULL, "prev_filter should have been set to NULL instead of %p\n", prev_filter);

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok_ole_success(hr, "CoRegisterMessageFilter");
    ok(prev_filter == NULL, "prev_filter should have been set to NULL instead of %p\n", prev_filter);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, "CoRegisterMessageFilter");

    CoUninitialize();
}

START_TEST(compobj)
{
    HMODULE hOle32 = GetModuleHandle("ole32");
    if (!(pCoInitializeEx = (void*)GetProcAddress(hOle32, "CoInitializeEx")))
    {
        trace("You need DCOM95 installed to run this test\n");
        return;
    }

    test_ProgIDFromCLSID();
    test_CLSIDFromProgID();
    test_CLSIDFromString();
    test_CoCreateInstance();
    test_ole_menu();
    test_CoGetClassObject();
    test_CoRegisterMessageFilter();
}
