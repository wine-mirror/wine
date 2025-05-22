/* Generic Video Decoder Transform
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
 * Copyright 2023 Shaun Ren for CodeWeavers
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
#include "mfobjects.h"
#include "mftransform.h"
#include "mediaerr.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"

#include "initguid.h"

#include "codecapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct subtype_info
{
    const GUID *subtype;
    WORD bpp;
    DWORD compression;
};

static const struct subtype_info subtype_info_list[] =
{
    { &MFVideoFormat_NV12,   12, MAKEFOURCC('N', 'V', '1', '2') },
    { &MFVideoFormat_YV12,   12, MAKEFOURCC('Y', 'V', '1', '2') },
    { &MFVideoFormat_IYUV,   12, MAKEFOURCC('I', 'Y', 'U', 'V') },
    { &MFVideoFormat_I420,   12, MAKEFOURCC('I', '4', '2', '0') },
    { &MFVideoFormat_YUY2,   16, MAKEFOURCC('Y', 'U', 'Y', '2') },
    { &MFVideoFormat_UYVY,   16, MAKEFOURCC('U', 'Y', 'V', 'Y') },
    { &MFVideoFormat_YVYU,   16, MAKEFOURCC('Y', 'V', 'Y', 'U') },
    { &MFVideoFormat_NV11,   12, MAKEFOURCC('N', 'V', '1', '1') },
    { &MFVideoFormat_P010,   24, MAKEFOURCC('P', '0', '1', '0') },
    { &MFVideoFormat_RGB8,   8,  BI_RGB },
    { &MFVideoFormat_RGB555, 16, BI_RGB },
    { &MFVideoFormat_RGB565, 16, BI_BITFIELDS },
    { &MFVideoFormat_RGB24,  24, BI_RGB },
    { &MFVideoFormat_RGB32,  32, BI_RGB },
    { &MEDIASUBTYPE_RGB8,    8,  BI_RGB },
    { &MEDIASUBTYPE_RGB555,  16, BI_RGB },
    { &MEDIASUBTYPE_RGB565,  16, BI_BITFIELDS },
    { &MEDIASUBTYPE_RGB24,   24, BI_RGB },
    { &MEDIASUBTYPE_RGB32,   32, BI_RGB },
};

static const GUID *const video_decoder_output_types[] =
{
    &MFVideoFormat_NV12,
    &MFVideoFormat_YV12,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_I420,
    &MFVideoFormat_YUY2,
};

struct video_decoder
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyBag IPropertyBag_iface;
    IPropertyStore IPropertyStore_iface;
    IUnknown *outer;
    LONG refcount;

    IMFAttributes *attributes;
    IMFAttributes *output_attributes;

    UINT input_type_count;
    const GUID *const *input_types;
    UINT output_type_count;
    const GUID *const *output_types;

    UINT64 sample_time;
    IMFMediaType *input_type;
    MFT_INPUT_STREAM_INFO input_info;
    IMFMediaType *output_type;
    MFT_OUTPUT_STREAM_INFO output_info;
    IMFMediaType *stream_type;

    wg_transform_t wg_transform;
    struct wg_transform_attrs wg_transform_attrs;
    struct wg_sample_queue *wg_sample_queue;

    IMFVideoSampleAllocatorEx *allocator;
    BOOL allocator_initialized;
    IMFTransform *copier;
    IMFMediaBuffer *temp_buffer;

    DMO_MEDIA_TYPE dmo_input_type;
    DMO_MEDIA_TYPE dmo_output_type;

    INT32 previous_stride;
};

static inline struct video_decoder *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct video_decoder, IUnknown_inner);
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct video_decoder *decoder = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = &decoder->IUnknown_inner;
    else if (IsEqualGUID(iid, &IID_IMFTransform))
        *out = &decoder->IMFTransform_iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject) && decoder->IMediaObject_iface.lpVtbl)
        *out = &decoder->IMediaObject_iface;
    else if (IsEqualGUID(iid, &IID_IPropertyBag) && decoder->IPropertyBag_iface.lpVtbl)
        *out = &decoder->IPropertyBag_iface;
    else if (IsEqualGUID(iid, &IID_IPropertyStore) && decoder->IPropertyStore_iface.lpVtbl)
        *out = &decoder->IPropertyStore_iface;
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
    struct video_decoder *decoder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", decoder, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct video_decoder *decoder = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", decoder, refcount);

    if (!refcount)
    {
        IMFTransform_Release(decoder->copier);
        IMFVideoSampleAllocatorEx_Release(decoder->allocator);
        if (decoder->temp_buffer)
            IMFMediaBuffer_Release(decoder->temp_buffer);
        if (decoder->wg_transform)
            wg_transform_destroy(decoder->wg_transform);
        if (decoder->input_type)
            IMFMediaType_Release(decoder->input_type);
        if (decoder->output_type)
            IMFMediaType_Release(decoder->output_type);
        if (decoder->output_attributes)
            IMFAttributes_Release(decoder->output_attributes);
        if (decoder->attributes)
            IMFAttributes_Release(decoder->attributes);
        wg_sample_queue_destroy(decoder->wg_sample_queue);
        free(decoder);
    }

    return refcount;
}

static const IUnknownVtbl unknown_vtbl =
{
    unknown_QueryInterface,
    unknown_AddRef,
    unknown_Release,
};

static WORD get_subtype_bpp(const GUID *subtype)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(subtype_info_list); ++i)
    {
        if (IsEqualGUID(subtype, subtype_info_list[i].subtype))
            return subtype_info_list[i].bpp;
    }

    return 0;
}

static DWORD get_subtype_compression(const GUID *subtype)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(subtype_info_list); ++i)
    {
        if (IsEqualGUID(subtype, subtype_info_list[i].subtype))
            return subtype_info_list[i].compression;
    }

    return 0;
}

static const GUID *get_dmo_subtype(const GUID *subtype)
{
    if (IsEqualGUID(subtype, &MFVideoFormat_RGB8))
        return &MEDIASUBTYPE_RGB8;
    else if (IsEqualGUID(subtype, &MFVideoFormat_RGB555))
        return &MEDIASUBTYPE_RGB555;
    else if (IsEqualGUID(subtype, &MFVideoFormat_RGB565))
        return &MEDIASUBTYPE_RGB565;
    else if (IsEqualGUID(subtype, &MFVideoFormat_RGB24))
        return &MEDIASUBTYPE_RGB24;
    else if (IsEqualGUID(subtype, &MFVideoFormat_RGB32))
        return &MEDIASUBTYPE_RGB32;
    else
        return subtype;
}

static struct video_decoder *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_decoder, IMFTransform_iface);
}

static HRESULT set_previous_stride(struct video_decoder *decoder)
{
    INT32 requested_stride;
    LONG default_stride;
    UINT64 ratio;
    UINT32 width;
    GUID subtype;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (FAILED(IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_SIZE, &ratio)))
        ratio = (UINT64)1920 << 32 | 1080;

    width = ratio >> 32;

    if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &default_stride)))
        return hr;

    if (FAILED(IMFMediaType_GetUINT32(decoder->output_type, &MF_MT_DEFAULT_STRIDE, (UINT32*)&requested_stride)))
        requested_stride = 0;

    if (default_stride > 0 || requested_stride < 0)
        decoder->previous_stride = -abs(default_stride);
    else
        decoder->previous_stride = abs(default_stride);

    return S_OK;
}

static HRESULT normalize_stride(struct video_decoder *decoder, IMFMediaType **ret)
{
    IMFMediaType *media_type = decoder->output_type;
    INT32 default_stride, requested_stride;
    DMO_MEDIA_TYPE amt;
    HRESULT hr;

    if (SUCCEEDED(hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_VideoInfo, &amt)))
    {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)amt.pbFormat;
        vih->bmiHeader.biHeight = abs(vih->bmiHeader.biHeight);
        hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &amt, ret);
        FreeMediaType(&amt);

        /* Stride must always be positive for YUV formats. Otherwise:
         * 1. if this is the first time an output type has been specified, it must either:
         *  a. match the sign of the requested MF_MT_DEFAULT_STRIDE value; or
         *  b. default to positive; or
         * 2. If an output type was previously specified and input sent it must:
         *  a. be negative if it was previously a YUV format; or
         *  b. match the sign of the previously requested MF_MT_DEFAULT_STRIDE attribute
         *     (defaulting to positive if it had not been provided)
         */

        if (SUCCEEDED(hr = IMFMediaType_GetUINT32(*ret, &MF_MT_DEFAULT_STRIDE, (UINT32*)&default_stride)))
        {
            if (FAILED(IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, (UINT32*)&requested_stride)))
                requested_stride = 0;

            /* default stride less than 0 means an RGB format */
            if (default_stride < 0 && (decoder->previous_stride > 0
                    || (!decoder->previous_stride && requested_stride >= 0)))
                hr = IMFMediaType_SetUINT32(*ret, &MF_MT_DEFAULT_STRIDE, abs(default_stride));
        }
    }

    return hr;
}

