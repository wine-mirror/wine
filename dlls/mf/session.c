/*
 * Copyright 2017 Nikolay Sivov
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
#include "mfidl.h"
#include "mfapi.h"
#include "mferror.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum session_command
{
    SESSION_CMD_CLEAR_TOPOLOGIES,
    SESSION_CMD_CLOSE,
    SESSION_CMD_SET_TOPOLOGY,
    SESSION_CMD_START,
    SESSION_CMD_PAUSE,
    SESSION_CMD_STOP,
};

struct session_op
{
    IUnknown IUnknown_iface;
    LONG refcount;
    enum session_command command;
    union
    {
        struct
        {
            DWORD flags;
            IMFTopology *topology;
        } set_topology;
        struct
        {
            GUID time_format;
            PROPVARIANT start_position;
        } start;
    } u;
};

struct queued_topology
{
    struct list entry;
    IMFTopology *topology;
    MF_TOPOSTATUS status;
};

enum session_state
{
    SESSION_STATE_STOPPED = 0,
    SESSION_STATE_STARTING,
    SESSION_STATE_RUNNING,
    SESSION_STATE_CLOSED,
    SESSION_STATE_SHUT_DOWN,
};

struct media_stream
{
    struct list entry;
    IMFMediaStream *stream;
};

enum source_state
{
    SOURCE_STATE_STOPPED = 0,
    SOURCE_STATE_STARTED,
    SOURCE_STATE_PAUSED,
};

struct media_source
{
    struct list entry;
    IMFMediaSource *source;
    enum source_state state;
    struct list streams;
};

struct media_sink
{
    struct list entry;
    IMFMediaSink *sink;
};

struct media_session
{
    IMFMediaSession IMFMediaSession_iface;
    IMFGetService IMFGetService_iface;
    IMFRateSupport IMFRateSupport_iface;
    IMFRateControl IMFRateControl_iface;
    IMFAsyncCallback commands_callback;
    IMFAsyncCallback events_callback;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
    IMFPresentationClock *clock;
    IMFRateControl *clock_rate_control;
    IMFTopoLoader *topo_loader;
    IMFQualityManager *quality_manager;
    struct
    {
        struct queued_topology current_topology;
        struct list sources;
        struct list sinks;
    } presentation;
    struct list topologies;
    enum session_state state;
    CRITICAL_SECTION cs;
};

struct clock_sink
{
    struct list entry;
    IMFClockStateSink *state_sink;
};

enum clock_command
{
    CLOCK_CMD_START = 0,
    CLOCK_CMD_STOP,
    CLOCK_CMD_PAUSE,
    CLOCK_CMD_SET_RATE,
    CLOCK_CMD_MAX,
};

enum clock_notification
{
    CLOCK_NOTIFY_START,
    CLOCK_NOTIFY_STOP,
    CLOCK_NOTIFY_PAUSE,
    CLOCK_NOTIFY_RESTART,
    CLOCK_NOTIFY_SET_RATE,
};

struct clock_state_change_param
{
    union
    {
        LONGLONG offset;
        float rate;
    } u;
};

struct sink_notification
{
    IUnknown IUnknown_iface;
    LONG refcount;
    MFTIME system_time;
    struct clock_state_change_param param;
    enum clock_notification notification;
    IMFClockStateSink *sink;
};

struct presentation_clock
{
    IMFPresentationClock IMFPresentationClock_iface;
    IMFRateControl IMFRateControl_iface;
    IMFTimer IMFTimer_iface;
    IMFShutdown IMFShutdown_iface;
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    IMFPresentationTimeSource *time_source;
    IMFClockStateSink *time_source_sink;
    MFCLOCK_STATE state;
    struct list sinks;
    float rate;
    CRITICAL_SECTION cs;
};

struct quality_manager
{
    IMFQualityManager IMFQualityManager_iface;
    LONG refcount;
};

static inline struct media_session *impl_from_IMFMediaSession(IMFMediaSession *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFMediaSession_iface);
}

static struct media_session *impl_from_commands_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, commands_callback);
}

static struct media_session *impl_from_events_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, events_callback);
}

static struct media_session *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFGetService_iface);
}

static struct media_session *impl_session_from_IMFRateSupport(IMFRateSupport *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFRateSupport_iface);
}

static struct media_session *impl_session_from_IMFRateControl(IMFRateControl *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFRateControl_iface);
}

static struct session_op *impl_op_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct session_op, IUnknown_iface);
}

static struct presentation_clock *impl_from_IMFPresentationClock(IMFPresentationClock *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFPresentationClock_iface);
}

static struct presentation_clock *impl_from_IMFRateControl(IMFRateControl *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFRateControl_iface);
}

static struct presentation_clock *impl_from_IMFTimer(IMFTimer *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFTimer_iface);
}

static struct presentation_clock *impl_from_IMFShutdown(IMFShutdown *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFShutdown_iface);
}

static struct presentation_clock *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct presentation_clock, IMFAsyncCallback_iface);
}

static struct sink_notification *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct sink_notification, IUnknown_iface);
}

static struct quality_manager *impl_from_IMFQualityManager(IMFQualityManager *iface)
{
    return CONTAINING_RECORD(iface, struct quality_manager, IMFQualityManager_iface);
}

/* IMFLocalMFTRegistration */
static HRESULT WINAPI local_mft_registration_QueryInterface(IMFLocalMFTRegistration *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFLocalMFTRegistration) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFLocalMFTRegistration_AddRef(iface);
        return S_OK;
    }

    WARN("Unexpected %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI local_mft_registration_AddRef(IMFLocalMFTRegistration *iface)
{
    return 2;
}

static ULONG WINAPI local_mft_registration_Release(IMFLocalMFTRegistration *iface)
{
    return 1;
}

static HRESULT WINAPI local_mft_registration_RegisterMFTs(IMFLocalMFTRegistration *iface, MFT_REGISTRATION_INFO *info,
        DWORD count)
{
    HRESULT hr = S_OK;
    DWORD i;

    TRACE("%p, %p, %u.\n", iface, info, count);

    for (i = 0; i < count; ++i)
    {
        if (FAILED(hr = MFTRegisterLocalByCLSID(&info[i].clsid, &info[i].guidCategory, info[i].pszName,
                info[i].uiFlags, info[i].cInTypes, info[i].pInTypes, info[i].cOutTypes, info[i].pOutTypes)))
        {
            break;
        }
    }

    return hr;
}

static const IMFLocalMFTRegistrationVtbl local_mft_registration_vtbl =
{
    local_mft_registration_QueryInterface,
    local_mft_registration_AddRef,
    local_mft_registration_Release,
    local_mft_registration_RegisterMFTs,
};

static IMFLocalMFTRegistration local_mft_registration = { &local_mft_registration_vtbl };

static HRESULT WINAPI session_op_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_op_AddRef(IUnknown *iface)
{
    struct session_op *op = impl_op_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&op->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI session_op_Release(IUnknown *iface)
{
    struct session_op *op = impl_op_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&op->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        switch (op->command)
        {
            case SESSION_CMD_SET_TOPOLOGY:
                if (op->u.set_topology.topology)
                    IMFTopology_Release(op->u.set_topology.topology);
                break;
            case SESSION_CMD_START:
                PropVariantClear(&op->u.start.start_position);
                break;
            default:
                ;
        }
        heap_free(op);
    }

    return refcount;
}

static IUnknownVtbl session_op_vtbl =
{
    session_op_QueryInterface,
    session_op_AddRef,
    session_op_Release,
};

static HRESULT create_session_op(enum session_command command, struct session_op **ret)
{
    struct session_op *op;

    if (!(op = heap_alloc_zero(sizeof(*op))))
        return E_OUTOFMEMORY;

    op->IUnknown_iface.lpVtbl = &session_op_vtbl;
    op->refcount = 1;
    op->command = command;

    *ret = op;

    return S_OK;
}

static HRESULT session_submit_command(struct media_session *session, struct session_op *op)
{
    HRESULT hr;

    EnterCriticalSection(&session->cs);
    if (session->state == SESSION_STATE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &session->commands_callback, &op->IUnknown_iface);
    LeaveCriticalSection(&session->cs);

    return hr;
}

static void session_clear_topologies(struct media_session *session)
{
    struct queued_topology *ptr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(ptr, next, &session->topologies, struct queued_topology, entry)
    {
        list_remove(&ptr->entry);
        IMFTopology_Release(ptr->topology);
        heap_free(ptr);
    }
}

static void session_set_topo_status(struct media_session *session, struct queued_topology *topology, HRESULT status,
        MF_TOPOSTATUS topo_status)
{
    IMFMediaEvent *event;
    PROPVARIANT param;

    if (topo_status == MF_TOPOSTATUS_INVALID)
        return;

    if (topo_status > topology->status)
    {
        param.vt = topology ? VT_UNKNOWN : VT_EMPTY;
        param.punkVal = topology ? (IUnknown *)topology->topology : NULL;

        if (FAILED(MFCreateMediaEvent(MESessionTopologyStatus, &GUID_NULL, status, &param, &event)))
            return;

        IMFMediaEvent_SetUINT32(event, &MF_EVENT_TOPOLOGY_STATUS, topo_status);
        topology->status = topo_status;
        IMFMediaEventQueue_QueueEvent(session->event_queue, event);
    }

    IMFMediaEvent_Release(event);
}

static HRESULT session_bind_output_nodes(IMFTopology *topology)
{
    MF_TOPOLOGY_TYPE node_type;
    IMFStreamSink *stream_sink;
    IMFMediaSink *media_sink;
    WORD node_count = 0, i;
    IMFTopologyNode *node;
    IMFActivate *activate;
    UINT32 stream_id;
    IUnknown *object;
    HRESULT hr;

    hr = IMFTopology_GetNodeCount(topology, &node_count);

    for (i = 0; i < node_count; ++i)
    {
        if (FAILED(hr = IMFTopology_GetNode(topology, i, &node)))
            break;

        if (FAILED(hr = IMFTopologyNode_GetNodeType(node, &node_type)) || node_type != MF_TOPOLOGY_OUTPUT_NODE)
        {
            IMFTopologyNode_Release(node);
            continue;
        }

        if (SUCCEEDED(hr = IMFTopologyNode_GetObject(node, &object)))
        {
            stream_sink = NULL;
            if (FAILED(IUnknown_QueryInterface(object, &IID_IMFStreamSink, (void **)&stream_sink)))
            {
                if (SUCCEEDED(hr = IUnknown_QueryInterface(object, &IID_IMFActivate, (void **)&activate)))
                {
                    if (SUCCEEDED(hr = IMFActivate_ActivateObject(activate, &IID_IMFMediaSink, (void **)&media_sink)))
                    {
                        if (FAILED(IMFTopologyNode_GetUINT32(node, &MF_TOPONODE_STREAMID, &stream_id)))
                            stream_id = 0;

                        stream_sink = NULL;
                        if (FAILED(IMFMediaSink_GetStreamSinkById(media_sink, stream_id, &stream_sink)))
                            hr = IMFMediaSink_AddStreamSink(media_sink, stream_id, NULL, &stream_sink);

                        if (stream_sink)
                            hr = IMFTopologyNode_SetObject(node, (IUnknown *)stream_sink);

                        IMFMediaSink_Release(media_sink);
                    }

                    IMFActivate_Release(activate);
                }
            }

            if (stream_sink)
                IMFStreamSink_Release(stream_sink);
            IUnknown_Release(object);
        }

        IMFTopologyNode_Release(node);
    }

    return hr;
}

static IMFTopology *session_get_next_topology(struct media_session *session)
{
    struct queued_topology *queued;

    if (!session->presentation.current_topology.topology)
    {
        struct list *head = list_head(&session->topologies);
        if (!head)
            return NULL;

        queued = LIST_ENTRY(head, struct queued_topology, entry);
        session->presentation.current_topology = *queued;
        list_remove(&queued->entry);
        heap_free(queued);
    }

    return session->presentation.current_topology.topology;
}

static void session_clear_presentation(struct media_session *session)
{
    struct media_source *source, *source2;
    struct media_stream *stream, *stream2;
    struct media_sink *sink, *sink2;

    if (session->presentation.current_topology.topology)
    {
        IMFTopology_Release(session->presentation.current_topology.topology);
        session->presentation.current_topology.topology = NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(source, source2, &session->presentation.sources, struct media_source, entry)
    {
        list_remove(&source->entry);

        LIST_FOR_EACH_ENTRY_SAFE(stream, stream2, &source->streams, struct media_stream, entry)
        {
            list_remove(&stream->entry);
            if (stream->stream)
                IMFMediaStream_Release(stream->stream);
            heap_free(stream);
        }

        if (source->source)
            IMFMediaSource_Release(source->source);
        heap_free(source);
    }

    LIST_FOR_EACH_ENTRY_SAFE(sink, sink2, &session->presentation.sinks, struct media_sink, entry)
    {
        list_remove(&sink->entry);

        if (sink->sink)
            IMFMediaSink_Release(sink->sink);
        heap_free(sink);
    }
}

static void session_start(struct media_session *session, const GUID *time_format, const PROPVARIANT *start_position)
{
    IMFTopologyNode *node;
    IMFTopology *topology;
    IMFCollection *nodes;
    DWORD count, i;
    HRESULT hr;

    EnterCriticalSection(&session->cs);

    switch (session->state)
    {
        case SESSION_STATE_STOPPED:
            topology = session_get_next_topology(session);

            if (!topology || FAILED(IMFTopology_GetSourceNodeCollection(topology, &nodes)))
                break;

            if (FAILED(IMFCollection_GetElementCount(nodes, &count)))
                break;

            for (i = 0; i < count; ++i)
            {
                if (SUCCEEDED(hr = IMFCollection_GetElement(nodes, i, (IUnknown **)&node)))
                {
                    IMFPresentationDescriptor *pd = NULL;
                    struct media_source *source;

                    if (!(source = heap_alloc_zero(sizeof(*source))))
                        hr = E_OUTOFMEMORY;

                    if (SUCCEEDED(hr))
                        hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_SOURCE, &IID_IMFMediaSource,
                                (void **)&source->source);

                    if (SUCCEEDED(hr))
                        hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR,
                                &IID_IMFPresentationDescriptor, (void **)&pd);

                    /* Subscribe to source events, start it. */
                    if (SUCCEEDED(hr))
                        hr = IMFMediaSource_BeginGetEvent(source->source, &session->events_callback,
                                (IUnknown *)source->source);

                    if (SUCCEEDED(hr))
                        hr = IMFMediaSource_Start(source->source, pd, time_format, start_position);

                    if (pd)
                        IMFPresentationDescriptor_Release(pd);

                    if (SUCCEEDED(hr))
                    {
                        list_add_tail(&session->presentation.sources, &source->entry);
                    }
                    else if (source)
                    {
                        if (source->source)
                            IMFMediaSource_Release(source->source);
                        heap_free(source);
                    }

                    IMFTopologyNode_Release(node);
                }
            }

            session->state = SESSION_STATE_STARTING;

            IMFCollection_Release(nodes);

            break;
        case SESSION_STATE_RUNNING:
            FIXME("Seeking is not implemented.\n");
            break;
        default:
            ;
    }

    LeaveCriticalSection(&session->cs);
}

