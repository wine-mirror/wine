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

static DWORD compare_rgb32(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect)
{
    DWORD x, y, size, diff = 0, width = (rect->right + 0xf) & ~0xf, height = (rect->bottom + 0xf) & ~0xf;

    /* skip BMP header from the dump */
    size = *(DWORD *)(expect + 2 + 2 * sizeof(DWORD));
    *length = *length + size;
    expect = expect + size;

    for (y = 0; y < height; y++, data += width * 4, expect += width * 4)
    {
        if (y < rect->top || y >= rect->bottom) continue;
        for (x = 0; x < width; x++)
        {
            if (x < rect->left || x >= rect->right) continue;
            diff += abs((int)expect[4 * x + 0] - (int)data[4 * x + 0]);
            diff += abs((int)expect[4 * x + 1] - (int)data[4 * x + 1]);
            diff += abs((int)expect[4 * x + 2] - (int)data[4 * x + 2]);
        }
    }

    size = (rect->right - rect->left) * (rect->bottom - rect->top) * 3;
    return diff * 100 / 256 / size;
}

static void dump_rgb32(const BYTE *data, DWORD length, const RECT *rect, HANDLE output)
{
    DWORD width = (rect->right + 0xf) & ~0xf, height = (rect->bottom + 0xf) & ~0xf;
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
            .biBitCount = 32, .biCompression = BI_RGB, .biSizeImage = width * height * 4,
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

#define check_rgb32_data(a, b, c, d) check_rgb32_data_(__LINE__, a, b, c, d)
static void check_rgb32_data_(int line, const WCHAR *filename, const BYTE *data, DWORD length, const RECT *rect)
{
    WCHAR output_path[MAX_PATH];
    const BYTE *expect_data;
    HRSRC resource;
    HANDLE output;
    DWORD diff;

    GetTempPathW(ARRAY_SIZE(output_path), output_path);
    lstrcatW(output_path, filename);
    output = CreateFileW(output_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());
    dump_rgb32(data, length, rect, output);
    trace("created %s\n", debugstr_w(output_path));
    CloseHandle(output);

    resource = FindResourceW(NULL, filename, (const WCHAR *)RT_RCDATA);
    ok(resource != 0, "FindResourceW failed, error %lu\n", GetLastError());
    expect_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));

    diff = compare_rgb32(data, &length, rect, expect_data);
    ok_(__FILE__, line)(diff == 0, "Unexpected %lu%% diff\n", diff);
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
    IMFDXGIDeviceManager *manager;
    ID3D11DeviceContext *context;
    D3D11_TEXTURE2D_DESC desc;
    IMFByteStream *stream;
    ID3D11Device *device;
    RECT dst_rect;
    UINT token;
    HRESULT hr;
    DWORD res;
    BSTR url;

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
    check_rgb32_data(L"rgb32frame.bmp", map_desc.pData, map_desc.RowPitch * desc.Height, &dst_rect);
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)rb_texture, 0);

    ID3D11DeviceContext_Release(context);
    ID3D11Texture2D_Release(rb_texture);

done:
    if (media_engine)
    {
        IMFMediaEngineEx_Shutdown(media_engine);
        IMFMediaEngineEx_Release(media_engine);
    }

    if (texture)
        ID3D11Texture2D_Release(texture);
    if (device)
        ID3D11Device_Release(device);

    IMFMediaEngineNotify_Release(&notify->IMFMediaEngineNotify_iface);
}

struct passthrough_mft
{
    IMFTransform IMFTransform_iface;
    LONG refcount;

    IMFMediaType *media_type_in, *media_type_out;
    IMFSample *sample;
    LONG processing_count;
    UINT32 index;

    CRITICAL_SECTION cs;
};

static struct passthrough_mft *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct passthrough_mft, IMFTransform_iface);
}

static HRESULT WINAPI passthrough_mft_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IMFTransform))
    {
        *out = &impl->IMFTransform_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI passthrough_mft_AddRef(IMFTransform *iface)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);
    return refcount;
}

static ULONG WINAPI passthrough_mft_Release(IMFTransform *iface)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    if (!refcount)
    {
        if (impl->media_type_out) IMFMediaType_Release(impl->media_type_out);
        if (impl->media_type_in) IMFMediaType_Release(impl->media_type_in);
        DeleteCriticalSection(&impl->cs);
        free(impl);
    }

    return refcount;
}

static HRESULT WINAPI passthrough_mft_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI passthrough_mft_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    *inputs = *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI passthrough_mft_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    info->dwFlags =
        MFT_OUTPUT_STREAM_PROVIDES_SAMPLES |
        MFT_OUTPUT_STREAM_WHOLE_SAMPLES |
        MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE |
        MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER;

    info->cbAlignment = 0;
    info->cbSize = 0;
    return S_OK;
}

