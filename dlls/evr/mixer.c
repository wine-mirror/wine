/*
 * Copyright 2020 Nikolay Sivov
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

#include "evr.h"
#include "d3d9.h"
#include "dxva2api.h"
#include "mfapi.h"
#include "mferror.h"

#include "evr_classes.h"

#include "initguid.h"
#include "evr9.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

#define MAX_MIXER_INPUT_STREAMS 16

struct input_stream
{
    unsigned int id;
    IMFAttributes *attributes;
    IMFMediaType *media_type;
    MFVideoNormalizedRect rect;
    unsigned int zorder;
    IMFSample *sample;
};

struct rt_format
{
    GUID device;
    D3DFORMAT format;
    IMFMediaType *media_type;
};

struct output_stream
{
    IMFMediaType *media_type;
    struct rt_format *rt_formats;
    unsigned int rt_formats_count;
};

struct video_mixer
{
    IMFTransform IMFTransform_iface;
    IMFVideoDeviceID IMFVideoDeviceID_iface;
    IMFTopologyServiceLookupClient IMFTopologyServiceLookupClient_iface;
    IMFVideoMixerControl2 IMFVideoMixerControl2_iface;
    IMFGetService IMFGetService_iface;
    IMFVideoMixerBitmap IMFVideoMixerBitmap_iface;
    IMFVideoPositionMapper IMFVideoPositionMapper_iface;
    IMFVideoProcessor IMFVideoProcessor_iface;
    IMFAttributes IMFAttributes_iface;
    IMFQualityAdvise IMFQualityAdvise_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;

    struct input_stream inputs[MAX_MIXER_INPUT_STREAMS];
    unsigned int input_ids[MAX_MIXER_INPUT_STREAMS];
    unsigned int input_count;
    struct output_stream output;

    IDirect3DDeviceManager9 *device_manager;
    IDirectXVideoProcessor *processor;
    HANDLE device_handle;

    IMediaEventSink *event_sink;
    IMFAttributes *attributes;
    IMFAttributes *internal_attributes;
    unsigned int mixing_flags;
    unsigned int is_streaming;
    COLORREF bkgnd_color;
    LONGLONG lower_bound;
    LONGLONG upper_bound;
    CRITICAL_SECTION cs;
};

static struct video_mixer *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IUnknown_inner);
}

static struct video_mixer *impl_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFTransform_iface);
}

static struct video_mixer *impl_from_IMFVideoDeviceID(IMFVideoDeviceID *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFVideoDeviceID_iface);
}

static struct video_mixer *impl_from_IMFTopologyServiceLookupClient(IMFTopologyServiceLookupClient *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFTopologyServiceLookupClient_iface);
}

static struct video_mixer *impl_from_IMFVideoMixerControl2(IMFVideoMixerControl2 *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFVideoMixerControl2_iface);
}

static struct video_mixer *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFGetService_iface);
}

static struct video_mixer *impl_from_IMFVideoMixerBitmap(IMFVideoMixerBitmap *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFVideoMixerBitmap_iface);
}

static struct video_mixer *impl_from_IMFVideoPositionMapper(IMFVideoPositionMapper *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFVideoPositionMapper_iface);
}

static struct video_mixer *impl_from_IMFVideoProcessor(IMFVideoProcessor *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFVideoProcessor_iface);
}

static struct video_mixer *impl_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFAttributes_iface);
}

static struct video_mixer *impl_from_IMFQualityAdvise(IMFQualityAdvise *iface)
{
    return CONTAINING_RECORD(iface, struct video_mixer, IMFQualityAdvise_iface);
}

static int video_mixer_compare_input_id(const void *a, const void *b)
{
    const unsigned int *key = a;
    const struct input_stream *input = b;

    if (*key > input->id) return 1;
    if (*key < input->id) return -1;
    return 0;
}

static HRESULT video_mixer_get_input(const struct video_mixer *mixer, unsigned int id, struct input_stream **stream)
{
    *stream = bsearch(&id, mixer->inputs, mixer->input_count, sizeof(*mixer->inputs), video_mixer_compare_input_id);
    return *stream ? S_OK : MF_E_INVALIDSTREAMNUMBER;
}

static void video_mixer_init_input(struct input_stream *stream)
{
    if (SUCCEEDED(MFCreateAttributes(&stream->attributes, 1)))
        IMFAttributes_SetUINT32(stream->attributes, &MF_SA_REQUIRED_SAMPLE_COUNT, 1);
    stream->rect.left = stream->rect.top = 0.0f;
    stream->rect.right = stream->rect.bottom = 1.0f;
}

static void video_mixer_clear_types(struct video_mixer *mixer)
{
    unsigned int i;

    for (i = 0; i < mixer->input_count; ++i)
    {
        if (mixer->inputs[i].media_type)
            IMFMediaType_Release(mixer->inputs[i].media_type);
        mixer->inputs[i].media_type = NULL;
        if (mixer->inputs[i].sample)
            IMFSample_Release(mixer->inputs[i].sample);
        mixer->inputs[i].sample = NULL;
    }
    for (i = 0; i < mixer->output.rt_formats_count; ++i)
    {
        IMFMediaType_Release(mixer->output.rt_formats[i].media_type);
    }
    heap_free(mixer->output.rt_formats);
    if (mixer->output.media_type)
        IMFMediaType_Release(mixer->output.media_type);
    mixer->output.media_type = NULL;
}

static HRESULT WINAPI video_mixer_inner_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IUnknown(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFTransform))
    {
        *obj = &mixer->IMFTransform_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoDeviceID))
    {
        *obj = &mixer->IMFVideoDeviceID_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFTopologyServiceLookupClient))
    {
        *obj = &mixer->IMFTopologyServiceLookupClient_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoMixerControl2) ||
            IsEqualIID(riid, &IID_IMFVideoMixerControl))
    {
        *obj = &mixer->IMFVideoMixerControl2_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &mixer->IMFGetService_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoMixerBitmap))
    {
        *obj = &mixer->IMFVideoMixerBitmap_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoPositionMapper))
    {
        *obj = &mixer->IMFVideoPositionMapper_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoProcessor))
    {
        *obj = &mixer->IMFVideoProcessor_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFAttributes))
    {
        *obj = &mixer->IMFAttributes_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFQualityAdvise))
    {
        *obj = &mixer->IMFQualityAdvise_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI video_mixer_inner_AddRef(IUnknown *iface)
{
    struct video_mixer *mixer = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&mixer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static void video_mixer_release_device_manager(struct video_mixer *mixer)
{
    if (mixer->processor)
        IDirectXVideoProcessor_Release(mixer->processor);
    if (mixer->device_manager)
    {
        IDirect3DDeviceManager9_CloseDeviceHandle(mixer->device_manager, mixer->device_handle);
        IDirect3DDeviceManager9_Release(mixer->device_manager);
    }
    mixer->device_handle = NULL;
    mixer->device_manager = NULL;
    mixer->processor = NULL;
}

static ULONG WINAPI video_mixer_inner_Release(IUnknown *iface)
{
    struct video_mixer *mixer = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&mixer->refcount);
    unsigned int i;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < mixer->input_count; ++i)
        {
            if (mixer->inputs[i].attributes)
                IMFAttributes_Release(mixer->inputs[i].attributes);
        }
        video_mixer_clear_types(mixer);
        video_mixer_release_device_manager(mixer);
        if (mixer->attributes)
            IMFAttributes_Release(mixer->attributes);
        if (mixer->internal_attributes)
            IMFAttributes_Release(mixer->internal_attributes);
        DeleteCriticalSection(&mixer->cs);
        free(mixer);
    }

    return refcount;
}

static const IUnknownVtbl video_mixer_inner_vtbl =
{
    video_mixer_inner_QueryInterface,
    video_mixer_inner_AddRef,
    video_mixer_inner_Release,
};

static HRESULT WINAPI video_mixer_transform_QueryInterface(IMFTransform *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    return IUnknown_QueryInterface(mixer->outer_unk, riid, obj);
}

static ULONG WINAPI video_mixer_transform_AddRef(IMFTransform *iface)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    return IUnknown_AddRef(mixer->outer_unk);
}

static ULONG WINAPI video_mixer_transform_Release(IMFTransform *iface)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    return IUnknown_Release(mixer->outer_unk);
}

static HRESULT WINAPI video_mixer_transform_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum,
        DWORD *input_maximum, DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("%p, %p, %p, %p, %p.\n", iface, input_minimum, input_maximum, output_minimum, output_maximum);

    *input_minimum = 1;
    *input_maximum = MAX_MIXER_INPUT_STREAMS;
    *output_minimum = 1;
    *output_maximum = 1;

    return S_OK;
}

static HRESULT WINAPI video_mixer_transform_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);

    TRACE("%p, %p, %p.\n", iface, inputs, outputs);

    EnterCriticalSection(&mixer->cs);
    if (inputs) *inputs = mixer->input_count;
    if (outputs) *outputs = 1;
    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static HRESULT WINAPI video_mixer_transform_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p, %u, %p.\n", iface, input_size, inputs, output_size, outputs);

    EnterCriticalSection(&mixer->cs);
    if (mixer->input_count > input_size || !output_size)
        hr = MF_E_BUFFERTOOSMALL;
    else if (inputs)
        memcpy(inputs, mixer->input_ids, mixer->input_count * sizeof(*inputs));
    if (outputs) *outputs = 0;
    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream *input;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, info);

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &input)))
    {
        memset(info, 0, sizeof(*info));
        if (id)
            info->dwFlags |= MFT_INPUT_STREAM_REMOVABLE | MFT_INPUT_STREAM_OPTIONAL;
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    TRACE("%p, %u, %p.\n", iface, id, info);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    memset(info, 0, sizeof(*info));

    return S_OK;
}

static HRESULT WINAPI video_mixer_transform_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);

    TRACE("%p, %p.\n", iface, attributes);

    if (!attributes)
        return E_POINTER;

    *attributes = mixer->attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI video_mixer_transform_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream *input;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, attributes);

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &input)))
    {
        *attributes = input->attributes;
        if (*attributes)
            IMFAttributes_AddRef(*attributes);
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    TRACE("%p, %u, %p.\n", iface, id, attributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream *input;
    unsigned int idx;
    HRESULT hr;

    TRACE("%p, %u.\n", iface, id);

    if (!id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&mixer->cs);

    /* Can't delete reference stream. */
    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &input)))
    {
        mixer->input_count--;
        idx = input - mixer->inputs;
        if (idx < mixer->input_count)
        {
            if (mixer->inputs[idx].attributes)
                IMFAttributes_Release(mixer->inputs[idx].attributes);
            memmove(&mixer->inputs[idx], &mixer->inputs[idx + 1], (mixer->input_count - idx) * sizeof(*mixer->inputs));
            memmove(&mixer->input_ids[idx], &mixer->input_ids[idx + 1], (mixer->input_count - idx) *
                    sizeof(*mixer->input_ids));
        }
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static int video_mixer_add_input_sort_compare(const void *a, const void *b)
{
    const struct input_stream *left = a, *right = b;
    return left->id != right->id ? (left->id < right->id ? -1 : 1) : 0;
};

static HRESULT WINAPI video_mixer_transform_AddInputStreams(IMFTransform *iface, DWORD count, DWORD *ids)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream inputs[MAX_MIXER_INPUT_STREAMS] = { {0} };
    struct input_stream *input;
    unsigned int i, len;
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, count, ids);

    if (!ids)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);
    if (count > ARRAY_SIZE(mixer->inputs) - mixer->input_count)
        hr = E_INVALIDARG;
    else
    {
        /* Test for collisions. */
        memcpy(inputs, mixer->inputs, mixer->input_count * sizeof(*inputs));
        for (i = 0; i < count; ++i)
            inputs[i + mixer->input_count].id = ids[i];

        len = mixer->input_count + count;

        qsort(inputs, len, sizeof(*inputs), video_mixer_add_input_sort_compare);

        for (i = 1; i < len; ++i)
        {
            if (inputs[i - 1].id == inputs[i].id)
            {
                hr = E_INVALIDARG;
                break;
            }
        }

        if (SUCCEEDED(hr))
        {
            unsigned int zorder = mixer->input_count;

            for (i = 0; i < count; ++i)
            {
                if ((input = bsearch(&ids[i], inputs, len, sizeof(*inputs), video_mixer_compare_input_id)))
                    video_mixer_init_input(input);
            }
            memcpy(&mixer->input_ids[mixer->input_count], ids, count * sizeof(*ids));
            memcpy(mixer->inputs, inputs, len * sizeof(*inputs));
            mixer->input_count += count;

            for (i = 0; i < count; ++i)
            {
                if (SUCCEEDED(video_mixer_get_input(mixer, ids[i], &input)))
                    input->zorder = zorder;
                zorder++;
            }
        }
    }
    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    TRACE("%p, %u, %u, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %u, %p.\n", iface, id, index, type);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&mixer->cs);

    if (!mixer->inputs[0].media_type)
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else if (index >= mixer->output.rt_formats_count)
        hr = MF_E_NO_MORE_TYPES;
    else
    {
        *type = mixer->output.rt_formats[index].media_type;
        IMFMediaType_AddRef(*type);
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT video_mixer_init_dxva_videodesc(IMFMediaType *media_type, DXVA2_VideoDesc *video_desc)
{
    const MFVIDEOFORMAT *video_format;
    IMFVideoMediaType *video_type;
    BOOL is_compressed = TRUE;
    HRESULT hr = S_OK;

    if (FAILED(IMFMediaType_QueryInterface(media_type, &IID_IMFVideoMediaType, (void **)&video_type)))
        return MF_E_INVALIDMEDIATYPE;

    video_format = IMFVideoMediaType_GetVideoFormat(video_type);
    IMFVideoMediaType_IsCompressedFormat(video_type, &is_compressed);

    if (!video_format || !video_format->videoInfo.dwWidth || !video_format->videoInfo.dwHeight || is_compressed)
    {
        hr = MF_E_INVALIDMEDIATYPE;
        goto done;
    }

    memset(video_desc, 0, sizeof(*video_desc));
    video_desc->SampleWidth = video_format->videoInfo.dwWidth;
    video_desc->SampleHeight = video_format->videoInfo.dwHeight;
    video_desc->Format = video_format->surfaceInfo.Format;

done:
    IMFVideoMediaType_Release(video_type);

    return hr;
}

static int rt_formats_sort_compare(const void *left, const void *right)
{
    const struct rt_format *format1 = left, *format2 = right;

    if (format1->format < format2->format) return -1;
    if (format1->format > format2->format) return 1;
    return 0;
}

static HRESULT video_mixer_collect_output_types(struct video_mixer *mixer, const DXVA2_VideoDesc *video_desc,
        IDirectXVideoProcessorService *service, unsigned int device_count, const GUID *devices, unsigned int flags)
{
    unsigned int i, j, format_count, count;
    struct rt_format *rt_formats = NULL, *ptr;
    HRESULT hr = MF_E_INVALIDMEDIATYPE;
    D3DFORMAT *formats;
    GUID subtype;

    count = 0;
    for (i = 0; i < device_count; ++i)
    {
        if (SUCCEEDED(IDirectXVideoProcessorService_GetVideoProcessorRenderTargets(service, &devices[i], video_desc,
              &format_count, &formats)))
        {
            if (!(ptr = heap_realloc(rt_formats, (count + format_count) * sizeof(*rt_formats))))
            {
                hr = E_OUTOFMEMORY;
                count = 0;
                CoTaskMemFree(formats);
                break;
            }
            rt_formats = ptr;

            for (j = 0; j < format_count; ++j)
            {
                rt_formats[count + j].format = formats[j];
                rt_formats[count + j].device = devices[i];
            }
            count += format_count;

            CoTaskMemFree(formats);
        }
    }

    if (count && !(flags & MFT_SET_TYPE_TEST_ONLY))
    {
        qsort(rt_formats, count, sizeof(*rt_formats), rt_formats_sort_compare);

        j = 0;
        for (i = j + 1; i < count; ++i)
        {
            if (rt_formats[i].format != rt_formats[j].format)
            {
                rt_formats[++j] = rt_formats[i];
            }
        }
        count = j + 1;

        memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
        if ((mixer->output.rt_formats = heap_calloc(count, sizeof(*mixer->output.rt_formats))))
        {
            for (i = 0; i < count; ++i)
            {
                subtype.Data1 = rt_formats[i].format;
                mixer->output.rt_formats[i] = rt_formats[i];
                MFCreateVideoMediaTypeFromSubtype(&subtype, (IMFVideoMediaType **)&mixer->output.rt_formats[i].media_type);
            }
            mixer->output.rt_formats_count = count;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            count = 0;
        }
    }

    heap_free(rt_formats);

    return count ? S_OK : hr;
}

static HRESULT video_mixer_open_device_handle(struct video_mixer *mixer)
{
    IDirect3DDeviceManager9_CloseDeviceHandle(mixer->device_manager, mixer->device_handle);
    mixer->device_handle = NULL;
    return IDirect3DDeviceManager9_OpenDeviceHandle(mixer->device_manager, &mixer->device_handle);
}

static HRESULT video_mixer_get_processor_service(struct video_mixer *mixer, IDirectXVideoProcessorService **service)
{
    HRESULT hr;

    if (!mixer->device_handle)
    {
        if (FAILED(hr = IDirect3DDeviceManager9_OpenDeviceHandle(mixer->device_manager, &mixer->device_handle)))
            return hr;
    }

    for (;;)
    {
        hr = IDirect3DDeviceManager9_GetVideoService(mixer->device_manager, mixer->device_handle,
                &IID_IDirectXVideoProcessorService, (void **)service);
        if (hr == DXVA2_E_NEW_VIDEO_DEVICE)
        {
            if (SUCCEEDED(hr = video_mixer_open_device_handle(mixer)))
                continue;
        }
        break;
    }

    return hr;
}

static HRESULT WINAPI video_mixer_transform_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *media_type, DWORD flags)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    IDirectXVideoProcessorService *service;
    DXVA2_VideoDesc video_desc;
    HRESULT hr = E_NOTIMPL;
    unsigned int count;
    GUID *guids;

    TRACE("%p, %u, %p, %#x.\n", iface, id, media_type, flags);

    EnterCriticalSection(&mixer->cs);

    if (!(flags & MFT_SET_TYPE_TEST_ONLY))
        video_mixer_clear_types(mixer);

    if (!mixer->device_manager)
        hr = MF_E_NOT_INITIALIZED;
    else
    {
        if (SUCCEEDED(hr = video_mixer_get_processor_service(mixer, &service)))
        {
            if (SUCCEEDED(hr = video_mixer_init_dxva_videodesc(media_type, &video_desc)))
            {
                if (!id)
                {
                    if (SUCCEEDED(hr = IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids(service, &video_desc,
                            &count, &guids)))
                    {
                        if (SUCCEEDED(hr = video_mixer_collect_output_types(mixer, &video_desc, service, count,
                                guids, flags)) && !(flags & MFT_SET_TYPE_TEST_ONLY))
                        {
                            if (mixer->inputs[0].media_type)
                                IMFMediaType_Release(mixer->inputs[0].media_type);
                            mixer->inputs[0].media_type = media_type;
                            IMFMediaType_AddRef(mixer->inputs[0].media_type);
                        }
                        CoTaskMemFree(guids);
                    }
                }
                else
                {
                    FIXME("Unimplemented for substreams.\n");
                    hr = E_NOTIMPL;
                }
            }
            IDirectXVideoProcessorService_Release(service);
        }
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    const unsigned int equality_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES |
            MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA;
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    HRESULT hr = MF_E_INVALIDMEDIATYPE;
    unsigned int i, compare_flags;

    TRACE("%p, %u, %p, %#x.\n", iface, id, type, flags);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&mixer->cs);

    for (i = 0; i < mixer->output.rt_formats_count; ++i)
    {
        compare_flags = 0;
        if (FAILED(IMFMediaType_IsEqual(type, mixer->output.rt_formats[i].media_type, &compare_flags)))
            continue;

        if ((compare_flags & equality_flags) == equality_flags)
        {
            hr = S_OK;
            break;
        }
    }

    if (SUCCEEDED(hr) && !(flags & MFT_SET_TYPE_TEST_ONLY))
    {
        IDirectXVideoProcessorService *service;

        if (SUCCEEDED(hr = video_mixer_get_processor_service(mixer, &service)))
        {
            DXVA2_VideoDesc video_desc;
            GUID subtype = { 0 };
            D3DFORMAT rt_format;

            if (mixer->processor)
                IDirectXVideoProcessor_Release(mixer->processor);
            mixer->processor = NULL;

            video_mixer_init_dxva_videodesc(mixer->inputs[0].media_type, &video_desc);
            IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype);
            rt_format = subtype.Data1;

            if (SUCCEEDED(hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &mixer->output.rt_formats[i].device,
                    &video_desc, rt_format, MAX_MIXER_INPUT_STREAMS, &mixer->processor)))
            {
                if (mixer->output.media_type)
                    IMFMediaType_Release(mixer->output.media_type);
                mixer->output.media_type = type;
                IMFMediaType_AddRef(mixer->output.media_type);
            }

            IDirectXVideoProcessorService_Release(service);
        }
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream *stream;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, type);

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &stream)))
    {
        if (!stream->media_type)
            hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        else
        {
            *type = (IMFMediaType *)stream->media_type;
            IMFMediaType_AddRef(*type);
        }
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, id, type);

    if (id)
        return MF_E_INVALIDSTREAMNUMBER;

    EnterCriticalSection(&mixer->cs);

    if (!mixer->output.media_type)
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else
    {
        *type = (IMFMediaType *)mixer->output.media_type;
        IMFMediaType_AddRef(*type);
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *status)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream *stream;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, status);

    if (!status)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);

    if (!mixer->output.media_type)
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &stream)))
    {
        *status = stream->sample ? 0 : MFT_INPUT_STATUS_ACCEPT_DATA;
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_GetOutputStatus(IMFTransform *iface, DWORD *status)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("%p, %p.\n", iface, status);

    if (!status)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);

    if (!mixer->output.media_type)
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    else
    {
        *status = MFT_OUTPUT_STATUS_SAMPLE_READY;
        for (i = 0; i < mixer->input_count; ++i)
        {
            if (!mixer->inputs[i].sample)
            {
                *status = 0;
                break;
            }
        }
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_transform_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);

    TRACE("%p, %s, %s.\n", iface, wine_dbgstr_longlong(lower), wine_dbgstr_longlong(upper));

    EnterCriticalSection(&mixer->cs);

    mixer->lower_bound = lower;
    mixer->upper_bound = upper;

    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static HRESULT WINAPI video_mixer_transform_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    FIXME("%p, %u, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_transform_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("%p, %u, %#lx.\n", iface, message, param);

    switch (message)
    {
        case MFT_MESSAGE_SET_D3D_MANAGER:

            EnterCriticalSection(&mixer->cs);

            video_mixer_release_device_manager(mixer);
            if (param)
                hr = IUnknown_QueryInterface((IUnknown *)param, &IID_IDirect3DDeviceManager9, (void **)&mixer->device_manager);

            LeaveCriticalSection(&mixer->cs);

            break;

        case MFT_MESSAGE_COMMAND_FLUSH:

            EnterCriticalSection(&mixer->cs);

            for (i = 0; i < mixer->input_count; ++i)
            {
                if (mixer->inputs[i].sample)
                {
                    IMFSample_Release(mixer->inputs[i].sample);
                    mixer->inputs[i].sample = NULL;
                }
            }

            LeaveCriticalSection(&mixer->cs);

            break;

        case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
        case MFT_MESSAGE_NOTIFY_END_STREAMING:

            EnterCriticalSection(&mixer->cs);

            mixer->is_streaming = message == MFT_MESSAGE_NOTIFY_BEGIN_STREAMING;

            LeaveCriticalSection(&mixer->cs);

            break;

        case MFT_MESSAGE_COMMAND_DRAIN:
            break;

        default:
            WARN("Message not handled %d.\n", message);
            hr = E_NOTIMPL;
    }

    return hr;
}

