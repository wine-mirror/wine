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
#include <math.h>
#include <float.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "mfidl.h"
#include "evr.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "mf_private.h"

#include "initguid.h"

DEFINE_GUID(_MF_TOPONODE_IMFActivate, 0x33706f4a, 0x309a, 0x49be, 0xa8, 0xdd, 0xe7, 0xc0, 0x87, 0x5e, 0xb6, 0x79);

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum session_command
{
    SESSION_CMD_CLEAR_TOPOLOGIES,
    SESSION_CMD_CLOSE,
    SESSION_CMD_SET_TOPOLOGY,
    SESSION_CMD_START,
    SESSION_CMD_PAUSE,
    SESSION_CMD_STOP,
    SESSION_CMD_SET_RATE,
    SESSION_CMD_SHUTDOWN,
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
        struct
        {
            BOOL thin;
            float rate;
        } set_rate;
        struct
        {
            IMFTopology *topology;
        } notify_topology;
    };
    struct list entry;
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
    SESSION_STATE_STARTING_SOURCES,
    SESSION_STATE_PREROLLING_SINKS,
    SESSION_STATE_STARTING_SINKS,
    SESSION_STATE_RESTARTING_SOURCES,
    SESSION_STATE_STARTED,
    SESSION_STATE_PAUSING_SINKS,
    SESSION_STATE_PAUSING_SOURCES,
    SESSION_STATE_PAUSED,
    SESSION_STATE_STOPPING_SINKS,
    SESSION_STATE_STOPPING_SOURCES,
    SESSION_STATE_FINALIZING_SINKS,
    SESSION_STATE_CLOSED,
    SESSION_STATE_SHUT_DOWN,
};

enum object_state
{
    OBJ_STATE_STOPPED = 0,
    OBJ_STATE_STARTED,
    OBJ_STATE_PAUSED,
    OBJ_STATE_PREROLLED,
    OBJ_STATE_INVALID,
};

enum media_source_flags
{
    SOURCE_FLAG_END_OF_PRESENTATION = 0x1,
};

struct media_source
{
    struct list entry;
    union
    {
        IMFMediaSource *source;
        IUnknown *object;
    };
    IMFPresentationDescriptor *pd;
    enum object_state state;
    unsigned int flags;
};

struct media_sink
{
    struct list entry;
    union
    {
        IMFMediaSink *sink;
        IUnknown *object;
    };
    IMFMediaSinkPreroll *preroll;
    IMFMediaEventGenerator *event_generator;
    BOOL finalized;
};

struct sample
{
    struct list entry;
    IMFSample *sample;
};

struct transform_stream
{
    struct list samples;
    unsigned int requests;
    unsigned int min_buffer_size;
    BOOL draining;
};

enum topo_node_flags
{
    TOPO_NODE_END_OF_STREAM = 0x1,
    TOPO_NODE_SCRUB_SAMPLE_COMPLETE = 0x2,
};

struct topo_node
{
    struct list entry;
    struct media_session *session;
    MF_TOPOLOGY_TYPE type;
    TOPOID node_id;
    IMFTopologyNode *node;
    enum object_state state;
    unsigned int flags;
    union
    {
        IMFMediaStream *source_stream;
        IMFStreamSink *sink_stream;
        IMFTransform *transform;
        IUnknown *object;
    } object;

    union
    {
        struct
        {
            IMFMediaSource *source;
            DWORD stream_id;
        } source;
        struct
        {
            unsigned int requests;
            IMFVideoSampleAllocatorNotify notify_cb;
            IMFVideoSampleAllocator *allocator;
            IMFVideoSampleAllocatorCallback *allocator_cb;
        } sink;
        struct
        {
            struct transform_stream *inputs;
            DWORD *input_map;
            unsigned int input_count;
            unsigned int next_input;

            struct transform_stream *outputs;
            DWORD *output_map;
            unsigned int output_count;
        } transform;
    } u;
};

enum presentation_flags
{
    SESSION_FLAG_SOURCES_SUBSCRIBED = 0x1,
    SESSION_FLAG_PRESENTATION_CLOCK_SET = 0x2,
    SESSION_FLAG_FINALIZE_SINKS = 0x4,
    SESSION_FLAG_NEEDS_PREROLL = 0x8,
    SESSION_FLAG_END_OF_PRESENTATION = 0x10,
    SESSION_FLAG_PENDING_RATE_CHANGE = 0x20,
    SESSION_FLAG_PENDING_COMMAND = 0x40,
};

struct media_session
{
    IMFMediaSession IMFMediaSession_iface;
    IMFGetService IMFGetService_iface;
    IMFRateSupport IMFRateSupport_iface;
    IMFRateControl IMFRateControl_iface;
    IMFTopologyNodeAttributeEditor IMFTopologyNodeAttributeEditor_iface;
    IMFAsyncCallback commands_callback;
    IMFAsyncCallback sa_ready_callback;
    IMFAsyncCallback events_callback;
    IMFAsyncCallback sink_finalizer_callback;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
    IMFPresentationClock *clock;
    IMFPresentationTimeSource *system_time_source;
    IMFRateControl *clock_rate_control;
    IMFTopoLoader *topo_loader;
    IMFQualityManager *quality_manager;
    struct
    {
        IMFTopology *current_topology;
        MF_TOPOSTATUS topo_status;
        MFTIME clock_stop_time;
        unsigned int flags;
        struct list sources;
        struct list sinks;
        struct list nodes;

        /* Latest Start() arguments. */
        GUID time_format;
        PROPVARIANT start_position;
        /* Latest SetRate() arguments. */
        BOOL thin;
        float rate;
    } presentation;
    struct list topologies;
    struct list commands;
    enum session_state state;
    DWORD caps;
    CRITICAL_SECTION cs;
};

static inline struct media_session *impl_from_IMFMediaSession(IMFMediaSession *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFMediaSession_iface);
}

static struct media_session *impl_from_commands_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, commands_callback);
}

static struct media_session *impl_from_sa_ready_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, sa_ready_callback);
}

static struct media_session *impl_from_events_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, events_callback);
}

static struct media_session *impl_from_sink_finalizer_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, sink_finalizer_callback);
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

static struct media_session *impl_session_from_IMFTopologyNodeAttributeEditor(IMFTopologyNodeAttributeEditor *iface)
{
    return CONTAINING_RECORD(iface, struct media_session, IMFTopologyNodeAttributeEditor_iface);
}

static struct session_op *impl_op_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct session_op, IUnknown_iface);
}

static struct topo_node *impl_node_from_IMFVideoSampleAllocatorNotify(IMFVideoSampleAllocatorNotify *iface)
{
    return CONTAINING_RECORD(iface, struct topo_node, u.sink.notify_cb);
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

    TRACE("%p, %p, %lu.\n", iface, info, count);

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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI session_op_Release(IUnknown *iface)
{
    struct session_op *op = impl_op_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&op->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        switch (op->command)
        {
            case SESSION_CMD_SET_TOPOLOGY:
                if (op->set_topology.topology)
                    IMFTopology_Release(op->set_topology.topology);
                break;
            case SESSION_CMD_START:
                PropVariantClear(&op->start.start_position);
                break;
            default:
                ;
        }
        free(op);
    }

    return refcount;
}

static const IUnknownVtbl session_op_vtbl =
{
    session_op_QueryInterface,
    session_op_AddRef,
    session_op_Release,
};

static HRESULT create_session_op(enum session_command command, struct session_op **ret)
{
    struct session_op *op;

    if (!(op = calloc(1, sizeof(*op))))
        return E_OUTOFMEMORY;

    op->IUnknown_iface.lpVtbl = &session_op_vtbl;
    op->refcount = 1;
    op->command = command;

    *ret = op;

    return S_OK;
}

static HRESULT session_is_shut_down(struct media_session *session)
{
    return session->state == SESSION_STATE_SHUT_DOWN ? MF_E_SHUTDOWN : S_OK;
}

static HRESULT session_submit_command(struct media_session *session, struct session_op *op)
{
    HRESULT hr;

    TRACE("session %p, op %p, command %u.\n", session, op, op->command);

    EnterCriticalSection(&session->cs);
    if (SUCCEEDED(hr = session_is_shut_down(session)))
    {
        if (list_empty(&session->commands) && !(session->presentation.flags & SESSION_FLAG_PENDING_COMMAND))
            hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &session->commands_callback, &op->IUnknown_iface);
        if (op->command == SESSION_CMD_SHUTDOWN)
            list_add_head(&session->commands, &op->entry);
        else
            list_add_tail(&session->commands, &op->entry);
        IUnknown_AddRef(&op->IUnknown_iface);
    }
    LeaveCriticalSection(&session->cs);

    return hr;
}

static HRESULT session_submit_simple_command(struct media_session *session, enum session_command command)
{
    struct session_op *op;
    HRESULT hr;

    if (FAILED(hr = create_session_op(command, &op)))
        return hr;

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);
    return hr;
}

static void session_clear_queued_topologies(struct media_session *session)
{
    struct queued_topology *ptr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(ptr, next, &session->topologies, struct queued_topology, entry)
    {
        list_remove(&ptr->entry);
        IMFTopology_Release(ptr->topology);
        free(ptr);
    }
}

static void session_set_topo_status(struct media_session *session, HRESULT status,
        MF_TOPOSTATUS topo_status)
{
    IMFMediaEvent *event;
    PROPVARIANT param;

    if (topo_status == MF_TOPOSTATUS_INVALID)
        return;

    if (list_empty(&session->topologies))
    {
        FIXME("Unexpectedly empty topology queue.\n");
        return;
    }

    if (topo_status > session->presentation.topo_status)
    {
        struct queued_topology *topology = LIST_ENTRY(list_head(&session->topologies), struct queued_topology, entry);

        param.vt = VT_UNKNOWN;
        param.punkVal = (IUnknown *)topology->topology;

        if (FAILED(MFCreateMediaEvent(MESessionTopologyStatus, &GUID_NULL, status, &param, &event)))
            return;

        session->presentation.topo_status = topo_status;

        IMFMediaEvent_SetUINT32(event, &MF_EVENT_TOPOLOGY_STATUS, topo_status);
        IMFMediaEventQueue_QueueEvent(session->event_queue, event);
        IMFMediaEvent_Release(event);
    }
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

                    if (SUCCEEDED(hr))
                        IMFTopologyNode_SetUnknown(node, &_MF_TOPONODE_IMFActivate, (IUnknown *)activate);

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

static HRESULT session_init_media_types(IMFTopology *topology)
{
    MF_TOPOLOGY_TYPE node_type;
    WORD node_count, i, j;
    IMFTopologyNode *node;
    IMFMediaType *type;
    DWORD input_count;
    HRESULT hr;

    if (FAILED(hr = IMFTopology_GetNodeCount(topology, &node_count)))
        return hr;

    for (i = 0; i < node_count; ++i)
    {
        if (FAILED(hr = IMFTopology_GetNode(topology, i, &node)))
            break;

        if (FAILED(hr = IMFTopologyNode_GetInputCount(node, &input_count))
                || FAILED(hr = IMFTopologyNode_GetNodeType(node, &node_type))
                || node_type != MF_TOPOLOGY_OUTPUT_NODE)
        {
            IMFTopologyNode_Release(node);
            continue;
        }

        for (j = 0; j < input_count; ++j)
        {
            IMFMediaTypeHandler *handler;
            IMFTopologyNode *up_node;
            DWORD up_output;

            if (SUCCEEDED(hr = IMFTopologyNode_GetInput(node, j, &up_node, &up_output)))
            {
                hr = topology_node_init_media_type(up_node, up_output, TRUE, &type);
                IMFTopologyNode_Release(up_node);
            }
            if (FAILED(hr))
                break;

            if (SUCCEEDED(hr = topology_node_get_type_handler(node, j, FALSE, &handler)))
            {
                hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, type);
                IMFMediaTypeHandler_Release(handler);
            }

            IMFMediaType_Release(type);
        }

        IMFTopologyNode_Release(node);
    }

    return hr;
}

static void session_set_caps(struct media_session *session, DWORD caps)
{
    DWORD delta = session->caps ^ caps;
    IMFMediaEvent *event;

    /* Delta is documented to reflect rate value changes as well, but it's not clear what to compare
       them to, since session always queries for current object rates. */
    if (!delta)
        return;

    session->caps = caps;

    if (FAILED(MFCreateMediaEvent(MESessionCapabilitiesChanged, &GUID_NULL, S_OK, NULL, &event)))
        return;

    IMFMediaEvent_SetUINT32(event, &MF_EVENT_SESSIONCAPS, caps);
    IMFMediaEvent_SetUINT32(event, &MF_EVENT_SESSIONCAPS_DELTA, delta);

    IMFMediaEventQueue_QueueEvent(session->event_queue, event);
    IMFMediaEvent_Release(event);
}

static HRESULT transform_stream_push_sample(struct transform_stream *stream, IMFSample *sample)
{
    struct sample *entry;

    if (!(entry = calloc(1, sizeof(*entry))))
        return E_OUTOFMEMORY;

    entry->sample = sample;
    IMFSample_AddRef(entry->sample);

    list_add_tail(&stream->samples, &entry->entry);
    return S_OK;
}

static HRESULT transform_stream_pop_sample(struct transform_stream *stream, IMFSample **sample)
{
    struct sample *entry;
    struct list *ptr;

    if (!(ptr = list_head(&stream->samples)))
        return MF_E_TRANSFORM_NEED_MORE_INPUT;

    entry = LIST_ENTRY(ptr, struct sample, entry);
    list_remove(&entry->entry);
    *sample = entry->sample;
    free(entry);
    return S_OK;
}

static void transform_stream_drop_samples(struct transform_stream *stream)
{
    IMFSample *sample;

    while (SUCCEEDED(transform_stream_pop_sample(stream, &sample)))
        IMFSample_Release(sample);
}

static void release_topo_node(struct topo_node *node)
{
    unsigned int i;

    switch (node->type)
    {
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            if (node->u.source.source)
                IMFMediaSource_Release(node->u.source.source);
            break;
        case MF_TOPOLOGY_TRANSFORM_NODE:
            for (i = 0; i < node->u.transform.input_count; ++i)
                transform_stream_drop_samples(&node->u.transform.inputs[i]);
            for (i = 0; i < node->u.transform.output_count; ++i)
                transform_stream_drop_samples(&node->u.transform.outputs[i]);
            free(node->u.transform.inputs);
            free(node->u.transform.outputs);
            free(node->u.transform.input_map);
            free(node->u.transform.output_map);
            break;
        case MF_TOPOLOGY_OUTPUT_NODE:
            if (node->u.sink.allocator)
                IMFVideoSampleAllocator_Release(node->u.sink.allocator);
            if (node->u.sink.allocator_cb)
            {
                IMFVideoSampleAllocatorCallback_SetCallback(node->u.sink.allocator_cb, NULL);
                IMFVideoSampleAllocatorCallback_Release(node->u.sink.allocator_cb);
            }
            break;
        default:
            ;
    }

    if (node->object.object)
        IUnknown_Release(node->object.object);
    if (node->node)
        IMFTopologyNode_Release(node->node);
    free(node);
}

