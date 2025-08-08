/*
 * Unit tests for miscellaneous vccorlib functions
 *
 * Copyright 2025 Piotr Caban
 * Copyright 2025 Vibhav Pant
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

#include "initguid.h"
#include "activation.h"
#include "objbase.h"
#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE; static unsigned int called_ ## func = 0

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func++; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func, n) \
    do { \
        ok(called_ ## func == n, "expected " #func " called %u times, got %u\n", n, called_ ## func); \
        expect_ ## func = FALSE; \
        called_ ## func = 0; \
    }while(0)

DEFINE_EXPECT(PreInitialize);
DEFINE_EXPECT(PostInitialize);
DEFINE_EXPECT(PreUninitialize);
DEFINE_EXPECT(PostUninitialize);

static HRESULT (__cdecl *pInitializeData)(int);
static void (__cdecl *pUninitializeData)(int);
static HRESULT (WINAPI *pGetActivationFactoryByPCWSTR)(const WCHAR *, const GUID *, void **);
static HRESULT (WINAPI *pGetIidsFn)(UINT32, UINT32 *, const GUID *, GUID **);

static BOOL init(void)
{
    HMODULE hmod = LoadLibraryA("vccorlib140.dll");

    if (!hmod)
    {
        win_skip("vccorlib140.dll not available\n");
        return FALSE;
    }

    pInitializeData = (void *)GetProcAddress(hmod,
            "?InitializeData@Details@Platform@@YAJH@Z");
    ok(pInitializeData != NULL, "InitializeData not available\n");
    pUninitializeData = (void *)GetProcAddress(hmod,
            "?UninitializeData@Details@Platform@@YAXH@Z");
    ok(pUninitializeData != NULL, "UninitializeData not available\n");

#ifdef __arm__
    pGetActivationFactoryByPCWSTR = (void *)GetProcAddress(hmod,
            "?GetActivationFactoryByPCWSTR@@YAJPAXAAVGuid@Platform@@PAPAX@Z");
    pGetIidsFn = (void *)GetProcAddress(hmod, "?GetIidsFn@@YAJHPAKPBU__s_GUID@@PAPAVGuid@Platform@@@Z");
#else
    if (sizeof(void *) == 8)
    {
        pGetActivationFactoryByPCWSTR = (void *)GetProcAddress(hmod,
                "?GetActivationFactoryByPCWSTR@@YAJPEAXAEAVGuid@Platform@@PEAPEAX@Z");
        pGetIidsFn = (void *)GetProcAddress(hmod, "?GetIidsFn@@YAJHPEAKPEBU__s_GUID@@PEAPEAVGuid@Platform@@@Z");
    }
    else
    {
        pGetActivationFactoryByPCWSTR = (void *)GetProcAddress(hmod,
                "?GetActivationFactoryByPCWSTR@@YGJPAXAAVGuid@Platform@@PAPAX@Z");
        pGetIidsFn = (void *)GetProcAddress(hmod, "?GetIidsFn@@YGJHPAKPBU__s_GUID@@PAPAVGuid@Platform@@@Z");
    }
#endif
    ok(pGetActivationFactoryByPCWSTR != NULL, "GetActivationFactoryByPCWSTR not available\n");
    ok(pGetIidsFn != NULL, "GetIidsFn not available\n");

    return TRUE;
}

static HRESULT WINAPI InitializeSpy_QI(IInitializeSpy *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IInitializeSpy) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IInitializeSpy_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InitializeSpy_AddRef(IInitializeSpy *iface)
{
    return 2;
}

static ULONG WINAPI InitializeSpy_Release(IInitializeSpy *iface)
{
    return 1;
}

static DWORD exp_coinit;
static HRESULT WINAPI InitializeSpy_PreInitialize(IInitializeSpy *iface, DWORD coinit, DWORD aptrefs)
{
    CHECK_EXPECT(PreInitialize);
    ok(coinit == exp_coinit, "coinit = %lx\n", coinit);
    return S_OK;
}

static HRESULT WINAPI InitializeSpy_PostInitialize(IInitializeSpy *iface, HRESULT hr, DWORD coinit, DWORD aptrefs)
{
    CHECK_EXPECT(PostInitialize);
    return hr;
}

static HRESULT WINAPI InitializeSpy_PreUninitialize(IInitializeSpy *iface, DWORD aptrefs)
{
    CHECK_EXPECT(PreUninitialize);
    return S_OK;
}

static HRESULT WINAPI InitializeSpy_PostUninitialize(IInitializeSpy *iface, DWORD aptrefs)
{
    CHECK_EXPECT(PostUninitialize);
    return S_OK;
}

static const IInitializeSpyVtbl InitializeSpyVtbl =
{
    InitializeSpy_QI,
    InitializeSpy_AddRef,
    InitializeSpy_Release,
    InitializeSpy_PreInitialize,
    InitializeSpy_PostInitialize,
    InitializeSpy_PreUninitialize,
    InitializeSpy_PostUninitialize
};

static IInitializeSpy InitializeSpy = { &InitializeSpyVtbl };

static void test_InitializeData(void)
{
    ULARGE_INTEGER cookie;
    HRESULT hr;

    hr = CoRegisterInitializeSpy(&InitializeSpy, &cookie);
    ok(hr == S_OK, "CoRegisterInitializeSpy returned %lx\n", hr);

    hr = pInitializeData(0);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);

    exp_coinit = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE;
    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(1);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(1);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);

    exp_coinit = COINIT_MULTITHREADED;
    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(2);
    ok(hr == RPC_E_CHANGED_MODE, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);

    pUninitializeData(0);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(1);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(2);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(2);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(2);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(3);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(3);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);

    hr = CoRevokeInitializeSpy(cookie);
    ok(hr == S_OK, "CoRevokeInitializeSpy returned %lx\n", hr);
}

static const GUID guid_null = {0};

static void test_GetActivationFactoryByPCWSTR(void)
{
    HRESULT hr;
    void *out;

    hr = pGetActivationFactoryByPCWSTR(L"Wine.Nonexistent.RuntimeClass", &IID_IActivationFactory, &out);
    ok(hr == CO_E_NOTINITIALIZED, "got hr %#lx\n", hr);

    hr = pInitializeData(1);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    hr = pGetActivationFactoryByPCWSTR(L"Wine.Nonexistent.RuntimeClass", &IID_IActivationFactory, &out);
    ok(hr == REGDB_E_CLASSNOTREG, "got hr %#lx\n", hr);

    hr = pGetActivationFactoryByPCWSTR(L"Windows.Foundation.Metadata.ApiInformation", &IID_IActivationFactory, &out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IActivationFactory_Release(out);

    hr = pGetActivationFactoryByPCWSTR(L"Windows.Foundation.Metadata.ApiInformation", &IID_IInspectable, &out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IActivationFactory_Release(out);

    hr = pGetActivationFactoryByPCWSTR(L"Windows.Foundation.Metadata.ApiInformation", &guid_null, &out);
    ok(hr == E_NOINTERFACE, "got hr %#lx\n", hr);

    pUninitializeData(1);
}

static void test_GetIidsFn(void)
{
    static const GUID guids_src[] = {IID_IUnknown, IID_IInspectable, IID_IAgileObject, IID_IMarshal, guid_null};
    GUID *guids_dest;
    UINT32 copied;
    HRESULT hr;

    guids_dest = NULL;
    copied = 0xdeadbeef;
    hr = pGetIidsFn(0, &copied, NULL, &guids_dest);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(copied == 0, "got copied %I32u\n", copied);
    ok(guids_dest != NULL, "got guids_dest %p\n", guids_dest);
    CoTaskMemFree(guids_dest);

    guids_dest = NULL;
    copied = 0;
    hr = pGetIidsFn(ARRAY_SIZE(guids_src), &copied, guids_src, &guids_dest);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(copied == ARRAY_SIZE(guids_src), "got copied %I32u\n", copied);
    ok(guids_dest != NULL, "got guids_dest %p\n", guids_dest);
    ok(!memcmp(guids_src, guids_dest, sizeof(*guids_dest) * copied), "unexpected guids_dest value.\n");
    CoTaskMemFree(guids_dest);
}

START_TEST(vccorlib)
{
    if(!init())
        return;

    test_InitializeData();
    test_GetActivationFactoryByPCWSTR();
    test_GetIidsFn();
}
