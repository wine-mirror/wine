/*
 * Copyright 2016 Andrew Eikum for CodeWeavers
 * Copyright 2017 Dmitry Timoshkov
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

#include <stdarg.h>
#include <wchar.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propvarutil.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

static const WCHAR *map_shortname_to_schema(const GUID *format, const WCHAR *name);

enum metadata_object_type
{
    BLOCK_READER,
    BLOCK_WRITER,
    READER,
    WRITER,
};

struct query_handler
{
    IWICMetadataQueryWriter IWICMetadataQueryWriter_iface;
    LONG ref;
    union
    {
        IUnknown *handler;
        IWICMetadataBlockReader *block_reader;
        IWICMetadataBlockWriter *block_writer;
        IWICMetadataReader *reader;
        IWICMetadataWriter *writer;
    } object;
    enum metadata_object_type object_type;
    WCHAR *root;
};

static bool is_writer_handler(const struct query_handler *handler)
{
    return handler->object_type == BLOCK_WRITER
            || handler->object_type == WRITER;
}

static bool is_block_handler(const struct query_handler *handler)
{
    return handler->object_type == BLOCK_READER
            || handler->object_type == BLOCK_WRITER;
}

static inline struct query_handler *impl_from_IWICMetadataQueryWriter(IWICMetadataQueryWriter *iface)
{
    return CONTAINING_RECORD(iface, struct query_handler, IWICMetadataQueryWriter_iface);
}

static HRESULT WINAPI query_handler_QueryInterface(IWICMetadataQueryWriter *iface, REFIID riid,
        void **ppvObject)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IWICMetadataQueryReader) ||
            (is_writer_handler(handler) && IsEqualGUID(riid, &IID_IWICMetadataQueryWriter)))
        *ppvObject = &handler->IWICMetadataQueryWriter_iface;
    else
        *ppvObject = NULL;

    if (*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI query_handler_AddRef(IWICMetadataQueryWriter *iface)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);
    ULONG ref = InterlockedIncrement(&handler->ref);
    TRACE("(%p) refcount=%lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI query_handler_Release(IWICMetadataQueryWriter *iface)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);
    ULONG ref = InterlockedDecrement(&handler->ref);
    TRACE("(%p) refcount=%lu\n", iface, ref);
    if (!ref)
    {
        IUnknown_Release(handler->object.handler);
        free(handler->root);
        free(handler);
    }
    return ref;
}

static HRESULT WINAPI query_handler_GetContainerFormat(IWICMetadataQueryWriter *iface, GUID *format)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);

    TRACE("(%p,%p)\n", iface, format);

    return is_block_handler(handler) ? IWICMetadataBlockReader_GetContainerFormat(handler->object.block_reader, format):
            IWICMetadataReader_GetMetadataFormat(handler->object.reader, format);
}

static HRESULT WINAPI query_handler_GetLocation(IWICMetadataQueryWriter *iface, UINT len, WCHAR *location, UINT *ret_len)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);
    UINT actual_len;

    TRACE("(%p,%u,%p,%p)\n", iface, len, location, ret_len);

    if (!ret_len) return E_INVALIDARG;

    actual_len = lstrlenW(handler->root) + 1;

    if (location)
    {
        if (len < actual_len)
            return WINCODEC_ERR_INSUFFICIENTBUFFER;

        memcpy(location, handler->root, actual_len * sizeof(WCHAR));
    }

    *ret_len = actual_len;

    return S_OK;
}

struct string_t
{
    const WCHAR *str;
    int len;
};

static const struct
{
    int len;
    WCHAR str[10];
    VARTYPE vt;
} str2vt[] =
{
    { 4, L"char", VT_I1 },
    { 5, L"uchar", VT_UI1 },
    { 5, L"short", VT_I2 },
    { 6, L"ushort", VT_UI2 },
    { 4, L"long", VT_I4 },
    { 5, L"ulong", VT_UI4 },
    { 3, L"int", VT_I4 },
    { 4, L"uint", VT_UI4 },
    { 8, L"longlong", VT_I8 },
    { 9, L"ulonglong", VT_UI8 },
    { 5, L"float", VT_R4 },
    { 6, L"double", VT_R8 },
    { 3, L"str", VT_LPSTR },
    { 4, L"wstr", VT_LPWSTR },
    { 4, L"guid", VT_CLSID },
    { 4, L"bool", VT_BOOL }
};

static VARTYPE map_type(struct string_t *str)
{
    UINT i;

    for (i = 0; i < ARRAY_SIZE(str2vt); i++)
    {
        if (str2vt[i].len == str->len)
        {
            if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                str->str, str->len, str2vt[i].str, str2vt[i].len) == CSTR_EQUAL)
                return str2vt[i].vt;
        }
    }

    WARN("type %s is not recognized\n", wine_dbgstr_wn(str->str, str->len));

    return VT_ILLEGAL;
}

static const WCHAR *get_type_name(VARTYPE vt)
{
    UINT i;

    for (i = 0; i < ARRAY_SIZE(str2vt); i++)
    {
        if (str2vt[i].vt == vt)
            return str2vt[i].str;
    }

    return NULL;
}

struct query_component
{
    unsigned int index;
    PROPVARIANT schema;
    PROPVARIANT id;
    union
    {
        IUnknown *handler;
        IWICMetadataReader *reader;
        IWICMetadataWriter *writer;
    };
};

struct query_parser
{
    const WCHAR *ptr;
    const WCHAR *query;

    WCHAR *scratch;

    struct query_component *components;
    size_t count;
    size_t capacity;

    struct query_component *last;
    struct query_component *prev;

    HRESULT hr;
};

static bool wincodecs_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    new_elements = realloc(*elements, new_capacity * size);
    if (!new_elements)
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

static bool parser_skip_char(struct query_parser *parser, WCHAR ch)
{
    if (FAILED(parser->hr)) return true;

    if (*parser->ptr != ch)
    {
        parser->hr = WINCODEC_ERR_INVALIDQUERYREQUEST;
        return true;
    }
    parser->ptr++;
    return false;
}

static void parse_query_index(struct query_parser *parser, unsigned int *ret)
{
    unsigned int index = 0, d;

    if (parser_skip_char(parser, '[')) return;

    if (*parser->ptr == '*' && *(parser->ptr + 1) == ']')
    {
        FIXME("[*] index value is not supported.\n");
        parser->ptr += 2;
        parser->hr = E_UNEXPECTED;
        return;
    }

    /* Sign prefix is not allowed */

    while (*parser->ptr)
    {
        if (*parser->ptr >= '0' && *parser->ptr <= '9')
            d = *parser->ptr - '0';
        else if (*parser->ptr == ']')
            break;
        else
        {
            parser->hr = WINCODEC_ERR_INVALIDQUERYCHARACTER;
            return;
        }

        index = index * 10 + d;
        parser->ptr++;
    }

    if (*parser->ptr != ']')
    {
        parser->hr = WINCODEC_ERR_INVALIDQUERYCHARACTER;
        return;
    }
    parser->ptr++;

    *ret = index;
}

