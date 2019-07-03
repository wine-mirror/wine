/*
 * Null renderer filter
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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
#include "dshow.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

typedef struct NullRendererImpl
{
    BaseRenderer renderer;
} NullRendererImpl;

static inline NullRendererImpl *impl_from_BaseRenderer(BaseRenderer *iface)
{
    return CONTAINING_RECORD(iface, NullRendererImpl, renderer);
}

static HRESULT WINAPI NullRenderer_DoRenderSample(BaseRenderer *iface, IMediaSample *pMediaSample)
{
    return S_OK;
}

static HRESULT WINAPI NullRenderer_CheckMediaType(BaseRenderer *iface, const AM_MEDIA_TYPE * pmt)
{
    TRACE("Not a stub!\n");
    return S_OK;
}

static void null_renderer_destroy(BaseRenderer *iface)
{
    NullRendererImpl *filter = impl_from_BaseRenderer(iface);

    strmbase_renderer_cleanup(&filter->renderer);
    CoTaskMemFree(filter);
}

static const BaseRendererFuncTable RendererFuncTable =
{
    .pfnCheckMediaType = NullRenderer_CheckMediaType,
    .pfnDoRenderSample = NullRenderer_DoRenderSample,
    .renderer_destroy = null_renderer_destroy,
};

static const IBaseFilterVtbl NullRenderer_Vtbl =
{
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    BaseRendererImpl_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

HRESULT NullRenderer_create(IUnknown *outer, void **out)
{
    static const WCHAR sink_name[] = {'I','n',0};

    HRESULT hr;
    NullRendererImpl *pNullRenderer;

    *out = NULL;

    pNullRenderer = CoTaskMemAlloc(sizeof(NullRendererImpl));

    hr = strmbase_renderer_init(&pNullRenderer->renderer, &NullRenderer_Vtbl, outer,
            &CLSID_NullRenderer, sink_name,
            (DWORD_PTR)(__FILE__ ": NullRendererImpl.csFilter"), &RendererFuncTable);

    if (FAILED(hr))
        CoTaskMemFree(pNullRenderer);
    else
        *out = &pNullRenderer->renderer.filter.IUnknown_inner;

    return S_OK;
}
