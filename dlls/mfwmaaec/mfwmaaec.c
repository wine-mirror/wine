/* Copyright 2026 Paul Gofman for CodeWeavers
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
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "dmoreg.h"
#include "dshow.h"
#include "mediaerr.h"
#include "mfapi.h"
#include "mferror.h"
#include "mftransform.h"
#include "wmcodecdsp.h"
#include "dmort.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aaec);

static const WAVEFORMATEX default_output_format = { WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16, 0 };
static const DWORD supported_rates[] = { 8000, 11025, 16000, 22050 };
static const unsigned int max_frame_size_ms = 40;

struct aaec
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyStore IPropertyStore_iface;
    IUnknown *outer;
    LONG refcount;
    BOOL source_mode;
    BOOL feature_mode;
    BOOL feat_agc;
    BOOL feat_center_clip;
    unsigned int device_indexes;
    unsigned int system_mode;
    unsigned int feat_ns;
    unsigned int feat_frame_size;
    unsigned int feat_vad;

    DMO_MEDIA_TYPE output_type;
    MFT_OUTPUT_STREAM_INFO output_info;
};

static const struct
{
    REFPROPERTYKEY key;
    VARTYPE vt;
    unsigned int offset;
}
property_def[] =
{
    { &MFPKEY_WMAAECMA_DMO_SOURCE_MODE, VT_BOOL, offsetof(struct aaec, source_mode) },
    { &MFPKEY_WMAAECMA_DEVICE_INDEXES, VT_I4, offsetof(struct aaec, device_indexes) },
    { &MFPKEY_WMAAECMA_SYSTEM_MODE, VT_I4, offsetof(struct aaec, system_mode) },
    { &MFPKEY_WMAAECMA_FEATR_AGC, VT_BOOL, offsetof(struct aaec, feat_agc) },
    { &MFPKEY_WMAAECMA_FEATR_NS, VT_I4, offsetof(struct aaec, feat_ns) },
    { &MFPKEY_WMAAECMA_FEATR_CENTER_CLIP, VT_BOOL, offsetof(struct aaec, feat_center_clip) },
    { &MFPKEY_WMAAECMA_FEATURE_MODE, VT_BOOL, offsetof(struct aaec, feature_mode) },
    { &MFPKEY_WMAAECMA_FEATR_FRAME_SIZE, VT_I4, offsetof(struct aaec, feat_frame_size) },
    { &MFPKEY_WMAAECMA_FEATR_VAD, VT_I4, offsetof(struct aaec, feat_vad) },
};

static struct aaec *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct aaec, IUnknown_inner);
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct aaec *impl = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &impl->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &impl->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &impl->IMediaObject_iface;
    else if (IsEqualIID(iid, &IID_IPropertyStore))
        *out = &impl->IPropertyStore_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI unknown_AddRef(IUnknown *iface)
{
    struct aaec *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct aaec *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        MoFreeMediaType(&impl->output_type);
        free(impl);
    }

    return refcount;
}

static const IUnknownVtbl unknown_vtbl =
{
    unknown_QueryInterface,
    unknown_AddRef,
    unknown_Release,
};

static struct aaec *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct aaec, IMFTransform_iface);
}

static HRESULT WINAPI transform_QueryInterface(IMFTransform *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IMFTransform(iface)->outer, iid, out);
}

static ULONG WINAPI transform_AddRef(IMFTransform *iface)
{
    return IUnknown_AddRef(impl_from_IMFTransform(iface)->outer);
}

static ULONG WINAPI transform_Release(IMFTransform *iface)
{
    return IUnknown_Release(impl_from_IMFTransform(iface)->outer);
}

static HRESULT WINAPI transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("iface %p, input_minimum %p, input_maximum %p, output_minimum %p, output_maximum %p.\n",
            iface, input_minimum, input_maximum, output_minimum, output_maximum);
    *input_minimum = *input_maximum = 0;
    *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    *inputs = 0;
    *outputs = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    TRACE("iface %p, input_size %lu, inputs %p, output_size %lu, outputs %p.\n", iface,
            input_size, inputs, output_size, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    FIXME("iface %p, id %#lx, info %p stub.\n", iface, id, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct aaec *aaec = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (id) return MF_E_INVALIDSTREAMNUMBER;
    *info = aaec->output_info;
    return IsEqualGUID(&aaec->output_type.majortype, &GUID_NULL) ? MF_E_TRANSFORM_TYPE_NOT_SET : S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    TRACE("iface %p, attributes %p.\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    FIXME("iface %p, id %#lx stub.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    FIXME("iface %p, streams %lu, ids %p stub.\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, index %#lx, type %p stub.\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);
    if (id) return MF_E_INVALIDSTREAMNUMBER;
    if (index) return MF_E_NO_MORE_TYPES;

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, default_output_format.nChannels);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, default_output_format.nSamplesPerSec);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, default_output_format.nAvgBytesPerSec);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, default_output_format.wBitsPerSample);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, default_output_format.nBlockAlign);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1);
    if (SUCCEEDED(hr))
        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1);

    if (SUCCEEDED(hr))
        IMFMediaType_AddRef((*type = media_type));
    IMFMediaType_Release(media_type);

    return S_OK;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("iface %p, id %#lx, type %p, flags %#lx stub.\n", iface, id, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    DWORD dmo_flags = flags & MFT_SET_TYPE_TEST_ONLY ? DMO_SET_TYPEF_TEST_ONLY : 0;
    struct aaec *aaec = impl_from_IMFTransform(iface);
    DMO_MEDIA_TYPE tmp = {0}, *mt = type ? &tmp : NULL;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (type && FAILED(hr = MFInitAMMediaTypeFromMFMediaType(type, FORMAT_WaveFormatEx, (AM_MEDIA_TYPE *)&tmp)))
        return hr;

    hr = IMediaObject_SetOutputType(&aaec->IMediaObject_iface, id, mt, dmo_flags);
    MoFreeMediaType(&tmp);
    switch (hr)
    {
        case DMO_E_INVALIDSTREAMINDEX:
            return MF_E_INVALIDSTREAMNUMBER;

        case S_FALSE:
        case DMO_E_INVALIDTYPE:
        case DMO_E_TYPE_NOT_ACCEPTED:
            return MF_E_INVALIDMEDIATYPE;
    }
    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, type %p stub.\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct aaec *aaec = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id) return MF_E_INVALIDSTREAMNUMBER;
    if (IsEqualGUID(&aaec->output_type.majortype, &GUID_NULL)) return MF_E_TRANSFORM_TYPE_NOT_SET;
    return MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &aaec->output_type, type);
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %#lx, flags %p stub.\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub.\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    TRACE("iface %p, lower %I64d, upper %I64d.\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub.\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %p stub.\n", iface, message, (void *)param);
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    FIXME("iface %p, id %#lx, sample %p, flags %#lx stub.\n", iface, id, sample, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *output, DWORD *output_status)
{
    FIXME("iface %p, flags %#lx, count %lu, output %p, output_status %p stub.\n", iface, flags, count,
            output, output_status);
    return E_NOTIMPL;
}

static const IMFTransformVtbl transform_vtbl =
{
    transform_QueryInterface,
    transform_AddRef,
    transform_Release,
    transform_GetStreamLimits,
    transform_GetStreamCount,
    transform_GetStreamIDs,
    transform_GetInputStreamInfo,
    transform_GetOutputStreamInfo,
    transform_GetAttributes,
    transform_GetInputStreamAttributes,
    transform_GetOutputStreamAttributes,
    transform_DeleteInputStream,
    transform_AddInputStreams,
    transform_GetInputAvailableType,
    transform_GetOutputAvailableType,
    transform_SetInputType,
    transform_SetOutputType,
    transform_GetInputCurrentType,
    transform_GetOutputCurrentType,
    transform_GetInputStatus,
    transform_GetOutputStatus,
    transform_SetOutputBounds,
    transform_ProcessEvent,
    transform_ProcessMessage,
    transform_ProcessInput,
    transform_ProcessOutput,
};

static struct aaec *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct aaec, IMediaObject_iface);
}

static HRESULT WINAPI media_object_QueryInterface(IMediaObject *iface, REFIID iid, void **obj)
{
    return IUnknown_QueryInterface(impl_from_IMediaObject(iface)->outer, iid, obj);
}

static ULONG WINAPI media_object_AddRef(IMediaObject *iface)
{
    return IUnknown_AddRef(impl_from_IMediaObject(iface)->outer);
}

static ULONG WINAPI media_object_Release(IMediaObject *iface)
{
    return IUnknown_Release(impl_from_IMediaObject(iface)->outer);
}

static HRESULT WINAPI media_object_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    TRACE("iface %p, input %p, output %p stub.\n", iface, input, output);
    *input = 0;
    *output = 1;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub.\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    TRACE("iface %p, index %lu, flags %p.\n", iface, index, flags);

    if (index) return DMO_E_INVALIDSTREAMINDEX;
    *flags = DMO_OUTPUT_STREAMF_WHOLE_SAMPLES;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type_index %lu, type %p stub.\n", impl_from_IMediaObject(iface), index,
            type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", impl_from_IMediaObject(iface), index,
            type_index, type);
    if (index) return DMO_E_INVALIDSTREAMINDEX;
    if (type_index) return DMO_E_NO_MORE_ITEMS;

    memset(type, 0, sizeof(*type));
    type->majortype = MEDIATYPE_Audio;
    type->subtype = MEDIASUBTYPE_PCM;
    type->formattype = FORMAT_WaveFormatEx;
    type->bFixedSizeSamples = TRUE;
    type->cbFormat = sizeof(default_output_format);
    if (!(type->pbFormat = CoTaskMemAlloc(sizeof(default_output_format))))
        return E_OUTOFMEMORY;
    *(WAVEFORMATEX *)type->pbFormat = default_output_format;

    return S_OK;
}

static HRESULT check_dmo_media_type(const DMO_MEDIA_TYPE *type)
{
    WAVEFORMATEX *wfx;
    unsigned int i;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Audio)) return DMO_E_TYPE_NOT_ACCEPTED;
    if (!IsEqualGUID(&type->formattype, &FORMAT_WaveFormatEx) || type->cbFormat < sizeof(*wfx)) return DMO_E_TYPE_NOT_ACCEPTED;
    if (!IsEqualGUID(&type->subtype, &MEDIASUBTYPE_PCM)) return DMO_E_TYPE_NOT_ACCEPTED;

    wfx = (WAVEFORMATEX *)type->pbFormat;
    if (!wfx->wBitsPerSample || !wfx->nAvgBytesPerSec || !wfx->nBlockAlign)
        return DMO_E_INVALIDTYPE;
    if (wfx->nChannels != 1) return DMO_E_TYPE_NOT_ACCEPTED;
    if (wfx->wFormatTag != WAVE_FORMAT_PCM) return DMO_E_TYPE_NOT_ACCEPTED;

    for (i = 0; i < ARRAY_SIZE(supported_rates); ++i)
    {
        if (wfx->nSamplesPerSec == supported_rates[i]) break;
    }
    if (i == ARRAY_SIZE(supported_rates)) return DMO_E_TYPE_NOT_ACCEPTED;
    return S_OK;
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %#lx, type %p, flags %#lx stub.\n", iface, index, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct aaec *aaec = impl_from_IMediaObject(iface);
    WAVEFORMATEX *wfx;
    HRESULT hr;

    TRACE("iface %p, index %#lx, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index) return DMO_E_INVALIDSTREAMINDEX;
    if (!type)
    {
        FIXME("clearing media type is not implemented.\n");
        return E_NOTIMPL;
    }

    if (FAILED(hr = check_dmo_media_type(type)))
    {
        WARN("format check failed.\n");
        return flags & DMO_SET_TYPEF_TEST_ONLY ? S_FALSE : hr;
    }
    if (flags & DMO_SET_TYPEF_TEST_ONLY) return S_OK;

    MoFreeMediaType(&aaec->output_type);
    if (FAILED(hr = MoCopyMediaType(&aaec->output_type, type)))
    {
        memset(&aaec->output_type, 0, sizeof(aaec->output_type));
        return hr;
    }

    wfx = (WAVEFORMATEX *)type->pbFormat;
    aaec->output_info.dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES;
    aaec->output_info.cbSize = max_frame_size_ms * wfx->nSamplesPerSec * wfx->wBitsPerSample / (8 * 1000);
    aaec->output_info.cbAlignment = wfx->nBlockAlign;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %#lx, type %p stub.\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    struct aaec *aaec = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %#lx, type %p.\n", iface, index, type);

    if (index) return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&aaec->output_type.majortype, &GUID_NULL)) return DMO_E_TYPE_NOT_SET;
    return MoCopyMediaType(type, &aaec->output_type);
}

static HRESULT WINAPI media_object_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size,
        DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, lookahead %p, alignment %p stub.\n", iface, index, size,
            lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct aaec *aaec = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, size %p, alignment %p.\n", iface, index, size, alignment);

    if (IsEqualGUID(&aaec->output_type.majortype, &GUID_NULL)) return DMO_E_TYPE_NOT_SET;
    *size = aaec->output_info.cbSize;
    *alignment = aaec->output_info.cbAlignment;
    return S_OK;
}

static HRESULT WINAPI media_object_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %lu, latency %p stub.\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %lu, latency %I64d stub.\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Flush(IMediaObject *iface)
{
    FIXME("iface %p stub.\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("iface %p, index %lu stub.\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub.\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub.\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub.\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_ProcessInput(IMediaObject *iface, DWORD index,
        IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    FIXME("iface %p, index %lu, buffer %p, flags %#lx, timestamp %I64d, timelength %I64d\n stub.", iface,
            index, buffer, flags, timestamp, timelength);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *output, DWORD *output_status)
{
    FIXME("iface %p, flags %#lx, count %lu, output %p, output_status %p stub.\n", iface, flags, count,
            output, output_status);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %ld stub.\n", iface, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl media_object_vtbl =
{
    media_object_QueryInterface,
    media_object_AddRef,
    media_object_Release,
    media_object_GetStreamCount,
    media_object_GetInputStreamInfo,
    media_object_GetOutputStreamInfo,
    media_object_GetInputType,
    media_object_GetOutputType,
    media_object_SetInputType,
    media_object_SetOutputType,
    media_object_GetInputCurrentType,
    media_object_GetOutputCurrentType,
    media_object_GetInputSizeInfo,
    media_object_GetOutputSizeInfo,
    media_object_GetInputMaxLatency,
    media_object_SetInputMaxLatency,
    media_object_Flush,
    media_object_Discontinuity,
    media_object_AllocateStreamingResources,
    media_object_FreeStreamingResources,
    media_object_GetInputStatus,
    media_object_ProcessInput,
    media_object_ProcessOutput,
    media_object_Lock,
};

static struct aaec *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct aaec, IPropertyStore_iface);
}

static HRESULT WINAPI property_store_QueryInterface(IPropertyStore *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IPropertyStore(iface)->outer, iid, out);
}

static ULONG WINAPI property_store_AddRef(IPropertyStore *iface)
{
    return IUnknown_AddRef(impl_from_IPropertyStore(iface)->outer);
}

static ULONG WINAPI property_store_Release(IPropertyStore *iface)
{
    return IUnknown_Release(impl_from_IPropertyStore(iface)->outer);
}

static HRESULT WINAPI property_store_GetCount(IPropertyStore *iface, DWORD *count)
{
    FIXME("iface %p, count %p stub.\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_GetAt(IPropertyStore *iface, DWORD index, PROPERTYKEY *key)
{
    FIXME("iface %p, index %lu, key %p stub.\n", iface, index, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    struct aaec *aaec = impl_from_IPropertyStore(iface);
    unsigned int i;

    TRACE("iface %p, key (%s,%lu), value %p.\n", iface, debugstr_guid(&key->fmtid), key->pid, value);

    PropVariantInit(value);
    for (i = 0; i < ARRAY_SIZE(property_def); ++i)
    {
        if (!IsEqualPropertyKey(*key, *property_def[i].key)) continue;
        V_VT(value) = property_def[i].vt;
        switch (property_def[i].vt)
        {
            case VT_BOOL:
                value->boolVal = *(BOOL *)((char *)aaec + property_def[i].offset) ? VARIANT_TRUE : VARIANT_FALSE;
                break;
            case VT_I4:
                value->lVal = *(unsigned int *)((char *)aaec + property_def[i].offset);
                break;
            default:
                ERR("Unknown vt %d.\n", property_def[i].vt);
                break;
        }
        return S_OK;
    }

    FIXME("key (%s,%lu) not recognized.\n", debugstr_guid(&key->fmtid), key->pid);
    return MF_E_TRANSFORM_PROPERTY_PID_NOT_RECOGNIZED;
}

static HRESULT WINAPI property_store_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    struct aaec *aaec = impl_from_IPropertyStore(iface);
    unsigned int i;

    TRACE("iface %p, key (%s,%lu), value %p.\n", iface, debugstr_guid(&key->fmtid), key->pid, value);

    for (i = 0; i < ARRAY_SIZE(property_def); ++i)
    {
        if (!IsEqualPropertyKey(*key, *property_def[i].key)) continue;
        if (V_VT(value) != property_def[i].vt) return MF_E_TRANSFORM_PROPERTY_VARIANT_TYPE_WRONG;
        TRACE("value %#lx.\n", value->lVal);
        switch (property_def[i].vt)
        {
            case VT_BOOL:
                if (value->boolVal != VARIANT_TRUE && value->boolVal != VARIANT_FALSE)
                    return MF_E_TRANSFORM_PROPERTY_VALUE_OUT_OF_RANGE;
                *(BOOL *)((char *)aaec + property_def[i].offset) = (value->boolVal == VARIANT_TRUE);
                break;
            case VT_I4:
                *(unsigned int *)((char *)aaec + property_def[i].offset) = value->lVal;
                break;
            default:
                ERR("Unknown vt %d.\n", property_def[i].vt);
                break;
        }
        return S_OK;
    }

    FIXME("key (%s,%lu) not recognized.\n", debugstr_guid(&key->fmtid), key->pid);
    return MF_E_TRANSFORM_PROPERTY_PID_NOT_RECOGNIZED;
}

static HRESULT WINAPI property_store_Commit(IPropertyStore *iface)
{
    FIXME("iface %p stub.\n", iface);
    return E_NOTIMPL;
}

static const IPropertyStoreVtbl property_store_vtbl =
{
    property_store_QueryInterface,
    property_store_AddRef,
    property_store_Release,
    property_store_GetCount,
    property_store_GetAt,
    property_store_GetValue,
    property_store_SetValue,
    property_store_Commit,
};

static HRESULT WINAPI aaec_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    struct aaec *impl;
    HRESULT hr;

    TRACE("outer %p, riid %s, out %p.\n", outer, debugstr_guid(riid), out);

    if (outer && !IsEqualGUID(riid, &IID_IUnknown))
        return E_NOINTERFACE;

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;
    impl->IUnknown_inner.lpVtbl = &unknown_vtbl;
    impl->IMFTransform_iface.lpVtbl = &transform_vtbl;
    impl->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    impl->IPropertyStore_iface.lpVtbl = &property_store_vtbl;
    impl->refcount = 1;
    impl->outer = outer ? outer : &impl->IUnknown_inner;
    impl->source_mode = TRUE;
    impl->device_indexes = 0xffffffff;
    impl->feat_ns = 1;
    impl->feat_center_clip = TRUE;
    impl->output_info.dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES;

    TRACE("Created aaec %p\n", impl);

    hr = IUnknown_QueryInterface(&impl->IUnknown_inner, riid, out);
    IUnknown_Release(&impl->IUnknown_inner);
    return hr;
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    *out = IsEqualGUID(riid, &IID_IClassFactory) || IsEqualGUID(riid, &IID_IUnknown) ? iface : NULL;
    return *out ? S_OK : E_NOINTERFACE;
}
static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}
static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}
static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    return S_OK;
}

static const IClassFactoryVtbl aaec_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    aaec_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory aaec_factory = {&aaec_factory_vtbl};
