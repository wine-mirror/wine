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

#include <math.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "mfapi.h"
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

/* Used with create flags. */
enum media_engine_flags
{
    /* MF_MEDIA_ENGINE_CREATEFLAGS_MASK is 0x1f. */
    FLAGS_ENGINE_SHUT_DOWN = 0x20,
    FLAGS_ENGINE_AUTO_PLAY = 0x40,
    FLAGS_ENGINE_LOOP = 0x80,
    FLAGS_ENGINE_PAUSED = 0x100,
    FLAGS_ENGINE_WAITING = 0x200,
    FLAGS_ENGINE_MUTED = 0x400,
    FLAGS_ENGINE_HAS_AUDIO = 0x800,
    FLAGS_ENGINE_HAS_VIDEO = 0x1000,
    FLAGS_ENGINE_FIRST_FRAME = 0x2000,
};

struct video_frame
{
    LONGLONG pts;
    SIZE size;
    SIZE ratio;
    TOPOID node_id;
};

struct media_engine
{
    IMFMediaEngine IMFMediaEngine_iface;
    IMFAsyncCallback session_events;
    IMFAsyncCallback load_handler;
    IMFSampleGrabberSinkCallback grabber_callback;
    LONG refcount;
    IMFMediaEngineNotify *callback;
    IMFAttributes *attributes;
    enum media_engine_mode mode;
    unsigned int flags;
    double playback_rate;
    double default_playback_rate;
    double volume;
    double duration;
    MF_MEDIA_ENGINE_ERR error_code;
    HRESULT extended_code;
    MF_MEDIA_ENGINE_READY ready_state;
    MF_MEDIA_ENGINE_PRELOAD preload;
    IMFMediaSession *session;
    IMFSourceResolver *resolver;
    BSTR current_source;
    struct video_frame video_frame;
    CRITICAL_SECTION cs;
};

struct media_error
{
    IMFMediaError IMFMediaError_iface;
    LONG refcount;
    unsigned int code;
    HRESULT extended_code;
};

static struct media_error *impl_from_IMFMediaError(IMFMediaError *iface)
{
    return CONTAINING_RECORD(iface, struct media_error, IMFMediaError_iface);
}

static HRESULT WINAPI media_error_QueryInterface(IMFMediaError *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaError) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaError_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_error_AddRef(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    ULONG refcount = InterlockedIncrement(&me->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_error_Release(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    ULONG refcount = InterlockedDecrement(&me->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        heap_free(me);

    return refcount;
}

static USHORT WINAPI media_error_GetErrorCode(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    TRACE("%p.\n", iface);
    return me->code;
}

static HRESULT WINAPI media_error_GetExtendedErrorCode(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    TRACE("%p.\n", iface);
    return me->extended_code;
}

static HRESULT WINAPI media_error_SetErrorCode(IMFMediaError *iface, MF_MEDIA_ENGINE_ERR code)
{
    struct media_error *me = impl_from_IMFMediaError(iface);

    TRACE("%p, %u.\n", iface, code);

    if ((unsigned int)code > MF_MEDIA_ENGINE_ERR_ENCRYPTED)
        return E_INVALIDARG;

    me->code = code;

    return S_OK;
}

static HRESULT WINAPI media_error_SetExtendedErrorCode(IMFMediaError *iface, HRESULT code)
{
    struct media_error *me = impl_from_IMFMediaError(iface);

    TRACE("%p, %#x.\n", iface, code);

    me->extended_code = code;

    return S_OK;
}

static const IMFMediaErrorVtbl media_error_vtbl =
{
    media_error_QueryInterface,
    media_error_AddRef,
    media_error_Release,
    media_error_GetErrorCode,
    media_error_GetExtendedErrorCode,
    media_error_SetErrorCode,
    media_error_SetExtendedErrorCode,
};

static HRESULT create_media_error(IMFMediaError **ret)
{
    struct media_error *object;

    *ret = NULL;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaError_iface.lpVtbl = &media_error_vtbl;
    object->refcount = 1;

    *ret = &object->IMFMediaError_iface;

    return S_OK;
}

static void media_engine_set_flag(struct media_engine *engine, unsigned int mask, BOOL value)
{
    if (value)
        engine->flags |= mask;
    else
        engine->flags &= ~mask;
}

static inline struct media_engine *impl_from_IMFMediaEngine(IMFMediaEngine *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, IMFMediaEngine_iface);
}

static struct media_engine *impl_from_session_events_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, session_events);
}