static HRESULT WINAPI mfsession_QueryInterface(IMFMediaSession *iface, REFIID riid, void **out)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaSession) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &session->IMFMediaSession_iface;
        IMFMediaSession_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *out = &session->IMFGetService_iface;
        IMFMediaSession_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mfsession_AddRef(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    ULONG refcount = InterlockedIncrement(&session->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI mfsession_Release(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    ULONG refcount = InterlockedDecrement(&session->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        session_clear_topologies(session);
        session_clear_presentation(session);
        if (session->event_queue)
            IMFMediaEventQueue_Release(session->event_queue);
        if (session->clock)
            IMFPresentationClock_Release(session->clock);
        if (session->clock_rate_control)
            IMFRateControl_Release(session->clock_rate_control);
        if (session->topo_loader)
            IMFTopoLoader_Release(session->topo_loader);
        if (session->quality_manager)
            IMFQualityManager_Release(session->quality_manager);
        DeleteCriticalSection(&session->cs);
        heap_free(session);
    }

    return refcount;
}

static HRESULT WINAPI mfsession_GetEvent(IMFMediaSession *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(session->event_queue, flags, event);
}

static HRESULT WINAPI mfsession_BeginGetEvent(IMFMediaSession *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(session->event_queue, callback, state);
}

static HRESULT WINAPI mfsession_EndGetEvent(IMFMediaSession *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(session->event_queue, result, event);
}

static HRESULT WINAPI mfsession_QueueEvent(IMFMediaSession *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %d, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(session->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI mfsession_SetTopology(IMFMediaSession *iface, DWORD flags, IMFTopology *topology)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    WORD node_count = 0;
    HRESULT hr;

    TRACE("%p, %#x, %p.\n", iface, flags, topology);

    if (topology)
    {
        if (FAILED(IMFTopology_GetNodeCount(topology, &node_count)) || node_count == 0)
            return E_INVALIDARG;
    }

    if (FAILED(hr = create_session_op(SESSION_CMD_SET_TOPOLOGY, &op)))
        return hr;

    op->u.set_topology.flags = flags;
    op->u.set_topology.topology = topology;
    if (op->u.set_topology.topology)
        IMFTopology_AddRef(op->u.set_topology.topology);

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_ClearTopologies(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p.\n", iface);

    if (FAILED(hr = create_session_op(SESSION_CMD_CLEAR_TOPOLOGIES, &op)))
        return hr;

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_Start(IMFMediaSession *iface, const GUID *format, const PROPVARIANT *start_position)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(format), start_position);

    if (!start_position)
        return E_POINTER;

    if (FAILED(hr = create_session_op(SESSION_CMD_START, &op)))
        return hr;

    op->u.start.time_format = format ? *format : GUID_NULL;
    hr = PropVariantCopy(&op->u.start.start_position, start_position);

    if (SUCCEEDED(hr))
        hr = session_submit_command(session, op);

    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_Pause(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p.\n", iface);

    if (FAILED(hr = create_session_op(SESSION_CMD_PAUSE, &op)))
        return hr;

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_Stop(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p.\n", iface);

    if (FAILED(hr = create_session_op(SESSION_CMD_STOP, &op)))
        return hr;

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_Close(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p.\n", iface);

    if (FAILED(hr = create_session_op(SESSION_CMD_CLOSE, &op)))
        return hr;

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_Shutdown(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    HRESULT hr = S_OK;

    FIXME("%p.\n", iface);

    EnterCriticalSection(&session->cs);
    if (session->state == SESSION_STATE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        session->state = SESSION_STATE_SHUT_DOWN;
        IMFMediaEventQueue_Shutdown(session->event_queue);
    }
    LeaveCriticalSection(&session->cs);

    return hr;
}

static HRESULT WINAPI mfsession_GetClock(IMFMediaSession *iface, IMFClock **clock)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %p.\n", iface, clock);

    *clock = (IMFClock *)session->clock;
    IMFClock_AddRef(*clock);

    return S_OK;
}

static HRESULT WINAPI mfsession_GetSessionCapabilities(IMFMediaSession *iface, DWORD *caps)
{
    FIXME("%p, %p.\n", iface, caps);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetFullTopology(IMFMediaSession *iface, DWORD flags, TOPOID id, IMFTopology **topology)
{
    FIXME("%p, %#x, %s, %p.\n", iface, flags, wine_dbgstr_longlong(id), topology);

    return E_NOTIMPL;
}

static const IMFMediaSessionVtbl mfmediasessionvtbl =
{
    mfsession_QueryInterface,
    mfsession_AddRef,
    mfsession_Release,
    mfsession_GetEvent,
    mfsession_BeginGetEvent,
    mfsession_EndGetEvent,
    mfsession_QueueEvent,
    mfsession_SetTopology,
    mfsession_ClearTopologies,
    mfsession_Start,
    mfsession_Pause,
    mfsession_Stop,
    mfsession_Close,
    mfsession_Shutdown,
    mfsession_GetClock,
    mfsession_GetSessionCapabilities,
    mfsession_GetFullTopology,
};

static HRESULT WINAPI session_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct media_session *session = impl_from_IMFGetService(iface);
    return IMFMediaSession_QueryInterface(&session->IMFMediaSession_iface, riid, obj);
}

static ULONG WINAPI session_get_service_AddRef(IMFGetService *iface)
{
    struct media_session *session = impl_from_IMFGetService(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_get_service_Release(IMFGetService *iface)
{
    struct media_session *session = impl_from_IMFGetService(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct media_session *session = impl_from_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(service, &MF_RATE_CONTROL_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFRateSupport))
        {
            *obj = &session->IMFRateSupport_iface;
        }
        else if (IsEqualIID(riid, &IID_IMFRateControl))
        {
            *obj = &session->IMFRateControl_iface;
        }
    }
    else if (IsEqualGUID(service, &MF_LOCAL_MFT_REGISTRATION_SERVICE))
    {
        return IMFLocalMFTRegistration_QueryInterface(&local_mft_registration, riid, obj);
    }
    else
        FIXME("Unsupported service %s.\n", debugstr_guid(service));

    if (*obj)
        IUnknown_AddRef((IUnknown *)*obj);

    return *obj ? S_OK : E_NOINTERFACE;
}

static const IMFGetServiceVtbl session_get_service_vtbl =
{
    session_get_service_QueryInterface,
    session_get_service_AddRef,
    session_get_service_Release,
    session_get_service_GetService,
};

static HRESULT WINAPI session_commands_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_commands_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_commands_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_commands_callback_Release(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_commands_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_commands_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static void session_raise_topology_set(struct media_session *session, IMFTopology *topology, HRESULT status)
{
    PROPVARIANT param;

    param.vt = topology ? VT_UNKNOWN : VT_EMPTY;
    param.punkVal = (IUnknown *)topology;

    IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MESessionTopologySet, &GUID_NULL, status, &param);
}

static HRESULT WINAPI session_commands_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct session_op *op = impl_op_from_IUnknown(IMFAsyncResult_GetStateNoAddRef(result));
    struct media_session *session = impl_from_commands_callback_IMFAsyncCallback(iface);
    HRESULT status = S_OK;

    switch (op->command)
    {
        case SESSION_CMD_CLEAR_TOPOLOGIES:
            EnterCriticalSection(&session->cs);
            session_clear_topologies(session);
            LeaveCriticalSection(&session->cs);

            IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MESessionTopologiesCleared, &GUID_NULL,
                    S_OK, NULL);
            break;
        case SESSION_CMD_SET_TOPOLOGY:
        {
            IMFTopology *topology = op->u.set_topology.topology;
            MF_TOPOSTATUS topo_status = MF_TOPOSTATUS_INVALID;
            struct queued_topology *queued_topology = NULL;
            DWORD flags = op->u.set_topology.flags;

            /* Resolve unless claimed to be full. */
            if (!(flags & MFSESSION_SETTOPOLOGY_CLEAR_CURRENT) && topology)
            {
                if (!(flags & MFSESSION_SETTOPOLOGY_NORESOLUTION))
                {
                    IMFTopology *resolved_topology = NULL;

                    status = session_bind_output_nodes(topology);

                    if (SUCCEEDED(status))
                        status = IMFTopoLoader_Load(session->topo_loader, topology, &resolved_topology, NULL /* FIXME? */);

                    if (SUCCEEDED(status))
                    {
                        topology = resolved_topology;
                        topo_status = MF_TOPOSTATUS_READY;
                    }
                }

                if (SUCCEEDED(status))
                {
                    if (!(queued_topology = heap_alloc_zero(sizeof(*queued_topology))))
                        status = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(status))
                {
                    queued_topology->topology = topology;
                    IMFTopology_AddRef(queued_topology->topology);
                }
            }

            EnterCriticalSection(&session->cs);

            if (flags & MFSESSION_SETTOPOLOGY_CLEAR_CURRENT)
            {
                if ((topology && topology == session->presentation.current_topology.topology) || !topology)
                {
                    /* FIXME: stop current topology, queue next one. */
                    session_clear_presentation(session);
                }
                else
                    status = S_FALSE;
                topo_status = MF_TOPOSTATUS_READY;
                queued_topology = &session->presentation.current_topology;
            }
            else if (queued_topology)
            {
                if (flags & MFSESSION_SETTOPOLOGY_IMMEDIATE)
                {
                    session_clear_topologies(session);
                    session_clear_presentation(session);
                }

                list_add_tail(&session->topologies, &queued_topology->entry);
            }

            session_raise_topology_set(session, topology, status);
            session_set_topo_status(session, queued_topology, status, topo_status);

            LeaveCriticalSection(&session->cs);

            break;
        }
        case SESSION_CMD_START:
        {
            session_start(session, &op->u.start.time_format, &op->u.start.start_position);
            break;
        }
        case SESSION_CMD_PAUSE:
            break;
        case SESSION_CMD_CLOSE:
            EnterCriticalSection(&session->cs);
            if (session->state != SESSION_STATE_CLOSED)
            {
                /* FIXME: actually do something to presentation objects */
                session->state = SESSION_STATE_CLOSED;
                IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MESessionClosed, &GUID_NULL, S_OK, NULL);
            }
            LeaveCriticalSection(&session->cs);
            break;
        default:
            ;
    }

    return S_OK;
}

static const IMFAsyncCallbackVtbl session_commands_callback_vtbl =
{
    session_commands_callback_QueryInterface,
    session_commands_callback_AddRef,
    session_commands_callback_Release,
    session_commands_callback_GetParameters,
    session_commands_callback_Invoke,
};

static HRESULT WINAPI session_events_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_events_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_events_callback_Release(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_events_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_events_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT session_add_media_stream(struct media_source *source, IMFMediaStream *stream)
{
    struct media_stream *media_stream;

    if (!(media_stream = heap_alloc_zero(sizeof(*media_stream))))
        return E_OUTOFMEMORY;

    media_stream->stream = stream;
    IMFMediaStream_AddRef(media_stream->stream);

    list_add_tail(&source->streams, &media_stream->entry);

    return S_OK;
}

static BOOL session_set_source_state(struct media_session *session, IMFMediaSource *source, enum source_state state)
{
    struct media_source *cur;
    BOOL ret = TRUE;

    LIST_FOR_EACH_ENTRY(cur, &session->presentation.sources, struct media_source, entry)
    {
        if (source == cur->source)
            cur->state = state;
        ret &= cur->state == state;
    }

    return ret;
}

static HRESULT session_set_sinks_clock(struct media_session *session)
{
    IMFTopology *topology = session->presentation.current_topology.topology;
    IMFTopologyNode *node;
    IMFCollection *nodes;
    DWORD count, i;
    HRESULT hr;

    if (!list_empty(&session->presentation.sinks))
        return S_OK;

    if (FAILED(hr = IMFTopology_GetOutputNodeCollection(topology, &nodes)))
        return hr;

    if (FAILED(hr = IMFCollection_GetElementCount(nodes, &count)))
    {
        IMFCollection_Release(nodes);
        return hr;
    }

    for (i = 0; i < count; ++i)
    {
        IMFStreamSink *stream_sink = NULL;
        struct media_sink *sink;

        if (FAILED(hr = IMFCollection_GetElement(nodes, i, (IUnknown **)&node)))
            break;

        if (!(sink = heap_alloc_zero(sizeof(*sink))))
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
            hr = IMFTopologyNode_GetObject(node, (IUnknown **)&stream_sink);

        if (SUCCEEDED(hr))
            hr = IMFStreamSink_GetMediaSink(stream_sink, &sink->sink);

        if (SUCCEEDED(hr))
            hr = IMFMediaSink_SetPresentationClock(sink->sink, session->clock);

        if (SUCCEEDED(hr))
            hr = IMFStreamSink_BeginGetEvent(stream_sink, &session->events_callback, (IUnknown *)stream_sink);

        if (stream_sink)
            IMFStreamSink_Release(stream_sink);

        if (SUCCEEDED(hr))
        {
            list_add_tail(&session->presentation.sinks, &sink->entry);
        }
        else if (sink)
        {
            if (sink->sink)
                IMFMediaSink_Release(sink->sink);
            heap_free(sink);
        }

        IMFTopologyNode_Release(node);
    }

    IMFCollection_Release(nodes);

    return hr;
}

static HRESULT WINAPI session_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_session *session = impl_from_events_callback_IMFAsyncCallback(iface);
    IMFMediaEventGenerator *event_source;
    enum source_state source_state;
    IMFMediaEvent *event = NULL;
    MediaEventType event_type;
    struct media_source *cur;
    IMFMediaSource *source;
    IMFMediaStream *stream;
    PROPVARIANT value;
    HRESULT hr;
    BOOL ret;

    if (FAILED(hr = IMFAsyncResult_GetState(result, (IUnknown **)&event_source)))
        return hr;

    if (FAILED(hr = IMFMediaEventGenerator_EndGetEvent(event_source, result, &event)))
    {
        WARN("Failed to get event from %p, hr %#x.\n", event_source, hr);
        goto failed;
    }

    if (FAILED(hr = IMFMediaEvent_GetType(event, &event_type)))
    {
        WARN("Failed to get event type, hr %#x.\n", hr);
        goto failed;
    }

    value.vt = VT_EMPTY;
    if (FAILED(hr = IMFMediaEvent_GetValue(event, &value)))
    {
        WARN("Failed to get event value, hr %#x.\n", hr);
        goto failed;
    }

    switch (event_type)
    {
        case MESourceStarted:
        case MESourcePaused:
        case MESourceStopped:
            if (event_type == MESourceStarted)
                source_state = SOURCE_STATE_STARTED;
            else if (event_type == MESourcePaused)
                source_state = SOURCE_STATE_PAUSED;
            else
                source_state = SOURCE_STATE_STOPPED;

            EnterCriticalSection(&session->cs);

            ret = session_set_source_state(session, (IMFMediaSource *)event_source, source_state);
            if (ret && event_type == MESourceStarted)
            {
                session_set_topo_status(session, &session->presentation.current_topology, S_OK,
                        MF_TOPOSTATUS_STARTED_SOURCE);

                if (session->state == SESSION_STATE_STARTING)
                {
                    if (SUCCEEDED(session_set_sinks_clock(session)))
                    {
                        session->state = SESSION_STATE_RUNNING;
                    }
                }
            }

            LeaveCriticalSection(&session->cs);
            break;
        case MENewStream:
            stream = (IMFMediaStream *)value.punkVal;

            if (value.vt != VT_UNKNOWN || !stream)
            {
                WARN("Unexpected event value.\n");
                break;
            }

            if (FAILED(hr = IMFMediaStream_GetMediaSource(stream, &source)))
                break;

            EnterCriticalSection(&session->cs);

            LIST_FOR_EACH_ENTRY(cur, &session->presentation.sources, struct media_source, entry)
            {
                if (source == cur->source)
                {
                    hr = session_add_media_stream(cur, stream);

                    if (SUCCEEDED(hr))
                        hr = IMFMediaStream_BeginGetEvent(stream, &session->events_callback, (IUnknown *)stream);

                    break;
                }
            }

            LeaveCriticalSection(&session->cs);

            break;
        default:
            ;
    }

    PropVariantClear(&value);

failed:
    if (event)
        IMFMediaEvent_Release(event);
    IMFMediaEventGenerator_Release(event_source);

    return hr;
}

static const IMFAsyncCallbackVtbl session_events_callback_vtbl =
{
    session_events_callback_QueryInterface,
    session_events_callback_AddRef,
    session_events_callback_Release,
    session_events_callback_GetParameters,
    session_events_callback_Invoke,
};

static HRESULT WINAPI session_rate_support_QueryInterface(IMFRateSupport *iface, REFIID riid, void **obj)
{
    struct media_session *session = impl_session_from_IMFRateSupport(iface);
    return IMFMediaSession_QueryInterface(&session->IMFMediaSession_iface, riid, obj);
}

static ULONG WINAPI session_rate_support_AddRef(IMFRateSupport *iface)
{
    struct media_session *session = impl_session_from_IMFRateSupport(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_rate_support_Release(IMFRateSupport *iface)
{
    struct media_session *session = impl_session_from_IMFRateSupport(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_rate_support_GetSlowestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    FIXME("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI session_rate_support_GetFastestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    FIXME("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI session_rate_support_IsRateSupported(IMFRateSupport *iface, BOOL thin, float rate,
        float *nearest_supported_rate)
{
    FIXME("%p, %d, %f, %p.\n", iface, thin, rate, nearest_supported_rate);

    return E_NOTIMPL;
}

static const IMFRateSupportVtbl session_rate_support_vtbl =
{
    session_rate_support_QueryInterface,
    session_rate_support_AddRef,
    session_rate_support_Release,
    session_rate_support_GetSlowestRate,
    session_rate_support_GetFastestRate,
    session_rate_support_IsRateSupported,
};

static HRESULT WINAPI session_rate_control_QueryInterface(IMFRateControl *iface, REFIID riid, void **obj)
{
    struct media_session *session = impl_session_from_IMFRateControl(iface);
    return IMFMediaSession_QueryInterface(&session->IMFMediaSession_iface, riid, obj);
}

static ULONG WINAPI session_rate_control_AddRef(IMFRateControl *iface)
{
    struct media_session *session = impl_session_from_IMFRateControl(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_rate_control_Release(IMFRateControl *iface)
{
    struct media_session *session = impl_session_from_IMFRateControl(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_rate_control_SetRate(IMFRateControl *iface, BOOL thin, float rate)
{
    FIXME("%p, %d, %f.\n", iface, thin, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI session_rate_control_GetRate(IMFRateControl *iface, BOOL *thin, float *rate)
{
    struct media_session *session = impl_session_from_IMFRateControl(iface);

    TRACE("%p, %p, %p.\n", iface, thin, rate);

    return IMFRateControl_GetRate(session->clock_rate_control, thin, rate);
}

static const IMFRateControlVtbl session_rate_control_vtbl =
{
    session_rate_control_QueryInterface,
    session_rate_control_AddRef,
    session_rate_control_Release,
    session_rate_control_SetRate,
    session_rate_control_GetRate,
};

/***********************************************************************
 *      MFCreateMediaSession (mf.@)
 */
HRESULT WINAPI MFCreateMediaSession(IMFAttributes *config, IMFMediaSession **session)
{
    BOOL without_quality_manager = FALSE;
    struct media_session *object;
    HRESULT hr;

    TRACE("%p, %p.\n", config, session);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaSession_iface.lpVtbl = &mfmediasessionvtbl;
    object->IMFGetService_iface.lpVtbl = &session_get_service_vtbl;
    object->IMFRateSupport_iface.lpVtbl = &session_rate_support_vtbl;
    object->IMFRateControl_iface.lpVtbl = &session_rate_control_vtbl;
    object->commands_callback.lpVtbl = &session_commands_callback_vtbl;
    object->events_callback.lpVtbl = &session_events_callback_vtbl;
    object->refcount = 1;
    list_init(&object->topologies);
    list_init(&object->presentation.sources);
    list_init(&object->presentation.sinks);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreatePresentationClock(&object->clock)))
        goto failed;

    if (FAILED(hr = IMFPresentationClock_QueryInterface(object->clock, &IID_IMFRateControl,
            (void **)&object->clock_rate_control)))
    {
        goto failed;
    }

    if (config)
    {
        GUID clsid;

        if (SUCCEEDED(IMFAttributes_GetGUID(config, &MF_SESSION_TOPOLOADER, &clsid)))
        {
            if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTopoLoader,
                    (void **)&object->topo_loader)))
            {
                WARN("Failed to create custom topology loader, hr %#x.\n", hr);
            }
        }

        if (SUCCEEDED(IMFAttributes_GetGUID(config, &MF_SESSION_QUALITY_MANAGER, &clsid)))
        {
            if (!(without_quality_manager = IsEqualGUID(&clsid, &GUID_NULL)))
            {
                if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFQualityManager,
                        (void **)&object->quality_manager)))
                {
                    WARN("Failed to create custom quality manager, hr %#x.\n", hr);
                }
            }
        }
    }

    if (!object->topo_loader && FAILED(hr = MFCreateTopoLoader(&object->topo_loader)))
        goto failed;

    if (!object->quality_manager && !without_quality_manager &&
            FAILED(hr = MFCreateStandardQualityManager(&object->quality_manager)))
    {
        goto failed;
    }

    *session = &object->IMFMediaSession_iface;

    return S_OK;

failed:
    IMFMediaSession_Release(&object->IMFMediaSession_iface);
    return hr;
}

static HRESULT WINAPI present_clock_QueryInterface(IMFPresentationClock *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFPresentationClock) ||
            IsEqualIID(riid, &IID_IMFClock) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &clock->IMFPresentationClock_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFRateControl))
    {
        *out = &clock->IMFRateControl_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFTimer))
    {
        *out = &clock->IMFTimer_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFShutdown))
    {
        *out = &clock->IMFShutdown_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI present_clock_AddRef(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    ULONG refcount = InterlockedIncrement(&clock->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI present_clock_Release(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    ULONG refcount = InterlockedDecrement(&clock->refcount);
    struct clock_sink *sink, *sink2;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (clock->time_source)
            IMFPresentationTimeSource_Release(clock->time_source);
        if (clock->time_source_sink)
            IMFClockStateSink_Release(clock->time_source_sink);
        LIST_FOR_EACH_ENTRY_SAFE(sink, sink2, &clock->sinks, struct clock_sink, entry)
        {
            list_remove(&sink->entry);
            IMFClockStateSink_Release(sink->state_sink);
            heap_free(sink);
        }
        DeleteCriticalSection(&clock->cs);
        heap_free(clock);
    }

    return refcount;
}

static HRESULT WINAPI present_clock_GetClockCharacteristics(IMFPresentationClock *iface, DWORD *flags)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;

    TRACE("%p, %p.\n", iface, flags);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetClockCharacteristics(clock->time_source, flags);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetCorrelatedTime(IMFPresentationClock *iface, DWORD reserved,
        LONGLONG *clock_time, MFTIME *system_time)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;

    TRACE("%p, %#x, %p, %p.\n", iface, reserved, clock_time, system_time);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetCorrelatedTime(clock->time_source, reserved, clock_time, system_time);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetContinuityKey(IMFPresentationClock *iface, DWORD *key)
{
    TRACE("%p, %p.\n", iface, key);

    *key = 0;

    return S_OK;
}

static HRESULT WINAPI present_clock_GetState(IMFPresentationClock *iface, DWORD reserved, MFCLOCK_STATE *state)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);

    TRACE("%p, %#x, %p.\n", iface, reserved, state);

    EnterCriticalSection(&clock->cs);
    *state = clock->state;
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static HRESULT WINAPI present_clock_GetProperties(IMFPresentationClock *iface, MFCLOCK_PROPERTIES *props)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;

    TRACE("%p, %p.\n", iface, props);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetProperties(clock->time_source, props);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_SetTimeSource(IMFPresentationClock *iface,
        IMFPresentationTimeSource *time_source)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, time_source);

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        IMFPresentationTimeSource_Release(clock->time_source);
    if (clock->time_source_sink)
        IMFClockStateSink_Release(clock->time_source_sink);
    clock->time_source = NULL;
    clock->time_source_sink = NULL;

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&clock->time_source_sink);
    if (SUCCEEDED(hr))
    {
        clock->time_source = time_source;
        IMFPresentationTimeSource_AddRef(clock->time_source);
    }

    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetTimeSource(IMFPresentationClock *iface,
        IMFPresentationTimeSource **time_source)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, time_source);

    if (!time_source)
        return E_INVALIDARG;

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
    {
        *time_source = clock->time_source;
        IMFPresentationTimeSource_AddRef(*time_source);
    }
    else
        hr = MF_E_CLOCK_NO_TIME_SOURCE;
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_GetTime(IMFPresentationClock *iface, MFTIME *time)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    HRESULT hr = MF_E_CLOCK_NO_TIME_SOURCE;
    MFTIME systime;

    TRACE("%p, %p.\n", iface, time);

    if (!time)
        return E_POINTER;

    EnterCriticalSection(&clock->cs);
    if (clock->time_source)
        hr = IMFPresentationTimeSource_GetCorrelatedTime(clock->time_source, 0, time, &systime);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_AddClockStateSink(IMFPresentationClock *iface, IMFClockStateSink *state_sink)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_sink *sink, *cur;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, state_sink);

    if (!state_sink)
        return E_INVALIDARG;

    sink = heap_alloc(sizeof(*sink));
    if (!sink)
        return E_OUTOFMEMORY;

    sink->state_sink = state_sink;
    IMFClockStateSink_AddRef(sink->state_sink);

    EnterCriticalSection(&clock->cs);
    LIST_FOR_EACH_ENTRY(cur, &clock->sinks, struct clock_sink, entry)
    {
        if (cur->state_sink == state_sink)
        {
            hr = E_INVALIDARG;
            break;
        }
    }
    if (SUCCEEDED(hr))
        list_add_tail(&clock->sinks, &sink->entry);
    LeaveCriticalSection(&clock->cs);

    if (FAILED(hr))
    {
        IMFClockStateSink_Release(sink->state_sink);
        heap_free(sink);
    }

    return hr;
}

