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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "mfmediaengine.h"
#include "mferror.h"
#include "dxgi.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

enum media_engine_mode
{
    MEDIA_ENGINE_INVALID,
    MEDIA_ENGINE_AUDIO_MODE,
    MEDIA_ENGINE_RENDERING_MODE,
    MEDIA_ENGINE_FRAME_SERVER_MODE,
};

struct media_engine
{
    IMFMediaEngine IMFMediaEngine_iface;
    LONG refcount;
    DWORD flags;
    IMFMediaEngineNotify *callback;
    UINT64 playback_hwnd;
    DXGI_FORMAT output_format;
    IMFDXGIDeviceManager *dxgi_manager;
    enum media_engine_mode mode;
};

static inline struct media_engine *impl_from_IMFMediaEngine(IMFMediaEngine *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, IMFMediaEngine_iface);
}

static HRESULT WINAPI media_engine_QueryInterface(IMFMediaEngine *iface, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaEngine) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngine_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_AddRef(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    ULONG refcount = InterlockedIncrement(&engine->refcount);

    TRACE("(%p) ref=%u.\n", iface, refcount);

    return refcount;
}

static void free_media_engine(struct media_engine *engine)
{
    if (engine->callback)
        IMFMediaEngineNotify_Release(engine->callback);
    if (engine->dxgi_manager)
        IMFDXGIDeviceManager_Release(engine->dxgi_manager);
    heap_free(engine);
}

static ULONG WINAPI media_engine_Release(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    ULONG refcount = InterlockedDecrement(&engine->refcount);

    TRACE("(%p) ref=%u.\n", iface, refcount);

    if (!refcount)
        free_media_engine(engine);

    return refcount;
}

