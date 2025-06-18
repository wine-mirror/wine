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

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

struct test_driver
{
    IDispenserDriver IDispenserDriver_iface;
    HRESULT destroy_resource_hr;
};

static struct test_driver *impl_from_IDispenserDriver(IDispenserDriver *iface)
{
    return CONTAINING_RECORD(iface, struct test_driver, IDispenserDriver_iface);
}

static HRESULT WINAPI driver_QueryInterface(IDispenserDriver *iface, REFIID riid, void **object)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispenserDriver))
    {
        *object = iface;

        return S_OK;
    }

    ok(0, "Unknown interface %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI driver_AddRef(IDispenserDriver *iface)
{
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
    struct test_driver *driver = impl_from_IDispenserDriver(iface);
    todo_wine CHECK_EXPECT2(driver_DestroyResource);
    return driver->destroy_resource_hr;
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

static void init_test_driver(struct test_driver *driver)
{
    driver->IDispenserDriver_iface.lpVtbl = &driver_vtbl;
    driver->destroy_resource_hr = S_OK;
}

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
    HRESULT hr;
    IDispenserManager *dispenser = NULL;
    IHolder *holder1 = NULL, *holder2 = NULL, *holder3 = NULL;
    HANDLE thread;
    RESID resid;
    DWORD ret;
    BSTR str;
    struct test_driver driver;

    hr = CoCreateInstance( &CLSID_DispenserManager, NULL, CLSCTX_ALL, &IID_IDispenserManager, (void**)&dispenser);
    ok(hr == S_OK, "Failed to create object 0x%08lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("DispenserManager not available\n");
        return;
    }

    thread = CreateThread(NULL, 0, com_thread, NULL, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "wait failed\n");
    GetExitCodeThread(thread, &ret);
    ok(ret == CO_E_NOTINITIALIZED, "got unexpected hr %#lx\n", ret);
    CloseHandle(thread);

    init_test_driver(&driver);

    hr = IDispenserManager_RegisterDispenser(dispenser, &driver.IDispenserDriver_iface, L"SC.Pool 0 0", &holder1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* The above call creates an MTA thread, but we need to wait for it to
     * actually initialize. */
    Sleep(200);
    thread = CreateThread(NULL, 0, com_thread, NULL, 0, NULL);
    ok(!WaitForSingleObject(thread, 20000), "wait failed\n");
    GetExitCodeThread(thread, &ret);
    ok(ret == S_OK, "got unexpected hr %#lx\n", ret);
    CloseHandle(thread);

    hr = IDispenserManager_RegisterDispenser(dispenser, &driver.IDispenserDriver_iface, L"SC.Pool 0 0", &holder2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(holder1 != holder2, "same holder object returned\n");

    hr = IDispenserManager_RegisterDispenser(dispenser, &driver.IDispenserDriver_iface, L"SC.Pool 1 1", &holder3);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if(holder1)
    {
        SET_EXPECT(driver_CreateResource);
        SET_EXPECT(driver_Release);

        str = SysAllocString(L"data1");
        hr = IHolder_AllocResource(holder1, (RESTYPID)str, &resid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(resid == 10, "got %d\n", (int)resid);
        SysFreeString(str);

        CHECK_CALLED(driver_CreateResource);
        todo_wine CHECK_CALLED_BROKEN(driver_Release);

        SET_EXPECT(driver_ResetResource);
        hr = IHolder_FreeResource(holder1, resid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        todo_wine CHECK_CALLED(driver_ResetResource);

        SET_EXPECT(driver_DestroyResource);
        SET_EXPECT(driver_Release);
        hr = IHolder_Close(holder1);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(driver_Release);
        CHECK_CALLED(driver_DestroyResource);

        IHolder_Release(holder1);
    }
    if(holder2)
    {
        SET_EXPECT(driver_CreateResource);
        SET_EXPECT(driver_Release);

        str = SysAllocString(L"data1");
        hr = IHolder_AllocResource(holder2, (RESTYPID)str, &resid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(resid == 10, "got %d\n", (int)resid);
        SysFreeString(str);

        CHECK_CALLED(driver_CreateResource);
        todo_wine CHECK_CALLED_BROKEN(driver_Release);

        SET_EXPECT(driver_ResetResource);
        hr = IHolder_FreeResource(holder2, resid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        todo_wine CHECK_CALLED(driver_ResetResource);

        /* DestroyResource return doesn't directly affect the Holder Close return value */
        driver.destroy_resource_hr = E_FAIL;
        SET_EXPECT(driver_DestroyResource);
        SET_EXPECT(driver_Release);
        hr = IHolder_Close(holder2);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(driver_Release);
        CHECK_CALLED(driver_DestroyResource);
        driver.destroy_resource_hr = S_OK;
        IHolder_Release(holder2);
    }
    if(holder3)
    {
        SET_EXPECT(driver_Release);
        hr = IHolder_Close(holder3);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(driver_Release);
        IHolder_Release(holder3);
    }

    IDispenserManager_Release(dispenser);
}

static void test_new_moniker_serialize(const WCHAR *clsid, const WCHAR *progid, IMoniker *moniker)
{
    DWORD expected_size, progid_len = 0;
    ULARGE_INTEGER size;
    IStream *stream;
    HGLOBAL hglobal;
    CLSID guid;
    HRESULT hr;
    DWORD *ptr;

    hr = IMoniker_GetSizeMax(moniker, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    expected_size = sizeof(GUID) + 2 * sizeof(DWORD);
    if (progid)
    {
        progid_len = lstrlenW(progid) * sizeof(*progid);
        expected_size += progid_len;
    }

    size.QuadPart = 0;
    hr = IMoniker_GetSizeMax(moniker, &size);
    ok(hr == S_OK, "Failed to get size, hr %#lx.\n", hr);
    ok(size.QuadPart == expected_size, "Unexpected size %s, expected %#lx.\n", wine_dbgstr_longlong(size.QuadPart),
            expected_size);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#lx.\n", hr);

    hr = IMoniker_Save(moniker, stream, FALSE);
    ok(hr == S_OK, "Failed to save moniker, hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "Failed to get a handle, hr %#lx.\n", hr);

    ptr = GlobalLock(hglobal);
    ok(!!ptr, "Failed to get data pointer.\n");

    hr = CLSIDFromString(clsid, &guid);
    ok(hr == S_OK, "Failed to get CLSID, hr %#lx.\n", hr);
    ok(IsEqualGUID((GUID *)ptr, &guid), "Unexpected buffer content.\n");
    ptr += sizeof(GUID)/sizeof(DWORD);

    /* Serialization format:

       GUID guid;
       DWORD progid_len;
       WCHAR progid[progid_len/2];
       DWORD null;
    */

    if (progid)
    {
        ok(*ptr == progid_len, "Unexpected progid length.\n");
        ptr++;
        ok(!memcmp(ptr, progid, progid_len), "Unexpected progid.\n");
        ptr += progid_len / sizeof(DWORD);
    }
    else
    {
        ok(*ptr == 0, "Unexpected progid length.\n");
        ptr++;
    }
    ok(*ptr == 0, "Unexpected terminator.\n");

    GlobalUnlock(hglobal);

    IStream_Release(stream);
}

static void test_new_moniker(void)
{
    IMoniker *moniker, *moniker2, *inverse, *class_moniker, *moniker_left;
    IRunningObjectTable *rot;
    IUnknown *obj, *obj2;
    BIND_OPTS2 bind_opts;
    DWORD moniker_type;
    IROTData *rot_data;
    IBindCtx *bindctx;
    FILETIME filetime;
    DWORD hash, eaten;
    CLSID clsid;
    HRESULT hr;
    WCHAR *str;

    hr = CreateBindCtx(0, &bindctx);
    ok(hr == S_OK, "Failed to create bind context, hr %#lx.\n", hr);

    eaten = 0;
    hr = MkParseDisplayName(bindctx, L"new:20d04fe0-3aea-1069-a2d8-08002b30309d", &eaten, &moniker);
    ok(hr == S_OK, "Failed to parse display name, hr %#lx.\n", hr);
    ok(eaten == 40, "Unexpected eaten length %lu.\n", eaten);

    hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rot_data);
    ok(hr == S_OK, "Failed to get IROTData, hr %#lx.\n", hr);
    IROTData_Release(rot_data);

    eaten = 0;
    hr = IMoniker_ParseDisplayName(moniker, bindctx, NULL, (WCHAR *)L"new:20d04fe0-3aea-1069-a2d8-08002b30309d",
            &eaten, &moniker2);
    ok(hr == S_OK, "Failed to parse display name, hr %#lx.\n", hr);
    ok(eaten == 40, "Unexpected eaten length %lu.\n", eaten);
    IMoniker_Release(moniker2);

    hr = IMoniker_QueryInterface(moniker, &IID_IParseDisplayName, (void **)&obj);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    /* Object creation. */
    hr = CLSIDFromProgID(L"new", &clsid);
    ok(hr == S_OK, "Failed to get clsid, hr %#lx.\n", hr);

    hr = CreateClassMoniker(&clsid, &class_moniker);
    ok(hr == S_OK, "Failed to create class moniker, hr %#lx.\n", hr);

    hr = IMoniker_BindToObject(class_moniker, bindctx, NULL, &IID_IParseDisplayName, (void **)&obj);
    ok(hr == S_OK, "Failed to get parsing interface, hr %#lx.\n", hr);
    IUnknown_Release(obj);

    hr = IMoniker_BindToObject(class_moniker, bindctx, NULL, &IID_IClassFactory, (void **)&obj);
    ok(hr == S_OK, "Failed to get parsing interface, hr %#lx.\n", hr);
    IUnknown_Release(obj);

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IParseDisplayName, (void **)&obj);
    ok(hr == S_OK, "Failed to get parsing interface, hr %#lx.\n", hr);
    IUnknown_Release(obj);

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void **)&obj);
    ok(hr == S_OK, "Failed to get parsing interface, hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(obj, &IID_IParseDisplayName, (void **)&obj2);
    ok(hr == S_OK, "Failed to get parsing interface, hr %#lx.\n", hr);
    IUnknown_Release(obj);

    IMoniker_Release(class_moniker);

    /* Reducing. */
    moniker_left = (void *)0xdeadbeef;
    hr = IMoniker_Reduce(moniker, bindctx, MKRREDUCE_ONE, &moniker_left, &moniker2);
    ok(hr == MK_S_REDUCED_TO_SELF, "Unexpected hr %#lx.\n", hr);
    ok(moniker_left == (void *)0xdeadbeef, "Unexpected left moniker.\n");
    ok(moniker2 == moniker, "Unexpected returned moniker.\n");
    IMoniker_Release(moniker2);

    hr = IMoniker_Reduce(moniker, bindctx, MKRREDUCE_ONE, NULL, &moniker2);
    ok(hr == MK_S_REDUCED_TO_SELF, "Unexpected hr %#lx.\n", hr);
    ok(moniker2 == moniker, "Unexpected returned moniker.\n");
    IMoniker_Release(moniker2);

    hr = IMoniker_Reduce(moniker, bindctx, MKRREDUCE_ONE, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Hashing */
    hash = 0;
    hr = IMoniker_Hash(moniker, &hash);
    ok(hr == S_OK, "Failed to get a hash, hr %#lx.\n", hr);
    ok(hash == 0x20d04fe0, "Unexpected hash value %#lx.\n", hash);

    moniker_type = MKSYS_CLASSMONIKER;
    hr = IMoniker_IsSystemMoniker(moniker, &moniker_type);
    ok(hr == S_FALSE || broken(hr == S_OK) /* XP */, "Unexpected hr %#lx.\n", hr);
    ok(moniker_type == MKSYS_NONE, "Unexpected moniker type %ld.\n", moniker_type);

    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == MK_E_UNAVAILABLE, "Unexpected hr %#lx.\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&obj);
    ok(hr == S_OK, "Failed to bind to object, hr %#lx.\n", hr);
    IUnknown_Release(obj);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&obj);
    todo_wine
    ok(hr == MK_E_NOSTORAGE, "Unexpected hr %#lx.\n", hr);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == S_OK, "Failed to create inverse moniker, hr %#lx.\n", hr);
    moniker_type = MKSYS_NONE;
    hr = IMoniker_IsSystemMoniker(inverse, &moniker_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(moniker_type == MKSYS_ANTIMONIKER, "Unexpected moniker type %ld.\n", moniker_type);
    IMoniker_Release(inverse);

    hr = IMoniker_Enum(moniker, FALSE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    obj = (IUnknown *)moniker;
    hr = IMoniker_Enum(moniker, FALSE, (IEnumMoniker **)&obj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(obj == NULL, "Unexpected return value.\n");

    /* Serialization. */
    test_new_moniker_serialize(L"{20d04fe0-3aea-1069-a2d8-08002b30309d}", NULL, moniker);

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMoniker_GetClassID(moniker, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMoniker_Release(moniker);

    /* Full path to create new object, using progid. */
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.cbStruct = sizeof(bind_opts);
    bind_opts.dwClassContext = CLSCTX_INPROC_SERVER;

    hr = CoGetObject(L"new:msxml2.domdocument", (BIND_OPTS *)&bind_opts, &IID_IXMLDOMDocument, (void **)&obj);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);
    IUnknown_Release(obj);

    IBindCtx_Release(bindctx);

    /* Returned object is not bound to context. */
    hr = CreateBindCtx(0, &bindctx);
    ok(hr == S_OK, "Failed to create bind context, hr %#lx.\n", hr);

    eaten = 0;
    hr = MkParseDisplayName(bindctx, L"new:msxml2.domdocument", &eaten, &moniker);
    ok(hr == S_OK, "Failed to parse display name, hr %#lx.\n", hr);
    ok(eaten, "Unexpected eaten length %lu.\n", eaten);

    hr = CLSIDFromProgID(L"msxml2.domdocument", &clsid);
    ok(hr == S_OK, "Failed to get clsid, hr %#lx.\n", hr);

    hr = StringFromCLSID(&clsid, &str);
    ok(hr == S_OK, "Failed to get guid string, hr %#lx.\n", hr);

    test_new_moniker_serialize(str, L"msxml2.domdocument", moniker);
    CoTaskMemFree(str);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&obj);
    ok(hr == S_OK, "Failed to bind to object, hr %#lx.\n", hr);
    EXPECT_REF(obj, 1);

    hr = IBindCtx_GetRunningObjectTable(bindctx, &rot);
    ok(hr == S_OK, "Failed to get rot, hr %#lx.\n", hr);

    hr = IRunningObjectTable_GetObject(rot, moniker, &obj2);
    todo_wine
    ok(hr == MK_E_UNAVAILABLE, "Unexpected hr %#lx.\n", hr);

    IRunningObjectTable_Release(rot);

    IUnknown_Release(obj);

    IBindCtx_Release(bindctx);
}

START_TEST(comsvcs)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_dispenser();
    test_new_moniker();

    CoUninitialize();
}