static HRESULT try_create_wg_transform(struct video_decoder *decoder, IMFMediaType *output_type)
{
    /* Call of Duty: Black Ops 3 doesn't care about the ProcessInput/ProcessOutput
     * return values, it calls them in a specific order and expects the decoder
     * transform to be able to queue its input buffers. We need to use a buffer list
     * to match its expectations.
     */
    UINT32 low_latency;

    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }

    if (SUCCEEDED(IMFAttributes_GetUINT32(decoder->attributes, &MF_LOW_LATENCY, &low_latency)))
        decoder->wg_transform_attrs.low_latency = !!low_latency;

    return wg_transform_create_mf(decoder->input_type, output_type, &decoder->wg_transform_attrs, &decoder->wg_transform);
}

static HRESULT create_output_media_type(struct video_decoder *decoder, const GUID *subtype,
        IMFMediaType *output_type, IMFMediaType **media_type)
{
    IMFMediaType *default_type = decoder->output_type, *stream_type = output_type ? output_type : decoder->stream_type;
    MFVideoArea default_aperture = {{0}}, aperture;
    IMFVideoMediaType *video_type;
    LONG default_stride, stride;
    UINT32 value, width, height;
    UINT64 ratio;
    HRESULT hr;

    if (FAILED(hr = MFCreateVideoMediaTypeFromSubtype(subtype, &video_type)))
        return hr;

    if (FAILED(IMFMediaType_GetUINT64(stream_type, &MF_MT_FRAME_SIZE, &ratio)))
        ratio = (UINT64)1920 << 32 | 1080;
    if (FAILED(hr = IMFVideoMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, ratio)))
        goto done;
    width = ratio >> 32;
    height = ratio;

    default_aperture.Area.cx = width;
    default_aperture.Area.cy = height;

    if (FAILED(IMFMediaType_GetUINT64(stream_type, &MF_MT_FRAME_RATE, &ratio)))
        ratio = (UINT64)30000 << 32 | 1001;
    if (FAILED(hr = IMFVideoMediaType_SetUINT64(video_type, &MF_MT_FRAME_RATE, ratio)))
        goto done;

    if (FAILED(IMFMediaType_GetUINT64(stream_type, &MF_MT_PIXEL_ASPECT_RATIO, &ratio)))
        ratio = (UINT64)1 << 32 | 1;
    if (FAILED(hr = IMFVideoMediaType_SetUINT64(video_type, &MF_MT_PIXEL_ASPECT_RATIO, ratio)))
        goto done;

    if (!output_type || FAILED(IMFMediaType_GetUINT32(output_type, &MF_MT_SAMPLE_SIZE, &value)))
        hr = MFCalculateImageSize(subtype, width, height, &value);
    if (FAILED(hr) || FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_SAMPLE_SIZE, value)))
        goto done;

    /* WMV decoder uses positive stride by default, and enforces it for YUV formats,
     * accepts negative stride for RGB if specified */
    if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype->Data1, width, &default_stride)))
        goto done;
    if (!output_type || FAILED(IMFMediaType_GetUINT32(output_type, &MF_MT_DEFAULT_STRIDE, (UINT32 *)&stride)))
        stride = abs(default_stride);
    else if (default_stride > 0)
        stride = abs(stride);
    if (FAILED(hr) || FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_DEFAULT_STRIDE, stride)))
        goto done;

    if (!output_type || FAILED(IMFMediaType_GetUINT32(output_type, &MF_MT_VIDEO_NOMINAL_RANGE, (UINT32 *)&value)))
        value = MFNominalRange_Wide;
    if (FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_VIDEO_NOMINAL_RANGE, value)))
        goto done;

    if (!default_type || FAILED(IMFMediaType_GetUINT32(default_type, &MF_MT_INTERLACE_MODE, &value)))
        value = MFVideoInterlace_MixedInterlaceOrProgressive;
    if (FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_INTERLACE_MODE, value)))
        goto done;

    if (!default_type || FAILED(IMFMediaType_GetUINT32(default_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value)))
        value = 1;
    if (FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, value)))
        goto done;

    if (!default_type || FAILED(IMFMediaType_GetUINT32(default_type, &MF_MT_VIDEO_ROTATION, &value)))
        value = 0;
    if (FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_VIDEO_ROTATION, value)))
        goto done;

    if (!default_type || FAILED(IMFMediaType_GetUINT32(default_type, &MF_MT_FIXED_SIZE_SAMPLES, &value)))
        value = 1;
    if (FAILED(hr = IMFVideoMediaType_SetUINT32(video_type, &MF_MT_FIXED_SIZE_SAMPLES, value)))
        goto done;

    if (FAILED(IMFMediaType_GetBlob(stream_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value)))
        aperture = default_aperture;
    if (FAILED(hr = IMFVideoMediaType_SetBlob(video_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture))))
        goto done;

    if (FAILED(IMFMediaType_GetBlob(stream_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value)))
        aperture = default_aperture;
    if (FAILED(hr = IMFVideoMediaType_SetBlob(video_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture))))
        goto done;

    if (FAILED(IMFMediaType_GetBlob(stream_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value)))
        aperture = default_aperture;
    if (FAILED(hr = IMFVideoMediaType_SetBlob(video_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture))))
        goto done;

