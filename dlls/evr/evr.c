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
};

static struct evr *impl_from_strmbase_renderer(struct strmbase_renderer *iface)
{
    return CONTAINING_RECORD(iface, struct evr, renderer);
}

static void evr_destroy(struct strmbase_renderer *iface)
{
    struct evr *filter = impl_from_strmbase_renderer(iface);

    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
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
    struct evr *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer,
            &CLSID_EnhancedVideoRenderer, L"EVR Input0", &renderer_ops);

    TRACE("Created EVR %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
