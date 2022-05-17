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

    AM_MEDIA_TYPE media_type;
    WCHAR *file_name;
};

static inline struct asf_reader *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct asf_reader, filter);
}

static struct strmbase_pin *asf_reader_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void asf_reader_destroy(struct strmbase_filter *iface)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);

    free(filter->file_name);
    FreeMediaType(&filter->media_type);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT asf_reader_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IFileSourceFilter))
    {
        *out = &filter->IFileSourceFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = asf_reader_get_pin,
    .filter_destroy = asf_reader_destroy,
    .filter_query_interface = asf_reader_query_interface,
};

static inline struct asf_reader *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, struct asf_reader, IFileSourceFilter_iface);
}

static HRESULT WINAPI file_source_QueryInterface(IFileSourceFilter *iface, REFIID iid, void **out)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_QueryInterface(&filter->filter.IBaseFilter_iface, iid, out);
}

static ULONG WINAPI file_source_AddRef(IFileSourceFilter *iface)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_AddRef(&filter->filter.IBaseFilter_iface);
}

static ULONG WINAPI file_source_Release(IFileSourceFilter *iface)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_Release(&filter->filter.IBaseFilter_iface);
}

static HRESULT WINAPI file_source_Load(IFileSourceFilter *iface, LPCOLESTR file_name, const AM_MEDIA_TYPE *media_type)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    TRACE("filter %p, file_name %s, media_type %p.\n", filter, debugstr_w(file_name), media_type);
    strmbase_dump_media_type(media_type);

    if (!file_name)
        return E_POINTER;

    if (filter->file_name)
        return E_FAIL;

    if (!(filter->file_name = wcsdup(file_name)))
        return E_OUTOFMEMORY;

    if (media_type)
        CopyMediaType(&filter->media_type, media_type);

    return S_OK;
}

static HRESULT WINAPI file_source_GetCurFile(IFileSourceFilter *iface, LPOLESTR *file_name, AM_MEDIA_TYPE *media_type)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    TRACE("filter %p, file_name %p, media_type %p.\n", filter, file_name, media_type);

    if (!file_name)
        return E_POINTER;
    *file_name = NULL;

    if (media_type)
    {
        media_type->majortype = filter->media_type.majortype;
        media_type->subtype = filter->media_type.subtype;
        media_type->lSampleSize = filter->media_type.lSampleSize;
        media_type->pUnk = filter->media_type.pUnk;
        media_type->cbFormat = filter->media_type.cbFormat;
    }

    if (filter->file_name)
    {
        *file_name = CoTaskMemAlloc((wcslen(filter->file_name) + 1) * sizeof(WCHAR));
        wcscpy(*file_name, filter->file_name);
    }

    return S_OK;
}

static const IFileSourceFilterVtbl file_source_vtbl =
{
    file_source_QueryInterface,
    file_source_AddRef,
    file_source_Release,
    file_source_Load,
    file_source_GetCurFile,
};

HRESULT asf_reader_create(IUnknown *outer, IUnknown **out)
{
    struct asf_reader *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_WMAsfReader, &filter_ops);
    object->IFileSourceFilter_iface.lpVtbl = &file_source_vtbl;

    TRACE("Created WM ASF reader %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