static void session_shutdown_current_topology(struct media_session *session)
{
    unsigned int shutdown, force_shutdown;
    MF_TOPOLOGY_TYPE node_type;
    IMFStreamSink *stream_sink;
    IMFTopology *topology;
    IMFTopologyNode *node;
    IMFActivate *activate;
    IMFMediaSink *sink;
    WORD idx = 0;
    HRESULT hr;

    topology = session->presentation.current_topology;
    force_shutdown = session->state == SESSION_STATE_SHUT_DOWN;

    /* FIXME: should handle async MFTs, but these are not supported by the rest of the pipeline currently. */

    while (SUCCEEDED(IMFTopology_GetNode(topology, idx++, &node)))
    {
        if (SUCCEEDED(IMFTopologyNode_GetNodeType(node, &node_type)) &&
                node_type == MF_TOPOLOGY_OUTPUT_NODE)
        {
            shutdown = 1;
            IMFTopologyNode_GetUINT32(node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, &shutdown);

            if (force_shutdown || shutdown)
            {
                if (SUCCEEDED(IMFTopologyNode_GetUnknown(node, &_MF_TOPONODE_IMFActivate, &IID_IMFActivate,
                        (void **)&activate)))
                {
                    if (FAILED(hr = IMFActivate_ShutdownObject(activate)))
                        WARN("Failed to shut down activation object for the sink, hr %#lx.\n", hr);
                    IMFActivate_Release(activate);
                }
                else if (SUCCEEDED(topology_node_get_object(node, &IID_IMFStreamSink, (void **)&stream_sink)))
                {
                    if (SUCCEEDED(IMFStreamSink_GetMediaSink(stream_sink, &sink)))
                    {
                        IMFMediaSink_Shutdown(sink);
                        IMFMediaSink_Release(sink);
                    }

                    IMFStreamSink_Release(stream_sink);
                }
            }
        }

        IMFTopologyNode_Release(node);
    }
}

static void session_clear_command_list(struct media_session *session)
{
    struct session_op *op, *op2;

    LIST_FOR_EACH_ENTRY_SAFE(op, op2, &session->commands, struct session_op, entry)
    {
        list_remove(&op->entry);
        IUnknown_Release(&op->IUnknown_iface);
    }
}

static void session_clear_presentation(struct media_session *session)
{
    struct media_source *source, *source2;
    struct media_sink *sink, *sink2;
    struct topo_node *node, *node2;

    session_shutdown_current_topology(session);

    IMFTopology_Clear(session->presentation.current_topology);
    session->presentation.topo_status = MF_TOPOSTATUS_INVALID;
    session->presentation.flags = 0;

    LIST_FOR_EACH_ENTRY_SAFE(source, source2, &session->presentation.sources, struct media_source, entry)
    {
        list_remove(&source->entry);
        if (source->source)
            IMFMediaSource_Release(source->source);
        if (source->pd)
            IMFPresentationDescriptor_Release(source->pd);
        free(source);
    }

    LIST_FOR_EACH_ENTRY_SAFE(node, node2, &session->presentation.nodes, struct topo_node, entry)
    {
        list_remove(&node->entry);
        release_topo_node(node);
    }

    LIST_FOR_EACH_ENTRY_SAFE(sink, sink2, &session->presentation.sinks, struct media_sink, entry)
    {
        list_remove(&sink->entry);

        if (sink->sink)
            IMFMediaSink_Release(sink->sink);
        if (sink->preroll)
            IMFMediaSinkPreroll_Release(sink->preroll);
        if (sink->event_generator)
            IMFMediaEventGenerator_Release(sink->event_generator);
        free(sink);
    }
}

static struct topo_node *session_get_node_by_id(const struct media_session *session, TOPOID id)
{
    struct topo_node *node;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->node_id == id)
            return node;
    }

    return NULL;
}

static void session_command_complete(struct media_session *session)
{
    struct session_op *op;
    struct list *e;

    session->presentation.flags &= ~SESSION_FLAG_PENDING_COMMAND;

    /* Submit next command. */
    if ((e = list_head(&session->commands)))
    {
        op = LIST_ENTRY(e, struct session_op, entry);
        MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &session->commands_callback, &op->IUnknown_iface);
    }
}

static void session_command_complete_with_event(struct media_session *session, MediaEventType event,
        HRESULT status, const PROPVARIANT *param)
{
    IMFMediaEventQueue_QueueEventParamVar(session->event_queue, event, &GUID_NULL, status, param);
    session_command_complete(session);
}

static HRESULT session_subscribe_sources(struct media_session *session)
{
    struct media_source *source;
    HRESULT hr = S_OK;

    if (session->presentation.flags & SESSION_FLAG_SOURCES_SUBSCRIBED)
        return hr;

    LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
    {
        if (FAILED(hr = IMFMediaSource_BeginGetEvent(source->source, &session->events_callback,
                source->object)))
        {
            WARN("Failed to subscribe to source events, hr %#lx.\n", hr);
            return hr;
        }
    }

    session->presentation.flags |= SESSION_FLAG_SOURCES_SUBSCRIBED;
    return hr;
}

static void session_flush_nodes(struct media_session *session)
{
    struct topo_node *node;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_OUTPUT_NODE)
            IMFStreamSink_Flush(node->object.sink_stream);
        else if (node->type == MF_TOPOLOGY_TRANSFORM_NODE)
            IMFTransform_ProcessMessage(node->object.transform, MFT_MESSAGE_COMMAND_FLUSH, 0);
    }
}

static void session_start(struct media_session *session, const GUID *time_format, const PROPVARIANT *start_position)
{
    struct media_source *source;
    struct topo_node *topo_node;
    MFTIME duration;
    HRESULT hr;
    UINT i;

    switch (session->state)
    {
        case SESSION_STATE_STOPPED:

            /* Start request with no current topology. */
            if (session->presentation.topo_status == MF_TOPOSTATUS_INVALID)
            {
                session_command_complete_with_event(session, MESessionStarted, MF_E_INVALIDREQUEST, NULL);
                break;
            }

            /* fallthrough */
        case SESSION_STATE_PAUSED:

            session->presentation.time_format = *time_format;
            session->presentation.start_position.vt = VT_EMPTY;
            PropVariantCopy(&session->presentation.start_position, start_position);

            if (FAILED(hr = session_subscribe_sources(session)))
            {
                session_command_complete_with_event(session, MESessionStarted, hr, NULL);
                return;
            }

            LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
            {
                if (FAILED(hr = IMFMediaSource_Start(source->source, source->pd, &GUID_NULL, start_position)))
                {
                    WARN("Failed to start media source %p, hr %#lx.\n", source->source, hr);
                    session_command_complete_with_event(session, MESessionStarted, hr, NULL);
                    return;
                }
            }

            LIST_FOR_EACH_ENTRY(topo_node, &session->presentation.nodes, struct topo_node, entry)
            {
                if (topo_node->type == MF_TOPOLOGY_TRANSFORM_NODE)
                {
                    for (i = 0; i < topo_node->u.transform.input_count; i++)
                    {
                        struct transform_stream *stream = &topo_node->u.transform.inputs[i];
                        stream->draining = FALSE;
                    }
                }
            }

            session->state = SESSION_STATE_STARTING_SOURCES;
            break;
        case SESSION_STATE_STARTED:
            /* Check for invalid positions */
            LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
            {
                hr = IMFPresentationDescriptor_GetUINT64(source->pd, &MF_PD_DURATION, (UINT64 *)&duration);
                if (SUCCEEDED(hr) && IsEqualGUID(time_format, &GUID_NULL)
                        && start_position->vt == VT_I8 && start_position->hVal.QuadPart > duration)
                {
                    WARN("Start position %s out of range, hr %#lx.\n", wine_dbgstr_longlong(start_position->hVal.QuadPart), hr);
                    session_command_complete_with_event(session, MESessionStarted, MF_E_INVALID_POSITION, NULL);
                    return;
                }
            }

            /* Stop sources */
            LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
            {
                if (FAILED(hr = IMFMediaSource_Stop(source->source)))
                {
                    WARN("Failed to stop media source %p, hr %#lx.\n", source->source, hr);
                    session_command_complete_with_event(session, MESessionStarted, hr, NULL);
                    return;
                }
            }

            session->presentation.time_format = *time_format;
            session->presentation.start_position.vt = VT_EMPTY;
            PropVariantCopy(&session->presentation.start_position, start_position);

            /* SESSION_STATE_STARTED -> SESSION_STATE_RESTARTING_SOURCES -> SESSION_STATE_STARTED */
            session->state = SESSION_STATE_RESTARTING_SOURCES;
            break;
        default:
            session_command_complete_with_event(session, MESessionStarted, MF_E_INVALIDREQUEST, NULL);
            break;
    }
}

static void session_set_started(struct media_session *session)
{
    struct media_source *source;
    IMFMediaEvent *event;
    unsigned int caps;
    DWORD flags;

    session->state = SESSION_STATE_STARTED;

    caps = session->caps | MFSESSIONCAP_PAUSE;

    LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
    {
        if (SUCCEEDED(IMFMediaSource_GetCharacteristics(source->source, &flags)))
        {
            if (!(flags & MFMEDIASOURCE_CAN_PAUSE))
            {
                caps &= ~MFSESSIONCAP_PAUSE;
                break;
            }
        }
    }

    session_set_caps(session, caps);

    if (SUCCEEDED(MFCreateMediaEvent(MESessionStarted, &GUID_NULL, S_OK, NULL, &event)))
    {
        IMFMediaEvent_SetUINT64(event, &MF_EVENT_PRESENTATION_TIME_OFFSET, 0);
        IMFMediaEventQueue_QueueEvent(session->event_queue, event);
        IMFMediaEvent_Release(event);
    }
    session_command_complete(session);
}

static void session_set_paused(struct media_session *session, unsigned int state, HRESULT status)
{
    /* Failed event status could indicate a failure during normal transition to paused state,
       or an attempt to pause from invalid initial state. To finalize failed transition in the former case,
       state is still forced to PAUSED, otherwise previous state is retained. */
    if (state != ~0u) session->state = state;
    if (SUCCEEDED(status))
        session_set_caps(session, session->caps & ~MFSESSIONCAP_PAUSE);
    session_command_complete_with_event(session, MESessionPaused, status, NULL);
}

static void session_set_closed(struct media_session *session, HRESULT status)
{
    session->state = SESSION_STATE_CLOSED;
    if (SUCCEEDED(status))
        session_set_caps(session, session->caps & ~(MFSESSIONCAP_START | MFSESSIONCAP_SEEK));
    session_command_complete_with_event(session, MESessionClosed, status, NULL);
}

static void session_pause(struct media_session *session)
{
    unsigned int state = ~0u;
    HRESULT hr;

    switch (session->state)
    {
        case SESSION_STATE_STARTED:

            /* Transition in two steps - pause the clock, wait for sinks, then pause sources. */
            if (SUCCEEDED(hr = IMFPresentationClock_Pause(session->clock)))
                session->state = SESSION_STATE_PAUSING_SINKS;
            state = SESSION_STATE_PAUSED;

            break;

        case SESSION_STATE_STOPPED:
            hr = MF_E_SESSION_PAUSEWHILESTOPPED;
            break;
        default:
            hr = MF_E_INVALIDREQUEST;
    }

    if (FAILED(hr))
        session_set_paused(session, state, hr);
}

static void session_clear_end_of_presentation(struct media_session *session)
{
    struct media_source *source;
    struct topo_node *node;

    session->presentation.flags &= ~SESSION_FLAG_END_OF_PRESENTATION;
    LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
    {
        source->flags &= ~SOURCE_FLAG_END_OF_PRESENTATION;
    }
    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        node->flags &= ~TOPO_NODE_END_OF_STREAM;
    }
    session->presentation.topo_status = MF_TOPOSTATUS_READY;
}

static void session_set_stopped(struct media_session *session, HRESULT status)
{
    MediaEventType event_type;
    IMFMediaEvent *event;

    session->state = SESSION_STATE_STOPPED;
    event_type = session->presentation.flags & SESSION_FLAG_END_OF_PRESENTATION ? MESessionEnded : MESessionStopped;

    if (SUCCEEDED(MFCreateMediaEvent(event_type, &GUID_NULL, status, NULL, &event)))
    {
        IMFMediaEvent_SetUINT64(event, &MF_SESSION_APPROX_EVENT_OCCURRENCE_TIME, session->presentation.clock_stop_time);
        IMFMediaEventQueue_QueueEvent(session->event_queue, event);
        IMFMediaEvent_Release(event);
    }
    session_clear_end_of_presentation(session);
    session_command_complete(session);
}

static void session_stop(struct media_session *session)
{
    HRESULT hr = MF_E_INVALIDREQUEST;

    switch (session->state)
    {
        case SESSION_STATE_STARTED:
        case SESSION_STATE_PAUSED:

            /* Transition in two steps - stop the clock, wait for sinks, then stop sources. */
            IMFPresentationClock_GetTime(session->clock, &session->presentation.clock_stop_time);
            if (SUCCEEDED(hr = IMFPresentationClock_Stop(session->clock)))
                session->state = SESSION_STATE_STOPPING_SINKS;
            else
                session_set_stopped(session, hr);

            break;
        case SESSION_STATE_STOPPED:
            hr = S_OK;
            /* fallthrough */
        default:
            session_command_complete_with_event(session, MESessionStopped, hr, NULL);
            break;
    }
}

static HRESULT session_finalize_sinks(struct media_session *session)
{
    IMFFinalizableMediaSink *fin_sink;
    BOOL sinks_finalized = TRUE;
    struct media_sink *sink;
    HRESULT hr = S_OK;

    session->presentation.flags &= ~SESSION_FLAG_FINALIZE_SINKS;
    session->state = SESSION_STATE_FINALIZING_SINKS;

    LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
    {
        if (SUCCEEDED(IMFMediaSink_QueryInterface(sink->sink, &IID_IMFFinalizableMediaSink, (void **)&fin_sink)))
        {
            hr = IMFFinalizableMediaSink_BeginFinalize(fin_sink, &session->sink_finalizer_callback,
                    (IUnknown *)fin_sink);
            IMFFinalizableMediaSink_Release(fin_sink);
            if (FAILED(hr))
                break;
            sinks_finalized = FALSE;
        }
        else
            sink->finalized = TRUE;
    }

    if (sinks_finalized)
        session_set_closed(session, hr);

    return hr;
}