done:
    if (SUCCEEDED(hr))
        *media_type = (IMFMediaType *)video_type;
    else
    {
        IMFVideoMediaType_Release(video_type);
        *media_type = NULL;
    }

    return hr;
}

static HRESULT init_allocator(struct video_decoder *decoder)
{
    HRESULT hr;

    if (decoder->allocator_initialized)
        return S_OK;

    if (FAILED(hr = IMFTransform_SetInputType(decoder->copier, 0, decoder->output_type, 0)))
        return hr;
    if (FAILED(hr = IMFTransform_SetOutputType(decoder->copier, 0, decoder->output_type, 0)))
        return hr;

    if (FAILED(hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(decoder->allocator, 10, 10,
            decoder->attributes, decoder->output_type)))
        return hr;
    decoder->allocator_initialized = TRUE;
    return S_OK;
}

static void uninit_allocator(struct video_decoder *decoder)
{
    IMFVideoSampleAllocatorEx_UninitializeSampleAllocator(decoder->allocator);
    decoder->allocator_initialized = FALSE;
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
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    *info = decoder->input_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    *info = decoder->output_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    FIXME("iface %p, attributes %p semi-stub!\n", iface, attributes);

    if (!attributes)
        return E_POINTER;

    IMFAttributes_AddRef((*attributes = decoder->attributes));
    return S_OK;
}