static HRESULT WINAPI present_clock_RemoveClockStateSink(IMFPresentationClock *iface,
        IMFClockStateSink *state_sink)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_sink *sink;

    TRACE("%p, %p.\n", iface, state_sink);

    if (!state_sink)
        return E_INVALIDARG;

    EnterCriticalSection(&clock->cs);
    LIST_FOR_EACH_ENTRY(sink, &clock->sinks, struct clock_sink, entry)
    {
        if (sink->state_sink == state_sink)
        {
            IMFClockStateSink_Release(sink->state_sink);
            list_remove(&sink->entry);
            heap_free(sink);
            break;
        }
    }
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static HRESULT WINAPI sink_notification_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_notification_AddRef(IUnknown *iface)
{
    struct sink_notification *notification = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&notification->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sink_notification_Release(IUnknown *iface)
{
    struct sink_notification *notification = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&notification->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        IMFClockStateSink_Release(notification->sink);
        heap_free(notification);
    }

    return refcount;
}

static const IUnknownVtbl sinknotificationvtbl =
{
    sink_notification_QueryInterface,
    sink_notification_AddRef,
    sink_notification_Release,
};

static HRESULT create_sink_notification(MFTIME system_time, struct clock_state_change_param param,
        enum clock_notification notification, IMFClockStateSink *sink, IUnknown **out)
{
    struct sink_notification *object;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IUnknown_iface.lpVtbl = &sinknotificationvtbl;
    object->refcount = 1;
    object->system_time = system_time;
    object->param = param;
    object->notification = notification;
    object->sink = sink;
    IMFClockStateSink_AddRef(object->sink);