static struct media_engine *impl_from_load_handler_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, load_handler);
}

static struct media_engine *impl_from_IMFSampleGrabberSinkCallback(IMFSampleGrabberSinkCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, grabber_callback);
}

static unsigned int get_gcd(unsigned int a, unsigned int b)
{
    unsigned int m;

    while (b)
    {
        m = a % b;
        a = b;
        b = m;
    }

    return a;
}

static void media_engine_get_frame_size(struct media_engine *engine, IMFTopology *topology)
{
    IMFMediaTypeHandler *handler;
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFTopologyNode *node;
    unsigned int gcd;
    UINT64 size;
    HRESULT hr;

    engine->video_frame.size.cx = 0;
    engine->video_frame.size.cy = 0;
    engine->video_frame.ratio.cx = 1;
    engine->video_frame.ratio.cy = 1;

    if (FAILED(IMFTopology_GetNodeByID(topology, engine->video_frame.node_id, &node)))
        return;

    hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR,
            &IID_IMFStreamDescriptor, (void **)&sd);
    IMFTopologyNode_Release(node);
    if (FAILED(hr))
        return;

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    IMFStreamDescriptor_Release(sd);
    if (FAILED(hr))
        return;

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    IMFMediaTypeHandler_Release(handler);
    if (FAILED(hr))
    {
        WARN("Failed to get current media type %#x.\n", hr);
        return;
    }

    IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &size);

    engine->video_frame.size.cx = size >> 32;
    engine->video_frame.size.cy = size;

    if ((gcd = get_gcd(engine->video_frame.size.cx, engine->video_frame.size.cy)))
    {
        engine->video_frame.ratio.cx = engine->video_frame.size.cx / gcd;
        engine->video_frame.ratio.cy = engine->video_frame.size.cy / gcd;
    }

    IMFMediaType_Release(media_type);
}

static HRESULT WINAPI media_engine_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_session_events_AddRef(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFMediaEngine_AddRef(&engine->IMFMediaEngine_iface);
}

static ULONG WINAPI media_engine_session_events_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFMediaEngine_Release(&engine->IMFMediaEngine_iface);
}

static HRESULT WINAPI media_engine_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_session_events_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    IMFMediaEvent *event = NULL;
    MediaEventType event_type;
    HRESULT hr;

    if (FAILED(hr = IMFMediaSession_EndGetEvent(engine->session, result, &event)))
    {
        WARN("Failed to get session event, hr %#x.\n", hr);
        goto failed;
    }

    if (FAILED(hr = IMFMediaEvent_GetType(event, &event_type)))
    {
        WARN("Failed to get event type, hr %#x.\n", hr);
        goto failed;
    }

    switch (event_type)
    {
        case MEBufferingStarted:
        case MEBufferingStopped:

            IMFMediaEngineNotify_EventNotify(engine->callback, event_type == MEBufferingStarted ?
                    MF_MEDIA_ENGINE_EVENT_BUFFERINGSTARTED : MF_MEDIA_ENGINE_EVENT_BUFFERINGENDED, 0, 0);
            break;
        case MESessionTopologyStatus:
        {
            UINT32 topo_status = 0;
            IMFTopology *topology;
            PROPVARIANT value;

            IMFMediaEvent_GetUINT32(event, &MF_EVENT_TOPOLOGY_STATUS, &topo_status);
            if (topo_status != MF_TOPOSTATUS_READY)
                break;

            value.vt = VT_EMPTY;
            if (FAILED(IMFMediaEvent_GetValue(event, &value)))
                break;

            if (value.vt != VT_UNKNOWN)
            {
                PropVariantClear(&value);
                break;
            }

            topology = (IMFTopology *)value.punkVal;

            EnterCriticalSection(&engine->cs);

            engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_METADATA;

            media_engine_get_frame_size(engine, topology);

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_DURATIONCHANGE, 0, 0);
            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA, 0, 0);

            engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_ENOUGH_DATA;

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADEDDATA, 0, 0);
            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_CANPLAY, 0, 0);

            LeaveCriticalSection(&engine->cs);

            PropVariantClear(&value);

            break;
        }
        case MESessionStarted:

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PLAYING, 0, 0);
            break;
        case MESessionEnded:

            EnterCriticalSection(&engine->cs);
            media_engine_set_flag(engine, FLAGS_ENGINE_FIRST_FRAME, FALSE);
            engine->video_frame.pts = MINLONGLONG;
            LeaveCriticalSection(&engine->cs);

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_ENDED, 0, 0);
            break;
    }

