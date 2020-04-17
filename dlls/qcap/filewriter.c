/*
 * File writer filter
 *
 * Copyright (C) 2020 Zebediah Figura
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

#include "dshow.h"
#include "qcap_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

struct file_writer
{
    struct strmbase_filter filter;
};

static inline struct file_writer *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct file_writer, filter);
}

static struct strmbase_pin *file_writer_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void file_writer_destroy(struct strmbase_filter *iface)
{
    struct file_writer *filter = impl_from_strmbase_filter(iface);

    strmbase_filter_cleanup(&filter->filter);
    heap_free(filter);
}

static struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = file_writer_get_pin,
    .filter_destroy = file_writer_destroy,
};

HRESULT file_writer_create(IUnknown *outer, IUnknown **out)
{
    struct file_writer *object;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_FileWriter, &filter_ops);

    TRACE("Created file writer %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