static bool parser_unescape(struct query_parser *parser)
{
    if (*parser->ptr == '\\')
    {
        parser->ptr++;
        if (!*parser->ptr) return true;
    }

    return false;
}

static HRESULT init_propvar_from_string(const WCHAR *str, PROPVARIANT *var)
{
    size_t size = (wcslen(str) + 1) * sizeof(*str);
    WCHAR *s;

    if (!(s = CoTaskMemAlloc(size)))
        return E_OUTOFMEMORY;
    memcpy(s, str, size);

    var->pwszVal = s;
    var->vt = VT_LPWSTR;
    return S_OK;
}

static void parse_query_name(struct query_parser *parser, PROPVARIANT *item)
{
    size_t len = 0;

    while (*parser->ptr && (*parser->ptr != '/' && *parser->ptr != ':'))
    {
        if (parser_unescape(parser)) break;
        parser->scratch[len++] = *parser->ptr;
        parser->ptr++;
    }

    if (!len)
    {
        parser->hr = WINCODEC_ERR_INVALIDQUERYREQUEST;
        return;
    }

    parser->scratch[len] = 0;

    parser->hr = init_propvar_from_string(parser->scratch, item);
}

static void parse_query_data_item(struct query_parser *parser, PROPVARIANT *item)
{
    struct string_t span;
    PROPVARIANT v;
    GUID guid;
    VARTYPE vt;
    size_t len;

    if (parser_skip_char(parser, '{')) return;

    /* Empty "{}" item represents VT_EMPTY. */
    if (*parser->ptr == '}')
    {
        item->vt = VT_EMPTY;
        parser->ptr++;
        return;
    }

    /* Type */
    span.str = parser->ptr;
    span.len = 0;
    while (*parser->ptr && *parser->ptr != '=')
    {
        span.len++;
        parser->ptr++;
    }

    if (parser_skip_char(parser, '=')) return;

    vt = map_type(&span);
    if (vt == VT_ILLEGAL)
    {
        parser->hr = WINCODEC_ERR_WRONGSTATE;
        return;
    }

    /* Value */
    len = 0;
    while (*parser->ptr && *parser->ptr != '}')
    {
        if (parser_unescape(parser)) break;
        parser->scratch[len++] = *parser->ptr;
        parser->ptr++;
    }

    if (parser_skip_char(parser, '}')) return;

    parser->scratch[len] = 0;

    if (vt == VT_CLSID)
    {
        if (UuidFromStringW(parser->scratch, &guid))
        {
            parser->hr = WINCODEC_ERR_INVALIDQUERYREQUEST;
            return;
        }

        parser->hr = InitPropVariantFromCLSID(&guid, item);
    }
    else
    {
        v.vt = VT_LPWSTR;
        v.pwszVal = parser->scratch;
        parser->hr = PropVariantChangeType(item, &v, 0, vt);
    }
}

static void parse_query_item(struct query_parser *parser, PROPVARIANT *item)
{
    if (FAILED(parser->hr))
        return;

    if (*parser->ptr == '{')
        parse_query_data_item(parser, item);
    else
        parse_query_name(parser, item);
}

static void parse_add_component(struct query_parser *parser, struct query_component *comp)
{
    if (!wincodecs_array_reserve((void **)&parser->components, &parser->capacity,
            parser->count + 1, sizeof(*parser->components)))
    {
        parser->hr = E_OUTOFMEMORY;
        return;
    }

    parser->components[parser->count++] = *comp;
}