failed:

    if (event)
        IMFMediaEvent_Release(event);

    if (FAILED(hr = IMFMediaSession_BeginGetEvent(engine->session, iface, NULL)))
        WARN("Failed to subscribe to session events, hr %#x.\n", hr);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_engine_session_events_vtbl =
{
    media_engine_callback_QueryInterface,
    media_engine_session_events_AddRef,
    media_engine_session_events_Release,
    media_engine_callback_GetParameters,
    media_engine_session_events_Invoke,
};

static ULONG WINAPI media_engine_load_handler_AddRef(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    return IMFMediaEngine_AddRef(&engine->IMFMediaEngine_iface);
}

static ULONG WINAPI media_engine_load_handler_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    return IMFMediaEngine_Release(&engine->IMFMediaEngine_iface);
}

static HRESULT media_engine_create_source_node(IMFMediaSource *source, IMFPresentationDescriptor *pd, IMFStreamDescriptor *sd,
        IMFTopologyNode **node)
{
    HRESULT hr;

    if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, node)))
        return hr;

    IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
    IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);

    return S_OK;
}

static HRESULT media_engine_create_audio_renderer(struct media_engine *engine, IMFTopologyNode **node)
{
    unsigned int category, role;
    IMFActivate *sar_activate;
    HRESULT hr;

    *node = NULL;

    if (FAILED(hr = MFCreateAudioRendererActivate(&sar_activate)))
        return hr;

    /* Configuration attributes keys differ between Engine and SAR. */
    if (SUCCEEDED(IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_CATEGORY, &category)))
        IMFActivate_SetUINT32(sar_activate, &MF_AUDIO_RENDERER_ATTRIBUTE_STREAM_CATEGORY, category);
    if (SUCCEEDED(IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE, &role)))
        IMFActivate_SetUINT32(sar_activate, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, role);

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, node)))
    {
        IMFTopologyNode_SetObject(*node, (IUnknown *)sar_activate);
        IMFTopologyNode_SetUINT32(*node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    }

    IMFActivate_Release(sar_activate);

    return hr;
}

static HRESULT media_engine_create_video_renderer(struct media_engine *engine, IMFTopologyNode **node)
{
    DXGI_FORMAT output_format;
    IMFMediaType *media_type;
    IMFActivate *activate;
    GUID subtype;
    HRESULT hr;

    *node = NULL;

    if (FAILED(IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, &output_format)))
    {
        WARN("Output format was not specified.\n");
        return E_FAIL;
    }

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    if (!(subtype.Data1 = MFMapDXGIFormatToDX9Format(output_format)))
    {
        WARN("Unrecognized output format %#x.\n", output_format);
        return E_FAIL;
    }

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &subtype);

    hr = MFCreateSampleGrabberSinkActivate(media_type, &engine->grabber_callback, &activate);
    IMFMediaType_Release(media_type);
    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, node)))
    {
        IMFTopologyNode_SetObject(*node, (IUnknown *)activate);
        IMFTopologyNode_SetUINT32(*node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    }

    IMFActivate_Release(activate);

    return hr;
}

