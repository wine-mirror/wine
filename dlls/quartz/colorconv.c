/*
 * Color converter
 *
 * Copyright 2026 Brendan McGrath
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

#include "quartz_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct color_converter
{
    struct strmbase_filter filter;
};

static struct color_converter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct color_converter, filter);
}

static struct strmbase_pin *color_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void color_destroy(struct strmbase_filter *iface)
{
    struct color_converter *filter = impl_from_strmbase_filter(iface);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT color_init_stream(struct strmbase_filter *iface)
{
    return S_OK;
}

static HRESULT color_cleanup_stream(struct strmbase_filter *iface)
{
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = color_get_pin,
    .filter_destroy = color_destroy,
    .filter_init_stream = color_init_stream,
    .filter_cleanup_stream = color_cleanup_stream,
};

HRESULT color_create(IUnknown *outer, IUnknown **out)
{
    struct color_converter *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_Colour, &filter_ops);

    TRACE("Created Color Converter %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