    *out = &object->IUnknown_iface;

    return S_OK;
}

static HRESULT clock_call_state_change(MFTIME system_time, struct clock_state_change_param param,
        enum clock_notification notification, IMFClockStateSink *sink)
{
    HRESULT hr = S_OK;

    switch (notification)
    {
        case CLOCK_NOTIFY_START:
            hr = IMFClockStateSink_OnClockStart(sink, system_time, param.u.offset);
            break;
        case CLOCK_NOTIFY_STOP:
            hr = IMFClockStateSink_OnClockStop(sink, system_time);
            break;
        case CLOCK_NOTIFY_PAUSE:
            hr = IMFClockStateSink_OnClockPause(sink, system_time);
            break;
        case CLOCK_NOTIFY_RESTART:
            hr = IMFClockStateSink_OnClockRestart(sink, system_time);
            break;
        case CLOCK_NOTIFY_SET_RATE:
            /* System time source does not allow 0.0 rate, presentation clock allows it without raising errors. */
            IMFClockStateSink_OnClockSetRate(sink, system_time, param.u.rate);
            break;
        default:
            ;
    }

    return hr;
}

static HRESULT clock_change_state(struct presentation_clock *clock, enum clock_command command,
        struct clock_state_change_param param)
{
    static const BYTE state_change_is_allowed[MFCLOCK_STATE_PAUSED+1][CLOCK_CMD_MAX] =
    {   /*              S  S* P, R  */
        /* INVALID */ { 1, 1, 1, 1 },
        /* RUNNING */ { 1, 1, 1, 1 },
        /* STOPPED */ { 1, 1, 0, 1 },
        /* PAUSED  */ { 1, 1, 0, 1 },
    };
    static const MFCLOCK_STATE states[CLOCK_CMD_MAX] =
    {
        /* CLOCK_CMD_START    */ MFCLOCK_STATE_RUNNING,
        /* CLOCK_CMD_STOP     */ MFCLOCK_STATE_STOPPED,
        /* CLOCK_CMD_PAUSE    */ MFCLOCK_STATE_PAUSED,
        /* CLOCK_CMD_SET_RATE */ 0, /* Unused */
    };
    static const enum clock_notification notifications[CLOCK_CMD_MAX] =
    {
        /* CLOCK_CMD_START    */ CLOCK_NOTIFY_START,
        /* CLOCK_CMD_STOP     */ CLOCK_NOTIFY_STOP,
        /* CLOCK_CMD_PAUSE    */ CLOCK_NOTIFY_PAUSE,
        /* CLOCK_CMD_SET_RATE */ CLOCK_NOTIFY_SET_RATE,
    };
    enum clock_notification notification;
    struct clock_sink *sink;
    IUnknown *notify_object;
    IMFAsyncResult *result;
    MFTIME system_time;
    HRESULT hr;