static void session_close(struct media_session *session)
{
    HRESULT hr = S_OK;

    switch (session->state)
    {
        case SESSION_STATE_STOPPED:
            hr = session_finalize_sinks(session);
            break;
        case SESSION_STATE_STARTED:
        case SESSION_STATE_PAUSED:
            session->presentation.flags |= SESSION_FLAG_FINALIZE_SINKS;
            if (SUCCEEDED(hr = IMFPresentationClock_Stop(session->clock)))
                session->state = SESSION_STATE_STOPPING_SINKS;
            break;
        default:
            hr = MF_E_INVALIDREQUEST;
            break;
    }

    session_clear_queued_topologies(session);
    if (FAILED(hr))
        session_set_closed(session, hr);
}

static void session_clear_topologies(struct media_session *session)
{
    HRESULT hr = S_OK;

    if (session->state == SESSION_STATE_CLOSED)
        hr = MF_E_INVALIDREQUEST;
    else
        session_clear_queued_topologies(session);
    session_command_complete_with_event(session, MESessionTopologiesCleared, hr, NULL);
}

static HRESULT session_is_presentation_rate_supported(struct media_session *session, BOOL thin, float rate,
        float *nearest_rate)
{
    IMFRateSupport *rate_support;
    struct media_source *source;
    struct media_sink *sink;
    float value = 0.0f, tmp;
    HRESULT hr = S_OK;
    DWORD flags;

    if (!nearest_rate) nearest_rate = &tmp;

    if (rate == 0.0f)
    {
        *nearest_rate = 1.0f;
        return S_OK;
    }

    if (session->presentation.topo_status != MF_TOPOSTATUS_INVALID)
    {
        LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
        {
            if (FAILED(hr = MFGetService(source->object, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport,
                    (void **)&rate_support)))
            {
                value = 1.0f;
                break;
            }

            value = rate;
            if (FAILED(hr = IMFRateSupport_IsRateSupported(rate_support, thin, rate, &value)))
                WARN("Source does not support rate %f, hr %#lx.\n", rate, hr);
            IMFRateSupport_Release(rate_support);

            /* Only "first" source is considered. */
            break;
        }

        if (SUCCEEDED(hr))
        {
            /* For sinks only check if rate is supported, ignoring nearest values. */
            LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
            {
                flags = 0;
                if (FAILED(hr = IMFMediaSink_GetCharacteristics(sink->sink, &flags)))
                    break;

                if (flags & MEDIASINK_RATELESS)
                    continue;

                if (FAILED(MFGetService(sink->object, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport,
                        (void **)&rate_support)))
                    continue;

                hr = IMFRateSupport_IsRateSupported(rate_support, thin, rate, NULL);
                IMFRateSupport_Release(rate_support);
                if (FAILED(hr))
                {
                    WARN("Sink %p does not support rate %f, hr %#lx.\n", sink->sink, rate, hr);
                    break;
                }
            }
        }
    }

    *nearest_rate = value;

    return hr;
}

static void session_set_consumed_clock(IUnknown *object, IMFPresentationClock *clock)
{
    IMFClockConsumer *consumer;

    if (SUCCEEDED(IUnknown_QueryInterface(object, &IID_IMFClockConsumer, (void **)&consumer)))
    {
        IMFClockConsumer_SetPresentationClock(consumer, clock);
        IMFClockConsumer_Release(consumer);
    }
}

static void session_set_presentation_clock(struct media_session *session)
{
    IMFPresentationTimeSource *time_source = NULL;
    struct media_source *source;
    struct media_sink *sink;
    struct topo_node *node;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_TRANSFORM_NODE)
            IMFTransform_ProcessMessage(node->object.transform, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    }

    if (!(session->presentation.flags & SESSION_FLAG_PRESENTATION_CLOCK_SET))
    {
        /* Attempt to get time source from the sinks. */
        LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
        {
            if (SUCCEEDED(IMFMediaSink_QueryInterface(sink->sink, &IID_IMFPresentationTimeSource,
                    (void **)&time_source)))
                 break;
        }

        if (time_source)
        {
            hr = IMFPresentationClock_SetTimeSource(session->clock, time_source);
            IMFPresentationTimeSource_Release(time_source);
        }
        else
            hr = IMFPresentationClock_SetTimeSource(session->clock, session->system_time_source);

        if (FAILED(hr))
            WARN("Failed to set time source, hr %#lx.\n", hr);

        LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
        {
            if (node->type != MF_TOPOLOGY_OUTPUT_NODE)
                continue;

            if (FAILED(hr = IMFStreamSink_BeginGetEvent(node->object.sink_stream, &session->events_callback,
                    node->object.object)))
            {
                WARN("Failed to subscribe to stream sink events, hr %#lx.\n", hr);
            }
        }

        /* Set clock for all topology nodes. */
        LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
        {
            session_set_consumed_clock(source->object, session->clock);
        }

        LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
        {
            if (sink->event_generator && FAILED(hr = IMFMediaEventGenerator_BeginGetEvent(sink->event_generator,
                    &session->events_callback, (IUnknown *)sink->event_generator)))
            {
                WARN("Failed to subscribe to sink events, hr %#lx.\n", hr);
            }

            if (FAILED(hr = IMFMediaSink_SetPresentationClock(sink->sink, session->clock)))
                WARN("Failed to set presentation clock for the sink, hr %#lx.\n", hr);
        }

        LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
        {
            if (node->type != MF_TOPOLOGY_TRANSFORM_NODE)
                continue;

            session_set_consumed_clock(node->object.object, session->clock);
        }

        session->presentation.flags |= SESSION_FLAG_PRESENTATION_CLOCK_SET;
    }
}

static void session_set_rate(struct media_session *session, BOOL thin, float rate)
{
    IMFRateControl *rate_control;
    struct media_source *source;
    float clock_rate = 0.0f;
    PROPVARIANT param;
    HRESULT hr;

    hr = session_is_presentation_rate_supported(session, thin, rate, NULL);

    if (SUCCEEDED(hr))
        hr = IMFRateControl_GetRate(session->clock_rate_control, NULL, &clock_rate);

    if (SUCCEEDED(hr) && (rate != clock_rate) && SUCCEEDED(hr = session_subscribe_sources(session)))
    {
        LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
        {
            if (SUCCEEDED(hr = MFGetService(source->object, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl,
                    (void **)&rate_control)))
            {
                hr = IMFRateControl_SetRate(rate_control, thin, rate);
                IMFRateControl_Release(rate_control);
                if (SUCCEEDED(hr))
                {
                    session->presentation.flags |= SESSION_FLAG_PENDING_RATE_CHANGE;
                    session->presentation.rate = rate;
                    return;
                }
            }

            break;
        }
    }

    param.vt = VT_R4;
    param.fltVal = rate;
    session_command_complete_with_event(session, MESessionRateChanged, hr, SUCCEEDED(hr) ? &param : NULL);
}

static void session_complete_rate_change(struct media_session *session)
{
    PROPVARIANT param;
    HRESULT hr;

    if (!(session->presentation.flags & SESSION_FLAG_PENDING_RATE_CHANGE))
        return;

    session->presentation.flags &= ~SESSION_FLAG_PENDING_RATE_CHANGE;
    session_set_presentation_clock(session);

    hr = IMFRateControl_SetRate(session->clock_rate_control, session->presentation.thin,
            session->presentation.rate);

    param.vt = VT_R4;
    param.fltVal = session->presentation.rate;

    IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MESessionRateChanged, &GUID_NULL, hr,
            SUCCEEDED(hr) ? &param : NULL);
    session_command_complete(session);
}

static struct media_source *session_get_media_source(struct media_session *session, IMFMediaSource *source)
{
    struct media_source *cur;

    LIST_FOR_EACH_ENTRY(cur, &session->presentation.sources, struct media_source, entry)
    {
        if (source == cur->source)
            return cur;
    }

    return NULL;
}

static void session_release_media_source(struct media_source *source)
{
    IMFMediaSource_Release(source->source);
    if (source->pd)
        IMFPresentationDescriptor_Release(source->pd);
    free(source);
}

static HRESULT session_add_media_source(struct media_session *session, IMFTopologyNode *node, IMFMediaSource *source)
{
    struct media_source *media_source;
    HRESULT hr;

    if (session_get_media_source(session, source))
        return S_FALSE;

    if (!(media_source = calloc(1, sizeof(*media_source))))
        return E_OUTOFMEMORY;

    media_source->source = source;
    IMFMediaSource_AddRef(media_source->source);

    hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, &IID_IMFPresentationDescriptor,
            (void **)&media_source->pd);

    if (SUCCEEDED(hr))
        list_add_tail(&session->presentation.sources, &media_source->entry);
    else
        session_release_media_source(media_source);

    return hr;
}

static void session_raise_topology_set(struct media_session *session, IMFTopology *topology, HRESULT status)
{
    PROPVARIANT param;

    param.vt = topology ? VT_UNKNOWN : VT_EMPTY;
    param.punkVal = (IUnknown *)topology;

    IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MESessionTopologySet, &GUID_NULL, status, &param);
}

static DWORD session_get_object_rate_caps(IUnknown *object)
{
    IMFRateSupport *rate_support;
    DWORD caps = 0;
    float rate;

    if (SUCCEEDED(MFGetService(object, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rate_support)))
    {
        rate = 0.0f;
        if (SUCCEEDED(IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, TRUE, &rate)) && rate != 0.0f)
            caps |= MFSESSIONCAP_RATE_FORWARD;

        rate = 0.0f;
        if (SUCCEEDED(IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, TRUE, &rate)) && rate != 0.0f)
            caps |= MFSESSIONCAP_RATE_REVERSE;

        IMFRateSupport_Release(rate_support);
    }

    return caps;
}

static HRESULT session_add_media_sink(struct media_session *session, IMFTopologyNode *node, IMFMediaSink *sink)
{
    struct media_sink *media_sink;
    unsigned int disable_preroll = 0;
    DWORD flags;

    LIST_FOR_EACH_ENTRY(media_sink, &session->presentation.sinks, struct media_sink, entry)
    {
        if (sink == media_sink->sink)
            return S_FALSE;
    }

    if (!(media_sink = calloc(1, sizeof(*media_sink))))
        return E_OUTOFMEMORY;

    media_sink->sink = sink;
    IMFMediaSink_AddRef(media_sink->sink);

    IMFMediaSink_QueryInterface(media_sink->sink, &IID_IMFMediaEventGenerator, (void **)&media_sink->event_generator);

    IMFTopologyNode_GetUINT32(node, &MF_TOPONODE_DISABLE_PREROLL, &disable_preroll);
    if (SUCCEEDED(IMFMediaSink_GetCharacteristics(sink, &flags)) && flags & MEDIASINK_CAN_PREROLL && !disable_preroll)
    {
        if (SUCCEEDED(IMFMediaSink_QueryInterface(media_sink->sink, &IID_IMFMediaSinkPreroll, (void **)&media_sink->preroll)))
            session->presentation.flags |= SESSION_FLAG_NEEDS_PREROLL;
    }

    list_add_tail(&session->presentation.sinks, &media_sink->entry);

    return S_OK;
}

static DWORD transform_node_get_stream_id(struct topo_node *node, BOOL output, unsigned int index)
{
    DWORD *map = output ? node->u.transform.output_map : node->u.transform.input_map;
    return map ? map[index] : index;
}

static HRESULT session_set_transform_stream_info(struct topo_node *node)
{
    DWORD *input_map = NULL, *output_map = NULL;
    DWORD i, input_count, output_count;
    struct transform_stream *streams;
    unsigned int block_alignment;
    IMFMediaType *media_type;
    UINT32 bytes_per_second;
    GUID major = { 0 };
    HRESULT hr;

    hr = IMFTransform_GetStreamCount(node->object.transform, &input_count, &output_count);
    if (SUCCEEDED(hr) && (input_count > 1 || output_count > 1))
    {
        input_map = calloc(input_count, sizeof(*input_map));
        output_map = calloc(output_count, sizeof(*output_map));
        if (FAILED(IMFTransform_GetStreamIDs(node->object.transform, input_count, input_map,
                output_count, output_map)))
        {
            /* Assume sequential identifiers. */
            free(input_map);
            free(output_map);
            input_map = output_map = NULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        node->u.transform.input_map = input_map;
        node->u.transform.output_map = output_map;

        streams = calloc(input_count, sizeof(*streams));
        for (i = 0; i < input_count; ++i)
            list_init(&streams[i].samples);
        node->u.transform.inputs = streams;
        node->u.transform.input_count = input_count;

        streams = calloc(output_count, sizeof(*streams));
        for (i = 0; i < output_count; ++i)
        {
            list_init(&streams[i].samples);

            if (SUCCEEDED(IMFTransform_GetOutputCurrentType(node->object.transform,
                    transform_node_get_stream_id(node, TRUE, i), &media_type)))
            {
                if (SUCCEEDED(IMFMediaType_GetMajorType(media_type, &major)) && IsEqualGUID(&major, &MFMediaType_Audio)
                        && SUCCEEDED(IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_alignment)))
                {
                    streams[i].min_buffer_size = block_alignment;
                    if (SUCCEEDED(IMFMediaType_GetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bytes_per_second)))
                        streams[i].min_buffer_size = max(streams[i].min_buffer_size, bytes_per_second);
                }
                IMFMediaType_Release(media_type);
            }
        }
        node->u.transform.outputs = streams;
        node->u.transform.output_count = output_count;
    }

    return hr;
}

static HRESULT session_get_stream_sink_type(IMFStreamSink *sink, IMFMediaType **media_type)
{
    IMFMediaTypeHandler *handler;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFStreamSink_GetMediaTypeHandler(sink, &handler)))
    {
        hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, media_type);
        IMFMediaTypeHandler_Release(handler);
    }

    return hr;
}

static HRESULT WINAPI node_sample_allocator_cb_QueryInterface(IMFVideoSampleAllocatorNotify *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorNotify) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFVideoSampleAllocatorNotify_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI node_sample_allocator_cb_AddRef(IMFVideoSampleAllocatorNotify *iface)
{
    return 2;
}

static ULONG WINAPI node_sample_allocator_cb_Release(IMFVideoSampleAllocatorNotify *iface)
{
    return 1;
}