static HRESULT WINAPI transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    TRACE("iface %p, id %#lx, attributes %p.\n", iface, id, attributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id, IMFAttributes **attributes)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    FIXME("iface %p, id %#lx, attributes %p semi-stub!\n", iface, id, attributes);

    if (!attributes)
        return E_POINTER;
    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    IMFAttributes_AddRef((*attributes = decoder->output_attributes));
    return S_OK;
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
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;
    if (index >= decoder->input_type_count)
        return MF_E_NO_MORE_TYPES;
    return MFCreateVideoMediaTypeFromSubtype(decoder->input_types[index], (IMFVideoMediaType **)type);
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id,
        DWORD index, IMFMediaType **type)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;
    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (index >= decoder->output_type_count)
        return MF_E_NO_MORE_TYPES;
    return create_output_media_type(decoder, decoder->output_types[index], NULL, type);
}

static HRESULT update_output_info_size(struct video_decoder *decoder, UINT32 width, UINT32 height)
{
    HRESULT hr = E_FAIL;
    UINT32 i, size;

    decoder->output_info.cbSize = 0;

    for (i = 0; i < decoder->output_type_count; ++i)
    {
        if (FAILED(hr = MFCalculateImageSize(decoder->output_types[i], width, height, &size)))
            return hr;
        decoder->output_info.cbSize = max(size, decoder->output_info.cbSize);
    }

    return hr;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (decoder->input_type)
        {
            IMFMediaType_Release(decoder->input_type);
            decoder->input_type = NULL;
        }
        if (decoder->output_type)
        {
            IMFMediaType_Release(decoder->output_type);
            decoder->output_type = NULL;
        }
        if (decoder->wg_transform)
        {
            wg_transform_destroy(decoder->wg_transform);
            decoder->wg_transform = 0;
        }

        return S_OK;
    }

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return E_INVALIDARG;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < decoder->input_type_count; ++i)
        if (IsEqualGUID(&subtype, decoder->input_types[i]))
            break;
    if (i == decoder->input_type_count)
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (decoder->output_type)
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }

    if (decoder->input_type)
        IMFMediaType_Release(decoder->input_type);
    IMFMediaType_AddRef((decoder->input_type = type));

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        if (FAILED(hr = IMFMediaType_SetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, frame_size)))
            WARN("Failed to update stream type frame size, hr %#lx\n", hr);
        if (FAILED(hr = update_output_info_size(decoder, frame_size >> 32, frame_size)))
            return hr;
    }

    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }
    return S_OK;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    UINT64 frame_size, stream_frame_size;
    IMFMediaType *output_type;
    GUID major, subtype;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (decoder->output_type)
        {
            IMFMediaType_Release(decoder->output_type);
            decoder->output_type = NULL;
        }
        if (decoder->wg_transform)
        {
            wg_transform_destroy(decoder->wg_transform);
            decoder->wg_transform = 0;
        }

        return S_OK;
    }

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; i < decoder->output_type_count; ++i)
        if (IsEqualGUID(&subtype, decoder->output_types[i]))
            break;
    if (i == decoder->output_type_count)
        return MF_E_INVALIDMEDIATYPE;

    if (FAILED(hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
        return MF_E_INVALIDMEDIATYPE;
    if (SUCCEEDED(IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, &stream_frame_size))
            && frame_size != stream_frame_size)
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (decoder->output_type)
        IMFMediaType_Release(decoder->output_type);
    IMFMediaType_AddRef((decoder->output_type = type));

    /* WMV decoder outputs RGB formats with default stride forced to negative, likely a
     * result of internal conversion to DMO media type */
    if (!decoder->IMediaObject_iface.lpVtbl)
    {
        output_type = decoder->output_type;
        IMFMediaType_AddRef(output_type);
    }
    else if (FAILED(hr = normalize_stride(decoder, &output_type)))
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
        return hr;
    }

    if (decoder->wg_transform)
        hr = wg_transform_set_output_type(decoder->wg_transform, output_type);
    else
        hr = try_create_wg_transform(decoder, output_type);

    IMFMediaType_Release(output_type);

    if (FAILED(hr))
    {
        IMFMediaType_Release(decoder->output_type);
        decoder->output_type = NULL;
    }
    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p\n", iface, id, type);

    if (!decoder->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    return IMFMediaType_CopyAllItems(decoder->input_type, (IMFAttributes *)*type);
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    GUID subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p\n", iface, id, type);

    if (!decoder->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    if (FAILED(hr = IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;
    return create_output_media_type(decoder, &subtype, decoder->output_type, type);
}

static HRESULT WINAPI transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, flags %p.\n", iface, id, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *flags = MFT_INPUT_STATUS_ACCEPT_DATA;
    return S_OK;
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
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, message %#x, param %Ix.\n", iface, message, param);

    switch (message)
    {
    case MFT_MESSAGE_SET_D3D_MANAGER:
        if (FAILED(hr = IMFVideoSampleAllocatorEx_SetDirectXManager(decoder->allocator, (IUnknown *)param)))
            return hr;

        uninit_allocator(decoder);
        if (param)
            decoder->output_info.dwFlags |= MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
        else
            decoder->output_info.dwFlags &= ~MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
        return S_OK;

    case MFT_MESSAGE_COMMAND_DRAIN:
        return decoder->wg_transform ? wg_transform_drain(decoder->wg_transform) : MF_E_TRANSFORM_TYPE_NOT_SET;

    case MFT_MESSAGE_COMMAND_FLUSH:
        return decoder->wg_transform ? wg_transform_flush(decoder->wg_transform) : MF_E_TRANSFORM_TYPE_NOT_SET;

    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        decoder->sample_time = -1;
        return S_OK;

    default:
        FIXME("Ignoring message %#x.\n", message);
        return S_OK;
    }
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (decoder->sample_time == -1 && FAILED(IMFSample_GetSampleTime(sample, (LONGLONG *)&decoder->sample_time)))
        decoder->sample_time = 0;

    if (!decoder->previous_stride)
        set_previous_stride(decoder);

    return wg_transform_push_mf(decoder->wg_transform, sample, decoder->wg_sample_queue);
}

static HRESULT output_sample(struct video_decoder *decoder, IMFSample **out, IMFSample *src_sample)
{
    MFT_OUTPUT_DATA_BUFFER output[1];
    IMFSample *sample;
    DWORD status;
    HRESULT hr;

    if (FAILED(hr = init_allocator(decoder)))
    {
        ERR("Failed to initialize allocator, hr %#lx.\n", hr);
        return hr;
    }
    if (FAILED(hr = IMFVideoSampleAllocatorEx_AllocateSample(decoder->allocator, &sample)))
        return hr;

    if (FAILED(hr = IMFTransform_ProcessInput(decoder->copier, 0, src_sample, 0)))
    {
        IMFSample_Release(sample);
        return hr;
    }
    output[0].pSample = sample;
    if (FAILED(hr = IMFTransform_ProcessOutput(decoder->copier, 0, 1, output, &status)))
    {
        IMFSample_Release(sample);
        return hr;
    }
    *out = sample;
    return S_OK;
}

static HRESULT handle_stream_type_change(struct video_decoder *decoder)
{
    UINT64 frame_size, frame_rate;
    HRESULT hr;

    if (decoder->stream_type)
        IMFMediaType_Release(decoder->stream_type);
    if (FAILED(hr = wg_transform_get_output_type(decoder->wg_transform, &decoder->stream_type)))
    {
        WARN("Failed to get transform output type, hr %#lx\n", hr);
        return hr;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_RATE, &frame_rate))
            && FAILED(hr = IMFMediaType_SetUINT64(decoder->stream_type, &MF_MT_FRAME_RATE, frame_rate)))
            WARN("Failed to update stream type frame size, hr %#lx\n", hr);

    if (FAILED(hr = IMFMediaType_GetUINT64(decoder->stream_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;
    if (FAILED(hr = update_output_info_size(decoder, frame_size >> 32, frame_size)))
        return hr;
    uninit_allocator(decoder);

    return MF_E_TRANSFORM_STREAM_CHANGE;
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct video_decoder *decoder = impl_from_IMFTransform(iface);
    UINT32 sample_size;
    LONGLONG duration;
    IMFSample *sample;
    UINT64 frame_size, frame_rate;
    bool preserve_timestamps;
    GUID subtype;
    DWORD size;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!decoder->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!(sample = samples->pSample) && !(decoder->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES))
        return E_INVALIDARG;

    if (FAILED(hr = IMFMediaType_GetGUID(decoder->output_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;
    if (FAILED(hr = IMFMediaType_GetUINT64(decoder->output_type, &MF_MT_FRAME_SIZE, &frame_size)))
        return hr;
    if (FAILED(hr = MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size, &sample_size)))
        return hr;

    if (decoder->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)
    {
        if (decoder->temp_buffer)
        {
            if (FAILED(IMFMediaBuffer_GetMaxLength(decoder->temp_buffer, &size)) || size < sample_size)
            {
                IMFMediaBuffer_Release(decoder->temp_buffer);
                decoder->temp_buffer = NULL;
            }
        }
        if (!decoder->temp_buffer && FAILED(hr = MFCreateMemoryBuffer(sample_size, &decoder->temp_buffer)))
            return hr;
        if (FAILED(hr = MFCreateSample(&sample)))
            return hr;
        if (FAILED(hr = IMFSample_AddBuffer(sample, decoder->temp_buffer)))
        {
            IMFSample_Release(sample);
            return hr;
        }
    }

    if (SUCCEEDED(hr = wg_transform_read_mf(decoder->wg_transform, sample,
            sample_size, &samples->dwStatus, &preserve_timestamps)))
    {
        wg_sample_queue_flush(decoder->wg_sample_queue, false);

        if (decoder->IMediaObject_iface.lpVtbl)
            duration = 0; /* WMV decoder doesn't output any timestamp or duration */
        else
        {
            if (FAILED(IMFMediaType_GetUINT64(decoder->input_type, &MF_MT_FRAME_RATE, &frame_rate)))
                frame_rate = (UINT64)30000 << 32 | 1001;
            duration = MulDiv(10000000, (UINT32)frame_rate, frame_rate >> 32);
        }

        if (!preserve_timestamps)
        {
            if (FAILED(IMFSample_SetSampleTime(sample, decoder->sample_time)))
                WARN("Failed to set sample time\n");
            if (FAILED(IMFSample_SetSampleDuration(sample, duration)))
                WARN("Failed to set sample duration\n");
            decoder->sample_time += duration;
        }
    }

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        samples[0].dwStatus |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        *status |= MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE;
        hr = handle_stream_type_change(decoder);
    }

    if (decoder->output_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)
    {
        if (hr == S_OK && FAILED(hr = output_sample(decoder, &samples->pSample, sample)))
            ERR("Failed to output sample, hr %#lx.\n", hr);
        IMFSample_Release(sample);
    }

    return hr;
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

static inline struct video_decoder *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct video_decoder, IMediaObject_iface);
}

static HRESULT WINAPI media_object_QueryInterface(IMediaObject *iface, REFIID iid, void **obj)
{
    return IUnknown_QueryInterface(impl_from_IMediaObject(iface)->outer, iid, obj);
}

static ULONG WINAPI media_object_AddRef(IMediaObject *iface)
{
    return  IUnknown_AddRef(impl_from_IMediaObject(iface)->outer);
}

static ULONG WINAPI media_object_Release(IMediaObject *iface)
{
    return  IUnknown_Release(impl_from_IMediaObject(iface)->outer);
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
    struct video_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= decoder->input_type_count)
        return DMO_E_NO_MORE_ITEMS;
    if (!type)
        return S_OK;

    memset(type, 0, sizeof(*type));
    type->majortype = MFMediaType_Video;
    type->subtype = *get_dmo_subtype(decoder->input_types[type_index]);
    type->bFixedSizeSamples = FALSE;
    type->bTemporalCompression = TRUE;
    type->lSampleSize = 0;

    return S_OK;
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    struct video_decoder *decoder = impl_from_IMediaObject(iface);
    UINT64 frame_size, frame_rate;
    IMFMediaType *media_type;
    VIDEOINFOHEADER *info;
    const GUID *subtype;
    LONG width, height;
    UINT32 image_size;
    HRESULT hr;

    TRACE("iface %p, index %lu, type_index %lu, type %p.\n", iface, index, type_index, type);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (type_index >= decoder->output_type_count)
        return DMO_E_NO_MORE_ITEMS;
    if (!type)
        return S_OK;
    if (IsEqualGUID(&decoder->dmo_input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION,
            &decoder->dmo_input_type, &media_type)))
        return hr;

    if (FAILED(IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size)))
        frame_size = 0;
    if (FAILED(IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &frame_rate)))
        frame_rate = (UINT64)1 << 32 | 1;

    width = frame_size >> 32;
    height = (UINT32)frame_size;
    subtype = get_dmo_subtype(decoder->output_types[type_index]);
    if (FAILED(hr = MFCalculateImageSize(subtype, width, height, &image_size)))
    {
        FIXME("Failed to get image size of subtype %s.\n", debugstr_guid(subtype));
        IMFMediaType_Release(media_type);
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
    if (frame_rate)
        MFFrameRateToAverageTimePerFrame(frame_rate >> 32, frame_rate, (UINT64 *)&info->AvgTimePerFrame);
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth  = width;
    info->bmiHeader.biHeight = height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = get_subtype_bpp(subtype);
    info->bmiHeader.biCompression = get_subtype_compression(subtype);
    info->bmiHeader.biSizeImage = image_size;

    IMFMediaType_Release(media_type);
    return S_OK;
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    struct video_decoder *decoder = impl_from_IMediaObject(iface);
    IMFMediaType *media_type;
    unsigned int i;

    TRACE("iface %p, index %lu, type %p, flags %#lx.\n", iface, index, type, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!type)
    {
        if (flags & DMO_SET_TYPEF_CLEAR)
        {
            FreeMediaType(&decoder->dmo_input_type);
            memset(&decoder->dmo_input_type, 0, sizeof(decoder->dmo_input_type));
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

    for (i = 0; i < decoder->input_type_count; ++i)
        if (IsEqualGUID(&type->subtype, get_dmo_subtype(decoder->input_types[i])))
            break;
    if (i == decoder->input_type_count)
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (FAILED(MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION,
            (void *)type, &media_type)))
        return DMO_E_TYPE_NOT_ACCEPTED;
    IMFMediaType_Release(media_type);

    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    FreeMediaType(&decoder->dmo_input_type);
    CopyMediaType(&decoder->dmo_input_type, type);
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
    struct video_decoder *decoder = impl_from_IMediaObject(iface);
    IMFMediaType *media_type;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, index %lu, type %p, flags %#lx,\n", iface, index, type, flags);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;

    if (!type)
    {
        if (flags & DMO_SET_TYPEF_CLEAR)
        {
            FreeMediaType(&decoder->dmo_output_type);
            memset(&decoder->dmo_output_type, 0, sizeof(decoder->dmo_output_type));
            if (decoder->wg_transform)
            {
                wg_transform_destroy(decoder->wg_transform);
                decoder->wg_transform = 0;
            }
            return S_OK;
        }
        return E_POINTER;
    }

    if (IsEqualGUID(&decoder->dmo_input_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Video))
        return DMO_E_TYPE_NOT_ACCEPTED;

    for (i = 0; i < decoder->output_type_count; ++i)
        if (IsEqualGUID(&type->subtype, get_dmo_subtype(decoder->output_types[i])))
            break;
    if (i == decoder->output_type_count)
        return DMO_E_TYPE_NOT_ACCEPTED;

    if (FAILED(MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION,
            (void *)type, &media_type)))
        return DMO_E_TYPE_NOT_ACCEPTED;
    IMFMediaType_Release(media_type);

    if (flags & DMO_SET_TYPEF_TEST_ONLY)
        return S_OK;

    FreeMediaType(&decoder->dmo_output_type);
    CopyMediaType(&decoder->dmo_output_type, type);

    /* Set up wg_transform. */
    if (decoder->wg_transform)
    {
        wg_transform_destroy(decoder->wg_transform);
        decoder->wg_transform = 0;
    }
    if (FAILED(hr = wg_transform_create_quartz(&decoder->dmo_input_type, type,
            &decoder->wg_transform_attrs, &decoder->wg_transform)))
        return hr;

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
    struct video_decoder *decoder = impl_from_IMediaObject(iface);
    IMFMediaType *media_type;
    HRESULT hr;

    TRACE("iface %p, index %lu, size %p, alignment %p.\n", iface, index, size, alignment);

    if (index > 0)
        return DMO_E_INVALIDSTREAMINDEX;
    if (IsEqualGUID(&decoder->dmo_output_type.majortype, &GUID_NULL))
        return DMO_E_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;
    if (SUCCEEDED(hr = MFInitMediaTypeFromAMMediaType(media_type, &decoder->dmo_output_type))
            && SUCCEEDED(hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, (UINT32 *)size)))
        *alignment = 1;
    IMFMediaType_Release(media_type);

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
    struct video_decoder *decoder = impl_from_IMediaObject(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

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
    struct video_decoder *decoder = impl_from_IMediaObject(iface);

    TRACE("iface %p, index %lu, buffer %p, flags %#lx, timestamp %s, timelength %s.\n", iface,
             index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));

    if (!decoder->wg_transform)
        return DMO_E_TYPE_NOT_SET;

    return wg_transform_push_dmo(decoder->wg_transform, buffer, flags, timestamp, timelength, decoder->wg_sample_queue);
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct video_decoder *decoder = impl_from_IMediaObject(iface);
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