    if (!clock->time_source)
        return MF_E_CLOCK_NO_TIME_SOURCE;

    if (command != CLOCK_CMD_SET_RATE && clock->state == states[command] && clock->state != MFCLOCK_STATE_RUNNING)
        return MF_E_CLOCK_STATE_ALREADY_SET;

    if (!state_change_is_allowed[clock->state][command])
        return MF_E_INVALIDREQUEST;

    system_time = MFGetSystemTime();

    if (command == CLOCK_CMD_START && clock->state == MFCLOCK_STATE_PAUSED &&
            param.u.offset == PRESENTATION_CURRENT_POSITION)
    {
        notification = CLOCK_NOTIFY_RESTART;
    }
    else
        notification = notifications[command];

    if (FAILED(hr = clock_call_state_change(system_time, param, notification, clock->time_source_sink)))
        return hr;

    clock->state = states[command];

    LIST_FOR_EACH_ENTRY(sink, &clock->sinks, struct clock_sink, entry)
    {
        if (SUCCEEDED(create_sink_notification(system_time, param, notification, sink->state_sink, &notify_object)))
        {
            hr = MFCreateAsyncResult(notify_object, &clock->IMFAsyncCallback_iface, NULL, &result);
            IUnknown_Release(notify_object);
            if (SUCCEEDED(hr))
            {
                MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_STANDARD, result);
                IMFAsyncResult_Release(result);
            }
        }
    }

    return S_OK;
}

