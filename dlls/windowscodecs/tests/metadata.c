/*
 * Copyright 2011 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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

#define IFD_SHORT 3
#define IFD_LONG 4
#define IFD_RATIONAL 5

#include "pshpack2.h"
struct IFD_entry
{
    SHORT id;
    SHORT type;
    ULONG length;
    LONG  value;
};

struct IFD_rational
{
    LONG numerator;
    LONG denominator;
};

static const struct
{
    USHORT number_of_entries;
    struct IFD_entry entry[6];
    ULONG next_IFD;
    struct IFD_rational xres;
} IFD_data =
{
    6,
    {
        { 0xfe,  IFD_SHORT, 1, 1 }, /* NEWSUBFILETYPE */
        { 0x100, IFD_LONG, 1, 222 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 333 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 24 }, /* BITSPERSAMPLE */
        { 0x103, IFD_LONG, 1, 32773 }, /* COMPRESSION: packbits */
        { 0x11a, IFD_RATIONAL, 1, /* XRESOLUTION */
          sizeof(USHORT) + sizeof(struct IFD_entry) * 6 + sizeof(ULONG) }
    },
    0,
    { 900, 3 }
};
#include "poppack.h"

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
    IWICMetadataBlockReader *blockreader;
    PROPVARIANT schema, id, value;
    ULONG items_returned;

    hr = CoCreateInstance(&CLSID_WICUnknownMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
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

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    todo_wine ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
        IWICMetadataBlockReader_Release(blockreader);

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
            ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", value.vt);
            ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(value).pszVal);

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
    ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(value).pszVal);
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
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", value.vt);
    ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(value).pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, 1, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetValueByIndex failed, hr=%x\n", hr);

    IWICMetadataReader_Release(reader);
}

static void test_metadata_IFD(void)
{
    static const struct test_data
    {
        ULONG type, id;
        LONGLONG value;
    } td[6] =
    {
        { VT_UI2, 0xfe, 1 },
        { VT_UI4, 0x100, 222 },
        { VT_UI4, 0x101, 333 },
        { VT_UI2, 0x102, 24 },
        { VT_UI4, 0x103, 32773 },
        { VT_UI8, 0x11a,  ((LONGLONG)3 << 32) | 900 }
    };
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICMetadataBlockReader *blockreader;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    ULONG items_returned, count, i;
    GUID format;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICIfdMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    todo_wine ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount error %#x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    load_stream((IUnknown*)reader, (const char *)&IFD_data, sizeof(IFD_data));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == 6, "unexpected count %u\n", count);

    hr = IWICMetadataReader_GetEnumerator(reader, NULL);
    ok(hr == E_INVALIDARG, "GetEnumerator error %#x\n", hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator error %#x\n", hr);

    for (i = 0; i < count; i++)
    {
        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next error %#x\n", hr);
        ok(items_returned == 1, "unexpected item count %u\n", items_returned);

        ok(schema.vt == VT_EMPTY, "%u: unexpected vt: %u\n", i, schema.vt);
        ok(id.vt == VT_UI2, "%u: unexpected vt: %u\n", i, id.vt);
        ok(U(id).uiVal == td[i].id, "%u: unexpected id: %#x\n", i, U(id).uiVal);
        ok(value.vt == td[i].type, "%u: unexpected vt: %u\n", i, value.vt);
        ok(U(value).uhVal.QuadPart == td[i].value, "%u: unexpected id: %d/%d\n", i, U(value).uhVal.LowPart, U(value).uhVal.HighPart);

        PropVariantClear(&schema);
        PropVariantClear(&id);
        PropVariantClear(&value);
    }

    hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
    ok(hr == S_FALSE, "Next should fail\n");
    ok(items_returned == 0, "unexpected item count %u\n", items_returned);

    IWICEnumMetadataItem_Release(enumerator);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "unexpected format %s\n", debugstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataFormat(reader, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataFormat should fail\n");

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, count - 1, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %u\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, count - 1, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %u\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(id.vt == VT_UI2, "unexpected vt: %u\n", id.vt);
    ok(U(id).uiVal == 0xfe, "unexpected id: %#x\n", U(id).uiVal);
    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(value.vt == VT_UI2, "unexpected vt: %u\n", value.vt);
    ok(U(value).ulVal == 1, "unexpected id: %u\n", U(value).ulVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, count, &schema, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetValueByIndex should fail\n");

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
        IWICMetadataBlockReader_Release(blockreader);

    IWICMetadataReader_Release(reader);
}

static void test_metadata_Exif(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICMetadataBlockReader *blockreader;
    UINT count=0;

    hr = CoCreateInstance(&CLSID_WICExifMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    todo_wine ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount error %#x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
        IWICMetadataBlockReader_Release(blockreader);

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
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

    stream = create_stream(metadata_tEXt, sizeof(metadata_tEXt));

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionsDefault,
        stream, &reader);
todo_wine
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

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
    test_metadata_IFD();
    test_metadata_Exif();
    test_create_reader();

    CoUninitialize();
}