static inline struct video_decoder *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct video_decoder, IPropertyBag_iface);
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

static inline struct video_decoder *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct video_decoder, IPropertyStore_iface);
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

static HRESULT video_decoder_create_with_types(const GUID *const *input_types, UINT input_type_count,
        const GUID *const *output_types, UINT output_type_count, IUnknown *outer, struct video_decoder **out)
{
    struct video_decoder *decoder;
    HRESULT hr;

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;

    decoder->IUnknown_inner.lpVtbl = &unknown_vtbl;
    decoder->IMFTransform_iface.lpVtbl = &transform_vtbl;
    decoder->refcount = 1;
    decoder->outer = outer ? outer : &decoder->IUnknown_inner;

    decoder->input_type_count = input_type_count;
    decoder->input_types = input_types;
    decoder->output_type_count = output_type_count;
    decoder->output_types = output_types;

    if (FAILED(hr = MFCreateMediaType(&decoder->stream_type)))
        goto failed;
    if (FAILED(hr = MFCreateAttributes(&decoder->attributes, 16)))
        goto failed;
    if (FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_LOW_LATENCY, FALSE))
            || FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_SA_D3D_AWARE, TRUE))
            || FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &MF_SA_D3D11_AWARE, TRUE))
            || FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes,
                    &MFT_DECODER_EXPOSE_OUTPUT_TYPES_IN_NATIVE_ORDER, FALSE)))
        goto failed;

    if (FAILED(hr = MFCreateAttributes(&decoder->output_attributes, 0)))
        goto failed;
    if (FAILED(hr = wg_sample_queue_create(&decoder->wg_sample_queue)))
        goto failed;
    if (FAILED(hr = MFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&decoder->allocator)))
        goto failed;
    if (FAILED(hr = MFCreateSampleCopierMFT(&decoder->copier)))
        goto failed;

    decoder->wg_transform_attrs.input_queue_length = 15;
    decoder->wg_transform_attrs.preserve_timestamps = TRUE;

    *out = decoder;
    TRACE("Created decoder %p\n", decoder);
    return S_OK;

