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

struct AudioMediaStreamImpl;

typedef struct {
    BaseInputPin pin;
    struct AudioMediaStreamImpl *parent;
} AudioMediaStreamInputPin;

typedef struct AudioMediaStreamImpl {
    IAMMediaStream IAMMediaStream_iface;
    IAudioMediaStream IAudioMediaStream_iface;
    LONG ref;
    IMultiMediaStream* parent;
    MSPID purpose_id;
    STREAM_TYPE stream_type;
    AudioMediaStreamInputPin *input_pin;
    CRITICAL_SECTION critical_section;
} AudioMediaStreamImpl;

static inline AudioMediaStreamImpl *impl_from_AudioMediaStream_IAMMediaStream(IAMMediaStream *iface)
{
    return CONTAINING_RECORD(iface, AudioMediaStreamImpl, IAMMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_QueryInterface(IAMMediaStream *iface,
                                                        REFIID riid, void **ret_iface)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

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
        *ret_iface = &This->input_pin->pin.pin.IPin_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IMemInputPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->input_pin->pin.IMemInputPin_iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioMediaStreamImpl_IAMMediaStream_AddRef(IAMMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI AudioMediaStreamImpl_IAMMediaStream_Release(IAMMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    if (!ref)
    {
        BaseInputPin_Destroy((BaseInputPin *)This->input_pin);
        DeleteCriticalSection(&This->critical_section);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_GetMultiMediaStream(IAMMediaStream *iface,
        IMultiMediaStream** multi_media_stream)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_GetInformation(IAMMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_SetSameFormat(IAMMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, flags);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_AllocateSample(IAMMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_CreateSharedSample(IAMMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_SendEndOfStream(IAMMediaStream *iface, DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, flags);

    return S_FALSE;
}

/*** IAMMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_Initialize(IAMMediaStream *iface, IUnknown *source_object, DWORD flags,
                                                    REFMSPID purpose_id, const STREAM_TYPE stream_type)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p,%u) stub!\n", This, iface, source_object, flags, purpose_id, stream_type);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_SetState(IAMMediaStream *iface, FILTER_STATE state)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%u) stub!\n", This, iface, state);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_JoinAMMultiMediaStream(IAMMediaStream *iface, IAMMultiMediaStream *am_multi_media_stream)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, am_multi_media_stream);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *media_stream_filter)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, media_stream_filter);

    This->input_pin->pin.pin.pinInfo.pFilter = (IBaseFilter *)media_stream_filter;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *filtergraph)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, filtergraph);

    return S_FALSE;
}

static const struct IAMMediaStreamVtbl AudioMediaStreamImpl_IAMMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    AudioMediaStreamImpl_IAMMediaStream_QueryInterface,
    AudioMediaStreamImpl_IAMMediaStream_AddRef,
    AudioMediaStreamImpl_IAMMediaStream_Release,
    /*** IMediaStream methods ***/
    AudioMediaStreamImpl_IAMMediaStream_GetMultiMediaStream,
    AudioMediaStreamImpl_IAMMediaStream_GetInformation,
    AudioMediaStreamImpl_IAMMediaStream_SetSameFormat,
    AudioMediaStreamImpl_IAMMediaStream_AllocateSample,
    AudioMediaStreamImpl_IAMMediaStream_CreateSharedSample,
    AudioMediaStreamImpl_IAMMediaStream_SendEndOfStream,
    /*** IAMMediaStream methods ***/
    AudioMediaStreamImpl_IAMMediaStream_Initialize,
    AudioMediaStreamImpl_IAMMediaStream_SetState,
    AudioMediaStreamImpl_IAMMediaStream_JoinAMMultiMediaStream,
    AudioMediaStreamImpl_IAMMediaStream_JoinFilter,
    AudioMediaStreamImpl_IAMMediaStream_JoinFilterGraph
};

static inline AudioMediaStreamImpl *impl_from_IAudioMediaStream(IAudioMediaStream *iface)
{
    return CONTAINING_RECORD(iface, AudioMediaStreamImpl, IAudioMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_QueryInterface(IAudioMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);
    return IAMMediaStream_QueryInterface(&This->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI AudioMediaStreamImpl_IAudioMediaStream_AddRef(IAudioMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_AddRef(&This->IAMMediaStream_iface);
}

static ULONG WINAPI AudioMediaStreamImpl_IAudioMediaStream_Release(IAudioMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_Release(&This->IAMMediaStream_iface);
}

/*** IMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_GetMultiMediaStream(IAudioMediaStream *iface,
        IMultiMediaStream **multi_media_stream)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_GetInformation(IAudioMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", iface, This, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_SetSameFormat(IAudioMediaStream *iface,
        IMediaStream *stream_format, DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", iface, This, stream_format, flags);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_AllocateSample(IAudioMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", iface, This, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_CreateSharedSample(IAudioMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", iface, This, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_SendEndOfStream(IAudioMediaStream *iface,
        DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", iface, This, flags);

    return S_FALSE;
}

/*** IAudioMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_GetFormat(IAudioMediaStream *iface, WAVEFORMATEX *wave_format_current)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format_current);

    if (!wave_format_current)
        return E_POINTER;

    return MS_E_NOSTREAM;

}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_SetFormat(IAudioMediaStream *iface, const WAVEFORMATEX *wave_format)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_CreateSample(IAudioMediaStream *iface, IAudioData *audio_data,
                                                         DWORD flags, IAudioStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%u,%p)\n", iface, This, audio_data, flags, sample);

    if (!audio_data)
        return E_POINTER;

    return audiostreamsample_create(iface, audio_data, sample);
}

static const struct IAudioMediaStreamVtbl AudioMediaStreamImpl_IAudioMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    AudioMediaStreamImpl_IAudioMediaStream_QueryInterface,
    AudioMediaStreamImpl_IAudioMediaStream_AddRef,
    AudioMediaStreamImpl_IAudioMediaStream_Release,
    /*** IMediaStream methods ***/
    AudioMediaStreamImpl_IAudioMediaStream_GetMultiMediaStream,
    AudioMediaStreamImpl_IAudioMediaStream_GetInformation,
    AudioMediaStreamImpl_IAudioMediaStream_SetSameFormat,
    AudioMediaStreamImpl_IAudioMediaStream_AllocateSample,
    AudioMediaStreamImpl_IAudioMediaStream_CreateSharedSample,
    AudioMediaStreamImpl_IAudioMediaStream_SendEndOfStream,
    /*** IAudioMediaStream methods ***/
    AudioMediaStreamImpl_IAudioMediaStream_GetFormat,
    AudioMediaStreamImpl_IAudioMediaStream_SetFormat,
    AudioMediaStreamImpl_IAudioMediaStream_CreateSample
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

static inline AudioMediaStreamInputPin *impl_from_AudioMediaStreamInputPin_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, AudioMediaStreamInputPin, pin.pin.IPin_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI AudioMediaStreamInputPin_IPin_QueryInterface(IPin *iface, REFIID riid, void **ret_iface)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_QueryInterface(&This->parent->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI AudioMediaStreamInputPin_IPin_AddRef(IPin *iface)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_AddRef(&This->parent->IAMMediaStream_iface);
}

static ULONG WINAPI AudioMediaStreamInputPin_IPin_Release(IPin *iface)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_Release(&This->parent->IAMMediaStream_iface);
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

static const IPinVtbl AudioMediaStreamInputPin_IPin_Vtbl =
{
    AudioMediaStreamInputPin_IPin_QueryInterface,
    AudioMediaStreamInputPin_IPin_AddRef,
    AudioMediaStreamInputPin_IPin_Release,
    BaseInputPinImpl_Connect,
    BaseInputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    audio_sink_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseInputPinImpl_EndOfStream,
    BaseInputPinImpl_BeginFlush,
    BaseInputPinImpl_EndFlush,
    BaseInputPinImpl_NewSegment,
};

static HRESULT WINAPI AudioMediaStreamInputPin_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *media_type)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(&base->IPin_iface);

    TRACE("(%p)->(%p)\n", This, media_type);

    if (IsEqualGUID(&media_type->majortype, &MEDIATYPE_Audio))
    {
        if (IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_PCM))
        {
            TRACE("Audio sub-type %s matches\n", debugstr_guid(&media_type->subtype));
            return S_OK;
        }
    }

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamInputPin_GetMediaType(BasePin *base, int index, AM_MEDIA_TYPE *media_type)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(&base->IPin_iface);

    TRACE("(%p)->(%d,%p)\n", This, index, media_type);

    /* FIXME: Reset structure as we only fill majortype and minortype for now */
    ZeroMemory(media_type, sizeof(*media_type));

    if (index)
        return S_FALSE;

    media_type->majortype = MEDIATYPE_Audio;
    media_type->subtype = MEDIASUBTYPE_PCM;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamInputPin_Receive(BaseInputPin *base, IMediaSample *sample)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(&base->pin.IPin_iface);

    FIXME("(%p)->(%p) stub!\n", This, sample);

    return E_NOTIMPL;
}

static const BaseInputPinFuncTable AudioMediaStreamInputPin_FuncTable =
{
    {
        AudioMediaStreamInputPin_CheckMediaType,
        AudioMediaStreamInputPin_GetMediaType,
    },
    AudioMediaStreamInputPin_Receive,
};

HRESULT audiomediastream_create(IMultiMediaStream *parent, const MSPID *purpose_id,
        IUnknown *stream_object, STREAM_TYPE stream_type, IAMMediaStream **media_stream)
{
    AudioMediaStreamImpl *object;
    PIN_INFO pin_info;
    HRESULT hr;

    TRACE("(%p,%s,%p,%p)\n", parent, debugstr_guid(purpose_id), stream_object, media_stream);

    if (stream_object)
        FIXME("Specifying a stream object is not yet supported.\n");

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAMMediaStream_iface.lpVtbl = &AudioMediaStreamImpl_IAMMediaStream_Vtbl;
    object->IAudioMediaStream_iface.lpVtbl = &AudioMediaStreamImpl_IAudioMediaStream_Vtbl;
    object->ref = 1;

    InitializeCriticalSection(&object->critical_section);

    pin_info.pFilter = NULL;
    pin_info.dir = PINDIR_INPUT;
    pin_info.achName[0] = 'I';
    StringFromGUID2(purpose_id, pin_info.achName + 1, MAX_PIN_NAME - 1);
    hr = BaseInputPin_Construct(&AudioMediaStreamInputPin_IPin_Vtbl,
        sizeof(AudioMediaStreamInputPin), &pin_info, &AudioMediaStreamInputPin_FuncTable,
        &object->critical_section, NULL, (IPin **)&object->input_pin);
    if (FAILED(hr))
        goto out_object;

    object->input_pin->parent = object;

    object->parent = parent;
    object->purpose_id = *purpose_id;
    object->stream_type = stream_type;

    *media_stream = &object->IAMMediaStream_iface;

    return S_OK;

out_object:
    HeapFree(GetProcessHeap(), 0, object);

    return hr;
}
