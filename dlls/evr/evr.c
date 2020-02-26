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
#include "wine/strmbase.h"

#include "initguid.h"
#include "dxva2api.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

typedef struct
{
    struct strmbase_renderer renderer;
} evr_filter;

static inline evr_filter *impl_from_strmbase_renderer(struct strmbase_renderer *iface)
{
    return CONTAINING_RECORD(iface, evr_filter, renderer);
}

static void evr_destroy(struct strmbase_renderer *iface)
{
    evr_filter *filter = impl_from_strmbase_renderer(iface);

    strmbase_renderer_cleanup(&filter->renderer);
    CoTaskMemFree(filter);
}

static HRESULT WINAPI evr_DoRenderSample(struct strmbase_renderer *iface, IMediaSample *sample)
{
    FIXME("Not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI evr_CheckMediaType(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    FIXME("Not implemented.\n");
    return E_NOTIMPL;
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .pfnCheckMediaType = evr_CheckMediaType,
    .pfnDoRenderSample = evr_DoRenderSample,
    .renderer_destroy = evr_destroy,
};

HRESULT evr_filter_create(IUnknown *outer, void **out)
{
    evr_filter *object;

    *out = NULL;

    object = CoTaskMemAlloc(sizeof(evr_filter));
    if (!object)
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer,
            &CLSID_EnhancedVideoRenderer, L"EVR Input0", &renderer_ops);

    *out = &object->renderer.filter.IUnknown_inner;

    return S_OK;
}
