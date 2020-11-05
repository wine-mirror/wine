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

static IMFMediaEngineClassFactory *factory;

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

static IMFMediaEngine *create_media_engine(IMFMediaEngineNotify *callback)
{
    IMFDXGIDeviceManager *manager;
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    UINT token;
    HRESULT hr;

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Failed to create dxgi device manager, hr %#x.\n", hr);

    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "Failed to create attributes, hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)callback);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_UNKNOWN);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, 0, attributes, &media_engine);
    ok(hr == S_OK, "Failed to create media engine, hr %#x.\n", hr);

    IMFAttributes_Release(attributes);
    IMFDXGIDeviceManager_Release(manager);

    return media_engine;
}

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

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFMediaEngineClassFactory,
            (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* pre-win8 */, "Failed to create class factory, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        win_skip("Media Engine is not supported.\n");
        return;
    }

    /* Aggregation is not supported. */
    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER,
            &IID_IMFMediaEngineClassFactory, (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION, "Unexpected hr %#x.\n", hr);

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
}

static void test_CreateInstance(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *notify = &notify_impl.IMFMediaEngineNotify_iface;
    IMFDXGIDeviceManager *manager;
    IMFMediaEngine *media_engine;
    IMFAttributes *attributes;
    UINT token;
    HRESULT hr;

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Failed to create dxgi device manager, hr %#x.\n", hr);

    hr = MFCreateAttributes(&attributes, 3);
    ok(hr == S_OK, "Failed to create attributes, hr %#x.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
            attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_OPM_HWND, NULL);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE,
            attributes, &media_engine);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    IMFAttributes_DeleteAllItems(attributes);

    hr = IMFAttributes_SetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown *)notify);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFAttributes_SetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_UNKNOWN);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaEngineClassFactory_CreateInstance(factory, MF_MEDIA_ENGINE_WAITFORSTABLE_STATE, attributes, &media_engine);
    ok(hr == S_OK, "Failed to create media engine, hr %#x.\n", hr);

    IMFMediaEngine_Release(media_engine);
    IMFAttributes_Release(attributes);
    IMFDXGIDeviceManager_Release(manager);
}