static HRESULT WINAPI media_engine_GetError(IMFMediaEngine *iface, IMFMediaError **error)
{
    FIXME("(%p, %p): stub.\n", iface, error);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetErrorCode(IMFMediaEngine *iface, MF_MEDIA_ENGINE_ERR error)
{
    FIXME("(%p, %d): stub.\n", iface, error);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetSourceElements(IMFMediaEngine *iface, IMFMediaEngineSrcElements *elements)
{
    FIXME("(%p, %p): stub.\n", iface, elements);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetSource(IMFMediaEngine *iface, BSTR url)
{
    FIXME("(%p, %s): stub.\n", iface, debugstr_w(url));

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetCurrentSource(IMFMediaEngine *iface, BSTR *url)
{
    FIXME("(%p, %p): stub.\n", iface, url);

    return E_NOTIMPL;
}

static USHORT WINAPI media_engine_GetNetworkState(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0;
}

static MF_MEDIA_ENGINE_PRELOAD WINAPI media_engine_GetPreload(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return MF_MEDIA_ENGINE_PRELOAD_NONE;
}

static HRESULT WINAPI media_engine_SetPreload(IMFMediaEngine *iface, MF_MEDIA_ENGINE_PRELOAD preload)
{
    FIXME("(%p, %d): stub.\n", iface, preload);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetBuffered(IMFMediaEngine *iface, IMFMediaTimeRange **buffered)
{
    FIXME("(%p, %p): stub.\n", iface, buffered);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_Load(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_CanPlayType(IMFMediaEngine *iface, BSTR type, MF_MEDIA_ENGINE_CANPLAY *answer)
{
    FIXME("(%p, %s, %p): stub.\n", iface, debugstr_w(type), answer);

    return E_NOTIMPL;
}

static USHORT WINAPI media_engine_GetReadyState(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0;
}

static BOOL WINAPI media_engine_IsSeeking(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static double WINAPI media_engine_GetCurrentTime(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static HRESULT WINAPI media_engine_SetCurrentTime(IMFMediaEngine *iface, double time)
{
    FIXME("(%p, %f): stub.\n", iface, time);

    return E_NOTIMPL;
}

static double WINAPI media_engine_GetStartTime(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static double WINAPI media_engine_GetDuration(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static BOOL WINAPI media_engine_IsPaused(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static double WINAPI media_engine_GetDefaultPlaybackRate(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static HRESULT WINAPI media_engine_SetDefaultPlaybackRate(IMFMediaEngine *iface, double rate)
{
    FIXME("(%p, %f): stub.\n", iface, rate);

    return E_NOTIMPL;
}

static double WINAPI media_engine_GetPlaybackRate(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static HRESULT WINAPI media_engine_SetPlaybackRate(IMFMediaEngine *iface, double rate)
{
    FIXME("(%p, %f): stub.\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetPlayed(IMFMediaEngine *iface, IMFMediaTimeRange **played)
{
    FIXME("(%p, %p): stub.\n", iface, played);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetSeekable(IMFMediaEngine *iface, IMFMediaTimeRange **seekable)
{
    FIXME("(%p, %p): stub.\n", iface, seekable);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_IsEnded(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static BOOL WINAPI media_engine_GetAutoPlay(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static HRESULT WINAPI media_engine_SetAutoPlay(IMFMediaEngine *iface, BOOL autoplay)
{
    FIXME("(%p, %d): stub.\n", iface, autoplay);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_GetLoop(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static HRESULT WINAPI media_engine_SetLoop(IMFMediaEngine *iface, BOOL loop)
{
    FIXME("(%p, %d): stub.\n", iface, loop);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_Play(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_Pause(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_GetMuted(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static HRESULT WINAPI media_engine_SetMuted(IMFMediaEngine *iface, BOOL muted)
{
    FIXME("(%p, %d): stub.\n", iface, muted);

    return E_NOTIMPL;
}

static double WINAPI media_engine_GetVolume(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static HRESULT WINAPI media_engine_SetVolume(IMFMediaEngine *iface, double volume)
{
    FIXME("(%p, %f): stub.\n", iface, volume);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_HasVideo(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static BOOL WINAPI media_engine_HasAudio(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static HRESULT WINAPI media_engine_GetNativeVideoSize(IMFMediaEngine *iface, DWORD *cx, DWORD *cy)
{
    FIXME("(%p, %p, %p): stub.\n", iface, cx, cy);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetVideoAspectRatio(IMFMediaEngine *iface, DWORD *cx, DWORD *cy)
{
    FIXME("(%p, %p, %p): stub.\n", iface, cx, cy);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_Shutdown(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_TransferVideoFrame(IMFMediaEngine *iface, IUnknown *surface,
                                                      const MFVideoNormalizedRect *src,
                                                      const RECT *dst, const MFARGB *color)
{
    FIXME("(%p, %p, %p, %p, %p): stub.\n", iface, surface, src, dst, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_OnVideoStreamTick(IMFMediaEngine *iface, LONGLONG *time)
{
    FIXME("(%p, %p): stub.\n", iface, time);

    return E_NOTIMPL;
}

static const IMFMediaEngineVtbl media_engine_vtbl =
{
    media_engine_QueryInterface,
    media_engine_AddRef,
    media_engine_Release,
    media_engine_GetError,
    media_engine_SetErrorCode,
    media_engine_SetSourceElements,
    media_engine_SetSource,
    media_engine_GetCurrentSource,
    media_engine_GetNetworkState,
    media_engine_GetPreload,
    media_engine_SetPreload,
    media_engine_GetBuffered,
    media_engine_Load,
    media_engine_CanPlayType,
    media_engine_GetReadyState,
    media_engine_IsSeeking,
    media_engine_GetCurrentTime,
    media_engine_SetCurrentTime,
    media_engine_GetStartTime,
    media_engine_GetDuration,
    media_engine_IsPaused,
    media_engine_GetDefaultPlaybackRate,
    media_engine_SetDefaultPlaybackRate,
    media_engine_GetPlaybackRate,
    media_engine_SetPlaybackRate,
    media_engine_GetPlayed,
    media_engine_GetSeekable,
    media_engine_IsEnded,
    media_engine_GetAutoPlay,
    media_engine_SetAutoPlay,
    media_engine_GetLoop,
    media_engine_SetLoop,
    media_engine_Play,
    media_engine_Pause,
    media_engine_GetMuted,
    media_engine_SetMuted,
    media_engine_GetVolume,
    media_engine_SetVolume,
    media_engine_HasVideo,
    media_engine_HasAudio,
    media_engine_GetNativeVideoSize,
    media_engine_GetVideoAspectRatio,
    media_engine_Shutdown,
    media_engine_TransferVideoFrame,
    media_engine_OnVideoStreamTick,
};

static HRESULT WINAPI media_engine_factory_QueryInterface(IMFMediaEngineClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaEngineClassFactory) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngineClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_factory_AddRef(IMFMediaEngineClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI media_engine_factory_Release(IMFMediaEngineClassFactory *iface)
{
    return 1;
}

static HRESULT init_media_engine(IMFAttributes *attributes, struct media_engine *engine)
{
    HRESULT hr;

    hr = IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, &IID_IMFMediaEngineNotify,
                                  (void **)&engine->callback);
    if (FAILED(hr))
        return MF_E_ATTRIBUTENOTFOUND;

    IMFAttributes_GetUINT64(attributes, &MF_MEDIA_ENGINE_PLAYBACK_HWND, &engine->playback_hwnd);
    IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_DXGI_MANAGER, &IID_IMFDXGIDeviceManager,
                             (void **)&engine->dxgi_manager);
    hr = IMFAttributes_GetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, &engine->output_format);
    if (engine->playback_hwnd) /* FIXME: handle MF_MEDIA_ENGINE_PLAYBACK_VISUAL */
        engine->mode = MEDIA_ENGINE_RENDERING_MODE;
    else
    {
        if (SUCCEEDED(hr))
            engine->mode = MEDIA_ENGINE_FRAME_SERVER_MODE;
        else
            engine->mode = MEDIA_ENGINE_AUDIO_MODE;
    }

    return S_OK;
}

static HRESULT WINAPI media_engine_factory_CreateInstance(IMFMediaEngineClassFactory *iface, DWORD flags,
                                                          IMFAttributes *attributes, IMFMediaEngine **engine)
{
    struct media_engine *object;
    HRESULT hr;

    TRACE("(%p, %#x, %p, %p).\n", iface, flags, attributes, engine);

    if (!attributes || !engine)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = init_media_engine(attributes, object);
    if (FAILED(hr))
    {
        free_media_engine(object);
        return hr;
    }

    object->IMFMediaEngine_iface.lpVtbl = &media_engine_vtbl;
    object->refcount = 1;
    object->flags = flags;

    *engine = &object->IMFMediaEngine_iface;

    return S_OK;
}

static HRESULT WINAPI media_engine_factory_CreateTimeRange(IMFMediaEngineClassFactory *iface,
                                                           IMFMediaTimeRange **range)
{
    FIXME("(%p, %p): stub.\n", iface, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_factory_CreateError(IMFMediaEngineClassFactory *iface, IMFMediaError **error)
{
    FIXME("(%p, %p): stub.\n", iface, error);

    return E_NOTIMPL;
}

static const IMFMediaEngineClassFactoryVtbl media_engine_factory_vtbl =
{
    media_engine_factory_QueryInterface,
    media_engine_factory_AddRef,
    media_engine_factory_Release,
    media_engine_factory_CreateInstance,
    media_engine_factory_CreateTimeRange,
    media_engine_factory_CreateError,
};

static IMFMediaEngineClassFactory media_engine_factory = { &media_engine_factory_vtbl };

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("(%s, %p).\n", debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("interface %s not implemented.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p).\n", outer, debugstr_guid(riid), obj);

    *obj = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFMediaEngineClassFactory_QueryInterface(&media_engine_factory, riid, obj);
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%d): stub.\n", dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl class_factory_vtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer,
};

static IClassFactory classfactory = { &class_factory_vtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj)
{
    TRACE("(%s, %s, %p).\n", debugstr_guid(clsid), debugstr_guid(riid), obj);

    if (IsEqualGUID(clsid, &CLSID_MFMediaEngineClassFactory))
        return IClassFactory_QueryInterface(&classfactory, riid, obj);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    *obj = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