static HRESULT WINAPI passthrough_mft_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    static const GUID *types[] = { &MFMediaType_Video, &MFMediaType_Audio };
    HRESULT hr;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    if (index > ARRAY_SIZE(types) - 1)
        return MF_E_NO_MORE_TYPES;

    if (SUCCEEDED(hr = MFCreateMediaType(type)))
        hr = IMFMediaType_SetGUID(*type, &MF_MT_MAJOR_TYPE, types[index]);

    return hr;
}

static HRESULT WINAPI passthrough_mft_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&impl->cs);

    if (index)
    {
        hr = MF_E_NO_MORE_TYPES;
    }
    else if (impl->media_type_out)
    {
        *type = impl->media_type_out;
        IMFMediaType_AddRef(*type);
    }
    else if (impl->media_type_in)
    {
        *type = impl->media_type_in;
        IMFMediaType_AddRef(*type);
    }
    else
    {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI passthrough_mft_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&impl->cs);

    if (!(flags & MFT_SET_TYPE_TEST_ONLY))
    {
        if (impl->media_type_in)
            IMFMediaType_Release(impl->media_type_in);

        impl->media_type_in = type;
        IMFMediaType_AddRef(impl->media_type_in);
    }

    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI passthrough_mft_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&impl->cs);

    if (impl->media_type_out)
        IMFMediaType_Release(impl->media_type_out);

    impl->media_type_out = type;
    IMFMediaType_AddRef(impl->media_type_out);

    LeaveCriticalSection(&impl->cs);

    return S_OK;
}

static HRESULT WINAPI passthrough_mft_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&impl->cs);
    if (impl->media_type_in)
    {
        *type = impl->media_type_in;
        IMFMediaType_AddRef(*type);
    }
    else
    {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    }
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI passthrough_mft_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&impl->cs);

    if (impl->media_type_out)
    {
        *type = impl->media_type_out;
        IMFMediaType_AddRef(*type);
    }
    else
    {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI passthrough_mft_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI passthrough_mft_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    if (message == MFT_MESSAGE_COMMAND_FLUSH)
        return E_NOTIMPL;

    return S_OK;
}

static HRESULT WINAPI passthrough_mft_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&impl->cs);
    if (impl->sample)
    {
        hr = MF_E_NOTACCEPTING;
    }
    else
    {
        impl->sample = sample;
        IMFSample_AddRef(impl->sample);
    }

    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI passthrough_mft_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct passthrough_mft *impl = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;
    UINT32 val = 41;

    if (count != 1)
        return E_INVALIDARG;

    EnterCriticalSection(&impl->cs);

    if (impl->sample)
    {
        hr = IMFSample_GetUINT32(impl->sample, &IID_IMFSample, &val);

        if (impl->index > 0)
        {
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(val == impl->index, "Got unexpected value %u.\n", val);
        }
        else
        {
            ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
        }

        IMFSample_SetUINT32(impl->sample, &IID_IMFSample, impl->index + 1);

        samples->pSample = impl->sample;
        *status = samples[0].dwStatus = 0;
        impl->processing_count++;

        impl->sample = NULL;

        hr = S_OK;
    }
    else
    {
        hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    LeaveCriticalSection(&impl->cs);

    return hr;
}

static const IMFTransformVtbl passthrough_mft_vtbl =
{
    passthrough_mft_QueryInterface,
    passthrough_mft_AddRef,
    passthrough_mft_Release,
    passthrough_mft_GetStreamLimits,
    passthrough_mft_GetStreamCount,
    passthrough_mft_GetStreamIDs,
    passthrough_mft_GetInputStreamInfo,
    passthrough_mft_GetOutputStreamInfo,
    passthrough_mft_GetAttributes,
    passthrough_mft_GetInputStreamAttributes,
    passthrough_mft_GetOutputStreamAttributes,
    passthrough_mft_DeleteInputStream,
    passthrough_mft_AddInputStreams,
    passthrough_mft_GetInputAvailableType,
    passthrough_mft_GetOutputAvailableType,
    passthrough_mft_SetInputType,
    passthrough_mft_SetOutputType,
    passthrough_mft_GetInputCurrentType,
    passthrough_mft_GetOutputCurrentType,
    passthrough_mft_GetInputStatus,
    passthrough_mft_GetOutputStatus,
    passthrough_mft_SetOutputBounds,
    passthrough_mft_ProcessEvent,
    passthrough_mft_ProcessMessage,
    passthrough_mft_ProcessInput,
    passthrough_mft_ProcessOutput,
};

