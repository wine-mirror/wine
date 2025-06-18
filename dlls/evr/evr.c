/*
 * Copyright 2017 Fabian Maurer
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

#include "wine/debug.h"

#include <stdio.h>

#include "evr_private.h"
#include "d3d9.h"
#include "mferror.h"
#include "mfapi.h"

#include "initguid.h"
#include "dxva2api.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

enum evr_flags
{
    EVR_INIT_SERVICES = 0x1, /* Currently in InitServices() call. */
    EVR_MIXER_INITED_SERVICES = 0x2,
    EVR_PRESENTER_INITED_SERVICES = 0x4,
};

struct evr
{
    struct strmbase_renderer renderer;
    IEVRFilterConfig IEVRFilterConfig_iface;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    IMFGetService IMFGetService_iface;
    IMFVideoRenderer IMFVideoRenderer_iface;
    IMediaEventSink IMediaEventSink_iface;
    IMFTopologyServiceLookup IMFTopologyServiceLookup_iface;

    IMFTransform *mixer;
    IMFVideoPresenter *presenter;
    IMFVideoSampleAllocator *allocator;
    IMFMediaType *media_type;
    unsigned int flags;
};

static void evr_uninitialize(struct evr *filter)
{
    if (filter->mixer)
    {
        IMFTransform_Release(filter->mixer);
        filter->mixer = NULL;
    }

    if (filter->presenter)
    {
        IMFVideoPresenter_Release(filter->presenter);
        filter->presenter = NULL;
    }
}

static HRESULT evr_initialize(struct evr *filter, IMFTransform *mixer, IMFVideoPresenter *presenter)
{
    HRESULT hr = S_OK;

    if (mixer)
    {
        IMFTransform_AddRef(mixer);
    }
    else if (FAILED(hr = CoCreateInstance(&CLSID_MFVideoMixer9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFTransform, (void **)&mixer)))
    {
        WARN("Failed to create default mixer instance, hr %#lx.\n", hr);
        return hr;
    }

    if (presenter)
    {
        IMFVideoPresenter_AddRef(presenter);
    }
    else if (FAILED(hr = CoCreateInstance(&CLSID_MFVideoPresenter9, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMFVideoPresenter, (void **)&presenter)))
    {
        WARN("Failed to create default presenter instance, hr %#lx.\n", hr);
        IMFTransform_Release(mixer);
        return hr;
    }

    evr_uninitialize(filter);

    filter->mixer = mixer;
    filter->presenter = presenter;

    /* FIXME: configure mixer and presenter */

    return hr;
}

static struct evr *impl_from_strmbase_renderer(struct strmbase_renderer *iface)
{
    return CONTAINING_RECORD(iface, struct evr, renderer);
}

static HRESULT evr_query_interface(struct strmbase_renderer *iface, REFIID iid, void **out)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    if (IsEqualGUID(iid, &IID_IEVRFilterConfig))
        *out = &filter->IEVRFilterConfig_iface;
    else if (IsEqualGUID(iid, &IID_IAMFilterMiscFlags))
        *out = &filter->IAMFilterMiscFlags_iface;
    else if (IsEqualGUID(iid, &IID_IMFGetService))
        *out = &filter->IMFGetService_iface;
    else if (IsEqualGUID(iid, &IID_IMFVideoRenderer))
        *out = &filter->IMFVideoRenderer_iface;
    else if (IsEqualGUID(iid, &IID_IMediaEventSink))
        *out = &filter->IMediaEventSink_iface;
    else if (IsEqualGUID(iid, &IID_IMFTopologyServiceLookup))
        *out = &filter->IMFTopologyServiceLookup_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static BOOL evr_is_mixer_d3d_aware(const struct evr *filter)
{
    IMFAttributes *attributes;
    unsigned int value = 0;
    BOOL ret;

    if (FAILED(IMFTransform_QueryInterface(filter->mixer, &IID_IMFAttributes, (void **)&attributes)))
        return FALSE;

    ret = SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &value)) && value;
    IMFAttributes_Release(attributes);
    return ret;
}

static HRESULT evr_get_service(void *unk, REFGUID service, REFIID riid, void **obj)
{
    IMFGetService *gs;
    HRESULT hr;

    if (SUCCEEDED(hr = IUnknown_QueryInterface((IUnknown *)unk, &IID_IMFGetService, (void **)&gs)))
    {
        hr = IMFGetService_GetService(gs, service, riid, obj);
        IMFGetService_Release(gs);
    }

    return hr;
}