static HRESULT session_request_sample_from_node(struct media_session *session, IMFTopologyNode *node, DWORD output);

static HRESULT WINAPI node_sample_allocator_cb_NotifyRelease(IMFVideoSampleAllocatorNotify *iface)
{
    struct topo_node *topo_node = impl_node_from_IMFVideoSampleAllocatorNotify(iface);
    MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &topo_node->session->sa_ready_callback, (IUnknown *)iface);
    return S_OK;
}

static const IMFVideoSampleAllocatorNotifyVtbl node_sample_allocator_cb_vtbl =
{
    node_sample_allocator_cb_QueryInterface,
    node_sample_allocator_cb_AddRef,
    node_sample_allocator_cb_Release,
    node_sample_allocator_cb_NotifyRelease,
};

static HRESULT session_append_node(struct media_session *session, IMFTopologyNode *node)
{
    struct topo_node *topo_node;
    IMFMediaSink *media_sink;
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    HRESULT hr = S_OK;

    if (!(topo_node = calloc(1, sizeof(*topo_node))))
        return E_OUTOFMEMORY;

    IMFTopologyNode_GetNodeType(node, &topo_node->type);
    IMFTopologyNode_GetTopoNodeID(node, &topo_node->node_id);
    topo_node->node = node;
    IMFTopologyNode_AddRef(topo_node->node);
    topo_node->session = session;

    switch (topo_node->type)
    {
        case MF_TOPOLOGY_OUTPUT_NODE:
            topo_node->u.sink.notify_cb.lpVtbl = &node_sample_allocator_cb_vtbl;

            if (FAILED(hr = topology_node_get_object(node, &IID_IMFStreamSink, (void **)&topo_node->object.object)))
            {
                WARN("Failed to get stream sink interface, hr %#lx.\n", hr);
                break;
            }

            if (FAILED(hr = IMFStreamSink_GetMediaSink(topo_node->object.sink_stream, &media_sink)))
                break;

            if (SUCCEEDED(hr = session_add_media_sink(session, node, media_sink)))
            {
                if (SUCCEEDED(session_get_stream_sink_type(topo_node->object.sink_stream, &media_type)))
                {
                    if (SUCCEEDED(MFGetService(topo_node->object.object, &MR_VIDEO_ACCELERATION_SERVICE,
                        &IID_IMFVideoSampleAllocator, (void **)&topo_node->u.sink.allocator)))
                    {
                        if (FAILED(hr = IMFVideoSampleAllocator_InitializeSampleAllocator(topo_node->u.sink.allocator,
                                2, media_type)))
                        {
                            WARN("Failed to initialize sample allocator for the stream, hr %#lx.\n", hr);
                        }
                        IMFVideoSampleAllocator_QueryInterface(topo_node->u.sink.allocator,
                                &IID_IMFVideoSampleAllocatorCallback, (void **)&topo_node->u.sink.allocator_cb);
                        IMFVideoSampleAllocatorCallback_SetCallback(topo_node->u.sink.allocator_cb,
                                &topo_node->u.sink.notify_cb);
                    }
                    IMFMediaType_Release(media_type);
                }
            }
            IMFMediaSink_Release(media_sink);

            break;
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            if (FAILED(IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_SOURCE, &IID_IMFMediaSource,
                    (void **)&topo_node->u.source.source)))
            {
                WARN("Missing MF_TOPONODE_SOURCE, hr %#lx.\n", hr);
                break;
            }

            if (FAILED(hr = session_add_media_source(session, node, topo_node->u.source.source)))
                break;

            if (FAILED(hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR,
                    &IID_IMFStreamDescriptor, (void **)&sd)))
            {
                WARN("Missing MF_TOPONODE_STREAM_DESCRIPTOR, hr %#lx.\n", hr);
                break;
            }

            hr = IMFStreamDescriptor_GetStreamIdentifier(sd, &topo_node->u.source.stream_id);
            IMFStreamDescriptor_Release(sd);

            break;
        case MF_TOPOLOGY_TRANSFORM_NODE:

            if (SUCCEEDED(hr = topology_node_get_object(node, &IID_IMFTransform, (void **)&topo_node->object.transform)))
            {
                hr = session_set_transform_stream_info(topo_node);
            }
            else
                WARN("Failed to get IMFTransform for MFT node, hr %#lx.\n", hr);

            break;
        case MF_TOPOLOGY_TEE_NODE:
            FIXME("Unsupported node type %d.\n", topo_node->type);

            break;
        default:
            ;
    }

    if (SUCCEEDED(hr))
        list_add_tail(&session->presentation.nodes, &topo_node->entry);
    else
        release_topo_node(topo_node);

    return hr;
}

static HRESULT session_collect_nodes(struct media_session *session)
{
    IMFTopology *topology = session->presentation.current_topology;
    IMFTopologyNode *node;
    WORD i, count = 0;
    HRESULT hr;

    if (!list_empty(&session->presentation.nodes))
        return S_OK;

    if (FAILED(hr = IMFTopology_GetNodeCount(topology, &count)))
        return hr;

    for (i = 0; i < count; ++i)
    {
        if (FAILED(hr = IMFTopology_GetNode(topology, i, &node)))
        {
            WARN("Failed to get node %u.\n", i);
            break;
        }

        hr = session_append_node(session, node);
        IMFTopologyNode_Release(node);
        if (FAILED(hr))
        {
            WARN("Failed to add node %u.\n", i);
            break;
        }
    }

    return hr;
}

static HRESULT session_set_current_topology(struct media_session *session, IMFTopology *topology)
{
    struct media_source *source;
    DWORD caps, object_flags;
    struct media_sink *sink;
    struct topo_node *node;
    IMFMediaEvent *event;
    HRESULT hr;

    if (FAILED(hr = IMFTopology_CloneFrom(session->presentation.current_topology, topology)))
    {
        WARN("Failed to clone topology, hr %#lx.\n", hr);
        return hr;
    }

    session_collect_nodes(session);

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_TRANSFORM_NODE)
        {
            if (FAILED(hr = IMFTransform_ProcessMessage(node->object.transform,
                    MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0)))
                return hr;
        }
    }

    /* FIXME: attributes are all zero for now */
    if (SUCCEEDED(MFCreateMediaEvent(MESessionNotifyPresentationTime, &GUID_NULL, S_OK, NULL, &event)))
    {
        IMFMediaEvent_SetUINT64(event, &MF_EVENT_START_PRESENTATION_TIME, 0);
        IMFMediaEvent_SetUINT64(event, &MF_EVENT_PRESENTATION_TIME_OFFSET, 0);
        IMFMediaEvent_SetUINT64(event, &MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT, 0);

        IMFMediaEventQueue_QueueEvent(session->event_queue, event);
        IMFMediaEvent_Release(event);
    }

    /* Update session caps. */
    caps = MFSESSIONCAP_START | MFSESSIONCAP_SEEK | MFSESSIONCAP_RATE_FORWARD | MFSESSIONCAP_RATE_REVERSE |
            MFSESSIONCAP_DOES_NOT_USE_NETWORK;

    LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
    {
        if (!caps)
            break;

        object_flags = 0;
        if (SUCCEEDED(IMFMediaSource_GetCharacteristics(source->source, &object_flags)))
        {
            if (!(object_flags & MFMEDIASOURCE_DOES_NOT_USE_NETWORK))
                caps &= ~MFSESSIONCAP_DOES_NOT_USE_NETWORK;
            if (!(object_flags & MFMEDIASOURCE_CAN_SEEK))
                caps &= ~MFSESSIONCAP_SEEK;
        }

        /* Mask unsupported rate caps. */

        caps &= session_get_object_rate_caps(source->object)
                | ~(MFSESSIONCAP_RATE_FORWARD | MFSESSIONCAP_RATE_REVERSE);
    }

    LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
    {
        if (!caps)
            break;

        object_flags = 0;
        if (SUCCEEDED(IMFMediaSink_GetCharacteristics(sink->sink, &object_flags)))
        {
            if (!(object_flags & MEDIASINK_RATELESS))
                caps &= session_get_object_rate_caps(sink->object)
                        | ~(MFSESSIONCAP_RATE_FORWARD | MFSESSIONCAP_RATE_REVERSE);
        }
    }

    session_set_caps(session, caps);

    return S_OK;
}

static void session_set_topology(struct media_session *session, DWORD flags, IMFTopology *topology)
{
    IMFTopology *resolved_topology = NULL;
    HRESULT hr = S_OK;

    /* Resolve unless claimed to be full. */
    if (!(flags & MFSESSION_SETTOPOLOGY_CLEAR_CURRENT) && topology)
    {
        if (!(flags & MFSESSION_SETTOPOLOGY_NORESOLUTION))
        {
            hr = session_bind_output_nodes(topology);

            if (SUCCEEDED(hr))
                hr = IMFTopoLoader_Load(session->topo_loader, topology, &resolved_topology, NULL /* FIXME? */);
            if (SUCCEEDED(hr))
                hr = session_init_media_types(resolved_topology);

            if (SUCCEEDED(hr))
            {
                topology = resolved_topology;
            }
        }
    }

    if (flags & MFSESSION_SETTOPOLOGY_CLEAR_CURRENT)
    {
        if ((topology && topology == session->presentation.current_topology) || !topology)
        {
            /* FIXME: stop current topology, queue next one. */
            session_clear_presentation(session);
        }
        else
            hr = S_FALSE;

        topology = NULL;
    }
    else if (topology && flags & MFSESSION_SETTOPOLOGY_IMMEDIATE)
    {
        session_clear_queued_topologies(session);
        session_clear_presentation(session);
    }

    session_raise_topology_set(session, topology, hr);

    /* With no current topology set it right away, otherwise queue. */
    if (topology)
    {
        struct queued_topology *queued_topology;

        if ((queued_topology = calloc(1, sizeof(*queued_topology))))
        {
            queued_topology->topology = topology;
            IMFTopology_AddRef(queued_topology->topology);

            list_add_tail(&session->topologies, &queued_topology->entry);
        }

        if (session->presentation.topo_status == MF_TOPOSTATUS_INVALID)
        {
            hr = session_set_current_topology(session, topology);
            session_set_topo_status(session, hr, MF_TOPOSTATUS_READY);
            if (session->quality_manager)
                IMFQualityManager_NotifyTopology(session->quality_manager, topology);
        }
    }

    if (resolved_topology)
        IMFTopology_Release(resolved_topology);
}

static HRESULT WINAPI mfsession_QueryInterface(IMFMediaSession *iface, REFIID riid, void **out)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    *out = NULL;

    if (IsEqualIID(riid, &IID_IMFMediaSession) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &session->IMFMediaSession_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *out = &session->IMFGetService_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFRateSupport))
    {
        *out = &session->IMFRateSupport_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFRateControl))
    {
        *out = &session->IMFRateControl_iface;
    }

    if (*out)
    {
        IMFMediaSession_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI mfsession_AddRef(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    ULONG refcount = InterlockedIncrement(&session->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI mfsession_Release(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    ULONG refcount = InterlockedDecrement(&session->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        session_clear_queued_topologies(session);
        session_clear_presentation(session);
        session_clear_command_list(session);
        if (session->presentation.current_topology)
            IMFTopology_Release(session->presentation.current_topology);
        if (session->event_queue)
            IMFMediaEventQueue_Release(session->event_queue);
        if (session->clock)
            IMFPresentationClock_Release(session->clock);
        if (session->system_time_source)
            IMFPresentationTimeSource_Release(session->system_time_source);
        if (session->clock_rate_control)
            IMFRateControl_Release(session->clock_rate_control);
        if (session->topo_loader)
            IMFTopoLoader_Release(session->topo_loader);
        if (session->quality_manager)
            IMFQualityManager_Release(session->quality_manager);
        DeleteCriticalSection(&session->cs);
        free(session);
    }

    return refcount;
}

static HRESULT WINAPI mfsession_GetEvent(IMFMediaSession *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

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

    TRACE("%p, %ld, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(session->event_queue, event_type, ext_type, hr, value);
}

static HRESULT session_check_stream_descriptor(IMFPresentationDescriptor *pd, IMFStreamDescriptor *sd)
{
    IMFStreamDescriptor *selected_sd;
    DWORD i, count;
    BOOL selected;
    HRESULT hr;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, &count)))
    {
        WARN("Failed to get stream descriptor count, hr %#lx.\n", hr);
        return MF_E_TOPO_STREAM_DESCRIPTOR_NOT_SELECTED;
    }

    for (i = 0; i < count; ++i)
    {
        if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, i,
                    &selected, &selected_sd)))
        {
            WARN("Failed to get stream descriptor %lu, hr %#lx.\n", i, hr);
            return MF_E_TOPO_STREAM_DESCRIPTOR_NOT_SELECTED;
        }
        IMFStreamDescriptor_Release(selected_sd);

        if (selected_sd == sd)
        {
            if (selected)
                return S_OK;

            WARN("Presentation descriptor %p stream %p is not selected.\n", pd, sd);
            return MF_E_TOPO_STREAM_DESCRIPTOR_NOT_SELECTED;
        }
    }

    WARN("Failed to find stream descriptor %lu, hr %#lx.\n", i, hr);
    return MF_E_TOPO_STREAM_DESCRIPTOR_NOT_SELECTED;
}

static HRESULT session_check_topology(IMFTopology *topology)
{
    MF_TOPOLOGY_TYPE node_type;
    IMFTopologyNode *node;
    WORD node_count, i;
    HRESULT hr;

    if (!topology)
        return S_OK;

    if (FAILED(IMFTopology_GetNodeCount(topology, &node_count))
            || node_count == 0)
        return E_INVALIDARG;

    for (i = 0; i < node_count; ++i)
    {
        if (FAILED(hr = IMFTopology_GetNode(topology, i, &node)))
            break;

        if (FAILED(hr = IMFTopologyNode_GetNodeType(node, &node_type)))
        {
            IMFTopologyNode_Release(node);
            break;
        }

        switch (node_type)
        {
            case MF_TOPOLOGY_SOURCESTREAM_NODE:
            {
                IMFPresentationDescriptor *pd;
                IMFStreamDescriptor *sd;
                IMFMediaSource *source;

                if (FAILED(hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_SOURCE, &IID_IMFMediaSource,
                        (void **)&source)))
                {
                    WARN("Missing MF_TOPONODE_SOURCE, hr %#lx.\n", hr);
                    IMFTopologyNode_Release(node);
                    return MF_E_TOPO_MISSING_SOURCE;
                }
                IMFMediaSource_Release(source);

                if (FAILED(hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR,
                        &IID_IMFPresentationDescriptor, (void **)&pd)))
                {
                    WARN("Missing MF_TOPONODE_PRESENTATION_DESCRIPTOR, hr %#lx.\n", hr);
                    IMFTopologyNode_Release(node);
                    return MF_E_TOPO_MISSING_PRESENTATION_DESCRIPTOR;
                }

                if (FAILED(hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR,
                        &IID_IMFStreamDescriptor, (void **)&sd)))
                {
                    WARN("Missing MF_TOPONODE_STREAM_DESCRIPTOR, hr %#lx.\n", hr);
                    IMFPresentationDescriptor_Release(pd);
                    IMFTopologyNode_Release(node);
                    return MF_E_TOPO_MISSING_STREAM_DESCRIPTOR;
                }

                hr = session_check_stream_descriptor(pd, sd);
                IMFPresentationDescriptor_Release(pd);
                IMFStreamDescriptor_Release(sd);
                if (FAILED(hr))
                {
                    IMFTopologyNode_Release(node);
                    return hr;
                }

                break;
            }

            default:
                break;
        }

        IMFTopologyNode_Release(node);
    }

    return hr;
}

