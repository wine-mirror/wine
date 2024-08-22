/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#include "mfsrcsnk_private.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "wine/winedmo.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct media_source
{
    IMFMediaSource IMFMediaSource_iface;
    LONG refcount;

    CRITICAL_SECTION cs;
    IMFMediaEventQueue *queue;
    IMFByteStream *stream;

    enum
    {
        SOURCE_STOPPED,
        SOURCE_SHUTDOWN,
    } state;
};

static struct media_source *media_source_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);

    TRACE("source %p, riid %s, out %p\n", source, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IMFMediaSource))
    {
        IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
        *out = &source->IMFMediaSource_iface;
        return S_OK;
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_source_AddRef(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    ULONG refcount = InterlockedIncrement(&source->refcount);
    TRACE("source %p, refcount %ld\n", source, refcount);
    return refcount;
}

static ULONG WINAPI media_source_Release(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    ULONG refcount = InterlockedDecrement(&source->refcount);

    TRACE("source %p, refcount %ld\n", source, refcount);

    if (!refcount)
    {
        IMFMediaSource_Shutdown(iface);

        IMFMediaEventQueue_Release(source->queue);
        IMFByteStream_Release(source->stream);

        source->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&source->cs);

        free(source);
    }

    return refcount;
}

static HRESULT WINAPI media_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, flags %#lx, event %p\n", source, flags, event);
    return IMFMediaEventQueue_GetEvent(source->queue, flags, event);
}

static HRESULT WINAPI media_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, callback %p, state %p\n", source, callback, state);
    return IMFMediaEventQueue_BeginGetEvent(source->queue, callback, state);
}

static HRESULT WINAPI media_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, result %p, event %p\n", source, result, event);
    return IMFMediaEventQueue_EndGetEvent(source->queue, result, event);
}

static HRESULT WINAPI media_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, event_type %#lx, ext_type %s, hr %#lx, value %p\n", source, event_type,
            debugstr_guid(ext_type), hr, debugstr_propvar(value));
    return IMFMediaEventQueue_QueueEventParamVar(source->queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_source_GetCharacteristics(IMFMediaSource *iface, DWORD *characteristics)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    HRESULT hr;

    TRACE("source %p, characteristics %p\n", source, characteristics);

    EnterCriticalSection(&source->cs);
    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        *characteristics = MFMEDIASOURCE_CAN_SEEK | MFMEDIASOURCE_CAN_PAUSE;
        hr = S_OK;
    }
    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **descriptor)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    FIXME("source %p, descriptor %p, stub!\n", source, descriptor);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *descriptor, const GUID *format,
        const PROPVARIANT *position)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    FIXME("source %p, descriptor %p, format %s, position %s, stub!\n", source, descriptor,
            debugstr_guid(format), debugstr_propvar(position));
    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_Stop(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    FIXME("source %p, stub!\n", source);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_Pause(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    FIXME("source %p, stub!\n", source);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_Shutdown(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);

    TRACE("source %p\n", source);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
    {
        LeaveCriticalSection(&source->cs);
        return MF_E_SHUTDOWN;
    }
    source->state = SOURCE_SHUTDOWN;

    IMFMediaEventQueue_Shutdown(source->queue);
    IMFByteStream_Close(source->stream);

    LeaveCriticalSection(&source->cs);

    return S_OK;
}

static const IMFMediaSourceVtbl media_source_vtbl =
{
    media_source_QueryInterface,
    media_source_AddRef,
    media_source_Release,
    media_source_GetEvent,
    media_source_BeginGetEvent,
    media_source_EndGetEvent,
    media_source_QueueEvent,
    media_source_GetCharacteristics,
    media_source_CreatePresentationDescriptor,
    media_source_Start,
    media_source_Stop,
    media_source_Pause,
    media_source_Shutdown,
};

static HRESULT media_source_create(const WCHAR *url, IMFByteStream *stream, IMFMediaSource **out)
{
    struct media_source *source;
    HRESULT hr;

    TRACE("url %s, stream %p, out %p\n", debugstr_w(url), stream, out);

    if (!(source = calloc(1, sizeof(*source))))
        return E_OUTOFMEMORY;
    source->IMFMediaSource_iface.lpVtbl = &media_source_vtbl;
    source->refcount = 1;

    if (FAILED(hr = MFCreateEventQueue(&source->queue)))
    {
        free(source);
        return hr;
    }

    IMFByteStream_AddRef((source->stream = stream));

    InitializeCriticalSectionEx(&source->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    source->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");

    *out = &source->IMFMediaSource_iface;
    TRACE("created source %p\n", source);
    return S_OK;
}

struct byte_stream_handler
{
    IMFByteStreamHandler IMFByteStreamHandler_iface;
    LONG refcount;
};

static struct byte_stream_handler *byte_stream_handler_from_IMFByteStreamHandler(IMFByteStreamHandler *iface)
{
    return CONTAINING_RECORD(iface, struct byte_stream_handler, IMFByteStreamHandler_iface);
}

static HRESULT WINAPI byte_stream_handler_QueryInterface(IMFByteStreamHandler *iface, REFIID riid, void **out)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);

    TRACE("handler %p, riid %s, out %p\n", handler, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IMFByteStreamHandler))
    {
        IMFByteStreamHandler_AddRef(&handler->IMFByteStreamHandler_iface);
        *out = &handler->IMFByteStreamHandler_iface;
        return S_OK;
    }

    WARN("Unsupported %s\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI byte_stream_handler_AddRef(IMFByteStreamHandler *iface)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);
    TRACE("handler %p, refcount %ld\n", handler, refcount);
    return refcount;
}

