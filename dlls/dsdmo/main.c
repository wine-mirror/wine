/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#include "dmo.h"
#include "mmreg.h"
#include "mmsystem.h"
#include "uuids.h"
#include "strmif.h"
#include "initguid.h"
#include "medparam.h"
#include "dsound.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsdmo);

struct effect
{
    IMediaObject IMediaObject_iface;
    IMediaObjectInPlace IMediaObjectInPlace_iface;
    IMediaParams IMediaParams_iface;
    IMediaParamInfo IMediaParamInfo_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;

    CRITICAL_SECTION cs;
    WAVEFORMATEX format;

    const struct effect_ops *ops;
};

struct effect_ops
{
    void *(*query_interface)(struct effect *effect, REFIID iid);
    void (*destroy)(struct effect *effect);
};

static struct effect *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IUnknown_inner);
}

static HRESULT WINAPI effect_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IUnknown(iface);

    TRACE("effect %p, iid %s, out %p.\n", effect, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &effect->IMediaObject_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObjectInPlace))
        *out = &effect->IMediaObjectInPlace_iface;
    else if (IsEqualGUID(iid, &IID_IMediaParams))
        *out = &effect->IMediaParams_iface;
    else if (IsEqualGUID(iid, &IID_IMediaParamInfo))
        *out = &effect->IMediaParamInfo_iface;
    else if (!(*out = effect->ops->query_interface(effect, iid)))
    {
        WARN("%s not implemented; returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI effect_inner_AddRef(IUnknown *iface)
{
    struct effect *effect = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&effect->refcount);

    TRACE("%p increasing refcount to %lu.\n", effect, refcount);
    return refcount;
}

static ULONG WINAPI effect_inner_Release(IUnknown *iface)
{
    struct effect *effect = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&effect->refcount);

    TRACE("%p decreasing refcount to %lu.\n", effect, refcount);

    if (!refcount)
    {
        effect->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&effect->cs);
        effect->ops->destroy(effect);
    }
    return refcount;
}

static const IUnknownVtbl effect_inner_vtbl =
{
    effect_inner_QueryInterface,
    effect_inner_AddRef,
    effect_inner_Release,
};

static struct effect *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IMediaObject_iface);
}

static HRESULT WINAPI effect_QueryInterface(IMediaObject *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    return IUnknown_QueryInterface(effect->outer_unk, iid, out);
}

static ULONG WINAPI effect_AddRef(IMediaObject *iface)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    return IUnknown_AddRef(effect->outer_unk);
}

static ULONG WINAPI effect_Release(IMediaObject *iface)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    return IUnknown_Release(effect->outer_unk);
}

static HRESULT WINAPI effect_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    FIXME("iface %p, input %p, output %p, stub!\n", iface, input, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type_index %lu, type %p, stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type_index %lu, type %p, stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    const WAVEFORMATEX *format;

    TRACE("iface %p, index %lu, type %p, flags %#lx.\n", iface, index, type, flags);

    if (flags & DMO_SET_TYPEF_CLEAR)
    {
        EnterCriticalSection(&effect->cs);
        memset(&effect->format, 0, sizeof(effect->format));
        LeaveCriticalSection(&effect->cs);
        return S_OK;
    }

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Audio))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!IsEqualGUID(&type->subtype, &MEDIASUBTYPE_PCM)
            && !IsEqualGUID(&type->subtype, &MEDIASUBTYPE_IEEE_FLOAT))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!IsEqualGUID(&type->formattype, &FORMAT_WaveFormatEx))
        return DMO_E_TYPE_NOT_ACCEPTED;

    format = (const WAVEFORMATEX *)type->pbFormat;
    if (format->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (format->wBitsPerSample != 8 && format->wBitsPerSample != 16)
        {
            WARN("Rejecting %u-bit integer PCM.\n", format->wBitsPerSample);
            return DMO_E_TYPE_NOT_ACCEPTED;
        }
    }
    else if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        if (format->wBitsPerSample != 32)
        {
            WARN("Rejecting %u-bit float PCM.\n", format->wBitsPerSample);
            return DMO_E_TYPE_NOT_ACCEPTED;
        }
    }
    else
    {
        WARN("Rejecting tag %#x.\n", format->wFormatTag);
        return DMO_E_TYPE_NOT_ACCEPTED;
    }

    if (format->nChannels != 1 && format->nChannels != 2)
    {
        WARN("Rejecting %u channels.\n", format->nChannels);
        return DMO_E_TYPE_NOT_ACCEPTED;
    }

    EnterCriticalSection(&effect->cs);
    effect->format = *format;
    LeaveCriticalSection(&effect->cs);

    return S_OK;
}

static HRESULT WINAPI effect_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, type %p, flags %#lx.\n", iface, index, type, flags);

    if (flags & DMO_SET_TYPEF_CLEAR)
        return S_OK;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Audio))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!IsEqualGUID(&type->subtype, &MEDIASUBTYPE_PCM)
            && !IsEqualGUID(&type->subtype, &MEDIASUBTYPE_IEEE_FLOAT))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!IsEqualGUID(&type->formattype, &FORMAT_WaveFormatEx))
        return DMO_E_TYPE_NOT_ACCEPTED;

    EnterCriticalSection(&effect->cs);
    hr = memcmp(type->pbFormat, &effect->format, sizeof(WAVEFORMATEX)) ? DMO_E_TYPE_NOT_ACCEPTED : S_OK;
    LeaveCriticalSection(&effect->cs);

    return hr;
}