static HRESULT WINAPI mfsession_SetTopology(IMFMediaSession *iface, DWORD flags, IMFTopology *topology)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p, %#lx, %p.\n", iface, flags, topology);

    if (FAILED(hr = session_check_topology(topology)))
        return hr;

    if (FAILED(hr = create_session_op(SESSION_CMD_SET_TOPOLOGY, &op)))
        return hr;

    op->set_topology.flags = flags;
    op->set_topology.topology = topology;
    if (op->set_topology.topology)
        IMFTopology_AddRef(op->set_topology.topology);

    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI mfsession_ClearTopologies(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p.\n", iface);

    return session_submit_simple_command(session, SESSION_CMD_CLEAR_TOPOLOGIES);
}

static HRESULT WINAPI mfsession_Start(IMFMediaSession *iface, const GUID *format, const PROPVARIANT *start_position)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(format), debugstr_propvar(start_position));

    if (!start_position)
        return E_POINTER;

    if (FAILED(hr = create_session_op(SESSION_CMD_START, &op)))
        return hr;

    op->start.time_format = format ? *format : GUID_NULL;
    hr = PropVariantCopy(&op->start.start_position, start_position);

    if (SUCCEEDED(hr))
        hr = session_submit_command(session, op);

    IUnknown_Release(&op->IUnknown_iface);
    return hr;
}

static HRESULT WINAPI mfsession_Pause(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p.\n", iface);

    return session_submit_simple_command(session, SESSION_CMD_PAUSE);
}

static HRESULT WINAPI mfsession_Stop(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p.\n", iface);

    return session_submit_simple_command(session, SESSION_CMD_STOP);
}

static HRESULT WINAPI mfsession_Close(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);

    TRACE("%p.\n", iface);

    return session_submit_simple_command(session, SESSION_CMD_CLOSE);
}

static HRESULT WINAPI mfsession_Shutdown(IMFMediaSession *iface)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&session->cs);
    if (SUCCEEDED(hr = session_is_shut_down(session)))
    {
        session->state = SESSION_STATE_SHUT_DOWN;
        IMFMediaEventQueue_Shutdown(session->event_queue);
        if (session->quality_manager)
            IMFQualityManager_Shutdown(session->quality_manager);
        MFShutdownObject((IUnknown *)session->clock);
        IMFPresentationClock_Release(session->clock);
        session->clock = NULL;
        session_clear_presentation(session);
        session_clear_queued_topologies(session);
        session_submit_simple_command(session, SESSION_CMD_SHUTDOWN);
    }
    LeaveCriticalSection(&session->cs);

    return hr;
}

static HRESULT WINAPI mfsession_GetClock(IMFMediaSession *iface, IMFClock **clock)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&session->cs);
    if (SUCCEEDED(hr = session_is_shut_down(session)))
    {
        *clock = (IMFClock *)session->clock;
        IMFClock_AddRef(*clock);
    }
    LeaveCriticalSection(&session->cs);

    return hr;
}

static HRESULT WINAPI mfsession_GetSessionCapabilities(IMFMediaSession *iface, DWORD *caps)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, caps);

    if (!caps)
        return E_POINTER;

    EnterCriticalSection(&session->cs);
    if (SUCCEEDED(hr = session_is_shut_down(session)))
        *caps = session->caps;
    LeaveCriticalSection(&session->cs);

    return hr;
}

static HRESULT WINAPI mfsession_GetFullTopology(IMFMediaSession *iface, DWORD flags, TOPOID id, IMFTopology **topology)
{
    struct media_session *session = impl_from_IMFMediaSession(iface);
    struct queued_topology *queued;
    TOPOID topo_id;
    HRESULT hr;

    TRACE("%p, %#lx, %s, %p.\n", iface, flags, wine_dbgstr_longlong(id), topology);

    *topology = NULL;

    EnterCriticalSection(&session->cs);

    if (SUCCEEDED(hr = session_is_shut_down(session)))
    {
        if (flags & MFSESSION_GETFULLTOPOLOGY_CURRENT)
        {
            if (session->presentation.topo_status != MF_TOPOSTATUS_INVALID)
                *topology = session->presentation.current_topology;
            else
                hr = MF_E_INVALIDREQUEST;
        }
        else
        {
            LIST_FOR_EACH_ENTRY(queued, &session->topologies, struct queued_topology, entry)
            {
                if (SUCCEEDED(IMFTopology_GetTopologyID(queued->topology, &topo_id)) && topo_id == id)
                {
                    *topology = queued->topology;
                    break;
                }
            }
        }

        if (*topology)
            IMFTopology_AddRef(*topology);
    }

    LeaveCriticalSection(&session->cs);

    return hr;
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

typedef BOOL (*p_renderer_node_test_func)(IMFMediaSink *sink);

static BOOL session_video_renderer_test_func(IMFMediaSink *sink)
{
    IUnknown *obj;
    HRESULT hr;

    /* Use first sink to support IMFVideoRenderer. */
    hr = IMFMediaSink_QueryInterface(sink, &IID_IMFVideoRenderer, (void **)&obj);
    if (obj)
        IUnknown_Release(obj);

    return hr == S_OK;
}

static BOOL session_audio_renderer_test_func(IMFMediaSink *sink)
{
    return mf_is_sar_sink(sink);
}

static HRESULT session_get_renderer_node_service(struct media_session *session,
        p_renderer_node_test_func node_test_func, REFGUID service, REFIID riid, void **obj)
{
    HRESULT hr = E_NOINTERFACE;
    IMFStreamSink *stream_sink;
    IMFTopologyNode *node;
    IMFCollection *nodes;
    IMFMediaSink *sink;
    unsigned int i = 0;

    if (session->presentation.current_topology)
    {
        if (SUCCEEDED(IMFTopology_GetOutputNodeCollection(session->presentation.current_topology,
                &nodes)))
        {
            while (IMFCollection_GetElement(nodes, i++, (IUnknown **)&node) == S_OK)
            {
                if (SUCCEEDED(topology_node_get_object(node, &IID_IMFStreamSink, (void **)&stream_sink)))
                {
                    if (SUCCEEDED(IMFStreamSink_GetMediaSink(stream_sink, &sink)))
                    {
                        if (node_test_func(sink))
                        {
                            if (FAILED(hr = MFGetService((IUnknown *)sink, service, riid, obj)))
                                WARN("Failed to get service from renderer node, %#lx.\n", hr);
                        }
                        IMFMediaSink_Release(sink);
                    }
                    IMFStreamSink_Release(stream_sink);
                }

                IMFTopologyNode_Release(node);

                if (*obj)
                    break;
            }

            IMFCollection_Release(nodes);
        }
    }

    return hr;
}

static HRESULT session_get_audio_render_service(struct media_session *session, REFGUID service,
        REFIID riid, void **obj)
{
    return session_get_renderer_node_service(session, session_audio_renderer_test_func,
            service, riid, obj);
}

static HRESULT session_get_video_render_service(struct media_session *session, REFGUID service,
        REFIID riid, void **obj)
{
    return session_get_renderer_node_service(session, session_video_renderer_test_func,
            service, riid, obj);
}

static HRESULT WINAPI session_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct media_session *session = impl_from_IMFGetService(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    *obj = NULL;

    EnterCriticalSection(&session->cs);
    if (FAILED(hr = session_is_shut_down(session)))
    {
    }
    else if (IsEqualGUID(service, &MF_RATE_CONTROL_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFRateSupport))
        {
            *obj = &session->IMFRateSupport_iface;
        }
        else if (IsEqualIID(riid, &IID_IMFRateControl))
        {
            *obj = &session->IMFRateControl_iface;
        }
        else
            hr = E_NOINTERFACE;

        if (*obj)
            IUnknown_AddRef((IUnknown *)*obj);
    }
    else if (IsEqualGUID(service, &MF_LOCAL_MFT_REGISTRATION_SERVICE))
    {
        hr = IMFLocalMFTRegistration_QueryInterface(&local_mft_registration, riid, obj);
    }
    else if (IsEqualGUID(service, &MF_TOPONODE_ATTRIBUTE_EDITOR_SERVICE))
    {
        *obj = &session->IMFTopologyNodeAttributeEditor_iface;
        IUnknown_AddRef((IUnknown *)*obj);
    }
    else if (IsEqualGUID(service, &MR_VIDEO_RENDER_SERVICE))
    {
        hr = session_get_video_render_service(session, service, riid, obj);
    }
    else if (IsEqualGUID(service, &MR_POLICY_VOLUME_SERVICE) ||
            IsEqualGUID(service, &MR_STREAM_VOLUME_SERVICE))
    {
        hr = session_get_audio_render_service(session, service, riid, obj);
    }
    else
        FIXME("Unsupported service %s.\n", debugstr_guid(service));

    LeaveCriticalSection(&session->cs);

    return hr;
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

static void session_deliver_pending_samples(struct media_session *session, IMFTopologyNode *node);

static HRESULT WINAPI session_commands_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct session_op *op = impl_op_from_IUnknown(IMFAsyncResult_GetStateNoAddRef(result));
    struct media_session *session = impl_from_commands_callback_IMFAsyncCallback(iface);

    TRACE("session %p, op %p, command %u.\n", session, op, op->command);

    EnterCriticalSection(&session->cs);

    if (session->presentation.flags & SESSION_FLAG_PENDING_COMMAND)
    {
        WARN("session %p command is in progress, waiting for it to complete.\n", session);
        LeaveCriticalSection(&session->cs);
        return S_OK;
    }

    list_remove(&op->entry);
    session->presentation.flags |= SESSION_FLAG_PENDING_COMMAND;

    switch (op->command)
    {
        case SESSION_CMD_CLEAR_TOPOLOGIES:
            session_clear_topologies(session);
            break;
        case SESSION_CMD_SET_TOPOLOGY:
            session_set_topology(session, op->set_topology.flags, op->set_topology.topology);
            session_command_complete(session);
            break;
        case SESSION_CMD_START:
            session_start(session, &op->start.time_format, &op->start.start_position);
            break;
        case SESSION_CMD_PAUSE:
            session_pause(session);
            break;
        case SESSION_CMD_STOP:
            session_stop(session);
            break;
        case SESSION_CMD_CLOSE:
            session_close(session);
            break;
        case SESSION_CMD_SET_RATE:
            session_set_rate(session, op->set_rate.thin, op->set_rate.rate);
            break;
        case SESSION_CMD_SHUTDOWN:
            session_clear_command_list(session);
            session_command_complete(session);
            break;
        default:
            ;
    }

    LeaveCriticalSection(&session->cs);

    IUnknown_Release(&op->IUnknown_iface);

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

static HRESULT WINAPI session_sa_ready_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_sa_ready_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_sa_ready_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_sa_ready_callback_Release(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_sa_ready_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_sa_ready_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI session_sa_ready_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    IMFVideoSampleAllocatorNotify *notify = (IMFVideoSampleAllocatorNotify *)IMFAsyncResult_GetStateNoAddRef(result);
    struct topo_node *topo_node = impl_node_from_IMFVideoSampleAllocatorNotify(notify);
    struct media_session *session = impl_from_sa_ready_callback_IMFAsyncCallback(iface);
    IMFTopologyNode *upstream_node;
    DWORD upstream_output;

    EnterCriticalSection(&session->cs);

    if (topo_node->u.sink.requests)
    {
        if (SUCCEEDED(IMFTopologyNode_GetInput(topo_node->node, 0, &upstream_node, &upstream_output)))
        {
            session_deliver_pending_samples(session, upstream_node);
            IMFTopologyNode_Release(upstream_node);
        }
    }

    LeaveCriticalSection(&session->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl session_sa_ready_callback_vtbl =
{
    session_sa_ready_callback_QueryInterface,
    session_sa_ready_callback_AddRef,
    session_sa_ready_callback_Release,
    session_sa_ready_callback_GetParameters,
    session_sa_ready_callback_Invoke,
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

static HRESULT session_add_media_stream(struct media_session *session, IMFMediaSource *source, IMFMediaStream *stream)
{
    struct topo_node *node;
    IMFStreamDescriptor *sd;
    DWORD stream_id = 0;
    HRESULT hr;

    if (FAILED(hr = IMFMediaStream_GetStreamDescriptor(stream, &sd)))
        return hr;

    hr = IMFStreamDescriptor_GetStreamIdentifier(sd, &stream_id);
    IMFStreamDescriptor_Release(sd);
    if (FAILED(hr))
        return hr;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_SOURCESTREAM_NODE && node->u.source.source == source
                && node->u.source.stream_id == stream_id)
        {
            if (node->object.source_stream)
            {
                WARN("Node already has stream set.\n");
                return S_FALSE;
            }

            node->object.source_stream = stream;
            IMFMediaStream_AddRef(node->object.source_stream);
            break;
        }
    }

    return S_OK;
}

static BOOL session_is_source_nodes_state(struct media_session *session, enum object_state state)
{
    struct media_source *source;
    struct topo_node *node;

    LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
    {
        if (source->state != state)
            return FALSE;
    }

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_SOURCESTREAM_NODE && node->state != state)
            return FALSE;
    }

    return TRUE;
}

static BOOL session_is_output_nodes_state(struct media_session *session, enum object_state state)
{
    struct topo_node *node;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_OUTPUT_NODE && node->state != state)
            return FALSE;
    }

    return TRUE;
}