failed:
    if (decoder->allocator)
        IMFVideoSampleAllocatorEx_Release(decoder->allocator);
    if (decoder->wg_sample_queue)
        wg_sample_queue_destroy(decoder->wg_sample_queue);
    if (decoder->output_attributes)
        IMFAttributes_Release(decoder->output_attributes);
    if (decoder->attributes)
        IMFAttributes_Release(decoder->attributes);
    if (decoder->stream_type)
        IMFMediaType_Release(decoder->stream_type);
    free(decoder);
    return hr;
}

static const GUID *const h264_decoder_input_types[] =
{
    &MFVideoFormat_H264,
    &MFVideoFormat_H264_ES,
};

HRESULT h264_decoder_create(REFIID riid, void **out)
{
    const MFVIDEOFORMAT output_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_I420,
    };
    const MFVIDEOFORMAT input_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .guidFormat = MFVideoFormat_H264,
    };
    struct video_decoder *decoder;
    HRESULT hr;

    TRACE("riid %s, out %p.\n", debugstr_guid(riid), out);

    if (FAILED(hr = check_video_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support H.264 decoding, please install appropriate plugins\n");
        return hr;
    }

    if (FAILED(hr = video_decoder_create_with_types(h264_decoder_input_types, ARRAY_SIZE(h264_decoder_input_types),
            video_decoder_output_types, ARRAY_SIZE(video_decoder_output_types), NULL, &decoder)))
        return hr;

    if (FAILED(hr = IMFAttributes_SetUINT32(decoder->attributes, &CODECAPI_AVDecVideoAcceleration_H264, TRUE)))
    {
        IMFTransform_Release(&decoder->IMFTransform_iface);
        return hr;
    }

    decoder->input_info.dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE;
    decoder->input_info.cbSize = 0x1000;
    decoder->output_info.dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER
            | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
    decoder->output_info.cbSize = 1920 * 1088 * 2;

    decoder->wg_transform_attrs.output_plane_align = 15;
    decoder->wg_transform_attrs.allow_format_change = TRUE;

    TRACE("Created h264 transform %p.\n", &decoder->IMFTransform_iface);

    hr = IMFTransform_QueryInterface(&decoder->IMFTransform_iface, riid, out);
    IMFTransform_Release(&decoder->IMFTransform_iface);
    return hr;
}

