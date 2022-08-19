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

#include "initguid.h"
#include "dxva2api.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

struct evr
{
    struct strmbase_renderer renderer;
    IEVRFilterConfig IEVRFilterConfig_iface;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    IMFGetService IMFGetService_iface;
    IMFVideoRenderer IMFVideoRenderer_iface;

    IMFTransform *mixer;
    IMFVideoPresenter *presenter;
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
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static void evr_destroy(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    evr_uninitialize(filter);
    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
}

static HRESULT evr_render(struct strmbase_renderer *iface, IMediaSample *sample)
{
    FIXME("Not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT evr_query_accept(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    FIXME("Not implemented.\n");
    return E_NOTIMPL;
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .renderer_query_accept = evr_query_accept,
    .renderer_render = evr_render,
    .renderer_query_interface = evr_query_interface,
    .renderer_destroy = evr_destroy,
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
    IMFGetService *gs;

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

    TRACE("Created EVR %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
