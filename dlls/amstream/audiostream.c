/*
 * Primary audio stream
 *
 * Copyright 2012 Christian Costa
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
#include "amstream_private.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

static const WCHAR sink_id[] = L"I{A35FF56B-9FDA-11D0-8FDF-00C04FD9189D}";

typedef struct {
    IAudioStreamSample IAudioStreamSample_iface;
    LONG ref;
    IMediaStream *parent;
    IAudioData *audio_data;
} IAudioStreamSampleImpl;

static inline IAudioStreamSampleImpl *impl_from_IAudioStreamSample(IAudioStreamSample *iface)
{
    return CONTAINING_RECORD(iface, IAudioStreamSampleImpl, IAudioStreamSample_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_QueryInterface(IAudioStreamSample *iface,
        REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IStreamSample) ||
        IsEqualGUID(riid, &IID_IAudioStreamSample))
    {
        IAudioStreamSample_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    ERR("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAudioStreamSampleImpl_AddRef(IAudioStreamSample *iface)
{
    IAudioStreamSampleImpl *This = impl_from_IAudioStreamSample(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IAudioStreamSampleImpl_Release(IAudioStreamSample *iface)
{
    IAudioStreamSampleImpl *This = impl_from_IAudioStreamSample(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IStreamSample methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_GetMediaStream(IAudioStreamSample *iface, IMediaStream **media_stream)
{
    FIXME("(%p)->(%p): stub\n", iface, media_stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_GetSampleTimes(IAudioStreamSample *iface, STREAM_TIME *start_time,
                                                                 STREAM_TIME *end_time, STREAM_TIME *current_time)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, start_time, end_time, current_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_SetSampleTimes(IAudioStreamSample *iface, const STREAM_TIME *start_time,
                                                                 const STREAM_TIME *end_time)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, start_time, end_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_Update(IAudioStreamSample *iface, DWORD flags, HANDLE event,
                                                         PAPCFUNC func_APC, DWORD APC_data)
{
    FIXME("(%p)->(%x,%p,%p,%u): stub\n", iface, flags, event, func_APC, APC_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_CompletionStatus(IAudioStreamSample *iface, DWORD flags, DWORD milliseconds)
{
    FIXME("(%p)->(%x,%u): stub\n", iface, flags, milliseconds);

    return E_NOTIMPL;
}

/*** IAudioStreamSample methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_GetAudioData(IAudioStreamSample *iface, IAudioData **audio_data)
{
    FIXME("(%p)->(%p): stub\n", iface, audio_data);

    return E_NOTIMPL;
}

static const struct IAudioStreamSampleVtbl AudioStreamSample_Vtbl =
{
    /*** IUnknown methods ***/
    IAudioStreamSampleImpl_QueryInterface,
    IAudioStreamSampleImpl_AddRef,
    IAudioStreamSampleImpl_Release,
    /*** IStreamSample methods ***/
    IAudioStreamSampleImpl_GetMediaStream,
    IAudioStreamSampleImpl_GetSampleTimes,
    IAudioStreamSampleImpl_SetSampleTimes,
    IAudioStreamSampleImpl_Update,
    IAudioStreamSampleImpl_CompletionStatus,
    /*** IAudioStreamSample methods ***/
    IAudioStreamSampleImpl_GetAudioData
};

static HRESULT audiostreamsample_create(IAudioMediaStream *parent, IAudioData *audio_data, IAudioStreamSample **audio_stream_sample)
{
    IAudioStreamSampleImpl *object;

    TRACE("(%p)\n", audio_stream_sample);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAudioStreamSampleImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAudioStreamSample_iface.lpVtbl = &AudioStreamSample_Vtbl;
    object->ref = 1;
    object->parent = (IMediaStream*)parent;
    object->audio_data = audio_data;

    *audio_stream_sample = &object->IAudioStreamSample_iface;

    return S_OK;
}

struct audio_stream
{
    IAMMediaStream IAMMediaStream_iface;
    IAudioMediaStream IAudioMediaStream_iface;
    IMemInputPin IMemInputPin_iface;
    IPin IPin_iface;
    LONG ref;

    IMultiMediaStream* parent;
    MSPID purpose_id;
    STREAM_TYPE stream_type;
    CRITICAL_SECTION cs;
    IMediaStreamFilter *filter;