static HRESULT media_engine_create_topology(struct media_engine *engine, IMFMediaSource *source)
{
    IMFStreamDescriptor *sd_audio = NULL, *sd_video = NULL;
    unsigned int stream_count = 0, i;
    IMFPresentationDescriptor *pd;
    IMFTopology *topology;
    UINT64 duration;
    HRESULT hr;

    memset(&engine->video_frame, 0, sizeof(engine->video_frame));

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd)))
        return hr;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, &stream_count)))
        WARN("Failed to get stream count, hr %#x.\n", hr);

    /* Enable first video stream and first audio stream. */

    for (i = 0; i < stream_count; ++i)
    {
        IMFMediaTypeHandler *type_handler;
        IMFStreamDescriptor *sd;
        BOOL selected;

        IMFPresentationDescriptor_DeselectStream(pd, i);

        if (sd_audio && sd_video)
            continue;

        IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, i, &selected, &sd);

        if (SUCCEEDED(IMFStreamDescriptor_GetMediaTypeHandler(sd, &type_handler)))
        {
            GUID major = { 0 };

            IMFMediaTypeHandler_GetMajorType(type_handler, &major);

            if (IsEqualGUID(&major, &MFMediaType_Audio) && !sd_audio)
            {
                sd_audio = sd;
                IMFStreamDescriptor_AddRef(sd_audio);
                IMFPresentationDescriptor_SelectStream(pd, i);
            }
            else if (IsEqualGUID(&major, &MFMediaType_Video) && !sd_video && !(engine->flags & MF_MEDIA_ENGINE_AUDIOONLY))
            {
                sd_video = sd;
                IMFStreamDescriptor_AddRef(sd_video);
                IMFPresentationDescriptor_SelectStream(pd, i);
            }

            IMFMediaTypeHandler_Release(type_handler);
        }
    }

    if (!sd_video && !sd_audio)
    {
        IMFPresentationDescriptor_Release(pd);
        return E_UNEXPECTED;
    }

    if (sd_video)
        engine->flags |= FLAGS_ENGINE_HAS_VIDEO;
    if (sd_audio)
        engine->flags |= FLAGS_ENGINE_HAS_AUDIO;

    /* Assume live source if duration was not provided. */
    if (SUCCEEDED(IMFPresentationDescriptor_GetUINT64(pd, &MF_PD_DURATION, &duration)))
    {
        /* Convert 100ns to seconds. */
        engine->duration = duration / 10000000;
    }
    else
        engine->duration = INFINITY;

    if (SUCCEEDED(hr = MFCreateTopology(&topology)))
    {
        IMFTopologyNode *sar_node = NULL, *audio_src = NULL;
        IMFTopologyNode *grabber_node = NULL, *video_src = NULL;

        if (sd_audio)
        {
            if (FAILED(hr = media_engine_create_source_node(source, pd, sd_audio, &audio_src)))
                WARN("Failed to create audio source node, hr %#x.\n", hr);

            if (FAILED(hr = media_engine_create_audio_renderer(engine, &sar_node)))
                WARN("Failed to create audio renderer node, hr %#x.\n", hr);

            if (sar_node && audio_src)
            {
                IMFTopology_AddNode(topology, audio_src);
                IMFTopology_AddNode(topology, sar_node);
                IMFTopologyNode_ConnectOutput(audio_src, 0, sar_node, 0);
            }

            if (sar_node)
                IMFTopologyNode_Release(sar_node);
            if (audio_src)
                IMFTopologyNode_Release(audio_src);
        }

        if (SUCCEEDED(hr) && sd_video)
        {
            if (FAILED(hr = media_engine_create_source_node(source, pd, sd_video, &video_src)))
                WARN("Failed to create video source node, hr %#x.\n", hr);

            if (FAILED(hr = media_engine_create_video_renderer(engine, &grabber_node)))
                WARN("Failed to create video grabber node, hr %#x.\n", hr);

            if (grabber_node && video_src)
            {
                IMFTopology_AddNode(topology, video_src);
                IMFTopology_AddNode(topology, grabber_node);
                IMFTopologyNode_ConnectOutput(video_src, 0, grabber_node, 0);
            }

            if (SUCCEEDED(hr))
                IMFTopologyNode_GetTopoNodeID(video_src, &engine->video_frame.node_id);

            if (grabber_node)
                IMFTopologyNode_Release(grabber_node);
            if (video_src)
                IMFTopologyNode_Release(video_src);
        }

        if (SUCCEEDED(hr))
            hr = IMFMediaSession_SetTopology(engine->session, MFSESSION_SETTOPOLOGY_IMMEDIATE, topology);
    }

    if (topology)
        IMFTopology_Release(topology);

    if (sd_video)
        IMFStreamDescriptor_Release(sd_video);
    if (sd_audio)
        IMFStreamDescriptor_Release(sd_audio);

    IMFPresentationDescriptor_Release(pd);

    return hr;
}

