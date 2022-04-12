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
};

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
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static void evr_destroy(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

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

HRESULT evr_filter_create(IUnknown *outer, void **out)
{
    struct evr *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer,
            &CLSID_EnhancedVideoRenderer, L"EVR Input0", &renderer_ops);
    object->IEVRFilterConfig_iface.lpVtbl = &filter_config_vtbl;
    object->IAMFilterMiscFlags_iface.lpVtbl = &filter_misc_flags_vtbl;

    TRACE("Created EVR %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