    IPin *peer;
    IMemAllocator *allocator;
    AM_MEDIA_TYPE mt;
};

static inline struct audio_stream *impl_from_IAMMediaStream(IAMMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IAMMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI audio_IAMMediaStream_QueryInterface(IAMMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream) ||
        IsEqualGUID(riid, &IID_IAMMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IAudioMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->IAudioMediaStream_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->IPin_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IMemInputPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->IMemInputPin_iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_IAMMediaStream_AddRef(IAMMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI audio_IAMMediaStream_Release(IAMMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    if (!ref)
    {
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI audio_IAMMediaStream_GetMultiMediaStream(IAMMediaStream *iface,
        IMultiMediaStream** multi_media_stream)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_GetInformation(IAMMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_SetSameFormat(IAMMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD flags)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, flags);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_AllocateSample(IAMMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_CreateSharedSample(IAMMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_SendEndOfStream(IAMMediaStream *iface, DWORD flags)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, flags);

    return S_FALSE;
}

/*** IAMMediaStream methods ***/
static HRESULT WINAPI audio_IAMMediaStream_Initialize(IAMMediaStream *iface, IUnknown *source_object, DWORD flags,
                                                    REFMSPID purpose_id, const STREAM_TYPE stream_type)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p,%u) stub!\n", This, iface, source_object, flags, purpose_id, stream_type);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_SetState(IAMMediaStream *iface, FILTER_STATE state)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%u) stub!\n", This, iface, state);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_JoinAMMultiMediaStream(IAMMediaStream *iface,
        IAMMultiMediaStream *am_multi_media_stream)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, am_multi_media_stream);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *filter)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, filter %p.\n", stream, filter);

    stream->filter = filter;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *filtergraph)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, filtergraph);

    return S_FALSE;
}

static const struct IAMMediaStreamVtbl audio_IAMMediaStream_vtbl =
{
    audio_IAMMediaStream_QueryInterface,
    audio_IAMMediaStream_AddRef,
    audio_IAMMediaStream_Release,
    audio_IAMMediaStream_GetMultiMediaStream,
    audio_IAMMediaStream_GetInformation,
    audio_IAMMediaStream_SetSameFormat,
    audio_IAMMediaStream_AllocateSample,
    audio_IAMMediaStream_CreateSharedSample,
    audio_IAMMediaStream_SendEndOfStream,
    audio_IAMMediaStream_Initialize,
    audio_IAMMediaStream_SetState,
    audio_IAMMediaStream_JoinAMMultiMediaStream,
    audio_IAMMediaStream_JoinFilter,
    audio_IAMMediaStream_JoinFilterGraph,
};

static inline struct audio_stream *impl_from_IAudioMediaStream(IAudioMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IAudioMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI audio_IAudioMediaStream_QueryInterface(IAudioMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);
    return IAMMediaStream_QueryInterface(&This->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI audio_IAudioMediaStream_AddRef(IAudioMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_AddRef(&This->IAMMediaStream_iface);
}

static ULONG WINAPI audio_IAudioMediaStream_Release(IAudioMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_Release(&This->IAMMediaStream_iface);
}

/*** IMediaStream methods ***/
static HRESULT WINAPI audio_IAudioMediaStream_GetMultiMediaStream(IAudioMediaStream *iface,
        IMultiMediaStream **multi_media_stream)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI audio_IAudioMediaStream_GetInformation(IAudioMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", iface, This, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI audio_IAudioMediaStream_SetSameFormat(IAudioMediaStream *iface,
        IMediaStream *stream_format, DWORD flags)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", iface, This, stream_format, flags);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAudioMediaStream_AllocateSample(IAudioMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", iface, This, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAudioMediaStream_CreateSharedSample(IAudioMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", iface, This, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAudioMediaStream_SendEndOfStream(IAudioMediaStream *iface,
        DWORD flags)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", iface, This, flags);

    return S_FALSE;
}

/*** IAudioMediaStream methods ***/
static HRESULT WINAPI audio_IAudioMediaStream_GetFormat(IAudioMediaStream *iface, WAVEFORMATEX *wave_format_current)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format_current);

    if (!wave_format_current)
        return E_POINTER;

    return MS_E_NOSTREAM;

}

