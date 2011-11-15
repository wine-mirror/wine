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
        ok((propvar).blob.cbSize == (length), "expected size %i, got %i\n", (length), (propvar).blob.cbSize); \
        if ((propvar).blob.cbSize == (length)) { \
            ok(!memcmp((propvar).blob.pBlobData, (data), (length)), "unexpected data\n"); \
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

static void load_stream(IUnknown *reader, const char *data, int data_size)
{
    HRESULT hr;
    IWICPersistStream *persist;
    IStream *stream;
    HGLOBAL hdata;
    void *locked_data;

    hdata = GlobalAlloc(GMEM_MOVEABLE, data_size);
    ok(hdata != 0, "GlobalAlloc failed\n");
    if (!hdata) return;

    locked_data = GlobalLock(hdata);
    memcpy(locked_data, data, data_size);
    GlobalUnlock(hdata);

    hr = CreateStreamOnHGlobal(hdata, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

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
    ULONG items_returned;

    hr = CoCreateInstance(&CLSID_WICPngTextMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    todo_wine ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    load_stream((IUnknown*)reader, metadata_tEXt, sizeof(metadata_tEXt));

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
            ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
            ok(!strcmp(id.pszVal, "winetest"), "unexpected id: %s\n", id.pszVal);
            ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
            ok(!strcmp(value.pszVal, "value"), "unexpected value: %s\n", id.pszVal);

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

START_TEST(metadata)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_metadata_unknown();
    test_metadata_tEXt();

    CoUninitialize();
}
