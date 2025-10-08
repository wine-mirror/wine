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
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"

#include "mfapi.h"
#include "mfidl.h"
#include "mfmediaengine.h"
#include "mferror.h"
#include "dxgi.h"
#include "d3d11.h"
#include "initguid.h"
#include "mmdeviceapi.h"
#include "audiosessiontypes.h"
#include "wincodec.h"

#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);
static HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device_out,
        D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context);

static IMFMediaEngineClassFactory *factory;

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %ld, expected %ld.\n", rc, ref);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static BOOL compare_double(double a, double b, double allowed_error)
{
    return fabs(a - b) <= allowed_error;
}

static DWORD compare_rgb(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect, UINT bits)
{
    DWORD x, y, step = bits / 8, data_size, diff = 0, width = size->cx, height = size->cy;

    /* skip BMP header from the dump */
    data_size = *(DWORD *)(expect + 2 + 2 * sizeof(DWORD));
    *length = *length + data_size;
    expect = expect + data_size;

    for (y = 0; y < height; y++, data += width * step, expect += width * step)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[step * x + 0] - (int)data[step * x + 0]);
            diff += abs((int)expect[step * x + 1] - (int)data[step * x + 1]);
            if (step >= 3) diff += abs((int)expect[step * x + 2] - (int)data[step * x + 2]);
        }
    }

    data_size = (rect->right - rect->left) * (rect->bottom - rect->top) * min(step, 3);
    return diff * 100 / 256 / data_size;
}

static DWORD compare_rgb32(const BYTE *data, DWORD *length, const SIZE *size, const RECT *rect, const BYTE *expect)
{
    return compare_rgb(data, length, size, rect, expect, 32);
}

static void dump_rgb(const BYTE *data, DWORD length, const SIZE *size, HANDLE output, UINT bits)
{
    DWORD width = size->cx, height = size->cy;
    static const char magic[2] = "BM";
    struct
    {
        DWORD length;
        DWORD reserved;
        DWORD offset;
        BITMAPINFOHEADER biHeader;
    } header =
    {
        .length = length + sizeof(header) + 2, .offset = sizeof(header) + 2,
        .biHeader =
        {
            .biSize = sizeof(BITMAPINFOHEADER), .biWidth = width, .biHeight = height, .biPlanes = 1,
            .biBitCount = bits, .biCompression = BI_RGB, .biSizeImage = width * height * (bits / 8),
        },
    };
    DWORD written;
    BOOL ret;

    ret = WriteFile(output, magic, sizeof(magic), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(magic), "written %lu bytes\n", written);
    ret = WriteFile(output, &header, sizeof(header), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(header), "written %lu bytes\n", written);
    ret = WriteFile(output, data, length, &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == length, "written %lu bytes\n", written);
}

static void dump_rgb32(const BYTE *data, DWORD length, const SIZE *size, HANDLE output)
{
    return dump_rgb(data, length, size, output, 32);
}

#define check_rgb32_data(a, b, c, d) check_rgb32_data_(__LINE__, a, b, c, d)
static DWORD check_rgb32_data_(int line, const WCHAR *filename, const BYTE *data, DWORD length, const RECT *rect)
{
    SIZE size = {rect->right, rect->bottom};
    WCHAR output_path[MAX_PATH];
    const BYTE *expect_data;
    HRSRC resource;
    HANDLE output;

    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, filename);
    output = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());
    dump_rgb32(data, length, &size, output);
    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output);

    resource = FindResourceW(NULL, filename, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    expect_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));

    return compare_rgb32(data, &length, &size, rect, expect_data);
}

