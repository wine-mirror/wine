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
    BaseFilter filter;
} evr_filter;

static const IBaseFilterVtbl basefilter_vtbl =
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

static inline evr_filter *impl_from_BaseFilter(BaseFilter *iface)
{
    return CONTAINING_RECORD(iface, evr_filter, filter);
}

static IPin *evr_get_pin(BaseFilter *iface, unsigned int index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);
    return NULL;
}

static void evr_destroy(BaseFilter *iface)
{
    evr_filter *filter = impl_from_BaseFilter(iface);

    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static const BaseFilterFuncTable basefilter_functable =
{
    .filter_get_pin = evr_get_pin,
    .filter_destroy = evr_destroy,
};

HRESULT evr_filter_create(IUnknown *outer, void **out)
{
    evr_filter *object;

    *out = NULL;

    object = CoTaskMemAlloc(sizeof(evr_filter));
    if (!object)
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, &basefilter_vtbl, outer, &CLSID_EnhancedVideoRenderer,
                    (DWORD_PTR)(__FILE__ ": EVR.csFilter"), &basefilter_functable);

    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