extern GUID MFVideoFormat_IV50;
static const GUID *const iv50_decoder_input_types[] =
{
    &MFVideoFormat_IV50,
};
static const GUID *const iv50_decoder_output_types[] =
{
    &MFVideoFormat_YV12,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_NV11,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB8,
};

HRESULT WINAPI winegstreamer_create_video_decoder(IMFTransform **out)
{
    struct video_decoder *decoder;
    HRESULT hr;

    TRACE("out %p.\n", out);

    if (!init_gstreamer())
        return E_FAIL;

    if (FAILED(hr = video_decoder_create_with_types(iv50_decoder_input_types, ARRAY_SIZE(iv50_decoder_input_types),
            iv50_decoder_output_types, ARRAY_SIZE(iv50_decoder_output_types), NULL, &decoder)))
        return hr;

    TRACE("Created iv50 transform %p.\n", &decoder->IMFTransform_iface);

    *out = &decoder->IMFTransform_iface;
    return S_OK;
}

extern const GUID MEDIASUBTYPE_VC1S;
extern const GUID MEDIASUBTYPE_WMV_Unknown;
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
static const GUID *const wmv_decoder_output_types[] =
{
    &MFVideoFormat_NV12,
    &MFVideoFormat_YV12,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_I420,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_NV11,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_RGB555,
    &MFVideoFormat_RGB8,
};

HRESULT wmv_decoder_create(IUnknown *outer, IUnknown **out)
{
    const MFVIDEOFORMAT output_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_I420,
    };
    const MFVIDEOFORMAT input_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_WMV3,
    };
    struct video_decoder *decoder;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    if (FAILED(hr = check_video_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support WMV decoding, please install appropriate plugins\n");
        return hr;
    }

    if (FAILED(hr = video_decoder_create_with_types(wmv_decoder_input_types, ARRAY_SIZE(wmv_decoder_input_types),
            wmv_decoder_output_types, ARRAY_SIZE(wmv_decoder_output_types), outer, &decoder)))
        return hr;

    decoder->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    decoder->IPropertyBag_iface.lpVtbl = &property_bag_vtbl;
    decoder->IPropertyStore_iface.lpVtbl = &property_store_vtbl;

    TRACE("Created wmv transform %p, media object %p.\n",
            &decoder->IMFTransform_iface, &decoder->IMediaObject_iface);

    *out = &decoder->IUnknown_inner;
    return S_OK;
}