static HRESULT evr_init_services(struct evr *filter)
{
    IMFTopologyServiceLookupClient *lookup_client;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFTransform_QueryInterface(filter->mixer, &IID_IMFTopologyServiceLookupClient,
            (void **)&lookup_client)))
    {
        filter->flags |= EVR_INIT_SERVICES;
        if (SUCCEEDED(hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client,
                &filter->IMFTopologyServiceLookup_iface)))
        {
            filter->flags |= EVR_MIXER_INITED_SERVICES;
        }
        filter->flags &= ~EVR_INIT_SERVICES;
        IMFTopologyServiceLookupClient_Release(lookup_client);
    }

    if (FAILED(hr)) return hr;

    /* Set device manager that presenter should have created. */
    if (evr_is_mixer_d3d_aware(filter))
    {
        IUnknown *device_manager;
        IMFGetService *gs;

        if (SUCCEEDED(IUnknown_QueryInterface(filter->presenter, &IID_IMFGetService, (void **)&gs)))
        {
            if (SUCCEEDED(IMFGetService_GetService(gs, &MR_VIDEO_RENDER_SERVICE, &IID_IDirect3DDeviceManager9,
                    (void **)&device_manager)))
            {
                IMFTransform_ProcessMessage(filter->mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)device_manager);
                IUnknown_Release(device_manager);
            }

            IMFGetService_Release(gs);
        }
    }

    if (SUCCEEDED(hr = IMFVideoPresenter_QueryInterface(filter->presenter, &IID_IMFTopologyServiceLookupClient,
            (void **)&lookup_client)))
    {
        filter->flags |= EVR_INIT_SERVICES;
        if (SUCCEEDED(hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client,
                &filter->IMFTopologyServiceLookup_iface)))
        {
            filter->flags |= EVR_PRESENTER_INITED_SERVICES;
        }
        filter->flags &= ~EVR_INIT_SERVICES;
        IMFTopologyServiceLookupClient_Release(lookup_client);
    }

    return hr;
}

static void evr_release_services(struct evr *filter)
{
    IMFTopologyServiceLookupClient *lookup_client;

    if (filter->flags & EVR_MIXER_INITED_SERVICES && SUCCEEDED(IMFTransform_QueryInterface(filter->mixer,
            &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client)))
    {
        IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
        IMFTopologyServiceLookupClient_Release(lookup_client);
        filter->flags &= ~EVR_MIXER_INITED_SERVICES;
    }

    if (filter->flags & EVR_PRESENTER_INITED_SERVICES && SUCCEEDED(IMFVideoPresenter_QueryInterface(filter->presenter,
            &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client)))
    {
        IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
        IMFTopologyServiceLookupClient_Release(lookup_client);
        filter->flags &= ~EVR_PRESENTER_INITED_SERVICES;
    }
}

static void evr_set_input_type(struct evr *filter, IMFMediaType *media_type)
{
    if (filter->media_type)
        IMFMediaType_Release(filter->media_type);
    if ((filter->media_type = media_type))
        IMFMediaType_AddRef(filter->media_type);
    if (!media_type && filter->allocator)
        IMFVideoSampleAllocator_UninitializeSampleAllocator(filter->allocator);
}

static HRESULT evr_test_input_type(struct evr *filter, const AM_MEDIA_TYPE *mt, IMFMediaType **ret)
{
    IMFMediaType *media_type;
    HRESULT hr = S_OK;

    if (!filter->presenter)
        hr = evr_initialize(filter, NULL, NULL);

    if (SUCCEEDED(hr))
        hr = evr_init_services(filter);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&media_type);

    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = MFInitMediaTypeFromAMMediaType(media_type, mt)))
        {
            /* TODO: some pin -> mixer input mapping is necessary to test the substreams. */
            if (SUCCEEDED(hr = IMFTransform_SetInputType(filter->mixer, 0, media_type, MFT_SET_TYPE_TEST_ONLY)))
            {
                if (ret)
                    IMFMediaType_AddRef((*ret = media_type));
            }
        }

        IMFMediaType_Release(media_type);
    }

    return hr;
}

