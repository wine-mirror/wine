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

#define NONAMELESSUNION
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

static inline ULONG read_ulong_be(BYTE* data)
{
    return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
}

static HRESULT LoadTextMetadata(IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
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
        HeapFree(GetProcessHeap(), 0, data);
        return E_FAIL;
    }

    value_len = data_size - name_len - 1;

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem));
    name = HeapAlloc(GetProcessHeap(), 0, name_len + 1);
    value = HeapAlloc(GetProcessHeap(), 0, value_len + 1);
    if (!result || !name || !value)
    {
        HeapFree(GetProcessHeap(), 0, data);
        HeapFree(GetProcessHeap(), 0, result);
        HeapFree(GetProcessHeap(), 0, name);
        HeapFree(GetProcessHeap(), 0, value);
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

    *items = result;
    *item_count = 1;

    HeapFree(GetProcessHeap(), 0, data);

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

static HRESULT LoadGamaMetadata(IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    ULONG gamma;
    LPWSTR name;
    MetadataItem *result;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    if (data_size < 4)
    {
        HeapFree(GetProcessHeap(), 0, data);
        return E_FAIL;
    }

    gamma = read_ulong_be(data);

    HeapFree(GetProcessHeap(), 0, data);

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem));
    name = HeapAlloc(GetProcessHeap(), 0, sizeof(L"ImageGamma"));
    if (!result || !name)
    {
        HeapFree(GetProcessHeap(), 0, result);
        HeapFree(GetProcessHeap(), 0, name);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result[0].schema);
    PropVariantInit(&result[0].id);
    PropVariantInit(&result[0].value);

    memcpy(name, L"ImageGamma", sizeof(L"ImageGamma"));

    result[0].id.vt = VT_LPWSTR;
    result[0].id.pwszVal = name;
    result[0].value.vt = VT_UI4;
    result[0].value.ulVal = gamma;

    *items = result;
    *item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl GamaReader_Vtbl = {
    0,
    &CLSID_WICPngGamaMetadataReader,
    LoadGamaMetadata
};

HRESULT PngGamaReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&GamaReader_Vtbl, iid, ppv);
}

static HRESULT LoadChrmMetadata(IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    static const WCHAR names[8][12] = {
        L"WhitePointX",
        L"WhitePointY",
        L"RedX",
        L"RedY",
        L"GreenX",
        L"GreenY",
        L"BlueX",
        L"BlueY",
    };
    LPWSTR dyn_names[8] = {0};
    MetadataItem *result;
    int i;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    if (data_size < 32)
    {
        HeapFree(GetProcessHeap(), 0, data);
        return E_FAIL;
    }

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem)*8);
    for (i=0; i<8; i++)
    {
        dyn_names[i] = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(lstrlenW(names[i])+1));
        if (!dyn_names[i]) break;
    }
    if (!result || i < 8)
    {
        HeapFree(GetProcessHeap(), 0, result);
        for (i=0; i<8; i++)
            HeapFree(GetProcessHeap(), 0, dyn_names[i]);
        HeapFree(GetProcessHeap(), 0, data);
        return E_OUTOFMEMORY;
    }

    for (i=0; i<8; i++)
    {
        PropVariantInit(&result[i].schema);

        PropVariantInit(&result[i].id);
        result[i].id.vt = VT_LPWSTR;
        result[i].id.pwszVal = dyn_names[i];
        lstrcpyW(dyn_names[i], names[i]);

        PropVariantInit(&result[i].value);
        result[i].value.vt = VT_UI4;
        result[i].value.ulVal = read_ulong_be(&data[i*4]);
    }

    *items = result;
    *item_count = 8;

    HeapFree(GetProcessHeap(), 0, data);

    return S_OK;
}

static const MetadataHandlerVtbl ChrmReader_Vtbl = {
    0,
    &CLSID_WICPngChrmMetadataReader,
    LoadChrmMetadata
};

HRESULT PngChrmReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&ChrmReader_Vtbl, iid, ppv);
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