static HRESULT WINAPI audio_IAudioMediaStream_SetFormat(IAudioMediaStream *iface, const WAVEFORMATEX *wave_format)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_IAudioMediaStream_CreateSample(IAudioMediaStream *iface, IAudioData *audio_data,
                                                         DWORD flags, IAudioStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%u,%p)\n", iface, This, audio_data, flags, sample);

    if (!audio_data)
        return E_POINTER;

    return audiostreamsample_create(iface, audio_data, sample);
}

static const struct IAudioMediaStreamVtbl audio_IAudioMediaStream_vtbl =
{
    audio_IAudioMediaStream_QueryInterface,
    audio_IAudioMediaStream_AddRef,
    audio_IAudioMediaStream_Release,
    audio_IAudioMediaStream_GetMultiMediaStream,
    audio_IAudioMediaStream_GetInformation,
    audio_IAudioMediaStream_SetSameFormat,
    audio_IAudioMediaStream_AllocateSample,
    audio_IAudioMediaStream_CreateSharedSample,
    audio_IAudioMediaStream_SendEndOfStream,
    audio_IAudioMediaStream_GetFormat,
    audio_IAudioMediaStream_SetFormat,
    audio_IAudioMediaStream_CreateSample,
};

struct enum_media_types
{
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refcount;
    unsigned int index;
};

static const IEnumMediaTypesVtbl enum_media_types_vtbl;

static struct enum_media_types *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, struct enum_media_types, IEnumMediaTypes_iface);
}

static HRESULT WINAPI enum_media_types_QueryInterface(IEnumMediaTypes *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumMediaTypes))
    {
        IEnumMediaTypes_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_media_types_AddRef(IEnumMediaTypes *iface)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);
    ULONG refcount = InterlockedIncrement(&enum_media_types->refcount);
    TRACE("%p increasing refcount to %u.\n", enum_media_types, refcount);
    return refcount;
}

static ULONG WINAPI enum_media_types_Release(IEnumMediaTypes *iface)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);
    ULONG refcount = InterlockedDecrement(&enum_media_types->refcount);
    TRACE("%p decreasing refcount to %u.\n", enum_media_types, refcount);
    if (!refcount)
        heap_free(enum_media_types);
    return refcount;
}

static HRESULT WINAPI enum_media_types_Next(IEnumMediaTypes *iface, ULONG count, AM_MEDIA_TYPE **mts, ULONG *ret_count)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);

    TRACE("iface %p, count %u, mts %p, ret_count %p.\n", iface, count, mts, ret_count);

    if (!ret_count)
        return E_POINTER;

    if (count && !enum_media_types->index)
    {
        mts[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        memset(mts[0], 0, sizeof(AM_MEDIA_TYPE));
        mts[0]->majortype = MEDIATYPE_Audio;
        mts[0]->subtype = MEDIASUBTYPE_PCM;
        ++enum_media_types->index;
        *ret_count = 1;
        return count == 1 ? S_OK : S_FALSE;
    }

    *ret_count = 0;
    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_media_types_Skip(IEnumMediaTypes *iface, ULONG count)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);

    TRACE("iface %p, count %u.\n", iface, count);

    enum_media_types->index += count;
    return S_OK;
}

static HRESULT WINAPI enum_media_types_Reset(IEnumMediaTypes *iface)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);

    TRACE("iface %p.\n", iface);

    enum_media_types->index = 0;
    return S_OK;
}

static HRESULT WINAPI enum_media_types_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **out)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);
    struct enum_media_types *object;

    TRACE("iface %p, out %p.\n", iface, out);

    if (!(object = heap_alloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumMediaTypes_iface.lpVtbl = &enum_media_types_vtbl;
    object->refcount = 1;
    object->index = enum_media_types->index;

    *out = &object->IEnumMediaTypes_iface;
    return S_OK;
}

static const IEnumMediaTypesVtbl enum_media_types_vtbl =
{
    enum_media_types_QueryInterface,
    enum_media_types_AddRef,
    enum_media_types_Release,
    enum_media_types_Next,
    enum_media_types_Skip,
    enum_media_types_Reset,
    enum_media_types_Clone,
};

static inline struct audio_stream *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IPin_iface);
}