static HRESULT WINAPI video_mixer_transform_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    struct input_stream *input;
    HRESULT hr;

    TRACE("%p, %u, %p, %#x.\n", iface, id, sample, flags);

    if (!sample)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &input)))
    {
        if (!input->media_type || !mixer->output.media_type)
            hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        else if (input->sample)
            hr = MF_E_NOTACCEPTING;
        else
        {
            mixer->is_streaming = 1;
            input->sample = sample;
            IMFSample_AddRef(input->sample);
        }
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT video_mixer_get_sample_surface(IMFSample *sample, IDirect3DSurface9 **surface)
{
    IMFMediaBuffer *buffer;
    IMFGetService *gs;
    HRESULT hr;

    if (FAILED(hr = IMFSample_GetBufferByIndex(sample, 0, &buffer)))
        return hr;

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFGetService, (void **)&gs);
    IMFMediaBuffer_Release(buffer);
    if (FAILED(hr))
        return hr;

    hr = IMFGetService_GetService(gs, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)surface);
    IMFGetService_Release(gs);
    return hr;
}

static HRESULT video_mixer_get_d3d_device(struct video_mixer *mixer, IDirect3DDevice9 **device)
{
    HRESULT hr;

    for (;;)
    {
        hr = IDirect3DDeviceManager9_LockDevice(mixer->device_manager, mixer->device_handle,
                device, TRUE);
        if (hr == DXVA2_E_NEW_VIDEO_DEVICE)
        {
            if (SUCCEEDED(hr = video_mixer_open_device_handle(mixer)))
                continue;
        }
        break;
    }

    return hr;
}