static HRESULT WINAPI present_clock_Start(IMFPresentationClock *iface, LONGLONG start_offset)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_state_change_param param = {{0}};
    HRESULT hr;

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(start_offset));

    EnterCriticalSection(&clock->cs);
    param.u.offset = start_offset;
    hr = clock_change_state(clock, CLOCK_CMD_START, param);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_Stop(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_state_change_param param = {{0}};
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_STOP, param);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_Pause(IMFPresentationClock *iface)
{
    struct presentation_clock *clock = impl_from_IMFPresentationClock(iface);
    struct clock_state_change_param param = {{0}};
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&clock->cs);
    hr = clock_change_state(clock, CLOCK_CMD_PAUSE, param);
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static const IMFPresentationClockVtbl presentationclockvtbl =
{
    present_clock_QueryInterface,
    present_clock_AddRef,
    present_clock_Release,
    present_clock_GetClockCharacteristics,
    present_clock_GetCorrelatedTime,
    present_clock_GetContinuityKey,
    present_clock_GetState,
    present_clock_GetProperties,
    present_clock_SetTimeSource,
    present_clock_GetTimeSource,
    present_clock_GetTime,
    present_clock_AddClockStateSink,
    present_clock_RemoveClockStateSink,
    present_clock_Start,
    present_clock_Stop,
    present_clock_Pause,
};

