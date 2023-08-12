/*
 * Copyright 2009 Maarten Lankhorst
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

#include "wine/test.h"

#define COBJMACROS

#include "initguid.h"
#include "endpointvolume.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "spatialaudioclient.h"
#include "audiopolicy.h"
#include "dshow.h"
#include "dsound.h"
#include "devpkey.h"
#include "devicetopology.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static UINT g_num_mmdevs;
static WCHAR g_device_path[MAX_PATH];

/* Some of the QueryInterface tests are really just to check if I got the IIDs right :) */

/* IMMDeviceCollection appears to have no QueryInterface method and instead forwards to mme */
static void test_collection(IMMDeviceEnumerator *mme, IMMDeviceCollection *col)
{
    IMMDeviceCollection *col2;
    IMMDeviceEnumerator *mme2;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;
    UINT numdev;
    IMMDevice *dev;

    /* collection doesn't keep a ref on parent */
    IMMDeviceEnumerator_AddRef(mme);
    ref = IMMDeviceEnumerator_Release(mme);
    ok(ref == 2, "Reference count on parent is %lu\n", ref);

    ref = IMMDeviceCollection_AddRef(col);
    IMMDeviceCollection_Release(col);
    ok(ref == 2, "Invalid reference count %lu on collection\n", ref);

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "Null ppv returns %08lx\n", hr);

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Cannot query for IID_IUnknown: 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        ok((IUnknown*)col == unk, "Pointers are not identical %p/%p/%p\n", col, unk, mme);
        IUnknown_Release(unk);
    }

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IMMDeviceCollection, (void**)&col2);
    ok(hr == S_OK, "Cannot query for IID_IMMDeviceCollection: 0x%08lx\n", hr);
    if (hr == S_OK)
        IMMDeviceCollection_Release(col2);

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IMMDeviceEnumerator, (void**)&mme2);
    ok(hr == E_NOINTERFACE, "Query for IID_IMMDeviceEnumerator returned: 0x%08lx\n", hr);
    if (hr == S_OK)
        IMMDeviceEnumerator_Release(mme2);

    hr = IMMDeviceCollection_GetCount(col, NULL);
    ok(hr == E_POINTER, "GetCount returned 0x%08lx\n", hr);

    hr = IMMDeviceCollection_GetCount(col, &numdev);
    ok(hr == S_OK, "GetCount returned 0x%08lx\n", hr);

    dev = (void*)(LONG_PTR)0x12345678;
    hr = IMMDeviceCollection_Item(col, numdev, &dev);
    ok(hr == E_INVALIDARG, "Asking for too high device returned 0x%08lx\n", hr);
    ok(dev == NULL, "Returned non-null device\n");

    g_num_mmdevs = numdev;

    if (numdev)
    {
        hr = IMMDeviceCollection_Item(col, 0, NULL);
        ok(hr == E_POINTER, "Query with null pointer returned 0x%08lx\n", hr);

        hr = IMMDeviceCollection_Item(col, 0, &dev);
        ok(hr == S_OK, "Valid Item returned 0x%08lx\n", hr);
        ok(dev != NULL, "Device is null!\n");
        if (dev != NULL)
        {
            char temp[128];
            WCHAR *id = NULL;
            if (IMMDevice_GetId(dev, &id) == S_OK)
            {
                IMMDevice *dev2;

                lstrcpyW(g_device_path, id);
                temp[sizeof(temp)-1] = 0;
                WideCharToMultiByte(CP_ACP, 0, id, -1, temp, sizeof(temp)-1, NULL, NULL);
                trace("Device found: %s\n", temp);

                hr = IMMDeviceEnumerator_GetDevice(mme, id, &dev2);
                ok(hr == S_OK, "GetDevice failed: %08lx\n", hr);

                IMMDevice_Release(dev2);

                CoTaskMemFree(id);
            }
        }
        if (dev)
            IMMDevice_Release(dev);
    }
    IMMDeviceCollection_Release(col);
}

static struct {
    LONG ref;
    HANDLE evt;
    CRITICAL_SECTION lock;
    IActivateAudioInterfaceAsyncOperation *op;
    DWORD main_tid;
    char msg_pfx[128];
    IUnknown *result_iface;
    HRESULT result_hr;
} async_activate_test;

