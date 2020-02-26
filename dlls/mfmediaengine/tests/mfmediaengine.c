/*
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mfmediaengine.h"
#include "mferror.h"
#include "dxgi.h"
#include "initguid.h"

#include "wine/heap.h"
#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %d, expected %d.\n", rc, ref);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    X(MFCreateDXGIDeviceManager);
#undef X
}

struct media_engine_notify
{
    IMFMediaEngineNotify IMFMediaEngineNotify_iface;
    LONG refcount;
};

static inline struct media_engine_notify *impl_from_IMFMediaEngineNotify(IMFMediaEngineNotify *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine_notify, IMFMediaEngineNotify_iface);
}

static HRESULT WINAPI media_engine_notify_QueryInterface(IMFMediaEngineNotify *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaEngineNotify) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngineNotify_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_notify_AddRef(IMFMediaEngineNotify *iface)
{
    struct media_engine_notify *notify = impl_from_IMFMediaEngineNotify(iface);
    return InterlockedIncrement(&notify->refcount);
}

static ULONG WINAPI media_engine_notify_Release(IMFMediaEngineNotify *iface)
{
    struct media_engine_notify *notify = impl_from_IMFMediaEngineNotify(iface);
    return InterlockedDecrement(&notify->refcount);
}

static HRESULT WINAPI media_engine_notify_EventNotify(IMFMediaEngineNotify *iface, DWORD event, DWORD_PTR param1, DWORD param2)
{
    return S_OK;
}

static IMFMediaEngineNotifyVtbl media_engine_notify_vtbl =
{
    media_engine_notify_QueryInterface,
    media_engine_notify_AddRef,
    media_engine_notify_Release,
    media_engine_notify_EventNotify,
};

static void test_factory(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *notify = &notify_impl.IMFMediaEngineNotify_iface;
    IMFMediaEngineClassFactory *factory, *factory2;
    IMFDXGIDeviceManager *manager;
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    UINT token;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IMFMediaEngineClassFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* pre-win8 */,
       "Failed to create class factory, hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER,
                          &IID_IMFMediaEngineClassFactory, (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION || broken(hr == REGDB_E_CLASSNOTREG) /* pre-win8 */,
       "Unexpected hr %#x.\n", hr);

    if (!factory)
    {
        win_skip("Not IMFMediaEngineClassFactory support.\n");
        CoUninitialize();
        return;
    }

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "MFCreateDXGIDeviceManager failed: %#x.\n", hr);
    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "MFCreateAttributes failed: %#x.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
                                                   attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "IMFMediaEngineClassFactory_CreateInstance got %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_OPM_HWND, NULL);
    ok(hr == S_OK, "IMFAttributes_SetUnknown failed: %#x.\n", hr);
    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
                                                   attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "IMFMediaEngineClassFactory_CreateInstance got %#x.\n", hr);

    IMFAttributes_DeleteAllItems(attributes);
    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)notify);
    ok(hr == S_OK, "IMFAttributes_SetUnknown failed: %#x.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_UNKNOWN);
    ok(hr == S_OK, "IMFAttributes_SetUINT32 failed: %#x.\n", hr);
    EXPECT_REF(factory, 1);
    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
                                                   attributes, &media_engine);
    ok(hr == S_OK, "IMFMediaEngineClassFactory_CreateInstance failed: %#x.\n", hr);
    EXPECT_REF(factory, 1);

    IMFMediaEngine_Release(media_engine);
    IMFAttributes_Release(attributes);
    IMFDXGIDeviceManager_Release(manager);
    IMFMediaEngineClassFactory_Release(factory);

    CoUninitialize();
}

START_TEST(mfmediaengine)
{
    HRESULT hr;

    init_functions();

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "MFStartup failed: %#x.\n", hr);

    test_factory();

    MFShutdown();
}
