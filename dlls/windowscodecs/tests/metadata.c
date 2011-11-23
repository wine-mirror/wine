/*
 * Copyright 2011 Vincent Povirk for CodeWeavers
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

#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "wine/test.h"

#define expect_blob(propvar, data, length) do { \
    ok((propvar).vt == VT_BLOB, "unexpected vt: %i\n", (propvar).vt); \
    if ((propvar).vt == VT_BLOB) { \
        ok(U(propvar).blob.cbSize == (length), "expected size %u, got %u\n", (ULONG)(length), U(propvar).blob.cbSize); \
        if (U(propvar).blob.cbSize == (length)) { \
            ok(!memcmp(U(propvar).blob.pBlobData, (data), (length)), "unexpected data\n"); \
        } \
    } \
} while (0)

static const char metadata_unknown[] = "lalala";

static const char metadata_tEXt[] = {
    0,0,0,14, /* chunk length */
    't','E','X','t', /* chunk type */
    'w','i','n','e','t','e','s','t',0, /* keyword */
    'v','a','l','u','e', /* text */
    0x3f,0x64,0x19,0xf3 /* chunk CRC */
};

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    if(!riid)
        return "(null)";

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static IStream *create_stream(const char *data, int data_size)
{
    HRESULT hr;
    IStream *stream;
    HGLOBAL hdata;
    void *locked_data;

    hdata = GlobalAlloc(GMEM_MOVEABLE, data_size);
    ok(hdata != 0, "GlobalAlloc failed\n");
    if (!hdata) return NULL;

    locked_data = GlobalLock(hdata);
    memcpy(locked_data, data, data_size);
    GlobalUnlock(hdata);

    hr = CreateStreamOnHGlobal(hdata, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed, hr=%x\n", hr);

    return stream;
}

static void load_stream(IUnknown *reader, const char *data, int data_size)
{
    HRESULT hr;
    IWICPersistStream *persist;
    IStream *stream;

    stream = create_stream(data, data_size);
    if (!stream)
        return;

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void**)&persist);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICPersistStream_LoadEx(persist, stream, NULL, WICPersistOptionsDefault);
        ok(hr == S_OK, "LoadEx failed, hr=%x\n", hr);

        IWICPersistStream_Release(persist);
    }

    IStream_Release(stream);
}

static void test_metadata_unknown(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    ULONG items_returned;

    hr = CoCreateInstance(&CLSID_WICUnknownMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    todo_wine ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    load_stream((IUnknown*)reader, metadata_unknown, sizeof(metadata_unknown));

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        PropVariantInit(&schema);
        PropVariantInit(&id);
        PropVariantInit(&value);

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next failed, hr=%x\n", hr);
        ok(items_returned == 1, "unexpected item count %i\n", items_returned);

        if (hr == S_OK && items_returned == 1)
        {
            ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
            ok(id.vt == VT_EMPTY, "unexpected vt: %i\n", id.vt);
            expect_blob(value, metadata_unknown, sizeof(metadata_unknown));

            PropVariantClear(&schema);
            PropVariantClear(&id);
            PropVariantClear(&value);
        }

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_FALSE, "Next failed, hr=%x\n", hr);
        ok(items_returned == 0, "unexpected item count %i\n", items_returned);

        IWICEnumMetadataItem_Release(enumerator);
    }

    IWICMetadataReader_Release(reader);
}

static void test_metadata_tEXt(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    ULONG items_returned, count;
    GUID format;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngTextMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    todo_wine ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 0, "unexpected count %i\n", count);

    load_stream((IUnknown*)reader, metadata_tEXt, sizeof(metadata_tEXt));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 1, "unexpected count %i\n", count);

    hr = IWICMetadataReader_GetEnumerator(reader, NULL);
    ok(hr == E_INVALIDARG, "GetEnumerator failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next failed, hr=%x\n", hr);
        ok(items_returned == 1, "unexpected item count %i\n", items_returned);

        if (hr == S_OK && items_returned == 1)
        {
            ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
            ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
            ok(!strcmp(U(id).pszVal, "winetest"), "unexpected id: %s\n", U(id).pszVal);
            ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
            ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(id).pszVal);

            PropVariantClear(&schema);
            PropVariantClear(&id);
            PropVariantClear(&value);
        }

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_FALSE, "Next failed, hr=%x\n", hr);
        ok(items_returned == 0, "unexpected item count %i\n", items_returned);

        IWICEnumMetadataItem_Release(enumerator);
    }

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "unexpected format %s\n", debugstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataFormat(reader, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataFormat failed, hr=%x\n", hr);

    id.vt = VT_LPSTR;
    U(id).pszVal = CoTaskMemAlloc(strlen("winetest") + 1);
    strcpy(U(id).pszVal, "winetest");

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, &value);
    ok(hr == E_INVALIDARG, "GetValue failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(id).pszVal);
    PropVariantClear(&value);

    strcpy(U(id).pszVal, "test");

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "GetValue failed, hr=%x\n", hr);

    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);
    ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(id).pszVal, "winetest"), "unexpected id: %s\n", U(id).pszVal);
    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(id).pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, 1, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetValueByIndex failed, hr=%x\n", hr);

    IWICMetadataReader_Release(reader);
}

static void test_create_reader(void)
{
    HRESULT hr;
    IWICComponentFactory *factory;
    IStream *stream;
    IWICMetadataReader *reader;
    UINT count=0;
    GUID format;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICComponentFactory, (void**)&factory);
    todo_wine ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    stream = create_stream(metadata_tEXt, sizeof(metadata_tEXt));

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionsDefault,
        stream, &reader);
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %i\n", count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "unexpected format %s\n", debugstr_guid(&format));

        IWICMetadataReader_Release(reader);
    }

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatWmp, NULL, WICPersistOptionsDefault,
        stream, &reader);
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %i\n", count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "unexpected format %s\n", debugstr_guid(&format));

        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);

    IWICComponentFactory_Release(factory);
}

START_TEST(metadata)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_metadata_unknown();
    test_metadata_tEXt();
    test_create_reader();

    CoUninitialize();
}
