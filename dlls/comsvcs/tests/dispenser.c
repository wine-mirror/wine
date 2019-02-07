/*
 * Copyright 2018 Alistair Leslie-Hughes
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
#include "ole2.h"
#include "comsvcs.h"
#include "msxml.h"

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
    trace(#func "\n"); \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(driver_CreateResource);
DEFINE_EXPECT(driver_DestroyResource);
DEFINE_EXPECT(driver_ResetResource);
DEFINE_EXPECT(driver_Release);

HRESULT driver_DestroyResource_ret = S_OK;

static HRESULT WINAPI driver_QueryInterface(IDispenserDriver *iface, REFIID riid, void **object)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispenserDriver))
    {
        *object = iface;

        return S_OK;
    }

    ok(0, "Unknown iterface %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI driver_AddRef(IDispenserDriver *iface)
{
    trace("AddRef\n");
    return 2;
}

static ULONG WINAPI driver_Release(IDispenserDriver *iface)
{
    CHECK_EXPECT2(driver_Release);
    return 1;
}

static HRESULT WINAPI driver_CreateResource(IDispenserDriver *iface, const RESTYPID restypid, RESID *resid, TIMEINSECS *destroy)
{
    CHECK_EXPECT2(driver_CreateResource);

    *resid = 10;
    return S_OK;
}

static HRESULT WINAPI driver_RateResource(IDispenserDriver *iface, const RESTYPID restypid, const RESID resid,
    const BOOL requires, RESOURCERATING *rating)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI driver_EnlistResource(IDispenserDriver *iface, const RESID resid, const TRANSID transid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI driver_ResetResource(IDispenserDriver *iface, const RESID resid)
{
    CHECK_EXPECT2(driver_ResetResource);
    ok((int)resid == 10, "RESID %d\n", (int)resid);
    return S_OK;
}

static HRESULT WINAPI driver_DestroyResource(IDispenserDriver *iface, const RESID resid)
{
    todo_wine CHECK_EXPECT2(driver_DestroyResource);
    return driver_DestroyResource_ret;
}

static HRESULT WINAPI driver_DestroyResourceS(IDispenserDriver *iface, const SRESID resid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}


static const struct IDispenserDriverVtbl driver_vtbl =
{
    driver_QueryInterface,
    driver_AddRef,
    driver_Release,
    driver_CreateResource,
    driver_RateResource,
    driver_EnlistResource,
    driver_ResetResource,
    driver_DestroyResource,
    driver_DestroyResourceS
};

static IDispenserDriver DispenserDriver = { &driver_vtbl };

static DWORD WINAPI com_thread(void *arg)
{
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    if (hr == S_OK) IUnknown_Release(unk);

    return hr;
}

static void create_dispenser(void)
{
    static const WCHAR pool0[] = {'S','C','.','P','o','o','l',' ','0',' ','0',0};
    static const WCHAR pool1[] = {'S','C','.','P','o','o','l',' ','1',' ','1',0};
    static const WCHAR data[]  = {'d','a','t','a','1',0};
    HRESULT hr;
    IDispenserManager *dispenser = NULL;
    IHolder *holder1 = NULL, *holder2 = NULL, *holder3 = NULL;
    HANDLE thread;
    RESID resid;
    DWORD ret;
    BSTR str;

    hr = CoCreateInstance( &CLSID_DispenserManager, NULL, CLSCTX_ALL, &IID_IDispenserManager, (void**)&dispenser);
    ok(hr == S_OK, "Failed to create object 0x%08x\n", hr);
    if(FAILED(hr))
    {
        win_skip("DispenserManager not available\n");
        return;
    }

    thread = CreateThread(NULL, 0, com_thread, NULL, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "wait failed\n");
    GetExitCodeThread(thread, &ret);
    ok(ret == CO_E_NOTINITIALIZED, "got unexpected hr %#x\n", ret);

    hr = IDispenserManager_RegisterDispenser(dispenser, &DispenserDriver, pool0, &holder1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* The above call creates an MTA thread, but we need to wait for it to
     * actually initialize. */
    Sleep(200);
    thread = CreateThread(NULL, 0, com_thread, NULL, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "wait failed\n");
    GetExitCodeThread(thread, &ret);
    ok(ret == S_OK, "got unexpected hr %#x\n", ret);

    hr = IDispenserManager_RegisterDispenser(dispenser, &DispenserDriver, pool0, &holder2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(holder1 != holder2, "same holder object returned\n");

    hr = IDispenserManager_RegisterDispenser(dispenser, &DispenserDriver, pool1, &holder3);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if(holder1)
    {
        SET_EXPECT(driver_CreateResource);
        SET_EXPECT(driver_Release);

        str = SysAllocString(data);
        hr = IHolder_AllocResource(holder1, (RESTYPID)str, &resid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(resid == 10, "got %d\n", (int)resid);
        SysFreeString(str);

        CHECK_CALLED(driver_CreateResource);
        todo_wine CHECK_CALLED_BROKEN(driver_Release);

        SET_EXPECT(driver_ResetResource);
        hr = IHolder_FreeResource(holder1, resid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        todo_wine CHECK_CALLED(driver_ResetResource);

        SET_EXPECT(driver_DestroyResource);
        SET_EXPECT(driver_Release);
        hr = IHolder_Close(holder1);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        CHECK_CALLED(driver_Release);
        CHECK_CALLED(driver_DestroyResource);

        IHolder_Release(holder1);
    }
    if(holder2)
    {
        SET_EXPECT(driver_CreateResource);
        SET_EXPECT(driver_Release);

        str = SysAllocString(data);
        hr = IHolder_AllocResource(holder2, (RESTYPID)str, &resid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(resid == 10, "got %d\n", (int)resid);
        SysFreeString(str);

        CHECK_CALLED(driver_CreateResource);
        todo_wine CHECK_CALLED_BROKEN(driver_Release);

        SET_EXPECT(driver_ResetResource);
        hr = IHolder_FreeResource(holder2, resid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        todo_wine CHECK_CALLED(driver_ResetResource);

        /* DestroyResource return doesn't directly affect the Holder Close return value */
        driver_DestroyResource_ret = E_FAIL;
        SET_EXPECT(driver_DestroyResource);
        SET_EXPECT(driver_Release);
        hr = IHolder_Close(holder2);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        CHECK_CALLED(driver_Release);
        CHECK_CALLED(driver_DestroyResource);
        driver_DestroyResource_ret = S_OK;
        IHolder_Release(holder2);
    }
    if(holder3)
    {
        SET_EXPECT(driver_Release);
        hr = IHolder_Close(holder3);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        CHECK_CALLED(driver_Release);
        IHolder_Release(holder3);
    }

    IDispenserManager_Release(dispenser);
}


START_TEST(dispenser)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_dispenser();

    CoUninitialize();
}
