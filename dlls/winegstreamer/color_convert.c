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
#include "mfobjects.h"
#include "mftransform.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const GUID *const input_types[] =
{
    &MFVideoFormat_YV12,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_I420,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB555,
    &MEDIASUBTYPE_RGB8,
    &MEDIASUBTYPE_V216,
    &MEDIASUBTYPE_V410,
    &MFVideoFormat_NV11,
    &MFVideoFormat_Y41P,
    &MFVideoFormat_Y41T,
    &MFVideoFormat_Y42T,
    &MFVideoFormat_YVU9,
};
static const GUID *const output_types[] =
{
    &MFVideoFormat_YV12,
    &MFVideoFormat_YUY2,
    &MFVideoFormat_UYVY,
    &MFVideoFormat_AYUV,
    &MFVideoFormat_NV12,
    &MFVideoFormat_RGB32,
    &MFVideoFormat_RGB565,
    &MFVideoFormat_I420,
    &MFVideoFormat_IYUV,
    &MFVideoFormat_YVYU,
    &MFVideoFormat_RGB24,
    &MFVideoFormat_RGB555,
    &MEDIASUBTYPE_RGB8,
    &MEDIASUBTYPE_V216,
    &MEDIASUBTYPE_V410,
    &MFVideoFormat_NV11,
};

struct color_convert
{
    IUnknown IUnknown_inner;
    IMFTransform IMFTransform_iface;
    IMediaObject IMediaObject_iface;
    IPropertyBag IPropertyBag_iface;
    IPropertyStore IPropertyStore_iface;
    IUnknown *outer;
    LONG refcount;

    IMFMediaType *input_type;
    MFT_INPUT_STREAM_INFO input_info;
    IMFMediaType *output_type;
    MFT_OUTPUT_STREAM_INFO output_info;

    wg_transform_t wg_transform;
    struct wg_sample_queue *wg_sample_queue;
};

static inline struct color_convert *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IUnknown_inner);
}

static void update_video_aperture(MFVideoInfo *input_info, MFVideoInfo *output_info)
{
    static const MFVideoArea empty_area = {0};

    /* Tests show that the color converter ignores aperture entirely, probably a side
     * effect of an internal conversion to VIDEOINFOHEADER2, as the component is also
     * exposing a IMediaObject interface, and designed for dshow.
     */

    input_info->GeometricAperture = empty_area;
    input_info->MinimumDisplayAperture = empty_area;
    input_info->PanScanAperture = empty_area;

    output_info->GeometricAperture = empty_area;
    output_info->MinimumDisplayAperture = empty_area;
    output_info->PanScanAperture = empty_area;
}

static HRESULT normalize_media_types(IMFMediaType **input_type, IMFMediaType **output_type)
{
    MFVIDEOFORMAT *input_format, *output_format;
    UINT32 size;
    HRESULT hr;

    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(*input_type, &input_format, &size)))
        return hr;
    if (FAILED(hr = MFCreateMFVideoFormatFromMFMediaType(*output_type, &output_format, &size)))
    {
        CoTaskMemFree(input_format);
        return hr;
    }

    update_video_aperture(&input_format->videoInfo, &output_format->videoInfo);

    if (FAILED(hr = MFCreateVideoMediaType(input_format, (IMFVideoMediaType **)input_type)))
        goto done;
    if (FAILED(hr = MFCreateVideoMediaType(output_format, (IMFVideoMediaType **)output_type)))
    {
        IMFMediaType_Release(*input_type);
        *input_type = NULL;
    }

done:
    CoTaskMemFree(input_format);
    CoTaskMemFree(output_format);
    return hr;
}

static HRESULT try_create_wg_transform(struct color_convert *impl)
{
    IMFMediaType *input_type = impl->input_type, *output_type = impl->output_type;
    struct wg_transform_attrs attrs = {0};
    HRESULT hr;

    if (impl->wg_transform)
    {
        wg_transform_destroy(impl->wg_transform);
        impl->wg_transform = 0;
    }

    if (FAILED(hr = normalize_media_types(&input_type, &output_type)))
        return hr;
    hr = wg_transform_create_mf(input_type, output_type, &attrs, &impl->wg_transform);
    IMFMediaType_Release(output_type);
    IMFMediaType_Release(input_type);

    return hr;
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct color_convert *impl = impl_from_IUnknown(iface);

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
    struct color_convert *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&impl->refcount);

    TRACE("iface %p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct color_convert *impl = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (impl->wg_transform)
            wg_transform_destroy(impl->wg_transform);
        if (impl->input_type)
            IMFMediaType_Release(impl->input_type);
        if (impl->output_type)
            IMFMediaType_Release(impl->output_type);

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

static struct color_convert *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IMFTransform_iface);
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
    struct color_convert *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (!impl->input_type || !impl->output_type)
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *info = impl->input_info;
    return S_OK;
}

