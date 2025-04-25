/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2016 Dmitry Timoshkov
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlwapi.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

static inline USHORT read_ushort_be(BYTE* data)
{
    return data[0] << 8 | data[1];
}

static inline ULONG read_ulong_be(BYTE* data)
{
    return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
}

static HRESULT LoadTextMetadata(MetadataHandler *handler, IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    ULONG name_len, value_len;
    BYTE *name_end_ptr;
    LPSTR name, value;
    MetadataItem *result;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    name_end_ptr = memchr(data, 0, data_size);

    name_len = name_end_ptr - data;

    if (!name_end_ptr || name_len > 79)
    {
        free(data);
        return E_FAIL;
    }

    value_len = data_size - name_len - 1;

    result = calloc(1, sizeof(MetadataItem));
    name = CoTaskMemAlloc(name_len + 1);
    value = CoTaskMemAlloc(value_len + 1);
    if (!result || !name || !value)
    {
        free(data);
        free(result);
        CoTaskMemFree(name);
        CoTaskMemFree(value);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result[0].schema);
    PropVariantInit(&result[0].id);
    PropVariantInit(&result[0].value);

    memcpy(name, data, name_len + 1);
    memcpy(value, name_end_ptr + 1, value_len);
    value[value_len] = 0;

    result[0].id.vt = VT_LPSTR;
    result[0].id.pszVal = name;
    result[0].value.vt = VT_LPSTR;
    result[0].value.pszVal = value;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 1;

    free(data);

    return S_OK;
}

static const MetadataHandlerVtbl TextReader_Vtbl = {
    0,
    &CLSID_WICPngTextMetadataReader,
    LoadTextMetadata
};

HRESULT PngTextReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&TextReader_Vtbl, iid, ppv);
}

static HRESULT create_gamma_item(ULONG gamma, MetadataItem **item)
{
    HRESULT hr;

    if (!(*item = calloc(1, sizeof(**item))))
        return E_OUTOFMEMORY;

    hr = init_propvar_from_string(L"ImageGamma", &(*item)->id);
    if (FAILED(hr))
    {
        free(*item);
        return hr;
    }

    (*item)->value.vt = VT_UI4;
    (*item)->value.ulVal = gamma;

    return S_OK;
}

static HRESULT LoadGamaMetadata(MetadataHandler *handler, IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    ULONG gamma;
    MetadataItem *result;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    if (data_size < 4)
    {
        free(data);
        return E_FAIL;
    }

    gamma = read_ulong_be(data);

    free(data);

    if (FAILED(hr = create_gamma_item(gamma, &result)))
        return hr;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 1;

    return S_OK;
}

static HRESULT CreateGamaHandler(MetadataHandler *handler)
{
    MetadataItem *item;
    HRESULT hr;

    if (FAILED(hr = create_gamma_item(45455, &item)))
        return hr;

    handler->items = item;
    handler->item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl GamaReader_Vtbl = {
    0,
    &CLSID_WICPngGamaMetadataReader,
    LoadGamaMetadata,
    CreateGamaHandler,
};

HRESULT PngGamaReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&GamaReader_Vtbl, iid, ppv);
}

static HRESULT create_chrm_items(const ULONG *values, MetadataItem **ret)
{
    static const WCHAR *names[8] =
    {
        L"WhitePointX",
        L"WhitePointY",
        L"RedX",
        L"RedY",
        L"GreenX",
        L"GreenY",
        L"BlueX",
        L"BlueY",
    };
    MetadataItem *items;
    HRESULT hr = S_OK;

    if (!(items = calloc(ARRAY_SIZE(names), sizeof(*items))))
        return E_OUTOFMEMORY;

    for (int i = 0; i < ARRAY_SIZE(names); ++i)
    {
        if (FAILED(hr = init_propvar_from_string(names[i], &items[i].id)))
            break;

        items[i].value.vt = VT_UI4;
        items[i].value.ulVal = values[i];
    }

    if (FAILED(hr))
    {
        for (int i = 0; i < ARRAY_SIZE(names); ++i)
            clear_metadata_item(&items[i]);
        free(items);
        items = NULL;
    }

    *ret = items;

    return hr;
}

static HRESULT LoadChrmMetadata(MetadataHandler *handler, IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    MetadataItem *result;
    ULONG values[8];
    int i;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    if (data_size < 32)
    {
        free(data);
        return E_FAIL;
    }

    for (i = 0; i < ARRAY_SIZE(values); ++i)
        values[i] = read_ulong_be(&data[i*4]);

    free(data);

    if (FAILED(hr = create_chrm_items(values, &result)))
        return hr;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = ARRAY_SIZE(values);

    return S_OK;
}