static void parse_query_component(struct query_parser *parser)
{
    struct query_component comp = { 0 };
    GUID guid;

    if (*parser->ptr != '/')
    {
        parser->hr = WINCODEC_ERR_PROPERTYNOTSUPPORTED;
        return;
    }
    parser->ptr++;

    /* Optional index */
    if (*parser->ptr == '[')
        parse_query_index(parser, &comp.index);

    parse_query_item(parser, &comp.id);
    if (*parser->ptr == ':')
    {
        parser->ptr++;

        comp.schema = comp.id;
        PropVariantInit(&comp.id);
        parse_query_item(parser, &comp.id);
    }

    /* Resolve known names. */
    if (comp.id.vt == VT_LPWSTR)
    {
        if (SUCCEEDED(WICMapShortNameToGuid(comp.id.pwszVal, &guid)))
        {
            PropVariantClear(&comp.id);
            parser->hr = InitPropVariantFromCLSID(&guid, &comp.id);
        }
    }

    if (SUCCEEDED(parser->hr))
    {
        if (comp.id.vt == VT_CLSID)
        {
            PropVariantClear(&comp.schema);
            if (comp.index)
            {
                comp.schema.vt = VT_UI2;
                comp.schema.uiVal = comp.index;
            }
        }

        parse_add_component(parser, &comp);

        if (FAILED(parser->hr))
        {
            PropVariantClear(&comp.schema);
            PropVariantClear(&comp.id);
        }
    }
}

static HRESULT parser_set_top_level_metadata_handler(struct query_handler *query_handler,
        struct query_parser *parser)
{
    struct query_component *comp;
    IWICMetadataReader *handler;
    HRESULT hr;
    GUID format;
    UINT count, i, matched_index;

    /* Nested handlers are created on IWICMetadataReader/IWICMetadataWriter instances
       directly. Same applies to the query writers created with CreateQueryWriter()/CreateQueryWriterFromReader().

       However decoders and encoders will be using block handlers. */

    if (!is_block_handler(query_handler))
        return S_OK;

    comp = &parser->components[0];

    /* Root component has to be an object within block collection, it's located using {CLSID, index} pair. */
    if (comp->id.vt != VT_CLSID)
        return WINCODEC_ERR_INVALIDQUERYREQUEST;

    hr = IWICMetadataBlockReader_GetCount(query_handler->object.block_reader, &count);
    if (hr != S_OK) return hr;

    matched_index = 0;

    for (i = 0; i < count; ++i)
    {
        if (is_writer_handler(query_handler))
            hr = IWICMetadataBlockWriter_GetWriterByIndex(query_handler->object.block_writer, i, (IWICMetadataWriter **)&handler);
        else
            hr = IWICMetadataBlockReader_GetReaderByIndex(query_handler->object.block_reader, i, &handler);

        if (FAILED(hr))
            break;

        hr = IWICMetadataReader_GetMetadataFormat(handler, &format);
        if (hr == S_OK)
        {
            if (IsEqualGUID(&format, comp->id.puuid))
            {
                if (matched_index == comp->index)
                    break;

                matched_index++;
            }
        }

        IWICMetadataReader_Release(handler);
        handler = NULL;

        if (hr != S_OK) break;
    }

    if (FAILED(hr)) return hr;

    comp->reader = handler;
    return comp->reader ? S_OK : WINCODEC_ERR_PROPERTYNOTFOUND;
}

static void parser_resolve_component_handlers(struct query_handler *query_handler, struct query_parser *parser)
{
    PROPVARIANT value;
    const WCHAR *url;
    GUID guid;
    size_t i;

    if (FAILED(parser->hr = parser_set_top_level_metadata_handler(query_handler, parser)))
        return;

    /* First component contains the root handler for this query. It's provided either
       through block reader or specified explicitly on query creation. */
    for (i = 1; i < parser->count; ++i)
    {
        struct query_component *prev_comp = &parser->components[i - 1];
        struct query_component *comp = &parser->components[i];

        if (!prev_comp->handler)
            continue;

        /* Expand schema urls for "known" formats. */
        if (comp->schema.vt == VT_LPWSTR)
        {
            if (SUCCEEDED(IWICMetadataReader_GetMetadataFormat(prev_comp->reader, &guid)))
            {
                url = map_shortname_to_schema(&guid, comp->schema.pwszVal);
                if (url)
                {
                    PropVariantClear(&comp->schema);
                    init_propvar_from_string(url, &comp->schema);
                }
            }
        }

        PropVariantInit(&value);
        if (FAILED(parser->hr = IWICMetadataReader_GetValue(prev_comp->reader, &comp->schema, &comp->id, &value)))
            break;

        if (value.vt == VT_UNKNOWN)
        {
            parser->hr = IUnknown_QueryInterface(value.punkVal, is_writer_handler(query_handler) ?
                    &IID_IWICMetadataWriter : &IID_IWICMetadataReader, (void **)&comp->handler);
        }
        PropVariantClear(&value);

        if (FAILED(parser->hr))
            break;
    }
}

static HRESULT parse_query(struct query_handler *query_handler, const WCHAR *query,
        struct query_parser *parser)
{
    struct query_component comp = { 0 };
    size_t len;

    memset(parser, 0, sizeof(*parser));

    /* Unspecified item is only allowed at root level. Replace it with an empty item notation,
       so that it can work properly for the readers and fail, as it should, for the block readers. */
    if (!wcscmp(query, L"/"))
        query = L"/{}";

    len = wcslen(query) + 1;
    if (!(parser->scratch = malloc(len * sizeof(WCHAR))))
    {
        parser->hr = E_OUTOFMEMORY;
        return parser->hr;
    }

    parser->query = query;
    parser->ptr = query;

    if (!is_block_handler(query_handler))
    {
        comp.handler = query_handler->object.handler;
        IUnknown_AddRef(comp.handler);
        parse_add_component(parser, &comp);
    }

    while (*parser->ptr && parser->hr == S_OK)
        parse_query_component(parser);

    if (FAILED(parser->hr)) return parser->hr;

    if (!parser->count)
        return parser->hr = WINCODEC_ERR_INVALIDQUERYREQUEST;

    parser_resolve_component_handlers(query_handler, parser);

