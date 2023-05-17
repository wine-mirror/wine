/* Gstreamer Media Sink
 *
 * Copyright 2023 Ziqing Hui for CodeWeavers
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

#include "gst_private.h"
#include "mferror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct media_sink
{
    IMFFinalizableMediaSink IMFFinalizableMediaSink_iface;
    LONG refcount;
    CRITICAL_SECTION cs;
    bool shutdown;

    IMFByteStream *bytestream;
};

static struct media_sink *impl_from_IMFFinalizableMediaSink(IMFFinalizableMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct media_sink, IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI media_sink_QueryInterface(IMFFinalizableMediaSink *iface, REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFFinalizableMediaSink) ||
            IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI media_sink_AddRef(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&media_sink->refcount);
    TRACE("iface %p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI media_sink_Release(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&media_sink->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFFinalizableMediaSink_Shutdown(iface);
        IMFByteStream_Release(media_sink->bytestream);
        media_sink->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&media_sink->cs);
        free(media_sink);
    }

    return refcount;
}

static HRESULT WINAPI media_sink_GetCharacteristics(IMFFinalizableMediaSink *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_AddStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    FIXME("iface %p, stream_sink_id %#lx, media_type %p, stream_sink %p stub!\n",
            iface, stream_sink_id, media_type, stream_sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_RemoveStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id)
{
    FIXME("iface %p, stream_sink_id %#lx stub!\n", iface, stream_sink_id);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkCount(IMFFinalizableMediaSink *iface, DWORD *count)
{
    FIXME("iface %p, count %p stub!\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkByIndex(IMFFinalizableMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    FIXME("iface %p, index %lu, stream %p stub!\n", iface, index, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkById(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    FIXME("iface %p, stream_sink_id %#lx, stream %p stub!\n", iface, stream_sink_id, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_SetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock *clock)
{
    FIXME("iface %p, clock %p stub!\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock **clock)
{
    FIXME("iface %p, clock %p stub!\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_Shutdown(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&media_sink->cs);

    if (media_sink->shutdown)
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_SHUTDOWN;
    }

    IMFByteStream_Close(media_sink->bytestream);

    media_sink->shutdown = TRUE;

    LeaveCriticalSection(&media_sink->cs);

    return S_OK;
}

static HRESULT WINAPI media_sink_BeginFinalize(IMFFinalizableMediaSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    FIXME("iface %p, callback %p, state %p stub!\n", iface, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_EndFinalize(IMFFinalizableMediaSink *iface, IMFAsyncResult *result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);

    return E_NOTIMPL;
}

static const IMFFinalizableMediaSinkVtbl media_sink_vtbl =
{
    media_sink_QueryInterface,
    media_sink_AddRef,
    media_sink_Release,
    media_sink_GetCharacteristics,
    media_sink_AddStreamSink,
    media_sink_RemoveStreamSink,
    media_sink_GetStreamSinkCount,
    media_sink_GetStreamSinkByIndex,
    media_sink_GetStreamSinkById,
    media_sink_SetPresentationClock,
    media_sink_GetPresentationClock,
    media_sink_Shutdown,
    media_sink_BeginFinalize,
    media_sink_EndFinalize,
};

static HRESULT media_sink_create(IMFByteStream *bytestream, struct media_sink **out)
{
    struct media_sink *media_sink;

    TRACE("bytestream %p, out %p.\n", bytestream, out);

    if (!bytestream)
        return E_POINTER;

    if (!(media_sink = calloc(1, sizeof(*media_sink))))
        return E_OUTOFMEMORY;

    media_sink->IMFFinalizableMediaSink_iface.lpVtbl = &media_sink_vtbl;
    media_sink->refcount = 1;
    InitializeCriticalSection(&media_sink->cs);
    media_sink->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");
    IMFByteStream_AddRef((media_sink->bytestream = bytestream));

    *out = media_sink;
    TRACE("Created media sink %p.\n", media_sink);

    return S_OK;
}

static HRESULT WINAPI sink_class_factory_QueryInterface(IMFSinkClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFSinkClassFactory)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSinkClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_class_factory_AddRef(IMFSinkClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI sink_class_factory_Release(IMFSinkClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI sink_class_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    struct media_sink *media_sink;
    HRESULT hr;

    TRACE("iface %p, bytestream %p, video_type %p, audio_type %p, out %p.\n",
            iface, bytestream, video_type, audio_type, out);

    if (FAILED(hr = media_sink_create(bytestream, &media_sink)))
        return hr;

    *out = (IMFMediaSink *)&media_sink->IMFFinalizableMediaSink_iface;
    return S_OK;
}

static const IMFSinkClassFactoryVtbl sink_class_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    sink_class_factory_CreateMediaSink,
};

static IMFSinkClassFactory sink_class_factory = { &sink_class_factory_vtbl };

HRESULT sink_class_factory_create(IUnknown *outer, IUnknown **out)
{
    *out = (IUnknown *)&sink_class_factory;
    return S_OK;
}