static HRESULT CreateChrmHandler(MetadataHandler *handler)
{
    const ULONG values[8] = { 31270, 32900, 64000, 33000, 30000, 60000, 15000, 6000 };
    MetadataItem *items;
    HRESULT hr;

    if (FAILED(hr = create_chrm_items(values, &items)))
        return hr;

    handler->items = items;
    handler->item_count = ARRAY_SIZE(values);

    return S_OK;
}

static const MetadataHandlerVtbl ChrmReader_Vtbl = {
    0,
    &CLSID_WICPngChrmMetadataReader,
    LoadChrmMetadata,
    CreateChrmHandler,
};

HRESULT PngChrmReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&ChrmReader_Vtbl, iid, ppv);
}

static HRESULT create_hist_item(USHORT *data, ULONG count, MetadataItem **item)
{
    HRESULT hr;

    if (!(*item = calloc(1, sizeof(**item))))
        return E_OUTOFMEMORY;

    hr = init_propvar_from_string(L"Frequencies", &(*item)->id);
    if (FAILED(hr))
    {
        free(*item);
        return hr;
    }

    (*item)->value.vt = VT_UI2 | VT_VECTOR;
    (*item)->value.caui.cElems = count;
    (*item)->value.caui.pElems = data;

    return S_OK;
}

static HRESULT LoadHistMetadata(MetadataHandler *handler, IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size, element_count, i;
    MetadataItem *result;
    USHORT *elements;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    element_count = data_size / 2;
    elements = CoTaskMemAlloc(element_count * sizeof(USHORT));
    if (!elements)
    {
        free(data);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < element_count; i++)
        elements[i] = read_ushort_be(data + i * 2);

    free(data);

    if (FAILED(hr = create_hist_item(elements, element_count, &result)))
    {
        CoTaskMemFree(elements);
        return E_OUTOFMEMORY;
    }

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 1;

    return S_OK;
}

static HRESULT CreateHistHandler(MetadataHandler *handler)
{
    MetadataItem *item;
    HRESULT hr;

    if (FAILED(hr = create_hist_item(NULL, 0, &item)))
        return hr;

    handler->items = item;
    handler->item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl HistReader_Vtbl = {
    0,
    &CLSID_WICPngHistMetadataReader,
    LoadHistMetadata,
    CreateHistHandler,
};

HRESULT PngHistReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&HistReader_Vtbl, iid, ppv);
}

struct time_data
{
    USHORT year;
    BYTE month;
    BYTE day;
    BYTE hour;
    BYTE minute;
    BYTE second;
};

static HRESULT create_time_items(const struct time_data *data, MetadataItem **ret)
{
    static const WCHAR *names[6] =
    {
        L"Year",
        L"Month",
        L"Day",
        L"Hour",
        L"Minute",
        L"Second",
    };
    MetadataItem *items;
    HRESULT hr = S_OK;

    if (!(items = calloc(ARRAY_SIZE(names), sizeof(*items))))
        return E_OUTOFMEMORY;

    for (int i = 0; i < ARRAY_SIZE(names); ++i)
    {
        if (FAILED(hr = init_propvar_from_string(names[i], &items[i].id)))
            break;
    }

    if (SUCCEEDED(hr))
    {
        items[0].value.vt = VT_UI2;
        items[0].value.uiVal = data->year;
        items[1].value.vt = VT_UI1;
        items[1].value.bVal = data->month;
        items[2].value.vt = VT_UI1;
        items[2].value.bVal = data->day;
        items[3].value.vt = VT_UI1;
        items[3].value.bVal = data->hour;
        items[4].value.vt = VT_UI1;
        items[4].value.bVal = data->minute;
        items[5].value.vt = VT_UI1;
        items[5].value.bVal = data->second;
    }

    if (FAILED(hr))
    {
        for (int i = 0; i < ARRAY_SIZE(names); ++i)
            clear_metadata_item(&items[i]);
        free(items);
        items = NULL;
    }

    *ret = items;

    return hr;
}

static HRESULT LoadTimeMetadata(MetadataHandler *handler, IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    MetadataItem *result;
    struct time_data time_data;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    if (data_size != 7)
    {
        free(data);
        return E_FAIL;
    }

    time_data.year   = read_ushort_be(data);
    time_data.month  = data[2];
    time_data.day    = data[3];
    time_data.hour   = data[4];
    time_data.minute = data[5];
    time_data.second = data[6];

    free(data);

    if (FAILED(hr = create_time_items(&time_data, &result)))
        return hr;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 6;

    return S_OK;
}