    /* Validate that query is usable - it should produce an object or
       an object followed by a value id. */
    if (SUCCEEDED(parser->hr))
    {
        parser->last = &parser->components[parser->count - 1];
        parser->prev = parser->count > 1 ? &parser->components[parser->count - 2] : NULL;

        if (!parser->last->handler && !(parser->prev && parser->prev->handler))
            return parser->hr = WINCODEC_ERR_INVALIDQUERYREQUEST;
    }

    return parser->hr;
}

static void parser_cleanup(struct query_parser *parser)
{
    size_t i;

    for (i = 0; i < parser->count; ++i)
    {
        if (parser->components[i].handler)
            IUnknown_Release(parser->components[i].handler);
        PropVariantClear(&parser->components[i].schema);
        PropVariantClear(&parser->components[i].id);
    }
    free(parser->components);
    free(parser->scratch);
}

static HRESULT create_query_handler(IUnknown *block_handler, enum metadata_object_type object_type,
        const WCHAR *location, IWICMetadataQueryWriter **ret);

static HRESULT WINAPI query_handler_GetMetadataByName(IWICMetadataQueryWriter *iface, LPCWSTR query, PROPVARIANT *value)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);
    struct query_component *last, *prev;
    struct query_parser parser;
    HRESULT hr;

    TRACE("(%p,%s,%p)\n", iface, wine_dbgstr_w(query), value);

    if (SUCCEEDED(hr = parse_query(handler, query, &parser)))
    {
        last = parser.last;
        prev = parser.prev;

        if (last->handler)
        {
            if (value)
            {
                value->vt = VT_UNKNOWN;
                hr = create_query_handler(last->handler, is_writer_handler(handler) ? WRITER : READER,
                        parser.query, (IWICMetadataQueryWriter **)&value->punkVal);
            }
        }
        else
        {
            hr = IWICMetadataReader_GetValue(prev->reader, &last->schema, &last->id, value);
        }
    }

    parser_cleanup(&parser);

    return hr;
}

static WCHAR *query_get_guid_item_string(WCHAR *str, unsigned int len, const GUID *guid, bool resolve)
{
    if (resolve && SUCCEEDED(WICMapGuidToShortName(guid, len, str, NULL)))
        return str;

    swprintf(str, len, L"{guid=%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1], guid->Data4[2],
            guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return str;
}

struct string_enumerator_guids
{
    struct
    {
        GUID guid;
        unsigned int count;
    } *entries;
    size_t count;
    size_t capacity;
};

struct string_enumerator
{
    IEnumString IEnumString_iface;
    LONG ref;

    struct string_enumerator_guids guids;

    IEnumUnknown *object_enumerator;
    IWICEnumMetadataItem *metadata_enumerator;
};

static struct string_enumerator *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, struct string_enumerator, IEnumString_iface);
}