static enum object_state session_get_object_state_for_event(MediaEventType event)
{
    switch (event)
    {
        case MESourceSeeked:
        case MEStreamSeeked:
        case MESourceStarted:
        case MEStreamStarted:
        case MEStreamSinkStarted:
            return OBJ_STATE_STARTED;
        case MESourcePaused:
        case MEStreamPaused:
        case MEStreamSinkPaused:
            return OBJ_STATE_PAUSED;
        case MESourceStopped:
        case MEStreamStopped:
        case MEStreamSinkStopped:
            return OBJ_STATE_STOPPED;
        case MEStreamSinkPrerolled:
            return OBJ_STATE_PREROLLED;
        default:
            return OBJ_STATE_INVALID;
    }
}

static HRESULT session_start_clock(struct media_session *session)
{
    LONGLONG start_offset = 0;
    HRESULT hr;

    if (IsEqualGUID(&session->presentation.time_format, &GUID_NULL))
    {
        if (session->presentation.start_position.vt == VT_EMPTY)
            start_offset = PRESENTATION_CURRENT_POSITION;
        else if (session->presentation.start_position.vt == VT_I8)
            start_offset = session->presentation.start_position.hVal.QuadPart;
        else
            FIXME("Unhandled position type %d.\n", session->presentation.start_position.vt);
    }
    else
        FIXME("Unhandled time format %s.\n", debugstr_guid(&session->presentation.time_format));

    if (FAILED(hr = IMFPresentationClock_Start(session->clock, start_offset)))
        WARN("Failed to start session clock, hr %#lx.\n", hr);

    return hr;
}

static struct topo_node *session_get_node_object(struct media_session *session, IUnknown *object,
        MF_TOPOLOGY_TYPE node_type)
{
    struct topo_node *node = NULL;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == node_type && object == node->object.object)
            return node;
    }

    return NULL;
}

static BOOL session_set_node_object_state(struct media_session *session, IUnknown *object,
        MF_TOPOLOGY_TYPE node_type, enum object_state state)
{
    struct topo_node *node;
    BOOL changed = FALSE;

    if ((node = session_get_node_object(session, object, node_type)))
    {
        changed = node->state != state;
        node->state = state;
    }

    return changed;
}

static void session_set_source_object_state(struct media_session *session, IUnknown *object,
        MediaEventType event_type)
{
    struct media_source *source;
    IMFStreamSink *stream_sink;
    struct media_source *src;
    struct media_sink *sink;
    enum object_state state;
    BOOL changed = FALSE;
    DWORD i, count;
    HRESULT hr;

    if ((state = session_get_object_state_for_event(event_type)) == OBJ_STATE_INVALID)
        return;

    switch (event_type)
    {
        case MESourceSeeked:
        case MESourceStarted:
        case MESourcePaused:
        case MESourceStopped:

            LIST_FOR_EACH_ENTRY(src, &session->presentation.sources, struct media_source, entry)
            {
                if (object == src->object)
                {
                    changed = src->state != state;
                    src->state = state;
                    break;
                }
            }
            break;
        case MEStreamSeeked:
        case MEStreamStarted:
        case MEStreamPaused:
        case MEStreamStopped:

            changed = session_set_node_object_state(session, object, MF_TOPOLOGY_SOURCESTREAM_NODE, state);
        default:
            ;
    }

    if (!changed)
        return;

    switch (session->state)
    {
        case SESSION_STATE_STARTING_SOURCES:
            if (!session_is_source_nodes_state(session, OBJ_STATE_STARTED))
                break;

            session_set_topo_status(session, S_OK, MF_TOPOSTATUS_STARTED_SOURCE);

            session_set_presentation_clock(session);

            /* If sinks are already started, start session immediately. This can happen when doing a
             * seek from SESSION_STATE_STARTED */
            if (session_is_output_nodes_state(session, OBJ_STATE_STARTED)
                    && SUCCEEDED(session_start_clock(session)))
            {
                session_set_started(session);
                return;
            }

            if ((session->presentation.flags & SESSION_FLAG_NEEDS_PREROLL) && session_is_output_nodes_state(session, OBJ_STATE_STOPPED))
            {
                MFTIME preroll_time = 0;

                if (session->presentation.start_position.vt == VT_I8)
                    preroll_time = session->presentation.start_position.hVal.QuadPart;

                /* Mark stream sinks without prerolling support as PREROLLED to keep state test logic generic. */
                LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
                {
                    if (sink->preroll)
                    {
                        /* FIXME: abort and enter error state on failure. */
                        if (FAILED(hr = IMFMediaSinkPreroll_NotifyPreroll(sink->preroll, preroll_time)))
                            WARN("Preroll notification failed, hr %#lx.\n", hr);
                    }
                    else
                    {
                        if (SUCCEEDED(IMFMediaSink_GetStreamSinkCount(sink->sink, &count)))
                        {
                            for (i = 0; i < count; ++i)
                            {
                                if (SUCCEEDED(IMFMediaSink_GetStreamSinkByIndex(sink->sink, i, &stream_sink)))
                                {
                                    session_set_node_object_state(session, (IUnknown *)stream_sink, MF_TOPOLOGY_OUTPUT_NODE,
                                            OBJ_STATE_PREROLLED);
                                    IMFStreamSink_Release(stream_sink);
                                }
                            }
                        }
                    }
                }
                session->state = SESSION_STATE_PREROLLING_SINKS;
            }
            else if (SUCCEEDED(session_start_clock(session)))
                session->state = SESSION_STATE_STARTING_SINKS;

            break;
        case SESSION_STATE_RESTARTING_SOURCES:
            if (!session_is_source_nodes_state(session, OBJ_STATE_STOPPED))
                break;

            session_flush_nodes(session);

            /* Start sources */
            LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
            {
                if (FAILED(hr = IMFMediaSource_Start(source->source, source->pd,
                        &session->presentation.time_format, &session->presentation.start_position)))
                {
                    WARN("Failed to start media source %p, hr %#lx.\n", source->source, hr);
                    session_command_complete_with_event(session, MESessionStarted, hr, NULL);
                    return;
                }
            }
            session->state = SESSION_STATE_STARTING_SOURCES;
            break;
        case SESSION_STATE_PAUSING_SOURCES:
            if (!session_is_source_nodes_state(session, OBJ_STATE_PAUSED))
                break;

            session_set_paused(session, SESSION_STATE_PAUSED, S_OK);
            break;
        case SESSION_STATE_STOPPING_SOURCES:
            if (!session_is_source_nodes_state(session, OBJ_STATE_STOPPED))
                break;

            session_flush_nodes(session);
            session_set_caps(session, session->caps & ~MFSESSIONCAP_PAUSE);

            if (session->presentation.flags & SESSION_FLAG_FINALIZE_SINKS)
                session_finalize_sinks(session);
            else
                session_set_stopped(session, S_OK);

            break;
        default:
            ;
    }
}

static void session_set_sink_stream_state(struct media_session *session, IMFStreamSink *stream,
        MediaEventType event_type)
{
    struct media_source *source;
    enum object_state state;
    HRESULT hr = S_OK;
    BOOL changed;

    if ((state = session_get_object_state_for_event(event_type)) == OBJ_STATE_INVALID)
        return;

    if (!(changed = session_set_node_object_state(session, (IUnknown *)stream, MF_TOPOLOGY_OUTPUT_NODE, state)))
        return;

    switch (session->state)
    {
        case SESSION_STATE_PREROLLING_SINKS:
            if (!session_is_output_nodes_state(session, OBJ_STATE_PREROLLED))
                break;

            if (SUCCEEDED(session_start_clock(session)))
                session->state = SESSION_STATE_STARTING_SINKS;
            break;
        case SESSION_STATE_STARTING_SINKS:
            if (!session_is_output_nodes_state(session, OBJ_STATE_STARTED))
                break;

            session_set_started(session);
            break;
        case SESSION_STATE_PAUSING_SINKS:
            if (!session_is_output_nodes_state(session, OBJ_STATE_PAUSED))
                break;

            session->state = SESSION_STATE_PAUSING_SOURCES;

            LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
            {
                if (FAILED(hr = IMFMediaSource_Pause(source->source)))
                    break;
            }

            if (FAILED(hr))
                session_set_paused(session, SESSION_STATE_PAUSED, hr);

            break;
        case SESSION_STATE_STOPPING_SINKS:
            if (!session_is_output_nodes_state(session, OBJ_STATE_STOPPED))
                break;

            session->state = SESSION_STATE_STOPPING_SOURCES;

            LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
            {
                if (session->presentation.flags & SESSION_FLAG_END_OF_PRESENTATION)
                    IMFMediaSource_Stop(source->source);
                else if (FAILED(hr = IMFMediaSource_Stop(source->source)))
                    break;
            }

            if (session->presentation.flags & SESSION_FLAG_END_OF_PRESENTATION || FAILED(hr))
                session_set_stopped(session, hr);

            break;
        default:
            ;
    }
}

static HRESULT transform_get_external_output_sample(const struct media_session *session, struct topo_node *transform,
        unsigned int output_index, const MFT_OUTPUT_STREAM_INFO *stream_info, IMFSample **sample)
{
    IMFTopologyNode *downstream_node;
    IMFMediaBuffer *buffer = NULL;
    struct topo_node *topo_node;
    unsigned int buffer_size;
    DWORD downstream_input;
    TOPOID node_id;
    HRESULT hr;

    if (FAILED(IMFTopologyNode_GetOutput(transform->node, output_index, &downstream_node, &downstream_input)))
    {
        WARN("Failed to get connected node for output %u.\n", output_index);
        return MF_E_UNEXPECTED;
    }

    IMFTopologyNode_GetTopoNodeID(downstream_node, &node_id);
    IMFTopologyNode_Release(downstream_node);

    topo_node = session_get_node_by_id(session, node_id);

    if (topo_node->type == MF_TOPOLOGY_OUTPUT_NODE && topo_node->u.sink.allocator)
    {
        hr = IMFVideoSampleAllocator_AllocateSample(topo_node->u.sink.allocator, sample);
    }
    else
    {
        buffer_size = max(stream_info->cbSize, transform->u.transform.outputs[output_index].min_buffer_size);

        hr = MFCreateAlignedMemoryBuffer(buffer_size, stream_info->cbAlignment, &buffer);
        if (SUCCEEDED(hr))
            hr = MFCreateSample(sample);

        if (SUCCEEDED(hr))
            hr = IMFSample_AddBuffer(*sample, buffer);

        if (buffer)
            IMFMediaBuffer_Release(buffer);
    }

    return hr;
}

static HRESULT transform_node_pull_samples(const struct media_session *session, struct topo_node *node)
{
    MFT_OUTPUT_STREAM_INFO stream_info;
    MFT_OUTPUT_DATA_BUFFER *buffers;
    HRESULT hr = E_UNEXPECTED;
    DWORD status = 0;
    unsigned int i;

    if (!(buffers = calloc(node->u.transform.output_count, sizeof(*buffers))))
        return E_OUTOFMEMORY;

    for (i = 0; i < node->u.transform.output_count; ++i)
    {
        buffers[i].dwStreamID = transform_node_get_stream_id(node, TRUE, i);
        buffers[i].pSample = NULL;
        buffers[i].dwStatus = 0;
        buffers[i].pEvents = NULL;

        memset(&stream_info, 0, sizeof(stream_info));
        if (FAILED(hr = IMFTransform_GetOutputStreamInfo(node->object.transform, buffers[i].dwStreamID, &stream_info)))
            break;

        if (!(stream_info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)))
        {
            if (FAILED(hr = transform_get_external_output_sample(session, node, i, &stream_info, &buffers[i].pSample)))
                break;
        }
    }

    if (SUCCEEDED(hr))
        hr = IMFTransform_ProcessOutput(node->object.transform, 0, node->u.transform.output_count, buffers, &status);

    /* Collect returned samples for all streams. */
    for (i = 0; i < node->u.transform.output_count; ++i)
    {
        struct transform_stream *stream = &node->u.transform.outputs[i];

        if (buffers[i].pEvents)
            IMFCollection_Release(buffers[i].pEvents);

        if (SUCCEEDED(hr) && !(buffers[i].dwStatus & MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE))
        {
            if (session->quality_manager)
                IMFQualityManager_NotifyProcessOutput(session->quality_manager, node->node, i, buffers[i].pSample);
            if (FAILED(hr = transform_stream_push_sample(stream, buffers[i].pSample)))
                WARN("Failed to queue output sample, hr %#lx\n", hr);
        }

        if (buffers[i].pSample)
            IMFSample_Release(buffers[i].pSample);
    }

    free(buffers);

    return hr;
}

static BOOL transform_node_is_drained(struct topo_node *topo_node)
{
    UINT i;

    for (i = 0; i < topo_node->u.transform.input_count; i++)
    {
        struct transform_stream *stream = &topo_node->u.transform.inputs[i];
        if (!stream->draining)
            return FALSE;
    }

    return TRUE;
}

static BOOL transform_node_has_requests(struct topo_node *topo_node)
{
    UINT i;

    for (i = 0; i < topo_node->u.transform.output_count; i++)
        if (topo_node->u.transform.outputs[i].requests)
            return TRUE;

    return FALSE;
}

static HRESULT transform_node_push_sample(const struct media_session *session, struct topo_node *topo_node,
        UINT input, IMFSample *sample)
{
    struct transform_stream *stream = &topo_node->u.transform.inputs[input];
    UINT id = transform_node_get_stream_id(topo_node, FALSE, input);
    IMFTransform *transform = topo_node->object.transform;
    HRESULT hr;

    if (sample)
    {
        hr = IMFTransform_ProcessInput(transform, id, sample, 0);
        if (hr == MF_E_NOTACCEPTING)
            hr = transform_stream_push_sample(stream, sample);
    }
    else
    {
        if (FAILED(hr = IMFTransform_ProcessMessage(transform, MFT_MESSAGE_COMMAND_DRAIN, id)))
            WARN("Drain command failed for transform, hr %#lx.\n", hr);
        else
            stream->draining = TRUE;
    }

    return hr;
}

static void session_deliver_sample_to_node(struct media_session *session, IMFTopologyNode *node, unsigned int input,
        IMFSample *sample);