static HRESULT WINAPI present_clock_rate_control_QueryInterface(IMFRateControl *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    return IMFPresentationClock_QueryInterface(&clock->IMFPresentationClock_iface, riid, out);
}

static ULONG WINAPI present_clock_rate_control_AddRef(IMFRateControl *iface)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_rate_control_Release(IMFRateControl *iface)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_rate_SetRate(IMFRateControl *iface, BOOL thin, float rate)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);
    struct clock_state_change_param param;
    HRESULT hr;

    TRACE("%p, %d, %f.\n", iface, thin, rate);

    if (thin)
        return MF_E_THINNING_UNSUPPORTED;

    EnterCriticalSection(&clock->cs);
    param.u.rate = rate;
    if (SUCCEEDED(hr = clock_change_state(clock, CLOCK_CMD_SET_RATE, param)))
        clock->rate = rate;
    LeaveCriticalSection(&clock->cs);

    return hr;
}

static HRESULT WINAPI present_clock_rate_GetRate(IMFRateControl *iface, BOOL *thin, float *rate)
{
    struct presentation_clock *clock = impl_from_IMFRateControl(iface);

    TRACE("%p, %p, %p.\n", iface, thin, rate);

    if (!rate)
        return E_INVALIDARG;

    if (thin)
        *thin = FALSE;

    EnterCriticalSection(&clock->cs);
    *rate = clock->rate;
    LeaveCriticalSection(&clock->cs);

    return S_OK;
}

