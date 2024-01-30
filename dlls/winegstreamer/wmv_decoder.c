/* Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "mfapi.h"
#include "mferror.h"
#include "mediaerr.h"
#include "mfobjects.h"
#include "mftransform.h"
#include "wmcodecdsp.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

extern const GUID MEDIASUBTYPE_VC1S;

DEFINE_GUID(MEDIASUBTYPE_WMV_Unknown, 0x7ce12ca9,0xbfbf,0x43d9,0x9d,0x00,0x82,0xb8,0xed,0x54,0x31,0x6b);

struct decoder_type
{
    const GUID *subtype;
    WORD bpp;
    DWORD compression;
};

static const GUID *const wmv_decoder_input_types[] =
{
    &MEDIASUBTYPE_WMV1,
    &MEDIASUBTYPE_WMV2,
    &MEDIASUBTYPE_WMVA,
    &MEDIASUBTYPE_WMVP,
    &MEDIASUBTYPE_WVP2,
    &MEDIASUBTYPE_WMV_Unknown,
    &MEDIASUBTYPE_WVC1,
    &MEDIASUBTYPE_WMV3,
    &MEDIASUBTYPE_VC1S,
};

static const struct decoder_type wmv_decoder_output_types[] =
{
    { &MEDIASUBTYPE_NV12,   12, MAKEFOURCC('N', 'V', '1', '2') },
    { &MEDIASUBTYPE_YV12,   12, MAKEFOURCC('Y', 'V', '1', '2') },
    { &MEDIASUBTYPE_IYUV,   12, MAKEFOURCC('I', 'Y', 'U', 'V') },
    { &MEDIASUBTYPE_I420,   12, MAKEFOURCC('I', '4', '2', '0') },
    { &MEDIASUBTYPE_YUY2,   16, MAKEFOURCC('Y', 'U', 'Y', '2') },
    { &MEDIASUBTYPE_UYVY,   16, MAKEFOURCC('U', 'Y', 'V', 'Y') },
    { &MEDIASUBTYPE_YVYU,   16, MAKEFOURCC('Y', 'V', 'Y', 'U') },
    { &MEDIASUBTYPE_NV11,   12, MAKEFOURCC('N', 'V', '1', '1') },
    { &MEDIASUBTYPE_RGB32,  32, BI_RGB },
    { &MEDIASUBTYPE_RGB24,  24, BI_RGB },
    { &MEDIASUBTYPE_RGB565, 16, BI_BITFIELDS },
    { &MEDIASUBTYPE_RGB555, 16, BI_RGB },
    { &MEDIASUBTYPE_RGB8,   8,  BI_RGB },
};

struct wmv_decoder
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyBag IPropertyBag_iface;
    IPropertyStore IPropertyStore_iface;
    IUnknown *outer;
    LONG refcount;

    struct wg_format input_format;
    struct wg_format output_format;
    GUID output_subtype;

    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;
};

static bool wg_format_is_set(struct wg_format *format)
{
    return format->major_type != WG_MAJOR_TYPE_UNKNOWN;
}

static inline struct wmv_decoder *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct wmv_decoder, IUnknown_inner);
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct wmv_decoder *impl = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &impl->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &impl->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &impl->IMediaObject_iface;
    else if (IsEqualIID(iid, &IID_IPropertyBag))
        *out = &impl->IPropertyBag_iface;
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
    struct wmv_decoder *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct wmv_decoder *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (impl->wg_transform)
            wg_transform_destroy(impl->wg_transform);
        wg_sample_queue_destroy(impl->wg_sample_queue);
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

static struct wmv_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct wmv_decoder, IMFTransform_iface);
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
    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;
    return S_OK;
}

static HRESULT WINAPI transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("iface %p, inputs %p, outputs %p.\n", iface, inputs, outputs);
    *inputs = *outputs = 1;
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
    FIXME("iface %p, id %#lx, info %p stub!\n", iface, id, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    FIXME("iface %p, id %#lx, info %p stub!\n", iface, id, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    FIXME("iface %p, attributes %p stub!\n", iface, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    FIXME("iface %p, id %#lx, attributes %p stub!\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("iface %p, id %#lx.\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("iface %p, streams %lu, ids %p.\n", iface, streams, ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, index %lu, type %p stub!\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, index %lu, type %p stub!\n", iface, id, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("iface %p, id %#lx, type %p, flags %#lx stub!\n", iface, id, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("iface %p, id %#lx, type %p, flags %#lx stub!\n", iface, id, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("iface %p, id %#lx, type %p stub!\n", iface, id, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("iface %p, id %#lx, flags %p stub!\n", iface, id, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    TRACE("iface %p, lower %I64d, upper %I64d.\n", iface, lower, upper);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("iface %p, id %#lx, event %p stub!\n", iface, id, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("iface %p, message %#x, param %#Ix stub!\n", iface, message, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    FIXME("iface %p, id %#lx, sample %p, flags %#lx stub!\n", iface, id, sample, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    FIXME("iface %p, flags %#lx, count %lu, samples %p, status %p stub!\n", iface, flags, count, samples, status);
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

static inline struct wmv_decoder *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct wmv_decoder, IMediaObject_iface);
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
    TRACE("iface %p, input %p, output %p.\n", iface, input, output);

    if (!input || !output)
        return E_POINTER;

    *input = *output = 1;

    return S_OK;
}

static HRESULT WINAPI media_object_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= ARRAY_SIZE(wmv_decoder_input_types))
        return DMO_E_NO_MORE_ITEMS;
    if (!type)
        return S_OK;

    memset(type, 0, sizeof(*type));
    type->majortype = MFMediaType_Video;
    type->subtype = *wmv_decoder_input_types[type_index];
    type->bFixedSizeSamples = FALSE;
    type->bTemporalCompression = TRUE;
    type->lSampleSize = 0;

    return S_OK;
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);
    VIDEOINFOHEADER *info;
    const GUID *subtype;
    LONG width, height;
    UINT32 image_size;
    HRESULT hr;

    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= ARRAY_SIZE(wmv_decoder_output_types))
        return DMO_E_NO_MORE_ITEMS;
    if (!type)
        return S_OK;
    if (!wg_format_is_set(&decoder->input_format))
        return DMO_E_TYPE_NOT_SET;

    width = decoder->input_format.u.video_wmv.width;
    height = abs(decoder->input_format.u.video_wmv.height);
    subtype = wmv_decoder_output_types[type_index].subtype;
    if (FAILED(hr = MFCalculateImageSize(subtype, width, height, &image_size)))
    {
        FIXME("Failed to get image size of subtype %s.\n", debugstr_guid(subtype));
        return hr;
    }

    memset(type, 0, sizeof(*type));
    type->majortype = MFMediaType_Video;
    type->subtype = *subtype;
    type->bFixedSizeSamples = TRUE;
    type->bTemporalCompression = FALSE;
    type->lSampleSize = image_size;
    type->formattype = FORMAT_VideoInfo;
    type->cbFormat = sizeof(VIDEOINFOHEADER);
    type->pbFormat = CoTaskMemAlloc(type->cbFormat);
    memset(type->pbFormat, 0, type->cbFormat);

    info = (VIDEOINFOHEADER *)type->pbFormat;
    info->rcSource.right  = width;
    info->rcSource.bottom = height;
    info->rcTarget.right  = width;
    info->rcTarget.bottom = height;
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth  = width;
    info->bmiHeader.biHeight = height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = wmv_decoder_output_types[type_index].bpp;
    info->bmiHeader.biCompression = wmv_decoder_output_types[type_index].compression;
    info->bmiHeader.biSizeImage = image_size;

    return S_OK;
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);
    struct wg_format wg_format;
    unsigned int i;

    TRACE("iface %p, index %lu, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!type)
    {
        if (flags & DMO_SET_TYPEF_CLEAR)
        {
            memset(&decoder->input_format, 0, sizeof(decoder->input_format));
            if (decoder->wg_transform)
            {
                wg_transform_destroy(decoder->wg_transform);
                decoder->wg_transform = 0;
            }
            return S_OK;
        }
        return DMO_E_TYPE_NOT_ACCEPTED;
    }

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Video))
        return DMO_E_TYPE_NOT_ACCEPTED;

    for (i = 0; i < ARRAY_SIZE(wmv_decoder_input_types); ++i)
        if (IsEqualGUID(&type->subtype, wmv_decoder_input_types[i]))
            break;
    if (i == ARRAY_SIZE(wmv_decoder_input_types))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!amt_to_wg_format((const AM_MEDIA_TYPE *)type, &wg_format))
        return DMO_E_TYPE_NOT_ACCEPTED;
    assert(wg_format.major_type == WG_MAJOR_TYPE_VIDEO_WMV);

    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    decoder->input_format = wg_format;
    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }

    return S_OK;
}

static HRESULT WINAPI media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);
    struct wg_transform_attrs attrs = {0};
    struct wg_format wg_format;
    unsigned int i;

    TRACE("iface %p, index %lu, type %p, flags %#lx,\n", iface, index, type, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!type)
    {
        if (flags & DMO_SET_TYPEF_CLEAR)
        {
            memset(&decoder->output_format, 0, sizeof(decoder->output_format));
            if (decoder->wg_transform)
            {
                wg_transform_destroy(decoder->wg_transform);
                decoder->wg_transform = 0;
            }
            return S_OK;
        }
        return E_POINTER;
    }

    if (!wg_format_is_set(&decoder->input_format))
        return DMO_E_TYPE_NOT_SET;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Video))
        return DMO_E_TYPE_NOT_ACCEPTED;

    for (i = 0; i < ARRAY_SIZE(wmv_decoder_output_types); ++i)
        if (IsEqualGUID(&type->subtype, wmv_decoder_output_types[i].subtype))
            break;
    if (i == ARRAY_SIZE(wmv_decoder_output_types))
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (!amt_to_wg_format((const AM_MEDIA_TYPE *)type, &wg_format))
        return DMO_E_TYPE_NOT_ACCEPTED;
    assert(wg_format.major_type == WG_MAJOR_TYPE_VIDEO);

    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    decoder->output_subtype = type->subtype;
    decoder->output_format = wg_format;

    /* Set up wg_transform. */
    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }
    if (!(decoder->wg_transform = wg_transform_create(&decoder->input_format, &decoder->output_format, &attrs)))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI media_object_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type %p stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type %p stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size,
        DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %lu, size %p, lookahead %p, alignment %p stub!\n", iface, index, size,
            lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p, index %lu, size %p, alignment %p.\n", iface, index, size, alignment);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (!wg_format_is_set(&decoder->output_format))
        return DMO_E_TYPE_NOT_SET;

    if (FAILED(hr = MFCalculateImageSize(&decoder->output_subtype,
            decoder->output_format.u.video.width, abs(decoder->output_format.u.video.height), (UINT32 *)size)))
    {
        FIXME("Failed to get image size of subtype %s.\n", debugstr_guid(&decoder->output_subtype));
        return hr;
    }
    *alignment = 1;

    return S_OK;
}

