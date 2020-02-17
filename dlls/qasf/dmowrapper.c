/*
 * DMO wrapper filter
 *
 * Copyright (C) 2019 Zebediah Figura
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

#include "qasf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(qasf);

struct dmo_wrapper
{
    struct strmbase_filter filter;
    IDMOWrapperFilter IDMOWrapperFilter_iface;
};

static inline struct dmo_wrapper *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, filter);
}

static inline struct dmo_wrapper *impl_from_IDMOWrapperFilter(IDMOWrapperFilter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, IDMOWrapperFilter_iface);
}

static HRESULT WINAPI dmo_wrapper_filter_QueryInterface(IDMOWrapperFilter *iface, REFIID iid, void **out)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI dmo_wrapper_filter_AddRef(IDMOWrapperFilter *iface)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI dmo_wrapper_filter_Release(IDMOWrapperFilter *iface)
{
    struct dmo_wrapper *filter = impl_from_IDMOWrapperFilter(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI dmo_wrapper_filter_Init(IDMOWrapperFilter *iface, REFCLSID clsid, REFCLSID category)
{
    FIXME("iface %p, clsid %s, category %s, stub!\n",
            iface, wine_dbgstr_guid(clsid), wine_dbgstr_guid(category));
    return E_NOTIMPL;
}

static const IDMOWrapperFilterVtbl dmo_wrapper_filter_vtbl =
{
    dmo_wrapper_filter_QueryInterface,
    dmo_wrapper_filter_AddRef,
    dmo_wrapper_filter_Release,
    dmo_wrapper_filter_Init,
};

static struct strmbase_pin *dmo_wrapper_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void dmo_wrapper_destroy(struct strmbase_filter *iface)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT dmo_wrapper_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct dmo_wrapper *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IDMOWrapperFilter))
    {
        *out = &filter->IDMOWrapperFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = dmo_wrapper_get_pin,
    .filter_destroy = dmo_wrapper_destroy,
    .filter_query_interface = dmo_wrapper_query_interface,
};

HRESULT dmo_wrapper_create(IUnknown *outer, IUnknown **out)
{
    struct dmo_wrapper *object;

    if (!(object = calloc(sizeof(*object), 1)))
        return E_OUTOFMEMORY;

    /* Always pass NULL as the outer object; see test_aggregation(). */
    strmbase_filter_init(&object->filter, NULL, &CLSID_DMOWrapperFilter, &filter_ops);

    object->IDMOWrapperFilter_iface.lpVtbl = &dmo_wrapper_filter_vtbl;

    TRACE("Created DMO wrapper %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
