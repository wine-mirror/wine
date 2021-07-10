/*
 * WM ASF reader
 *
 * Copyright 2020 Jactry Zeng for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct asf_reader
{
    struct strmbase_filter filter;
    IFileSourceFilter IFileSourceFilter_iface;

    AM_MEDIA_TYPE type;
    WCHAR *filename;
};

static inline struct asf_reader *impl_reader_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct asf_reader, filter);
}

static struct strmbase_pin *asf_reader_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void asf_reader_destroy(struct strmbase_filter *iface)
{
    struct asf_reader *filter = impl_reader_from_strmbase_filter(iface);

    free(filter->filename);
    FreeMediaType(&filter->type);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT asf_reader_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct asf_reader *filter = impl_reader_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IFileSourceFilter))
    {
        *out = &filter->IFileSourceFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = asf_reader_get_pin,
    .filter_destroy = asf_reader_destroy,
    .filter_query_interface = asf_reader_query_interface,
};

static inline struct asf_reader *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, struct asf_reader, IFileSourceFilter_iface);
}

static HRESULT WINAPI filesourcefilter_QueryInterface(IFileSourceFilter *iface, REFIID iid, void **out)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_QueryInterface(&filter->filter.IBaseFilter_iface, iid, out);
}

static ULONG WINAPI filesourcefilter_AddRef(IFileSourceFilter *iface)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_AddRef(&filter->filter.IBaseFilter_iface);
}

static ULONG WINAPI filesourcefilter_Release(IFileSourceFilter *iface)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_Release(&filter->filter.IBaseFilter_iface);
}

static HRESULT WINAPI filesourcefilter_Load(IFileSourceFilter *iface, LPCOLESTR filename, const AM_MEDIA_TYPE *type)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    TRACE("filter %p, filename %s, type %p.\n", filter, debugstr_w(filename), type);
    strmbase_dump_media_type(type);

    if (!filename)
        return E_POINTER;

    if (filter->filename)
        return E_FAIL;

    if (!(filter->filename = wcsdup(filename)))
        return E_OUTOFMEMORY;

    if (type)
        CopyMediaType(&filter->type, type);

    return S_OK;
}

static HRESULT WINAPI filesourcefilter_GetCurFile(IFileSourceFilter *iface, LPOLESTR *filename, AM_MEDIA_TYPE *type)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    TRACE("filter %p, filename %p, type %p.\n", filter, filename, type);

    if (!filename)
        return E_POINTER;
    *filename = NULL;

    if (type)
    {
        type->majortype = filter->type.majortype;
        type->subtype = filter->type.subtype;
        type->lSampleSize = filter->type.lSampleSize;
        type->pUnk = filter->type.pUnk;
        type->cbFormat = filter->type.cbFormat;
    }

    if (filter->filename)
    {
        *filename = CoTaskMemAlloc((wcslen(filter->filename) + 1) * sizeof(WCHAR));
        wcscpy(*filename, filter->filename);
    }

    return S_OK;
}

static const IFileSourceFilterVtbl filesourcefilter_vtbl =
{
    filesourcefilter_QueryInterface,
    filesourcefilter_AddRef,
    filesourcefilter_Release,
    filesourcefilter_Load,
    filesourcefilter_GetCurFile,
};

HRESULT asf_reader_create(IUnknown *outer, IUnknown **out)
{
    struct asf_reader *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_WMAsfReader, &filter_ops);
    object->IFileSourceFilter_iface.lpVtbl = &filesourcefilter_vtbl;

    TRACE("Created WM ASF reader %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