static HRESULT WINAPI media_engine_load_handler_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    IUnknown *object = NULL;
    HRESULT hr;

    EnterCriticalSection(&engine->cs);

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADSTART, 0, 0);

    if (FAILED(hr = IMFSourceResolver_EndCreateObjectFromURL(engine->resolver, result, &obj_type, &object)))
        WARN("Failed to create source object, hr %#x.\n", hr);

    if (object)
    {
        if (SUCCEEDED(hr = IUnknown_QueryInterface(object, &IID_IMFMediaSource, (void **)&source)))
        {
            hr = media_engine_create_topology(engine, source);
            IMFMediaSource_Release(source);
        }
        IUnknown_Release(object);
    }

    if (FAILED(hr))
    {
        engine->error_code = MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED;
        engine->extended_code = hr;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_ERROR, engine->error_code,
                engine->extended_code);
    }

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_engine_load_handler_vtbl =
{
    media_engine_callback_QueryInterface,
    media_engine_load_handler_AddRef,
    media_engine_load_handler_Release,
    media_engine_callback_GetParameters,
    media_engine_load_handler_Invoke,
};

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
    if (engine->session)
        IMFMediaSession_Release(engine->session);
    if (engine->attributes)
        IMFAttributes_Release(engine->attributes);
    if (engine->resolver)
        IMFSourceResolver_Release(engine->resolver);
    SysFreeString(engine->current_source);
    DeleteCriticalSection(&engine->cs);
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
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, error);

    *error = NULL;

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->error_code)
    {
        if (SUCCEEDED(hr = create_media_error(error)))
        {
            IMFMediaError_SetErrorCode(*error, engine->error_code);
            IMFMediaError_SetExtendedErrorCode(*error, engine->extended_code);
        }
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetErrorCode(IMFMediaEngine *iface, MF_MEDIA_ENGINE_ERR code)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u.\n", iface, code);

    if ((unsigned int)code > MF_MEDIA_ENGINE_ERR_ENCRYPTED)
        return E_INVALIDARG;

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        engine->error_code = code;
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetSourceElements(IMFMediaEngine *iface, IMFMediaEngineSrcElements *elements)
{
    FIXME("(%p, %p): stub.\n", iface, elements);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetSource(IMFMediaEngine *iface, BSTR url)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s.\n", iface, debugstr_w(url));

    EnterCriticalSection(&engine->cs);

    SysFreeString(engine->current_source);
    engine->current_source = NULL;
    if (url)
        engine->current_source = SysAllocString(url);

    engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_NOTHING;

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

    if (url)
    {
        IPropertyStore *props = NULL;
        unsigned int flags;

        flags = MF_RESOLUTION_MEDIASOURCE;
        if (engine->flags & MF_MEDIA_ENGINE_DISABLE_LOCAL_PLUGINS)
            flags |= MF_RESOLUTION_DISABLE_LOCAL_PLUGINS;

        IMFAttributes_GetUnknown(engine->attributes, &MF_MEDIA_ENGINE_SOURCE_RESOLVER_CONFIG_STORE,
                &IID_IPropertyStore, (void **)&props);
        hr = IMFSourceResolver_BeginCreateObjectFromURL(engine->resolver, url, flags, props, NULL, &engine->load_handler, NULL);
        if (props)
            IPropertyStore_Release(props);
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetCurrentSource(IMFMediaEngine *iface, BSTR *url)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, url);

    *url = NULL;

    EnterCriticalSection(&engine->cs);
    if (engine->current_source)
    {
        if (!(*url = SysAllocString(engine->current_source)))
            hr = E_OUTOFMEMORY;
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static USHORT WINAPI media_engine_GetNetworkState(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0;
}

static MF_MEDIA_ENGINE_PRELOAD WINAPI media_engine_GetPreload(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    MF_MEDIA_ENGINE_PRELOAD preload;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    preload = engine->preload;
    LeaveCriticalSection(&engine->cs);

    return preload;
}

static HRESULT WINAPI media_engine_SetPreload(IMFMediaEngine *iface, MF_MEDIA_ENGINE_PRELOAD preload)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    TRACE("%p, %d.\n", iface, preload);

    EnterCriticalSection(&engine->cs);
    engine->preload = preload;
    LeaveCriticalSection(&engine->cs);

    return S_OK;
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
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    unsigned short state;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    state = engine->ready_state;
    LeaveCriticalSection(&engine->cs);

    return state;
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
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = engine->duration;
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_IsPaused(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_PAUSED);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static double WINAPI media_engine_GetDefaultPlaybackRate(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double rate;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    rate = engine->default_playback_rate;
    LeaveCriticalSection(&engine->cs);

    return rate;
}

static HRESULT WINAPI media_engine_SetDefaultPlaybackRate(IMFMediaEngine *iface, double rate)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, rate);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->default_playback_rate != rate)
    {
        engine->default_playback_rate = rate;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_RATECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static double WINAPI media_engine_GetPlaybackRate(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double rate;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    rate = engine->playback_rate;
    LeaveCriticalSection(&engine->cs);

    return rate;
}

static HRESULT WINAPI media_engine_SetPlaybackRate(IMFMediaEngine *iface, double rate)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, rate);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->playback_rate != rate)
    {
        engine->playback_rate = rate;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_RATECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
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
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_AUTO_PLAY);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_SetAutoPlay(IMFMediaEngine *iface, BOOL autoplay)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    FIXME("(%p, %d): stub.\n", iface, autoplay);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_AUTO_PLAY, autoplay);
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static BOOL WINAPI media_engine_GetLoop(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_LOOP);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_SetLoop(IMFMediaEngine *iface, BOOL loop)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    FIXME("(%p, %d): stub.\n", iface, loop);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_LOOP, loop);
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_Play(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    PROPVARIANT var;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

    if (!(engine->flags & FLAGS_ENGINE_WAITING))
    {
        engine->flags &= ~FLAGS_ENGINE_PAUSED;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PLAY, 0, 0);

        var.vt = VT_EMPTY;
        IMFMediaSession_Start(engine->session, &GUID_NULL, &var);

        engine->flags |= FLAGS_ENGINE_WAITING;
    }

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_WAITING, 0, 0);

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_Pause(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);

    if (!(engine->flags & FLAGS_ENGINE_PAUSED))
    {
        engine->flags &= ~FLAGS_ENGINE_WAITING;
        engine->flags |= FLAGS_ENGINE_PAUSED;

        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_TIMEUPDATE, 0, 0);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PAUSE, 0, 0);
    }

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static BOOL WINAPI media_engine_GetMuted(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL ret;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    ret = !!(engine->flags & FLAGS_ENGINE_MUTED);
    LeaveCriticalSection(&engine->cs);

    return ret;
}