static void init_functions(void)
{
    HMODULE mod;

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    if ((mod = GetModuleHandleA("mfplat.dll")))
    {
        X(MFCreateDXGIDeviceManager);
    }

    if ((mod = LoadLibraryA("d3d11.dll")))
    {
        X(D3D11CreateDevice);
    }
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
    ULONG refcount = InterlockedDecrement(&notify->refcount);

    if (!refcount)
        free(notify);

    return refcount;
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

static struct media_engine_notify *create_callback(void)
{
    struct media_engine_notify *object;

    object = calloc(1, sizeof(*object));

    object->IMFMediaEngineNotify_iface.lpVtbl = &media_engine_notify_vtbl;
    object->refcount = 1;

    return object;
}

static ID3D11Device *create_d3d11_device(void)
{
    static const D3D_FEATURE_LEVEL default_feature_level[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    const D3D_FEATURE_LEVEL *feature_level;
    unsigned int feature_level_count;
    ID3D11Device *device;

    feature_level = default_feature_level;
    feature_level_count = ARRAY_SIZE(default_feature_level);

    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            feature_level, feature_level_count, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0,
            feature_level, feature_level_count, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
            feature_level, feature_level_count, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static HRESULT create_media_engine(IMFMediaEngineNotify *callback, IMFDXGIDeviceManager *manager, UINT32 output_format,
        REFIID riid, void **obj)
{
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    HRESULT hr;

    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (manager)
    {
        hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown *)manager);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)callback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, output_format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, 0, attributes, &media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_Release(attributes);

    hr = IMFMediaEngine_QueryInterface(media_engine, riid, obj);
    IMFMediaEngine_Release(media_engine);

    return hr;
}

static void test_factory(void)
{
    IMFMediaEngineClassFactory *factory, *factory2;
    struct media_engine_notify *notify;
    IMFDXGIDeviceManager *manager;
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    UINT token;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFMediaEngineClassFactory,
            (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* pre-win8 */, "Failed to create class factory, hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        win_skip("Media Engine is not supported.\n");
        return;
    }

    notify = create_callback();

    /* Aggregation is not supported. */
    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER,
            &IID_IMFMediaEngineClassFactory, (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE, attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_OPM_HWND, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE, attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_DeleteAllItems(attributes);
    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)&notify->IMFMediaEngineNotify_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_UNKNOWN);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(factory, 1);
    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE, attributes, &media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(factory, 1);

    IMFMediaEngine_Release(media_engine);
    IMFAttributes_Release(attributes);
    IMFDXGIDeviceManager_Release(manager);
    IMFMediaEngineClassFactory_Release(factory);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_CreateInstance(void)
{
    struct media_engine_notify *notify;
    IMFMediaEngineEx *media_engine_ex;
    IMFDXGIDeviceManager *manager;
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    IUnknown *unk;
    UINT token;
    HRESULT hr;
    BOOL ret;

    notify = create_callback();

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
            attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_OPM_HWND, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
            attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_DeleteAllItems(attributes);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)&notify->IMFMediaEngineNotify_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_UNKNOWN);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_REAL_TIME_MODE
            | MF_MEDIA_ENGINE_WAITFORSTABLE_STATE, attributes, &media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(media_engine, &IID_IMFMediaEngine, TRUE);

    hr = IMFMediaEngine_QueryInterface(media_engine, &IID_IMFGetService, (void **)&unk);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* supported since win10 */, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);

    if (SUCCEEDED(IMFMediaEngine_QueryInterface(media_engine, &IID_IMFMediaEngineEx, (void **)&media_engine_ex)))
    {
        hr = IMFMediaEngineEx_GetRealTimeMode(media_engine_ex, &ret);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ret, "Unexpected value.\n");

        hr = IMFMediaEngineEx_SetRealTimeMode(media_engine_ex, FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetRealTimeMode(media_engine_ex, &ret);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!ret, "Unexpected value.\n");

        hr = IMFMediaEngineEx_SetRealTimeMode(media_engine_ex, TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetRealTimeMode(media_engine_ex, &ret);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(ret, "Unexpected value.\n");

        IMFMediaEngineEx_Release(media_engine_ex);
    }

    IMFMediaEngine_Release(media_engine);
    IMFAttributes_Release(attributes);
    IMFDXGIDeviceManager_Release(manager);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_Shutdown(void)
{
    MF_MEDIA_ENGINE_CANPLAY can_play_state;
    struct media_engine_notify *notify;
    IMFMediaEngineEx *media_engine_ex;
    IMFMediaTimeRange *time_range;
    IMFMediaEngine *media_engine;
    PROPVARIANT propvar;
    DWORD flags, cx, cy;
    unsigned int state;
    UINT32 value;
    double val;
    HRESULT hr;
    BSTR str;
    BOOL ret;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngine, (void **)&media_engine);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == MF_E_SHUTDOWN || broken(hr == S_OK) /* before win10 */, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_SetSource(media_engine, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_GetCurrentSource(media_engine, &str);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_GetNetworkState(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    /* Preload mode is still accessible. */
    state = IMFMediaEngine_GetPreload(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    hr = IMFMediaEngine_SetPreload(media_engine, MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_GetPreload(media_engine);
    ok(state == MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC, "Unexpected state %d.\n", state);

    hr = IMFMediaEngine_SetPreload(media_engine, 100);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_GetPreload(media_engine);
    ok(state == 100, "Unexpected state %d.\n", state);

    hr = IMFMediaEngine_GetBuffered(media_engine, &time_range);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_Load(media_engine);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    str = SysAllocString(L"video/mp4");
    hr = IMFMediaEngine_CanPlayType(media_engine, str, &can_play_state);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    state = IMFMediaEngine_GetReadyState(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    state = IMFMediaEngine_IsSeeking(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    val = IMFMediaEngine_GetCurrentTime(media_engine);
    ok(val == 0.0, "Unexpected time %f.\n", val);

    hr = IMFMediaEngine_SetCurrentTime(media_engine, 1.0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    val = IMFMediaEngine_GetStartTime(media_engine);
    ok(val == 0.0, "Unexpected time %f.\n", val);

    state = IMFMediaEngine_IsPaused(media_engine);
    ok(!!state, "Unexpected state %d.\n", state);

    val = IMFMediaEngine_GetDefaultPlaybackRate(media_engine);
    ok(val == 1.0, "Unexpected rate %f.\n", val);

    hr = IMFMediaEngine_SetDefaultPlaybackRate(media_engine, 2.0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    val = IMFMediaEngine_GetPlaybackRate(media_engine);
    ok(val == 1.0, "Unexpected rate %f.\n", val);

    hr = IMFMediaEngine_GetPlayed(media_engine, &time_range);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_GetSeekable(media_engine, &time_range);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_IsEnded(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    /* Autoplay mode is still accessible. */
    state = IMFMediaEngine_GetAutoPlay(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_SetAutoPlay(media_engine, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_GetAutoPlay(media_engine);
    ok(!!state, "Unexpected state.\n");

    /* Loop mode is still accessible. */
    state = IMFMediaEngine_GetLoop(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_SetLoop(media_engine, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_GetLoop(media_engine);
    ok(!!state, "Unexpected state.\n");

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_Pause(media_engine);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_GetMuted(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_SetMuted(media_engine, TRUE);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    val = IMFMediaEngine_GetVolume(media_engine);
    ok(val == 1.0, "Unexpected value %f.\n", val);

    hr = IMFMediaEngine_SetVolume(media_engine, 2.0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    state = IMFMediaEngine_HasVideo(media_engine);
    ok(!state, "Unexpected state.\n");

    state = IMFMediaEngine_HasAudio(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_GetNativeVideoSize(media_engine, &cx, &cy);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_GetVideoAspectRatio(media_engine, &cx, &cy);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    if (SUCCEEDED(IMFMediaEngine_QueryInterface(media_engine, &IID_IMFMediaEngineEx, (void **)&media_engine_ex)))
    {
        hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine_ex, NULL, NULL);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetAudioStreamCategory(media_engine_ex, &value);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetAudioEndpointRole(media_engine_ex, &value);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_SetAudioStreamCategory(media_engine_ex, AudioCategory_ForegroundOnlyMedia);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_SetAudioEndpointRole(media_engine_ex, eConsole);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetResourceCharacteristics(media_engine_ex, NULL);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetResourceCharacteristics(media_engine_ex, &flags);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetRealTimeMode(media_engine_ex, NULL);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetRealTimeMode(media_engine_ex, &ret);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_SetRealTimeMode(media_engine_ex, TRUE);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetPresentationAttribute(media_engine_ex, &MF_PD_DURATION, &propvar);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaEngineEx_GetStreamAttribute(media_engine_ex, 0, &MF_SD_PROTECTED, &propvar);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        IMFMediaEngineEx_Release(media_engine_ex);
    }

    IMFMediaEngine_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_Play(void)
{
    struct media_engine_notify *notify;
    IMFMediaTimeRange *range, *range1;
    IMFMediaEngine *media_engine;
    LONGLONG pts;
    DWORD count;
    HRESULT hr;
    BOOL ret;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngine, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_GetBuffered(media_engine, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaEngine_GetBuffered(media_engine, &range1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range != range1, "Unexpected pointer.\n");

    count = IMFMediaTimeRange_GetLength(range);
    ok(!count, "Unexpected count %lu.\n", count);

    IMFMediaTimeRange_Release(range);
    IMFMediaTimeRange_Release(range1);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(ret, "Unexpected state %d.\n", ret);

    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    pts = 0;
    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, &pts);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(pts == MINLONGLONG, "Unexpected timestamp.\n");

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!ret, "Unexpected state %d.\n", ret);

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, &pts);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!ret, "Unexpected state %d.\n", ret);

    IMFMediaEngine_Release(media_engine);

    /* Play -> Pause */
    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngine, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!ret, "Unexpected state %d.\n", ret);

    hr = IMFMediaEngine_Pause(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!!ret, "Unexpected state %d.\n", ret);

    IMFMediaEngine_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_playback_rate(void)
{
    struct media_engine_notify *notify;
    IMFMediaEngine *media_engine;
    double rate;
    HRESULT hr;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngine, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    rate = IMFMediaEngine_GetDefaultPlaybackRate(media_engine);
    ok(rate == 1.0, "Unexpected default rate.\n");

    rate = IMFMediaEngine_GetPlaybackRate(media_engine);
    ok(rate == 1.0, "Unexpected default rate.\n");

    hr = IMFMediaEngine_SetPlaybackRate(media_engine, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    rate = IMFMediaEngine_GetPlaybackRate(media_engine);
    ok(rate == 0.0, "Unexpected default rate.\n");

    hr = IMFMediaEngine_SetDefaultPlaybackRate(media_engine, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaEngine_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_mute(void)
{
    struct media_engine_notify *notify;
    IMFMediaEngine *media_engine;
    HRESULT hr;
    BOOL ret;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngine, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_GetMuted(media_engine);
    ok(!ret, "Unexpected state.\n");

    hr = IMFMediaEngine_SetMuted(media_engine, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_GetMuted(media_engine);
    ok(ret, "Unexpected state.\n");

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaEngine_GetMuted(media_engine);
    ok(ret, "Unexpected state.\n");

    IMFMediaEngine_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_error(void)
{
    struct media_engine_notify *notify;
    IMFMediaEngine *media_engine;
    IMFMediaError *eo, *eo2;
    unsigned int code;
    HRESULT hr;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngine, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    eo = (void *)0xdeadbeef;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!eo, "Unexpected instance.\n");

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_ENCRYPTED + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_ABORTED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    eo = NULL;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!eo, "Unexpected instance.\n");

    eo2 = NULL;
    hr = IMFMediaEngine_GetError(media_engine, &eo2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(eo2 != eo, "Unexpected instance.\n");

    IMFMediaError_Release(eo2);
    IMFMediaError_Release(eo);

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_NOERROR);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    eo = (void *)0xdeadbeef;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!eo, "Unexpected instance.\n");

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    eo = (void *)0xdeadbeef;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    ok(!eo, "Unexpected instance.\n");

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_NOERROR);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFMediaEngine_Release(media_engine);

    /* Error object. */
    hr = IMFMediaEngineClassFactory_CreateError(factory, &eo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    code = IMFMediaError_GetErrorCode(eo);
    ok(code == MF_MEDIA_ENGINE_ERR_NOERROR, "Unexpected code %u.\n", code);

    hr = IMFMediaError_GetExtendedErrorCode(eo);
    ok(hr == S_OK, "Unexpected code %#lx.\n", hr);

    hr = IMFMediaError_SetErrorCode(eo, MF_MEDIA_ENGINE_ERR_ENCRYPTED + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaError_SetErrorCode(eo, MF_MEDIA_ENGINE_ERR_ABORTED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    code = IMFMediaError_GetErrorCode(eo);
    ok(code == MF_MEDIA_ENGINE_ERR_ABORTED, "Unexpected code %u.\n", code);

    hr = IMFMediaError_SetExtendedErrorCode(eo, E_FAIL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaError_GetExtendedErrorCode(eo);
    ok(hr == E_FAIL, "Unexpected code %#lx.\n", hr);

    IMFMediaError_Release(eo);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_time_range(void)
{
    IMFMediaTimeRange *range;
    double start, end;
    DWORD count;
    HRESULT hr;
    BOOL ret;

    hr = IMFMediaEngineClassFactory_CreateTimeRange(factory, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Empty ranges. */
    hr = IMFMediaTimeRange_Clear(range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ret = IMFMediaTimeRange_ContainsTime(range, 10.0);
    ok(!ret, "Unexpected return value %d.\n", ret);

    count = IMFMediaTimeRange_GetLength(range);
    ok(!count, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Add a range. */
    hr = IMFMediaTimeRange_AddRange(range, 10.0, 1.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 1, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 10.0, "Unexpected start %.e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 1.0, "Unexpected end %.e.\n", end);

    hr = IMFMediaTimeRange_AddRange(range, 2.0, 3.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 1, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 2.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 3.0, "Unexpected end %.8e.\n", end);

    hr = IMFMediaTimeRange_AddRange(range, 10.0, 9.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 2, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 2.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 3.0, "Unexpected end %.8e.\n", end);

    start = 0.0;
    hr = IMFMediaTimeRange_GetStart(range, 1, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 10.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 1, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 9.0, "Unexpected end %.8e.\n", end);

    hr = IMFMediaTimeRange_AddRange(range, 2.0, 9.1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 2, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 2.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 9.1, "Unexpected end %.8e.\n", end);

    hr = IMFMediaTimeRange_GetStart(range, 1, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 10.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 1, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 9.0, "Unexpected end %.8e.\n", end);

    hr = IMFMediaTimeRange_AddRange(range, 8.5, 2.5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 2, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 2.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 9.1, "Unexpected end %.8e.\n", end);

    hr = IMFMediaTimeRange_AddRange(range, 20.0, 20.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 3, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_Clear(range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(!count, "Unexpected range count.\n");

    /* Intersect */
    hr = IMFMediaTimeRange_AddRange(range, 5.0, 10.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTimeRange_AddRange(range, 6.0, 12.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 5.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 12.0, "Unexpected end %.8e.\n", end);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 1, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_AddRange(range, 4.0, 6.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 1, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 4.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 12.0, "Unexpected end %.8e.\n", end);

    hr = IMFMediaTimeRange_AddRange(range, 5.0, 3.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IMFMediaTimeRange_GetLength(range);
    ok(count == 1, "Unexpected range count.\n");

    hr = IMFMediaTimeRange_GetStart(range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 4.0, "Unexpected start %.8e.\n", start);

    hr = IMFMediaTimeRange_GetEnd(range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(end == 12.0, "Unexpected end %.8e.\n", end);

    IMFMediaTimeRange_Release(range);
}

static void test_SetSourceFromByteStream(void)
{
    struct media_engine_notify *notify;
    IMFMediaEngineEx *media_engine;
    PROPVARIANT propvar;
    DWORD count, flags;
    HRESULT hr;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    if (FAILED(hr))
    {
        win_skip("IMFMediaEngineEx is not supported.\n");
        IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
        return;
    }

    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_GetResourceCharacteristics(media_engine, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_GetResourceCharacteristics(media_engine, &flags);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_GetPresentationAttribute(media_engine, &MF_PD_DURATION, &propvar);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_GetStreamAttribute(media_engine, 0, &MF_SD_PROTECTED, &propvar);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_GetNumberOfStreams(media_engine, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    count = 123;
    hr = IMFMediaEngineEx_GetNumberOfStreams(media_engine, &count);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(count == 123, "Unexpected value %lu.\n", count);

    IMFMediaEngineEx_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_audio_configuration(void)
{
    struct media_engine_notify *notify;
    IMFMediaEngineEx *media_engine;
    UINT32 value;
    HRESULT hr;

    notify = create_callback();

    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_UNKNOWN,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    if (FAILED(hr))
    {
        IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
        return;
    }

    hr = IMFMediaEngineEx_GetAudioStreamCategory(media_engine, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == AudioCategory_Other, "Unexpected value %u.\n", value);

    hr = IMFMediaEngineEx_GetAudioEndpointRole(media_engine, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == eMultimedia, "Unexpected value %u.\n", value);

    IMFMediaEngineEx_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static IMFByteStream *load_resource(const WCHAR *name, const WCHAR *mime)
{
    IMFAttributes *attributes;
    const BYTE *resource_data;
    IMFByteStream *stream;
    ULONG resource_len;
    HRSRC resource;
    HRESULT hr;

    resource = FindResourceW(NULL, name, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW %s failed, error %lu\n", debugstr_w(name), GetLastError());
    resource_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    resource_len = SizeofResource(GetModuleHandleW(NULL), resource);

    hr = MFCreateTempFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_Write(stream, resource_data, resource_len, &resource_len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_SetCurrentPosition(stream, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, mime);
    ok(hr == S_OK, "Failed to set string value, hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);

    return stream;
}

struct test_transfer_notify
{
    IMFMediaEngineNotify IMFMediaEngineNotify_iface;
    LONG refcount;

    IMFMediaEngineEx *media_engine;
    HANDLE ready_event, frame_ready_event;
    HRESULT error;
};

static struct test_transfer_notify *impl_from_test_transfer_notify(IMFMediaEngineNotify *iface)
{
    return CONTAINING_RECORD(iface, struct test_transfer_notify, IMFMediaEngineNotify_iface);
}

static HRESULT WINAPI test_transfer_notify_QueryInterface(IMFMediaEngineNotify *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IMFMediaEngineNotify))
    {
        *obj = iface;
        IMFMediaEngineNotify_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_transfer_notify_AddRef(IMFMediaEngineNotify *iface)
{
    struct test_transfer_notify *notify = impl_from_test_transfer_notify(iface);
    return InterlockedIncrement(&notify->refcount);
}

static ULONG WINAPI test_transfer_notify_Release(IMFMediaEngineNotify *iface)
{
    struct test_transfer_notify *notify = impl_from_test_transfer_notify(iface);
    ULONG refcount = InterlockedDecrement(&notify->refcount);

    if (!refcount)
    {
        CloseHandle(notify->frame_ready_event);
        CloseHandle(notify->ready_event);
        free(notify);
    }

    return refcount;
}

static HRESULT WINAPI test_transfer_notify_EventNotify(IMFMediaEngineNotify *iface, DWORD event, DWORD_PTR param1, DWORD param2)
{
    struct test_transfer_notify *notify = impl_from_test_transfer_notify(iface);
    IMFMediaEngineEx *media_engine = notify->media_engine;
    DWORD width, height;
    HRESULT hr;
    BOOL ret;

    switch (event)
    {
    case MF_MEDIA_ENGINE_EVENT_CANPLAY:
        hr = IMFMediaEngineEx_Play(media_engine);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        break;

    case MF_MEDIA_ENGINE_EVENT_FORMATCHANGE:
        ret = IMFMediaEngineEx_HasVideo(media_engine);
        ok(ret, "Unexpected HasVideo %u.\n", ret);
        hr = IMFMediaEngineEx_GetNativeVideoSize(media_engine, &width, &height);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(width == 64, "Unexpected width %lu.\n", width);
        ok(height == 64, "Unexpected height %lu.\n", height);
        break;

    case MF_MEDIA_ENGINE_EVENT_ERROR:
        ok(broken(param2 == MF_E_UNSUPPORTED_BYTESTREAM_TYPE), "Unexpected error %#lx\n", param2);
        notify->error = param2;
        /* fallthrough */
    case MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY:
        SetEvent(notify->frame_ready_event);
        break;
    case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
        SetEvent(notify->ready_event);
        break;
    }

    return S_OK;
}

static IMFMediaEngineNotifyVtbl test_transfer_notify_vtbl =
{
    test_transfer_notify_QueryInterface,
    test_transfer_notify_AddRef,
    test_transfer_notify_Release,
    test_transfer_notify_EventNotify,
};

static struct test_transfer_notify *create_transfer_notify(void)
{
    struct test_transfer_notify *object;

    object = calloc(1, sizeof(*object));
    object->IMFMediaEngineNotify_iface.lpVtbl = &test_transfer_notify_vtbl;
    object->refcount = 1;
    object->ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!object->ready_event, "Failed to create an event, error %lu.\n", GetLastError());

    object->frame_ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!object->frame_ready_event, "Failed to create an event, error %lu.\n", GetLastError());

    return object;
}

static void test_TransferVideoFrame(void)
{
    struct test_transfer_notify *notify;
    ID3D11Texture2D *texture = NULL, *rb_texture;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    IMFMediaEngineEx *media_engine = NULL;
    IWICImagingFactory *factory = NULL;
    IMFDXGIDeviceManager *manager;
    ID3D11DeviceContext *context;
    D3D11_TEXTURE2D_DESC desc;
    IWICBitmap *bitmap = NULL;
    IMFByteStream *stream;
    ID3D11Device *device;
    RECT dst_rect;
    UINT token;
    HRESULT hr;
    DWORD res;
    BSTR url;
    LONGLONG pts;

    stream = load_resource(L"i420-64x64.avi", L"video/avi");

    notify = create_transfer_notify();

    if (!(device = create_d3d11_device()))
    {
        skip("Failed to create a D3D11 device, skipping tests.\n");
        goto done;
    }

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_media_engine(&notify->IMFMediaEngineNotify_iface, manager, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);

    IMFDXGIDeviceManager_Release(manager);

    if (!(notify->media_engine = media_engine))
        goto done;

    memset(&desc, 0, sizeof(desc));
    desc.Width = 64;
    desc.Height = 64;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.SampleDesc.Count = 1;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(factory, desc.Width, desc.Height, &GUID_WICPixelFormat32bppBGR,
            WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    url = SysAllocString(L"i420-64x64.avi");
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, stream, url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(url);
    IMFByteStream_Release(stream);

    res = WaitForSingleObject(notify->frame_ready_event, 5000);
    ok(!res, "Unexpected res %#lx.\n", res);

    if (FAILED(notify->error))
    {
        win_skip("Media engine reported error %#lx, skipping tests.\n", notify->error);
        goto done;
    }

    res = 0;
    hr = IMFMediaEngineEx_GetNumberOfStreams(media_engine, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(res == 2, "Unexpected stream count %lu.\n", res);

    /* FIXME: Wine first video frame is often full of garbage, wait for another update */
    res = WaitForSingleObject(notify->ready_event, 500);
    /* It's also missing the MF_MEDIA_ENGINE_EVENT_TIMEUPDATE notifications */
    todo_wine
    ok(!res, "Unexpected res %#lx.\n", res);

    SetRect(&dst_rect, 0, 0, desc.Width, desc.Height);
    IMFMediaEngineEx_OnVideoStreamTick(notify->media_engine, &pts);
    hr = IMFMediaEngineEx_TransferVideoFrame(notify->media_engine, (IUnknown *)texture, NULL, &dst_rect, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ID3D11Texture2D_GetDesc(texture, &desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &rb_texture);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ID3D11Device_GetImmediateContext(device, &context);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)rb_texture,
            0, 0, 0, 0, (ID3D11Resource *)texture, 0, NULL);

    memset(&map_desc, 0, sizeof(map_desc));
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)rb_texture, 0, D3D11_MAP_READ, 0, &map_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!map_desc.pData, "got pData %p\n", map_desc.pData);
    ok(map_desc.DepthPitch == 16384, "got DepthPitch %u\n", map_desc.DepthPitch);
    ok(map_desc.RowPitch == desc.Width * 4, "got RowPitch %u\n", map_desc.RowPitch);
    res = check_rgb32_data(L"rgb32frame.bmp", map_desc.pData, map_desc.RowPitch * desc.Height, &dst_rect);
    ok(res == 0, "Unexpected %lu%% diff\n", res);
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)rb_texture, 0);

    ID3D11DeviceContext_Release(context);
    ID3D11Texture2D_Release(rb_texture);

    hr = IMFMediaEngineEx_TransferVideoFrame(notify->media_engine, (IUnknown *)bitmap, NULL, &dst_rect, NULL);
    /* not supported if a DXGI device manager was provided */
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

done:
    if (media_engine)
    {
        IMFMediaEngineEx_Shutdown(media_engine);
        IMFMediaEngineEx_Release(media_engine);
    }

    if (bitmap)
        IWICBitmap_Release(bitmap);
    if (factory)
        IWICImagingFactory_Release(factory);
    if (texture)
        ID3D11Texture2D_Release(texture);
    if (device)
        ID3D11Device_Release(device);

    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_TransferVideoFrame_wic(void)
{
    struct test_transfer_notify *notify;
    UINT lock_buffer_size, lock_buffer_stride;
    IMFMediaEngineEx *media_engine = NULL;
    IWICImagingFactory *factory = NULL;
    IWICBitmap *bitmap = NULL;
    IMFByteStream *stream;
    IWICBitmapLock *lock;
    WICRect wicrc = {0};
    BYTE *lock_buffer;
    RECT dst_rect;
    LONGLONG pts;
    HRESULT hr;
    DWORD res;
    BSTR url;

    stream = load_resource(L"i420-64x64.avi", L"video/avi");

    notify = create_transfer_notify();

    create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);

    if (!(notify->media_engine = media_engine))
        goto done;

    wicrc.Width = 64;
    wicrc.Height = 64;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(factory, wicrc.Width, wicrc.Height, &GUID_WICPixelFormat32bppBGR,
            WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    url = SysAllocString(L"i420-64x64.avi");
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, stream, url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(url);
    IMFByteStream_Release(stream);

    res = WaitForSingleObject(notify->frame_ready_event, 5000);
    ok(!res, "Unexpected res %#lx.\n", res);

    if (FAILED(notify->error))
    {
        win_skip("Media engine reported error %#lx, skipping tests.\n", notify->error);
        goto done;
    }

    /* FIXME: Wine first video frame is often full of garbage, wait for another update */
    res = WaitForSingleObject(notify->ready_event, 500);
    /* It's also missing the MF_MEDIA_ENGINE_EVENT_TIMEUPDATE notifications */
    todo_wine
    ok(!res, "Unexpected res %#lx.\n", res);

    SetRect(&dst_rect, 0, 0, wicrc.Width, wicrc.Height);
    IMFMediaEngineEx_OnVideoStreamTick(notify->media_engine, &pts);
    hr = IMFMediaEngineEx_TransferVideoFrame(notify->media_engine, (IUnknown *)bitmap, NULL, &dst_rect, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmap_Lock(bitmap, &wicrc, WICBitmapLockRead, &lock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!lock_buffer, "got null lock_buffer\n");
    ok(lock_buffer_size == 16384, "got lock_buffer_size %u\n", lock_buffer_size);
    ok(lock_buffer_stride == wicrc.Width * 4, "got lock_buffer_stride %u\n", lock_buffer_stride);
    res = check_rgb32_data(L"rgb32frame.bmp", lock_buffer, lock_buffer_stride * wicrc.Height, &dst_rect);
    ok(res == 0, "Unexpected %lu%% diff\n", res);

    IWICBitmapLock_Release(lock);

done:
    if (media_engine)
    {
        IMFMediaEngineEx_Shutdown(media_engine);
        IMFMediaEngineEx_Release(media_engine);
    }

    if (bitmap)
        IWICBitmap_Release(bitmap);
    if (factory)
        IWICImagingFactory_Release(factory);

    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

struct test_transform
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFAttributes *attributes;

    UINT input_count;
    IMFMediaType **input_types;
    IMFMediaType *input_type;

    UINT output_count;
    IMFMediaType **output_types;
    IMFMediaType *output_type;

    IMFSample *sample;
    UINT sample_count;
};

static struct test_transform *test_transform_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct test_transform, IMFTransform_iface);
}

static HRESULT WINAPI test_transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IMFTransform))
    {
        IMFTransform_AddRef(&transform->IMFTransform_iface);
        *out = &transform->IMFTransform_iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_transform_AddRef(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);
    return refcount;
}

static ULONG WINAPI test_transform_Release(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    if (!refcount)
    {
        if (transform->input_type)
            IMFMediaType_Release(transform->input_type);
        if (transform->output_type)
            IMFMediaType_Release(transform->output_type);
        free(transform);
    }

    return refcount;
}

static HRESULT WINAPI test_transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI test_transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    memset(info, 0, sizeof(*info));
    return S_OK;
}

static HRESULT WINAPI test_transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!(*attributes = transform->attributes))
        return E_NOTIMPL;
    IMFAttributes_AddRef(*attributes);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    if (index >= transform->input_count)
    {
        *type = NULL;
        return MF_E_NO_MORE_TYPES;
    }

    *type = transform->input_types[index];
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);

    if (index >= transform->output_count)
    {
        *type = NULL;
        return MF_E_NO_MORE_TYPES;
    }

    *type = transform->output_types[index];
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    GUID subtype, desired;
    HRESULT hr;

    if (type)
    {
        hr = IMFMediaType_GetGUID(transform->input_types[0], &MF_MT_SUBTYPE, &subtype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &desired);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (!IsEqualGUID(&subtype, &desired))
            return MF_E_INVALIDMEDIATYPE;
    }

    if (flags & MFT_SET_TYPE_TEST_ONLY)
    {
        todo_wine ok(0, "Unexpected %s call.\n", __func__);
        return winetest_platform_is_wine ? S_OK : E_NOTIMPL;
    }
    if (transform->input_type)
        IMFMediaType_Release(transform->input_type);
    if ((transform->input_type = type))
        IMFMediaType_AddRef(transform->input_type);
    return S_OK;
}

static HRESULT WINAPI test_transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    GUID subtype, desired;
    HRESULT hr;

    if (type)
    {
        hr = IMFMediaType_GetGUID(transform->output_types[0], &MF_MT_SUBTYPE, &subtype);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &desired);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (!IsEqualGUID(&subtype, &desired))
            return MF_E_INVALIDMEDIATYPE;
    }

    if (flags & MFT_SET_TYPE_TEST_ONLY)
    {
        todo_wine ok(0, "Unexpected %s call.\n", __func__);
        return winetest_platform_is_wine ? S_OK : E_NOTIMPL;
    }
    if (transform->output_type)
        IMFMediaType_Release(transform->output_type);
    if ((transform->output_type = type))
        IMFMediaType_AddRef(transform->output_type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!(*type = transform->input_type))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!(*type = transform->output_type))
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    IMFMediaType_AddRef(*type);
    return S_OK;
}

static HRESULT WINAPI test_transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    ok(0, "Unexpected %s call.\n", __func__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    return S_OK;
}

static HRESULT WINAPI test_transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (transform->sample)
        return MF_E_NOTACCEPTING;
    transform->sample = sample;
    IMFSample_AddRef(transform->sample);
    return S_OK;
}

static HRESULT WINAPI test_transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *data, DWORD *status)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    if (!transform->sample)
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    transform->sample_count++;
    data->pSample = transform->sample;
    transform->sample = NULL;
    *status = 0;
    return S_OK;
}

static UINT test_transform_get_sample_count(IMFTransform *iface)
{
    struct test_transform *transform = test_transform_from_IMFTransform(iface);
    return transform->sample_count;
}

static const IMFTransformVtbl test_transform_vtbl =
{
    test_transform_QueryInterface,
    test_transform_AddRef,
    test_transform_Release,
    test_transform_GetStreamLimits,
    test_transform_GetStreamCount,
    test_transform_GetStreamIDs,
    test_transform_GetInputStreamInfo,
    test_transform_GetOutputStreamInfo,
    test_transform_GetAttributes,
    test_transform_GetInputStreamAttributes,
    test_transform_GetOutputStreamAttributes,
    test_transform_DeleteInputStream,
    test_transform_AddInputStreams,
    test_transform_GetInputAvailableType,
    test_transform_GetOutputAvailableType,
    test_transform_SetInputType,
    test_transform_SetOutputType,
    test_transform_GetInputCurrentType,
    test_transform_GetOutputCurrentType,
    test_transform_GetInputStatus,
    test_transform_GetOutputStatus,
    test_transform_SetOutputBounds,
    test_transform_ProcessEvent,
    test_transform_ProcessMessage,
    test_transform_ProcessInput,
    test_transform_ProcessOutput,
};

static HRESULT WINAPI test_transform_create(UINT input_count, IMFMediaType **input_types,
        UINT output_count, IMFMediaType **output_types, IMFTransform **out)
{
    struct test_transform *transform;

    if (!(transform = calloc(1, sizeof(*transform))))
        return E_OUTOFMEMORY;
    transform->IMFTransform_iface.lpVtbl = &test_transform_vtbl;
    transform->refcount = 1;

    transform->input_count = input_count;
    transform->input_types = input_types;
    transform->input_type = input_types[0];
    IMFMediaType_AddRef(transform->input_type);
    transform->output_count = output_count;
    transform->output_types = output_types;
    transform->output_type = output_types[0];
    IMFMediaType_AddRef(transform->output_type);

    *out = &transform->IMFTransform_iface;
    return S_OK;
}

static void test_effect(void)
{
    IMFTransform *video_effect = NULL, *video_effect2 = NULL, *audio_effect = NULL, *audio_effect2 = NULL;
    IMFMediaType *video_i420, *video_rgb32, *audio_pcm;
    IMFMediaEngineEx *media_engine = NULL;
    struct test_transfer_notify *notify;
    ID3D11Texture2D *texture = NULL;
    IMFDXGIDeviceManager *manager;
    ID3D11Device *device = NULL;
    D3D11_TEXTURE2D_DESC desc;
    IMFByteStream *stream;
    IMFMediaSink *sink;
    UINT token, count;
    RECT dst_rect;
    HRESULT hr;
    DWORD res;
    BSTR url;

    stream = load_resource(L"i420-64x64.avi", L"video/avi");

    notify = create_transfer_notify();

    hr = MFCreateMediaType(&video_i420);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(video_i420, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(video_i420, &MF_MT_SUBTYPE, &MFVideoFormat_I420);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(video_i420, &MF_MT_FRAME_SIZE, (UINT64)64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&video_rgb32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(video_rgb32, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(video_rgb32, &MF_MT_SUBTYPE, &MFVideoFormat_ARGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(video_rgb32, &MF_MT_FRAME_SIZE, (UINT64)64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&audio_pcm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(audio_pcm, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(audio_pcm, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(audio_pcm, &MF_MT_AUDIO_NUM_CHANNELS, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(audio_pcm, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(audio_pcm, &MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(audio_pcm, &MF_MT_AUDIO_BLOCK_ALIGNMENT, 8);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(audio_pcm, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 352800);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (!(device = create_d3d11_device()))
    {
        skip("Failed to create a D3D11 device, skipping tests.\n");
        goto done;
    }

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    create_media_engine(&notify->IMFMediaEngineNotify_iface, manager, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    IMFDXGIDeviceManager_Release(manager);
    notify->media_engine = media_engine;

    memset(&desc, 0, sizeof(desc));
    desc.Width = 64;
    desc.Height = 64;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.SampleDesc.Count = 1;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_RemoveAllEffects(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = test_transform_create(1, &video_rgb32, 1, &video_rgb32, &video_effect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = test_transform_create(1, &video_i420, 1, &video_i420, &video_effect2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)video_effect, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_effect, 2);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)video_effect2, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_effect2, 2);

    hr = IMFMediaEngineEx_RemoveAllEffects(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_effect, 1);
    EXPECT_REF(video_effect2, 1);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)video_effect, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_effect, 2);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)video_effect2, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_effect2, 2);

    hr = test_transform_create(1, &audio_pcm, 1, &audio_pcm, &audio_effect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = test_transform_create(1, &audio_pcm, 1, &audio_pcm, &audio_effect2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_InsertAudioEffect(media_engine, (IUnknown *)audio_effect, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(audio_effect, 2);

    hr = IMFMediaEngineEx_InsertAudioEffect(media_engine, (IUnknown *)audio_effect2, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(audio_effect2, 2);

    url = SysAllocString(L"i420-64x64.avi");
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, stream, url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(url);
    IMFByteStream_Release(stream);

    /* Wait for MediaEngine to be ready. */
    res = WaitForSingleObject(notify->frame_ready_event, 5000);
    ok(!res, "Unexpected res %#lx.\n", res);

    SetRect(&dst_rect, 0, 0, desc.Width, desc.Height);
    hr = IMFMediaEngineEx_TransferVideoFrame(notify->media_engine, (IUnknown *)texture, NULL, &dst_rect, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = test_transform_get_sample_count(video_effect);
    ok(count > 0, "Unexpected processing count %u.\n", count);
    count = test_transform_get_sample_count(video_effect2);
    ok(count > 0, "Unexpected processing count %u.\n", count);

    if (SUCCEEDED(hr = MFCreateAudioRenderer(NULL, &sink)))
    {
        count = test_transform_get_sample_count(audio_effect);
        ok(count > 0, "Unexpected processing count %u.\n", count);
        count = test_transform_get_sample_count(audio_effect2);
        ok(count > 0, "Unexpected processing count %u.\n", count);

        IMFMediaSink_Release(sink);
    }
    else if (hr == MF_E_NO_AUDIO_PLAYBACK_DEVICE)
    {
        count = test_transform_get_sample_count(audio_effect);
        ok(!count, "Unexpected processing count %u.\n", count);
        count = test_transform_get_sample_count(audio_effect2);
        ok(!count, "Unexpected processing count %u.\n", count);
    }

    hr = IMFMediaEngineEx_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaEngineEx_RemoveAllEffects(media_engine);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    IMFMediaEngineEx_Release(media_engine);

    ID3D11Texture2D_Release(texture);

    IMFTransform_Release(audio_effect2);
    IMFTransform_Release(audio_effect);
    IMFTransform_Release(video_effect2);
    IMFTransform_Release(video_effect);

    ID3D11Device_Release(device);

done:
    IMFMediaType_Release(audio_pcm);
    IMFMediaType_Release(video_rgb32);
    IMFMediaType_Release(video_i420);

    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

static void test_GetDuration(void)
{
    static const double allowed_error = 0.000001;
    struct test_transfer_notify *notify;
    IMFMediaEngineEx *media_engine;
    IMFByteStream *stream;
    double duration;
    HRESULT hr;
    DWORD res;
    BSTR url;

    notify = create_transfer_notify();
    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    notify->media_engine = media_engine;

    stream = load_resource(L"i420-64x64.avi", L"video/avi");
    url = SysAllocString(L"i420-64x64.avi");
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, stream, url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    res = WaitForSingleObject(notify->frame_ready_event, 5000);
    ok(!res, "Unexpected res %#lx.\n", res);

    duration = IMFMediaEngineEx_GetDuration(media_engine);
    ok(compare_double(duration, 0.133467, allowed_error), "Got unexpected duration %lf.\n", duration);

    SysFreeString(url);
    IMFByteStream_Release(stream);
    IMFMediaEngineEx_Shutdown(media_engine);
    IMFMediaEngineEx_Release(media_engine);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

struct unseekable_stream
{
    IMFByteStream IMFByteStream_iface;
    IMFByteStream *original_stream;
    LONG refcount;
};

static struct unseekable_stream *impl_unseekable_stream_from_IMFByteStream(IMFByteStream *iface)
{
    return CONTAINING_RECORD(iface, struct unseekable_stream, IMFByteStream_iface);
}

static HRESULT WINAPI unseekable_stream_QueryInterface(IMFByteStream *iface,
        REFIID riid, void **out)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IMFByteStream))
    {
        IMFByteStream_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    return IMFByteStream_QueryInterface(stream->original_stream, riid, out);
}

static ULONG WINAPI unseekable_stream_AddRef(IMFByteStream *iface)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return InterlockedIncrement(&stream->refcount);
}

static ULONG WINAPI unseekable_stream_Release(IMFByteStream *iface)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    if (!refcount)
    {
        IMFByteStream_Release(stream->original_stream);
        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI unseekable_stream_GetCapabilities(IMFByteStream *iface,
        DWORD *capabilities)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);
    HRESULT hr;

    hr = IMFByteStream_GetCapabilities(stream->original_stream, capabilities);
    if (SUCCEEDED(hr))
        *capabilities &= ~(MFBYTESTREAM_IS_SEEKABLE | MFBYTESTREAM_HAS_SLOW_SEEK);

    return hr;
}

static HRESULT WINAPI unseekable_stream_GetLength(IMFByteStream *iface, QWORD *length)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_GetLength(stream->original_stream, length);
}

static HRESULT WINAPI unseekable_stream_SetLength(IMFByteStream *iface, QWORD length)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_SetLength(stream->original_stream, length);
}

static HRESULT WINAPI unseekable_stream_GetCurrentPosition(IMFByteStream *iface,
        QWORD *position)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_GetCurrentPosition(stream->original_stream, position);
}

static HRESULT WINAPI unseekable_stream_SetCurrentPosition(IMFByteStream *iface,
        QWORD position)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_SetCurrentPosition(stream->original_stream, position);
}

static HRESULT WINAPI unseekable_stream_IsEndOfStream(IMFByteStream *iface, BOOL *eos)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_IsEndOfStream(stream->original_stream, eos);
}

static HRESULT WINAPI unseekable_stream_Read(IMFByteStream *iface, BYTE *data,
        ULONG count, ULONG *byte_read)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_Read(stream->original_stream, data, count, byte_read);
}

static HRESULT WINAPI unseekable_stream_BeginRead(IMFByteStream *iface, BYTE *data,
        ULONG size, IMFAsyncCallback *callback, IUnknown *state)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_BeginRead(stream->original_stream, data, size, callback, state);
}

static HRESULT WINAPI unseekable_stream_EndRead(IMFByteStream *iface,
        IMFAsyncResult *result, ULONG *byte_read)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_EndRead(stream->original_stream, result, byte_read);
}

static HRESULT WINAPI unseekable_stream_Write(IMFByteStream *iface, const BYTE *data,
        ULONG count, ULONG *written)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_Write(stream->original_stream, data, count, written);
}

static HRESULT WINAPI unseekable_stream_BeginWrite(IMFByteStream *iface,
        const BYTE *data, ULONG size, IMFAsyncCallback *callback, IUnknown *state)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_BeginWrite(stream->original_stream, data, size, callback, state);
}

static HRESULT WINAPI unseekable_stream_EndWrite(IMFByteStream *iface,
        IMFAsyncResult *result, ULONG *written)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_EndWrite(stream->original_stream, result, written);
}

static HRESULT WINAPI unseekable_stream_Seek(IMFByteStream *iface,
        MFBYTESTREAM_SEEK_ORIGIN seek, LONGLONG offset, DWORD flags, QWORD *current)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_Seek(stream->original_stream, seek, offset, flags, current);
}

static HRESULT WINAPI unseekable_stream_Flush(IMFByteStream *iface)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_Flush(stream->original_stream);
}

static HRESULT WINAPI unseekable_stream_Close(IMFByteStream *iface)
{
    struct unseekable_stream *stream = impl_unseekable_stream_from_IMFByteStream(iface);

    return IMFByteStream_Close(stream->original_stream);
}

static const IMFByteStreamVtbl unseekable_stream_vtbl =
{
    unseekable_stream_QueryInterface,
    unseekable_stream_AddRef,
    unseekable_stream_Release,
    unseekable_stream_GetCapabilities,
    unseekable_stream_GetLength,
    unseekable_stream_SetLength,
    unseekable_stream_GetCurrentPosition,
    unseekable_stream_SetCurrentPosition,
    unseekable_stream_IsEndOfStream,
    unseekable_stream_Read,
    unseekable_stream_BeginRead,
    unseekable_stream_EndRead,
    unseekable_stream_Write,
    unseekable_stream_BeginWrite,
    unseekable_stream_EndWrite,
    unseekable_stream_Seek,
    unseekable_stream_Flush,
    unseekable_stream_Close,
};

static IMFByteStream *create_unseekable_stream(IMFByteStream *stream)
{
    struct unseekable_stream *object;

    object = calloc(1, sizeof(*object));
    object->IMFByteStream_iface.lpVtbl = &unseekable_stream_vtbl;
    object->original_stream = stream;
    IMFByteStream_AddRef(stream);
    object->refcount = 1;

    return &object->IMFByteStream_iface;
}

struct test_seek_notify
{
    IMFMediaEngineNotify IMFMediaEngineNotify_iface;
    HANDLE playing_event;
    HANDLE seeking_event;
    HANDLE seeked_event;
    HANDLE time_update_event;
    BOOL seeking_event_received;
    BOOL time_update_event_received;
    HRESULT expected_error;
    HRESULT error;
    LONG refcount;
};

static struct test_seek_notify *impl_from_test_seek_notify(IMFMediaEngineNotify *iface)
{
    return CONTAINING_RECORD(iface, struct test_seek_notify, IMFMediaEngineNotify_iface);
}

static HRESULT WINAPI test_seek_notify_QueryInterface(IMFMediaEngineNotify *iface, REFIID riid,
        void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMFMediaEngineNotify))
    {
        *obj = iface;
        IMFMediaEngineNotify_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_seek_notify_AddRef(IMFMediaEngineNotify *iface)
{
    struct test_seek_notify *notify = impl_from_test_seek_notify(iface);

    return InterlockedIncrement(&notify->refcount);
}

static ULONG WINAPI test_seek_notify_Release(IMFMediaEngineNotify *iface)
{
    struct test_seek_notify *notify = impl_from_test_seek_notify(iface);
    ULONG refcount = InterlockedDecrement(&notify->refcount);

    if (!refcount)
    {
        CloseHandle(notify->playing_event);
        CloseHandle(notify->seeking_event);
        CloseHandle(notify->seeked_event);
        CloseHandle(notify->time_update_event);
        free(notify);
    }

    return refcount;
}

static HRESULT WINAPI test_seek_notify_EventNotify(IMFMediaEngineNotify *iface, DWORD event,
        DWORD_PTR param1, DWORD param2)
{
    struct test_seek_notify *notify = impl_from_test_seek_notify(iface);

    switch (event)
    {
    case MF_MEDIA_ENGINE_EVENT_PLAYING:
        SetEvent(notify->playing_event);
        break;
    case MF_MEDIA_ENGINE_EVENT_SEEKING:
        notify->seeking_event_received = TRUE;
        SetEvent(notify->seeking_event);
        break;
    case MF_MEDIA_ENGINE_EVENT_SEEKED:
        SetEvent(notify->seeked_event);
        break;
    case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
        notify->time_update_event_received = TRUE;
        SetEvent(notify->time_update_event);
        break;
    case MF_MEDIA_ENGINE_EVENT_ERROR:
        ok(param2 == notify->expected_error, "Unexpected error %#lx\n", param2);
        notify->error = param2;
        break;
    }

    return S_OK;
}

static IMFMediaEngineNotifyVtbl test_seek_notify_vtbl =
{
    test_seek_notify_QueryInterface,
    test_seek_notify_AddRef,
    test_seek_notify_Release,
    test_seek_notify_EventNotify,
};

static struct test_seek_notify *create_seek_notify(void)
{
    struct test_seek_notify *object;

    object = calloc(1, sizeof(*object));
    object->IMFMediaEngineNotify_iface.lpVtbl = &test_seek_notify_vtbl;
    object->playing_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    object->seeking_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    object->seeked_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    object->time_update_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!object->playing_event, "Failed to create an event, error %lu.\n", GetLastError());
    ok(!!object->seeking_event, "Failed to create an event, error %lu.\n", GetLastError());
    ok(!!object->seeked_event, "Failed to create an event, error %lu.\n", GetLastError());
    ok(!!object->time_update_event, "Failed to create an event, error %lu.\n", GetLastError());
    object->refcount = 1;
    return object;
}

static void test_GetSeekable(void)
{
    IMFByteStream *stream, *unseekable_stream = NULL;
    struct test_seek_notify *notify;
    IMFMediaEngineEx *media_engine;
    IMFMediaTimeRange *time_range;
    double start, end, duration;
    ULONG refcount;
    DWORD count;
    HRESULT hr;
    DWORD res;
    BSTR url;

    notify = create_seek_notify();
    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);

    stream = load_resource(L"i420-64x64.avi", L"video/avi");
    url = SysAllocString(L"i420-64x64.avi");
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, stream, url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Media engine is not ready */
    hr = IMFMediaEngineEx_GetSeekable(media_engine, &time_range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    count = IMFMediaTimeRange_GetLength(time_range);
    ok(!count, "Unexpected count %lu.\n", count);
    refcount = IMFMediaTimeRange_Release(time_range);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

    hr = IMFMediaEngineEx_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    res = WaitForSingleObject(notify->playing_event, 5000);
    ok(!res, "Unexpected res %#lx.\n", res);
    if (FAILED(notify->error))
    {
        win_skip("Media engine reported error %#lx, skipping tests.\n", notify->error);
        goto done;
    }

    /* Media engine is ready */
    hr = IMFMediaEngineEx_GetSeekable(media_engine, &time_range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    count = IMFMediaTimeRange_GetLength(time_range);
    ok(count == 1, "Unexpected count %lu.\n", count);
    hr = IMFMediaTimeRange_GetStart(time_range, 0, &start);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(start == 0, "Unexpected start %lf.\n", start);
    hr = IMFMediaTimeRange_GetEnd(time_range, 0, &end);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    duration = IMFMediaEngineEx_GetDuration(media_engine);
    ok(end == duration, "Unexpected end %lf.\n", end);
    refcount = IMFMediaTimeRange_Release(time_range);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

    /* Media engine is shut down */
    hr = IMFMediaEngineEx_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    time_range = (IMFMediaTimeRange *)0xdeadbeef;
    hr = IMFMediaEngineEx_GetSeekable(media_engine, &time_range);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    ok(time_range == NULL || broken(time_range == (IMFMediaTimeRange *)0xdeadbeef) /* <= Win10 1507 */,
       "Got unexpected pointer.\n");

    /* IMFMediaEngineEx_Shutdown can release in parallel. A small sleep allows this test to pass more
     * often than not. But given its a matter of timing, this test is marked flaky
     */
    Sleep(10);
    refcount = IMFMediaEngineEx_Release(media_engine);
    flaky_wine
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

    /* Unseekable bytestreams */
    notify = create_seek_notify();
    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
    unseekable_stream = create_unseekable_stream(stream);
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, unseekable_stream, url);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
        goto done;

    hr = IMFMediaEngineEx_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    notify->expected_error = MF_E_INVALIDREQUEST;
    res = WaitForSingleObject(notify->playing_event, 5000);
    ok(res == S_OK, "Unexpected res %#lx.\n", res);

    hr = IMFMediaEngineEx_GetSeekable(media_engine, &time_range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    count = IMFMediaTimeRange_GetLength(time_range);
    ok(!count, "Unexpected count %lu.\n", count);
    refcount = IMFMediaTimeRange_Release(time_range);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

done:
    hr = IMFMediaEngineEx_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaEngineEx_Release(media_engine);
    if (unseekable_stream) IMFByteStream_Release(unseekable_stream);
    SysFreeString(url);
    IMFByteStream_Release(stream);
}

struct test_extension
{
    IMFMediaEngineExtension IMFMediaEngineExtension_iface;
    LONG refcount;
};

static struct test_extension *impl_from_IMFMediaEngineExtension(IMFMediaEngineExtension *iface)
{
    return CONTAINING_RECORD(iface, struct test_extension, IMFMediaEngineExtension_iface);
}

static HRESULT WINAPI test_extension_QI(IMFMediaEngineExtension *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaEngineExtension) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngineExtension_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_extension_AddRef(IMFMediaEngineExtension *iface)
{
    struct test_extension *extension = impl_from_IMFMediaEngineExtension(iface);
    return InterlockedIncrement(&extension->refcount);
}

static ULONG WINAPI test_extension_Release(IMFMediaEngineExtension *iface)
{
    struct test_extension *extension = impl_from_IMFMediaEngineExtension(iface);
    ULONG refcount = InterlockedDecrement(&extension->refcount);

    if (!refcount)
        free(extension);

    return refcount;
}

static HRESULT WINAPI test_extension_CanPlayType(IMFMediaEngineExtension *iface,
        BOOL audio_only, BSTR mime_type, MF_MEDIA_ENGINE_CANPLAY *answer)
{
    return 0x80001234;
}

static HRESULT WINAPI test_extension_BeginCreateObject(IMFMediaEngineExtension *iface,
        BSTR url, IMFByteStream *bytestream, MF_OBJECT_TYPE type, IUnknown **cancel_cookie,
        IMFAsyncCallback *callback, IUnknown *state)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_extension_CancelObjectCreation(IMFMediaEngineExtension *iface,
        IUnknown *cancel_cookie)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_extension_EndCreateObject(IMFMediaEngineExtension *iface, IMFAsyncResult *result,
        IUnknown **object)
{
    return E_NOTIMPL;
}

static const IMFMediaEngineExtensionVtbl test_extension_vtbl =
{
    test_extension_QI,
    test_extension_AddRef,
    test_extension_Release,
    test_extension_CanPlayType,
    test_extension_BeginCreateObject,
    test_extension_CancelObjectCreation,
    test_extension_EndCreateObject,
};

static struct test_extension *create_extension(void)
{
    struct test_extension *object;

    object = calloc(1, sizeof(*object));

    object->IMFMediaEngineExtension_iface.lpVtbl = &test_extension_vtbl;
    object->refcount = 1;

    return object;
}

static void test_media_extension(void)
{
    struct media_engine_notify *notify;
    struct test_extension *extension;
    MF_MEDIA_ENGINE_CANPLAY answer;
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    HRESULT hr;
    BSTR mime;

    notify = create_callback();

    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    extension = create_extension();
    ok(!!extension, "Failed to create an extension.\n");

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)&notify->IMFMediaEngineNotify_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_UNKNOWN);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_EXTENSION, (IUnknown *)&extension->IMFMediaEngineExtension_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_AUDIOONLY, attributes, &media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_Release(attributes);

    mime = SysAllocString(L"doesnotexist");
    hr = IMFMediaEngine_CanPlayType(media_engine, mime, &answer);
    ok(hr == 0x80001234, "Unexpected hr %#lx.\n", hr);
    SysFreeString(mime);

    IMFMediaEngine_Release(media_engine);

    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
    IMFMediaEngineExtension_Release(&extension->IMFMediaEngineExtension_iface);
}

#define test_seek_result(a, b, c) _test_seek_result(__LINE__, a, b, c)
static void _test_seek_result(int line, IMFMediaEngineEx *media_engine,
        struct test_seek_notify *notify, double expected_time)
{
    static const double allowed_error = 0.05;
    static const int timeout = 1000;
    double time;
    DWORD res;

    ok(notify->seeking_event_received, "Seeking event not received.\n");
    notify->seeking_event_received = FALSE;
    res = WaitForSingleObject(notify->seeking_event, timeout);
    ok_(__FILE__, line)(!res, "Waiting for seeking event returned %#lx.\n", res);
    res = WaitForSingleObject(notify->seeked_event, timeout);
    ok_(__FILE__, line)(!res, "Waiting for seeked event returned %#lx.\n", res);
    res = WaitForSingleObject(notify->time_update_event, timeout);
    ok_(__FILE__, line)(!res, "Waiting for ready event returned %#lx.\n", res);
    time = IMFMediaEngineEx_GetCurrentTime(media_engine);
    ok_(__FILE__, line)(compare_double(time, expected_time, allowed_error), "Unexpected time %lf.\n", time);
}

static void test_SetCurrentTime(void)
{
    static const double allowed_error = 0.05;
    static const int timeout = 1000;
    IMFByteStream *stream, *unseekable_stream = NULL;
    double time, duration, start, end;
    struct test_seek_notify *notify;
    IMFMediaEngineEx *media_engine;
    ULONG refcount;
    HRESULT hr;
    DWORD res;
    BOOL ret;
    BSTR url;

    notify = create_seek_notify();
    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);

    stream = load_resource(L"i420-64x64.avi", L"video/avi");
    url = SysAllocString(L"i420-64x64.avi");
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, stream, url);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    res = WaitForSingleObject(notify->playing_event, 5000);
    ok(!res, "Unexpected res %#lx.\n", res);

    duration = IMFMediaEngineEx_GetDuration(media_engine);
    ok(duration > 0, "Got invalid duration.\n");
    start = 0;
    end = duration;

    /* Test playing state */
    hr = IMFMediaEngineEx_SetCurrentTime(media_engine, end);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        test_seek_result(media_engine, notify, end);

    /* Test seeking with a negative position */
    hr = IMFMediaEngineEx_SetCurrentTime(media_engine, -1);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        test_seek_result(media_engine, notify, 0);

    /* Test seeking beyond duration */
    hr = IMFMediaEngineEx_SetCurrentTime(media_engine, end + 1);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        test_seek_result(media_engine, notify, end);

    hr = IMFMediaEngineEx_SetCurrentTimeEx(media_engine, start, MF_MEDIA_ENGINE_SEEK_MODE_NORMAL);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        test_seek_result(media_engine, notify, start);

    hr = IMFMediaEngineEx_SetCurrentTimeEx(media_engine, end, MF_MEDIA_ENGINE_SEEK_MODE_APPROXIMATE);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
        test_seek_result(media_engine, notify, end);

    /* Test paused state */
    hr = IMFMediaEngineEx_Pause(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_SetCurrentTime(media_engine, start);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(notify->seeking_event_received, "Seeking event not received.\n");
        notify->seeking_event_received = FALSE;
        ok(notify->time_update_event_received, "Time update event not received.\n");
        notify->time_update_event_received = FALSE;
        res = WaitForSingleObject(notify->seeking_event, timeout);
        ok(!res, "Unexpected res %#lx.\n", res);
        res = WaitForSingleObject(notify->seeked_event, timeout);
        ok(res == WAIT_TIMEOUT || res == 0, /* No timeout sometimes on Win10+ */
                "Unexpected res %#lx.\n", res);
        res = WaitForSingleObject(notify->time_update_event, timeout);
        ok(!res, "Unexpected res %#lx.\n", res);
        time = IMFMediaEngineEx_GetCurrentTime(media_engine);
        ok(compare_double(time, start, allowed_error), "Unexpected time %lf.\n", time);
    }

    Sleep(end * 1000);

    ret = IMFMediaEngineEx_IsPaused(media_engine);
    ok(ret, "Unexpected ret %d.\n", ret);
    time = IMFMediaEngineEx_GetCurrentTime(media_engine);
    ok(compare_double(time, start, allowed_error)
            || broken(time >= end) /* Windows 11 21H2 AMD GPU TestBot */, "Unexpected time %lf.\n", time);

    hr = IMFMediaEngineEx_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    res = WaitForSingleObject(notify->seeked_event, timeout);
    ok(res == WAIT_TIMEOUT, "Unexpected res %#lx.\n", res);

    /* Media engine is shut down */
    hr = IMFMediaEngineEx_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_SetCurrentTime(media_engine, start);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaEngineEx_SetCurrentTimeEx(media_engine, start, MF_MEDIA_ENGINE_SEEK_MODE_NORMAL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    refcount = IMFMediaEngineEx_Release(media_engine);
    flaky_wine
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

    /* Unseekable bytestreams */
    notify = create_seek_notify();
    hr = create_media_engine(&notify->IMFMediaEngineNotify_iface, NULL, DXGI_FORMAT_B8G8R8X8_UNORM,
            &IID_IMFMediaEngineEx, (void **)&media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
    unseekable_stream = create_unseekable_stream(stream);
    hr = IMFMediaEngineEx_SetSourceFromByteStream(media_engine, unseekable_stream, url);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
        goto done;

    hr = IMFMediaEngineEx_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    notify->expected_error = MF_E_INVALIDREQUEST;
    res = WaitForSingleObject(notify->playing_event, 5000);
    ok(res == S_OK, "Unexpected res %#lx.\n", res);

    hr = IMFMediaEngineEx_SetCurrentTime(media_engine, end);
    ok(hr == S_OK || broken(hr == MF_INVALID_STATE_ERR) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(!notify->seeking_event_received, "Seeking event received.\n");
        res = WaitForSingleObject(notify->seeking_event, timeout);
        ok(res == WAIT_TIMEOUT, "Unexpected res %#lx.\n", res);
        res = WaitForSingleObject(notify->seeked_event, timeout);
        ok(res == WAIT_TIMEOUT, "Unexpected res %#lx.\n", res);
        res = WaitForSingleObject(notify->time_update_event, timeout);
        ok(!res, "Unexpected res %#lx.\n", res);
    }

done:
    hr = IMFMediaEngineEx_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    refcount = IMFMediaEngineEx_Release(media_engine);
    ok(!refcount || broken(refcount == 1) /* Win8.1 */, "Got unexpected refcount %lu.\n", refcount);
    IMFByteStream_Release(unseekable_stream);
    SysFreeString(url);
    IMFByteStream_Release(stream);
}

START_TEST(mfmediaengine)
{
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFMediaEngineClassFactory,
            (void **)&factory);
    if (FAILED(hr))
    {
        win_skip("Media Engine is not supported.\n");
        CoUninitialize();
        return;
    }

    init_functions();

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "MFStartup failed: %#lx.\n", hr);

    test_factory();
    test_CreateInstance();
    test_Shutdown();
    test_Play();
    test_playback_rate();
    test_mute();
    test_error();
    test_time_range();
    test_SetSourceFromByteStream();
    test_audio_configuration();
    test_TransferVideoFrame();
    test_TransferVideoFrame_wic();
    test_effect();
    test_GetDuration();
    test_GetSeekable();
    test_media_extension();
    test_SetCurrentTime();

    IMFMediaEngineClassFactory_Release(factory);

    CoUninitialize();

    MFShutdown();
}