static void transform_node_deliver_samples(struct media_session *session, struct topo_node *topo_node)
{
    IMFTopologyNode *up_node = topo_node->node, *down_node;
    BOOL drained = transform_node_is_drained(topo_node);
    DWORD output, input;
    IMFSample *sample;
    HRESULT hr = S_OK;

    /* Push down all available output. */
    for (output = 0; SUCCEEDED(hr) && output < topo_node->u.transform.output_count; ++output)
    {
        struct transform_stream *stream = &topo_node->u.transform.outputs[output];

        if (FAILED(hr = IMFTopologyNode_GetOutput(up_node, output, &down_node, &input)))
        {
            WARN("Failed to node %p/%lu output, hr %#lx.\n", up_node, output, hr);
            continue;
        }

        while (stream->requests)
        {
            if (FAILED(hr = transform_stream_pop_sample(stream, &sample)))
            {
                /* try getting more samples by calling IMFTransform_ProcessOutput */
                if (FAILED(hr = transform_node_pull_samples(session, topo_node)))
                    break;
                if (FAILED(hr = transform_stream_pop_sample(stream, &sample)))
                    break;
            }

            session_deliver_sample_to_node(session, down_node, input, sample);
            stream->requests--;
            IMFSample_Release(sample);
        }

        while (stream->requests && drained)
        {
            session_deliver_sample_to_node(session, down_node, input, NULL);
            stream->requests--;
        }

        IMFTopologyNode_Release(down_node);
    }

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && transform_node_has_requests(topo_node))
    {
        struct transform_stream *stream;

        input = topo_node->u.transform.next_input++ % topo_node->u.transform.input_count;
        stream = &topo_node->u.transform.inputs[input];

        if (SUCCEEDED(transform_stream_pop_sample(stream, &sample)))
            session_deliver_sample_to_node(session, topo_node->node, input, sample);
        else if (FAILED(hr = IMFTopologyNode_GetInput(topo_node->node, input, &up_node, &output)))
            WARN("Failed to get node %p/%lu input, hr %#lx\n", topo_node->node, input, hr);
        else
        {
            if (FAILED(hr = session_request_sample_from_node(session, up_node, output)))
                WARN("Failed to request sample from upstream node %p/%lu, hr %#lx\n", up_node, output, hr);
            IMFTopologyNode_Release(up_node);
        }
    }
}

static void session_deliver_sample_to_node(struct media_session *session, IMFTopologyNode *node, unsigned int input,
        IMFSample *sample)
{
    struct topo_node *topo_node;
    MF_TOPOLOGY_TYPE node_type;
    TOPOID node_id;
    HRESULT hr;

    if (session->quality_manager)
        IMFQualityManager_NotifyProcessInput(session->quality_manager, node, input, sample);

    IMFTopologyNode_GetNodeType(node, &node_type);
    IMFTopologyNode_GetTopoNodeID(node, &node_id);

    topo_node = session_get_node_by_id(session, node_id);

    switch (node_type)
    {
        case MF_TOPOLOGY_OUTPUT_NODE:
            if (topo_node->u.sink.requests)
            {
                if (sample)
                {
                    if (FAILED(hr = IMFStreamSink_ProcessSample(topo_node->object.sink_stream, sample)))
                        WARN("Stream sink failed to process sample, hr %#lx.\n", hr);
                }
                else if (FAILED(hr = IMFStreamSink_PlaceMarker(topo_node->object.sink_stream, MFSTREAMSINK_MARKER_ENDOFSEGMENT,
                        NULL, NULL)))
                {
                    WARN("Failed to place sink marker, hr %#lx.\n", hr);
                }
                topo_node->u.sink.requests--;
            }
            break;
        case MF_TOPOLOGY_TRANSFORM_NODE:
            if (FAILED(hr = transform_node_push_sample(session, topo_node, input, sample)))
                WARN("Failed to push or queue sample to transform, hr %#lx\n", hr);
            transform_node_pull_samples(session, topo_node);
            transform_node_deliver_samples(session, topo_node);
            break;
        case MF_TOPOLOGY_TEE_NODE:
            FIXME("Unhandled downstream node type %d.\n", node_type);
            break;
        default:
            ;
    }
}

static void session_deliver_pending_samples(struct media_session *session, IMFTopologyNode *node)
{
    struct topo_node *topo_node;
    MF_TOPOLOGY_TYPE node_type;
    TOPOID node_id;

    IMFTopologyNode_GetNodeType(node, &node_type);
    IMFTopologyNode_GetTopoNodeID(node, &node_id);

    topo_node = session_get_node_by_id(session, node_id);

    switch (node_type)
    {
        case MF_TOPOLOGY_TRANSFORM_NODE:
            transform_node_pull_samples(session, topo_node);
            transform_node_deliver_samples(session, topo_node);
            break;
        default:
            FIXME("Unexpected node type %u.\n", node_type);
    }
}


static HRESULT session_request_sample_from_node(struct media_session *session, IMFTopologyNode *node, DWORD output)
{
    IMFTopologyNode *down_node;
    struct topo_node *topo_node;
    MF_TOPOLOGY_TYPE node_type;
    HRESULT hr = S_OK;
    IMFSample *sample;
    TOPOID node_id;
    DWORD input;

    IMFTopologyNode_GetNodeType(node, &node_type);
    IMFTopologyNode_GetTopoNodeID(node, &node_id);

    topo_node = session_get_node_by_id(session, node_id);

    switch (node_type)
    {
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            if (FAILED(hr = IMFMediaStream_RequestSample(topo_node->object.source_stream, NULL)))
                WARN("Sample request failed, hr %#lx.\n", hr);
            break;
        case MF_TOPOLOGY_TRANSFORM_NODE:
        {
            struct transform_stream *stream = &topo_node->u.transform.outputs[output];

            if (FAILED(hr = IMFTopologyNode_GetOutput(node, output, &down_node, &input)))
            {
                WARN("Failed to node %p/%lu output, hr %#lx.\n", node, output, hr);
                break;
            }

            if (SUCCEEDED(transform_stream_pop_sample(stream, &sample)))
            {
                session_deliver_sample_to_node(session, down_node, input, sample);
                IMFSample_Release(sample);
            }
            else if (transform_node_has_requests(topo_node))
            {
                /* there's already requests pending, just increase the counter */
                stream->requests++;
            }
            else
            {
                stream->requests++;
                transform_node_deliver_samples(session, topo_node);
            }

            IMFTopologyNode_Release(down_node);
            break;
        }
        case MF_TOPOLOGY_TEE_NODE:
            FIXME("Unhandled upstream node type %d.\n", node_type);
        default:
            hr = E_UNEXPECTED;
    }

    return hr;
}

static void session_request_sample(struct media_session *session, IMFStreamSink *sink_stream)
{
    struct topo_node *sink_node = NULL, *node;
    IMFTopologyNode *upstream_node;
    DWORD upstream_output;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_OUTPUT_NODE && node->object.sink_stream == sink_stream)
        {
            sink_node = node;
            break;
        }
    }

    if (!sink_node)
        return;

    if (FAILED(hr = IMFTopologyNode_GetInput(sink_node->node, 0, &upstream_node, &upstream_output)))
    {
        WARN("Failed to get upstream node connection, hr %#lx.\n", hr);
        return;
    }

    sink_node->u.sink.requests++;
    if (FAILED(session_request_sample_from_node(session, upstream_node, upstream_output)))
        sink_node->u.sink.requests--;
    IMFTopologyNode_Release(upstream_node);
}

static void session_deliver_sample(struct media_session *session, IMFMediaStream *stream, const PROPVARIANT *value)
{
    struct topo_node *source_node = NULL, *node;
    IMFTopologyNode *downstream_node;
    DWORD downstream_input;
    HRESULT hr;

    if (value && (value->vt != VT_UNKNOWN || !value->punkVal))
    {
        WARN("Unexpected value type %d.\n", value->vt);
        return;
    }

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_SOURCESTREAM_NODE && node->object.source_stream == stream)
        {
            source_node = node;
            break;
        }
    }

    if (!source_node)
        return;

    if (!value)
        source_node->flags |= TOPO_NODE_END_OF_STREAM;

    if (FAILED(hr = IMFTopologyNode_GetOutput(source_node->node, 0, &downstream_node, &downstream_input)))
    {
        WARN("Failed to get downstream node connection, hr %#lx.\n", hr);
        return;
    }

    session_deliver_sample_to_node(session, downstream_node, downstream_input, value ? (IMFSample *)value->punkVal : NULL);
    IMFTopologyNode_Release(downstream_node);
}

static void session_sink_invalidated(struct media_session *session, IMFMediaEvent *event, IMFStreamSink *sink)
{
    struct topo_node *node, *sink_node = NULL;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
    {
        if (node->type == MF_TOPOLOGY_OUTPUT_NODE && node->object.sink_stream == sink)
        {
            sink_node = node;
            break;
        }
    }

    if (!sink_node)
        return;

    if (!event)
    {
        if (FAILED(hr = MFCreateMediaEvent(MESinkInvalidated, &GUID_NULL, S_OK, NULL, &event)))
            WARN("Failed to create event, hr %#lx.\n", hr);
    }

    if (!event)
        return;

    IMFMediaEvent_SetUINT64(event, &MF_EVENT_OUTPUT_NODE, sink_node->node_id);
    IMFMediaEventQueue_QueueEvent(session->event_queue, event);

    session_set_topo_status(session, S_OK, MF_TOPOSTATUS_ENDED);
}

static BOOL session_nodes_is_mask_set(struct media_session *session, MF_TOPOLOGY_TYPE node_type, unsigned int flags)
{
    struct media_source *source;
    struct topo_node *node;

    if (node_type == MF_TOPOLOGY_MAX)
    {
        LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
        {
            if ((source->flags & flags) != flags)
                return FALSE;
        }
    }
    else
    {
        LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
        {
            if (node->type == node_type && (node->flags & flags) != flags)
                return FALSE;
        }
    }

    return TRUE;
}

static void session_nodes_unset_mask(struct media_session *session, MF_TOPOLOGY_TYPE node_type, unsigned int flags)
{
    struct media_source *source;
    struct topo_node *node;

    if (node_type == MF_TOPOLOGY_MAX)
    {
        LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
        {
            source->flags &= ~flags;
        }
    }
    else
    {
        LIST_FOR_EACH_ENTRY(node, &session->presentation.nodes, struct topo_node, entry)
        {
            if (node->type == node_type)
            {
                node->flags &= ~flags;
            }
        }
    }
}

static void session_raise_end_of_presentation(struct media_session *session)
{
    if (!(session_nodes_is_mask_set(session, MF_TOPOLOGY_SOURCESTREAM_NODE, TOPO_NODE_END_OF_STREAM)))
        return;

    if (!(session->presentation.flags & SESSION_FLAG_END_OF_PRESENTATION))
    {
        if (session_nodes_is_mask_set(session, MF_TOPOLOGY_MAX, SOURCE_FLAG_END_OF_PRESENTATION))
        {
            session->presentation.flags |= SESSION_FLAG_END_OF_PRESENTATION | SESSION_FLAG_PENDING_COMMAND;
            IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MEEndOfPresentation, &GUID_NULL, S_OK, NULL);
        }
    }
}

static void session_handle_end_of_stream(struct media_session *session, IMFMediaStream *stream)
{
    struct topo_node *node;

    if (!(node = session_get_node_object(session, (IUnknown *)stream, MF_TOPOLOGY_SOURCESTREAM_NODE))
            || node->flags & TOPO_NODE_END_OF_STREAM)
    {
        return;
    }

    session_deliver_sample(session, stream, NULL);

    session_raise_end_of_presentation(session);
}

static void session_handle_end_of_presentation(struct media_session *session, IMFMediaSource *object)
{
    struct media_source *source;

    LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
    {
        if (source->source == object)
        {
            if (!(source->flags & SOURCE_FLAG_END_OF_PRESENTATION))
            {
                source->flags |= SOURCE_FLAG_END_OF_PRESENTATION;
                session_raise_end_of_presentation(session);
            }

            break;
        }
    }
}

static void session_sink_stream_marker(struct media_session *session, IMFStreamSink *stream_sink)
{
    struct topo_node *node;

    if (!(node = session_get_node_object(session, (IUnknown *)stream_sink, MF_TOPOLOGY_OUTPUT_NODE))
            || node->flags & TOPO_NODE_END_OF_STREAM)
    {
        return;
    }

    node->flags |= TOPO_NODE_END_OF_STREAM;

    if (session->presentation.flags & SESSION_FLAG_END_OF_PRESENTATION &&
            session_nodes_is_mask_set(session, MF_TOPOLOGY_OUTPUT_NODE, TOPO_NODE_END_OF_STREAM))
    {
        session_set_topo_status(session, S_OK, MF_TOPOSTATUS_ENDED);
        session_set_caps(session, session->caps & ~MFSESSIONCAP_PAUSE);
        session_stop(session);
    }
}

static void session_sink_stream_scrub_complete(struct media_session *session, IMFStreamSink *stream_sink)
{
    struct topo_node *node;

    if (!(node = session_get_node_object(session, (IUnknown *)stream_sink, MF_TOPOLOGY_OUTPUT_NODE))
            || node->flags & TOPO_NODE_SCRUB_SAMPLE_COMPLETE)
    {
        return;
    }

    node->flags |= TOPO_NODE_SCRUB_SAMPLE_COMPLETE;

    /* Scrubbing event is not limited to the started state transition, or even the started state.
       Events are processed and forwarded at any point after transition from initial idle state. */
    if (session->presentation.flags & SESSION_FLAG_SOURCES_SUBSCRIBED &&
            session_nodes_is_mask_set(session, MF_TOPOLOGY_OUTPUT_NODE, TOPO_NODE_SCRUB_SAMPLE_COMPLETE))
    {
        IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MESessionScrubSampleComplete, &GUID_NULL, S_OK, NULL);
        session_nodes_unset_mask(session, MF_TOPOLOGY_OUTPUT_NODE, TOPO_NODE_SCRUB_SAMPLE_COMPLETE);
    }
}