static const IMFRateControlVtbl presentclockratecontrolvtbl =
{
    present_clock_rate_control_QueryInterface,
    present_clock_rate_control_AddRef,
    present_clock_rate_control_Release,
    present_clock_rate_SetRate,
    present_clock_rate_GetRate,
};

static HRESULT WINAPI present_clock_timer_QueryInterface(IMFTimer *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    return IMFPresentationClock_QueryInterface(&clock->IMFPresentationClock_iface, riid, out);
}

static ULONG WINAPI present_clock_timer_AddRef(IMFTimer *iface)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_timer_Release(IMFTimer *iface)
{
    struct presentation_clock *clock = impl_from_IMFTimer(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_timer_SetTimer(IMFTimer *iface, DWORD flags, LONGLONG time,
        IMFAsyncCallback *callback, IUnknown *state, IUnknown **cancel_key)
{
    FIXME("%p, %#x, %s, %p, %p, %p.\n", iface, flags, wine_dbgstr_longlong(time), callback, state, cancel_key);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_timer_CancelTimer(IMFTimer *iface, IUnknown *cancel_key)
{
    FIXME("%p, %p.\n", iface, cancel_key);

    return E_NOTIMPL;
}

static const IMFTimerVtbl presentclocktimervtbl =
{
    present_clock_timer_QueryInterface,
    present_clock_timer_AddRef,
    present_clock_timer_Release,
    present_clock_timer_SetTimer,
    present_clock_timer_CancelTimer,
};

static HRESULT WINAPI present_clock_shutdown_QueryInterface(IMFShutdown *iface, REFIID riid, void **out)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    return IMFPresentationClock_QueryInterface(&clock->IMFPresentationClock_iface, riid, out);
}

static ULONG WINAPI present_clock_shutdown_AddRef(IMFShutdown *iface)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_shutdown_Release(IMFShutdown *iface)
{
    struct presentation_clock *clock = impl_from_IMFShutdown(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_shutdown_Shutdown(IMFShutdown *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_shutdown_GetShutdownStatus(IMFShutdown *iface, MFSHUTDOWN_STATUS *status)
{
    FIXME("%p, %p.\n", iface, status);

    return E_NOTIMPL;
}

static const IMFShutdownVtbl presentclockshutdownvtbl =
{
    present_clock_shutdown_QueryInterface,
    present_clock_shutdown_AddRef,
    present_clock_shutdown_Release,
    present_clock_shutdown_Shutdown,
    present_clock_shutdown_GetShutdownStatus,
};

static HRESULT WINAPI present_clock_sink_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", wine_dbgstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI present_clock_sink_callback_AddRef(IMFAsyncCallback *iface)
{
    struct presentation_clock *clock = impl_from_IMFAsyncCallback(iface);
    return IMFPresentationClock_AddRef(&clock->IMFPresentationClock_iface);
}

static ULONG WINAPI present_clock_sink_callback_Release(IMFAsyncCallback *iface)
{
    struct presentation_clock *clock = impl_from_IMFAsyncCallback(iface);
    return IMFPresentationClock_Release(&clock->IMFPresentationClock_iface);
}

static HRESULT WINAPI present_clock_sink_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI present_clock_sink_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sink_notification *data;
    IUnknown *object;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &object)))
        return hr;

    data = impl_from_IUnknown(object);

    clock_call_state_change(data->system_time, data->param, data->notification, data->sink);

    IUnknown_Release(object);

    return S_OK;
}

static const IMFAsyncCallbackVtbl presentclocksinkcallbackvtbl =
{
    present_clock_sink_callback_QueryInterface,
    present_clock_sink_callback_AddRef,
    present_clock_sink_callback_Release,
    present_clock_sink_callback_GetParameters,
    present_clock_sink_callback_Invoke,
};

/***********************************************************************
 *      MFCreatePresentationClock (mf.@)
 */
HRESULT WINAPI MFCreatePresentationClock(IMFPresentationClock **clock)
{
    struct presentation_clock *object;

    TRACE("%p.\n", clock);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFPresentationClock_iface.lpVtbl = &presentationclockvtbl;
    object->IMFRateControl_iface.lpVtbl = &presentclockratecontrolvtbl;
    object->IMFTimer_iface.lpVtbl = &presentclocktimervtbl;
    object->IMFShutdown_iface.lpVtbl = &presentclockshutdownvtbl;
    object->IMFAsyncCallback_iface.lpVtbl = &presentclocksinkcallbackvtbl;
    object->refcount = 1;
    list_init(&object->sinks);
    object->rate = 1.0f;
    InitializeCriticalSection(&object->cs);

    *clock = &object->IMFPresentationClock_iface;

    return S_OK;
}

static HRESULT WINAPI standard_quality_manager_QueryInterface(IMFQualityManager *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFQualityManager) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFQualityManager_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI standard_quality_manager_AddRef(IMFQualityManager *iface)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);
    ULONG refcount = InterlockedIncrement(&manager->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI standard_quality_manager_Release(IMFQualityManager *iface)
{
    struct quality_manager *manager = impl_from_IMFQualityManager(iface);
    ULONG refcount = InterlockedDecrement(&manager->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(manager);
    }

    return refcount;
}

static HRESULT WINAPI standard_quality_manager_NotifyTopology(IMFQualityManager *iface, IMFTopology *topology)
{
    FIXME("%p, %p stub.\n", iface, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_NotifyPresentationClock(IMFQualityManager *iface,
        IMFPresentationClock *clock)
{
    FIXME("%p, %p stub.\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_NotifyProcessInput(IMFQualityManager *iface, IMFTopologyNode *node,
        LONG input_index, IMFSample *sample)
{
    FIXME("%p, %p, %d, %p stub.\n", iface, node, input_index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_NotifyProcessOutput(IMFQualityManager *iface, IMFTopologyNode *node,
        LONG output_index, IMFSample *sample)
{
    FIXME("%p, %p, %d, %p stub.\n", iface, node, output_index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_NotifyQualityEvent(IMFQualityManager *iface, IUnknown *object,
        IMFMediaEvent *event)
{
    FIXME("%p, %p, %p stub.\n", iface, object, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI standard_quality_manager_Shutdown(IMFQualityManager *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static IMFQualityManagerVtbl standard_quality_manager_vtbl =
{
    standard_quality_manager_QueryInterface,
    standard_quality_manager_AddRef,
    standard_quality_manager_Release,
    standard_quality_manager_NotifyTopology,
    standard_quality_manager_NotifyPresentationClock,
    standard_quality_manager_NotifyProcessInput,
    standard_quality_manager_NotifyProcessOutput,
    standard_quality_manager_NotifyQualityEvent,
    standard_quality_manager_Shutdown,
};

HRESULT WINAPI MFCreateStandardQualityManager(IMFQualityManager **manager)
{
    struct quality_manager *object;

    TRACE("%p.\n", manager);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFQualityManager_iface.lpVtbl = &standard_quality_manager_vtbl;
    object->refcount = 1;

    *manager = &object->IMFQualityManager_iface;

    return S_OK;
}