static HRESULT WINAPI transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, info %p.\n", iface, id, info);

    if (!impl->input_type || !impl->output_type)
    {
        memset(info, 0, sizeof(*info));
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    *info = impl->output_info;
    return S_OK;
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
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (index >= ARRAY_SIZE(input_types))
        return MF_E_NO_MORE_TYPES;
    subtype = input_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, subtype)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    IMFMediaType *media_type;
    const GUID *subtype;
    HRESULT hr;

    TRACE("iface %p, id %#lx, index %#lx, type %p.\n", iface, id, index, type);

    *type = NULL;

    if (index >= ARRAY_SIZE(output_types))
        return MF_E_NO_MORE_TYPES;
    subtype = output_types[index];

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, subtype)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1)))
        goto done;
    if (FAILED(hr = IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1)))
        goto done;

    IMFMediaType_AddRef((*type = media_type));

done:
    IMFMediaType_Release(media_type);
    return hr;
}

static HRESULT WINAPI transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    UINT32 stride;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (impl->input_type)
        {
            IMFMediaType_Release(impl->input_type);
            impl->input_type = NULL;
        }
        if (impl->wg_transform)
        {
            wg_transform_destroy(impl->wg_transform);
            impl->wg_transform = 0;
        }

        return S_OK;
    }

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_ATTRIBUTENOTFOUND;

    if (!IsEqualGUID(&major, &MFMediaType_Video)
            || IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size))
        return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(input_types); ++i)
        if (IsEqualGUID(&subtype, input_types[i]))
            break;
    if (i == ARRAY_SIZE(input_types))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (!impl->input_type && FAILED(hr = MFCreateMediaType(&impl->input_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)impl->input_type)))
    {
        IMFMediaType_Release(impl->input_type);
        impl->input_type = NULL;
    }
    if (FAILED(IMFMediaType_GetUINT32(impl->input_type, &MF_MT_DEFAULT_STRIDE, &stride)))
    {
        if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, frame_size >> 32, (LONG *)&stride)))
        {
            IMFMediaType_Release(impl->input_type);
            impl->input_type = NULL;
        }
        if (FAILED(hr = IMFMediaType_SetUINT32(impl->input_type, &MF_MT_DEFAULT_STRIDE, abs((INT32)stride))))
        {
            IMFMediaType_Release(impl->input_type);
            impl->input_type = NULL;
        }
    }

    if (impl->output_type && FAILED(hr = try_create_wg_transform(impl)))
    {
        IMFMediaType_Release(impl->input_type);
        impl->input_type = NULL;
    }

    if (FAILED(hr) || FAILED(MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size,
            (UINT32 *)&impl->input_info.cbSize)))
        impl->input_info.cbSize = 0;

    return hr;
}

static HRESULT WINAPI transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    GUID major, subtype;
    UINT64 frame_size;
    UINT32 stride;
    HRESULT hr;
    ULONG i;

    TRACE("iface %p, id %#lx, type %p, flags %#lx.\n", iface, id, type, flags);

    if (!type)
    {
        if (impl->output_type)
        {
            IMFMediaType_Release(impl->output_type);
            impl->output_type = NULL;
        }
        if (impl->wg_transform)
        {
            wg_transform_destroy(impl->wg_transform);
            impl->wg_transform = 0;
        }

        return S_OK;
    }

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)) ||
            FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return MF_E_ATTRIBUTENOTFOUND;

    if (!IsEqualGUID(&major, &MFMediaType_Video)
            || IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size))
        return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(output_types); ++i)
        if (IsEqualGUID(&subtype, output_types[i]))
            break;
    if (i == ARRAY_SIZE(output_types))
        return MF_E_INVALIDMEDIATYPE;
    if (flags & MFT_SET_TYPE_TEST_ONLY)
        return S_OK;

    if (!impl->output_type && FAILED(hr = MFCreateMediaType(&impl->output_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)impl->output_type)))
    {
        IMFMediaType_Release(impl->output_type);
        impl->output_type = NULL;
    }
    if (FAILED(IMFMediaType_GetUINT32(impl->output_type, &MF_MT_DEFAULT_STRIDE, &stride)))
    {
        if (FAILED(hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, frame_size >> 32, (LONG *)&stride)))
        {
            IMFMediaType_Release(impl->output_type);
            impl->output_type = NULL;
        }
        if (FAILED(hr = IMFMediaType_SetUINT32(impl->output_type, &MF_MT_DEFAULT_STRIDE, abs((INT32)stride))))
        {
            IMFMediaType_Release(impl->output_type);
            impl->output_type = NULL;
        }
    }

    if (impl->input_type && FAILED(hr = try_create_wg_transform(impl)))
    {
        IMFMediaType_Release(impl->output_type);
        impl->output_type = NULL;
    }

    if (FAILED(hr) || FAILED(MFCalculateImageSize(&subtype, frame_size >> 32, (UINT32)frame_size,
            (UINT32 *)&impl->output_info.cbSize)))
        impl->output_info.cbSize = 0;

    return hr;
}

