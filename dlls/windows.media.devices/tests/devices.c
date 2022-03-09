/*
 * Copyright 2021 Andrew Eikum for CodeWeavers
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "mmdeviceapi.h"
#include "audioclient.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_Devices
#include "windows.media.devices.h"

#include "wine/test.h"

static HRESULT (WINAPI *pActivateAudioInterfaceAsync)(const WCHAR *path,
            REFIID riid, PROPVARIANT *params,
            IActivateAudioInterfaceCompletionHandler *done_handler,
            IActivateAudioInterfaceAsyncOperation **op);

static IMMDeviceEnumerator *g_mmdevenum;
static WCHAR *g_default_capture_id, *g_default_render_id;

static struct {
    LONG ref;
    HANDLE evt;
    CRITICAL_SECTION lock;
    IActivateAudioInterfaceAsyncOperation *op;
    char msg_pfx[128];
    IUnknown *result_iface;
    HRESULT result_hr;
} async_activate_test;

static HRESULT WINAPI async_activate_QueryInterface(
        IActivateAudioInterfaceCompletionHandler *iface,
        REFIID riid,
        void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAgileObject) ||
            IsEqualIID(riid, &IID_IActivateAudioInterfaceCompletionHandler))
    {
        *ppvObject = iface;
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_activate_AddRef(
        IActivateAudioInterfaceCompletionHandler *iface)
{
    return InterlockedIncrement(&async_activate_test.ref);
}

static ULONG WINAPI async_activate_Release(
        IActivateAudioInterfaceCompletionHandler *iface)
{
    ULONG ref = InterlockedDecrement(&async_activate_test.ref);
    if (ref == 1)
        SetEvent(async_activate_test.evt);
    return ref;
}

static HRESULT WINAPI async_activate_ActivateCompleted(
        IActivateAudioInterfaceCompletionHandler *iface,
        IActivateAudioInterfaceAsyncOperation *op)
{
    HRESULT hr;

    EnterCriticalSection(&async_activate_test.lock);
    ok(op == async_activate_test.op,
            "%s: Got different completion operation\n",
            async_activate_test.msg_pfx);
    LeaveCriticalSection(&async_activate_test.lock);

    hr = IActivateAudioInterfaceAsyncOperation_GetActivateResult(op,
            &async_activate_test.result_hr, &async_activate_test.result_iface);
    ok(hr == S_OK,
            "%s: GetActivateResult failed: %08lx\n",
            async_activate_test.msg_pfx, hr);

    return S_OK;
}

static IActivateAudioInterfaceCompletionHandlerVtbl async_activate_vtbl = {
    async_activate_QueryInterface,
    async_activate_AddRef,
    async_activate_Release,
    async_activate_ActivateCompleted,
};

static IActivateAudioInterfaceCompletionHandler async_activate_done = {
    &async_activate_vtbl
};

static void test_MediaDeviceStatics(void)
{
    static const WCHAR *media_device_statics_name = L"Windows.Media.Devices.MediaDevice";

    IMediaDeviceStatics *media_device_statics = NULL;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    HANDLE done_evt = CreateEventW(NULL, FALSE, FALSE, NULL);
    IMMDevice *mmdev;
    HSTRING str;
    HRESULT hr;
    DWORD dr;

    hr = WindowsCreateString(media_device_statics_name, wcslen(media_device_statics_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx\n", hr);
    WindowsDeleteString(str);

    /* interface tests */
    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IMediaDeviceStatics, (void **)&media_device_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IMediaDeviceStatics failed, hr %#lx\n", hr);

    hr = IMediaDeviceStatics_QueryInterface(media_device_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IMediaDeviceStatics_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);
    ok(tmp_inspectable == inspectable, "IMediaDeviceStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IMediaDeviceStatics_QueryInterface(media_device_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IMediaDeviceStatics_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);
    ok(tmp_agile_object == agile_object, "IMediaDeviceStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    InitializeCriticalSection(&async_activate_test.lock);

    /* test default capture device creation */
    hr = IMediaDeviceStatics_GetDefaultAudioCaptureId(media_device_statics, AudioDeviceRole_Default, &str);
    ok(hr == S_OK, "GetDefaultAudioCaptureId failed: %08lx\n", hr);
    ok((!!g_default_capture_id) == (!!str),
            "Presence of default capture device doesn't match expected state\n");

    if (g_default_capture_id)
    {
        ok(wcsstr(WindowsGetStringRawBuffer(str, NULL), g_default_capture_id) != NULL,
                "Expected to find substring of default capture id in %s\n",
                wine_dbgstr_w(WindowsGetStringRawBuffer(str, NULL)));

        /* returned id does not work in GetDevice... */
        hr = IMMDeviceEnumerator_GetDevice(g_mmdevenum, WindowsGetStringRawBuffer(str, NULL), &mmdev);
        ok(hr == E_INVALIDARG, "GetDevice gave wrong error: %08lx\n", hr);

        /* ...but does work in ActivateAudioInterfaceAsync */
        async_activate_test.ref = 1;
        async_activate_test.evt = done_evt;
        async_activate_test.op = NULL;
        async_activate_test.result_iface = NULL;
        async_activate_test.result_hr = 0;
        strcpy(async_activate_test.msg_pfx, "capture_activate");

        EnterCriticalSection(&async_activate_test.lock);
        hr = pActivateAudioInterfaceAsync(WindowsGetStringRawBuffer(str, NULL),
                &IID_IAudioClient2, NULL, &async_activate_done, &async_activate_test.op);
        ok(hr == S_OK, "ActivateAudioInterfaceAsync failed: %08lx\n", hr);
        LeaveCriticalSection(&async_activate_test.lock);

        IActivateAudioInterfaceAsyncOperation_Release(async_activate_test.op);

        dr = WaitForSingleObject(async_activate_test.evt, 1000); /* wait for all refs other than our own to be released */
        ok(dr == WAIT_OBJECT_0, "Timed out waiting for async activate to complete\n");
        ok(async_activate_test.result_hr == S_OK, "Got unexpected activation result: %08lx\n", async_activate_test.result_hr);
        ok(async_activate_test.result_iface != NULL, "Expected to get WASAPI interface, but got NULL\n");
        IUnknown_Release(async_activate_test.result_iface);

        WindowsDeleteString(str);
    }

    /* test default render device creation */
    hr = IMediaDeviceStatics_GetDefaultAudioRenderId(media_device_statics, AudioDeviceRole_Default, &str);
    ok(hr == S_OK, "GetDefaultAudioRenderId failed: %08lx\n", hr);
    ok((!!g_default_render_id) == (!!str),
            "Presence of default render device doesn't match expected state\n");

    if (g_default_render_id)
    {
        ok(wcsstr(WindowsGetStringRawBuffer(str, NULL), g_default_render_id) != NULL,
                "Expected to find substring of default render id in %s\n",
                wine_dbgstr_w(WindowsGetStringRawBuffer(str, NULL)));

        /* returned id does not work in GetDevice... */
        hr = IMMDeviceEnumerator_GetDevice(g_mmdevenum, WindowsGetStringRawBuffer(str, NULL), &mmdev);
        ok(hr == E_INVALIDARG, "GetDevice gave wrong error: %08lx\n", hr);

        /* ...but does work in ActivateAudioInterfaceAsync */
        async_activate_test.ref = 1;
        async_activate_test.evt = done_evt;
        async_activate_test.op = NULL;
        async_activate_test.result_iface = NULL;
        async_activate_test.result_hr = 0;
        strcpy(async_activate_test.msg_pfx, "render_activate");

        EnterCriticalSection(&async_activate_test.lock);
        hr = pActivateAudioInterfaceAsync(WindowsGetStringRawBuffer(str, NULL),
                &IID_IAudioClient2, NULL, &async_activate_done, &async_activate_test.op);
        ok(hr == S_OK, "ActivateAudioInterfaceAsync failed: %08lx\n", hr);
        LeaveCriticalSection(&async_activate_test.lock);

        IActivateAudioInterfaceAsyncOperation_Release(async_activate_test.op);

        dr = WaitForSingleObject(async_activate_test.evt, 1000); /* wait for all refs other than our own to be released */
        ok(dr == WAIT_OBJECT_0, "Timed out waiting for async activate to complete\n");
        ok(async_activate_test.result_hr == S_OK, "Got unexpected activation result: %08lx\n", async_activate_test.result_hr);
        ok(async_activate_test.result_iface != NULL, "Expected to get WASAPI interface, but got NULL\n");
        IUnknown_Release(async_activate_test.result_iface);

        WindowsDeleteString(str);
    }

    /* cleanup */
    CloseHandle(done_evt);
    DeleteCriticalSection(&async_activate_test.lock);
    IMediaDeviceStatics_Release(media_device_statics);
}

START_TEST(devices)
{
    HMODULE mmdevapi;
    IMMDevice *mmdev;
    HRESULT hr;

    if (!(mmdevapi = LoadLibraryW(L"mmdevapi.dll")))
    {
        win_skip("Failed to load mmdevapi.dll, skipping tests\n");
        return;
    }

#define LOAD_FUNCPTR(x) \
    if (!(p##x = (void*)GetProcAddress(mmdevapi, #x))) \
    { \
        win_skip("Failed to find %s in mmdevapi.dll, skipping tests.\n", #x); \
        return; \
    }

    LOAD_FUNCPTR(ActivateAudioInterfaceAsync);
#undef LOAD_FUNCPTR

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&g_mmdevenum);
    ok(hr == S_OK, "Couldn't make MMDeviceEnumerator: %08lx\n", hr);

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(g_mmdevenum, eCapture, eMultimedia, &mmdev);
    if (hr == S_OK)
    {
        hr = IMMDevice_GetId(mmdev, &g_default_capture_id);
        ok(hr == S_OK, "IMMDevice::GetId(capture) failed: %08lx\n", hr);

        IMMDevice_Release(mmdev);
    }
    if (hr != S_OK)
        g_default_capture_id = NULL;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(g_mmdevenum, eRender, eMultimedia, &mmdev);
    if (hr == S_OK)
    {
        hr = IMMDevice_GetId(mmdev, &g_default_render_id);
        ok(hr == S_OK, "IMMDevice::GetId(render) failed: %08lx\n", hr);

        IMMDevice_Release(mmdev);
    }
    if (hr != S_OK)
        g_default_render_id = NULL;

    test_MediaDeviceStatics();

    CoTaskMemFree(g_default_capture_id);
    CoTaskMemFree(g_default_render_id);
    IMMDeviceEnumerator_Release(g_mmdevenum);

    RoUninitialize();
}