static HRESULT WINAPI audio_sink_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    return IAMMediaStream_QueryInterface(&stream->IAMMediaStream_iface, iid, out);
}

static ULONG WINAPI audio_sink_AddRef(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    return IAMMediaStream_AddRef(&stream->IAMMediaStream_iface);
}

static ULONG WINAPI audio_sink_Release(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    return IAMMediaStream_Release(&stream->IAMMediaStream_iface);
}

static HRESULT WINAPI audio_sink_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    WARN("iface %p, peer %p, mt %p, unexpected call!\n", iface, peer, mt);
    return E_UNEXPECTED;
}

static HRESULT WINAPI audio_sink_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    PIN_DIRECTION dir;

    TRACE("stream %p, peer %p, mt %p.\n", stream, peer, mt);

    EnterCriticalSection(&stream->cs);

    if (stream->peer)
    {
        LeaveCriticalSection(&stream->cs);
        return VFW_E_ALREADY_CONNECTED;
    }

    IPin_QueryDirection(peer, &dir);
    if (dir != PINDIR_OUTPUT)
    {
        WARN("Rejecting connection from input pin.\n");
        LeaveCriticalSection(&stream->cs);
        return VFW_E_INVALID_DIRECTION;
    }

    CopyMediaType(&stream->mt, mt);
    IPin_AddRef(stream->peer = peer);

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_sink_Disconnect(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p.\n", stream);

    EnterCriticalSection(&stream->cs);

    if (!stream->peer)
    {
        LeaveCriticalSection(&stream->cs);
        return S_FALSE;
    }

    IPin_Release(stream->peer);
    stream->peer = NULL;
    FreeMediaType(&stream->mt);
    memset(&stream->mt, 0, sizeof(AM_MEDIA_TYPE));

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_sink_ConnectedTo(IPin *iface, IPin **peer)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("stream %p, peer %p.\n", stream, peer);

    EnterCriticalSection(&stream->cs);

    if (stream->peer)
    {
        IPin_AddRef(*peer = stream->peer);
        hr = S_OK;
    }
    else
    {
        *peer = NULL;
        hr = VFW_E_NOT_CONNECTED;
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI audio_sink_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mt)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("stream %p, mt %p.\n", stream, mt);

    EnterCriticalSection(&stream->cs);

    if (stream->peer)
    {
        CopyMediaType(mt, &stream->mt);
        hr = S_OK;
    }
    else
    {
        memset(mt, 0, sizeof(AM_MEDIA_TYPE));
        hr = VFW_E_NOT_CONNECTED;
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI audio_sink_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p, info %p.\n", stream, info);

    IBaseFilter_AddRef(info->pFilter = (IBaseFilter *)stream->filter);
    info->dir = PINDIR_INPUT;
    wcscpy(info->achName, sink_id);

    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    TRACE("iface %p, dir %p.\n", iface, dir);
    *dir = PINDIR_INPUT;
    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryId(IPin *iface, WCHAR **id)
{
    TRACE("iface %p, id %p.\n", iface, id);

    if (!(*id = CoTaskMemAlloc(sizeof(sink_id))))
        return E_OUTOFMEMORY;

    wcscpy(*id, sink_id);

    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mt)
{
    TRACE("iface %p, mt %p.\n", iface, mt);
    return S_OK;
}

static HRESULT WINAPI audio_sink_EnumMediaTypes(IPin *iface, IEnumMediaTypes **enum_media_types)
{
    struct enum_media_types *object;

    TRACE("iface %p, enum_media_types %p.\n", iface, enum_media_types);

    if (!enum_media_types)
        return E_POINTER;

    if (!(object = heap_alloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumMediaTypes_iface.lpVtbl = &enum_media_types_vtbl;
    object->refcount = 1;
    object->index = 0;

    *enum_media_types = &object->IEnumMediaTypes_iface;
    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *count)
{
    TRACE("iface %p, pins %p, count %p.\n", iface, pins, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sink_EndOfStream(IPin *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sink_BeginFlush(IPin *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sink_EndFlush(IPin *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sink_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    FIXME("iface %p, start %s, stop %s, rate %0.16e, stub!\n",
            iface, wine_dbgstr_longlong(start), wine_dbgstr_longlong(stop), rate);
    return E_NOTIMPL;
}

static const IPinVtbl audio_sink_vtbl =
{
    audio_sink_QueryInterface,
    audio_sink_AddRef,
    audio_sink_Release,
    audio_sink_Connect,
    audio_sink_ReceiveConnection,
    audio_sink_Disconnect,
    audio_sink_ConnectedTo,
    audio_sink_ConnectionMediaType,
    audio_sink_QueryPinInfo,
    audio_sink_QueryDirection,
    audio_sink_QueryId,
    audio_sink_QueryAccept,
    audio_sink_EnumMediaTypes,
    audio_sink_QueryInternalConnections,
    audio_sink_EndOfStream,
    audio_sink_BeginFlush,
    audio_sink_EndFlush,
    audio_sink_NewSegment,
};

static inline struct audio_stream *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IMemInputPin_iface);
}

static HRESULT WINAPI audio_meminput_QueryInterface(IMemInputPin *iface, REFIID iid, void **out)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    return IAMMediaStream_QueryInterface(&stream->IAMMediaStream_iface, iid, out);
}

static ULONG WINAPI audio_meminput_AddRef(IMemInputPin *iface)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    return IAMMediaStream_AddRef(&stream->IAMMediaStream_iface);
}

static ULONG WINAPI audio_meminput_Release(IMemInputPin *iface)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    return IAMMediaStream_Release(&stream->IAMMediaStream_iface);
}

static HRESULT WINAPI audio_meminput_GetAllocator(IMemInputPin *iface, IMemAllocator **allocator)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);

    TRACE("stream %p, allocator %p.\n", stream, allocator);

    if (stream->allocator)
    {
        IMemAllocator_AddRef(*allocator = stream->allocator);
        return S_OK;
    }

    *allocator = NULL;
    return VFW_E_NO_ALLOCATOR;
}

static HRESULT WINAPI audio_meminput_NotifyAllocator(IMemInputPin *iface, IMemAllocator *allocator, BOOL readonly)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);

    TRACE("stream %p, allocator %p, readonly %d.\n", stream, allocator, readonly);

    if (!allocator)
        return E_POINTER;

    if (allocator)
        IMemAllocator_AddRef(allocator);
    if (stream->allocator)
        IMemAllocator_Release(stream->allocator);
    stream->allocator = allocator;

    return S_OK;
}