static HRESULT WINAPI string_enumerator_QueryInterface(IEnumString *iface, REFIID riid, void **ppv)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);

    TRACE("iface %p, riid %s, ppv %p.\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IEnumString) || IsEqualGUID(riid, &IID_IUnknown))
        *ppv = &this->IEnumString_iface;
    else
    {
        WARN("Unknown riid %s.\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(&this->IEnumString_iface);
    return S_OK;
}

static ULONG WINAPI string_enumerator_AddRef(IEnumString *iface)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);
    ULONG ref = InterlockedIncrement(&this->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI string_enumerator_Release(IEnumString *iface)
{
    struct string_enumerator *enumerator = impl_from_IEnumString(iface);
    ULONG ref = InterlockedDecrement(&enumerator->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        if (enumerator->object_enumerator)
            IEnumUnknown_Release(enumerator->object_enumerator);
        if (enumerator->metadata_enumerator)
            IWICEnumMetadataItem_Release(enumerator->metadata_enumerator);
        free(enumerator->guids.entries);
        free(enumerator);
    }

    return ref;
}

static HRESULT string_enumerator_update_guid_index(struct string_enumerator *enumerator,
        GUID *guid, unsigned int *index, IUnknown *object)
{
    IWICMetadataReader *reader = NULL;
    HRESULT hr;
    size_t i;

    *index = 0;

    hr = IUnknown_QueryInterface(object, &IID_IWICMetadataReader, (void **)&reader);

    if (SUCCEEDED(hr))
        hr = IWICMetadataReader_GetMetadataFormat(reader, guid);

    if (reader)
        IWICMetadataReader_Release(reader);

    if (SUCCEEDED(hr))
    {
        for (i = 0; i < enumerator->guids.count; ++i)
        {
            if (IsEqualGUID(&enumerator->guids.entries[i].guid, guid))
            {
                *index = enumerator->guids.entries[i].count++;
                return S_OK;
            }
        }

        if (!wincodecs_array_reserve((void **)&enumerator->guids.entries, &enumerator->guids.capacity,
                enumerator->guids.count + 1, sizeof(*enumerator->guids.entries)))
        {
            return E_OUTOFMEMORY;
        }
        enumerator->guids.entries[enumerator->guids.count].guid = *guid;
        enumerator->guids.entries[enumerator->guids.count].count = 1;
        enumerator->guids.count++;
    }

    return hr;
}

static HRESULT get_query_item_name(const PROPVARIANT *var, WCHAR **name)
{
    const WCHAR *type;
    WCHAR buffer[64];
    PROPVARIANT dest;
    HRESULT hr;
    size_t len;

    if (var->vt == VT_CLSID)
    {
        const WCHAR *value = query_get_guid_item_string(buffer, ARRAY_SIZE(buffer), var->puuid, false);
        if (!(*name = wcsdup(value)))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    len = 4;
    PropVariantInit(&dest);
    type = get_type_name(var->vt);
    if (type)
    {
        if (FAILED(hr = PropVariantChangeType(&dest, var, 0, VT_LPWSTR))) return hr;
        len += wcslen(type) + wcslen(dest.pwszVal);
    }

    if (!(*name = malloc(len * sizeof(WCHAR))))
    {
        PropVariantClear(&dest);
        return E_OUTOFMEMORY;
    }

    if (type)
        swprintf(*name, len, L"{%s=%s}", type, dest.pwszVal);
    else
        wcscpy(*name, L"{}");
    PropVariantClear(&dest);

    return S_OK;
}

static HRESULT WINAPI string_enumerator_Next(IEnumString *iface, ULONG count,
        LPOLESTR *strings, ULONG *fetched)
{
    struct string_enumerator *enumerator = impl_from_IEnumString(iface);
    WCHAR *str, name[64];
    unsigned int index;
    IUnknown **objects;
    HRESULT hr = S_OK;
    ULONG tmp;
    GUID guid;

    TRACE("iface %p, count %lu, strings %p, fetched %p.\n", iface, count, strings, fetched);

    if (!strings)
        return E_INVALIDARG;

    if (!fetched) fetched = &tmp;
    *fetched = 0;

    if (enumerator->object_enumerator)
    {
        ULONG fetched_objects;

        if (!(objects = calloc(count, sizeof(*objects))))
            return E_OUTOFMEMORY;

        fetched_objects = 0;
        if (SUCCEEDED(hr = IEnumUnknown_Next(enumerator->object_enumerator, count, objects, &fetched_objects)))
        {
            for (ULONG i = 0; i < fetched_objects; ++i)
            {
                index = 0;
                hr = string_enumerator_update_guid_index(enumerator, &guid, &index, objects[i]);
                if (FAILED(hr)) break;

                if (!(str = CoTaskMemAlloc(64 * sizeof(WCHAR))))
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                query_get_guid_item_string(name, ARRAY_SIZE(name), &guid, true);

                if (index)
                    swprintf(str, 64, L"/[%u]%s", index, name);
                else
                    swprintf(str, 64, L"/%s", name);

                strings[i] = str;
                *fetched += 1;
            }

            for (ULONG i = 0; i < fetched_objects; ++i)
                IUnknown_Release(objects[i]);
        }

        free(objects);
    }
    else if (enumerator->metadata_enumerator)
    {
        PROPVARIANT *vars, *schemas, *ids;
        ULONG vars_count = max(count, 1);
        ULONG fetched_items;

        if (!(vars = calloc(vars_count * 2, sizeof(*vars))))
            return E_OUTOFMEMORY;
        schemas = vars;
        ids = vars + vars_count;

        fetched_items = 0;
        if (SUCCEEDED(hr = IWICEnumMetadataItem_Next(enumerator->metadata_enumerator, count, schemas, ids, NULL, &fetched_items)))
        {
            for (ULONG i = 0; i < fetched_items; ++i)
            {
                WCHAR *schema_name = NULL, *id_name = NULL;
                size_t size;

                if (SUCCEEDED(hr) && schemas[i].vt != VT_EMPTY)
                    hr = get_query_item_name(&schemas[i], &schema_name);
                if (SUCCEEDED(hr))
                    hr = get_query_item_name(&ids[i], &id_name);

                if (SUCCEEDED(hr))
                {
                    size = 4;
                    if (schema_name)
                        size += wcslen(schema_name);
                    if (id_name)
                        size += wcslen(id_name);

                    if (!(strings[i] = CoTaskMemAlloc(size * sizeof(WCHAR))))
                        hr = E_OUTOFMEMORY;

                    if (SUCCEEDED(hr))
                    {
                        wcscpy(strings[i], L"/");
                        if (schema_name)
                        {
                            wcscat(strings[i], schema_name);
                            wcscat(strings[i], L":");
                        }
                        wcscat(strings[i], id_name);
                        *fetched += 1;
                    }
                }

                free(schema_name);
                free(id_name);
                PropVariantClear(&schemas[i]);
                PropVariantClear(&ids[i]);
            }
        }

        free(vars);
    }

    if (SUCCEEDED(hr))
    {
        hr = *fetched < count ? S_FALSE : S_OK;
    }
    else
    {
        for (ULONG i = 0; i < *fetched; ++i)
        {
            CoTaskMemFree(strings[i]);
            strings[i] = NULL;
        }
        *fetched = 0;
    }

    return hr;
}

static HRESULT WINAPI string_enumerator_Reset(IEnumString *iface)
{
    struct string_enumerator *enumerator = impl_from_IEnumString(iface);

    TRACE("iface %p.\n", iface);

    enumerator->guids.count = 0;
    if (enumerator->object_enumerator)
        return IEnumUnknown_Reset(enumerator->object_enumerator);
    else if (enumerator->metadata_enumerator)
        return IWICEnumMetadataItem_Reset(enumerator->metadata_enumerator);

    return S_OK;
}

static HRESULT WINAPI string_enumerator_Skip(IEnumString *iface, ULONG count)
{
    struct string_enumerator *enumerator = impl_from_IEnumString(iface);
    unsigned int index;
    HRESULT hr = S_OK;
    IUnknown *object;
    ULONG fetched;
    GUID guid;

    TRACE("iface %p, count %lu.\n", iface, count);

    if (enumerator->object_enumerator)
    {
        while (count--)
        {
            hr = IEnumUnknown_Next(enumerator->object_enumerator, 1, &object, &fetched);
            if (hr != S_OK) break;

            string_enumerator_update_guid_index(enumerator, &guid, &index, object);
            IUnknown_Release(object);
        }
    }
    else if (enumerator->metadata_enumerator)
    {
        hr = IWICEnumMetadataItem_Skip(enumerator->metadata_enumerator, count);
    }

    return hr;
}

static HRESULT string_enumerator_init(struct string_enumerator *object, const struct string_enumerator_guids *guids,
        IEnumUnknown *object_enumerator, IWICEnumMetadataItem *metadata_enumerator);

static HRESULT WINAPI string_enumerator_Clone(IEnumString *iface, IEnumString **out)
{
    struct string_enumerator *enumerator = impl_from_IEnumString(iface);
    IWICEnumMetadataItem *metadata_enumerator = NULL;
    IEnumUnknown *object_enumerator = NULL;
    struct string_enumerator *object;
    HRESULT hr = S_OK;

    TRACE("iface %p, out %p.\n", iface, out);

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (enumerator->object_enumerator)
        hr = IEnumUnknown_Clone(enumerator->object_enumerator, &object_enumerator);
    else if (enumerator->metadata_enumerator)
        hr = IWICEnumMetadataItem_Clone(enumerator->metadata_enumerator, &metadata_enumerator);

    if (FAILED(hr))
    {
        free(object);
        return hr;
    }

    hr = string_enumerator_init(object, &enumerator->guids, object_enumerator, metadata_enumerator);

    if (metadata_enumerator)
        IWICEnumMetadataItem_Release(metadata_enumerator);
    if (object_enumerator)
        IEnumUnknown_Release(object_enumerator);

    if (SUCCEEDED(hr))
    {
        *out = &object->IEnumString_iface;
        IEnumString_AddRef(*out);
    }

    IEnumString_Release(&object->IEnumString_iface);

    return hr;
}

static const IEnumStringVtbl string_enumerator_vtbl =
{
    string_enumerator_QueryInterface,
    string_enumerator_AddRef,
    string_enumerator_Release,
    string_enumerator_Next,
    string_enumerator_Skip,
    string_enumerator_Reset,
    string_enumerator_Clone
};

static HRESULT string_enumerator_init(struct string_enumerator *object, const struct string_enumerator_guids *guids,
        IEnumUnknown *object_enumerator, IWICEnumMetadataItem *metadata_enumerator)
{
    object->IEnumString_iface.lpVtbl = &string_enumerator_vtbl;
    object->ref = 1;

    if (guids && guids->count)
    {
        if (!(object->guids.entries = calloc(guids->count, sizeof(*object->guids.entries))))
            return E_OUTOFMEMORY;

        memcpy(object->guids.entries, guids->entries, guids->count * sizeof(*object->guids.entries));
        object->guids.count = object->guids.capacity = guids->count;
    }

    object->object_enumerator = object_enumerator;
    if (object->object_enumerator)
        IEnumUnknown_AddRef(object->object_enumerator);
    object->metadata_enumerator = metadata_enumerator;
    if (object->metadata_enumerator)
        IWICEnumMetadataItem_AddRef(object->metadata_enumerator);

    return S_OK;
}

static HRESULT string_enumerator_create(struct query_handler *handler, IEnumString **out)
{
    IWICEnumMetadataItem *metadata_enumerator = NULL;
    IEnumUnknown *object_enumerator = NULL;
    struct string_enumerator *object;
    HRESULT hr;

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (is_block_handler(handler))
    {
        IWICMetadataBlockReader_GetEnumerator(handler->object.block_reader, &object_enumerator);
    }
    else
    {
        IWICMetadataReader_GetEnumerator(handler->object.reader, &metadata_enumerator);
    }

    hr = string_enumerator_init(object, NULL, object_enumerator, metadata_enumerator);

    if (object_enumerator)
        IEnumUnknown_Release(object_enumerator);
    if (metadata_enumerator)
        IWICEnumMetadataItem_Release(metadata_enumerator);

    if (SUCCEEDED(hr))
    {
        *out = &object->IEnumString_iface;
        IEnumString_AddRef(*out);
    }

    IEnumString_Release(&object->IEnumString_iface);

    return hr;
}

static HRESULT WINAPI query_handler_GetEnumerator(IWICMetadataQueryWriter *iface,
        IEnumString **enum_string)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);

    TRACE("iface %p, enum_string %p.\n", iface, enum_string);

    return string_enumerator_create(handler, enum_string);
}

static HRESULT WINAPI query_handler_SetMetadataByName(IWICMetadataQueryWriter *iface, LPCWSTR name, const PROPVARIANT *value)
{
    FIXME("iface %p, name %s, value %p stub.\n", iface, debugstr_w(name), value);

    return S_OK;
}

static HRESULT WINAPI query_handler_RemoveMetadataByName(IWICMetadataQueryWriter *iface, const WCHAR *query)
{
    struct query_handler *handler = impl_from_IWICMetadataQueryWriter(iface);
    struct query_component *last, *prev;
    struct query_parser parser;
    HRESULT hr;

    TRACE("iface %p, name %s.\n", iface, debugstr_w(query));

    if (!query)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = parse_query(handler, query, &parser)))
    {
        last = parser.last;
        prev = parser.prev;

        if (is_block_handler(handler) && parser.count == 1)
            hr = IWICMetadataBlockWriter_RemoveWriterByIndex(handler->object.block_writer, last->index);
        else
            hr = IWICMetadataWriter_RemoveValue(prev->writer, &last->schema, &last->id);
    }

    parser_cleanup(&parser);

    return hr;
}