static HRESULT WINAPI async_activate_QueryInterface(
        IActivateAudioInterfaceCompletionHandler *iface,
        REFIID riid,
        void **ppvObject)
{
    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAgileObject) ||
            IsEqualIID(riid, &IID_IActivateAudioInterfaceCompletionHandler)){
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
    if(ref == 1)
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

    ok(GetCurrentThreadId() != async_activate_test.main_tid,
            "%s: Expected callback on worker thread\n",
            async_activate_test.msg_pfx);

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

static void test_ActivateAudioInterfaceAsync(void)
{
    HRESULT (* WINAPI pActivateAudioInterfaceAsync)(const WCHAR *path,
            REFIID riid, PROPVARIANT *params,
            IActivateAudioInterfaceCompletionHandler *done_handler,
            IActivateAudioInterfaceAsyncOperation **op);
    HANDLE h_mmdev;
    HRESULT hr;
    LPOLESTR path;
    DWORD dr;
    IAudioClient3 *ac3;

    h_mmdev = LoadLibraryA("mmdevapi.dll");

    pActivateAudioInterfaceAsync = (void*)GetProcAddress(h_mmdev, "ActivateAudioInterfaceAsync");
    if (!pActivateAudioInterfaceAsync)
    {
        win_skip("ActivateAudioInterfaceAsync is not supported on Win <= 7\n");
        return;
    }

    /* some applications look this up by ordinal */
    pActivateAudioInterfaceAsync = (void*)GetProcAddress(h_mmdev, (char *)17);
    ok(pActivateAudioInterfaceAsync != NULL, "mmdevapi.ActivateAudioInterfaceAsync missing!\n");

    async_activate_test.ref = 1;
    async_activate_test.evt = CreateEventW(NULL, FALSE, FALSE, NULL);
    InitializeCriticalSection(&async_activate_test.lock);
    async_activate_test.op = NULL;
    async_activate_test.main_tid = GetCurrentThreadId();
    async_activate_test.result_iface = NULL;
    async_activate_test.result_hr = 0;


    /* try invalid device path */
    strcpy(async_activate_test.msg_pfx, "invalid_path");

    EnterCriticalSection(&async_activate_test.lock);
    hr = pActivateAudioInterfaceAsync(L"winetest_bogus", &IID_IAudioClient3, NULL, &async_activate_done, &async_activate_test.op);
    ok(hr == S_OK, "ActivateAudioInterfaceAsync failed: %08lx\n", hr);
    LeaveCriticalSection(&async_activate_test.lock);

    IActivateAudioInterfaceAsyncOperation_Release(async_activate_test.op);

    dr = WaitForSingleObject(async_activate_test.evt, 1000); /* wait for all refs other than our own to be released */
    ok(dr == WAIT_OBJECT_0, "Timed out waiting for async activate to complete\n");
    ok(async_activate_test.ref == 1, "ActivateAudioInterfaceAsync leaked a handler ref: %lu\n", async_activate_test.ref);
    ok(async_activate_test.result_hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
            "mmdevice activation gave wrong result: %08lx\n", async_activate_test.result_hr);
    ok(async_activate_test.result_iface == NULL, "Got non-NULL iface pointer: %p\n", async_activate_test.result_iface);


    /* device id from IMMDevice does not work */
    if(g_num_mmdevs > 0){
        strcpy(async_activate_test.msg_pfx, "mmdevice_id");

        EnterCriticalSection(&async_activate_test.lock);
        hr = pActivateAudioInterfaceAsync(g_device_path, &IID_IAudioClient3, NULL, &async_activate_done, &async_activate_test.op);
        ok(hr == S_OK, "ActivateAudioInterfaceAsync failed: %08lx\n", hr);
        LeaveCriticalSection(&async_activate_test.lock);

        IActivateAudioInterfaceAsyncOperation_Release(async_activate_test.op);

        dr = WaitForSingleObject(async_activate_test.evt, 1000);
        ok(dr == WAIT_OBJECT_0, "Timed out waiting for async activate to complete\n");
        ok(async_activate_test.ref == 1, "ActivateAudioInterfaceAsync leaked a handler ref: %lu\n", async_activate_test.ref);
        ok(async_activate_test.result_hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
                "mmdevice activation gave wrong result: %08lx\n", async_activate_test.result_hr);
        ok(async_activate_test.result_iface == NULL, "Got non-NULL iface pointer: %p\n", async_activate_test.result_iface);
    }


    /* try DEVINTERFACE_AUDIO_RENDER */
    strcpy(async_activate_test.msg_pfx, "audio_render");
    StringFromIID(&DEVINTERFACE_AUDIO_RENDER, &path);

    EnterCriticalSection(&async_activate_test.lock);
    hr = pActivateAudioInterfaceAsync(path, &IID_IAudioClient3, NULL, &async_activate_done, &async_activate_test.op);
    ok(hr == S_OK, "ActivateAudioInterfaceAsync failed: %08lx\n", hr);
    LeaveCriticalSection(&async_activate_test.lock);

    IActivateAudioInterfaceAsyncOperation_Release(async_activate_test.op);

    dr = WaitForSingleObject(async_activate_test.evt, 1000);
    ok(dr == WAIT_OBJECT_0, "Timed out waiting for async activate to complete\n");
    ok(async_activate_test.ref == 1, "ActivateAudioInterfaceAsync leaked a handler ref\n");
    ok(async_activate_test.result_hr == S_OK ||
            (g_num_mmdevs == 0 && async_activate_test.result_hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) || /* no devices */
            broken(async_activate_test.result_hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), /* win8 doesn't support DEVINTERFACE_AUDIO_RENDER */
            "mmdevice activation gave wrong result: %08lx\n", async_activate_test.result_hr);

    if(async_activate_test.result_hr == S_OK){
        ok(async_activate_test.result_iface != NULL, "Got NULL iface pointer on success?\n");

        /* returned iface should be the IID we requested */
        hr = IUnknown_QueryInterface(async_activate_test.result_iface, &IID_IAudioClient3, (void**)&ac3);
        ok(hr == S_OK, "Failed to query IAudioClient3: %08lx\n", hr);
        ok(async_activate_test.result_iface == (IUnknown*)ac3,
                "Activated interface other than IAudioClient3!\n");
        IAudioClient3_Release(ac3);

        IUnknown_Release(async_activate_test.result_iface);
    }

    CoTaskMemFree(path);

    CloseHandle(async_activate_test.evt);
    DeleteCriticalSection(&async_activate_test.lock);
}

static HRESULT WINAPI notif_QueryInterface(IMMNotificationClient *iface,
        const GUID *riid, void **obj)
{
    ok(0, "Unexpected QueryInterface call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI notif_AddRef(IMMNotificationClient *iface)
{
    ok(0, "Unexpected AddRef call\n");
    return 2;
}

static ULONG WINAPI notif_Release(IMMNotificationClient *iface)
{
    ok(0, "Unexpected Release call\n");
    return 1;
}

static HRESULT WINAPI notif_OnDeviceStateChanged(IMMNotificationClient *iface,
        const WCHAR *device_id, DWORD new_state)
{
    ok(0, "Unexpected OnDeviceStateChanged call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnDeviceAdded(IMMNotificationClient *iface,
        const WCHAR *device_id)
{
    ok(0, "Unexpected OnDeviceAdded call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnDeviceRemoved(IMMNotificationClient *iface,
        const WCHAR *device_id)
{
    ok(0, "Unexpected OnDeviceRemoved call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnDefaultDeviceChanged(IMMNotificationClient *iface,
        EDataFlow flow, ERole role, const WCHAR *device_id)
{
    ok(0, "Unexpected OnDefaultDeviceChanged call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnPropertyValueChanged(IMMNotificationClient *iface,
        const WCHAR *device_id, const PROPERTYKEY key)
{
    ok(0, "Unexpected OnPropertyValueChanged call\n");
    return E_NOTIMPL;
}

static IMMNotificationClientVtbl notif_vtbl = {
    notif_QueryInterface,
    notif_AddRef,
    notif_Release,
    notif_OnDeviceStateChanged,
    notif_OnDeviceAdded,
    notif_OnDeviceRemoved,
    notif_OnDefaultDeviceChanged,
    notif_OnPropertyValueChanged
};

static IMMNotificationClient notif = { &notif_vtbl };

static void test_connectors(IDeviceTopology *dt)
{
    HRESULT hr;
    UINT connector_count;

    hr = IDeviceTopology_GetConnectorCount(dt, &connector_count);
    ok(hr == S_OK, "GetConnectorCount returns 0x%08lx\n", hr);
    trace("connector count: %u\n", connector_count);

    if (hr == S_OK && connector_count > 0)
    {
        IConnector *connector;

        hr = IDeviceTopology_GetConnector(dt, 0, &connector);
        ok(hr == S_OK, "GetConnector returns 0x%08lx\n", hr);

        if (hr == S_OK)
        {
            ConnectorType type;

            hr = IConnector_GetType(connector, &type);
            ok(hr == S_OK, "GetConnector returns 0x%08lx\n", hr);
            trace("connector 0 type: %u\n", connector_count);
        }
    }
}

static void test_DeviceTopology(IMMDeviceEnumerator *mme)
{
    IMMDevice *dev = NULL;
    IDeviceTopology *dt = NULL;
    HRESULT hr;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08lx\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOTFOUND)
            win_skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08lx\n", hr);
        goto cleanup;
    }

    hr = IMMDevice_Activate(dev, &IID_IDeviceTopology, CLSCTX_INPROC_SERVER, NULL, (void**)&dt);
    ok(hr == S_OK || hr == E_NOINTERFACE, "IDeviceTopology Activation failed: 0x%08lx\n", hr);
    if (hr != S_OK || !dev)
    {
        if (hr == E_NOINTERFACE)
            win_skip("IDeviceTopology interface not found\n");
        else
            skip("IDeviceTopology Activation returns 0x%08lx\n", hr);
        goto cleanup;
    }

    test_connectors(dt);

    IDeviceTopology_Release(dt);

cleanup:
    if (dev)
        IMMDevice_Release(dev);
}

/* Only do parameter tests here, the actual MMDevice testing should be a separate test */
START_TEST(mmdevenum)
{
    HRESULT hr;
    IUnknown *unk = NULL;
    IMMDeviceEnumerator *mme, *mme2;
    ULONG ref;
    IMMDeviceCollection *col;
    IMMDevice *dev;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08lx\n", hr);
        return;
    }

    /* Odd behavior.. bug? */
    ref = IMMDeviceEnumerator_AddRef(mme);
    ok(ref == 3, "Invalid reference count after incrementing: %lu\n", ref);
    IMMDeviceEnumerator_Release(mme);

    hr = IMMDeviceEnumerator_QueryInterface(mme, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "returned 0x%08lx\n", hr);
    if (hr != S_OK) return;

    ok( (LONG_PTR)mme == (LONG_PTR)unk, "Pointers are unequal %p/%p\n", unk, mme);
    IUnknown_Release(unk);

    /* Proving that it is static.. */
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme2);
    ok(hr == S_OK, "CoCreateInstance failed: 0x%08lx\n", hr);
    IMMDeviceEnumerator_Release(mme2);
    ok(mme == mme2, "Pointers are not equal!\n");

    hr = IMMDeviceEnumerator_QueryInterface(mme, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "Null pointer on QueryInterface returned %08lx\n", hr);

    hr = IMMDeviceEnumerator_QueryInterface(mme, &GUID_NULL, (void**)&unk);
    ok(!unk, "Unk not reset to null after invalid QI\n");
    ok(hr == E_NOINTERFACE, "Invalid hr %08lx returned on IID_NULL\n", hr);

    hr = IMMDeviceEnumerator_GetDevice(mme, L"notadevice", NULL);
    ok(hr == E_POINTER, "GetDevice gave wrong error: %08lx\n", hr);

    hr = IMMDeviceEnumerator_GetDevice(mme, NULL, &dev);
    ok(hr == E_POINTER, "GetDevice gave wrong error: %08lx\n", hr);

    hr = IMMDeviceEnumerator_GetDevice(mme, L"notadevice", &dev);
    ok(hr == E_INVALIDARG, "GetDevice gave wrong error: %08lx\n", hr);

    col = (void*)(LONG_PTR)0x12345678;
    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, 0xffff, DEVICE_STATEMASK_ALL, &col);
    ok(hr == E_INVALIDARG, "Setting invalid data flow returned 0x%08lx\n", hr);
    ok(col == NULL, "Collection pointer non-null on failure\n");

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL+1, &col);
    ok(hr == E_INVALIDARG, "Setting invalid mask returned 0x%08lx\n", hr);

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL, NULL);
    ok(hr == E_POINTER, "Invalid pointer returned: 0x%08lx\n", hr);

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL, &col);
    ok(hr == S_OK, "Valid EnumAudioEndpoints returned 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        ok(!!col, "Returned null pointer\n");
        if (col)
            test_collection(mme, col);
    }

    hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(mme, NULL);
    ok(hr == E_POINTER, "RegisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "RegisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "RegisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, NULL);
    ok(hr == E_POINTER, "UnregisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, (IMMNotificationClient*)0xdeadbeef);
    ok(hr == E_NOTFOUND, "UnregisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "UnregisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "UnregisterEndpointNotificationCallback failed: %08lx\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, &notif);
    ok(hr == E_NOTFOUND, "UnregisterEndpointNotificationCallback failed: %08lx\n", hr);

    test_DeviceTopology(mme);

    IMMDeviceEnumerator_Release(mme);

    test_ActivateAudioInterfaceAsync();
}