static HRESULT WINAPI media_engine_SetMuted(IMFMediaEngine *iface, BOOL muted)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d.\n", iface, muted);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!!(engine->flags & FLAGS_ENGINE_MUTED) ^ !!muted)
    {
        media_engine_set_flag(engine, FLAGS_ENGINE_MUTED, muted);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static double WINAPI media_engine_GetVolume(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double volume;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    volume = engine->volume;
    LeaveCriticalSection(&engine->cs);

    return volume;
}

static HRESULT WINAPI media_engine_SetVolume(IMFMediaEngine *iface, double volume)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, volume);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (volume != engine->volume)
    {
        engine->volume = volume;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static BOOL WINAPI media_engine_HasVideo(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_HAS_VIDEO);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_HasAudio(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_HAS_AUDIO);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_GetNativeVideoSize(IMFMediaEngine *iface, DWORD *cx, DWORD *cy)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, cx, cy);

    if (!cx && !cy)
        return E_INVALIDARG;

    EnterCriticalSection(&engine->cs);

    if (!engine->video_frame.size.cx && !engine->video_frame.size.cy)
        hr = E_FAIL;
    else
    {
        if (cx) *cx = engine->video_frame.size.cx;
        if (cy) *cy = engine->video_frame.size.cy;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetVideoAspectRatio(IMFMediaEngine *iface, DWORD *cx, DWORD *cy)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, cx, cy);

    if (!cx && !cy)
        return E_INVALIDARG;

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!engine->video_frame.size.cx && !engine->video_frame.size.cy)
        hr = E_FAIL;
    else
    {
        if (cx) *cx = engine->video_frame.ratio.cx;
        if (cy) *cy = engine->video_frame.ratio.cy;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_Shutdown(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    FIXME("(%p): stub.\n", iface);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        engine->flags |= FLAGS_ENGINE_SHUT_DOWN;
        IMFMediaSession_Shutdown(engine->session);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_TransferVideoFrame(IMFMediaEngine *iface, IUnknown *surface,
                                                      const MFVideoNormalizedRect *src,
                                                      const RECT *dst, const MFARGB *color)
{
    FIXME("(%p, %p, %p, %p, %p): stub.\n", iface, surface, src, dst, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_OnVideoStreamTick(IMFMediaEngine *iface, LONGLONG *pts)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, pts);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!pts)
        hr = E_POINTER;
    else
    {
        *pts = engine->video_frame.pts;
        hr = *pts == MINLONGLONG ? S_FALSE : S_OK;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
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

static HRESULT WINAPI media_engine_grabber_callback_QueryInterface(IMFSampleGrabberSinkCallback *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFSampleGrabberSinkCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFSampleGrabberSinkCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_grabber_callback_AddRef(IMFSampleGrabberSinkCallback *iface)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);
    return IMFMediaEngine_AddRef(&engine->IMFMediaEngine_iface);
}

static ULONG WINAPI media_engine_grabber_callback_Release(IMFSampleGrabberSinkCallback *iface)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);
    return IMFMediaEngine_Release(&engine->IMFMediaEngine_iface);
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockStart(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime, LONGLONG start_offset)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockStop(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_FIRST_FRAME, FALSE);
    engine->video_frame.pts = MINLONGLONG;
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockPause(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockRestart(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockSetRate(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime, float rate)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnSetPresentationClock(IMFSampleGrabberSinkCallback *iface,
        IMFPresentationClock *clock)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface,
        REFGUID major_type, DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration,
        const BYTE *buffer, DWORD sample_size)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);

    EnterCriticalSection(&engine->cs);

    if (!(engine->flags & FLAGS_ENGINE_FIRST_FRAME))
    {
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY, 0, 0);
        engine->flags |= FLAGS_ENGINE_FIRST_FRAME;
    }
    engine->video_frame.pts = sample_time;

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return S_OK;
}

