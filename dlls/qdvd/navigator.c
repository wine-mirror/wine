/*
 * Navigator filter
 *
 * Copyright 2020 Gijs Vermeulen
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

#include "qdvd_private.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(qdvd);

struct navigator
{
    struct strmbase_filter filter;
};

static inline struct navigator *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, struct navigator, filter);
}

static struct strmbase_pin *navigator_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void navigator_destroy(struct strmbase_filter *iface)
{
    struct navigator *filter = impl_from_strmbase_filter(iface);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = navigator_get_pin,
    .filter_destroy = navigator_destroy,
};

HRESULT navigator_create(IUnknown *outer, IUnknown **out)
{
    struct navigator *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_DVDNavigator, &filter_ops);

    TRACE("Created DVD Navigator filter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