static HRESULT evr_connect(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);
    IMFVideoSampleAllocator *allocator;
    IMFMediaType *media_type;
    IUnknown *device_manager;
    HRESULT hr;

    if (SUCCEEDED(hr = evr_test_input_type(filter, mt, &media_type)))
    {
        if (SUCCEEDED(hr = IMFTransform_SetInputType(filter->mixer, 0, media_type, 0)))
            hr = IMFVideoPresenter_ProcessMessage(filter->presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);

        if (SUCCEEDED(hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocator, (void **)&allocator)))
        {
            if (SUCCEEDED(hr = evr_get_service(filter->presenter, &MR_VIDEO_RENDER_SERVICE,
                    &IID_IDirect3DDeviceManager9, (void **)&device_manager)))
            {
                if (SUCCEEDED(hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, device_manager)))
                {
                    if (SUCCEEDED(hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, media_type)))
                    {
                        IMFVideoSampleAllocator_AddRef((filter->allocator = allocator));
                    }
                }
                IUnknown_Release(device_manager);
            }

            IMFVideoSampleAllocator_Release(allocator);
        }

        if (SUCCEEDED(hr))
            evr_set_input_type(filter, media_type);

        IMFMediaType_Release(media_type);
    }

    return hr;
}

static void evr_disconnect(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    if (filter->mixer)
        IMFTransform_SetInputType(filter->mixer, 0, NULL, 0);
    evr_set_input_type(filter, NULL);
    evr_release_services(filter);
}

static void evr_destroy(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    evr_uninitialize(filter);
    evr_set_input_type(filter, NULL);
    if (filter->allocator)
        IMFVideoSampleAllocator_Release(filter->allocator);
    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
}

static HRESULT evr_copy_sample_buffer(struct evr *filter, const GUID *subtype, IMediaSample *input_sample, IMFSample **sample)
{
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    IMFMediaBuffer *buffer;
    UINT64 frame_size = 0;
    UINT32 width, lines;
    LONG src_stride;
    HRESULT hr;
    BYTE *src;
    BYTE *dst_uv;

    if (FAILED(hr = IMFMediaType_GetUINT32(filter->media_type, &MF_MT_DEFAULT_STRIDE, (UINT32 *)&src_stride)))
    {
        WARN("Unknown input buffer stride.\n");
        return hr;
    }
    IMFMediaType_GetUINT64(filter->media_type, &MF_MT_FRAME_SIZE, &frame_size);
    width = frame_size >> 32;
    lines = frame_size;


    if (FAILED(hr = IMediaSample_GetPointer(input_sample, &src)))
    {
        WARN("Failed to get pointer to sample data, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IMFVideoSampleAllocator_AllocateSample(filter->allocator, sample)))
    {
        WARN("Failed to allocate a sample, hr %#lx.\n", hr);
        return hr;
    }

    if (SUCCEEDED(hr = IMFSample_GetBufferByIndex(*sample, 0, &buffer)))
    {
        if (SUCCEEDED(hr = evr_get_service(buffer, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface)))
        {
            if (SUCCEEDED(hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, D3DLOCK_DISCARD)))
            {
                if (src_stride < 0) src += (-src_stride) * (lines - 1);

                if (IsEqualGUID(subtype, &MFVideoFormat_YUY2))
                {
                    width = (width + 1) & ~1;
                    MFCopyImage(locked_rect.pBits, locked_rect.Pitch, src, src_stride, width * 2, lines);
                }
                else if (IsEqualGUID(subtype, &MFVideoFormat_NV12))
                {
                    /* Width and height must be rounded up to even. */
                    width = (width + 1) & ~1;
                    lines = (lines + 1) & ~1;

                    /* Y plane */
                    MFCopyImage(locked_rect.pBits, locked_rect.Pitch, src, src_stride, width, lines);

                    /* UV plane */
                    dst_uv = (BYTE *)locked_rect.pBits + (lines * locked_rect.Pitch);
                    MFCopyImage(dst_uv, locked_rect.Pitch, src + (lines * src_stride), src_stride, width, lines / 2);
                }
                else
                {
                    /* All other formats are 32-bit, single plane. */
                    MFCopyImage(locked_rect.pBits, locked_rect.Pitch, src, src_stride, width * 4, lines);
                }
                IDirect3DSurface9_UnlockRect(surface);
            }

            IDirect3DSurface9_Release(surface);
        }
        IMFMediaBuffer_Release(buffer);
    }

    if (FAILED(hr))
    {
        IMFSample_Release(*sample);
        *sample = NULL;
    }

    return hr;
}

static HRESULT evr_render(struct strmbase_renderer *iface, IMediaSample *input_sample)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);
    HRESULT hr = E_NOTIMPL;
    GUID subtype = { 0 };
    IMFSample *sample;

    if (!filter->media_type)
    {
        WARN("Media type wasn't set.\n");
        return E_UNEXPECTED;
    }

    IMFMediaType_GetGUID(filter->media_type, &MF_MT_SUBTYPE, &subtype);

    if (IsEqualGUID(&subtype, &MFVideoFormat_ARGB32)
            || IsEqualGUID(&subtype, &MFVideoFormat_RGB32)
            || IsEqualGUID(&subtype, &MFVideoFormat_YUY2)
            || IsEqualGUID(&subtype, &MFVideoFormat_NV12))
    {
        if (SUCCEEDED(hr = evr_copy_sample_buffer(filter, &subtype, input_sample, &sample)))
        {
            if (SUCCEEDED(IMFTransform_ProcessInput(filter->mixer, 0, sample, 0)))
                IMFVideoPresenter_ProcessMessage(filter->presenter, MFVP_MESSAGE_PROCESSINPUTNOTIFY, 0);

            IMFSample_Release(sample);
        }
    }
    else
    {
        FIXME("Unhandled input type %s.\n", debugstr_guid(&subtype));
    }

    return hr;
}