static void test_Shutdown(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *callback = &notify_impl.IMFMediaEngineNotify_iface;
    IMFMediaTimeRange *time_range;
    IMFMediaEngine *media_engine;
    unsigned int state;
    DWORD cx, cy;
    double val;
    HRESULT hr;
    BSTR str;

    media_engine = create_media_engine(callback);

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == MF_E_SHUTDOWN || broken(hr == S_OK) /* before win10 */, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_SetSource(media_engine, NULL);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_GetCurrentSource(media_engine, &str);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_GetNetworkState(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    /* Preload mode is still accessible. */
    state = IMFMediaEngine_GetPreload(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    hr = IMFMediaEngine_SetPreload(media_engine, MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_GetPreload(media_engine);
    ok(state == MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC, "Unexpected state %d.\n", state);

    hr = IMFMediaEngine_SetPreload(media_engine, 100);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_GetPreload(media_engine);
    ok(state == 100, "Unexpected state %d.\n", state);

    hr = IMFMediaEngine_GetBuffered(media_engine, &time_range);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_Load(media_engine);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    str = SysAllocString(L"video/mp4");
    hr = IMFMediaEngine_CanPlayType(media_engine, str, &state);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);
    SysFreeString(str);

    state = IMFMediaEngine_GetReadyState(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    state = IMFMediaEngine_IsSeeking(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    val = IMFMediaEngine_GetCurrentTime(media_engine);
    ok(val == 0.0, "Unexpected time %f.\n", val);

    hr = IMFMediaEngine_SetCurrentTime(media_engine, 1.0);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    val = IMFMediaEngine_GetStartTime(media_engine);
    ok(val == 0.0, "Unexpected time %f.\n", val);

    state = IMFMediaEngine_IsPaused(media_engine);
    ok(!!state, "Unexpected state %d.\n", state);

    val = IMFMediaEngine_GetDefaultPlaybackRate(media_engine);
    ok(val == 1.0, "Unexpected rate %f.\n", val);

    hr = IMFMediaEngine_SetDefaultPlaybackRate(media_engine, 2.0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    val = IMFMediaEngine_GetPlaybackRate(media_engine);
    ok(val == 1.0, "Unexpected rate %f.\n", val);

    hr = IMFMediaEngine_GetPlayed(media_engine, &time_range);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_GetSeekable(media_engine, &time_range);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_IsEnded(media_engine);
    ok(!state, "Unexpected state %d.\n", state);

    /* Autoplay mode is still accessible. */
    state = IMFMediaEngine_GetAutoPlay(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_SetAutoPlay(media_engine, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_GetAutoPlay(media_engine);
    ok(!!state, "Unexpected state.\n");

    /* Loop mode is still accessible. */
    state = IMFMediaEngine_GetLoop(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_SetLoop(media_engine, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_GetLoop(media_engine);
    ok(!!state, "Unexpected state.\n");

    hr = IMFMediaEngine_Play(media_engine);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_Pause(media_engine);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_GetMuted(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_SetMuted(media_engine, TRUE);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    val = IMFMediaEngine_GetVolume(media_engine);
    ok(val == 1.0, "Unexpected value %f.\n", val);

    hr = IMFMediaEngine_SetVolume(media_engine, 2.0);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    state = IMFMediaEngine_HasVideo(media_engine);
    ok(!state, "Unexpected state.\n");

    state = IMFMediaEngine_HasAudio(media_engine);
    ok(!state, "Unexpected state.\n");

    hr = IMFMediaEngine_GetNativeVideoSize(media_engine, &cx, &cy);
todo_wine
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_GetVideoAspectRatio(media_engine, &cx, &cy);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaEngine_Release(media_engine);
}

static void test_Play(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *callback = &notify_impl.IMFMediaEngineNotify_iface;
    IMFMediaEngine *media_engine;
    LONGLONG pts;
    HRESULT hr;
    BOOL ret;

    media_engine = create_media_engine(callback);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(ret, "Unexpected state %d.\n", ret);

    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    pts = 0;
    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, &pts);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(pts == MINLONGLONG, "Unexpected timestamp.\n");

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!ret, "Unexpected state %d.\n", ret);

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_OnVideoStreamTick(media_engine, &pts);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!ret, "Unexpected state %d.\n", ret);

    IMFMediaEngine_Release(media_engine);

    /* Play -> Pause */
    media_engine = create_media_engine(callback);

    hr = IMFMediaEngine_Play(media_engine);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!ret, "Unexpected state %d.\n", ret);

    hr = IMFMediaEngine_Pause(media_engine);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ret = IMFMediaEngine_IsPaused(media_engine);
    ok(!!ret, "Unexpected state %d.\n", ret);

    IMFMediaEngine_Release(media_engine);
}

static void test_playback_rate(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *callback = &notify_impl.IMFMediaEngineNotify_iface;
    IMFMediaEngine *media_engine;
    double rate;
    HRESULT hr;

    media_engine = create_media_engine(callback);

    rate = IMFMediaEngine_GetDefaultPlaybackRate(media_engine);
    ok(rate == 1.0, "Unexpected default rate.\n");

    rate = IMFMediaEngine_GetPlaybackRate(media_engine);
    ok(rate == 1.0, "Unexpected default rate.\n");

    hr = IMFMediaEngine_SetPlaybackRate(media_engine, 0.0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    rate = IMFMediaEngine_GetPlaybackRate(media_engine);
    ok(rate == 0.0, "Unexpected default rate.\n");

    hr = IMFMediaEngine_SetDefaultPlaybackRate(media_engine, 0.0);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    IMFMediaEngine_Release(media_engine);
}

static void test_mute(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *callback = &notify_impl.IMFMediaEngineNotify_iface;
    IMFMediaEngine *media_engine;
    HRESULT hr;
    BOOL ret;

    media_engine = create_media_engine(callback);

    ret = IMFMediaEngine_GetMuted(media_engine);
    ok(!ret, "Unexpected state.\n");

    hr = IMFMediaEngine_SetMuted(media_engine, TRUE);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ret = IMFMediaEngine_GetMuted(media_engine);
    ok(ret, "Unexpected state.\n");

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    ret = IMFMediaEngine_GetMuted(media_engine);
    ok(ret, "Unexpected state.\n");

    IMFMediaEngine_Release(media_engine);
}

static void test_error(void)
{
    struct media_engine_notify notify_impl = {{&media_engine_notify_vtbl}, 1};
    IMFMediaEngineNotify *callback = &notify_impl.IMFMediaEngineNotify_iface;
    IMFMediaEngine *media_engine;
    IMFMediaError *eo, *eo2;
    unsigned int code;
    HRESULT hr;

    media_engine = create_media_engine(callback);

    eo = (void *)0xdeadbeef;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!eo, "Unexpected instance.\n");

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_ENCRYPTED + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_ABORTED);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    eo = NULL;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!eo, "Unexpected instance.\n");

    eo2 = NULL;
    hr = IMFMediaEngine_GetError(media_engine, &eo2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(eo2 != eo, "Unexpected instance.\n");

    IMFMediaError_Release(eo2);
    IMFMediaError_Release(eo);

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_NOERROR);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    eo = (void *)0xdeadbeef;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!eo, "Unexpected instance.\n");

    hr = IMFMediaEngine_Shutdown(media_engine);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    eo = (void *)0xdeadbeef;
    hr = IMFMediaEngine_GetError(media_engine, &eo);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);
    ok(!eo, "Unexpected instance.\n");

    hr = IMFMediaEngine_SetErrorCode(media_engine, MF_MEDIA_ENGINE_ERR_NOERROR);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaEngine_Release(media_engine);

    /* Error object. */
    hr = IMFMediaEngineClassFactory_CreateError(factory, &eo);
    ok(hr == S_OK, "Failed to create error object, hr %#x.\n", hr);

    code = IMFMediaError_GetErrorCode(eo);
    ok(code == MF_MEDIA_ENGINE_ERR_NOERROR, "Unexpected code %u.\n", code);

    hr = IMFMediaError_GetExtendedErrorCode(eo);
    ok(hr == S_OK, "Unexpected code %#x.\n", hr);

    hr = IMFMediaError_SetErrorCode(eo, MF_MEDIA_ENGINE_ERR_ENCRYPTED + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaError_SetErrorCode(eo, MF_MEDIA_ENGINE_ERR_ABORTED);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    code = IMFMediaError_GetErrorCode(eo);
    ok(code == MF_MEDIA_ENGINE_ERR_ABORTED, "Unexpected code %u.\n", code);

    hr = IMFMediaError_SetExtendedErrorCode(eo, E_FAIL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaError_GetExtendedErrorCode(eo);
    ok(hr == E_FAIL, "Unexpected code %#x.\n", hr);

    IMFMediaError_Release(eo);
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
    ok(hr == S_OK, "MFStartup failed: %#x.\n", hr);

    test_factory();
    test_CreateInstance();
    test_Shutdown();
    test_Play();
    test_playback_rate();
    test_mute();
    test_error();

    IMFMediaEngineClassFactory_Release(factory);

    CoUninitialize();

    MFShutdown();
}