static IWICMetadataQueryWriterVtbl query_handler_vtbl =
{
    query_handler_QueryInterface,
    query_handler_AddRef,
    query_handler_Release,
    query_handler_GetContainerFormat,
    query_handler_GetLocation,
    query_handler_GetMetadataByName,
    query_handler_GetEnumerator,
    query_handler_SetMetadataByName,
    query_handler_RemoveMetadataByName,
};

static HRESULT create_query_handler(IUnknown *block_handler, enum metadata_object_type object_type,
        const WCHAR *root, IWICMetadataQueryWriter **ret)
{
    struct query_handler *obj;
    WCHAR buff[64];
    HRESULT hr;
    GUID guid;

    obj = calloc(1, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IWICMetadataQueryWriter_iface.lpVtbl = &query_handler_vtbl;
    obj->ref = 1;
    IUnknown_AddRef(block_handler);
    obj->object.handler = block_handler;
    obj->object_type = object_type;
    if (!root)
    {
        if (is_block_handler(obj))
        {
            root = L"/";
        }
        else
        {
            if (FAILED(hr = IWICMetadataReader_GetMetadataFormat(obj->object.reader, &guid)))
            {
                IWICMetadataQueryWriter_Release(&obj->IWICMetadataQueryWriter_iface);
                return hr;
            }

            buff[0] = '/';
            query_get_guid_item_string(buff + 1, ARRAY_SIZE(buff) - 1, &guid, true);
            root = buff;
        }
    }

    obj->root = wcsdup(root);

    *ret = &obj->IWICMetadataQueryWriter_iface;

    return S_OK;
}

HRESULT MetadataQueryReader_CreateInstanceFromBlockReader(IWICMetadataBlockReader *block_reader,
        IWICMetadataQueryReader **out)
{
    return create_query_handler((IUnknown *)block_reader, BLOCK_READER, NULL, (IWICMetadataQueryWriter **)out);
}

HRESULT MetadataQueryWriter_CreateInstanceFromBlockWriter(IWICMetadataBlockWriter *block_writer,
        IWICMetadataQueryWriter **out)
{
    return create_query_handler((IUnknown *)block_writer, BLOCK_WRITER, NULL, out);
}

HRESULT MetadataQueryReader_CreateInstance(IWICMetadataReader *reader, IWICMetadataQueryReader **out)
{
    return create_query_handler((IUnknown *)reader, READER, NULL, (IWICMetadataQueryWriter **)out);
}

HRESULT MetadataQueryWriter_CreateInstance(IWICMetadataWriter *writer, IWICMetadataQueryWriter **out)
{
    return create_query_handler((IUnknown *)writer, WRITER, NULL, out);
}

static const struct
{
    const GUID *guid;
    const WCHAR *name;
} guid2name[] =
{
    { &GUID_ContainerFormatBmp, L"bmp" },
    { &GUID_ContainerFormatPng, L"png" },
    { &GUID_ContainerFormatIco, L"ico" },
    { &GUID_ContainerFormatJpeg, L"jpg" },
    { &GUID_ContainerFormatTiff, L"tiff" },
    { &GUID_ContainerFormatGif, L"gif" },
    { &GUID_ContainerFormatWmp, L"wmphoto" },
    { &GUID_MetadataFormatUnknown, L"unknown" },
    { &GUID_MetadataFormatIfd, L"ifd" },
    { &GUID_MetadataFormatSubIfd, L"sub" },
    { &GUID_MetadataFormatExif, L"exif" },
    { &GUID_MetadataFormatGps, L"gps" },
    { &GUID_MetadataFormatInterop, L"interop" },
    { &GUID_MetadataFormatApp0, L"app0" },
    { &GUID_MetadataFormatApp1, L"app1" },
    { &GUID_MetadataFormatApp13, L"app13" },
    { &GUID_MetadataFormatIPTC, L"iptc" },
    { &GUID_MetadataFormatIRB, L"irb" },
    { &GUID_MetadataFormat8BIMIPTC, L"8bimiptc" },
    { &GUID_MetadataFormat8BIMResolutionInfo, L"8bimResInfo" },
    { &GUID_MetadataFormat8BIMIPTCDigest, L"8bimiptcdigest" },
    { &GUID_MetadataFormatXMP, L"xmp" },
    { &GUID_MetadataFormatThumbnail, L"thumb" },
    { &GUID_MetadataFormatChunktEXt, L"tEXt" },
    { &GUID_MetadataFormatXMPStruct, L"xmpstruct" },
    { &GUID_MetadataFormatXMPBag, L"xmpbag" },
    { &GUID_MetadataFormatXMPSeq, L"xmpseq" },
    { &GUID_MetadataFormatXMPAlt, L"xmpalt" },
    { &GUID_MetadataFormatLSD, L"logscrdesc" },
    { &GUID_MetadataFormatIMD, L"imgdesc" },
    { &GUID_MetadataFormatGCE, L"grctlext" },
    { &GUID_MetadataFormatAPE, L"appext" },
    { &GUID_MetadataFormatJpegChrominance, L"chrominance" },
    { &GUID_MetadataFormatJpegLuminance, L"luminance" },
    { &GUID_MetadataFormatJpegComment, L"com" },
    { &GUID_MetadataFormatGifComment, L"commentext" },
    { &GUID_MetadataFormatChunkgAMA, L"gAMA" },
    { &GUID_MetadataFormatChunkbKGD, L"bKGD" },
    { &GUID_MetadataFormatChunkiTXt, L"iTXt" },
    { &GUID_MetadataFormatChunkcHRM, L"cHRM" },
    { &GUID_MetadataFormatChunkhIST, L"hIST" },
    { &GUID_MetadataFormatChunkiCCP, L"iCCP" },
    { &GUID_MetadataFormatChunksRGB, L"sRGB" },
    { &GUID_MetadataFormatChunktIME, L"tIME" }
};

HRESULT WINAPI WICMapGuidToShortName(REFGUID guid, UINT len, WCHAR *name, UINT *ret_len)
{
    UINT i;

    TRACE("%s,%u,%p,%p\n", wine_dbgstr_guid(guid), len, name, ret_len);

    if (!guid) return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(guid2name); i++)
    {
        if (IsEqualGUID(guid, guid2name[i].guid))
        {
            if (name)
            {
                if (!len) return E_INVALIDARG;

                len = min(len - 1, lstrlenW(guid2name[i].name));
                memcpy(name, guid2name[i].name, len * sizeof(WCHAR));
                name[len] = 0;

                if (len < lstrlenW(guid2name[i].name))
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            if (ret_len) *ret_len = lstrlenW(guid2name[i].name) + 1;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}

HRESULT WINAPI WICMapShortNameToGuid(PCWSTR name, GUID *guid)
{
    UINT i;

    TRACE("%s,%p\n", debugstr_w(name), guid);

    if (!name || !guid) return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(guid2name); i++)
    {
        if (!lstrcmpiW(name, guid2name[i].name))
        {
            *guid = *guid2name[i].guid;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}

static const struct
{
    const WCHAR *name;
    const WCHAR *schema;
} name2schema[] =
{
    { L"rdf", L"http://www.w3.org/1999/02/22-rdf-syntax-ns#" },
    { L"dc", L"http://purl.org/dc/elements/1.1/" },
    { L"xmp", L"http://ns.adobe.com/xap/1.0/" },
    { L"xmpidq", L"http://ns.adobe.com/xmp/Identifier/qual/1.0/" },
    { L"xmpRights", L"http://ns.adobe.com/xap/1.0/rights/" },
    { L"xmpMM", L"http://ns.adobe.com/xap/1.0/mm/" },
    { L"xmpBJ", L"http://ns.adobe.com/xap/1.0/bj/" },
    { L"xmpTPg", L"http://ns.adobe.com/xap/1.0/t/pg/" },
    { L"pdf", L"http://ns.adobe.com/pdf/1.3/" },
    { L"photoshop", L"http://ns.adobe.com/photoshop/1.0/" },
    { L"tiff", L"http://ns.adobe.com/tiff/1.0/" },
    { L"exif", L"http://ns.adobe.com/exif/1.0/" },
    { L"stDim", L"http://ns.adobe.com/xap/1.0/sType/Dimensions#" },
    { L"xapGImg", L"http://ns.adobe.com/xap/1.0/g/img/" },
    { L"stEvt", L"http://ns.adobe.com/xap/1.0/sType/ResourceEvent#" },
    { L"stRef", L"http://ns.adobe.com/xap/1.0/sType/ResourceRef#" },
    { L"stVer", L"http://ns.adobe.com/xap/1.0/sType/Version#" },
    { L"stJob", L"http://ns.adobe.com/xap/1.0/sType/Job#" },
    { L"aux", L"http://ns.adobe.com/exif/1.0/aux/" },
    { L"crs", L"http://ns.adobe.com/camera-raw-settings/1.0/" },
    { L"xmpDM", L"http://ns.adobe.com/xmp/1.0/DynamicMedia/" },
    { L"Iptc4xmpCore", L"http://iptc.org/std/Iptc4xmpCore/1.0/xmlns/" },
    { L"MicrosoftPhoto", L"http://ns.microsoft.com/photo/1.0/" },
    { L"MP", L"http://ns.microsoft.com/photo/1.2/" },
    { L"MPRI", L"http://ns.microsoft.com/photo/1.2/t/RegionInfo#" },
    { L"MPReg", L"http://ns.microsoft.com/photo/1.2/t/Region#" }
};

static const WCHAR *map_shortname_to_schema(const GUID *format, const WCHAR *name)
{
    UINT i;

    /* It appears that the only metadata formats
     * that support schemas are xmp and xmpstruct.
     */
    if (!IsEqualGUID(format, &GUID_MetadataFormatXMP) &&
        !IsEqualGUID(format, &GUID_MetadataFormatXMPStruct))
        return NULL;

    for (i = 0; i < ARRAY_SIZE(name2schema); i++)
    {
        if (!wcscmp(name2schema[i].name, name))
            return name2schema[i].schema;
    }

    return NULL;
}

HRESULT WINAPI WICMapSchemaToName(REFGUID format, LPWSTR schema, UINT len, WCHAR *name, UINT *ret_len)
{
    UINT i;

    TRACE("%s,%s,%u,%p,%p\n", wine_dbgstr_guid(format), debugstr_w(schema), len, name, ret_len);

    if (!format || !schema || !ret_len)
        return E_INVALIDARG;

    /* It appears that the only metadata formats
     * that support schemas are xmp and xmpstruct.
     */
    if (!IsEqualGUID(format, &GUID_MetadataFormatXMP) &&
        !IsEqualGUID(format, &GUID_MetadataFormatXMPStruct))
        return WINCODEC_ERR_PROPERTYNOTFOUND;

    for (i = 0; i < ARRAY_SIZE(name2schema); i++)
    {
        if (!wcscmp(name2schema[i].schema, schema))
        {
            if (name)
            {
                if (!len) return E_INVALIDARG;

                len = min(len - 1, lstrlenW(name2schema[i].name));
                memcpy(name, name2schema[i].name, len * sizeof(WCHAR));
                name[len] = 0;

                if (len < lstrlenW(name2schema[i].name))
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }

            *ret_len = lstrlenW(name2schema[i].name) + 1;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}