static HRESULT WINAPI transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!impl->input_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(impl->input_type, (IMFAttributes *)*type)))
        IMFMediaType_Release(*type);

    return hr;
}

static HRESULT WINAPI transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    HRESULT hr;

    TRACE("iface %p, id %#lx, type %p.\n", iface, id, type);

    if (id != 0)
        return MF_E_INVALIDSTREAMNUMBER;

    if (!impl->output_type)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    if (FAILED(hr = IMFMediaType_CopyAllItems(impl->output_type, (IMFAttributes *)*type)))
        IMFMediaType_Release(*type);

    return hr;
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
    return S_OK;
}

static HRESULT WINAPI transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);

    TRACE("iface %p, id %#lx, sample %p, flags %#lx.\n", iface, id, sample, flags);

    if (!impl->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    return wg_transform_push_mf(impl->wg_transform, sample, impl->wg_sample_queue);
}

static HRESULT WINAPI transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    struct color_convert *impl = impl_from_IMFTransform(iface);
    MFT_OUTPUT_STREAM_INFO info;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, count %lu, samples %p, status %p.\n", iface, flags, count, samples, status);

    if (count != 1)
        return E_INVALIDARG;

    if (!impl->wg_transform)
        return MF_E_TRANSFORM_TYPE_NOT_SET;

    *status = samples->dwStatus = 0;
    if (!samples->pSample)
        return E_INVALIDARG;

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(iface, 0, &info)))
        return hr;

    if (SUCCEEDED(hr = wg_transform_read_mf(impl->wg_transform, samples->pSample,
            info.cbSize, &samples->dwStatus, NULL)))
        wg_sample_queue_flush(impl->wg_sample_queue, false);

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

static inline struct color_convert *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IMediaObject_iface);
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
    FIXME("iface %p, input %p, output %p stub!\n", iface, input, output);
    return E_NOTIMPL;
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
    FIXME("iface %p, index %lu, type_index %lu, type %p stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index,
        DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %lu, type_index %lu, type %p stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetInputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %lu, type %p, flags %#lx stub!\n", iface, index, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_SetOutputType(IMediaObject *iface, DWORD index,
        const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %lu, type %p, flags %#lx stub!\n", iface, index, type, flags);
    return E_NOTIMPL;
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
    FIXME("iface %p, index %lu, size %p, alignment %p stub!\n", iface, index, size, alignment);
    return E_NOTIMPL;
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
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("iface %p, index %lu stub!\n", iface, index);
    return E_NOTIMPL;
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
    FIXME("iface %p, index %lu, flags %p stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_ProcessInput(IMediaObject *iface, DWORD index,
        IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    FIXME("iface %p, index %lu, buffer %p, flags %#lx, timestamp %s, timelength %s stub!\n", iface,
            index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));
    return E_NOTIMPL;
}

static HRESULT WINAPI media_object_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
        DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    FIXME("iface %p, flags %#lx, count %lu, buffers %p, status %p stub!\n", iface, flags, count, buffers, status);
    return E_NOTIMPL;
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

static inline struct color_convert *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IPropertyBag_iface);
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

static inline struct color_convert *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct color_convert, IPropertyStore_iface);
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

HRESULT color_convert_create(IUnknown *outer, IUnknown **out)
{
    const MFVIDEOFORMAT input_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_I420,
    };
    const MFVIDEOFORMAT output_format =
    {
        .dwSize = sizeof(MFVIDEOFORMAT),
        .videoInfo = {.dwWidth = 1920, .dwHeight = 1080},
        .guidFormat = MFVideoFormat_NV12,
    };
    struct color_convert *impl;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    if (FAILED(hr = check_video_transform_support(&input_format, &output_format)))
    {
        ERR_(winediag)("GStreamer doesn't support video conversion, please install appropriate plugins.\n");
        return hr;
    }

    if (!(impl = calloc(1, sizeof(*impl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wg_sample_queue_create(&impl->wg_sample_queue)))
    {
        free(impl);
        return hr;
    }

    impl->IUnknown_inner.lpVtbl = &unknown_vtbl;
    impl->IMFTransform_iface.lpVtbl = &transform_vtbl;
    impl->IMediaObject_iface.lpVtbl = &media_object_vtbl;
    impl->IPropertyBag_iface.lpVtbl = &property_bag_vtbl;
    impl->IPropertyStore_iface.lpVtbl = &property_store_vtbl;
    impl->refcount = 1;
    impl->outer = outer ? outer : &impl->IUnknown_inner;

    impl->input_info.cbAlignment = 1;
    impl->output_info.cbAlignment = 1;

    *out = &impl->IUnknown_inner;
    TRACE("Created %p\n", *out);
    return S_OK;
}