static HRESULT CreateTimeHandler(MetadataHandler *handler)
{
    static const struct time_data time_data = { .month = 1, .day = 1 };
    MetadataItem *items;
    HRESULT hr;

    if (FAILED(hr = create_time_items(&time_data, &items)))
        return hr;

    handler->items = items;
    handler->item_count = 6;

    return S_OK;
}

static const MetadataHandlerVtbl TimeReader_Vtbl = {
    0,
    &CLSID_WICPngTimeMetadataReader,
    LoadTimeMetadata,
    CreateTimeHandler,
};

HRESULT PngTimeReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&TimeReader_Vtbl, iid, ppv);
}

static const MetadataHandlerVtbl TimeWriter_Vtbl =
{
    METADATAHANDLER_FIXED_ITEMS | METADATAHANDLER_IS_WRITER,
    &CLSID_WICPngTimeMetadataWriter,
    LoadTimeMetadata,
    CreateTimeHandler,
};

HRESULT PngTimeWriter_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&TimeWriter_Vtbl, iid, ppv);
}

static HRESULT create_bkgd_item(const PROPVARIANT *value, MetadataItem **ret)
{
    MetadataItem *item;
    HRESULT hr;

    if (!(item = calloc(1, sizeof(*item))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_propvar_from_string(L"BackgroundColor", &item->id)))
    {
        free(item);
        return hr;
    }

    item->value = *value;
    *ret = item;

    return S_OK;
}

static HRESULT LoadBkgdMetadata(MetadataHandler *handler, IStream *stream, const GUID *preferred_vendor,
        DWORD persist_options)
{
    MetadataItem *result;
    PROPVARIANT value;
    ULONG data_size;
    BYTE type[4];
    HRESULT hr;
    BYTE *data;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    PropVariantInit(&value);
    if (data_size == 1)
    {
        value.vt = VT_UI1;
        value.bVal = *data;
    }
    else if (data_size == 2)
    {
        value.vt = VT_UI2;
        value.uiVal = read_ushort_be(data);
    }
    else if (data_size == 6)
    {
        value.vt = VT_UI2 | VT_VECTOR;
        value.caui.cElems = 3;
        if (!(value.caui.pElems = CoTaskMemAlloc(3 * sizeof(USHORT))))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            BYTE *ptr = data;
            for (int i = 0; i < value.caui.cElems; ++i, ptr += 2)
                value.caui.pElems[i] = read_ushort_be(ptr);
        }
    }
    else
    {
        hr = WINCODEC_ERR_BADMETADATAHEADER;
    }

    free(data);

    if (SUCCEEDED(hr))
        hr = create_bkgd_item(&value, &result);

    if (FAILED(hr))
    {
        PropVariantClear(&value);
        return hr;
    }

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 1;

    return S_OK;
}

static HRESULT CreateBkgdHandler(MetadataHandler *handler)
{
    MetadataItem *item;
    PROPVARIANT value;
    HRESULT hr;

    PropVariantInit(&value);
    if (FAILED(hr = create_bkgd_item(&value, &item)))
        return hr;

    handler->items = item;
    handler->item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl BkgdReader_Vtbl =
{
    0,
    &CLSID_WICPngBkgdMetadataReader,
    LoadBkgdMetadata,
    CreateBkgdHandler,
};

HRESULT PngBkgdReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&BkgdReader_Vtbl, iid, ppv);
}

static const MetadataHandlerVtbl BkgdWriter_Vtbl =
{
    METADATAHANDLER_FIXED_ITEMS | METADATAHANDLER_IS_WRITER,
    &CLSID_WICPngBkgdMetadataWriter,
    LoadBkgdMetadata,
    CreateBkgdHandler,
};

HRESULT PngBkgdWriter_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&BkgdWriter_Vtbl, iid, ppv);
}

HRESULT PngDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct decoder *decoder;
    struct decoder_info decoder_info;

    hr = png_decoder_create(&decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}

HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct encoder *encoder;
    struct encoder_info encoder_info;

    hr = png_encoder_create(&encoder_info, &encoder);

    if (SUCCEEDED(hr))
        hr = CommonEncoder_CreateInstance(encoder, &encoder_info, iid, ppv);

    return hr;
}