HRESULT passthrough_mft_create(UINT32 index, struct passthrough_mft **out)
{
    struct passthrough_mft *impl;

    *out = NULL;

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;

    impl->IMFTransform_iface.lpVtbl = &passthrough_mft_vtbl;
    impl->index = index;
    impl->refcount = 1;

    InitializeCriticalSection(&impl->cs);

    *out = impl;
    return S_OK;
}

static void test_effect(void)
{
    struct passthrough_mft *video_effect = NULL, *video_effect2 = NULL, *audio_effect = NULL, *audio_effect2 = NULL;
    IMFMediaEngineEx *media_engine = NULL;
    struct test_transfer_notify *notify;
    ID3D11Texture2D *texture = NULL;
    IMFDXGIDeviceManager *manager;
    ID3D11Device *device = NULL;
    D3D11_TEXTURE2D_DESC desc;
    IMFByteStream *stream;
    IMFMediaSink *sink;
    RECT dst_rect;
    UINT token;
    HRESULT hr;
    DWORD res;
    BSTR url;

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

    hr = IMFMediaEngineEx_RemoveAllEffects(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = passthrough_mft_create(0, &video_effect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = passthrough_mft_create(1, &video_effect2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)&video_effect->IMFTransform_iface, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&video_effect->IMFTransform_iface, 2);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)&video_effect2->IMFTransform_iface, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&video_effect2->IMFTransform_iface, 2);

    hr = IMFMediaEngineEx_RemoveAllEffects(media_engine);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&video_effect->IMFTransform_iface, 1);
    EXPECT_REF(&video_effect2->IMFTransform_iface, 1);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)&video_effect->IMFTransform_iface, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&video_effect->IMFTransform_iface, 2);

    hr = IMFMediaEngineEx_InsertVideoEffect(media_engine, (IUnknown *)&video_effect2->IMFTransform_iface, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&video_effect2->IMFTransform_iface, 2);

    hr = passthrough_mft_create(0, &audio_effect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = passthrough_mft_create(1, &audio_effect2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEngineEx_InsertAudioEffect(media_engine, (IUnknown *)&audio_effect->IMFTransform_iface, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&audio_effect->IMFTransform_iface, 2);

    hr = IMFMediaEngineEx_InsertAudioEffect(media_engine, (IUnknown *)&audio_effect2->IMFTransform_iface, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(&audio_effect2->IMFTransform_iface, 2);

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

    ok(video_effect->processing_count > 0, "Unexpected processing count %lu.\n", video_effect->processing_count);
    ok(video_effect2->processing_count > 0, "Unexpected processing count %lu.\n", video_effect2->processing_count);

    if (SUCCEEDED(hr = MFCreateAudioRenderer(NULL, &sink)))
    {
        ok(audio_effect->processing_count > 0, "Unexpected processing count %lu.\n", audio_effect->processing_count);
        ok(audio_effect2->processing_count > 0, "Unexpected processing count %lu.\n", audio_effect2->processing_count);

        IMFMediaSink_Release(sink);
    }
    else if (hr == MF_E_NO_AUDIO_PLAYBACK_DEVICE)
    {
        ok(!audio_effect->processing_count, "Unexpected processing count %lu.\n", audio_effect->processing_count);
        ok(!audio_effect2->processing_count, "Unexpected processing count %lu.\n", audio_effect2->processing_count);
    }

done:
    if (media_engine)
    {
        IMFMediaEngineEx_Shutdown(media_engine);

        hr = IMFMediaEngineEx_RemoveAllEffects(media_engine);
        ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

        IMFMediaEngineEx_Release(media_engine);
    }

    if (texture)
        ID3D11Texture2D_Release(texture);
    if (device)
        ID3D11Device_Release(device);

    if (audio_effect2)
        IMFTransform_Release(&audio_effect2->IMFTransform_iface);
    if (audio_effect)
        IMFTransform_Release(&audio_effect->IMFTransform_iface);

    if (video_effect2)
        IMFTransform_Release(&video_effect2->IMFTransform_iface);
    if (video_effect)
        IMFTransform_Release(&video_effect->IMFTransform_iface);

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
    ok(!!object->playing_event, "Failed to create an event, error %lu.\n", GetLastError());
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

    refcount = IMFMediaEngineEx_Release(media_engine);
    todo_wine
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
    test_effect();
    test_GetDuration();
    test_GetSeekable();
    test_media_extension();

    IMFMediaEngineClassFactory_Release(factory);

    CoUninitialize();

    MFShutdown();
}