static HRESULT WINAPI video_mixer_transform_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct video_mixer *mixer = impl_from_IMFTransform(iface);
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    HRESULT hr;

    TRACE("%p, %#x, %u, %p, %p.\n", iface, flags, count, buffers, status);

    if (!buffers || !count || count > 1 || !buffers->pSample)
        return E_INVALIDARG;

    if (buffers->dwStreamID)
        return MF_E_INVALIDSTREAMNUMBER;

    *status = 0;

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_sample_surface(buffers->pSample, &surface)))
    {
        if (mixer->is_streaming)
        {
            FIXME("Streaming state is not handled.\n");
            hr = E_NOTIMPL;
        }
        else
        {
            if (SUCCEEDED(hr = video_mixer_get_d3d_device(mixer, &device)))
            {
                IDirect3DDevice9_ColorFill(device, surface, NULL, 0);
                IDirect3DDeviceManager9_UnlockDevice(mixer->device_manager, mixer->device_handle, FALSE);
                IDirect3DDevice9_Release(device);
            }
        }
        IDirect3DSurface9_Release(surface);
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static const IMFTransformVtbl video_mixer_transform_vtbl =
{
    video_mixer_transform_QueryInterface,
    video_mixer_transform_AddRef,
    video_mixer_transform_Release,
    video_mixer_transform_GetStreamLimits,
    video_mixer_transform_GetStreamCount,
    video_mixer_transform_GetStreamIDs,
    video_mixer_transform_GetInputStreamInfo,
    video_mixer_transform_GetOutputStreamInfo,
    video_mixer_transform_GetAttributes,
    video_mixer_transform_GetInputStreamAttributes,
    video_mixer_transform_GetOutputStreamAttributes,
    video_mixer_transform_DeleteInputStream,
    video_mixer_transform_AddInputStreams,
    video_mixer_transform_GetInputAvailableType,
    video_mixer_transform_GetOutputAvailableType,
    video_mixer_transform_SetInputType,
    video_mixer_transform_SetOutputType,
    video_mixer_transform_GetInputCurrentType,
    video_mixer_transform_GetOutputCurrentType,
    video_mixer_transform_GetInputStatus,
    video_mixer_transform_GetOutputStatus,
    video_mixer_transform_SetOutputBounds,
    video_mixer_transform_ProcessEvent,
    video_mixer_transform_ProcessMessage,
    video_mixer_transform_ProcessInput,
    video_mixer_transform_ProcessOutput,
};

static HRESULT WINAPI video_mixer_device_id_QueryInterface(IMFVideoDeviceID *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFVideoDeviceID(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_device_id_AddRef(IMFVideoDeviceID *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoDeviceID(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_device_id_Release(IMFVideoDeviceID *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoDeviceID(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_device_id_GetDeviceID(IMFVideoDeviceID *iface, IID *device_id)
{
    TRACE("%p, %p.\n", iface, device_id);

    if (!device_id)
        return E_POINTER;

    memcpy(device_id, &IID_IDirect3DDevice9, sizeof(*device_id));

    return S_OK;
}

static const IMFVideoDeviceIDVtbl video_mixer_device_id_vtbl =
{
    video_mixer_device_id_QueryInterface,
    video_mixer_device_id_AddRef,
    video_mixer_device_id_Release,
    video_mixer_device_id_GetDeviceID,
};

static HRESULT WINAPI video_mixer_service_client_QueryInterface(IMFTopologyServiceLookupClient *iface,
        REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFTopologyServiceLookupClient(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_service_client_AddRef(IMFTopologyServiceLookupClient *iface)
{
    struct video_mixer *mixer = impl_from_IMFTopologyServiceLookupClient(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_service_client_Release(IMFTopologyServiceLookupClient *iface)
{
    struct video_mixer *mixer = impl_from_IMFTopologyServiceLookupClient(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_service_client_InitServicePointers(IMFTopologyServiceLookupClient *iface,
        IMFTopologyServiceLookup *service_lookup)
{
    struct video_mixer *mixer = impl_from_IMFTopologyServiceLookupClient(iface);
    unsigned int count;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, service_lookup);

    if (!service_lookup)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);

    count = 1;
    if (FAILED(hr = IMFTopologyServiceLookup_LookupService(service_lookup, MF_SERVICE_LOOKUP_GLOBAL, 0,
            &MR_VIDEO_RENDER_SERVICE, &IID_IMediaEventSink, (void **)&mixer->event_sink, &count)))
    {
        WARN("Failed to get renderer event sink, hr %#x.\n", hr);
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_service_client_ReleaseServicePointers(IMFTopologyServiceLookupClient *iface)
{
    struct video_mixer *mixer = impl_from_IMFTopologyServiceLookupClient(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&mixer->cs);

    if (mixer->event_sink)
        IMediaEventSink_Release(mixer->event_sink);
    mixer->event_sink = NULL;

    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static const IMFTopologyServiceLookupClientVtbl video_mixer_service_client_vtbl =
{
    video_mixer_service_client_QueryInterface,
    video_mixer_service_client_AddRef,
    video_mixer_service_client_Release,
    video_mixer_service_client_InitServicePointers,
    video_mixer_service_client_ReleaseServicePointers,
};

static HRESULT WINAPI video_mixer_control_QueryInterface(IMFVideoMixerControl2 *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_control_AddRef(IMFVideoMixerControl2 *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_control_Release(IMFVideoMixerControl2 *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_control_SetStreamZOrder(IMFVideoMixerControl2 *iface, DWORD id, DWORD zorder)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    struct input_stream *stream;
    HRESULT hr;

    TRACE("%p, %u, %u.\n", iface, id, zorder);

    /* Can't change reference stream. */
    if (!id && zorder)
        return E_INVALIDARG;

    EnterCriticalSection(&mixer->cs);

    if (zorder >= mixer->input_count)
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &stream)))
    {
        /* Lowest zorder only applies to reference stream. */
        if (id && !zorder)
            hr = MF_E_INVALIDREQUEST;
        else
            stream->zorder = zorder;
    }

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_control_GetStreamZOrder(IMFVideoMixerControl2 *iface, DWORD id, DWORD *zorder)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    struct input_stream *stream;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, zorder);

    if (!zorder)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &stream)))
        *zorder = stream->zorder;

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_control_SetStreamOutputRect(IMFVideoMixerControl2 *iface, DWORD id,
        const MFVideoNormalizedRect *rect)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    struct input_stream *stream;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, rect);

    if (!rect)
        return E_POINTER;

    if (rect->left > rect->right || rect->top > rect->bottom ||
            rect->left < 0.0f || rect->top < 0.0f || rect->right > 1.0f || rect->bottom > 1.0f)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &stream)))
        stream->rect = *rect;

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_control_GetStreamOutputRect(IMFVideoMixerControl2 *iface, DWORD id,
        MFVideoNormalizedRect *rect)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);
    struct input_stream *stream;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, id, rect);

    if (!rect)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);

    if (SUCCEEDED(hr = video_mixer_get_input(mixer, id, &stream)))
        *rect = stream->rect;

    LeaveCriticalSection(&mixer->cs);

    return hr;
}

static HRESULT WINAPI video_mixer_control_SetMixingPrefs(IMFVideoMixerControl2 *iface, DWORD flags)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);

    TRACE("%p, %#x.\n", iface, flags);

    EnterCriticalSection(&mixer->cs);
    mixer->mixing_flags = flags;
    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static HRESULT WINAPI video_mixer_control_GetMixingPrefs(IMFVideoMixerControl2 *iface, DWORD *flags)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerControl2(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (!flags)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);
    *flags = mixer->mixing_flags;
    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static const IMFVideoMixerControl2Vtbl video_mixer_control_vtbl =
{
    video_mixer_control_QueryInterface,
    video_mixer_control_AddRef,
    video_mixer_control_Release,
    video_mixer_control_SetStreamZOrder,
    video_mixer_control_GetStreamZOrder,
    video_mixer_control_SetStreamOutputRect,
    video_mixer_control_GetStreamOutputRect,
    video_mixer_control_SetMixingPrefs,
    video_mixer_control_GetMixingPrefs,
};

static HRESULT WINAPI video_mixer_getservice_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFGetService(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_getservice_AddRef(IMFGetService *iface)
{
    struct video_mixer *mixer = impl_from_IMFGetService(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_getservice_Release(IMFGetService *iface)
{
    struct video_mixer *mixer = impl_from_IMFGetService(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_getservice_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MR_VIDEO_MIXER_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFVideoMixerBitmap) ||
                IsEqualIID(riid, &IID_IMFVideoProcessor) ||
                IsEqualIID(riid, &IID_IMFVideoPositionMapper) ||
                IsEqualIID(riid, &IID_IMFVideoMixerControl) ||
                IsEqualIID(riid, &IID_IMFVideoMixerControl2))
        {
            return IMFGetService_QueryInterface(iface, riid, obj);
        }
    }

    FIXME("Unsupported service %s, riid %s.\n", debugstr_guid(service), debugstr_guid(riid));

    return E_NOTIMPL;
}

static const IMFGetServiceVtbl video_mixer_getservice_vtbl =
{
    video_mixer_getservice_QueryInterface,
    video_mixer_getservice_AddRef,
    video_mixer_getservice_Release,
    video_mixer_getservice_GetService,
};

static HRESULT WINAPI video_mixer_bitmap_QueryInterface(IMFVideoMixerBitmap *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerBitmap(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_bitmap_AddRef(IMFVideoMixerBitmap *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerBitmap(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_bitmap_Release(IMFVideoMixerBitmap *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoMixerBitmap(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_bitmap_SetAlphaBitmap(IMFVideoMixerBitmap *iface, const MFVideoAlphaBitmap *bitmap)
{
    FIXME("%p, %p.\n", iface, bitmap);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_bitmap_ClearAlphaBitmap(IMFVideoMixerBitmap *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_bitmap_UpdateAlphaBitmapParameters(IMFVideoMixerBitmap *iface,
        const MFVideoAlphaBitmapParams *params)
{
    FIXME("%p, %p.\n", iface, params);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_bitmap_GetAlphaBitmapParameters(IMFVideoMixerBitmap *iface, MFVideoAlphaBitmapParams *params)
{
    FIXME("%p, %p.\n", iface, params);

    return E_NOTIMPL;
}

static const IMFVideoMixerBitmapVtbl video_mixer_bitmap_vtbl =
{
    video_mixer_bitmap_QueryInterface,
    video_mixer_bitmap_AddRef,
    video_mixer_bitmap_Release,
    video_mixer_bitmap_SetAlphaBitmap,
    video_mixer_bitmap_ClearAlphaBitmap,
    video_mixer_bitmap_UpdateAlphaBitmapParameters,
    video_mixer_bitmap_GetAlphaBitmapParameters,
};

static HRESULT WINAPI video_mixer_position_mapper_QueryInterface(IMFVideoPositionMapper *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFVideoPositionMapper(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_position_mapper_AddRef(IMFVideoPositionMapper *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoPositionMapper(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_position_mapper_Release(IMFVideoPositionMapper *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoPositionMapper(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_position_mapper_MapOutputCoordinateToInputStream(IMFVideoPositionMapper *iface,
        float x_out, float y_out, DWORD output_stream, DWORD input_stream, float *x_in, float *y_in)
{
    FIXME("%p, %f, %f, %u, %u, %p, %p.\n", iface, x_out, y_out, output_stream, input_stream, x_in, y_in);

    return E_NOTIMPL;
}

static const IMFVideoPositionMapperVtbl video_mixer_position_mapper_vtbl =
{
    video_mixer_position_mapper_QueryInterface,
    video_mixer_position_mapper_AddRef,
    video_mixer_position_mapper_Release,
    video_mixer_position_mapper_MapOutputCoordinateToInputStream,
};

static HRESULT WINAPI video_mixer_processor_QueryInterface(IMFVideoProcessor *iface, REFIID riid, void **obj)
{
    struct video_mixer *mixer = impl_from_IMFVideoProcessor(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, obj);
}

static ULONG WINAPI video_mixer_processor_AddRef(IMFVideoProcessor *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoProcessor(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_processor_Release(IMFVideoProcessor *iface)
{
    struct video_mixer *mixer = impl_from_IMFVideoProcessor(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_processor_GetAvailableVideoProcessorModes(IMFVideoProcessor *iface, UINT *count,
        GUID **modes)
{
    FIXME("%p, %p, %p.\n", iface, count, modes);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetVideoProcessorCaps(IMFVideoProcessor *iface, GUID *mode,
        DXVA2_VideoProcessorCaps *caps)
{
    FIXME("%p, %s, %p.\n", iface, debugstr_guid(mode), caps);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetVideoProcessorMode(IMFVideoProcessor *iface, GUID *mode)
{
    FIXME("%p, %p.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_SetVideoProcessorMode(IMFVideoProcessor *iface, GUID *mode)
{
    FIXME("%p, %s.\n", iface, debugstr_guid(mode));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetProcAmpRange(IMFVideoProcessor *iface, DWORD prop, DXVA2_ValueRange *range)
{
    FIXME("%p, %#x, %p.\n", iface, prop, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetProcAmpValues(IMFVideoProcessor *iface, DWORD flags, DXVA2_ProcAmpValues *values)
{
    FIXME("%p, %#x, %p.\n", iface, flags, values);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_SetProcAmpValues(IMFVideoProcessor *iface, DWORD flags, DXVA2_ProcAmpValues *values)
{
    FIXME("%p, %#x, %p.\n", iface, flags, values);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetFilteringRange(IMFVideoProcessor *iface, DWORD prop, DXVA2_ValueRange *range)
{
    FIXME("%p, %#x, %p.\n", iface, prop, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetFilteringValue(IMFVideoProcessor *iface, DWORD prop, DXVA2_Fixed32 *value)
{
    FIXME("%p, %#x, %p.\n", iface, prop, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_SetFilteringValue(IMFVideoProcessor *iface, DWORD prop, DXVA2_Fixed32 *value)
{
    FIXME("%p, %#x, %p.\n", iface, prop, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_processor_GetBackgroundColor(IMFVideoProcessor *iface, COLORREF *color)
{
    struct video_mixer *mixer = impl_from_IMFVideoProcessor(iface);

    TRACE("%p, %p.\n", iface, color);

    if (!color)
        return E_POINTER;

    EnterCriticalSection(&mixer->cs);
    *color = mixer->bkgnd_color;
    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static HRESULT WINAPI video_mixer_processor_SetBackgroundColor(IMFVideoProcessor *iface, COLORREF color)
{
    struct video_mixer *mixer = impl_from_IMFVideoProcessor(iface);

    TRACE("%p, %#x.\n", iface, color);

    EnterCriticalSection(&mixer->cs);
    mixer->bkgnd_color = color;
    LeaveCriticalSection(&mixer->cs);

    return S_OK;
}

static const IMFVideoProcessorVtbl video_mixer_processor_vtbl =
{
    video_mixer_processor_QueryInterface,
    video_mixer_processor_AddRef,
    video_mixer_processor_Release,
    video_mixer_processor_GetAvailableVideoProcessorModes,
    video_mixer_processor_GetVideoProcessorCaps,
    video_mixer_processor_GetVideoProcessorMode,
    video_mixer_processor_SetVideoProcessorMode,
    video_mixer_processor_GetProcAmpRange,
    video_mixer_processor_GetProcAmpValues,
    video_mixer_processor_SetProcAmpValues,
    video_mixer_processor_GetFilteringRange,
    video_mixer_processor_GetFilteringValue,
    video_mixer_processor_SetFilteringValue,
    video_mixer_processor_GetBackgroundColor,
    video_mixer_processor_SetBackgroundColor,
};

static HRESULT WINAPI video_mixer_attributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **out)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, out);
}

static ULONG WINAPI video_mixer_attributes_AddRef(IMFAttributes *iface)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_attributes_Release(IMFAttributes *iface)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_attributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(mixer->internal_attributes, key, type);
}

static HRESULT WINAPI video_mixer_attributes_CompareItem(IMFAttributes *iface, REFGUID key,
        REFPROPVARIANT value, BOOL *result)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(mixer->internal_attributes, key, value, result);
}

static HRESULT WINAPI video_mixer_attributes_Compare(IMFAttributes *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE match_type, BOOL *ret)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, match_type, ret);

    return IMFAttributes_Compare(mixer->internal_attributes, theirs, match_type, ret);
}

static HRESULT WINAPI video_mixer_attributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(mixer->internal_attributes, key, length);
}

static HRESULT WINAPI video_mixer_attributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(mixer->internal_attributes, key, value, size, length);
}

static HRESULT WINAPI video_mixer_attributes_GetAllocatedString(IMFAttributes *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(mixer->internal_attributes, key, value, length);
}

static HRESULT WINAPI video_mixer_attributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(mixer->internal_attributes, key, size);
}

static HRESULT WINAPI video_mixer_attributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(mixer->internal_attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI video_mixer_attributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(mixer->internal_attributes, key, buf, size);
}

static HRESULT WINAPI video_mixer_attributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **out)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), out);

    return IMFAttributes_GetUnknown(mixer->internal_attributes, key, riid, out);
}

static HRESULT WINAPI video_mixer_attributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(mixer->internal_attributes, key);
}

static HRESULT WINAPI video_mixer_attributes_DeleteAllItems(IMFAttributes *iface)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(mixer->internal_attributes);
}

static HRESULT WINAPI video_mixer_attributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(mixer->internal_attributes, key, value);
}

static HRESULT WINAPI video_mixer_attributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(mixer->internal_attributes, key, buf, size);
}

static HRESULT WINAPI video_mixer_attributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(mixer->internal_attributes, key, unknown);
}

static HRESULT WINAPI video_mixer_attributes_LockStore(IMFAttributes *iface)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(mixer->internal_attributes);
}

static HRESULT WINAPI video_mixer_attributes_UnlockStore(IMFAttributes *iface)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(mixer->internal_attributes);
}

static HRESULT WINAPI video_mixer_attributes_GetCount(IMFAttributes *iface, UINT32 *count)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(mixer->internal_attributes, count);
}

static HRESULT WINAPI video_mixer_attributes_GetItemByIndex(IMFAttributes *iface, UINT32 index,
        GUID *key, PROPVARIANT *value)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(mixer->internal_attributes, index, key, value);
}

static HRESULT WINAPI video_mixer_attributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    struct video_mixer *mixer = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(mixer->internal_attributes, dest);
}

static const IMFAttributesVtbl video_mixer_attributes_vtbl =
{
    video_mixer_attributes_QueryInterface,
    video_mixer_attributes_AddRef,
    video_mixer_attributes_Release,
    video_mixer_attributes_GetItem,
    video_mixer_attributes_GetItemType,
    video_mixer_attributes_CompareItem,
    video_mixer_attributes_Compare,
    video_mixer_attributes_GetUINT32,
    video_mixer_attributes_GetUINT64,
    video_mixer_attributes_GetDouble,
    video_mixer_attributes_GetGUID,
    video_mixer_attributes_GetStringLength,
    video_mixer_attributes_GetString,
    video_mixer_attributes_GetAllocatedString,
    video_mixer_attributes_GetBlobSize,
    video_mixer_attributes_GetBlob,
    video_mixer_attributes_GetAllocatedBlob,
    video_mixer_attributes_GetUnknown,
    video_mixer_attributes_SetItem,
    video_mixer_attributes_DeleteItem,
    video_mixer_attributes_DeleteAllItems,
    video_mixer_attributes_SetUINT32,
    video_mixer_attributes_SetUINT64,
    video_mixer_attributes_SetDouble,
    video_mixer_attributes_SetGUID,
    video_mixer_attributes_SetString,
    video_mixer_attributes_SetBlob,
    video_mixer_attributes_SetUnknown,
    video_mixer_attributes_LockStore,
    video_mixer_attributes_UnlockStore,
    video_mixer_attributes_GetCount,
    video_mixer_attributes_GetItemByIndex,
    video_mixer_attributes_CopyAllItems
};

static HRESULT WINAPI video_mixer_quality_advise_QueryInterface(IMFQualityAdvise *iface, REFIID riid, void **out)
{
    struct video_mixer *mixer = impl_from_IMFQualityAdvise(iface);
    return IMFTransform_QueryInterface(&mixer->IMFTransform_iface, riid, out);
}

static ULONG WINAPI video_mixer_quality_advise_AddRef(IMFQualityAdvise *iface)
{
    struct video_mixer *mixer = impl_from_IMFQualityAdvise(iface);
    return IMFTransform_AddRef(&mixer->IMFTransform_iface);
}

static ULONG WINAPI video_mixer_quality_Release(IMFQualityAdvise *iface)
{
    struct video_mixer *mixer = impl_from_IMFQualityAdvise(iface);
    return IMFTransform_Release(&mixer->IMFTransform_iface);
}

static HRESULT WINAPI video_mixer_quality_advise_SetDropMode(IMFQualityAdvise *iface, MF_QUALITY_DROP_MODE mode)
{
    FIXME("%p, %u.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_quality_advise_SetQualityLevel(IMFQualityAdvise *iface, MF_QUALITY_LEVEL level)
{
    FIXME("%p, %u.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_quality_advise_GetDropMode(IMFQualityAdvise *iface, MF_QUALITY_DROP_MODE *mode)
{
    FIXME("%p, %p.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_quality_advise_GetQualityLevel(IMFQualityAdvise *iface, MF_QUALITY_LEVEL *level)
{
    FIXME("%p, %p.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_mixer_quality_advise_DropTime(IMFQualityAdvise *iface, LONGLONG interval)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(interval));

    return E_NOTIMPL;
}

static const IMFQualityAdviseVtbl video_mixer_quality_advise_vtbl =
{
    video_mixer_quality_advise_QueryInterface,
    video_mixer_quality_advise_AddRef,
    video_mixer_quality_Release,
    video_mixer_quality_advise_SetDropMode,
    video_mixer_quality_advise_SetQualityLevel,
    video_mixer_quality_advise_GetDropMode,
    video_mixer_quality_advise_GetQualityLevel,
    video_mixer_quality_advise_DropTime,
};

HRESULT WINAPI MFCreateVideoMixer(IUnknown *owner, REFIID riid_device, REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", owner, debugstr_guid(riid_device), debugstr_guid(riid), obj);

    *obj = NULL;

    if (!IsEqualIID(riid_device, &IID_IDirect3DDevice9))
        return E_INVALIDARG;

    return CoCreateInstance(&CLSID_MFVideoMixer9, owner, CLSCTX_INPROC_SERVER, riid, obj);
}

HRESULT evr_mixer_create(IUnknown *outer, void **out)
{
    struct video_mixer *object;
    MFVideoNormalizedRect rect;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTransform_iface.lpVtbl = &video_mixer_transform_vtbl;
    object->IMFVideoDeviceID_iface.lpVtbl = &video_mixer_device_id_vtbl;
    object->IMFTopologyServiceLookupClient_iface.lpVtbl = &video_mixer_service_client_vtbl;
    object->IMFVideoMixerControl2_iface.lpVtbl = &video_mixer_control_vtbl;
    object->IMFGetService_iface.lpVtbl = &video_mixer_getservice_vtbl;
    object->IMFVideoMixerBitmap_iface.lpVtbl = &video_mixer_bitmap_vtbl;
    object->IMFVideoPositionMapper_iface.lpVtbl = &video_mixer_position_mapper_vtbl;
    object->IMFVideoProcessor_iface.lpVtbl = &video_mixer_processor_vtbl;
    object->IMFAttributes_iface.lpVtbl = &video_mixer_attributes_vtbl;
    object->IMFQualityAdvise_iface.lpVtbl = &video_mixer_quality_advise_vtbl;
    object->IUnknown_inner.lpVtbl = &video_mixer_inner_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;
    object->input_count = 1;
    object->lower_bound = MFT_OUTPUT_BOUND_LOWER_UNBOUNDED;
    object->upper_bound = MFT_OUTPUT_BOUND_UPPER_UNBOUNDED;
    video_mixer_init_input(&object->inputs[0]);
    InitializeCriticalSection(&object->cs);
    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
    {
        IUnknown_Release(&object->IUnknown_inner);
        return hr;
    }
    if (FAILED(hr = MFCreateAttributes(&object->internal_attributes, 0)))
    {
        IUnknown_Release(&object->IUnknown_inner);
        return hr;
    }

    /* Default attributes configuration. */

    rect.left = rect.top = 0.0f;
    rect.right = rect.bottom = 1.0f;
    IMFAttributes_SetBlob(object->attributes, &VIDEO_ZOOM_RECT, (const UINT8 *)&rect, sizeof(rect));

    IMFAttributes_SetUINT32(object->internal_attributes, &MF_SA_D3D_AWARE, 1);

    *out = &object->IUnknown_inner;

    return S_OK;
}
