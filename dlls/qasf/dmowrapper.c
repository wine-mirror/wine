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
};

static inline struct dmo_wrapper *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_wrapper, filter);
}

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

static struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = dmo_wrapper_get_pin,
    .filter_destroy = dmo_wrapper_destroy,
};

HRESULT dmo_wrapper_create(IUnknown *outer, IUnknown **out)
{
    struct dmo_wrapper *object;

    if (!(object = calloc(sizeof(*object), 1)))
        return E_OUTOFMEMORY;

    /* Always pass NULL as the outer object; see test_aggregation(). */
    strmbase_filter_init(&object->filter, NULL, &CLSID_DMOWrapperFilter, &filter_ops);

    TRACE("Created DMO wrapper %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