static HRESULT WINAPI effect_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type %p, stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type %p, stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputSizeInfo(IMediaObject *iface, DWORD index,
        DWORD *size, DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, lookahead %p, alignment %p, stub!\n", iface, index, size, lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, alignment %p, stub!\n", iface, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %lu, latency %p, stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %lu, latency %s, stub!\n", iface, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Flush(IMediaObject *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("iface %p, index %lu, stub!\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_ProcessInput(IMediaObject *iface, DWORD index,
    IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    FIXME("iface %p, index %lu, buffer %p, flags %#lx, timestamp %s, timelength %s, stub!\n",
            iface, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_ProcessOutput(IMediaObject *iface, DWORD flags,
        DWORD count, DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    FIXME("iface %p, flags %#lx, count %lu, buffers %p, status %p, stub!\n", iface, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %ld, stub!\n", iface, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    effect_GetStreamCount,
    effect_GetInputStreamInfo,
    effect_GetOutputStreamInfo,
    effect_GetInputType,
    effect_GetOutputType,
    effect_SetInputType,
    effect_SetOutputType,
    effect_GetInputCurrentType,
    effect_GetOutputCurrentType,
    effect_GetInputSizeInfo,
    effect_GetOutputSizeInfo,
    effect_GetInputMaxLatency,
    effect_SetInputMaxLatency,
    effect_Flush,
    effect_Discontinuity,
    effect_AllocateStreamingResources,
    effect_FreeStreamingResources,
    effect_GetInputStatus,
    effect_ProcessInput,
    effect_ProcessOutput,
    effect_Lock,
};

static struct effect *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI effect_inplace_QueryInterface(IMediaObjectInPlace *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IMediaObjectInPlace(iface);
    return IUnknown_QueryInterface(effect->outer_unk, iid, out);
}

static ULONG WINAPI effect_inplace_AddRef(IMediaObjectInPlace *iface)
{
    struct effect *effect = impl_from_IMediaObjectInPlace(iface);
    return IUnknown_AddRef(effect->outer_unk);
}

static ULONG WINAPI effect_inplace_Release(IMediaObjectInPlace *iface)
{
    struct effect *effect = impl_from_IMediaObjectInPlace(iface);
    return IUnknown_Release(effect->outer_unk);
}

static HRESULT WINAPI effect_inplace_Process(IMediaObjectInPlace *iface, ULONG size,
        BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    static unsigned int once;

    if (!once++)
    {
        FIXME("iface %p, size %lu, data %p, start %s, flags %#lx, stub!\n",
                iface, size, data, wine_dbgstr_longlong(start), flags);
    }
    else
    {
        WARN("iface %p, size %lu, data %p, start %s, flags %#lx, stub!\n",
               iface, size, data, wine_dbgstr_longlong(start), flags);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI effect_inplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **out)
{
    FIXME("iface %p, out %p, stub!\n", iface, out);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_inplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    FIXME("iface %p, latency %p, stub!\n", iface, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl effect_inplace_vtbl =
{
    effect_inplace_QueryInterface,
    effect_inplace_AddRef,
    effect_inplace_Release,
    effect_inplace_Process,
    effect_inplace_Clone,
    effect_inplace_GetLatency,
};

static struct effect *impl_from_IMediaParams(IMediaParams *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IMediaParams_iface);
}

static HRESULT WINAPI effect_media_params_QueryInterface(IMediaParams *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IMediaParams(iface);
    return IUnknown_QueryInterface(effect->outer_unk, iid, out);
}

static ULONG WINAPI effect_media_params_AddRef(IMediaParams *iface)
{
    struct effect *effect = impl_from_IMediaParams(iface);
    return IUnknown_AddRef(effect->outer_unk);
}

static ULONG WINAPI effect_media_params_Release(IMediaParams *iface)
{
    struct effect *effect = impl_from_IMediaParams(iface);
    return IUnknown_Release(effect->outer_unk);
}

static HRESULT WINAPI effect_media_params_GetParam(IMediaParams *iface, DWORD index, MP_DATA *data)
{
    FIXME("iface %p, index %lu, data %p, stub!\n", iface, index, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_params_SetParam(IMediaParams *iface, DWORD index, MP_DATA data)
{
    FIXME("iface %p, index %lu, data %f, stub!\n", iface, index, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_params_AddEnvelope(IMediaParams *iface, DWORD index, DWORD count,
                                                      MP_ENVELOPE_SEGMENT *segments)
{
    FIXME("iface %p, index %lu, count %lu, segments %p, stub!\n", iface, index, count, segments);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_params_FlushEnvelope(IMediaParams *iface, DWORD index,
                                                        REFERENCE_TIME start, REFERENCE_TIME end)
{
    FIXME("iface %p, index %lu, start %s, end %s, stub!\n", iface, index,
          wine_dbgstr_longlong(start), wine_dbgstr_longlong(end));
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_params_SetTimeFormat(IMediaParams *iface, GUID guid,
                                                        MP_TIMEDATA time_data)
{
    FIXME("iface %p, guid %s, time_data %lx, stub!\n", iface, wine_dbgstr_guid(&guid), time_data);
    return E_NOTIMPL;
}

static const IMediaParamsVtbl effect_media_params_vtbl =
{
    effect_media_params_QueryInterface,
    effect_media_params_AddRef,
    effect_media_params_Release,
    effect_media_params_GetParam,
    effect_media_params_SetParam,
    effect_media_params_AddEnvelope,
    effect_media_params_FlushEnvelope,
    effect_media_params_SetTimeFormat,
};

static struct effect *impl_from_IMediaParamInfo(IMediaParamInfo *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IMediaParamInfo_iface);
}

static HRESULT WINAPI effect_media_param_info_QueryInterface(IMediaParamInfo *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IMediaParamInfo(iface);
    return IUnknown_QueryInterface(effect->outer_unk, iid, out);
}

static ULONG WINAPI effect_media_param_info_AddRef(IMediaParamInfo *iface)
{
    struct effect *effect = impl_from_IMediaParamInfo(iface);
    return IUnknown_AddRef(effect->outer_unk);
}

static ULONG WINAPI effect_media_param_info_Release(IMediaParamInfo *iface)
{
    struct effect *effect = impl_from_IMediaParamInfo(iface);
    return IUnknown_Release(effect->outer_unk);
}

static HRESULT WINAPI effect_media_param_info_GetParamCount(IMediaParamInfo *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_param_info_GetParamInfo(IMediaParamInfo *iface, DWORD index, MP_PARAMINFO *info)
{
    FIXME("iface %p, index %lu, info %p, stub!\n", iface, index, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_param_info_GetParamText(IMediaParamInfo *iface, DWORD index, WCHAR **text)
{
    FIXME("iface %p, index %lu, text %p, stub!\n", iface, index, text);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_param_info_GetNumTimeFormats(IMediaParamInfo *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_param_info_GetSupportedTimeFormat(IMediaParamInfo *iface, DWORD index, GUID *guid)
{
    FIXME("iface %p, index %lu, guid %p, stub!\n", iface, index, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_media_param_info_GetCurrentTimeFormat(IMediaParamInfo *iface, GUID *guid, MP_TIMEDATA *time_data)
{
    FIXME("iface %p, guid %p, time_data %p, stub!\n", iface, guid, time_data);
    return E_NOTIMPL;
}

static const IMediaParamInfoVtbl effect_media_param_info_vtbl =
{
    effect_media_param_info_QueryInterface,
    effect_media_param_info_AddRef,
    effect_media_param_info_Release,
    effect_media_param_info_GetParamCount,
    effect_media_param_info_GetParamInfo,
    effect_media_param_info_GetParamText,
    effect_media_param_info_GetNumTimeFormats,
    effect_media_param_info_GetSupportedTimeFormat,
    effect_media_param_info_GetCurrentTimeFormat,
};

static void effect_init(struct effect *effect, IUnknown *outer, const struct effect_ops *ops)
{
    effect->outer_unk = outer ? outer : &effect->IUnknown_inner;
    effect->refcount = 1;
    effect->IUnknown_inner.lpVtbl = &effect_inner_vtbl;
    effect->IMediaObject_iface.lpVtbl = &effect_vtbl;
    effect->IMediaObjectInPlace_iface.lpVtbl = &effect_inplace_vtbl;
    effect->IMediaParams_iface.lpVtbl = &effect_media_params_vtbl;
    effect->IMediaParamInfo_iface.lpVtbl = &effect_media_param_info_vtbl;

    InitializeCriticalSection(&effect->cs);
    effect->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": effect.cs");

    effect->ops = ops;
}

struct eq
{
    struct effect effect;
    IDirectSoundFXParamEq IDirectSoundFXParamEq_iface;
    DSFXParamEq params;
};

static struct eq *impl_from_IDirectSoundFXParamEq(IDirectSoundFXParamEq *iface)
{
    return CONTAINING_RECORD(iface, struct eq, IDirectSoundFXParamEq_iface);
}

static HRESULT WINAPI eq_params_QueryInterface(IDirectSoundFXParamEq *iface, REFIID iid, void **out)
{
    struct eq *effect = impl_from_IDirectSoundFXParamEq(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI eq_params_AddRef(IDirectSoundFXParamEq *iface)
{
    struct eq *effect = impl_from_IDirectSoundFXParamEq(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI eq_params_Release(IDirectSoundFXParamEq *iface)
{
    struct eq *effect = impl_from_IDirectSoundFXParamEq(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI eq_params_SetAllParameters(IDirectSoundFXParamEq *iface, const DSFXParamEq *params)
{
    struct eq *effect = impl_from_IDirectSoundFXParamEq(iface);

    TRACE("effect %p, params %p.\n", effect, params);

    EnterCriticalSection(&effect->effect.cs);
    effect->params = *params;
    LeaveCriticalSection(&effect->effect.cs);
    return S_OK;
}

static HRESULT WINAPI eq_params_GetAllParameters(IDirectSoundFXParamEq *iface, DSFXParamEq *params)
{
    struct eq *effect = impl_from_IDirectSoundFXParamEq(iface);

    TRACE("effect %p, params %p.\n", effect, params);

    EnterCriticalSection(&effect->effect.cs);
    *params = effect->params;
    LeaveCriticalSection(&effect->effect.cs);
    return S_OK;
}

static const IDirectSoundFXParamEqVtbl eq_params_vtbl =
{
    eq_params_QueryInterface,
    eq_params_AddRef,
    eq_params_Release,
    eq_params_SetAllParameters,
    eq_params_GetAllParameters,
};

static struct eq *impl_eq_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct eq, effect);
}

static void *eq_query_interface(struct effect *iface, REFIID iid)
{
    struct eq *effect = impl_eq_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXParamEq))
        return &effect->IDirectSoundFXParamEq_iface;
    return NULL;
}

static void eq_destroy(struct effect *iface)
{
    struct eq *effect = impl_eq_from_effect(iface);

    free(effect);
}

static const struct effect_ops eq_ops =
{
    .destroy = eq_destroy,
    .query_interface = eq_query_interface,
};

static HRESULT eq_create(IUnknown *outer, IUnknown **out)
{
    struct eq *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &eq_ops);
    object->IDirectSoundFXParamEq_iface.lpVtbl = &eq_params_vtbl;

    object->params.fCenter = 8000.0f;
    object->params.fBandwidth = 12.0f;
    object->params.fGain = 0.0f;

    TRACE("Created equalizer effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct reverb
{
    struct effect effect;
    IDirectSoundFXI3DL2Reverb IDirectSoundFXI3DL2Reverb_iface;
    DSFXI3DL2Reverb params;
};

static struct reverb *impl_from_IDirectSoundFXI3DL2Reverb(IDirectSoundFXI3DL2Reverb *iface)
{
    return CONTAINING_RECORD(iface, struct reverb, IDirectSoundFXI3DL2Reverb_iface);
}

static HRESULT WINAPI reverb_params_QueryInterface(IDirectSoundFXI3DL2Reverb *iface, REFIID iid, void **out)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI reverb_params_AddRef(IDirectSoundFXI3DL2Reverb *iface)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI reverb_params_Release(IDirectSoundFXI3DL2Reverb *iface)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI reverb_params_SetAllParameters(IDirectSoundFXI3DL2Reverb *iface, const DSFXI3DL2Reverb *params)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    TRACE("effect %p, params %p.\n", effect, params);

    EnterCriticalSection(&effect->effect.cs);
    effect->params = *params;
    LeaveCriticalSection(&effect->effect.cs);
    return S_OK;
}

static HRESULT WINAPI reverb_params_GetAllParameters(IDirectSoundFXI3DL2Reverb *iface, DSFXI3DL2Reverb *params)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    TRACE("effect %p, params %p.\n", effect, params);

    EnterCriticalSection(&effect->effect.cs);
    *params = effect->params;
    LeaveCriticalSection(&effect->effect.cs);
    return S_OK;
}

static HRESULT WINAPI reverb_params_SetPreset(IDirectSoundFXI3DL2Reverb *iface, DWORD preset)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    FIXME("effect %p, preset %lu, stub!\n", effect, preset);

    return E_NOTIMPL;
}

static HRESULT WINAPI reverb_params_GetPreset(IDirectSoundFXI3DL2Reverb *iface, DWORD *preset)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    FIXME("effect %p, preset %p, stub!\n", effect, preset);

    return E_NOTIMPL;
}

static HRESULT WINAPI reverb_params_SetQuality(IDirectSoundFXI3DL2Reverb *iface, LONG quality)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    FIXME("effect %p, quality %lu, stub!\n", effect, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI reverb_params_GetQuality(IDirectSoundFXI3DL2Reverb *iface, LONG *quality)
{
    struct reverb *effect = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    FIXME("effect %p, quality %p, stub!\n", effect, quality);

    return E_NOTIMPL;
}

static const IDirectSoundFXI3DL2ReverbVtbl reverb_params_vtbl =
{
    reverb_params_QueryInterface,
    reverb_params_AddRef,
    reverb_params_Release,
    reverb_params_SetAllParameters,
    reverb_params_GetAllParameters,
    reverb_params_SetPreset,
    reverb_params_GetPreset,
    reverb_params_SetQuality,
    reverb_params_GetQuality,
};

static struct reverb *impl_reverb_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct reverb, effect);
}

static void *reverb_query_interface(struct effect *iface, REFIID iid)
{
    struct reverb *effect = impl_reverb_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXI3DL2Reverb))
        return &effect->IDirectSoundFXI3DL2Reverb_iface;
    return NULL;
}

static void reverb_destroy(struct effect *iface)
{
    struct reverb *effect = impl_reverb_from_effect(iface);

    free(effect);
}

static const struct effect_ops reverb_ops =
{
    .destroy = reverb_destroy,
    .query_interface = reverb_query_interface,
};

static HRESULT reverb_create(IUnknown *outer, IUnknown **out)
{
    struct reverb *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &reverb_ops);
    object->IDirectSoundFXI3DL2Reverb_iface.lpVtbl = &reverb_params_vtbl;

    object->params.lRoom                = DSFX_I3DL2REVERB_ROOM_DEFAULT;
    object->params.lRoomHF              = DSFX_I3DL2REVERB_ROOMHF_DEFAULT;
    object->params.flRoomRolloffFactor  = DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_DEFAULT;
    object->params.flDecayTime          = DSFX_I3DL2REVERB_DECAYTIME_DEFAULT;
    object->params.flDecayHFRatio       = DSFX_I3DL2REVERB_DECAYHFRATIO_DEFAULT;
    object->params.lReflections         = DSFX_I3DL2REVERB_REFLECTIONS_DEFAULT;
    object->params.flReflectionsDelay   = DSFX_I3DL2REVERB_REFLECTIONSDELAY_DEFAULT;
    object->params.lReverb              = DSFX_I3DL2REVERB_REVERB_DEFAULT;
    object->params.flReverbDelay        = DSFX_I3DL2REVERB_REVERBDELAY_DEFAULT;
    object->params.flDiffusion          = DSFX_I3DL2REVERB_DIFFUSION_DEFAULT;
    object->params.flDensity            = DSFX_I3DL2REVERB_DENSITY_DEFAULT;
    object->params.flHFReference        = DSFX_I3DL2REVERB_HFREFERENCE_DEFAULT;

    TRACE("Created I3DL2 reverb effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct waves_reverb
{
    struct effect effect;
    IDirectSoundFXWavesReverb IDirectSoundFXWavesReverb_iface;
};

static struct waves_reverb *impl_from_IDirectSoundFXWavesReverb(IDirectSoundFXWavesReverb *iface)
{
    return CONTAINING_RECORD(iface, struct waves_reverb, IDirectSoundFXWavesReverb_iface);
}

static HRESULT WINAPI waves_reverb_params_QueryInterface(IDirectSoundFXWavesReverb *iface, REFIID iid, void **out)
{
    struct waves_reverb *effect = impl_from_IDirectSoundFXWavesReverb(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI waves_reverb_params_AddRef(IDirectSoundFXWavesReverb *iface)
{
    struct waves_reverb *effect = impl_from_IDirectSoundFXWavesReverb(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI waves_reverb_params_Release(IDirectSoundFXWavesReverb *iface)
{
    struct waves_reverb *effect = impl_from_IDirectSoundFXWavesReverb(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI waves_reverb_params_SetAllParameters(IDirectSoundFXWavesReverb *iface, const DSFXWavesReverb *params)
{
    struct waves_reverb *effect = impl_from_IDirectSoundFXWavesReverb(iface);

    FIXME("effect %p, params %p, stub!\n", effect, params);

    return E_NOTIMPL;
}

static HRESULT WINAPI waves_reverb_params_GetAllParameters(IDirectSoundFXWavesReverb *iface, DSFXWavesReverb *params)
{
    struct waves_reverb *effect = impl_from_IDirectSoundFXWavesReverb(iface);

    FIXME("effect %p, params %p, stub!\n", effect, params);

    return E_NOTIMPL;
}

static const IDirectSoundFXWavesReverbVtbl waves_reverb_params_vtbl =
{
    waves_reverb_params_QueryInterface,
    waves_reverb_params_AddRef,
    waves_reverb_params_Release,
    waves_reverb_params_SetAllParameters,
    waves_reverb_params_GetAllParameters,
};

static struct waves_reverb *impl_waves_reverb_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct waves_reverb, effect);
}

static void *waves_reverb_query_interface(struct effect *iface, REFIID iid)
{
    struct waves_reverb *effect = impl_waves_reverb_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXWavesReverb))
        return &effect->IDirectSoundFXWavesReverb_iface;
    return NULL;
}

static void waves_reverb_destroy(struct effect *iface)
{
    struct waves_reverb *effect = impl_waves_reverb_from_effect(iface);

    free(effect);
}

static const struct effect_ops waves_reverb_ops =
{
    .destroy = waves_reverb_destroy,
    .query_interface = waves_reverb_query_interface,
};

static HRESULT waves_reverb_create(IUnknown *outer, IUnknown **out)
{
    struct waves_reverb *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &waves_reverb_ops);
    object->IDirectSoundFXWavesReverb_iface.lpVtbl = &waves_reverb_params_vtbl;

    TRACE("Created waves reverb effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct dmo_echofx
{
    struct effect effect;
    IDirectSoundFXEcho  IDirectSoundFXEcho_iface;
};

static inline struct dmo_echofx *impl_from_IDirectSoundFXEcho(IDirectSoundFXEcho *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_echofx, IDirectSoundFXEcho_iface);
}

static HRESULT WINAPI echofx_QueryInterface(IDirectSoundFXEcho *iface, REFIID iid, void **out)
{
    struct dmo_echofx *effect = impl_from_IDirectSoundFXEcho(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI echofx_AddRef(IDirectSoundFXEcho *iface)
{
    struct dmo_echofx *effect = impl_from_IDirectSoundFXEcho(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI echofx_Release(IDirectSoundFXEcho *iface)
{
    struct dmo_echofx *effect = impl_from_IDirectSoundFXEcho(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI echofx_SetAllParameters(IDirectSoundFXEcho *iface, const DSFXEcho *echo)
{
    struct dmo_echofx *effect = impl_from_IDirectSoundFXEcho(iface);
    FIXME("(%p) %p\n", effect, echo);

    return E_NOTIMPL;
}

static HRESULT WINAPI echofx_GetAllParameters(IDirectSoundFXEcho *iface, DSFXEcho *echo)
{
    struct dmo_echofx *effect = impl_from_IDirectSoundFXEcho(iface);
    FIXME("(%p) %p\n", effect, echo);

    return E_NOTIMPL;
}

static const struct IDirectSoundFXEchoVtbl echofx_vtbl =
{
    echofx_QueryInterface,
    echofx_AddRef,
    echofx_Release,
    echofx_SetAllParameters,
    echofx_GetAllParameters
};

static struct dmo_echofx *impl_echo_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_echofx, effect);
}

static void *echo_query_interface(struct effect *iface, REFIID iid)
{
    struct dmo_echofx *effect = impl_echo_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXEcho))
        return &effect->IDirectSoundFXEcho_iface;
    return NULL;
}

static void echo_destroy(struct effect *iface)
{
    struct dmo_echofx *effect = impl_echo_from_effect(iface);

    free(effect);
}

static const struct effect_ops echo_ops =
{
    .destroy = echo_destroy,
    .query_interface = echo_query_interface,
};

static HRESULT echo_create(IUnknown *outer, IUnknown **out)
{
    struct dmo_echofx *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &echo_ops);
    object->IDirectSoundFXEcho_iface.lpVtbl = &echofx_vtbl;

    TRACE("Created echo effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct dmo_compressorfx
{
    struct effect effect;
    IDirectSoundFXCompressor IDirectSoundFXCompressor_iface;
};

static inline struct dmo_compressorfx *impl_from_IDirectSoundFXCompressor(IDirectSoundFXCompressor *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_compressorfx, IDirectSoundFXCompressor_iface);
}

static HRESULT WINAPI compressorfx_QueryInterface(IDirectSoundFXCompressor *iface, REFIID iid, void **out)
{
    struct dmo_compressorfx *effect = impl_from_IDirectSoundFXCompressor(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI compressorfx_AddRef(IDirectSoundFXCompressor *iface)
{
    struct dmo_compressorfx *effect = impl_from_IDirectSoundFXCompressor(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI compressorfx_Release(IDirectSoundFXCompressor *iface)
{
    struct dmo_compressorfx *effect = impl_from_IDirectSoundFXCompressor(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI compressorfx_SetAllParameters(IDirectSoundFXCompressor *iface, const DSFXCompressor *compressor)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);
    FIXME("(%p) %p\n", This, compressor);

    return E_NOTIMPL;
}

static HRESULT WINAPI compressorfx_GetAllParameters(IDirectSoundFXCompressor *iface, DSFXCompressor *compressor)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);
    FIXME("(%p) %p\n", This, compressor);

    return E_NOTIMPL;
}

static const struct IDirectSoundFXCompressorVtbl compressor_vtbl =
{
    compressorfx_QueryInterface,
    compressorfx_AddRef,
    compressorfx_Release,
    compressorfx_SetAllParameters,
    compressorfx_GetAllParameters
};

static struct dmo_compressorfx *impl_compressor_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_compressorfx, effect);
}

static void *compressor_query_interface(struct effect *iface, REFIID iid)
{
    struct dmo_compressorfx *effect = impl_compressor_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXCompressor))
        return &effect->IDirectSoundFXCompressor_iface;
    return NULL;
}

static void compressor_destroy(struct effect *iface)
{
    struct dmo_compressorfx *effect = impl_compressor_from_effect(iface);

    free(effect);
}

static const struct effect_ops compressor_ops =
{
    .destroy = compressor_destroy,
    .query_interface = compressor_query_interface,
};

static HRESULT compressor_create(IUnknown *outer, IUnknown **out)
{
    struct dmo_compressorfx *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &compressor_ops);
    object->IDirectSoundFXCompressor_iface.lpVtbl = &compressor_vtbl;

    TRACE("Created compressor effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct dmochorusfx
{
    struct effect effect;
    IDirectSoundFXChorus IDirectSoundFXChorus_iface;
};

static inline struct dmochorusfx *impl_from_IDirectSoundFXChorus(IDirectSoundFXChorus *iface)
{
    return CONTAINING_RECORD(iface, struct dmochorusfx, IDirectSoundFXChorus_iface);
}

static HRESULT WINAPI chorusfx_QueryInterface(IDirectSoundFXChorus *iface, REFIID iid, void **out)
{
    struct dmochorusfx *effect = impl_from_IDirectSoundFXChorus(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI chorusfx_AddRef(IDirectSoundFXChorus *iface)
{
    struct dmochorusfx *effect = impl_from_IDirectSoundFXChorus(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI chorusfx_Release(IDirectSoundFXChorus *iface)
{
    struct dmochorusfx *effect = impl_from_IDirectSoundFXChorus(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI chorusfx_SetAllParameters(IDirectSoundFXChorus *iface, const DSFXChorus *compressor)
{
    struct dmochorusfx *This = impl_from_IDirectSoundFXChorus(iface);
    FIXME("(%p) %p\n", This, compressor);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorusfx_GetAllParameters(IDirectSoundFXChorus *iface, DSFXChorus *compressor)
{
    struct dmochorusfx *This = impl_from_IDirectSoundFXChorus(iface);
    FIXME("(%p) %p\n", This, compressor);
    return E_NOTIMPL;
}

static const struct IDirectSoundFXChorusVtbl chorus_vtbl =
{
    chorusfx_QueryInterface,
    chorusfx_AddRef,
    chorusfx_Release,
    chorusfx_SetAllParameters,
    chorusfx_GetAllParameters
};

static struct dmochorusfx *implchorus_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct dmochorusfx, effect);
}

static void *chorus_query_interface(struct effect *iface, REFIID iid)
{
    struct dmochorusfx *effect = implchorus_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXChorus))
        return &effect->IDirectSoundFXChorus_iface;
    return NULL;
}

static void chorus_destroy(struct effect *iface)
{
    struct dmochorusfx *effect = implchorus_from_effect(iface);

    free(effect);
}

static const struct effect_ops chorus_ops =
{
    .destroy = chorus_destroy,
    .query_interface = chorus_query_interface,
};

static HRESULT chorus_create(IUnknown *outer, IUnknown **out)
{
    struct dmochorusfx *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &chorus_ops);
    object->IDirectSoundFXChorus_iface.lpVtbl = &chorus_vtbl;

    TRACE("Created chorus effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct dmo_flangerfx
{
    struct effect effect;
    IDirectSoundFXFlanger IDirectSoundFXFlanger_iface;
};

static inline struct dmo_flangerfx *impl_from_IDirectSoundFXFlanger(IDirectSoundFXFlanger *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_flangerfx, IDirectSoundFXFlanger_iface);
}

static HRESULT WINAPI flangerfx_QueryInterface(IDirectSoundFXFlanger *iface, REFIID iid, void **out)
{
    struct dmo_flangerfx *effect = impl_from_IDirectSoundFXFlanger(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI flangerfx_AddRef(IDirectSoundFXFlanger *iface)
{
    struct dmo_flangerfx *effect = impl_from_IDirectSoundFXFlanger(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI flangerfx_Release(IDirectSoundFXFlanger *iface)
{
    struct dmo_flangerfx *effect = impl_from_IDirectSoundFXFlanger(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI flangerfx_SetAllParameters(IDirectSoundFXFlanger *iface, const DSFXFlanger *flanger)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);
    FIXME("(%p) %p\n", This, flanger);
    return E_NOTIMPL;
}

static HRESULT WINAPI flangerfx_GetAllParameters(IDirectSoundFXFlanger *iface, DSFXFlanger *flanger)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);
    FIXME("(%p) %p\n", This, flanger);
    return E_NOTIMPL;
}

static const struct IDirectSoundFXFlangerVtbl flanger_vtbl =
{
    flangerfx_QueryInterface,
    flangerfx_AddRef,
    flangerfx_Release,
    flangerfx_SetAllParameters,
    flangerfx_GetAllParameters
};

static struct dmo_flangerfx *impl_flanger_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_flangerfx, effect);
}

static void *flanger_query_interface(struct effect *iface, REFIID iid)
{
    struct dmo_flangerfx *effect = impl_flanger_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXFlanger))
        return &effect->IDirectSoundFXFlanger_iface;
    return NULL;
}

static void flanger_destroy(struct effect *iface)
{
    struct dmo_flangerfx *effect = impl_flanger_from_effect(iface);

    free(effect);
}

static const struct effect_ops flanger_ops =
{
    .destroy = flanger_destroy,
    .query_interface = flanger_query_interface,
};

static HRESULT flanger_create(IUnknown *outer, IUnknown **out)
{
    struct dmo_flangerfx *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &flanger_ops);
    object->IDirectSoundFXFlanger_iface.lpVtbl = &flanger_vtbl;

    TRACE("Created flanger effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct distortion
{
    struct effect effect;
    IDirectSoundFXDistortion IDirectSoundFXDistortion_iface;
};

static inline struct distortion *impl_from_IDirectSoundFXDistortion(IDirectSoundFXDistortion *iface)
{
    return CONTAINING_RECORD(iface, struct distortion, IDirectSoundFXDistortion_iface);
}

static HRESULT WINAPI distortion_QueryInterface(IDirectSoundFXDistortion *iface, REFIID iid, void **out)
{
    struct distortion *effect = impl_from_IDirectSoundFXDistortion(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI distortion_AddRef(IDirectSoundFXDistortion *iface)
{
    struct distortion *effect = impl_from_IDirectSoundFXDistortion(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI distortion_Release(IDirectSoundFXDistortion *iface)
{
    struct distortion *effect = impl_from_IDirectSoundFXDistortion(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI distortion_SetAllParameters(IDirectSoundFXDistortion *iface, const DSFXDistortion *params)
{
    struct distortion *This = impl_from_IDirectSoundFXDistortion(iface);
    FIXME("(%p) %p\n", This, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortion_GetAllParameters(IDirectSoundFXDistortion *iface, DSFXDistortion *params)
{
    struct distortion *This = impl_from_IDirectSoundFXDistortion(iface);
    FIXME("(%p) %p\n", This, params);
    return E_NOTIMPL;
}

static const struct IDirectSoundFXDistortionVtbl distortion_vtbl =
{
    distortion_QueryInterface,
    distortion_AddRef,
    distortion_Release,
    distortion_SetAllParameters,
    distortion_GetAllParameters
};

static struct distortion *impl_distortion_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct distortion, effect);
}

static void *distortion_query_interface(struct effect *iface, REFIID iid)
{
    struct distortion *effect = impl_distortion_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXDistortion))
        return &effect->IDirectSoundFXDistortion_iface;
    return NULL;
}

static void distortion_destroy(struct effect *iface)
{
    struct distortion *effect = impl_distortion_from_effect(iface);

    free(effect);
}

static const struct effect_ops distortion_ops =
{
    .destroy = distortion_destroy,
    .query_interface = distortion_query_interface,
};

static HRESULT distortion_create(IUnknown *outer, IUnknown **out)
{
    struct distortion *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &distortion_ops);
    object->IDirectSoundFXDistortion_iface.lpVtbl = &distortion_vtbl;

    TRACE("Created distortion effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct gargle
{
    struct effect effect;
    IDirectSoundFXGargle IDirectSoundFXGargle_iface;
};

static inline struct gargle *impl_from_IDirectSoundFXGargle(IDirectSoundFXGargle *iface)
{
    return CONTAINING_RECORD(iface, struct gargle, IDirectSoundFXGargle_iface);
}

static HRESULT WINAPI gargle_QueryInterface(IDirectSoundFXGargle *iface, REFIID iid, void **out)
{
    struct gargle *effect = impl_from_IDirectSoundFXGargle(iface);
    return IUnknown_QueryInterface(effect->effect.outer_unk, iid, out);
}

static ULONG WINAPI gargle_AddRef(IDirectSoundFXGargle *iface)
{
    struct gargle *effect = impl_from_IDirectSoundFXGargle(iface);
    return IUnknown_AddRef(effect->effect.outer_unk);
}

static ULONG WINAPI gargle_Release(IDirectSoundFXGargle *iface)
{
    struct gargle *effect = impl_from_IDirectSoundFXGargle(iface);
    return IUnknown_Release(effect->effect.outer_unk);
}

static HRESULT WINAPI gargle_SetAllParameters(IDirectSoundFXGargle *iface, const DSFXGargle *params)
{
    struct gargle *This = impl_from_IDirectSoundFXGargle(iface);
    FIXME("(%p) %p\n", This, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI gargle_GetAllParameters(IDirectSoundFXGargle *iface, DSFXGargle *params)
{
    struct gargle *This = impl_from_IDirectSoundFXGargle(iface);
    FIXME("(%p) %p\n", This, params);
    return E_NOTIMPL;
}

static const struct IDirectSoundFXGargleVtbl gargle_vtbl =
{
    gargle_QueryInterface,
    gargle_AddRef,
    gargle_Release,
    gargle_SetAllParameters,
    gargle_GetAllParameters
};

static struct gargle *impl_gargle_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct gargle, effect);
}

static void *gargle_query_interface(struct effect *iface, REFIID iid)
{
    struct gargle *effect = impl_gargle_from_effect(iface);

    if (IsEqualGUID(iid, &IID_IDirectSoundFXGargle))
        return &effect->IDirectSoundFXGargle_iface;
    return NULL;
}

static void gargle_destroy(struct effect *iface)
{
    struct gargle *effect = impl_gargle_from_effect(iface);

    free(effect);
}

static const struct effect_ops gargle_ops =
{
    .destroy = gargle_destroy,
    .query_interface = gargle_query_interface,
};

static HRESULT gargle_create(IUnknown *outer, IUnknown **out)
{
    struct gargle *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &gargle_ops);
    object->IDirectSoundFXGargle_iface.lpVtbl = &gargle_vtbl;

    TRACE("Created gargle effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    *out = NULL;

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("lock %d, stub!\n", lock);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct
{
    const GUID *clsid;
    struct class_factory factory;
}
class_factories[] =
{
    {&GUID_DSFX_STANDARD_I3DL2REVERB,   {{&class_factory_vtbl}, reverb_create}},
    {&GUID_DSFX_STANDARD_PARAMEQ,       {{&class_factory_vtbl}, eq_create}},
    {&GUID_DSFX_WAVES_REVERB,           {{&class_factory_vtbl}, waves_reverb_create}},
    {&GUID_DSFX_STANDARD_ECHO,          {{&class_factory_vtbl}, echo_create}},
    {&GUID_DSFX_STANDARD_COMPRESSOR,    {{&class_factory_vtbl}, compressor_create}},
    {&GUID_DSFX_STANDARD_CHORUS,        {{&class_factory_vtbl}, chorus_create}},
    {&GUID_DSFX_STANDARD_FLANGER,       {{&class_factory_vtbl}, flanger_create}},
    {&GUID_DSFX_STANDARD_DISTORTION,    {{&class_factory_vtbl}, distortion_create}},
    {&GUID_DSFX_STANDARD_GARGLE,        {{&class_factory_vtbl}, gargle_create}},
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    unsigned int i;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    for (i = 0; i < ARRAY_SIZE(class_factories); ++i)
    {
        if (IsEqualGUID(clsid, class_factories[i].clsid))
            return IClassFactory_QueryInterface(&class_factories[i].factory.IClassFactory_iface, iid, out);
    }

    FIXME("%s not available, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