static HRESULT WINAPI audio_meminput_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *props)
{
    TRACE("iface %p, props %p.\n", iface, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_meminput_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    FIXME("iface %p, sample %p, stub!\n", iface, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_meminput_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **samples, LONG count, LONG *processed)
{
    FIXME("iface %p, samples %p, count %u, processed %p, stub!\n", iface, samples, count, processed);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_meminput_ReceiveCanBlock(IMemInputPin *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static const IMemInputPinVtbl audio_meminput_vtbl =
{
    audio_meminput_QueryInterface,
    audio_meminput_AddRef,
    audio_meminput_Release,
    audio_meminput_GetAllocator,
    audio_meminput_NotifyAllocator,
    audio_meminput_GetAllocatorRequirements,
    audio_meminput_Receive,
    audio_meminput_ReceiveMultiple,
    audio_meminput_ReceiveCanBlock,
};

HRESULT audio_stream_create(IMultiMediaStream *parent, const MSPID *purpose_id,
        IUnknown *stream_object, STREAM_TYPE stream_type, IAMMediaStream **media_stream)
{
    struct audio_stream *object;

    TRACE("(%p,%s,%p,%p)\n", parent, debugstr_guid(purpose_id), stream_object, media_stream);

    if (stream_object)
        FIXME("Specifying a stream object is not yet supported.\n");

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAMMediaStream_iface.lpVtbl = &audio_IAMMediaStream_vtbl;
    object->IAudioMediaStream_iface.lpVtbl = &audio_IAudioMediaStream_vtbl;
    object->IMemInputPin_iface.lpVtbl = &audio_meminput_vtbl;
    object->IPin_iface.lpVtbl = &audio_sink_vtbl;
    object->ref = 1;

    InitializeCriticalSection(&object->cs);
    object->parent = parent;
    object->purpose_id = *purpose_id;
    object->stream_type = stream_type;

    *media_stream = &object->IAMMediaStream_iface;

    return S_OK;
}