static const IMFSampleGrabberSinkCallbackVtbl media_engine_grabber_callback_vtbl =
{
    media_engine_grabber_callback_QueryInterface,
    media_engine_grabber_callback_AddRef,
    media_engine_grabber_callback_Release,
    media_engine_grabber_callback_OnClockStart,
    media_engine_grabber_callback_OnClockStop,
    media_engine_grabber_callback_OnClockPause,
    media_engine_grabber_callback_OnClockRestart,
    media_engine_grabber_callback_OnClockSetRate,
    media_engine_grabber_callback_OnSetPresentationClock,
    media_engine_grabber_callback_OnProcessSample,
    media_engine_grabber_callback_OnShutdown,
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

static HRESULT init_media_engine(DWORD flags, IMFAttributes *attributes, struct media_engine *engine)
{
    DXGI_FORMAT output_format;
    UINT64 playback_hwnd;
    HRESULT hr;

    engine->IMFMediaEngine_iface.lpVtbl = &media_engine_vtbl;
    engine->session_events.lpVtbl = &media_engine_session_events_vtbl;
    engine->load_handler.lpVtbl = &media_engine_load_handler_vtbl;
    engine->grabber_callback.lpVtbl = &media_engine_grabber_callback_vtbl;
    engine->refcount = 1;
    engine->flags = (flags & MF_MEDIA_ENGINE_CREATEFLAGS_MASK) | FLAGS_ENGINE_PAUSED;
    engine->default_playback_rate = 1.0;
    engine->playback_rate = 1.0;
    engine->volume = 1.0;
    engine->duration = NAN;
    engine->video_frame.pts = MINLONGLONG;
    InitializeCriticalSection(&engine->cs);

    hr = IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, &IID_IMFMediaEngineNotify,
            (void **)&engine->callback);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = MFCreateMediaSession(NULL, &engine->session)))
        return hr;

    if (FAILED(hr = IMFMediaSession_BeginGetEvent(engine->session, &engine->session_events, NULL)))
        return hr;

    if (FAILED(hr = MFCreateSourceResolver(&engine->resolver)))
        return hr;

    if (FAILED(hr = MFCreateAttributes(&engine->attributes, 0)))
        return hr;

    if (FAILED(hr = IMFAttributes_CopyAllItems(attributes, engine->attributes)))
        return hr;

    IMFAttributes_GetUINT64(attributes, &MF_MEDIA_ENGINE_PLAYBACK_HWND, &playback_hwnd);
    hr = IMFAttributes_GetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, &output_format);
    if (playback_hwnd) /* FIXME: handle MF_MEDIA_ENGINE_PLAYBACK_VISUAL */
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

    TRACE("%p, %#x, %p, %p.\n", iface, flags, attributes, engine);

    if (!attributes || !engine)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = init_media_engine(flags, attributes, object);
    if (FAILED(hr))
    {
        free_media_engine(object);
        return hr;
    }

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
    TRACE("%p, %p.\n", iface, error);

    return create_media_error(error);
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