static HRESULT WINAPI media_object_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %lu, latency %p stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %lu, latency %s stub!\n", iface, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Flush(IMediaObject *iface)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    if (FAILED(hr = wg_transform_flush(decoder->wg_transform)))
        return hr;

    wg_sample_queue_flush(decoder->wg_sample_queue, TRUE);

    return S_OK;
}

static HRESULT WINAPI media_object_Discontinuity(IMediaObject *iface, DWORD index)
{
    TRACE("iface %p, index %lu.\n", iface, index);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    return S_OK;
}

static HRESULT WINAPI media_object_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    TRACE("iface %p, index %lu, flags %p.\n", iface, index, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (!flags)
        return E_POINTER;

    *flags = DMO_INPUT_STATUSF_ACCEPT_DATA;

    return S_OK;
}

static HRESULT WINAPI media_object_ProcessInput(IMediaObject *iface, DWORD index,
        IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, buffer %p, flags %#lx, timestamp %s, timelength %s.\n", iface,
             index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

    return wg_transform_push_dmo(decoder->wg_transform, buffer, flags, timestamp, timelength, decoder->wg_sample_queue);
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct wmv_decoder *decoder = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, buffers %p, status %p.\n", iface, flags, count, buffers, status);

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

    if ((hr = wg_transform_read_dmo(decoder->wg_transform, buffers)) == MF_E_TRANSFORM_STREAM_CHANGE)
        hr = wg_transform_read_dmo(decoder->wg_transform, buffers);

    if (SUCCEEDED(hr))
        wg_sample_queue_flush(decoder->wg_sample_queue, false);

    return hr;
}