static HRESULT WINAPI session_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_session *session = impl_from_events_callback_IMFAsyncCallback(iface);
    IMFMediaEventGenerator *event_source;
    IMFMediaEvent *event = NULL;
    MediaEventType event_type;
    IUnknown *object = NULL;
    IMFMediaSource *source;
    IMFMediaStream *stream;
    PROPVARIANT value;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetState(result, (IUnknown **)&event_source)))
        return hr;

    if (FAILED(hr = IMFMediaEventGenerator_EndGetEvent(event_source, result, &event)))
    {
        WARN("Failed to get event from %p, hr %#lx.\n", event_source, hr);
        IMFMediaEventQueue_QueueEventParamVar(session->event_queue, MEError, &GUID_NULL, hr, NULL);
        IMFMediaEventGenerator_Release(event_source);
        return hr;
    }

    if (FAILED(hr = IMFMediaEvent_GetType(event, &event_type)))
    {
        WARN("Failed to get event type, hr %#lx.\n", hr);
        goto failed;
    }

    value.vt = VT_EMPTY;
    if (FAILED(hr = IMFMediaEvent_GetValue(event, &value)))
    {
        WARN("Failed to get event value, hr %#lx.\n", hr);
        goto failed;
    }

    switch (event_type)
    {
        case MESourceSeeked:
        case MEStreamSeeked:
            FIXME("Source/stream seeking, semi-stub!\n");
            /* fallthrough */
        case MESourceStarted:
        case MESourcePaused:
        case MESourceStopped:
        case MEStreamStarted:
        case MEStreamPaused:
        case MEStreamStopped:

            EnterCriticalSection(&session->cs);
            session_set_source_object_state(session, (IUnknown *)event_source, event_type);
            LeaveCriticalSection(&session->cs);

            break;

        case MESourceRateChanged:

            EnterCriticalSection(&session->cs);
            session_complete_rate_change(session);
            LeaveCriticalSection(&session->cs);

            break;

        case MEBufferingStarted:
        case MEBufferingStopped:

            EnterCriticalSection(&session->cs);
            if (session_get_media_source(session, (IMFMediaSource *)event_source))
            {
                if (event_type == MEBufferingStarted)
                    IMFPresentationClock_Pause(session->clock);
                else
                    IMFPresentationClock_Start(session->clock, PRESENTATION_CURRENT_POSITION);

                IMFMediaEventQueue_QueueEvent(session->event_queue, event);
            }
            LeaveCriticalSection(&session->cs);
            break;

        case MEReconnectStart:
        case MEReconnectEnd:

            EnterCriticalSection(&session->cs);
            if (session_get_media_source(session, (IMFMediaSource *)event_source))
                IMFMediaEventQueue_QueueEvent(session->event_queue, event);
            LeaveCriticalSection(&session->cs);
            break;

        case MEExtendedType:
        case MERendererEvent:
        case MEStreamSinkFormatChanged:

            IMFMediaEventQueue_QueueEvent(session->event_queue, event);
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
            if (SUCCEEDED(hr = session_add_media_stream(session, source, stream)))
                hr = IMFMediaStream_BeginGetEvent(stream, &session->events_callback, (IUnknown *)stream);
            LeaveCriticalSection(&session->cs);

            IMFMediaSource_Release(source);

            break;
        case MEStreamSinkStarted:
        case MEStreamSinkPaused:
        case MEStreamSinkStopped:
        case MEStreamSinkPrerolled:

            EnterCriticalSection(&session->cs);
            session_set_sink_stream_state(session, (IMFStreamSink *)event_source, event_type);
            LeaveCriticalSection(&session->cs);

            break;
        case MEStreamSinkMarker:

            EnterCriticalSection(&session->cs);
            session_sink_stream_marker(session, (IMFStreamSink *)event_source);
            LeaveCriticalSection(&session->cs);

            break;
        case MEStreamSinkRequestSample:

            EnterCriticalSection(&session->cs);
            session_request_sample(session, (IMFStreamSink *)event_source);
            LeaveCriticalSection(&session->cs);

            break;
        case MEStreamSinkScrubSampleComplete:

            EnterCriticalSection(&session->cs);
            session_sink_stream_scrub_complete(session, (IMFStreamSink *)event_source);
            LeaveCriticalSection(&session->cs);
            break;
        case MEMediaSample:

            EnterCriticalSection(&session->cs);
            session_deliver_sample(session, (IMFMediaStream *)event_source, &value);
            LeaveCriticalSection(&session->cs);

            break;
        case MEEndOfStream:

            EnterCriticalSection(&session->cs);
            session_handle_end_of_stream(session, (IMFMediaStream *)event_source);
            LeaveCriticalSection(&session->cs);

            break;

        case MEEndOfPresentation:

            EnterCriticalSection(&session->cs);
            session_handle_end_of_presentation(session, (IMFMediaSource *)event_source);
            LeaveCriticalSection(&session->cs);

            break;
        case MEAudioSessionGroupingParamChanged:
        case MEAudioSessionIconChanged:
        case MEAudioSessionNameChanged:
        case MEAudioSessionVolumeChanged:

            IMFMediaEventQueue_QueueEvent(session->event_queue, event);

            break;
        case MEAudioSessionDeviceRemoved:
        case MEAudioSessionDisconnected:
        case MEAudioSessionExclusiveModeOverride:
        case MEAudioSessionFormatChanged:
        case MEAudioSessionServerShutdown:

            IMFMediaEventQueue_QueueEvent(session->event_queue, event);
            /* fallthrough */
        case MESinkInvalidated:

            EnterCriticalSection(&session->cs);
            session_sink_invalidated(session, event_type == MESinkInvalidated ? event : NULL,
                    (IMFStreamSink *)event_source);
            LeaveCriticalSection(&session->cs);

            break;
        case MEQualityNotify:

            if (session->quality_manager)
            {
                if (FAILED(IMFMediaEventGenerator_QueryInterface(event_source, &IID_IMFStreamSink, (void **)&object)))
                    IMFMediaEventGenerator_QueryInterface(event_source, &IID_IMFTransform, (void **)&object);

                if (object)
                {
                    IMFQualityManager_NotifyQualityEvent(session->quality_manager, object, event);
                    IUnknown_Release(object);
                }
            }

            break;
        default:
            ;
    }

    PropVariantClear(&value);

failed:

    if (FAILED(hr = IMFMediaEventGenerator_BeginGetEvent(event_source, iface, (IUnknown *)event_source)))
    {
        WARN("Failed to re-subscribe, hr %#lx.\n", hr);
        IMFMediaEventQueue_QueueEvent(session->event_queue, event);
    }

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

static HRESULT WINAPI session_sink_finalizer_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
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

static ULONG WINAPI session_sink_finalizer_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_sink_finalizer_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI session_sink_finalizer_callback_Release(IMFAsyncCallback *iface)
{
    struct media_session *session = impl_from_sink_finalizer_callback_IMFAsyncCallback(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI session_sink_finalizer_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI session_sink_finalizer_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_session *session = impl_from_sink_finalizer_callback_IMFAsyncCallback(iface);
    IMFFinalizableMediaSink *fin_sink = NULL;
    BOOL sinks_finalized = TRUE;
    struct media_sink *sink;
    IUnknown *state;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetState(result, &state)))
        return hr;

    EnterCriticalSection(&session->cs);

    LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
    {
        if (state == sink->object)
        {
            if (FAILED(hr = IMFMediaSink_QueryInterface(sink->sink, &IID_IMFFinalizableMediaSink, (void **)&fin_sink)))
                WARN("Unexpected, missing IMFFinalizableMediaSink, hr %#lx.\n", hr);
        }
        else
        {
            sinks_finalized &= sink->finalized;
            if (!sinks_finalized)
                break;
        }
    }

    IUnknown_Release(state);

    if (fin_sink)
    {
        /* Complete session transition, or close prematurely on error. */
        if (SUCCEEDED(hr = IMFFinalizableMediaSink_EndFinalize(fin_sink, result)))
        {
            sink->finalized = TRUE;
            if (sinks_finalized)
                session_set_closed(session, hr);
        }
        IMFFinalizableMediaSink_Release(fin_sink);
    }

    LeaveCriticalSection(&session->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl session_sink_finalizer_callback_vtbl =
{
    session_sink_finalizer_callback_QueryInterface,
    session_sink_finalizer_callback_AddRef,
    session_sink_finalizer_callback_Release,
    session_sink_finalizer_callback_GetParameters,
    session_sink_finalizer_callback_Invoke,
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

static HRESULT session_presentation_object_get_rate(IUnknown *object, MFRATE_DIRECTION direction,
        BOOL thin, BOOL fastest, float *result)
{
    IMFRateSupport *rate_support;
    float rate;
    HRESULT hr;

    /* For objects that don't support rate control, it's assumed that only forward direction is allowed, at 1.0f. */

    if (FAILED(hr = MFGetService(object, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void **)&rate_support)))
    {
        if (direction == MFRATE_FORWARD)
        {
            *result = 1.0f;
            return S_OK;
        }
        else
            return MF_E_REVERSE_UNSUPPORTED;
    }

    rate = 0.0f;
    if (fastest)
    {
        if (SUCCEEDED(hr = IMFRateSupport_GetFastestRate(rate_support, direction, thin, &rate)))
            *result = min(fabsf(rate), *result);
    }
    else
    {
        if (SUCCEEDED(hr = IMFRateSupport_GetSlowestRate(rate_support, direction, thin, &rate)))
            *result = max(fabsf(rate), *result);
    }

    IMFRateSupport_Release(rate_support);

    return hr;
}

static HRESULT session_get_presentation_rate(struct media_session *session, MFRATE_DIRECTION direction,
        BOOL thin, BOOL fastest, float *result)
{
    struct media_source *source;
    struct media_sink *sink;
    HRESULT hr = E_POINTER;
    float rate;

    rate = fastest ? FLT_MAX : 0.0f;

    EnterCriticalSection(&session->cs);

    if (session->presentation.topo_status != MF_TOPOSTATUS_INVALID)
    {
        LIST_FOR_EACH_ENTRY(source, &session->presentation.sources, struct media_source, entry)
        {
            if (FAILED(hr = session_presentation_object_get_rate(source->object, direction, thin, fastest, &rate)))
                break;
        }

        if (SUCCEEDED(hr))
        {
            LIST_FOR_EACH_ENTRY(sink, &session->presentation.sinks, struct media_sink, entry)
            {
                if (FAILED(hr = session_presentation_object_get_rate(sink->object, direction, thin, fastest, &rate)))
                    break;
            }
        }
    }

    LeaveCriticalSection(&session->cs);

    if (SUCCEEDED(hr))
        *result = direction == MFRATE_FORWARD ? rate : -rate;

    return hr;
}

static HRESULT WINAPI session_rate_support_GetSlowestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    struct media_session *session = impl_session_from_IMFRateSupport(iface);

    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    return session_get_presentation_rate(session, direction, thin, FALSE, rate);
}

static HRESULT WINAPI session_rate_support_GetFastestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    struct media_session *session = impl_session_from_IMFRateSupport(iface);

    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    return session_get_presentation_rate(session, direction, thin, TRUE, rate);
}

static HRESULT WINAPI session_rate_support_IsRateSupported(IMFRateSupport *iface, BOOL thin, float rate,
        float *nearest_supported_rate)
{
    struct media_session *session = impl_session_from_IMFRateSupport(iface);
    HRESULT hr;

    TRACE("%p, %d, %f, %p.\n", iface, thin, rate, nearest_supported_rate);

    EnterCriticalSection(&session->cs);
    hr = session_is_presentation_rate_supported(session, thin, rate, nearest_supported_rate);
    LeaveCriticalSection(&session->cs);

    return hr;
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
    struct media_session *session = impl_session_from_IMFRateControl(iface);
    struct session_op *op;
    HRESULT hr;

    TRACE("%p, %d, %f.\n", iface, thin, rate);

    if (FAILED(hr = create_session_op(SESSION_CMD_SET_RATE, &op)))
        return hr;

    op->set_rate.thin = thin;
    op->set_rate.rate = rate;
    hr = session_submit_command(session, op);
    IUnknown_Release(&op->IUnknown_iface);
    return hr;
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

static HRESULT WINAPI node_attribute_editor_QueryInterface(IMFTopologyNodeAttributeEditor *iface,
        REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFTopologyNodeAttributeEditor) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFTopologyNodeAttributeEditor_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI node_attribute_editor_AddRef(IMFTopologyNodeAttributeEditor *iface)
{
    struct media_session *session = impl_session_from_IMFTopologyNodeAttributeEditor(iface);
    return IMFMediaSession_AddRef(&session->IMFMediaSession_iface);
}

static ULONG WINAPI node_attribute_editor_Release(IMFTopologyNodeAttributeEditor *iface)
{
    struct media_session *session = impl_session_from_IMFTopologyNodeAttributeEditor(iface);
    return IMFMediaSession_Release(&session->IMFMediaSession_iface);
}

static HRESULT WINAPI node_attribute_editor_UpdateNodeAttributes(IMFTopologyNodeAttributeEditor *iface,
        TOPOID id, DWORD count, MFTOPONODE_ATTRIBUTE_UPDATE *updates)
{
    FIXME("%p, %s, %lu, %p.\n", iface, wine_dbgstr_longlong(id), count, updates);

    return E_NOTIMPL;
}

static const IMFTopologyNodeAttributeEditorVtbl node_attribute_editor_vtbl =
{
    node_attribute_editor_QueryInterface,
    node_attribute_editor_AddRef,
    node_attribute_editor_Release,
    node_attribute_editor_UpdateNodeAttributes,
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

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaSession_iface.lpVtbl = &mfmediasessionvtbl;
    object->IMFGetService_iface.lpVtbl = &session_get_service_vtbl;
    object->IMFRateSupport_iface.lpVtbl = &session_rate_support_vtbl;
    object->IMFRateControl_iface.lpVtbl = &session_rate_control_vtbl;
    object->IMFTopologyNodeAttributeEditor_iface.lpVtbl = &node_attribute_editor_vtbl;
    object->commands_callback.lpVtbl = &session_commands_callback_vtbl;
    object->sa_ready_callback.lpVtbl = &session_sa_ready_callback_vtbl;
    object->events_callback.lpVtbl = &session_events_callback_vtbl;
    object->sink_finalizer_callback.lpVtbl = &session_sink_finalizer_callback_vtbl;
    object->refcount = 1;
    list_init(&object->topologies);
    list_init(&object->commands);
    list_init(&object->presentation.sources);
    list_init(&object->presentation.sinks);
    list_init(&object->presentation.nodes);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateTopology(&object->presentation.current_topology)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreatePresentationClock(&object->clock)))
        goto failed;

    if (FAILED(hr = MFCreateSystemTimeSource(&object->system_time_source)))
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
                WARN("Failed to create custom topology loader, hr %#lx.\n", hr);
            }
        }

        if (SUCCEEDED(IMFAttributes_GetGUID(config, &MF_SESSION_QUALITY_MANAGER, &clsid)))
        {
            if (!(without_quality_manager = IsEqualGUID(&clsid, &GUID_NULL)))
            {
                if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFQualityManager,
                        (void **)&object->quality_manager)))
                {
                    WARN("Failed to create custom quality manager, hr %#lx.\n", hr);
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

    if (object->quality_manager && FAILED(hr = IMFQualityManager_NotifyPresentationClock(object->quality_manager,
            object->clock)))
    {
        goto failed;
    }

    *session = &object->IMFMediaSession_iface;

    return S_OK;

failed:
    IMFMediaSession_Release(&object->IMFMediaSession_iface);
    return hr;
}