static ULONG WINAPI byte_stream_handler_Release(IMFByteStreamHandler *iface)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);
    TRACE("handler %p, refcount %ld\n", handler, refcount);
    if (!refcount)
        free(handler);
    return refcount;
}

static HRESULT WINAPI byte_stream_handler_BeginCreateObject(IMFByteStreamHandler *iface, IMFByteStream *stream, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    IMFAsyncResult *result;
    IMFMediaSource *source;
    HRESULT hr;
    DWORD caps;

    TRACE("handler %p, stream %p, url %s, flags %#lx, props %p, cookie %p, callback %p, state %p\n",
            handler, stream, debugstr_w(url), flags, props, cookie, callback, state);

    if (cookie)
        *cookie = NULL;
    if (!stream)
        return E_INVALIDARG;
    if (flags != MF_RESOLUTION_MEDIASOURCE)
        FIXME("Unimplemented flags %#lx\n", flags);

    if (FAILED(hr = IMFByteStream_GetCapabilities(stream, &caps)))
        return hr;
    if (!(caps & MFBYTESTREAM_IS_SEEKABLE))
    {
        FIXME("Non-seekable bytestreams not supported\n");
        return MF_E_BYTESTREAM_NOT_SEEKABLE;
    }

    if (FAILED(hr = media_source_create(url, stream, &source)))
        return hr;
    if (SUCCEEDED(hr = MFCreateAsyncResult((IUnknown *)source, callback, state, &result)))
    {
        hr = MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_IO, result);
        IMFAsyncResult_Release(result);
    }
    IMFMediaSource_Release(source);

    return hr;
}

static HRESULT WINAPI byte_stream_handler_EndCreateObject(IMFByteStreamHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *type, IUnknown **object)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    HRESULT hr;

    TRACE("handler %p, result %p, type %p, object %p\n", handler, result, type, object);

    *object = NULL;
    *type = MF_OBJECT_INVALID;

    if (SUCCEEDED(hr = IMFAsyncResult_GetStatus(result)))
    {
        hr = IMFAsyncResult_GetObject(result, object);
        *type = MF_OBJECT_MEDIASOURCE;
    }

    return hr;
}

static HRESULT WINAPI byte_stream_handler_CancelObjectCreation(IMFByteStreamHandler *iface, IUnknown *cookie)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    FIXME("handler %p, cookie %p, stub!\n", handler, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI byte_stream_handler_GetMaxNumberOfBytesRequiredForResolution(IMFByteStreamHandler *iface, QWORD *bytes)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    FIXME("handler %p, bytes %p, stub!\n", handler, bytes);
    return E_NOTIMPL;
}

static const IMFByteStreamHandlerVtbl byte_stream_handler_vtbl =
{
    byte_stream_handler_QueryInterface,
    byte_stream_handler_AddRef,
    byte_stream_handler_Release,
    byte_stream_handler_BeginCreateObject,
    byte_stream_handler_EndCreateObject,
    byte_stream_handler_CancelObjectCreation,
    byte_stream_handler_GetMaxNumberOfBytesRequiredForResolution,
};

static HRESULT byte_stream_plugin_create(IUnknown *outer, REFIID riid, void **out)
{
    struct byte_stream_handler *handler;
    HRESULT hr;

    TRACE("outer %p, riid %s, out %p\n", outer, debugstr_guid(riid), out);

    if (outer)
        return CLASS_E_NOAGGREGATION;
    if (!(handler = calloc(1, sizeof(*handler))))
        return E_OUTOFMEMORY;
    handler->IMFByteStreamHandler_iface.lpVtbl = &byte_stream_handler_vtbl;
    handler->refcount = 1;
    TRACE("created %p\n", handler);

    hr = IMFByteStreamHandler_QueryInterface(&handler->IMFByteStreamHandler_iface, riid, out);
    IMFByteStreamHandler_Release(&handler->IMFByteStreamHandler_iface);
    return hr;
}

static BOOL use_gst_byte_stream_handler(void)
{
    BOOL result;
    DWORD size = sizeof(result);

    /* @@ Wine registry key: HKCU\Software\Wine\MediaFoundation */
    if (!RegGetValueW( HKEY_CURRENT_USER, L"Software\\Wine\\MediaFoundation", L"DisableGstByteStreamHandler",
                       RRF_RT_REG_DWORD, NULL, &result, &size ))
        return !result;

    return TRUE;
}

static HRESULT WINAPI asf_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/x-ms-asf")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl asf_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    asf_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory asf_byte_stream_plugin_factory = {&asf_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI avi_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/avi")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl avi_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    avi_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory avi_byte_stream_plugin_factory = {&avi_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI mpeg4_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/mp4")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl mpeg4_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mpeg4_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory mpeg4_byte_stream_plugin_factory = {&mpeg4_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI wav_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("audio/wav")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl wav_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    wav_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory wav_byte_stream_plugin_factory = {&wav_byte_stream_plugin_factory_vtbl};