static HRESULT evr_query_accept(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);
    HRESULT hr;

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    hr = evr_test_input_type(filter, mt, NULL);
    evr_release_services(filter);

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);

    return hr;
}

/* FIXME: errors should be propagated from init/start/stop handlers. */
static void evr_init_stream(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    if (!filter->mixer) return;

    if (SUCCEEDED(IMFTransform_ProcessMessage(filter->mixer, MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0)))
        IMFVideoPresenter_ProcessMessage(filter->presenter, MFVP_MESSAGE_BEGINSTREAMING, 0);
}

static void evr_start_stream(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    if (filter->mixer)
        IMFTransform_ProcessMessage(filter->mixer, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
}

static void evr_stop_stream(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    if (!filter->mixer) return;

    if (SUCCEEDED(IMFTransform_ProcessMessage(filter->mixer, MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0)))
    {
        if (SUCCEEDED(IMFVideoPresenter_ProcessMessage(filter->presenter, MFVP_MESSAGE_ENDSTREAMING, 0)))
            IMFTransform_ProcessMessage(filter->mixer, MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
    }
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .renderer_query_accept = evr_query_accept,
    .renderer_render = evr_render,
    .renderer_query_interface = evr_query_interface,
    .renderer_connect = evr_connect,
    .renderer_disconnect = evr_disconnect,
    .renderer_destroy = evr_destroy,
    .renderer_init_stream = evr_init_stream,
    .renderer_start_stream = evr_start_stream,
    .renderer_stop_stream = evr_stop_stream,
};

static struct evr *impl_from_IEVRFilterConfig(IEVRFilterConfig *iface)
{
    return CONTAINING_RECORD(iface, struct evr, IEVRFilterConfig_iface);
}

static HRESULT WINAPI filter_config_QueryInterface(IEVRFilterConfig *iface, REFIID iid, void **out)
{
    struct evr *filter = impl_from_IEVRFilterConfig(iface);
    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI filter_config_AddRef(IEVRFilterConfig *iface)
{
    struct evr *filter = impl_from_IEVRFilterConfig(iface);
    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_config_Release(IEVRFilterConfig *iface)
{
    struct evr *filter = impl_from_IEVRFilterConfig(iface);
    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI filter_config_SetNumberOfStreams(IEVRFilterConfig *iface, DWORD count)
{
    struct evr *filter = impl_from_IEVRFilterConfig(iface);

    FIXME("filter %p, count %lu, stub!\n", filter, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_config_GetNumberOfStreams(IEVRFilterConfig *iface, DWORD *count)
{
    struct evr *filter = impl_from_IEVRFilterConfig(iface);

    FIXME("filter %p, count %p, stub!\n", filter, count);

    return E_NOTIMPL;
}

static const IEVRFilterConfigVtbl filter_config_vtbl =
{
    filter_config_QueryInterface,
    filter_config_AddRef,
    filter_config_Release,
    filter_config_SetNumberOfStreams,
    filter_config_GetNumberOfStreams,
};

static struct evr *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, struct evr, IAMFilterMiscFlags_iface);
}

static HRESULT WINAPI filter_misc_flags_QueryInterface(IAMFilterMiscFlags *iface, REFIID iid, void **out)
{
    struct evr *filter = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI filter_misc_flags_AddRef(IAMFilterMiscFlags *iface)
{
    struct evr *filter = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_misc_flags_Release(IAMFilterMiscFlags *iface)
{
    struct evr *filter = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_misc_flags_GetMiscFlags(IAMFilterMiscFlags *iface)
{
    TRACE("%p.\n", iface);

    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl filter_misc_flags_vtbl =
{
    filter_misc_flags_QueryInterface,
    filter_misc_flags_AddRef,
    filter_misc_flags_Release,
    filter_misc_flags_GetMiscFlags,
};

static struct evr *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct evr, IMFGetService_iface);
}

static HRESULT WINAPI filter_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct evr *filter = impl_from_IMFGetService(iface);
    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, riid, obj);
}

static ULONG WINAPI filter_get_service_AddRef(IMFGetService *iface)
{
    struct evr *filter = impl_from_IMFGetService(iface);
    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_get_service_Release(IMFGetService *iface)
{
    struct evr *filter = impl_from_IMFGetService(iface);
    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI filter_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct evr *filter = impl_from_IMFGetService(iface);
    HRESULT hr = E_NOINTERFACE;
    IMFGetService *gs = NULL;

    TRACE("iface %p, service %s, riid %s, obj %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    if (IsEqualGUID(service, &MR_VIDEO_RENDER_SERVICE))
    {
        if (!filter->presenter)
            hr = evr_initialize(filter, NULL, NULL);

        if (filter->presenter)
            hr = IMFVideoPresenter_QueryInterface(filter->presenter, &IID_IMFGetService, (void **)&gs);
    }
    else
    {
        FIXME("Unsupported service %s.\n", debugstr_guid(service));
    }

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);

    if (gs)
    {
        hr = IMFGetService_GetService(gs, service, riid, obj);
        IMFGetService_Release(gs);
    }

    return hr;
}

static const IMFGetServiceVtbl filter_get_service_vtbl =
{
    filter_get_service_QueryInterface,
    filter_get_service_AddRef,
    filter_get_service_Release,
    filter_get_service_GetService,
};

static struct evr *impl_from_IMFVideoRenderer(IMFVideoRenderer *iface)
{
    return CONTAINING_RECORD(iface, struct evr, IMFVideoRenderer_iface);
}

static HRESULT WINAPI filter_video_renderer_QueryInterface(IMFVideoRenderer *iface, REFIID riid, void **obj)
{
    struct evr *filter = impl_from_IMFVideoRenderer(iface);
    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, riid, obj);
}

static ULONG WINAPI filter_video_renderer_AddRef(IMFVideoRenderer *iface)
{
    struct evr *filter = impl_from_IMFVideoRenderer(iface);
    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_video_renderer_Release(IMFVideoRenderer *iface)
{
    struct evr *filter = impl_from_IMFVideoRenderer(iface);
    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI filter_video_renderer_InitializeRenderer(IMFVideoRenderer *iface, IMFTransform *mixer,
        IMFVideoPresenter *presenter)
{
    struct evr *filter = impl_from_IMFVideoRenderer(iface);
    HRESULT hr;

    TRACE("iface %p, mixer %p, presenter %p.\n", iface, mixer, presenter);

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    hr = evr_initialize(filter, mixer, presenter);

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);

    return hr;
}

static const IMFVideoRendererVtbl filter_video_renderer_vtbl =
{
    filter_video_renderer_QueryInterface,
    filter_video_renderer_AddRef,
    filter_video_renderer_Release,
    filter_video_renderer_InitializeRenderer,
};

static struct evr *impl_from_IMediaEventSink(IMediaEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct evr, IMediaEventSink_iface);
}

static HRESULT WINAPI filter_media_event_sink_QueryInterface(IMediaEventSink *iface, REFIID riid, void **obj)
{
    struct evr *filter = impl_from_IMediaEventSink(iface);
    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, riid, obj);
}

static ULONG WINAPI filter_media_event_sink_AddRef(IMediaEventSink *iface)
{
    struct evr *filter = impl_from_IMediaEventSink(iface);
    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_media_event_sink_Release(IMediaEventSink *iface)
{
    struct evr *filter = impl_from_IMediaEventSink(iface);
    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI filter_media_event_sink_Notify(IMediaEventSink *iface, LONG event, LONG_PTR param1, LONG_PTR param2)
{
    FIXME("iface %p, event %ld, param1 %Id, param2 %Id.\n", iface, event, param1, param2);

    return E_NOTIMPL;
}

static const IMediaEventSinkVtbl filter_media_event_sink_vtbl =
{
    filter_media_event_sink_QueryInterface,
    filter_media_event_sink_AddRef,
    filter_media_event_sink_Release,
    filter_media_event_sink_Notify,
};

static struct evr *impl_from_IMFTopologyServiceLookup(IMFTopologyServiceLookup *iface)
{
    return CONTAINING_RECORD(iface, struct evr, IMFTopologyServiceLookup_iface);
}

static HRESULT WINAPI filter_service_lookup_QueryInterface(IMFTopologyServiceLookup *iface, REFIID riid, void **obj)
{
    struct evr *filter = impl_from_IMFTopologyServiceLookup(iface);
    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, riid, obj);
}

static ULONG WINAPI filter_service_lookup_AddRef(IMFTopologyServiceLookup *iface)
{
    struct evr *filter = impl_from_IMFTopologyServiceLookup(iface);
    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_service_lookup_Release(IMFTopologyServiceLookup *iface)
{
    struct evr *filter = impl_from_IMFTopologyServiceLookup(iface);
    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI filter_service_lookup_LookupService(IMFTopologyServiceLookup *iface, MF_SERVICE_LOOKUP_TYPE lookup_type,
        DWORD index, REFGUID service, REFIID riid, void **objects, DWORD *num_objects)
{
    struct evr *filter = impl_from_IMFTopologyServiceLookup(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, lookup_type %d, index %lu, service %s, riid %s, objects %p, num_objects %p.\n",
            iface, lookup_type, index, debugstr_guid(service), debugstr_guid(riid), objects, num_objects);

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    if (!(filter->flags & EVR_INIT_SERVICES))
        hr = MF_E_NOTACCEPTING;
    else if (IsEqualGUID(service, &MR_VIDEO_RENDER_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMediaEventSink))
        {
            *objects = &filter->IMediaEventSink_iface;
            IUnknown_AddRef((IUnknown *)*objects);
        }
    }
    else if (IsEqualGUID(service, &MR_VIDEO_MIXER_SERVICE))
    {
        hr = IMFTransform_QueryInterface(filter->mixer, riid, objects);
    }
    else
    {
        WARN("Unsupported service %s.\n", debugstr_guid(service));
        hr = MF_E_UNSUPPORTED_SERVICE;
    }

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);

    return hr;
}

static const IMFTopologyServiceLookupVtbl filter_service_lookup_vtbl =
{
    filter_service_lookup_QueryInterface,
    filter_service_lookup_AddRef,
    filter_service_lookup_Release,
    filter_service_lookup_LookupService,
};

HRESULT evr_filter_create(IUnknown *outer, void **out)
{
    struct evr *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer,
            &CLSID_EnhancedVideoRenderer, L"EVR Input0", &renderer_ops);
    object->IEVRFilterConfig_iface.lpVtbl = &filter_config_vtbl;
    object->IAMFilterMiscFlags_iface.lpVtbl = &filter_misc_flags_vtbl;
    object->IMFGetService_iface.lpVtbl = &filter_get_service_vtbl;
    object->IMFVideoRenderer_iface.lpVtbl = &filter_video_renderer_vtbl;
    object->IMediaEventSink_iface.lpVtbl = &filter_media_event_sink_vtbl;
    object->IMFTopologyServiceLookup_iface.lpVtbl = &filter_service_lookup_vtbl;

    TRACE("Created EVR %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