static HRESULT WINAPI media_object_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %ld stub!\n", iface, lock);
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

static inline struct wmv_decoder *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct wmv_decoder, IPropertyBag_iface);
}

static HRESULT WINAPI property_bag_QueryInterface(IPropertyBag *iface, REFIID iid, void **out)
{
    return IUnknown_QueryInterface(impl_from_IPropertyBag(iface)->outer, iid, out);
}

static ULONG WINAPI property_bag_AddRef(IPropertyBag *iface)
{
    return IUnknown_AddRef(impl_from_IPropertyBag(iface)->outer);
}

static ULONG WINAPI property_bag_Release(IPropertyBag *iface)
{
    return IUnknown_Release(impl_from_IPropertyBag(iface)->outer);
}

static HRESULT WINAPI property_bag_Read(IPropertyBag *iface, const WCHAR *prop_name, VARIANT *value,
        IErrorLog *error_log)
{
    FIXME("iface %p, prop_name %s, value %p, error_log %p stub!\n", iface, debugstr_w(prop_name), value, error_log);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_bag_Write(IPropertyBag *iface, const WCHAR *prop_name, VARIANT *value)
{
    FIXME("iface %p, prop_name %s, value %p stub!\n", iface, debugstr_w(prop_name), value);
    return S_OK;
}

static const IPropertyBagVtbl property_bag_vtbl =
{
    property_bag_QueryInterface,
    property_bag_AddRef,
    property_bag_Release,
    property_bag_Read,
    property_bag_Write,
};

static inline struct wmv_decoder *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct wmv_decoder, IPropertyStore_iface);
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
    FIXME("iface %p, count %p stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_GetAt(IPropertyStore *iface, DWORD index, PROPERTYKEY *key)
{
    FIXME("iface %p, index %lu, key %p stub!\n", iface, index, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    FIXME("iface %p, key %p, value %p stub!\n", iface, key, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    FIXME("iface %p, key %p, value %p stub!\n", iface, key, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI property_store_Commit(IPropertyStore *iface)
{
    FIXME("iface %p stub!\n", iface);
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

HRESULT wmv_decoder_create(IUnknown *outer, IUnknown **out)
{
    static const struct wg_format input_format =
    {
        .major_type = WG_MAJOR_TYPE_VIDEO_WMV,
        .u.video_wmv.format = WG_WMV_VIDEO_FORMAT_WMV3,
    };
    static const struct wg_format output_format =
    {
        .major_type = WG_MAJOR_TYPE_VIDEO,
        .u.video =
        {
            .format = WG_VIDEO_FORMAT_NV12,
            .width = 1920,
            .height = 1080,
        },
    };
    struct wg_transform_attrs attrs = {0};
    wg_transform_t transform;
    struct wmv_decoder *decoder;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    if (!(transform = wg_transform_create(&input_format, &output_format, &attrs)))
    {
        ERR_(winediag)("GStreamer doesn't support WMV decoding, please install appropriate plugins.\n");
        return E_FAIL;
    }
    wg_transform_destroy(transform);

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = wg_sample_queue_create(&decoder->wg_sample_queue)))
    {
        free(decoder);
        return hr;
    }

    decoder->IUnknown_inner.lpVtbl = &unknown_vtbl;
    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    decoder->IPropertyBag_iface.lpVtbl = &property_bag_vtbl;
    decoder->IPropertyStore_iface.lpVtbl = &property_store_vtbl;
    decoder->refcount = 1;
    decoder->outer = outer ? outer : &decoder->IUnknown_inner;

    *out = &decoder->IUnknown_inner;
    TRACE("Created %p\n", *out);
    return S_OK;
}
