/*
 * Copyright 2011 Vincent Povirk for CodeWeavers
 * Copyright 2012,2017 Dmitry Timoshkov
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
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "propvarutil.h"
#include "wine/test.h"

#define expect_ref(obj,ref) expect_ref_((IUnknown *)obj, ref, __LINE__)
static void expect_ref_(IUnknown *obj, ULONG ref, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == ref, "Expected refcount %ld, got %ld.\n", ref, refcount);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

#define check_persist_options(a, b) check_persist_options_(__LINE__, a, b)
static void check_persist_options_(unsigned int line, void *iface_ptr, DWORD expected_options)
{
    IWICStreamProvider *stream_provider;
    IUnknown *iface = iface_ptr;
    DWORD options;
    HRESULT hr;

    if (SUCCEEDED(IUnknown_QueryInterface(iface, &IID_IWICStreamProvider, (void **)&stream_provider)))
    {
        hr = IWICStreamProvider_GetPersistOptions(stream_provider, &options);
        ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            ok_(__FILE__, line)(options == expected_options, "Unexpected options %#lx.\n", options);
        IWICStreamProvider_Release(stream_provider);
    }
    else
        ok_(__FILE__, line)(0, "IWICStreamProvider is not supported.\n");
}

static HRESULT get_persist_stream(void *iface_ptr, IStream **stream)
{
    IWICStreamProvider *stream_provider;
    IUnknown *iface = iface_ptr;
    HRESULT hr = E_UNEXPECTED;

    if (SUCCEEDED(IUnknown_QueryInterface(iface, &IID_IWICStreamProvider, (void **)&stream_provider)))
    {
        hr = IWICStreamProvider_GetStream(stream_provider, stream);
        IWICStreamProvider_Release(stream_provider);
    }

    return hr;
}

static HRESULT create_query_reader_from_metadata_reader(IWICComponentFactory *factory, IWICMetadataReader *metadata_reader,
        const GUID *container_format, IWICMetadataQueryReader **query_reader);

#define compare_blob(a,b,c) compare_blob_(__LINE__,a,b,c)
static void compare_blob_(unsigned int line, const PROPVARIANT *propvar, const char *data, ULONG length)
{
    ok_(__FILE__, line)(propvar->vt == VT_BLOB, "Unexpected vt: %i\n", propvar->vt);
    if (propvar->vt == VT_BLOB)
    {
        ok_(__FILE__, line)(propvar->blob.cbSize == length, "Expected size %lu, got %lu.\n", length, propvar->blob.cbSize);
        if (propvar->blob.cbSize == length)
            ok_(__FILE__, line)(!memcmp(propvar->blob.pBlobData, data, length), "Unexpected data.\n");
    }
}

static void test_block_reader_enumerator(IWICMetadataBlockReader *block_reader)
{
    IEnumUnknown *block_enum, *block_enum2;
    IUnknown *object, *object2;
    UINT block_count;
    ULONG fetched;
    HRESULT hr;

    hr = IWICMetadataBlockReader_GetEnumerator(block_reader, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataBlockReader_GetEnumerator(block_reader, &block_enum);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataBlockReader_GetEnumerator(block_reader, &block_enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(block_enum != block_enum2, "Unexpected instance.\n");

    hr = IWICMetadataBlockReader_GetCount(block_reader, &block_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(block_count > 0, "Unexpected block count %u.\n", block_count);

    fetched = 0;
    hr = IEnumUnknown_Next(block_enum, 1, &object, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);

    fetched = 0;
    hr = IEnumUnknown_Next(block_enum2, 1, &object2, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    ok(object == object2, "Unexpected instance.\n");
    IUnknown_Release(object2);

    hr = IEnumUnknown_Reset(block_enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    fetched = 0;
    hr = IEnumUnknown_Next(block_enum2, 1, &object2, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    ok(object == object2, "Unexpected instance.\n");
    IUnknown_Release(object2);

    IEnumUnknown_Release(block_enum2);

    /* Cloning. */
    hr = IEnumUnknown_Clone(block_enum, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IEnumUnknown_Clone(block_enum, &block_enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* The cloned object inherits cursor value. */
    fetched = 0;
    hr = IEnumUnknown_Next(block_enum2, 1, &object2, &fetched);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(fetched == 1, "Unexpected count %lu.\n", fetched);
        ok(object != object2, "Unexpected instance.\n");
        IUnknown_Release(object2);
    }
    else
    {
        ok(!fetched, "Unexpected count %lu.\n", fetched);
    }
    IEnumUnknown_Release(block_enum2);

    /* Skipping. */
    hr = IEnumUnknown_Skip(block_enum, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Reset(block_enum);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Skip(block_enum, block_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Reset(block_enum);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Skip(block_enum, block_count + 1);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Skip(block_enum, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    fetched = 0;
    hr = IEnumUnknown_Next(block_enum, 1, &object2, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(!fetched, "Unexpected count %lu.\n", fetched);
    hr = IEnumUnknown_Reset(block_enum);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Skip(block_enum, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    fetched = 0;
    hr = IEnumUnknown_Next(block_enum, 1, &object2, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    ok(object == object2, "Unexpected instance.\n");
    IUnknown_Release(object2);

    IUnknown_Release(object);
    IEnumUnknown_Release(block_enum);
}

enum ifd_entry_type
{
    IFD_BYTE = 1,
    IFD_ASCII = 2,
    IFD_SHORT = 3,
    IFD_LONG = 4,
    IFD_RATIONAL = 5,
    IFD_SBYTE = 6,
    IFD_UNDEFINED = 7,
    IFD_SSHORT = 8,
    IFD_SLONG = 9,
    IFD_SRATIONAL = 10,
    IFD_FLOAT = 11,
    IFD_DOUBLE = 12,
    IFD_IFD = 13,
};

#include "pshpack2.h"
struct IFD_entry
{
    SHORT id;
    SHORT type;
    ULONG count;
    LONG  value;
};

struct IFD_rational
{
    LONG numerator;
    LONG denominator;
};

static const struct ifd_data
{
    USHORT number_of_entries;
    struct IFD_entry entry[40];
    ULONG next_IFD;
    struct IFD_rational xres;
    DOUBLE double_val;
    struct IFD_rational srational_val;
    char string[14];
    SHORT short_val[4];
    LONG long_val[2];
    FLOAT float_val[2];
    struct IFD_rational rational[3];
} IFD_data =
{
    28,
    {
        { 0xfe,  IFD_SHORT, 1, 1 }, /* NEWSUBFILETYPE */
        { 0x100, IFD_LONG, 1, 222 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 333 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 24 }, /* BITSPERSAMPLE */
        { 0x103, IFD_LONG, 1, 32773 }, /* COMPRESSION: packbits */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct ifd_data, xres) },
        { 0xf001, IFD_BYTE, 1, 0x11223344 },
        { 0xf002, IFD_BYTE, 4, 0x11223344 },
        { 0xf003, IFD_SBYTE, 1, 0x11223344 },
        { 0xf004, IFD_SSHORT, 1, 0x11223344 },
        { 0xf005, IFD_SSHORT, 2, 0x11223344 },
        { 0xf006, IFD_SLONG, 1, 0x11223344 },
        { 0xf007, IFD_FLOAT, 1, 0x11223344 },
        { 0xf008, IFD_DOUBLE, 1, FIELD_OFFSET(struct ifd_data, double_val) },
        { 0xf009, IFD_SRATIONAL, 1, FIELD_OFFSET(struct ifd_data, srational_val) },
        { 0xf00a, IFD_BYTE, 13, FIELD_OFFSET(struct ifd_data, string) },
        { 0xf00b, IFD_SSHORT, 4, FIELD_OFFSET(struct ifd_data, short_val) },
        { 0xf00c, IFD_SLONG, 2, FIELD_OFFSET(struct ifd_data, long_val) },
        { 0xf00d, IFD_FLOAT, 2, FIELD_OFFSET(struct ifd_data, float_val) },
        { 0xf00e, IFD_ASCII, 13, FIELD_OFFSET(struct ifd_data, string) },
        { 0xf00f, IFD_ASCII, 4, 'a' | 'b' << 8 | 'c' << 16 | 'd' << 24 },
        { 0xf010, IFD_UNDEFINED, 13, FIELD_OFFSET(struct ifd_data, string) },
        { 0xf011, IFD_UNDEFINED, 4, 'a' | 'b' << 8 | 'c' << 16 | 'd' << 24 },
        { 0xf012, IFD_BYTE, 0, 0x11223344 },
        { 0xf013, IFD_SHORT, 0, 0x11223344 },
        { 0xf014, IFD_LONG, 0, 0x11223344 },
        { 0xf015, IFD_FLOAT, 0, 0x11223344 },
        { 0xf016, IFD_SRATIONAL, 3, FIELD_OFFSET(struct ifd_data, rational) },
    },
    0,
    { 900, 3 },
    1234567890.0987654321,
    { 0x1a2b3c4d, 0x5a6b7c8d },
    "Hello World!",
    { 0x0101, 0x0202, 0x0303, 0x0404 },
    { 0x11223344, 0x55667788 },
    { (FLOAT)1234.5678, (FLOAT)8765.4321 },
    { { 0x01020304, 0x05060708 }, { 0x10203040, 0x50607080 }, { 0x11223344, 0x55667788 } },
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

static const char metadata_gAMA[] = {
    0,0,0,4, /* chunk length */
    'g','A','M','A', /* chunk type */
    0,0,130,53, /* gamma */
    0xff,0xff,0xff,0xff /* chunk CRC */
};

static const char metadata_cHRM[] = {
    0,0,0,32, /* chunk length */
    'c','H','R','M', /* chunk type */
    0,0,122,40, 0,0,120,132, /* white point */
    0,0,240,0, 0,0,128,220, /* red */
    0,0,100,48, 0,0,234,100, /* green */
    0,0,58,140, 0,0,23,100, /* blue */
    0xff,0xff,0xff,0xff /* chunk CRC */
};

static const char metadata_hIST[] = {
    0,0,0,40, /* chunk length */
    'h','I','S','T', /* chunk type */
    0,1,  0,2,  0,3,  0,4,
    0,5,  0,6,  0,7,  0,8,
    0,9,  0,10, 0,11, 0,12,
    0,13, 0,14, 0,15, 0,16,
    0,17, 0,18, 0,19, 0,20,
    0xff,0xff,0xff,0xff
};

static const char metadata_tIME[] = {
    0,0,0,7, /* chunk length */
    't','I','M','E', /* chunk type */
    0x07,0xd0,0x01,0x02, /* year (2 bytes), month, day */
    0x0c,0x22,0x38, /* hour, minute, second */
    0xff,0xff,0xff,0xff
};

static const char pngimage[285] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
0xde,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,
0x13,0x01,0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd5,
0x06,0x03,0x0f,0x07,0x2d,0x12,0x10,0xf0,0xfd,0x00,0x00,0x00,0x0c,0x49,0x44,0x41,
0x54,0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,
0xe7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

/* 1x1 pixel gif */
static const char gifimage[35] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
0x01,0x00,0x3b
};

/* 1x1 pixel gif, 2 frames; first frame is white, second is black */
static const char animatedgif[] = {
'G','I','F','8','9','a',0x01,0x00,0x01,0x00,0xA1,0x00,0x00,
0x6F,0x6F,0x6F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0x21,0xFF,0x0B,'N','E','T','S','C','A','P','E','2','.','0',*/
0x21,0xFF,0x0B,'A','N','I','M','E','X','T','S','1','.','0',
0x03,0x01,0x05,0x00,0x00,
0x21,0xFE,0x0C,'H','e','l','l','o',' ','W','o','r','l','d','!',0x00,
0x21,0x01,0x0D,'a','n','i','m','a','t','i','o','n','.','g','i','f',0x00,
0x21,0xF9,0x04,0x00,0x0A,0x00,0xFF,0x00,0x2C,
0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
0xDE,0xDE,0xDE,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x4C,0x01,0x00,
0x21,0xF9,0x04,0x01,0x0A,0x00,0x01,0x00,
0x21,0xFE,0x08,'i','m','a','g','e',' ','#','1',0x00,
0x21,0x01,0x0C,'p','l','a','i','n','t','e','x','t',' ','#','1',0x00,
0x2C,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
0x4D,0x4D,0x4D,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x44,0x01,0x00,
0x21,0xFE,0x08,'i','m','a','g','e',' ','#','2',0x00,
0x21,0x01,0x0C,'p','l','a','i','n','t','e','x','t',' ','#','2',0x00,0x3B
};

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
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
    ok(hr == S_OK, "CreateStreamOnHGlobal failed, hr=%lx\n", hr);

    return stream;
}

static void load_stream(void *iface_ptr, const char *data, int data_size, DWORD persist_options)
{
    IWICStreamProvider *stream_provider;
    IUnknown *iface = iface_ptr;
    HRESULT hr;
    IWICPersistStream *persist;
    IStream *stream, *stream2;
    LARGE_INTEGER pos;
    ULARGE_INTEGER cur_pos;
    DWORD flags;
    GUID guid;

    stream = create_stream(data, data_size);
    if (!stream)
        return;

    hr = IUnknown_QueryInterface(iface, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    hr = IWICPersistStream_LoadEx(persist, NULL, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(iface, &IID_IWICStreamProvider, (void **)&stream_provider);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&guid, 0, sizeof(guid));
    hr = IWICStreamProvider_GetPreferredVendorGUID(stream_provider, &guid);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &GUID_VendorMicrosoft), "Unexpected vendor %s.\n", wine_dbgstr_guid(&guid));

    hr = IWICStreamProvider_GetPreferredVendorGUID(stream_provider, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICStreamProvider_GetPersistOptions(stream_provider, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    flags = 123;
    hr = IWICStreamProvider_GetPersistOptions(stream_provider, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!flags, "Unexpected options %#lx.\n", flags);

    hr = IWICStreamProvider_GetStream(stream_provider, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    stream2 = (void *)0xdeadbeef;
    hr = IWICStreamProvider_GetStream(stream_provider, &stream2);
    ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
    ok(stream2 == (void *)0xdeadbeef, "Unexpected stream pointer.\n");

    hr = IWICPersistStream_LoadEx(persist, stream, NULL, persist_options);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&guid, 0, sizeof(guid));
    hr = IWICStreamProvider_GetPreferredVendorGUID(stream_provider, &guid);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &GUID_VendorMicrosoft), "Unexpected vendor %s.\n", wine_dbgstr_guid(&guid));

    flags = ~persist_options;
    hr = IWICStreamProvider_GetPersistOptions(stream_provider, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == persist_options, "Unexpected options %#lx.\n", flags);

    if (persist_options & WICPersistOptionNoCacheStream)
    {
        stream2 = (void *)0xdeadbeef;
        hr = IWICStreamProvider_GetStream(stream_provider, &stream2);
        ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
        ok(stream2 == (void *)0xdeadbeef, "Unexpected stream pointer.\n");
    }
    else
    {
        stream2 = NULL;
        hr = IWICStreamProvider_GetStream(stream_provider, &stream2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(stream2 == stream, "Unexpected stream pointer.\n");
        if (stream2)
            IStream_Release(stream2);
    }

    IWICStreamProvider_Release(stream_provider);
    IWICPersistStream_Release(persist);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &cur_pos);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);
    /* IFD metadata reader doesn't rewind the stream to the start */
    ok(cur_pos.QuadPart == 0 || cur_pos.QuadPart <= data_size,
       "current stream pos is at %lx/%lx, data size %x\n", cur_pos.u.LowPart, cur_pos.u.HighPart, data_size);

    IStream_Release(stream);
}

struct test_data
{
    ULONG type, id;
    int count; /* if VT_VECTOR */
    LONGLONG value[13];
    const char *string;
    const WCHAR id_string[32];
};

static void compare_metadata(IWICMetadataReader *reader, const struct test_data *td, ULONG count)
{
    HRESULT hr;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    ULONG items_returned, i;

    hr = IWICMetadataReader_GetEnumerator(reader, NULL);
    ok(hr == E_INVALIDARG, "GetEnumerator error %#lx\n", hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator error %#lx\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    for (i = 0; i < count; i++)
    {
        winetest_push_context("%lu", i);

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next error %#lx\n", hr);
        ok(items_returned == 1, "unexpected item count %lu\n", items_returned);

        ok(schema.vt == VT_EMPTY, "Unexpected vt: %u\n", schema.vt);
        ok(id.vt == VT_UI2 || id.vt == VT_LPWSTR || id.vt == VT_EMPTY, "Unexpected vt: %u\n", id.vt);
        if (id.vt == VT_UI2)
            ok(id.uiVal == td[i].id, "Expected id %#lx, got %#x\n", td[i].id, id.uiVal);
        else if (id.vt == VT_LPWSTR)
            ok(!lstrcmpW(td[i].id_string, id.pwszVal),
               "Expected %s, got %s\n", wine_dbgstr_w(td[i].id_string), wine_dbgstr_w(id.pwszVal));

        ok(value.vt == td[i].type, "Expected vt %#lx, got %#x\n", td[i].type, value.vt);
        if (value.vt & VT_VECTOR)
        {
            ULONG j;
            switch (value.vt & ~VT_VECTOR)
            {
            case VT_I1:
            case VT_UI1:
                ok(td[i].count == value.caub.cElems, "Expected cElems %d, got %ld\n", td[i].count, value.caub.cElems);
                for (j = 0; j < value.caub.cElems; j++)
                    ok(td[i].value[j] == value.caub.pElems[j], "Expected value[%ld] %#I64x, got %#x\n", j, td[i].value[j], value.caub.pElems[j]);
                break;
            case VT_I2:
            case VT_UI2:
                ok(td[i].count == value.caui.cElems, "Expected cElems %d, got %ld\n", td[i].count, value.caui.cElems);
                for (j = 0; j < value.caui.cElems; j++)
                    ok(td[i].value[j] == value.caui.pElems[j], "Expected value[%ld] %#I64x, got %#x\n", j, td[i].value[j], value.caui.pElems[j]);
                break;
            case VT_I4:
            case VT_UI4:
            case VT_R4:
                ok(td[i].count == value.caul.cElems, "Expected cElems %d, got %ld\n", td[i].count, value.caul.cElems);
                for (j = 0; j < value.caul.cElems; j++)
                    ok(td[i].value[j] == value.caul.pElems[j], "Expected value[%ld] %#I64x, got %#lx\n", j, td[i].value[j], value.caul.pElems[j]);
                break;
            case VT_I8:
            case VT_UI8:
            case VT_R8:
                ok(td[i].count == value.cauh.cElems, "Expected cElems %d, got %ld\n", td[i].count, value.cauh.cElems);
                for (j = 0; j < value.cauh.cElems; j++)
                    ok(td[i].value[j] == value.cauh.pElems[j].QuadPart, "Expected value[%ld] %I64x, got %08lx/%08lx\n", j, td[i].value[j], value.cauh.pElems[j].u.LowPart, value.cauh.pElems[j].u.HighPart);
                break;
            case VT_LPSTR:
                ok(td[i].count == value.calpstr.cElems, "Expected cElems %d, got %ld\n", td[i].count, value.caub.cElems);
                for (j = 0; j < value.calpstr.cElems; j++)
                    trace("%lu: %s\n", j, value.calpstr.pElems[j]);
                /* fall through to not handled message */
            default:
                ok(0, "vector of type %d is not handled\n", value.vt & ~VT_VECTOR);
                break;
            }
        }
        else if (value.vt == VT_LPSTR)
        {
            ok(td[i].count == strlen(value.pszVal) ||
               broken(td[i].count == strlen(value.pszVal) + 1), /* before Win7 */
                   "Expected count %d, got %d\n", td[i].count, lstrlenA(value.pszVal));
            if (td[i].count == strlen(value.pszVal))
                ok(!strcmp(td[i].string, value.pszVal),
                   "Expected %s, got %s\n", td[i].string, value.pszVal);
        }
        else if (value.vt == VT_BLOB)
        {
            ok(td[i].count == value.blob.cbSize, "Expected count %d, got %ld\n", td[i].count, value.blob.cbSize);
            ok(!memcmp(td[i].string, value.blob.pBlobData, td[i].count), "Expected %s, got %s\n", td[i].string, value.blob.pBlobData);
        }
        else
            ok(value.uhVal.QuadPart == td[i].value[0], "Eexpected value %#I64x got %#lx/%#lx\n",
               td[i].value[0], value.uhVal.u.LowPart, value.uhVal.u.HighPart);

        PropVariantClear(&schema);
        PropVariantClear(&id);
        PropVariantClear(&value);

        winetest_pop_context();
    }

    hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
    ok(hr == S_FALSE, "Next should fail\n");
    ok(items_returned == 0, "unexpected item count %lu\n", items_returned);

    IWICEnumMetadataItem_Release(enumerator);
}

#define test_reader_container_format(a, b) _test_reader_container_format(a, b, __LINE__)
static void _test_reader_container_format(IWICMetadataReader *reader, const GUID *format, unsigned int line)
{
    IWICMetadataHandlerInfo *info;
    BOOL found = FALSE;
    GUID formats[8];
    UINT count;
    HRESULT hr;

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataHandlerInfo_GetContainerFormats(info, ARRAY_SIZE(formats), formats, &count);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(count > 0, "Unexpected count.\n");

    for (unsigned i = 0; i < count; ++i)
    {
        if (IsEqualGUID(&formats[i], format))
        {
            found = TRUE;
            break;
        }
    }
    ok_(__FILE__, line)(found, "Container format %s was not found.\n", wine_dbgstr_guid(format));

    if (!found)
    {
        for (unsigned i = 0; i < count; ++i)
            ok_(__FILE__, line)(0, "Available format %s.\n", wine_dbgstr_guid(&formats[i]));
    }

    IWICMetadataHandlerInfo_Release(info);
}

static void test_metadata_unknown(void)
{
    IWICMetadataQueryReader *query_reader, *query_reader2;
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    IWICComponentFactory *factory;
    PROPVARIANT schema, id, value;
    IWICMetadataWriter *writer;
    IWICPersistStream *persist;
    IEnumString *enum_string;
    IStream *stream;
    ULONG fetched;
    WCHAR *str;
    UINT count;

    hr = CoCreateInstance(&CLSID_WICUnknownMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);
    check_interface(reader, &IID_IWICMetadataBlockReader, FALSE);

    load_stream(reader, metadata_unknown, sizeof(metadata_unknown), WICPersistOptionDefault);

    /* Item enumerator. */
    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator failed, hr=%lx\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &fetched);
    ok(hr == S_OK, "Next failed, hr=%lx\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);

    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
    ok(id.vt == VT_EMPTY, "unexpected vt: %i\n", id.vt);
    compare_blob(&value, metadata_unknown, sizeof(metadata_unknown));

    PropVariantClear(&schema);
    PropVariantClear(&id);
    PropVariantClear(&value);

    hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &fetched);
    ok(hr == S_FALSE, "Next failed, hr=%lx\n", hr);
    ok(!fetched, "Unexpected count %lu.\n", fetched);

    hr = IWICEnumMetadataItem_Reset(enumerator);
    ok(hr == S_OK, "Reset failed, hr=%lx\n", hr);

    hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, NULL, NULL);
    ok(hr == S_OK, "Next failed, hr=%lx\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
    ok(id.vt == VT_EMPTY, "unexpected vt: %i\n", id.vt);
    PropVariantClear(&schema);
    PropVariantClear(&id);

    IWICEnumMetadataItem_Release(enumerator);

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    hr = get_persist_stream(reader, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IStream_Release(stream);

    hr = IWICPersistStream_LoadEx(persist, NULL, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = get_persist_stream(reader, &stream);
    ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
    check_persist_options(reader, 0);

    hr = IWICPersistStream_LoadEx(persist, NULL, NULL, WICPersistOptionNoCacheStream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(reader, WICPersistOptionNoCacheStream);
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    IWICPersistStream_Release(persist);

    /* Query reader. */
    load_stream(reader, metadata_unknown, sizeof(metadata_unknown), 0);

    hr = create_query_reader_from_metadata_reader(factory, reader, &GUID_NULL, &query_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/unknown", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %d.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    hr = IWICMetadataQueryReader_GetEnumerator(query_reader2, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = NULL;
    hr = IEnumString_Next(enum_string, 1, &str, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    ok(!wcscmp(str, L"/{}"), "Unexpected query %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
    IEnumString_Release(enum_string);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader2, L"/{}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    compare_blob(&value, metadata_unknown, sizeof(metadata_unknown));
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/unknown/{}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    compare_blob(&value, metadata_unknown, sizeof(metadata_unknown));
    PropVariantClear(&value);

    IWICMetadataQueryReader_Release(query_reader2);
    IWICMetadataQueryReader_Release(query_reader);

    IWICMetadataReader_Release(reader);

    /* "Unknown" writer. */
    hr = CoCreateInstance(&CLSID_WICUnknownMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, &schema, &id, &value);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(schema.vt == VT_EMPTY, "Unexpected type %d.\n", schema.vt);
        ok(id.vt == VT_EMPTY, "Unexpected type %d.\n", id.vt);
        ok(value.vt == VT_BLOB, "Unexpected type %d.\n", id.vt);
        ok(!value.blob.cbSize, "Unexpected size %lu.\n", value.blob.cbSize);
        ok(!value.blob.pBlobData, "Unexpected data pointer %p.\n", value.blob.pBlobData);
    }

    PropVariantInit(&schema);
    PropVariantInit(&value);

    id.vt = VT_EMPTY;
    V_UI4(&id) = 123;
    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&id);
    PropVariantInit(&schema);
    hr = IWICMetadataWriter_RemoveValue(writer, &schema, &id);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);

    id.vt = VT_UI4;
    hr = IWICMetadataWriter_RemoveValue(writer, &schema, &id);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_RemoveValueByIndex(writer, 0);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    load_stream(writer, metadata_unknown, sizeof(metadata_unknown), 0);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, NULL, NULL, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    compare_blob(&value, metadata_unknown, sizeof(metadata_unknown));
    PropVariantClear(&value);

    load_stream(writer, metadata_unknown, sizeof(metadata_unknown), 0);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, NULL, NULL, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    compare_blob(&value, metadata_unknown, sizeof(metadata_unknown));
    PropVariantClear(&value);

    load_stream(writer, metadata_unknown, sizeof(metadata_unknown), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
    IWICComponentFactory_Release(factory);
}

static void test_metadata_tEXt(void)
{
    IWICMetadataHandlerInfo *info;
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    IWICMetadataWriter *writer;
    ULONG items_returned;
    UINT count;
    GUID format;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngTextMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount failed, hr=%lx\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 0, "unexpected count %i\n", count);

    load_stream(reader, metadata_tEXt, sizeof(metadata_tEXt), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    todo_wine
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IWICMetadataHandlerInfo_Release(info);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 1, "unexpected count %i\n", count);

    hr = IWICMetadataReader_GetEnumerator(reader, NULL);
    ok(hr == E_INVALIDARG, "GetEnumerator failed, hr=%lx\n", hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator failed, hr=%lx\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next failed, hr=%lx\n", hr);
        ok(items_returned == 1, "unexpected item count %li\n", items_returned);

        if (hr == S_OK && items_returned == 1)
        {
            ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
            ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
            ok(!strcmp(id.pszVal, "winetest"), "unexpected id: %s\n", id.pszVal);
            ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", value.vt);
            ok(!strcmp(value.pszVal, "value"), "unexpected value: %s\n", value.pszVal);

            PropVariantClear(&schema);
            PropVariantClear(&id);
            PropVariantClear(&value);
        }

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_FALSE, "Next failed, hr=%lx\n", hr);
        ok(items_returned == 0, "unexpected item count %li\n", items_returned);

        IWICEnumMetadataItem_Release(enumerator);
    }

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataFormat(reader, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataFormat failed, hr=%lx\n", hr);

    id.vt = VT_LPSTR;
    id.pszVal = CoTaskMemAlloc(strlen("winetest") + 1);
    strcpy(id.pszVal, "winetest");

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, &value);
    ok(hr == E_INVALIDARG, "GetValue failed, hr=%lx\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(value.pszVal, "value"), "unexpected value: %s\n", value.pszVal);
    PropVariantClear(&value);

    strcpy(id.pszVal, "test");

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "GetValue failed, hr=%lx\n", hr);

    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%lx\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%lx\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%lx\n", hr);
    ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(id.pszVal, "winetest"), "unexpected id: %s\n", id.pszVal);
    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%lx\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", value.vt);
    ok(!strcmp(value.pszVal, "value"), "unexpected value: %s\n", value.pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, 1, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetValueByIndex failed, hr=%lx\n", hr);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICPngTextMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    load_stream(writer, metadata_tEXt, sizeof(metadata_tEXt), 0);
    load_stream(writer, metadata_tEXt, sizeof(metadata_tEXt), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_gAMA(void)
{
    IWICMetadataHandlerInfo *info;
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT schema, id, value;
    IWICMetadataWriter *writer;
    UINT count;
    GUID format;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngGamaMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /*winxp*/, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    load_stream(reader, metadata_gAMA, sizeof(metadata_gAMA), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    todo_wine
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IWICMetadataHandlerInfo_Release(info);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunkgAMA), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 1, "unexpected count %i\n", count);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, &id, &value);
    ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);

    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
    PropVariantClear(&schema);

    ok(id.vt == VT_LPWSTR, "unexpected vt: %i\n", id.vt);
    ok(!lstrcmpW(id.pwszVal, L"ImageGamma"), "unexpected value: %s\n", wine_dbgstr_w(id.pwszVal));
    PropVariantClear(&id);

    ok(value.vt == VT_UI4, "unexpected vt: %i\n", value.vt);
    ok(value.ulVal == 33333, "unexpected value: %lu\n", value.ulVal);
    PropVariantClear(&value);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICPngGamaMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(schema.vt == VT_EMPTY, "Unexpected type %d.\n", schema.vt);
    ok(id.vt == VT_LPWSTR, "Unexpected type %d.\n", id.vt);
    ok(!lstrcmpW(id.pwszVal, L"ImageGamma"), "Unexpected id %s.\n", wine_dbgstr_w(id.pwszVal));
    PropVariantClear(&id);

    ok(value.vt == VT_UI4, "Unexpected type %d.\n", value.vt);
    ok(value.ulVal == 45455, "unexpected value: %lu\n", value.ulVal);

    hr = IWICMetadataWriter_RemoveValueByIndex(writer, 0);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_RemoveValueByIndex(writer, count);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    id.vt = VT_EMPTY;
    V_UI4(&id) = 123;
    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, metadata_gAMA, sizeof(metadata_gAMA), 0);
    load_stream(writer, metadata_gAMA, sizeof(metadata_gAMA), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_cHRM(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT schema, id, value;
    IWICMetadataWriter *writer;
    UINT count;
    GUID format;
    int i;
    static const WCHAR *expected_names[8] =
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
    static const ULONG expected_vals[8] = {
        31272, 30852, 61440, 32988, 25648, 60004, 14988, 5988
    };
    static const ULONG default_values[8] = {
        31270,32900,64000, 33000,30000, 60000,15000, 6000
    };

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngChrmMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /*winxp*/, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    load_stream(reader, metadata_cHRM, sizeof(metadata_cHRM), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunkcHRM), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 8, "unexpected count %i\n", count);

    for (i=0; i<8; i++)
    {
        hr = IWICMetadataReader_GetValueByIndex(reader, i, &schema, &id, &value);
        ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);

        ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
        PropVariantClear(&schema);

        ok(id.vt == VT_LPWSTR, "unexpected vt: %i\n", id.vt);
        ok(!lstrcmpW(id.pwszVal, expected_names[i]), "got %s, expected %s\n", wine_dbgstr_w(id.pwszVal), wine_dbgstr_w(expected_names[i]));
        PropVariantClear(&id);

        ok(value.vt == VT_UI4, "unexpected vt: %i\n", value.vt);
        ok(value.ulVal == expected_vals[i], "got %lu, expected %lu\n", value.ulVal, expected_vals[i]);
        PropVariantClear(&value);
    }

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICPngChrmMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 8, "Unexpected count %u.\n", count);

    for (i = 0; i < count && count == ARRAY_SIZE(expected_names); ++i)
    {
        hr = IWICMetadataWriter_GetValueByIndex(writer, i, &schema, &id, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(schema.vt == VT_EMPTY, "Unexpected type %d.\n", schema.vt);
        PropVariantClear(&schema);

        ok(id.vt == VT_LPWSTR, "Unexpected type %d.\n", id.vt);
        ok(!lstrcmpW(id.pwszVal, expected_names[i]), "Unexpected id %s, expected %s.\n",
                wine_dbgstr_w(id.pwszVal), wine_dbgstr_w(expected_names[i]));
        PropVariantClear(&id);

        ok(value.vt == VT_UI4, "Unexpected type %d.\n", value.vt);
        ok(value.ulVal == default_values[i], "Got %lu, expected %lu.\n", value.ulVal, default_values[i]);
        PropVariantClear(&value);
    }

    for (i = 0; i < count; ++i)
    {
        hr = IWICMetadataWriter_RemoveValueByIndex(writer, i);
        ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);
    }
    hr = IWICMetadataWriter_RemoveValueByIndex(writer, count);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, metadata_cHRM, sizeof(metadata_cHRM), 0);
    load_stream(writer, metadata_cHRM, sizeof(metadata_cHRM), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_hIST(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT schema, id, value;
    IWICMetadataWriter *writer;
    UINT count, i;
    GUID format;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngHistMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /*winxp*/, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    load_stream(reader, metadata_hIST, sizeof(metadata_hIST), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunkhIST), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 1, "unexpected count %i\n", count);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, &id, &value);
    ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);

    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
    PropVariantClear(&schema);

    ok(id.vt == VT_LPWSTR, "unexpected vt: %i\n", id.vt);
    ok(!lstrcmpW(id.pwszVal, L"Frequencies"), "unexpected value: %s\n", wine_dbgstr_w(id.pwszVal));
    PropVariantClear(&id);

    ok(value.vt == (VT_UI2|VT_VECTOR), "unexpected vt: %i\n", value.vt);
    ok(20 == value.caui.cElems, "expected cElems %d, got %ld\n", 20, value.caub.cElems);
    for (i = 0; i < value.caui.cElems; i++)
        ok(i+1 == value.caui.pElems[i], "%u: expected value %u, got %u\n", i, i+1, value.caui.pElems[i]);
    PropVariantClear(&value);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICPngHistMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(schema.vt == VT_EMPTY, "Unexpected type %d.\n", schema.vt);

    ok(id.vt == VT_LPWSTR, "Unexpected type %d.\n", id.vt);
    ok(!lstrcmpW(id.pwszVal, L"Frequencies"), "Unexpected id %s.\n", wine_dbgstr_w(id.pwszVal));
    PropVariantClear(&id);

    ok(value.vt == (VT_UI2|VT_VECTOR), "Unexpected type %#x.\n", value.vt);
    ok(!value.caui.cElems, "Unexpected cElems %ld.\n", value.caub.cElems);
    ok(!value.caui.pElems, "Unexpected value %p.\n", value.caui.pElems);

    hr = IWICMetadataWriter_RemoveValueByIndex(writer, 0);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataWriter_RemoveValueByIndex(writer, 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, metadata_hIST, sizeof(metadata_hIST), 0);
    load_stream(writer, metadata_hIST, sizeof(metadata_hIST), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_tIME(void)
{
    IWICMetadataQueryReader *query_reader, *query_reader2;
    IWICComponentFactory *factory;
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IEnumString *enum_string;
    PROPVARIANT value;
    UINT i, count;
    ULONG fetched;
    WCHAR *str;
    GUID format;
    static const struct test_data td[] =
    {
        { VT_UI2, 0, 0, { 2000 }, NULL, L"Year" },
        { VT_UI1, 0, 0, { 1 }, NULL, L"Month" },
        { VT_UI1, 0, 0, { 2 }, NULL, L"Day" },
        { VT_UI1, 0, 0, { 12 }, NULL, L"Hour" },
        { VT_UI1, 0, 0, { 34 }, NULL, L"Minute" },
        { VT_UI1, 0, 0, { 56 }, NULL, L"Second" },
    };
    static const struct test_data default_data[] =
    {
        { VT_UI2, 0, 0, { 0 }, NULL, L"Year" },
        { VT_UI1, 0, 0, { 1 }, NULL, L"Month" },
        { VT_UI1, 0, 0, { 1 }, NULL, L"Day" },
        { VT_UI1, 0, 0, { 0 }, NULL, L"Hour" },
        { VT_UI1, 0, 0, { 0 }, NULL, L"Minute" },
        { VT_UI1, 0, 0, { 0 }, NULL, L"Second" },
    };

    hr = CoCreateInstance(&CLSID_WICPngTimeMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /*winxp*/, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    load_stream(reader, metadata_tIME, sizeof(metadata_tIME), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktIME), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %i\n", count);

    compare_metadata(reader, td, count);

    /* Query reader. */
    hr = create_query_reader_from_metadata_reader(factory, reader, &GUID_ContainerFormatPng, &query_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryReader_GetEnumerator(query_reader, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!wcscmp(str, L"/tIME"), "Unexpected query %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
    IEnumString_Release(enum_string);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/tIME", &value);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (hr == S_OK)
{
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %d.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    hr = IWICMetadataQueryReader_GetEnumerator(query_reader2, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(td); ++i)
    {
        WCHAR buff[16];

        str = NULL;
        hr = IEnumString_Next(enum_string, 1, &str, &fetched);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(fetched == 1, "Unexpected count %lu.\n", fetched);

        wcscpy(buff, L"/");
        wcscat(buff, td[i].id_string);

        ok(!wcscmp(str, buff), "Unexpected query %s.\n", wine_dbgstr_w(str));
        CoTaskMemFree(str);
    }

    IEnumString_Release(enum_string);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader2, L"/Day", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI1, "Unexpected type %d.\n", value.vt);
    ok(value.bVal == 2, "Unexpected value %u.\n", value.bVal);
    PropVariantClear(&value);

    IWICMetadataQueryReader_Release(query_reader2);
}
    IWICMetadataQueryReader_Release(query_reader);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICPngTimeMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 6, "Unexpected count %u.\n", count);

    compare_metadata((IWICMetadataReader *)writer, default_data, count);

    for (i = 0; i < count; ++i)
    {
        hr = IWICMetadataWriter_RemoveValueByIndex(writer, i);
        ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);
    }
    hr = IWICMetadataWriter_RemoveValueByIndex(writer, count);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, metadata_tIME, sizeof(metadata_tIME), 0);
    load_stream(writer, metadata_tIME, sizeof(metadata_tIME), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
    IWICComponentFactory_Release(factory);
}

static inline USHORT ushort_bswap(USHORT s)
{
    return (s >> 8) | (s << 8);
}

static inline ULONG ulong_bswap(ULONG l)
{
    return ((ULONG)ushort_bswap((USHORT)l) << 16) | ushort_bswap((USHORT)(l >> 16));
}

static inline ULONGLONG ulonglong_bswap(ULONGLONG ll)
{
    return ((ULONGLONG)ulong_bswap((ULONG)ll) << 32) | ulong_bswap((ULONG)(ll >> 32));
}

static void byte_swap_ifd_data(char *data)
{
    USHORT number_of_entries, i;
    struct IFD_entry *entry;
    char *data_start = data;

    number_of_entries = *(USHORT *)data;
    *(USHORT *)data = ushort_bswap(*(USHORT *)data);
    data += sizeof(USHORT);

    for (i = 0; i < number_of_entries; i++)
    {
        entry = (struct IFD_entry *)data;

        switch (entry->type)
        {
        case IFD_BYTE:
        case IFD_SBYTE:
        case IFD_ASCII:
        case IFD_UNDEFINED:
            if (entry->count > 4)
                entry->value = ulong_bswap(entry->value);
            break;

        case IFD_SHORT:
        case IFD_SSHORT:
            if (entry->count > 2)
            {
                ULONG j, count = entry->count;
                USHORT *us = (USHORT *)(data_start + entry->value);
                if (!count) count = 1;
                for (j = 0; j < count; j++)
                    us[j] = ushort_bswap(us[j]);

                entry->value = ulong_bswap(entry->value);
            }
            else
            {
                ULONG j, count = entry->count;
                USHORT *us = (USHORT *)&entry->value;
                if (!count) count = 1;
                for (j = 0; j < count; j++)
                    us[j] = ushort_bswap(us[j]);
            }
            break;

        case IFD_LONG:
        case IFD_SLONG:
        case IFD_FLOAT:
            if (entry->count > 1)
            {
                ULONG j, count = entry->count;
                ULONG *ul = (ULONG *)(data_start + entry->value);
                if (!count) count = 1;
                for (j = 0; j < count; j++)
                    ul[j] = ulong_bswap(ul[j]);
            }
            entry->value = ulong_bswap(entry->value);
            break;

        case IFD_RATIONAL:
        case IFD_SRATIONAL:
            {
                ULONG j;
                ULONG *ul = (ULONG *)(data_start + entry->value);
                for (j = 0; j < entry->count * 2; j++)
                    ul[j] = ulong_bswap(ul[j]);
            }
            entry->value = ulong_bswap(entry->value);
            break;

        case IFD_DOUBLE:
            {
                ULONG j;
                ULONGLONG *ull = (ULONGLONG *)(data_start + entry->value);
                for (j = 0; j < entry->count; j++)
                    ull[j] = ulonglong_bswap(ull[j]);
            }
            entry->value = ulong_bswap(entry->value);
            break;

        default:
            assert(0);
            break;
        }

        entry->id = ushort_bswap(entry->id);
        entry->type = ushort_bswap(entry->type);
        entry->count = ulong_bswap(entry->count);
        data += sizeof(*entry);
    }
}

static void test_ifd_content(IWICMetadataReader *reader)
{
    static const struct test_data td[28] =
    {
        { VT_UI2, 0xfe, 0, { 1 } },
        { VT_UI4, 0x100, 0, { 222 } },
        { VT_UI4, 0x101, 0, { 333 } },
        { VT_UI2, 0x102, 0, { 24 } },
        { VT_UI4, 0x103, 0, { 32773 } },
        { VT_UI8, 0x11a, 0, { ((LONGLONG)3 << 32) | 900 } },
        { VT_UI1, 0xf001, 0, { 0x44 } },
        { VT_UI1|VT_VECTOR, 0xf002, 4, { 0x44, 0x33, 0x22, 0x11 } },
        { VT_I1, 0xf003, 0, { 0x44 } },
        { VT_I2, 0xf004, 0, { 0x3344 } },
        { VT_I2|VT_VECTOR, 0xf005, 2, { 0x3344, 0x1122 } },
        { VT_I4, 0xf006, 0, { 0x11223344 } },
        { VT_R4, 0xf007, 0, { 0x11223344 } },
        { VT_R8, 0xf008, 0, { ((LONGLONG)0x41d26580 << 32) | 0xb486522c } },
        { VT_I8, 0xf009, 0, { ((LONGLONG)0x5a6b7c8d << 32) | 0x1a2b3c4d } },
        { VT_UI1|VT_VECTOR, 0xf00a, 13, { 'H','e','l','l','o',' ','W','o','r','l','d','!',0 } },
        { VT_I2|VT_VECTOR, 0xf00b, 4, { 0x0101, 0x0202, 0x0303, 0x0404 } },
        { VT_I4|VT_VECTOR, 0xf00c, 2, { 0x11223344, 0x55667788 } },
        { VT_R4|VT_VECTOR, 0xf00d, 2, { 0x449a522b, 0x4608f5ba } },
        { VT_LPSTR, 0xf00e, 12, { 0 }, "Hello World!" },
        { VT_LPSTR, 0xf00f, 4, { 0 }, "abcd" },
        { VT_BLOB, 0xf010, 13, { 0 }, "Hello World!" },
        { VT_BLOB, 0xf011, 4, { 0 }, "abcd" },
        { VT_UI1, 0xf012, 0, { 0x44 } },
        { VT_UI2, 0xf013, 0, { 0x3344 } },
        { VT_UI4, 0xf014, 0, { 0x11223344 } },
        { VT_R4, 0xf015, 0, { 0x11223344 } },
        { VT_I8|VT_VECTOR, 0xf016, 3,
          { ((LONGLONG)0x05060708 << 32) | 0x01020304,
            ((LONGLONG)0x50607080 << 32) | 0x10203040,
            ((LONGLONG)0x55667788 << 32) | 0x11223344 } },
    };
    PROPVARIANT schema, id, value;
    char *IFD_data_swapped;
    UINT count;
    HRESULT hr;

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    load_stream(reader, (const char *)&IFD_data, sizeof(IFD_data), WICPersistOptionLittleEndian);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    /* Test big-endian IFD data */
    IFD_data_swapped = HeapAlloc(GetProcessHeap(), 0, sizeof(IFD_data));
    memcpy(IFD_data_swapped, &IFD_data, sizeof(IFD_data));
    byte_swap_ifd_data(IFD_data_swapped);
    load_stream(reader, IFD_data_swapped, sizeof(IFD_data), WICPersistOptionBigEndian);
    check_persist_options(reader, WICPersistOptionBigEndian);
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);
    compare_metadata(reader, td, count);
    HeapFree(GetProcessHeap(), 0, IFD_data_swapped);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#lx\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, count - 1, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#lx\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#lx\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %u\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, count - 1, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#lx\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %u\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#lx\n", hr);
    ok(id.vt == VT_UI2, "unexpected vt: %u\n", id.vt);
    ok(id.uiVal == 0xfe, "unexpected id: %#x\n", id.uiVal);
    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    ok(hr == S_OK, "GetValueByIndex error %#lx\n", hr);
    ok(value.vt == VT_UI2, "unexpected vt: %u\n", value.vt);
    ok(value.uiVal == 1, "unexpected id: %#x\n", value.uiVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, count, &schema, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "expected WINCODEC_ERR_PROPERTYNOTFOUND, got %#lx\n", hr);

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "expected WINCODEC_ERR_PROPERTYNOTFOUND, got %#lx\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, NULL);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "expected WINCODEC_ERR_PROPERTYNOTFOUND, got %#lx\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, &value);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    id.vt = VT_UI2;
    id.uiVal = 0xf00e;
    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == S_OK, "GetValue error %#lx\n", hr);

    /* schema is ignored by Ifd metadata reader */
    schema.vt = VT_UI4;
    schema.ulVal = 0xdeadbeef;
    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == S_OK, "GetValue error %#lx\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(value.pszVal, "Hello World!"), "unexpected value: %s\n", value.pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    ok(hr == S_OK, "GetValue error %#lx\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(value.pszVal, "Hello World!"), "unexpected value: %s\n", value.pszVal);
    PropVariantClear(&value);
}

static void test_metadata_Ifd(void)
{
#include "pshpack2.h"
    static const struct ifd0_data
    {
        USHORT count;
        struct IFD_entry ifd0[2];
        ULONG next_IFD;
    }
    ifd0_data =
    {
        2,
        {
            /* Exif IFD pointer */
            { 0x8769, IFD_LONG, 1, 0 },
            /* GPS IFD pointer */
            { 0x8825, IFD_LONG, 1, 0 },
        },
    };
#include "poppack.h"

    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    PROPVARIANT id, value;
    GUID format;
    UINT count;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICIfdMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);
    check_interface(reader, &IID_IWICMetadataBlockReader, FALSE);

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount error %#lx\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    test_ifd_content(reader);

    test_reader_container_format(reader, &GUID_ContainerFormatTiff);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataFormat(reader, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataFormat should fail\n");

    /* IFD contains pointer tags. */
    load_stream(reader, (const char *)&ifd0_data, sizeof(ifd0_data), 0);
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == 2, "unexpected count %u\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %u.\n", value.vt);
    PropVariantClear(&value);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(reader, 1, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %u.\n", value.vt);
    PropVariantClear(&value);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICIfdMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    load_stream(writer, (const char *)&IFD_data, sizeof(IFD_data), 0);
    load_stream(writer, (const char *)&IFD_data, sizeof(IFD_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_Exif(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    UINT count=0;
    GUID format;

    hr = CoCreateInstance(&CLSID_WICExifMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);
    check_interface(reader, &IID_IWICMetadataBlockReader, FALSE);

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount error %#lx\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatExif), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    test_reader_container_format(reader, &GUID_MetadataFormatIfd);
    test_ifd_content(reader);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICExifMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    load_stream(writer, (const char *)&IFD_data, sizeof(IFD_data), 0);
    load_stream(writer, (const char *)&IFD_data, sizeof(IFD_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_Gps(void)
{
    PROPVARIANT schema, id, value;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    UINT count=0;
    GUID format;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICGpsMetadataReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);
    check_interface(reader, &IID_IWICMetadataBlockReader, FALSE);

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatGps), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    test_reader_container_format(reader, &GUID_MetadataFormatIfd);
    test_ifd_content(reader);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICGpsMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    id.vt = VT_UI2;
    id.uiVal = 0x300;
    value.vt = VT_UI2;
    value.ulVal = 555;
    PropVariantInit(&schema);
    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    id.uiVal = 1;
    hr = IWICMetadataWriter_RemoveValue(writer, &schema, &id);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);

    schema.vt = VT_UI4;
    id.uiVal = 0x300;
    hr = IWICMetadataWriter_RemoveValue(writer, &schema, &id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    load_stream(writer, (const char *)&IFD_data, sizeof(IFD_data), 0);
    load_stream(writer, (const char *)&IFD_data, sizeof(IFD_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_create_reader_from_container(void)
{
    HRESULT hr;
    IWICComponentFactory *factory;
    IStream *stream;
    IWICMetadataReader *reader;
    UINT count=0;
    GUID format;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICComponentFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    stream = create_stream(metadata_tEXt, sizeof(metadata_tEXt));

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        NULL, NULL, WICPersistOptionDefault,
        stream, &reader);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionDefault,
        NULL, &reader);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionDefault,
        stream, NULL);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionDefault,
        stream, &reader);
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%lx\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
        ok(count == 1, "unexpected count %i\n", count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "unexpected format %s\n", wine_dbgstr_guid(&format));

        IWICMetadataReader_Release(reader);
    }

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatWmp, NULL, WICPersistOptionDefault,
        stream, &reader);
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%lx\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
        ok(count == 1, "unexpected count %i\n", count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "unexpected format %s\n", wine_dbgstr_guid(&format));

        IWICMetadataReader_Release(reader);
    }

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory, &GUID_ContainerFormatWmp,
            NULL, WICMetadataCreationFailUnknown, stream, &reader);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    IWICComponentFactory_Release(factory);
}

static void test_CreateMetadataReader(void)
{
    IWICPersistStream *persist_stream;
    IWICComponentFactory *factory;
    IWICMetadataReader *reader;
    IStream *stream, *stream2;
    LARGE_INTEGER pos;
    UINT count = 0;
    GUID format;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = create_stream(metadata_tEXt, sizeof(metadata_tEXt));

    hr = IWICComponentFactory_CreateMetadataReader(factory, NULL, NULL, 0, stream, &reader);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(&format, 0xcc, sizeof(format));
    hr = IWICComponentFactory_CreateMetadataReader(factory, &format, NULL, 0, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = get_persist_stream(reader, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stream == stream2, "Unexpected stream.\n");
    IStream_Release(stream2);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataReader_Release(reader);

    memset(&format, 0xcc, sizeof(format));
    hr = IWICComponentFactory_CreateMetadataReader(factory, &format, NULL, WICMetadataCreationFailUnknown, stream, &reader);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt,
            NULL, 0, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = get_persist_stream(reader, &stream2);
    ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataReader_Release(reader);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt, NULL, 0,
            stream, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_ContainerFormatPng, NULL, 0, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataReader_Release(reader);

    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt, NULL, 0, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IWICMetadataWriter, FALSE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    IWICMetadataReader_Release(reader);

    /* Invalid vendor. */
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt, &IID_IUnknown, 0, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataReader_Release(reader);

    /* Mismatching metadata format. */
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatApp1, NULL, 0, stream, &reader);
    todo_wine
    ok(hr == WINCODEC_ERR_BADMETADATAHEADER, "Unexpected hr %#lx.\n", hr);

    /* With and without caching */
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt, NULL,
            WICPersistOptionNoCacheStream, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = get_persist_stream(reader, &stream2);
    ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICPersistStream_LoadEx(persist_stream, stream, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = get_persist_stream(reader, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stream == stream2, "Unexpected stream.\n");
    IStream_Release(stream2);

    /* Going from caching to no caching. */
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICPersistStream_LoadEx(persist_stream, stream, NULL, WICPersistOptionNoCacheStream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = get_persist_stream(reader, &stream2);
    ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
    check_persist_options(reader, WICPersistOptionNoCacheStream);

    IWICPersistStream_Release(persist_stream);

    IWICMetadataReader_Release(reader);

    IStream_Release(stream);

    IWICComponentFactory_Release(factory);
}

static void test_metadata_png(void)
{
    static const struct test_data td[6] =
    {
        { VT_UI2, 0, 0, { 2005 }, NULL, L"Year" },
        { VT_UI1, 0, 0, { 6 }, NULL, L"Month" },
        { VT_UI1, 0, 0, { 3 }, NULL, L"Day" },
        { VT_UI1, 0, 0, { 15 }, NULL, L"Hour" },
        { VT_UI1, 0, 0, { 7 }, NULL, L"Minute" },
        { VT_UI1, 0, 0, { 45 }, NULL, L"Second" }
    };
    IStream *stream;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICMetadataBlockReader *blockreader;
    IWICMetadataReader *reader, *reader2;
    IWICMetadataQueryReader *queryreader;
    IWICComponentFactory *factory;
    GUID containerformat;
    HRESULT hr;
    UINT count=0xdeadbeef;

    hr = CoCreateInstance(&CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    if (FAILED(hr)) return;

    stream = create_stream(pngimage, sizeof(pngimage));

    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

    check_interface(decoder, &IID_IWICMetadataBlockReader, FALSE);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame failed, hr=%lx\n", hr);

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    winetest_push_context("png");
    test_block_reader_enumerator(blockreader);
    winetest_pop_context();

    hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, NULL);
    ok(hr == E_INVALIDARG, "GetContainerFormat failed, hr=%lx\n", hr);

    hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &containerformat);
    ok(hr == S_OK, "GetContainerFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&containerformat, &GUID_ContainerFormatPng), "unexpected container format\n");

    hr = IWICMetadataBlockReader_GetCount(blockreader, NULL);
    ok(hr == E_INVALIDARG, "GetCount failed, hr=%lx\n", hr);

    hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 1, "unexpected count %d\n", count);

    hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, NULL);
    ok(hr == E_INVALIDARG, "GetReaderByIndex failed, hr=%lx\n", hr);

    hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
    ok(hr == S_OK, "GetReaderByIndex failed, hr=%lx\n", hr);
    hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(reader == reader2, "Unexpected instance.\n");
    IWICMetadataReader_Release(reader2);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &containerformat);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
    ok(IsEqualGUID(&containerformat, &GUID_MetadataFormatChunktIME), "Unexpected format %s.\n",
            wine_dbgstr_guid(&containerformat));

    test_reader_container_format(reader, &GUID_ContainerFormatPng);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == 6, "Unexpected item count %u.\n", count);
    if (count == 6)
        compare_metadata(reader, td, count);

    IWICMetadataReader_Release(reader);

    hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
    todo_wine ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "GetReaderByIndex failed, hr=%lx\n", hr);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICComponentFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, blockreader, &queryreader);
    ok(hr == S_OK, "CreateQueryReaderFromBlockReader failed: %08lx\n", hr);

    IWICMetadataQueryReader_Release(queryreader);

    IWICComponentFactory_Release(factory);

    IWICMetadataBlockReader_Release(blockreader);

    hr = IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &queryreader);
    ok(hr == S_OK, "GetMetadataQueryReader failed: %08lx\n", hr);
    IWICMetadataQueryReader_Release(queryreader);

    IWICBitmapFrameDecode_Release(frame);

    IWICBitmapDecoder_Release(decoder);

    IStream_Release(stream);
}

static void test_metadata_gif(void)
{
    static const struct test_data gif_LSD[9] =
    {
        { VT_UI1|VT_VECTOR, 0, 6, {'G','I','F','8','7','a'}, NULL, { 'S','i','g','n','a','t','u','r','e',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'C','o','l','o','r','R','e','s','o','l','u','t','i','o','n',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'B','a','c','k','g','r','o','u','n','d','C','o','l','o','r','I','n','d','e','x',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'P','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0 } }
    };
    static const struct test_data gif_IMD[8] =
    {
        { VT_UI2, 0, 0, { 0 }, NULL, { 'L','e','f','t',0 } },
        { VT_UI2, 0, 0, { 0 }, NULL, { 'T','o','p',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'I','n','t','e','r','l','a','c','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } }
    };
    static const struct test_data animated_gif_LSD[9] =
    {
        { VT_UI1|VT_VECTOR, 0, 6, {'G','I','F','8','9','a'}, NULL, { 'S','i','g','n','a','t','u','r','e',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 2 }, NULL, { 'C','o','l','o','r','R','e','s','o','l','u','t','i','o','n',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'B','a','c','k','g','r','o','u','n','d','C','o','l','o','r','I','n','d','e','x',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'P','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0 } }
    };
    static const struct test_data animated_gif_IMD[8] =
    {
        { VT_UI2, 0, 0, { 0 }, NULL, { 'L','e','f','t',0 } },
        { VT_UI2, 0, 0, { 0 }, NULL, { 'T','o','p',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'I','n','t','e','r','l','a','c','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 1 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } }
    };
    static const struct test_data animated_gif_GCE[5] =
    {
        { VT_UI1, 0, 0, { 0 }, NULL, { 'D','i','s','p','o','s','a','l',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'U','s','e','r','I','n','p','u','t','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'T','r','a','n','s','p','a','r','e','n','c','y','F','l','a','g',0 } },
        { VT_UI2, 0, 0, { 10 }, NULL, { 'D','e','l','a','y',0 } },
        { VT_UI1, 0, 0, { 1 }, NULL, { 'T','r','a','n','s','p','a','r','e','n','t','C','o','l','o','r','I','n','d','e','x',0 } }
    };
    static const struct test_data animated_gif_APE[2] =
    {
        { VT_UI1|VT_VECTOR, 0, 11, { 'A','N','I','M','E','X','T','S','1','.','0' }, NULL, { 'A','p','p','l','i','c','a','t','i','o','n',0 } },
        { VT_UI1|VT_VECTOR, 0, 4, { 0x03,0x01,0x05,0x00 }, NULL, { 'D','a','t','a',0 } }
    };
    static const struct test_data animated_gif_comment_1[1] =
    {
        { VT_LPSTR, 0, 12, { 0 }, "Hello World!", { 'T','e','x','t','E','n','t','r','y',0 } }
    };
    static const struct test_data animated_gif_comment_2[1] =
    {
        { VT_LPSTR, 0, 8, { 0 }, "image #1", { 'T','e','x','t','E','n','t','r','y',0 } }
    };
    static const struct test_data animated_gif_plain_1[1] =
    {
        { VT_BLOB, 0, 17, { 0 }, "\x21\x01\x0d\x61nimation.gif" }
    };
    static const struct test_data animated_gif_plain_2[1] =
    {
        { VT_BLOB, 0, 16, { 0 }, "\x21\x01\x0cplaintext #1" }
    };
    IStream *stream;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICMetadataBlockReader *blockreader;
    IWICMetadataReader *reader, *reader2;
    IWICMetadataQueryReader *queryreader;
    GUID format;
    HRESULT hr;
    UINT count;

    /* 1x1 pixel gif */
    stream = create_stream(gifimage, sizeof(gifimage));

    hr = CoCreateInstance(&CLSID_WICGifDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);
    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);

    IStream_Release(stream);

    /* global metadata block */
    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#lx\n", hr);

    if (SUCCEEDED(hr))
    {
        winetest_push_context("gif");
        test_block_reader_enumerator(blockreader);
        winetest_pop_context();

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#lx\n", hr);
        ok(count == 1, "expected 1, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader2);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);
        ok(reader == reader2, "Unexpected instance.\n");
        IWICMetadataReader_Release(reader2);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatLSD), /* Logical Screen Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            test_reader_container_format(reader, &GUID_ContainerFormatGif);

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(gif_LSD), "unexpected count %u\n", count);

            compare_metadata(reader, gif_LSD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    /* frame metadata block */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#lx\n", hr);

    if (SUCCEEDED(hr))
    {
        test_block_reader_enumerator(blockreader);

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#lx\n", hr);
        ok(count == 1, "expected 1, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);
        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader2);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);
        ok(reader == reader2, "Unexpected instance.\n");
        IWICMetadataReader_Release(reader2);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), /* Image Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(gif_IMD), "unexpected count %u\n", count);

            compare_metadata(reader, gif_IMD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);

    /* 1x1 pixel gif, 2 frames */
    stream = create_stream(animatedgif, sizeof(animatedgif));

    hr = CoCreateInstance(&CLSID_WICGifDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);
    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);

    IStream_Release(stream);

    /* global metadata block */
    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#lx\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatLSD), /* Logical Screen Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_LSD), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_LSD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatAPE), /* Application Extension */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_APE), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_APE, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 2, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatGifComment), /* Comment Extension */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_comment_1), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_comment_1, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 3, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown),
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_plain_1), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_plain_1, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 4, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    /* frame metadata block */
    hr = IWICBitmapDecoder_GetFrame(decoder, 1, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#lx\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), /* Image Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_IMD), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_IMD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatGCE), /* Graphic Control Extension */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_GCE), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_GCE, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 2, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatGifComment), /* Comment Extension */
                "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_comment_2), "unexpected count %u\n", count);

            if (count == 1)
            compare_metadata(reader, animated_gif_comment_2, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 3, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown),
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#lx\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_plain_2), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_plain_2, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 4, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, &queryreader);
    ok(hr == S_OK || broken(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION) /* before Vista */,
       "GetMetadataQueryReader error %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        static const struct
        {
            const char *query;
            HRESULT hr;
            UINT vt;
        } decoder_data[] =
        {
            { "/logscrdesc/Signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/[0]logscrdesc/Signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/logscrdesc/\\Signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/Logscrdesc/\\signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/logscrdesc/{str=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/[0]logscrdesc/{str=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/logscrdesc/{wstr=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/[0]logscrdesc/{wstr=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/appext/Application", S_OK, VT_UI1 | VT_VECTOR },
            { "/appext/{STR=APPlication}", S_OK, VT_UI1 | VT_VECTOR },
            { "/appext/{WSTR=APPlication}", S_OK, VT_UI1 | VT_VECTOR },
            { "/LogSCRdesC", S_OK, VT_UNKNOWN },
            { "/[0]LogSCRdesC", S_OK, VT_UNKNOWN },
            { "/appEXT", S_OK, VT_UNKNOWN },
            { "/[0]appEXT", S_OK, VT_UNKNOWN },
            { "grctlext", WINCODEC_ERR_PROPERTYNOTSUPPORTED, 0 },
            { "/imgdesc", WINCODEC_ERR_PROPERTYNOTFOUND, 0 },
        };
        WCHAR name[256];
        UINT len, i, j;
        PROPVARIANT value;
        IWICMetadataQueryReader *meta_reader;

        hr = IWICMetadataQueryReader_GetContainerFormat(queryreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        name[0] = 0;
        len = 0xdeadbeef;
        hr = IWICMetadataQueryReader_GetLocation(queryreader, 256, name, &len);
        ok(hr == S_OK, "GetLocation error %#lx\n", hr);
        ok(len == 2, "expected 2, got %u\n", len);
        ok(!lstrcmpW(name, L"/"), "expected '/', got %s\n", wine_dbgstr_w(name));

        for (i = 0; i < ARRAY_SIZE(decoder_data); i++)
        {
            WCHAR queryW[256];

            if (winetest_debug > 1)
                trace("query: %s\n", decoder_data[i].query);
            MultiByteToWideChar(CP_ACP, 0, decoder_data[i].query, -1, queryW, 256);

            hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, NULL);
            ok(hr == decoder_data[i].hr, "GetMetadataByName(%s) returned %#lx, expected %#lx\n", wine_dbgstr_w(queryW), hr, decoder_data[i].hr);

            PropVariantInit(&value);
            hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, &value);
            ok(hr == decoder_data[i].hr, "GetMetadataByName(%s) returned %#lx, expected %#lx\n", wine_dbgstr_w(queryW), hr, decoder_data[i].hr);
            ok(value.vt == decoder_data[i].vt, "expected %#x, got %#x\n", decoder_data[i].vt, value.vt);
            if (hr == S_OK && value.vt == VT_UNKNOWN)
            {
                hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&meta_reader);
                ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

                name[0] = 0;
                len = 0xdeadbeef;
                hr = IWICMetadataQueryReader_GetLocation(meta_reader, 256, name, &len);
                ok(hr == S_OK, "GetLocation error %#lx\n", hr);
                ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);
                ok(!lstrcmpW(name, queryW), "expected %s, got %s\n", wine_dbgstr_w(queryW), wine_dbgstr_w(name));

                for (j = 0; j < ARRAY_SIZE(decoder_data); j++)
                {
                    MultiByteToWideChar(CP_ACP, 0, decoder_data[j].query, -1, queryW, 256);

                    if (CompareStringW(LOCALE_NEUTRAL, NORM_IGNORECASE, queryW, len-1, name, len-1) == CSTR_EQUAL && decoder_data[j].query[len - 1] != 0)
                    {
                        if (winetest_debug > 1)
                            trace("query: %s\n", wine_dbgstr_w(queryW + len - 1));
                        PropVariantClear(&value);
                        hr = IWICMetadataQueryReader_GetMetadataByName(meta_reader, queryW + len - 1, &value);
                        ok(hr == decoder_data[j].hr, "GetMetadataByName(%s) returned %#lx, expected %#lx\n", wine_dbgstr_w(queryW + len - 1), hr, decoder_data[j].hr);
                        ok(value.vt == decoder_data[j].vt, "expected %#x, got %#x\n", decoder_data[j].vt, value.vt);
                    }
                }

                IWICMetadataQueryReader_Release(meta_reader);
            }

            PropVariantClear(&value);
        }

        IWICMetadataQueryReader_Release(queryreader);
    }

    hr = IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &queryreader);
    ok(hr == S_OK || broken(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION) /* before Vista */,
       "GetMetadataQueryReader error %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        static const struct
        {
            const char *query;
            HRESULT hr;
            UINT vt;
        } frame_data[] =
        {
            { "/grctlext/Delay", S_OK, VT_UI2 },
            { "/[0]grctlext/Delay", S_OK, VT_UI2 },
            { "/grctlext/{str=delay}", S_OK, VT_UI2 },
            { "/[0]grctlext/{str=delay}", S_OK, VT_UI2 },
            { "/grctlext/{wstr=delay}", S_OK, VT_UI2 },
            { "/[0]grctlext/{wstr=delay}", S_OK, VT_UI2 },
            { "/imgdesc/InterlaceFlag", S_OK, VT_BOOL },
            { "/imgdesc/{STR=interlaceFLAG}", S_OK, VT_BOOL },
            { "/imgdesc/{WSTR=interlaceFLAG}", S_OK, VT_BOOL },
            { "/grctlext", S_OK, VT_UNKNOWN },
            { "/[0]grctlext", S_OK, VT_UNKNOWN },
            { "/imgdesc", S_OK, VT_UNKNOWN },
            { "/[0]imgdesc", S_OK, VT_UNKNOWN },
            { "/LogSCRdesC", WINCODEC_ERR_PROPERTYNOTFOUND, 0 },
            { "/appEXT", WINCODEC_ERR_PROPERTYNOTFOUND, 0 },
            { "/grctlext/{\\str=delay}", WINCODEC_ERR_WRONGSTATE, 0 },
            { "/grctlext/{str=\\delay}", S_OK, VT_UI2 },
            { "grctlext/Delay", WINCODEC_ERR_PROPERTYNOTSUPPORTED, 0 },
        };
        static const WCHAR guidW[] = {'/','{','g','u','i','d','=','\\',0};
        WCHAR name[256], queryW[256];
        UINT len, i;
        PROPVARIANT value;
        IWICMetadataQueryReader *meta_reader;

        hr = IWICMetadataQueryReader_GetContainerFormat(queryreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        name[0] = 0;
        len = 0xdeadbeef;
        hr = IWICMetadataQueryReader_GetLocation(queryreader, 256, name, &len);
        ok(hr == S_OK, "GetLocation error %#lx\n", hr);
        ok(len == 2, "expected 2, got %u\n", len);
        ok(!lstrcmpW(name, L"/"), "expected '/', got %s\n", wine_dbgstr_w(name));

        for (i = 0; i < ARRAY_SIZE(frame_data); i++)
        {
            if (winetest_debug > 1)
                trace("query: %s\n", frame_data[i].query);
            MultiByteToWideChar(CP_ACP, 0, frame_data[i].query, -1, queryW, 256);
            PropVariantInit(&value);
            hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, &value);
            ok(hr == frame_data[i].hr, "GetMetadataByName(%s) returned %#lx, expected %#lx\n", wine_dbgstr_w(queryW), hr, frame_data[i].hr);
            ok(value.vt == frame_data[i].vt, "expected %#x, got %#x\n", frame_data[i].vt, value.vt);
            if (hr == S_OK && value.vt == VT_UNKNOWN)
            {
                hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&meta_reader);
                ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

                name[0] = 0;
                len = 0xdeadbeef;
                hr = IWICMetadataQueryReader_GetLocation(meta_reader, 256, name, &len);
                ok(hr == S_OK, "GetLocation error %#lx\n", hr);
                ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);
                ok(!lstrcmpW(name, queryW), "expected %s, got %s\n", wine_dbgstr_w(queryW), wine_dbgstr_w(name));

                IWICMetadataQueryReader_Release(meta_reader);
            }

            PropVariantClear(&value);
        }

        name[0] = 0;
        len = 0xdeadbeef;
        hr = WICMapGuidToShortName(&GUID_MetadataFormatIMD, 256, name, &len);
        ok(hr == S_OK, "WICMapGuidToShortName error %#lx\n", hr);
        ok(!lstrcmpW(name, L"imgdesc"), "wrong short name %s\n", wine_dbgstr_w(name));

        format = GUID_NULL;
        hr = WICMapShortNameToGuid(L"imgdesc", &format);
        ok(hr == S_OK, "WICMapGuidToShortName error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), "wrong guid %s\n", wine_dbgstr_guid(&format));

        format = GUID_NULL;
        hr = WICMapShortNameToGuid(L"ImgDesc", &format);
        ok(hr == S_OK, "WICMapGuidToShortName error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), "wrong guid %s\n", wine_dbgstr_guid(&format));

        lstrcpyW(queryW, guidW);
        StringFromGUID2(&GUID_MetadataFormatIMD, queryW + lstrlenW(queryW) - 1, 39);
        memcpy(queryW, guidW, sizeof(guidW) - 2);
        if (winetest_debug > 1)
            trace("query: %s\n", wine_dbgstr_w(queryW));
        PropVariantInit(&value);
        hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, &value);
        ok(hr == S_OK, "GetMetadataByName(%s) error %#lx\n", wine_dbgstr_w(queryW), hr);
        ok(value.vt == VT_UNKNOWN, "expected VT_UNKNOWN, got %#x\n", value.vt);
        PropVariantClear(&value);

        IWICMetadataQueryReader_Release(queryreader);
    }

    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_metadata_LSD(void)
{
    static const char LSD_data[] = "hello world!\x1\x2\x3\x4\xab\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf";
    static const struct test_data td[9] =
    {
        { VT_UI1|VT_VECTOR, 0, 6, {'w','o','r','l','d','!'}, NULL, L"Signature" },
        { VT_UI2, 0, 0, { 0x201 }, NULL, L"Width" },
        { VT_UI2, 0, 0, { 0x403 }, NULL, L"Height" },
        { VT_BOOL, 0, 0, { 1 }, NULL, L"GlobalColorTableFlag" },
        { VT_UI1, 0, 0, { 2 }, NULL, L"ColorResolution" },
        { VT_BOOL, 0, 0, { 1 }, NULL, L"SortFlag" },
        { VT_UI1, 0, 0, { 3 }, NULL, L"GlobalColorTableSize" },
        { VT_UI1, 0, 0, { 6 }, NULL, L"BackgroundColorIndex" },
        { VT_UI1, 0, 0, { 7 }, NULL, L"PixelAspectRatio" }
    };
    LARGE_INTEGER pos;
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy;
    GUID format;
    CLSID id;

    hr = CoCreateInstance(&CLSID_WICLSDMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    stream = create_stream(LSD_data, sizeof(LSD_data));

    test_reader_container_format(reader, &GUID_ContainerFormatGif);

    pos.QuadPart = 6;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IWICPersistStream_Load(persist, stream);
    ok(hr == S_OK, "Load error %#lx\n", hr);
    check_persist_options(reader, 0);

    IWICPersistStream_Release(persist);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatLSD), "wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    ok(hr == S_OK, "GetMetadataHandlerInfo error %#lx\n", hr);

    hr = IWICMetadataHandlerInfo_GetCLSID(info, &id);
    ok(hr == S_OK, "GetCLSID error %#lx\n", hr);
    ok(IsEqualGUID(&id, &CLSID_WICLSDMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&id));

    hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
    ok(hr == S_OK, "GetFriendlyName error %#lx\n", hr);
    ok(!lstrcmpW(name, L"Logical Screen Descriptor Reader"), "wrong LSD reader name %s\n", wine_dbgstr_w(name));

    IWICMetadataHandlerInfo_Release(info);
    IWICMetadataReader_Release(reader);

    IStream_Release(stream);

    hr = CoCreateInstance(&CLSID_WICLSDMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    load_stream(writer, LSD_data, sizeof(LSD_data), 0);
    load_stream(writer, LSD_data, sizeof(LSD_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_IMD(void)
{
    static const char IMD_data[] = "hello world!\x1\x2\x3\x4\x5\x6\x7\x8\xed\xa\xb\xc\xd\xe\xf";
    static const struct test_data td[8] =
    {
        { VT_UI2, 0, 0, { 0x201 }, NULL, L"Left" },
        { VT_UI2, 0, 0, { 0x403 }, NULL, L"Top" },
        { VT_UI2, 0, 0, { 0x605 }, NULL, L"Width" },
        { VT_UI2, 0, 0, { 0x807 }, NULL, L"Height" },
        { VT_BOOL, 0, 0, { 1 }, NULL, L"LocalColorTableFlag" },
        { VT_BOOL, 0, 0, { 1 }, NULL, L"InterlaceFlag" },
        { VT_BOOL, 0, 0, { 1 }, NULL, L"SortFlag" },
        { VT_UI1, 0, 0, { 5 }, NULL, L"LocalColorTableSize" }
    };
    LARGE_INTEGER pos;
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy;
    GUID format;
    CLSID id;

    hr = CoCreateInstance(&CLSID_WICIMDMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    test_reader_container_format(reader, &GUID_ContainerFormatGif);

    stream = create_stream(IMD_data, sizeof(IMD_data));

    pos.QuadPart = 12;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IWICPersistStream_Load(persist, stream);
    ok(hr == S_OK, "Load error %#lx\n", hr);
    check_persist_options(reader, 0);

    IWICPersistStream_Release(persist);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), "wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    ok(hr == S_OK, "GetMetadataHandlerInfo error %#lx\n", hr);

    hr = IWICMetadataHandlerInfo_GetCLSID(info, &id);
    ok(hr == S_OK, "GetCLSID error %#lx\n", hr);
    ok(IsEqualGUID(&id, &CLSID_WICIMDMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&id));

    hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
    ok(hr == S_OK, "GetFriendlyName error %#lx\n", hr);
    ok(!lstrcmpW(name, L"Image Descriptor Reader"), "wrong IMD reader name %s\n", wine_dbgstr_w(name));

    IWICMetadataHandlerInfo_Release(info);
    IWICMetadataReader_Release(reader);

    IStream_Release(stream);

    hr = CoCreateInstance(&CLSID_WICIMDMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    load_stream(writer, IMD_data, sizeof(IMD_data), 0);
    load_stream(writer, IMD_data, sizeof(IMD_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_GCE(void)
{
    static const char GCE_data[] = "hello world!\xa\x2\x3\x4\x5\x6\x7\x8\xed\xa\xb\xc\xd\xe\xf";
    static const struct test_data td[5] =
    {
        { VT_UI1, 0, 0, { 2 }, NULL, L"Disposal" },
        { VT_BOOL, 0, 0, { 1 }, NULL, L"UserInputFlag" },
        { VT_BOOL, 0, 0, { 0 }, NULL, L"TransparencyFlag" },
        { VT_UI2, 0, 0, { 0x302 }, NULL, L"Delay" },
        { VT_UI1, 0, 0, { 4 }, NULL, L"TransparentColorIndex" }
    };
    static const struct test_data default_data[5] =
    {
        { VT_UI1, 0, 0, { 0 }, NULL, L"Disposal" },
        { VT_BOOL, 0, 0, { 0 }, NULL, L"UserInputFlag" },
        { VT_BOOL, 0, 0, { 0 }, NULL, L"TransparencyFlag" },
        { VT_UI2, 0, 0, { 0 }, NULL, L"Delay" },
        { VT_UI1, 0, 0, { 0 }, NULL, L"TransparentColorIndex" }
    };
    LARGE_INTEGER pos;
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT i, count, dummy;
    GUID format;
    CLSID id;

    hr = CoCreateInstance(&CLSID_WICGCEMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    test_reader_container_format(reader, &GUID_ContainerFormatGif);

    stream = create_stream(GCE_data, sizeof(GCE_data));

    pos.QuadPart = 12;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IWICPersistStream_Load(persist, stream);
    ok(hr == S_OK, "Load error %#lx\n", hr);
    check_persist_options(reader, 0);

    IWICPersistStream_Release(persist);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatGCE), "wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    ok(hr == S_OK, "GetMetadataHandlerInfo error %#lx\n", hr);

    hr = IWICMetadataHandlerInfo_GetCLSID(info, &id);
    ok(hr == S_OK, "GetCLSID error %#lx\n", hr);
    ok(IsEqualGUID(&id, &CLSID_WICGCEMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&id));

    hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
    ok(hr == S_OK, "GetFriendlyName error %#lx\n", hr);
    ok(!lstrcmpW(name, L"Graphic Control Extension Reader"), "wrong GCE reader name %s\n", wine_dbgstr_w(name));

    IWICMetadataHandlerInfo_Release(info);
    IWICMetadataReader_Release(reader);

    IStream_Release(stream);

    hr = CoCreateInstance(&CLSID_WICGCEMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 5, "Unexpected count %u.\n", count);

    compare_metadata((IWICMetadataReader *)writer, default_data, count);

    for (i = 0; i < count; ++i)
    {
        hr = IWICMetadataWriter_RemoveValueByIndex(writer, i);
        ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);
    }
    hr = IWICMetadataWriter_RemoveValueByIndex(writer, count);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, GCE_data, sizeof(GCE_data), 0);
    load_stream(writer, GCE_data, sizeof(GCE_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_APE(void)
{
    static const char APE_data[] = { 0x21,0xff,0x0b,'H','e','l','l','o',' ','W','o','r','l','d',
                                     /*sub-block*/1,0x11,
                                     /*sub-block*/2,0x22,0x33,
                                     /*sub-block*/4,0x44,0x55,0x66,0x77,
                                     /*terminator*/0 };
    static const struct test_data td[2] =
    {
        { VT_UI1|VT_VECTOR, 0, 11, { 'H','e','l','l','o',' ','W','o','r','l','d' }, NULL, L"Application" },
        { VT_UI1|VT_VECTOR, 0, 10, { 1,0x11,2,0x22,0x33,4,0x44,0x55,0x66,0x77 }, NULL, L"Data" }
    };
    static const struct test_data default_data[2] =
    {
        { VT_EMPTY, 0, 0, { 0 }, NULL, L"Application" },
        { VT_UI1|VT_VECTOR, 0, 0, { 0 }, NULL, L"Data" }
    };
    WCHAR dataW[] = { 'd','a','t','a',0 };
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy, i;
    GUID format;
    CLSID clsid;
    PROPVARIANT id, value;

    hr = CoCreateInstance(&CLSID_WICAPEMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    test_reader_container_format(reader, &GUID_ContainerFormatGif);

    stream = create_stream(APE_data, sizeof(APE_data));

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IWICPersistStream_Load(persist, stream);
    ok(hr == S_OK, "Load error %#lx\n", hr);
    check_persist_options(reader, 0);

    IWICPersistStream_Release(persist);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatAPE), "wrong format %s\n", wine_dbgstr_guid(&format));

    PropVariantInit(&value);
    id.vt = VT_LPWSTR;
    id.pwszVal = dataW;

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    ok(hr == S_OK, "GetValue error %#lx\n", hr);
    ok(value.vt == (VT_UI1|VT_VECTOR), "unexpected vt: %i\n", id.vt);
    ok(td[1].count == value.caub.cElems, "expected cElems %d, got %ld\n", td[1].count, value.caub.cElems);
    for (i = 0; i < value.caub.cElems; i++)
        ok(td[1].value[i] == value.caub.pElems[i], "%u: expected value %#I64x, got %#x\n", i, td[1].value[i], value.caub.pElems[i]);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    ok(hr == S_OK, "GetMetadataHandlerInfo error %#lx\n", hr);

    hr = IWICMetadataHandlerInfo_GetCLSID(info, &clsid);
    ok(hr == S_OK, "GetCLSID error %#lx\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_WICAPEMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&clsid));

    hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
    ok(hr == S_OK, "GetFriendlyName error %#lx\n", hr);
    ok(!lstrcmpW(name, L"Application Extension Reader"), "wrong APE reader name %s\n", wine_dbgstr_w(name));

    IWICMetadataHandlerInfo_Release(info);
    IWICMetadataReader_Release(reader);

    IStream_Release(stream);

    hr = CoCreateInstance(&CLSID_WICAPEMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    compare_metadata((IWICMetadataReader *)writer, default_data, count);

    for (i = 0; i < count; ++i)
    {
        hr = IWICMetadataWriter_RemoveValueByIndex(writer, i);
        ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);
    }
    hr = IWICMetadataWriter_RemoveValueByIndex(writer, count);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, APE_data, sizeof(APE_data), 0);
    load_stream(writer, APE_data, sizeof(APE_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_metadata_GIF_comment(void)
{
    static const char GIF_comment_data[] = { 0x21,0xfe,
                                             /*sub-block*/5,'H','e','l','l','o',
                                             /*sub-block*/1,' ',
                                             /*sub-block*/6,'W','o','r','l','d','!',
                                             /*terminator*/0 };
    static const struct test_data td[1] =
    {
        { VT_LPSTR, 0, 12, { 0 }, "Hello World!", L"TextEntry" }
    };
    static const struct test_data default_data[1] =
    {
        { VT_LPSTR, 0, 0, { 0 }, "", L"TextEntry" }
    };
    WCHAR text_entryW[] = L"TEXTENTRY";
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT i, count, dummy;
    GUID format;
    CLSID clsid;
    PROPVARIANT id, value;

    hr = CoCreateInstance(&CLSID_WICGifCommentMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#lx\n", hr);
    if (FAILED(hr)) return;

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);

    test_reader_container_format(reader, &GUID_ContainerFormatGif);

    stream = create_stream(GIF_comment_data, sizeof(GIF_comment_data));

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IWICPersistStream_Load(persist, stream);
    ok(hr == S_OK, "Load error %#lx\n", hr);
    check_persist_options(reader, 0);

    IWICPersistStream_Release(persist);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatGifComment), "wrong format %s\n", wine_dbgstr_guid(&format));

    PropVariantInit(&value);
    id.vt = VT_LPWSTR;
    id.pwszVal = text_entryW;

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    ok(hr == S_OK, "GetValue error %#lx\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(value.pszVal, "Hello World!"), "unexpected value: %s\n", value.pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
    ok(hr == S_OK, "GetMetadataHandlerInfo error %#lx\n", hr);

    hr = IWICMetadataHandlerInfo_GetCLSID(info, &clsid);
    ok(hr == S_OK, "GetCLSID error %#lx\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_WICGifCommentMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&clsid));

    hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
    ok(hr == S_OK, "GetFriendlyName error %#lx\n", hr);
    ok(!lstrcmpW(name, L"Comment Extension Reader"), "wrong APE reader name %s\n", wine_dbgstr_w(name));

    IWICMetadataHandlerInfo_Release(info);
    IWICMetadataReader_Release(reader);

    IStream_Release(stream);

    hr = CoCreateInstance(&CLSID_WICGifCommentMetadataWriter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataWriter, (void **)&writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    compare_metadata((IWICMetadataReader *)writer, default_data, count);

    for (i = 0; i < count; ++i)
    {
        hr = IWICMetadataWriter_RemoveValueByIndex(writer, i);
        ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);
    }
    hr = IWICMetadataWriter_RemoveValueByIndex(writer, count);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    load_stream(writer, GIF_comment_data, sizeof(GIF_comment_data), 0);
    load_stream(writer, GIF_comment_data, sizeof(GIF_comment_data), WICPersistOptionNoCacheStream);

    IWICMetadataWriter_Release(writer);
}

static void test_WICMapGuidToShortName(void)
{
    HRESULT hr;
    UINT len;
    WCHAR name[16];

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, name, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 8, "got %u\n", len);
    ok(!lstrcmpW(name, L"unknown"), "got %s\n", wine_dbgstr_w(name));

    name[0] = 0;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, name, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!lstrcmpW(name, L"unknown"), "got %s\n", wine_dbgstr_w(name));

    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, NULL, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 8, "got %u\n", len);

    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 0, NULL, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 8, "got %u\n", len);

    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 0, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, NULL, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = WICMapGuidToShortName(&GUID_NULL, 0, NULL, NULL);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#lx\n", hr);

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 4, name, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "got %#lx\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(!lstrcmpW(name, L"unk"), "got %s\n", wine_dbgstr_w(name));

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 0, name, &len);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(!name[0], "got %s\n", wine_dbgstr_w(name));

    hr = WICMapGuidToShortName(NULL, 8, name, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);
}

static void test_WICMapShortNameToGuid(void)
{
    HRESULT hr;
    GUID guid;

    hr = WICMapShortNameToGuid(NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = WICMapShortNameToGuid(NULL, &guid);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = WICMapShortNameToGuid(L"unknown", NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = WICMapShortNameToGuid(L"unk", &guid);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#lx\n", hr);

    hr = WICMapShortNameToGuid(L"unknown", &guid);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(IsEqualGUID(&guid, &GUID_MetadataFormatUnknown), "got %s\n", wine_dbgstr_guid(&guid));

    hr = WICMapShortNameToGuid(L"xmp", &guid);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(IsEqualGUID(&guid, &GUID_MetadataFormatXMP), "got %s\n", wine_dbgstr_guid(&guid));

    guid = GUID_NULL;
    hr = WICMapShortNameToGuid(L"XmP", &guid);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(IsEqualGUID(&guid, &GUID_MetadataFormatXMP), "got %s\n", wine_dbgstr_guid(&guid));
}

static const GUID *guid_list[] =
{
    &GUID_ContainerFormatBmp,
    &GUID_ContainerFormatPng,
    &GUID_ContainerFormatIco,
    &GUID_ContainerFormatJpeg,
    &GUID_ContainerFormatTiff,
    &GUID_ContainerFormatGif,
    &GUID_ContainerFormatWmp,
    &GUID_MetadataFormatUnknown,
    &GUID_MetadataFormatIfd,
    &GUID_MetadataFormatSubIfd,
    &GUID_MetadataFormatExif,
    &GUID_MetadataFormatGps,
    &GUID_MetadataFormatInterop,
    &GUID_MetadataFormatApp0,
    &GUID_MetadataFormatApp1,
    &GUID_MetadataFormatApp13,
    &GUID_MetadataFormatIPTC,
    &GUID_MetadataFormatIRB,
    &GUID_MetadataFormat8BIMIPTC,
    &GUID_MetadataFormat8BIMResolutionInfo,
    &GUID_MetadataFormat8BIMIPTCDigest,
    &GUID_MetadataFormatXMP,
    &GUID_MetadataFormatThumbnail,
    &GUID_MetadataFormatChunktEXt,
    &GUID_MetadataFormatXMPStruct,
    &GUID_MetadataFormatXMPBag,
    &GUID_MetadataFormatXMPSeq,
    &GUID_MetadataFormatXMPAlt,
    &GUID_MetadataFormatLSD,
    &GUID_MetadataFormatIMD,
    &GUID_MetadataFormatGCE,
    &GUID_MetadataFormatAPE,
    &GUID_MetadataFormatJpegChrominance,
    &GUID_MetadataFormatJpegLuminance,
    &GUID_MetadataFormatJpegComment,
    &GUID_MetadataFormatGifComment,
    &GUID_MetadataFormatChunkgAMA,
    &GUID_MetadataFormatChunkbKGD,
    &GUID_MetadataFormatChunkiTXt,
    &GUID_MetadataFormatChunkcHRM,
    &GUID_MetadataFormatChunkhIST,
    &GUID_MetadataFormatChunkiCCP,
    &GUID_MetadataFormatChunksRGB,
    &GUID_MetadataFormatChunktIME
};

static WCHAR rdf_scheme[] = { 'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g','/','1','9','9','9','/','0','2','/','2','2','-','r','d','f','-','s','y','n','t','a','x','-','n','s','#',0 };
static WCHAR dc_scheme[] = { 'h','t','t','p',':','/','/','p','u','r','l','.','o','r','g','/','d','c','/','e','l','e','m','e','n','t','s','/','1','.','1','/',0 };
static WCHAR xmp_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/',0 };
static WCHAR xmpidq_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','m','p','/','I','d','e','n','t','i','f','i','e','r','/','q','u','a','l','/','1','.','0','/',0 };
static WCHAR xmpRights_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','r','i','g','h','t','s','/',0 };
static WCHAR xmpMM_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','m','m','/',0 };
static WCHAR xmpBJ_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','b','j','/',0 };
static WCHAR xmpTPg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','t','/','p','g','/',0 };
static WCHAR pdf_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','p','d','f','/','1','.','3','/',0 };
static WCHAR photoshop_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','p','h','o','t','o','s','h','o','p','/','1','.','0','/',0 };
static WCHAR tiff_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','t','i','f','f','/','1','.','0','/',0 };
static WCHAR exif_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','e','x','i','f','/','1','.','0','/',0 };
static WCHAR stDim_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','D','i','m','e','n','s','i','o','n','s','#',0 };
static WCHAR xapGImg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','g','/','i','m','g','/',0 };
static WCHAR stEvt_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','R','e','s','o','u','r','c','e','E','v','e','n','t','#',0 };
static WCHAR stRef_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','R','e','s','o','u','r','c','e','R','e','f','#',0 };
static WCHAR stVer_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','V','e','r','s','i','o','n','#',0 };
static WCHAR stJob_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','J','o','b','#',0 };
static WCHAR aux_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','e','x','i','f','/','1','.','0','/','a','u','x','/',0 };
static WCHAR crs_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','c','a','m','e','r','a','-','r','a','w','-','s','e','t','t','i','n','g','s','/','1','.','0','/',0 };
static WCHAR xmpDM_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','m','p','/','1','.','0','/','D','y','n','a','m','i','c','M','e','d','i','a','/',0 };
static WCHAR Iptc4xmpCore_scheme[] = { 'h','t','t','p',':','/','/','i','p','t','c','.','o','r','g','/','s','t','d','/','I','p','t','c','4','x','m','p','C','o','r','e','/','1','.','0','/','x','m','l','n','s','/',0 };
static WCHAR MicrosoftPhoto_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','0','/',0 };
static WCHAR MP_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/',0 };
static WCHAR MPRI_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/','t','/','R','e','g','i','o','n','I','n','f','o','#',0 };
static WCHAR MPReg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/','t','/','R','e','g','i','o','n','#',0 };

static WCHAR *schema_list[] =
{
    aux_scheme,
    rdf_scheme,
    dc_scheme,
    xmp_scheme,
    xmpidq_scheme,
    xmpRights_scheme,
    xmpMM_scheme,
    xmpBJ_scheme,
    xmpTPg_scheme,
    pdf_scheme,
    photoshop_scheme,
    tiff_scheme,
    exif_scheme,
    stDim_scheme,
    xapGImg_scheme,
    stEvt_scheme,
    stRef_scheme,
    stVer_scheme,
    stJob_scheme,
    crs_scheme,
    xmpDM_scheme,
    Iptc4xmpCore_scheme,
    MicrosoftPhoto_scheme,
    MP_scheme,
    MPRI_scheme,
    MPReg_scheme
};

static void test_WICMapSchemaToName(void)
{
    static WCHAR schemaW[] = L"http://ns.adobe.com/xap/1.0/";
    static WCHAR SCHEMAW[] = L"HTTP://ns.adobe.com/xap/1.0/";
    HRESULT hr;
    UINT len, i, j;
    WCHAR name[16];

    hr = WICMapSchemaToName(&GUID_MetadataFormatUnknown, NULL, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = WICMapSchemaToName(&GUID_MetadataFormatUnknown, schemaW, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = WICMapSchemaToName(&GUID_MetadataFormatUnknown, schemaW, 0, NULL, &len);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#lx\n", hr);

    hr = WICMapSchemaToName(NULL, schemaW, 0, NULL, &len);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 0, NULL, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 4, "got %u\n", len);

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 4, NULL, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 4, "got %u\n", len);

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, SCHEMAW, 0, NULL, &len);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#lx\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 4, name, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 4, "got %u\n", len);
    ok(!lstrcmpW(name, L"xmp"), "got %s\n", wine_dbgstr_w(name));

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 0, name, &len);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 3, name, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "got %#lx\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(!lstrcmpW(name, L"xm"), "got %s\n", wine_dbgstr_w(name));

    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 4, name, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    /* Check whether modern schemas are supported */
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schema_list[0], 0, NULL, &len);
    if (hr == WINCODEC_ERR_PROPERTYNOTFOUND)
    {
        win_skip("Modern schemas are not supported\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(guid_list); i++)
    {
        for (j = 0; j < ARRAY_SIZE(schema_list); j++)
        {
            hr = WICMapSchemaToName(guid_list[i], schema_list[j], 0, NULL, &len);
            if (IsEqualGUID(guid_list[i], &GUID_MetadataFormatXMP) ||
                IsEqualGUID(guid_list[i], &GUID_MetadataFormatXMPStruct))
            {
                ok(hr == S_OK, "%u: %u: format %s does not support schema %s\n",
                   i, j, wine_dbgstr_guid(guid_list[i]), wine_dbgstr_w(schema_list[j]));
            }
            else
            {
                ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "%u: %u: format %s supports schema %s\n",
                   i, j, wine_dbgstr_guid(guid_list[i]), wine_dbgstr_w(schema_list[j]));
            }
        }
    }
}

struct metadata_item
{
    const char *schema, *id_str;
    UINT id, type, value;
};

struct metadata_block
{
    const GUID *metadata_format;
    UINT count;
    const struct metadata_item *item;
};

struct metadata
{
    const GUID *container_format;
    UINT count;
    const struct metadata_block *block;
};

static char the_best[] = "The Best";
static char the_worst[] = "The Worst";

struct test_writer
{
    IWICMetadataWriter IWICMetadataWriter_iface;
    LONG refcount;
    const struct metadata_block *block;
};

static inline struct test_writer *impl_from_IWICMetadataWriter(IWICMetadataWriter *iface)
{
    return CONTAINING_RECORD(iface, struct test_writer, IWICMetadataWriter_iface);
}

static HRESULT WINAPI test_writer_QueryInterface(IWICMetadataWriter *iface, REFIID iid, void **out)
{
    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IWICMetadataReader) ||
        IsEqualIID(iid, &IID_IWICMetadataWriter))
    {
        *out = iface;
        IWICMetadataWriter_AddRef(iface);
        return S_OK;
    }

    ok(0, "unknown iid %s\n", wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_writer_AddRef(IWICMetadataWriter *iface)
{
    struct test_writer *writer = impl_from_IWICMetadataWriter(iface);
    return InterlockedIncrement(&writer->refcount);
}

static ULONG WINAPI test_writer_Release(IWICMetadataWriter *iface)
{
    struct test_writer *writer = impl_from_IWICMetadataWriter(iface);
    ULONG refcount = InterlockedDecrement(&writer->refcount);

    if (!refcount)
        free(writer);

    return refcount;
}

static HRESULT WINAPI test_writer_GetMetadataFormat(IWICMetadataWriter *iface, GUID *format)
{
    struct test_writer *writer = impl_from_IWICMetadataWriter(iface);

    *format = *writer->block->metadata_format;
    return S_OK;
}

static HRESULT WINAPI test_writer_GetMetadataHandlerInfo(IWICMetadataWriter *iface, IWICMetadataHandlerInfo **handler)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_writer_GetCount(IWICMetadataWriter *iface, UINT *count)
{
    struct test_writer *writer = impl_from_IWICMetadataWriter(iface);

    *count = writer->block->count;
    return S_OK;
}

static HRESULT WINAPI test_writer_GetValueByIndex(IWICMetadataWriter *iface, UINT index,
        PROPVARIANT *schema, PROPVARIANT *id, PROPVARIANT *value)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static int propvar_cmp(const PROPVARIANT *v1, LONGLONG value2)
{
    LONGLONG value1;

    if (PropVariantToInt64(v1, &value1) != S_OK) return -1;

    value1 -= value2;
    if (value1) return value1 < 0 ? -1 : 1;
    return 0;
}

static char *co_strdup(const char *str)
{
    char *out = CoTaskMemAlloc(strlen(str) + 1);
    strcpy(out, str);
    return out;
}

static HRESULT WINAPI test_writer_GetValue(IWICMetadataWriter *iface, const PROPVARIANT *schema, const PROPVARIANT *id, PROPVARIANT *value)
{
    struct test_writer *writer = impl_from_IWICMetadataWriter(iface);
    UINT i;

    ok(schema != NULL && id != NULL && value != NULL, "%p, %p, %p should not be NULL\n", schema, id, value);

    for (i = 0; i < writer->block->count; i++)
    {
        if (schema->vt != VT_EMPTY)
        {
            if (!writer->block->item[i].schema)
                continue;

            switch (schema->vt)
            {
            case VT_LPSTR:
                if (lstrcmpA(schema->pszVal, writer->block->item[i].schema) != 0)
                    continue;
                break;

            case VT_LPWSTR:
            {
                char schemaA[256];
                WideCharToMultiByte(CP_ACP, 0, schema->pwszVal, -1, schemaA, sizeof(schemaA), NULL, NULL);
                if (lstrcmpA(schemaA, writer->block->item[i].schema) != 0)
                    continue;
                break;
            }

            default:
                ok(0, "unsupported schema vt %u\n", schema->vt);
                continue;
            }
        }
        else if (writer->block->item[i].schema)
            continue;

        switch (id->vt)
        {
        case VT_LPSTR:
            if (writer->block->item[i].id_str)
            {
                if (!lstrcmpA(id->pszVal, writer->block->item[i].id_str))
                {
                    value->vt = VT_LPSTR;
                    value->pszVal = co_strdup(the_best);
                    return S_OK;
                }
                break;
            }
            break;

        case VT_LPWSTR:
            if (writer->block->item[i].id_str)
            {
                char idA[256];
                WideCharToMultiByte(CP_ACP, 0, id->pwszVal, -1, idA, sizeof(idA), NULL, NULL);
                if (!lstrcmpA(idA, writer->block->item[i].id_str))
                {
                    value->vt = VT_LPSTR;
                    value->pszVal = co_strdup(the_worst);
                    return S_OK;
                }
                break;
            }
            break;

        case VT_CLSID:
            if (IsEqualGUID(id->puuid, &GUID_MetadataFormatXMP) ||
                IsEqualGUID(id->puuid, &GUID_ContainerFormatTiff))
            {
                value->vt = VT_UNKNOWN;
                value->punkVal = (IUnknown *)iface;
                IUnknown_AddRef(value->punkVal);
                return S_OK;
            }
            break;

        default:
            if (!propvar_cmp(id, writer->block->item[i].id))
            {
                value->vt = writer->block->item[i].type;
                value->uiVal = writer->block->item[i].value;
                return S_OK;
            }
            break;
        }
    }

    return 0xdeadbeef;
}

static HRESULT WINAPI test_writer_GetEnumerator(IWICMetadataWriter *iface, IWICEnumMetadataItem **enumerator)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_writer_SetValue(IWICMetadataWriter *iface, const PROPVARIANT *schema,
        const PROPVARIANT *id, const PROPVARIANT *value)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_writer_SetValueByIndex(IWICMetadataWriter *iface, UINT index,
        const PROPVARIANT *schema, const PROPVARIANT *id, const PROPVARIANT *value)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_writer_RemoveValue(IWICMetadataWriter *iface, const PROPVARIANT *schema,
        const PROPVARIANT *id)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_writer_RemoveValueByIndex(IWICMetadataWriter *iface, UINT index)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static const IWICMetadataWriterVtbl test_writer_vtbl =
{
    test_writer_QueryInterface,
    test_writer_AddRef,
    test_writer_Release,
    test_writer_GetMetadataFormat,
    test_writer_GetMetadataHandlerInfo,
    test_writer_GetCount,
    test_writer_GetValueByIndex,
    test_writer_GetValue,
    test_writer_GetEnumerator,
    test_writer_SetValue,
    test_writer_SetValueByIndex,
    test_writer_RemoveValue,
    test_writer_RemoveValueByIndex,
};

static IWICMetadataWriter *create_test_writer(const struct metadata_block *block)
{
    struct test_writer *writer;

    writer = calloc(1, sizeof(*writer));
    writer->IWICMetadataWriter_iface.lpVtbl = &test_writer_vtbl;
    writer->refcount = 1;
    writer->block = block;

    return &writer->IWICMetadataWriter_iface;
}

struct test_block_writer
{
    IWICMetadataBlockWriter IWICMetadataBlockWriter_iface;
    LONG refcount;

    IWICMetadataWriter **writers;
    unsigned int count;

    GUID container_format;
};

struct test_block_enumerator
{
    IEnumUnknown IEnumUnknown_iface;
    LONG refcount;

    IUnknown **objects;
    unsigned int count;
    unsigned int pos;
};

static inline struct test_block_writer *impl_from_IWICMetadataBlockWriter(IWICMetadataBlockWriter *iface)
{
    return CONTAINING_RECORD(iface, struct test_block_writer, IWICMetadataBlockWriter_iface);
}

static inline struct test_block_enumerator *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct test_block_enumerator, IEnumUnknown_iface);
}

static HRESULT WINAPI test_block_writer_QueryInterface(IWICMetadataBlockWriter *iface, REFIID iid, void **out)
{
    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IWICMetadataBlockReader) ||
        IsEqualIID(iid, &IID_IWICMetadataBlockWriter))
    {
        *out = iface;
        IWICMetadataBlockWriter_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_block_writer_AddRef(IWICMetadataBlockWriter *iface)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);
    return InterlockedIncrement(&writer->refcount);
}

static ULONG WINAPI test_block_writer_Release(IWICMetadataBlockWriter *iface)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);
    ULONG refcount = InterlockedDecrement(&writer->refcount);
    unsigned int i;

    if (!refcount)
    {
        for (i = 0; i < writer->count; ++i)
            IWICMetadataWriter_Release(writer->writers[i]);
        free(writer->writers);
        free(writer);
    }

    return refcount;
}

static HRESULT WINAPI test_block_writer_GetContainerFormat(IWICMetadataBlockWriter *iface, GUID *format)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);

    *format = writer->container_format;
    return S_OK;
}

static HRESULT WINAPI test_block_writer_GetCount(IWICMetadataBlockWriter *iface, UINT *count)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);

    *count = writer->count;
    return S_OK;
}

static HRESULT WINAPI test_block_writer_GetReaderByIndex(IWICMetadataBlockWriter *iface, UINT index, IWICMetadataReader **out)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);

    *out = NULL;

    if (index >= writer->count)
        return E_INVALIDARG;

    *out = (IWICMetadataReader *)writer->writers[index];
    IWICMetadataReader_AddRef(*out);

    return S_OK;
}

static HRESULT WINAPI test_block_enumerator_QueryInterface(IEnumUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IEnumUnknown) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IEnumUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_block_enumerator_AddRef(IEnumUnknown *iface)
{
    struct test_block_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    return InterlockedIncrement(&enumerator->refcount);
}

static ULONG WINAPI test_block_enumerator_Release(IEnumUnknown *iface)
{
    struct test_block_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    ULONG refcount = InterlockedDecrement(&enumerator->refcount);

    if (!refcount)
    {
        for (int i = 0; i < enumerator->count; ++i)
            IUnknown_Release(enumerator->objects[i]);
        free(enumerator->objects);
        free(enumerator);
    }

    return refcount;
}

static HRESULT WINAPI test_block_enumerator_Next(IEnumUnknown *iface, ULONG count, IUnknown **ret, ULONG *fetched)
{
    struct test_block_enumerator *enumerator = impl_from_IEnumUnknown(iface);
    ULONG tmp;

    if (!fetched) fetched = &tmp;

    *fetched = 0;

    while (enumerator->pos < enumerator->count && *fetched < count)
    {
        *ret = enumerator->objects[enumerator->pos++];
        IUnknown_AddRef(*ret);

        *fetched = *fetched + 1;
        ret++;
    }

    return *fetched == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI test_block_enumerator_Skip(IEnumUnknown *iface, ULONG count)
{
    ok(0, "%s\n", __FUNCTION__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_block_enumerator_Reset(IEnumUnknown *iface)
{
    ok(0, "%s\n", __FUNCTION__);
    return E_NOTIMPL;
}

static HRESULT WINAPI test_block_enumerator_Clone(IEnumUnknown *iface, IEnumUnknown **ret)
{
    ok(0, "%s\n", __FUNCTION__);
    return E_NOTIMPL;
}

static const IEnumUnknownVtbl test_block_enumerator_vtbl =
{
    test_block_enumerator_QueryInterface,
    test_block_enumerator_AddRef,
    test_block_enumerator_Release,
    test_block_enumerator_Next,
    test_block_enumerator_Skip,
    test_block_enumerator_Reset,
    test_block_enumerator_Clone,
};

static HRESULT WINAPI test_block_writer_GetEnumerator(IWICMetadataBlockWriter *iface, IEnumUnknown **enumerator)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);
    struct test_block_enumerator *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumUnknown_iface.lpVtbl = &test_block_enumerator_vtbl;
    object->refcount = 1;
    object->objects = calloc(writer->count, sizeof(*object->objects));
    object->count = writer->count;

    for (int i = 0; i < writer->count; ++i)
    {
        object->objects[i] = (IUnknown *)writer->writers[i];
        IUnknown_AddRef(object->objects[i]);
    }

    *enumerator = &object->IEnumUnknown_iface;

    return S_OK;
}

static HRESULT WINAPI test_block_writer_InitializeFromBlockReader(IWICMetadataBlockWriter *iface,
        IWICMetadataBlockReader *reader)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_block_writer_GetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter **out)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);

    *out = NULL;

    if (index >= writer->count)
        return E_INVALIDARG;

    *out = writer->writers[index];
    IWICMetadataWriter_AddRef(*out);

    return S_OK;
}

static HRESULT WINAPI test_block_writer_AddWriter(IWICMetadataBlockWriter *iface,
        IWICMetadataWriter *_writer)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);

    writer->writers = realloc(writer->writers, (writer->count + 1) * sizeof(*writer->writers));
    writer->writers[writer->count++] = _writer;
    IWICMetadataWriter_AddRef(_writer);

    return S_OK;
}

static HRESULT WINAPI test_block_writer_SetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter *writer)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_block_writer_RemoveWriterByIndex(IWICMetadataBlockWriter *iface, UINT index)
{
    struct test_block_writer *writer = impl_from_IWICMetadataBlockWriter(iface);

    if (index >= writer->count)
        return E_INVALIDARG;

    IWICMetadataWriter_Release(writer->writers[index]);
    writer->writers[index] = NULL;
    writer->count--;
    if (index != writer->count)
        memcpy(&writer->writers[index], &writer->writers[index + 1], (writer->count - index) * sizeof(*writer->writers));

    return S_OK;
}

static const IWICMetadataBlockWriterVtbl test_block_writer_vtbl =
{
    test_block_writer_QueryInterface,
    test_block_writer_AddRef,
    test_block_writer_Release,
    test_block_writer_GetContainerFormat,
    test_block_writer_GetCount,
    test_block_writer_GetReaderByIndex,
    test_block_writer_GetEnumerator,
    test_block_writer_InitializeFromBlockReader,
    test_block_writer_GetWriterByIndex,
    test_block_writer_AddWriter,
    test_block_writer_SetWriterByIndex,
    test_block_writer_RemoveWriterByIndex,
};

static HRESULT create_test_block_writer(const GUID *container_format, IWICMetadataBlockWriter **writer)
{
    struct test_block_writer *object;

    object = calloc(1, sizeof(*object));
    object->IWICMetadataBlockWriter_iface.lpVtbl = &test_block_writer_vtbl;
    object->refcount = 1;
    object->container_format = *container_format;

    *writer = &object->IWICMetadataBlockWriter_iface;

    return S_OK;
}

static const char xmp[] = "http://ns.adobe.com/xap/1.0/";
static const char dc[] = "http://purl.org/dc/elements/1.1/";
static const char tiff[] = "http://ns.adobe.com/tiff/1.0/";

static const struct metadata_item item1[] =
{
    { NULL, NULL, 1, VT_I2, 3 }
};

static const struct metadata_item item2[] =
{
    { NULL, NULL, 1, VT_I2, 3 },
    { "xmp", "Rating", 4, VT_I4, 6 },
    { NULL, "Rating", 7, VT_UI2, 9 }
};

static const struct metadata_item item3[] =
{
    { NULL, NULL, 1, VT_I2, 3 },
    { NULL, NULL, 4, VT_I4, 6 },
    { NULL, NULL, 7, VT_UI4, 9 },
    { NULL, NULL, 10, VT_BOOL, 12 }
};

static const struct metadata_item item4[] =
{
    { NULL, NULL, 1, VT_I2, 3 },
    { xmp, "Rating", 4, VT_I4, 6 },
    { dc, NULL, 7, VT_UI4, 9 },
    { tiff, NULL, 10, VT_BOOL, 12 },
    { NULL, "RATING", 13, VT_I4, 15 },
    { NULL, "R}ATING", 16, VT_I2, 18 },
    { NULL, "xmp", 19, VT_UI4, 21 }
};

static const struct metadata_block block1[] =
{
    { &GUID_MetadataFormatIfd, 1, item1 }
};

static const struct metadata_block block2[] =
{
    { &GUID_MetadataFormatXMP, 1, item1 },
    { &GUID_MetadataFormatIfd, 3, item2 }
};

static const struct metadata_block block3[] =
{
    { &GUID_MetadataFormatXMP, 1, item1 },
    { &GUID_MetadataFormatIfd, 3, item2 },
    { &GUID_MetadataFormatXMP, 4, item3 },
    { &GUID_MetadataFormatXMP, 7, item4 },
    { &GUID_MetadataFormatIfd, 7, item4 }
};

static const struct metadata data1 =
{
    &GUID_ContainerFormatGif,
    1, block1
};

static const struct metadata data2 =
{
    &GUID_ContainerFormatTiff,
    2, block2
};

static const struct metadata data3 =
{
    &GUID_ContainerFormatPng,
    5, block3
};

static HRESULT create_query_reader_from_metadata_reader(IWICComponentFactory *factory, IWICMetadataReader *metadata_reader,
        const GUID *container_format, IWICMetadataQueryReader **query_reader)
{
    IWICMetadataBlockWriter *block_writer = NULL;
    IWICMetadataWriter *writer;
    HRESULT hr;

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, metadata_reader, NULL, &writer);

    if (SUCCEEDED(hr))
        hr = create_test_block_writer(container_format, &block_writer);

    if (SUCCEEDED(hr))
        hr = IWICMetadataBlockWriter_AddWriter(block_writer, writer);

    if (SUCCEEDED(hr))
        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory,
                (IWICMetadataBlockReader *)block_writer, query_reader);

    if (writer)
        IWICMetadataWriter_Release(writer);
    if (block_writer)
        IWICMetadataBlockWriter_Release(block_writer);

    return hr;
}

static HRESULT create_query_writer_from_metadata_reader(IWICComponentFactory *factory, IWICMetadataReader *metadata_reader,
        const GUID *container_format, IWICMetadataQueryWriter **query_writer)
{
    IWICMetadataBlockWriter *block_writer = NULL;
    IWICMetadataWriter *writer;
    HRESULT hr;

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, metadata_reader, NULL, &writer);

    if (SUCCEEDED(hr))
        hr = create_test_block_writer(container_format, &block_writer);

    if (SUCCEEDED(hr))
        hr = IWICMetadataBlockWriter_AddWriter(block_writer, writer);

    if (SUCCEEDED(hr))
        hr = IWICComponentFactory_CreateQueryWriterFromBlockWriter(factory, block_writer, query_writer);

    if (writer)
        IWICMetadataWriter_Release(writer);
    if (block_writer)
        IWICMetadataBlockWriter_Release(block_writer);

    return hr;
}

static HRESULT create_test_block_writer_init(const struct metadata *data, IWICMetadataBlockWriter **block_writer)
{
    IWICMetadataWriter *writer;
    unsigned int i;
    HRESULT hr;

    hr = create_test_block_writer(data->container_format, block_writer);
    if (SUCCEEDED(hr))
    {
        for (i = 0; i < data->count; ++i)
        {
            writer = create_test_writer(&data->block[i]);
            ok(!!writer, "Failed to create a writer.\n");
            hr = IWICMetadataBlockWriter_AddWriter(*block_writer, writer);
            ok(hr == S_OK, "Failed to add a writer, hr %#lx.\n", hr);
            IWICMetadataWriter_Release(writer);
        }
    }

    return hr;
}

static HRESULT create_query_reader(IWICComponentFactory *factory, const struct metadata *data,
        IWICMetadataQueryReader **reader)
{
    IWICMetadataBlockWriter *block_writer;
    HRESULT hr;

    hr = create_test_block_writer_init(data, &block_writer);

    if (SUCCEEDED(hr))
    {
        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory,
                (IWICMetadataBlockReader *)block_writer, reader);
        IWICMetadataBlockWriter_Release(block_writer);
    }

    return hr;
}

static void test_queryreader(void)
{
    static const struct
    {
        BOOL todo;
        const struct metadata *data;
        const WCHAR *query;
        HRESULT hr;
        UINT vt, value;
        const char *str_value;
    } test_data[] =
    {
        { FALSE, &data1, L"/ifd/{uchar=1}", S_OK, 2, 3, NULL },
        { FALSE, &data2, L"/ifd/xmp:{long=4}", S_OK, VT_I4, 6, NULL },
        { FALSE, &data2, L"/ifd/{str=xmp}:{uint=4}", S_OK, VT_I4, 6, NULL },
        { FALSE, &data3, L"/xmp/{char=7}", 0xdeadbeef },
        { FALSE, &data3, L"/[1]xmp/{short=7}", S_OK, VT_UI4, 9, NULL },
        { FALSE, &data3, L"/[1]ifd/{str=dc}:{uint=7}", 0xdeadbeef },
        { FALSE, &data3, L"/[1]ifd/{str=http://purl.org/dc/elements/1.1/}:{longlong=7}", S_OK, VT_UI4, 9, NULL },
        { FALSE, &data3, L"/[1]ifd/{str=http://ns.adobe.com/tiff/1.0/}:{int=10}", S_OK, 11, 12, NULL },
        { FALSE, &data3, L"/[2]xmp/xmp:{ulong=4}", S_OK, VT_I4, 6, NULL },
        { FALSE, &data3, L"/[2]xmp/{str=xmp}:{ulong=4}", 0xdeadbeef },

        { FALSE, &data3, L"/xmp", S_OK, VT_UNKNOWN, 0, NULL },
        { FALSE, &data3, L"/ifd/xmp", S_OK, VT_UNKNOWN, 0, NULL },
        { FALSE, &data3, L"/ifd/xmp/tiff", S_OK, VT_UNKNOWN, 0, NULL },
        { FALSE, &data3, L"/[0]ifd/[0]xmp/[0]tiff", S_OK, VT_UNKNOWN, 0, NULL },
        { TRUE, &data3, L"/[*]xmp", S_OK, VT_LPSTR, 0, the_worst },

        { FALSE, &data3, L"/ifd/\\Rating", S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, L"/[0]ifd/Rating", S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, L"/[2]xmp/xmp:{str=Rating}", S_OK, VT_LPSTR, 0, the_best },
        { FALSE, &data3, L"/[2]xmp/xmp:Rating", S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, L"/[1]ifd/{str=http://ns.adobe.com/xap/1.0/}:Rating", S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, L"/[1]ifd/{str=http://ns.adobe.com/xap/1.0/}:{str=Rating}", S_OK, VT_LPSTR, 0, the_best },
        { FALSE, &data3, L"/[1]ifd/{wstr=\\RATING}", S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, L"/[1]ifd/{str=R\\ATING}", S_OK, VT_LPSTR, 0, the_best },
        { FALSE, &data3, L"/[1]ifd/{str=R\\}ATING}", S_OK, VT_LPSTR, 0, the_best },

        { FALSE, &data1, L"[0]/ifd/Rating", WINCODEC_ERR_PROPERTYNOTSUPPORTED },
        { FALSE, &data1, L"/[+1]ifd/Rating", WINCODEC_ERR_INVALIDQUERYCHARACTER },
        { FALSE, &data1, L"/[-1]ifd/Rating", WINCODEC_ERR_INVALIDQUERYCHARACTER },
        { FALSE, &data1, L"/ifd/{\\str=Rating}", WINCODEC_ERR_WRONGSTATE },
        { FALSE, &data1, L"/ifd/{badtype=0}", WINCODEC_ERR_WRONGSTATE },
        { TRUE, &data1, L"/ifd/{uint=0x1234}", DISP_E_TYPEMISMATCH },
        { TRUE, &data1, L"/ifd/[0]Rating", E_INVALIDARG },
        { TRUE, &data1, L"/ifd/[*]Rating", WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT },
    };
    IWICMetadataBlockWriter *block_writer;
    HRESULT hr;
    IWICComponentFactory *factory;
    IWICMetadataQueryReader *reader;
    IWICMetadataWriter *writer;
    IEnumString *enum_string;
    GUID format;
    PROPVARIANT value;
    UINT i, count;
    WCHAR *str;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        winetest_push_context("%u", i);

        hr = create_query_reader(factory, test_data[i].data, &reader);
        ok(hr == S_OK, "Failed to create a query reader, hr %#lx.\n", hr);

        hr = IWICMetadataQueryReader_GetContainerFormat(reader, &format);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&format, test_data[i].data->container_format), "Expected %s, got %s.\n",
                wine_dbgstr_guid(test_data[i].data->container_format), wine_dbgstr_guid(&format));

        PropVariantInit(&value);
        hr = IWICMetadataQueryReader_GetMetadataByName(reader, test_data[i].query, &value);
        todo_wine_if(test_data[i].todo)
        ok(hr == test_data[i].hr, "Expected %#lx, got %#lx.\n", test_data[i].hr, hr);
        if (hr == S_OK)
        {
            ok(value.vt == test_data[i].vt, "Expected %u, got %u.\n", test_data[i].vt, value.vt);
            if (test_data[i].vt == value.vt)
            {
                if (value.vt == VT_UNKNOWN)
                {
                    IWICMetadataQueryReader *new_reader;
                    WCHAR location[256];
                    UINT len;

                    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&new_reader);
                    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

                    location[0] = 0;
                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 256, location, &len);
                    ok(hr == S_OK, "GetLocation error %#lx\n", hr);
                    ok(len == lstrlenW(test_data[i].query) + 1, "expected %u, got %u\n", lstrlenW(test_data[i].query) + 1, len);
                    ok(!lstrcmpW(location, test_data[i].query), "expected %s, got %s\n", wine_dbgstr_w(test_data[i].query), wine_dbgstr_w(location));

                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 256, location, NULL);
                    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

                    location[0] = 0;
                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 3, location, &len);
                    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "got %#lx\n", hr);
                    ok(len == 0xdeadbeef, "got %u\n", len);
                    ok(!location[0], "got %s\n", wine_dbgstr_w(location));

                    location[0] = 0;
                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 0, location, &len);
                    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "got %#lx\n", hr);
                    ok(len == 0xdeadbeef, "got %u\n", len);
                    ok(!location[0], "got %s\n", wine_dbgstr_w(location));

                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 0, NULL, &len);
                    ok(hr == S_OK, "GetLocation error %#lx\n", hr);
                    ok(len == lstrlenW(test_data[i].query) + 1, "expected %u, got %u\n", lstrlenW(test_data[i].query) + 1, len);

                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 3, NULL, &len);
                    ok(hr == S_OK, "GetLocation error %#lx\n", hr);
                    ok(len == lstrlenW(test_data[i].query) + 1, "expected %u, got %u\n", lstrlenW(test_data[i].query) + 1, len);

                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 0, NULL, NULL);
                    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

                    IWICMetadataQueryReader_Release(new_reader);
                }
                else if (value.vt == VT_LPSTR)
                    ok(!lstrcmpA(value.pszVal, test_data[i].str_value), "Expected %s, got %s.\n",
                           test_data[i].str_value, value.pszVal);
                else
                    ok(value.uiVal == test_data[i].value, "Expected %u, got %u\n",
                           test_data[i].value, value.uiVal);
            }

            PropVariantClear(&value);
        }

        IWICMetadataQueryReader_Release(reader);

        winetest_pop_context();
    }

    /* Modify block set after query reader was created. */
    hr = create_test_block_writer_init(&data1, &block_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataBlockWriter_GetCount(block_writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory,
            (IWICMetadataBlockReader *)block_writer, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryReader_GetEnumerator(reader, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(str, L"/ifd"), "Unexpected query %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateMetadataWriter(factory, &GUID_MetadataFormatApp1, NULL, 0, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataBlockWriter_AddWriter(block_writer, writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IWICMetadataWriter_Release(writer);
    hr = IWICMetadataBlockWriter_GetCount(block_writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    /* Existing enumerator does not pick it up. */
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    IEnumString_Release(enum_string);

    hr = IWICMetadataQueryReader_GetEnumerator(reader, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(str, L"/ifd"), "Unexpected query %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(str, L"/app1"), "Unexpected query %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    IEnumString_Release(enum_string);

    IWICMetadataQueryReader_Release(reader);

    IWICMetadataBlockWriter_Release(block_writer);

    IWICComponentFactory_Release(factory);
}

static void test_metadata_query_writer(void)
{
    static struct
    {
        REFCLSID rclsid;
        BOOL wine_supports_encoder;
        BOOL metadata_supported;
        BOOL succeeds_uninitialized;
    }
    tests[] =
    {
        {&CLSID_WICBmpEncoder,   TRUE, FALSE},
        {&CLSID_WICPngEncoder,   TRUE,  TRUE},
        {&CLSID_WICJpegEncoder,  TRUE,  TRUE},
        {&CLSID_WICGifEncoder,   TRUE,  TRUE},
        {&CLSID_WICTiffEncoder,  TRUE,  TRUE},
        {&CLSID_WICWmpEncoder,  FALSE,  TRUE, TRUE},
    };

    IWICMetadataQueryWriter *querywriter, *querywriter2;
    IEnumUnknown *block_enum, *block_enum2;
    IWICMetadataBlockWriter *blockwriter;
    IWICMetadataWriter *metadata_writer;
    IWICBitmapFrameEncode *frameencode;
    IWICMetadataWriter *app1_writer;
    IWICComponentFactory *factory;
    IWICBitmapEncoder *encoder;
    ULONG ref, count, fetched;
    IEnumString *enumstring;
    IUnknown *objects[1];
    LPOLESTR olestring;
    UINT block_count;
    IStream *stream;
    unsigned int i;
    GUID format;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("%u", i);

        hr = CoCreateInstance(tests[i].rclsid, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapEncoder, (void **)&encoder);
        todo_wine_if(!tests[i].wine_supports_encoder) ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
        {
            winetest_pop_context();
            continue;
        }

        blockwriter = NULL;
        querywriter = querywriter2 = NULL;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frameencode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IWICBitmapFrameEncode_QueryInterface(frameencode, &IID_IWICMetadataBlockWriter, (void**)&blockwriter);
        ok(hr == (tests[i].metadata_supported ? S_OK : E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICComponentFactory, (void**)&factory);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IWICComponentFactory_CreateQueryWriterFromBlockWriter(factory, blockwriter, &querywriter);
        ok(hr == (tests[i].metadata_supported ? S_OK : E_INVALIDARG), "Got unexpected hr %#lx.\n", hr);

        hr = IWICBitmapFrameEncode_GetMetadataQueryWriter(frameencode, &querywriter2);
        ok(hr == (tests[i].succeeds_uninitialized ? S_OK : WINCODEC_ERR_NOTINITIALIZED),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            IWICMetadataQueryWriter_Release(querywriter2);

        hr = IWICBitmapFrameEncode_Initialize(frameencode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IWICBitmapFrameEncode_GetMetadataQueryWriter(frameencode, &querywriter2);
        ok(hr == (tests[i].metadata_supported ? S_OK : WINCODEC_ERR_UNSUPPORTEDOPERATION),
                "Got unexpected hr %#lx.\n", hr);

        if (tests[i].metadata_supported)
            ok(querywriter2 != querywriter, "Got unexpected interfaces %p, %p.\n", querywriter, querywriter2);

        IWICComponentFactory_Release(factory);
        if (querywriter)
        {
            ref = get_refcount(querywriter);
            ok(ref == 1, "Got unexpected ref %lu.\n", ref);

            hr = IWICMetadataQueryWriter_QueryInterface(querywriter, &IID_IEnumString, (void **)&enumstring);
            ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

            hr = IWICMetadataQueryWriter_GetEnumerator(querywriter, &enumstring);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            ref = get_refcount(querywriter);
            ok(ref == 1, "Got unexpected ref %lu.\n", ref);

            hr = IEnumString_Skip(enumstring, 0);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            count = 0xdeadbeef;
            hr = IEnumString_Next(enumstring, 0, NULL, &count);
            ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
            ok(count == 0xdeadbeef, "Got unexpected count %lu.\n", count);

            hr = IEnumString_Next(enumstring, 0, &olestring, &count);
            ok(hr == S_OK || hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Got unexpected hr %#lx.\n", hr);

            count = 0xdeadbeef;
            hr = IEnumString_Next(enumstring, 1, &olestring, &count);
            ok(hr == S_OK || hr == S_FALSE, "Got unexpected hr %#lx, i %u.\n", hr, i);
            ok((hr && !count) || (!hr && count == 1), "Got unexpected hr %#lx, count %lu.\n", hr, count);
            if (count)
            {
                CoTaskMemFree(olestring);

                /* IEnumString_Skip() crashes at least on Win7 when
                 * trying to skip past the string count. */
                hr = IEnumString_Reset(enumstring);
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

                hr = IEnumString_Skip(enumstring, 1);
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            }
            IEnumString_Release(enumstring);

            IWICMetadataQueryWriter_Release(querywriter);
            IWICMetadataQueryWriter_Release(querywriter2);
            IWICMetadataBlockWriter_Release(blockwriter);
        }
        IWICBitmapFrameEncode_Release(frameencode);
        IStream_Release(stream);
        IWICBitmapEncoder_Release(encoder);

        winetest_pop_context();
    }

    /* Block enumerator behavior. */
    hr = CoCreateInstance(&CLSID_WICJpegEncoder, NULL, CLSCTX_INPROC_SERVER, &IID_IWICBitmapEncoder, (void **)&encoder);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frameencode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICBitmapFrameEncode_QueryInterface(frameencode, &IID_IWICMetadataBlockWriter, (void **)&blockwriter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICBitmapFrameEncode_Initialize(frameencode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IWICMetadataBlockWriter_GetEnumerator(blockwriter, &block_enum);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

if (SUCCEEDED(hr))
{
    hr = IEnumUnknown_Next(block_enum, 1, objects, &fetched);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    IUnknown_Release(*objects);

    hr = IWICMetadataBlockWriter_GetCount(blockwriter, &block_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(block_count == 1, "Unexpected block count %u.\n", block_count);
    hr = IWICMetadataBlockWriter_GetWriterByIndex(blockwriter, 0, &metadata_writer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICMetadataWriter_GetMetadataFormat(metadata_writer, &format);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp0), "Unexpected format %s.\n", debugstr_guid(&format));
    IWICMetadataWriter_Release(metadata_writer);

    hr = CoCreateInstance(&CLSID_WICApp1MetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&app1_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataBlockWriter_AddWriter(blockwriter, app1_writer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IWICMetadataBlockWriter_GetCount(blockwriter, &block_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(block_count == 2, "Unexpected block count %u.\n", block_count);

    /* Newly added object is not picked up. */
    hr = IEnumUnknown_Next(block_enum, 1, objects, &fetched);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);
    ok(!fetched, "Unexpected count %lu.\n", fetched);

    hr = IEnumUnknown_Reset(block_enum);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IEnumUnknown_Next(block_enum, 1, objects, &fetched);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    IUnknown_Release(*objects);
    hr = IEnumUnknown_Next(block_enum, 1, objects, &fetched);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);

    /* Cloning. */
    hr = IEnumUnknown_Clone(block_enum, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IEnumUnknown_Clone(block_enum, &block_enum2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    fetched = 0;
    hr = IEnumUnknown_Next(block_enum2, 1, objects, &fetched);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    IUnknown_Release(*objects);

    IEnumUnknown_Release(block_enum);
    IEnumUnknown_Release(block_enum2);
    IWICMetadataWriter_Release(app1_writer);
}
    IWICMetadataBlockWriter_Release(blockwriter);

    IWICBitmapFrameEncode_Release(frameencode);
    IWICBitmapEncoder_Release(encoder);
    IStream_Release(stream);
}

#include "pshpack2.h"
static const struct app1_data
{
    BYTE exif_header[6];
    BYTE bom[2];
    USHORT marker;
    ULONG ifd0_offset;

    USHORT ifd0_count;
    struct IFD_entry ifd0[4];
    ULONG next_IFD;

    USHORT exif_ifd_count;
    struct IFD_entry exif_ifd[1];
    ULONG next_IFD_2;

    USHORT gps_ifd_count;
    struct IFD_entry gps_ifd[1];
    ULONG next_IFD_3;
}
app1_data =
{
    { 'E','x','i','f',0,0 },
    { 'I','I' },
    0x002a,
    0x8,

    /* IFD 0 */
    4,
    {
        { 0x100,  IFD_LONG, 1, 222 }, /* IMAGEWIDTH */
        { 0x101,  IFD_LONG, 1, 333 }, /* IMAGELENGTH */
        /* Exif IFD pointer */
        { 0x8769, IFD_LONG, 1, FIELD_OFFSET(struct app1_data, exif_ifd_count) - 6 },
        /* GPS IFD pointer */
        { 0x8825, IFD_LONG, 1, FIELD_OFFSET(struct app1_data, gps_ifd_count) - 6 },
    },
    0,

    /* Exif IFD */
    1,
    {
        { 0x200, IFD_SHORT, 1, 444 },
    },
    0,

    /* GPS IFD */
    1,
    {
        { 0x300, IFD_SHORT, 1, 555 },
    },
    0,
};
#include "poppack.h"

static void test_metadata_App1(void)
{
    IWICMetadataReader *reader, *ifd_reader, *exif_reader, *gps_reader;
    IWICMetadataQueryReader *query_reader, *query_reader2;
    IWICMetadataQueryWriter *query_writer, *query_writer2;
    IWICEnumMetadataItem *enumerator;
    IStream *app1_stream, *stream2;
    IWICComponentFactory *factory;
    PROPVARIANT schema, id, value;
    IWICMetadataWriter *writer;
    UINT length, count;
    WCHAR path[64];
    ULONG fetched;
    GUID format;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_WICApp1MetadataReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(reader, &IID_IWICMetadataReader, TRUE);
    check_interface(reader, &IID_IPersist, TRUE);
    check_interface(reader, &IID_IPersistStream, TRUE);
    check_interface(reader, &IID_IWICPersistStream, TRUE);
    check_interface(reader, &IID_IWICStreamProvider, TRUE);
    check_interface(reader, &IID_IWICMetadataBlockReader, FALSE);

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    count = 1;
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    test_reader_container_format(reader, &GUID_ContainerFormatJpeg);

    load_stream(reader, (const char *)&app1_data, sizeof(app1_data), 0);
    check_persist_options(reader, 0);
    hr = get_persist_stream(reader, &app1_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    /* Enumerator returns top level item. */
    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICEnumMetadataItem_Next(enumerator, 1, NULL, &id, &value, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected count %lu.\n", fetched);
    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0, "Unexpected id %u.\n", id.uiVal);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    ok(!!value.punkVal, "Unexpected value.\n");
    PropVariantClear(&value);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICEnumMetadataItem_Next(enumerator, 1, NULL, &id, &value, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(!fetched, "Unexpected count %lu.\n", fetched);

    IWICEnumMetadataItem_Release(enumerator);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Top level IFD reader. */
    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0, "Unexpected id %u.\n", id.uiVal);

    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    ok(!!value.punkVal, "Unexpected value.\n");

    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataReader, (void **)&ifd_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetMetadataFormat(ifd_reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    check_persist_options(ifd_reader, 0);

    hr = get_persist_stream(ifd_reader, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(stream2, &IID_IWICStream, TRUE);
    ok(!!stream2 && app1_stream != stream2, "Unexpected stream.\n");
    IStream_Release(stream2);

    hr = IWICMetadataReader_GetCount(ifd_reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(ifd_reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0x100, "Unexpected id %#x.\n", id.uiVal);
    ok(value.vt == VT_UI4, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 222, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(ifd_reader, 1, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0x101, "Unexpected id %#x.\n", id.uiVal);
    ok(value.vt == VT_UI4, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 333, "Unexpected value %lu.\n", value.ulVal);

    /* Nested handlers are accessible by format GUIDs. */
    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);
    InitPropVariantFromCLSID(&GUID_MetadataFormatExif, &id);
    hr = IWICMetadataReader_GetValue(ifd_reader, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataReader, (void **)&exif_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);
    PropVariantClear(&id);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(ifd_reader, 2, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0x8769, "Unexpected id %#x.\n", id.uiVal);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataReader, (void **)&exif_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    /* Exif IFD */
    hr = IWICMetadataReader_GetMetadataFormat(exif_reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatExif), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    check_persist_options(exif_reader, 0);

    hr = get_persist_stream(exif_reader, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(stream2, &IID_IWICStream, TRUE);
    ok(!!stream2 && stream2 != app1_stream, "Unexpected stream.\n");
    IStream_Release(stream2);

    hr = IWICMetadataReader_GetCount(exif_reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(exif_reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0x200, "Unexpected id %#x.\n", id.uiVal);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 444, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(ifd_reader, 3, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0x8825, "Unexpected id %#x.\n", id.uiVal);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataReader, (void **)&gps_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&value);

    /* GPS IFD */
    hr = IWICMetadataReader_GetMetadataFormat(gps_reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatGps), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    check_persist_options(gps_reader, 0);

    hr = get_persist_stream(gps_reader, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_interface(stream2, &IID_IWICStream, TRUE);
    ok(!!stream2 && stream2 != app1_stream, "Unexpected stream.\n");
    IStream_Release(stream2);

    hr = IWICMetadataReader_GetCount(gps_reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(gps_reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(id.vt == VT_UI2, "Unexpected id type: %u.\n", id.vt);
    ok(id.uiVal == 0x300, "Unexpected id %#x.\n", id.uiVal);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 555, "Unexpected value %lu.\n", value.ulVal);

    IWICMetadataReader_Release(gps_reader);
    IWICMetadataReader_Release(exif_reader);
    IWICMetadataReader_Release(ifd_reader);

    IStream_Release(app1_stream);

    /* Query reader. */
    hr = create_query_reader_from_metadata_reader(factory, reader, &GUID_ContainerFormatJpeg, &query_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryReader_GetLocation(query_reader, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryReader_GetContainerFormat(query_reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_ContainerFormatJpeg), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);

    check_interface(value.punkVal, &IID_IWICMetadataQueryReader, TRUE);

    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryReader_GetLocation(query_reader2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryReader_GetContainerFormat(query_reader2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryReader_Release(query_reader2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryReader_GetLocation(query_reader2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1/ifd"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryReader_GetContainerFormat(query_reader2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryReader_Release(query_reader2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/gps", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryReader_GetLocation(query_reader2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1/ifd/gps"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryReader_GetContainerFormat(query_reader2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatGps), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryReader_Release(query_reader2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/exif", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryReader_GetLocation(query_reader2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1/ifd/exif"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryReader_GetContainerFormat(query_reader2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatExif), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryReader_Release(query_reader2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/exif/{ushort=512}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 444, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/gps/{ushort=768}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 555, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/{ushort=256}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI4, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 222, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/{ushort=34665}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    check_interface(value.punkVal, &IID_IWICMetadataQueryReader, TRUE);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd/{ushort=34853}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    check_interface(value.punkVal, &IID_IWICMetadataQueryReader, TRUE);
    PropVariantClear(&value);

    IWICMetadataQueryReader_Release(query_reader);

    /* Same queries using the query writer API. */
    hr = create_query_writer_from_metadata_reader(factory, reader, &GUID_ContainerFormatJpeg, &query_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);

    check_interface(value.punkVal, &IID_IWICMetadataQueryReader, TRUE);
    check_interface(value.punkVal, &IID_IWICMetadataQueryWriter, TRUE);

    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryWriter, (void **)&query_writer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_GetLocation(query_writer2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    IWICMetadataQueryWriter_Release(query_writer2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryWriter, (void **)&query_writer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_GetLocation(query_writer2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1/ifd"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryWriter_GetContainerFormat(query_writer2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryWriter_Release(query_writer2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/gps", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryWriter, (void **)&query_writer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_GetLocation(query_writer2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1/ifd/gps"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryWriter_GetContainerFormat(query_writer2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatGps), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryWriter_Release(query_writer2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/exif", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryWriter, (void **)&query_writer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_GetLocation(query_writer2, ARRAY_SIZE(path), path, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(path, L"/app1/ifd/exif"), "Unexpected path %s.\n", wine_dbgstr_w(path));
    hr = IWICMetadataQueryWriter_GetContainerFormat(query_writer2, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatExif), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataQueryWriter_Release(query_writer2);
    PropVariantClear(&value);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/exif/{ushort=512}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 444, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/gps/{ushort=768}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 555, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/{ushort=256}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI4, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 222, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/{ushort=34665}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type: %u.\n", value.vt);
    check_interface(value.punkVal, &IID_IWICMetadataQueryReader, TRUE);
    check_interface(value.punkVal, &IID_IWICMetadataQueryWriter, TRUE);
    PropVariantClear(&value);

    IWICMetadataQueryWriter_Release(query_writer);

    IWICMetadataReader_Release(reader);

    hr = CoCreateInstance(&CLSID_WICApp1MetadataWriter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(writer, &IID_IWICMetadataWriter, TRUE);
    check_interface(writer, &IID_IWICMetadataReader, TRUE);
    check_interface(writer, &IID_IPersist, TRUE);
    check_interface(writer, &IID_IPersistStream, TRUE);
    check_interface(writer, &IID_IWICPersistStream, TRUE);
    check_interface(writer, &IID_IWICStreamProvider, TRUE);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    id.vt = VT_UI2;
    id.uiVal = 0x300;
    value.vt = VT_UI2;
    value.ulVal = 555;
    PropVariantInit(&schema);
    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IWICMetadataWriter_Release(writer);

    IWICComponentFactory_Release(factory);
}

static void test_CreateMetadataWriterFromReader(void)
{
    IWICComponentFactory *factory;
    IWICMetadataReader *reader;
    IWICMetadataWriter *writer;
    IStream *stream, *stream2;
    PROPVARIANT id, value;
    GUID format;
    HRESULT hr;
    UINT count;
    char *data;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* tEXt, uninitialized */
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt,
            NULL, 0, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(reader, 0);

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, reader, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IWICMetadataReader_Release(reader);

    check_persist_options(writer, 0);
    IWICMetadataWriter_Release(writer);

    /* tEXt, loaded */
    stream = create_stream(metadata_tEXt, sizeof(metadata_tEXt));
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatChunktEXt,
            NULL, 0, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(reader, 0);

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, NULL, NULL, &writer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, reader, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    expect_ref(reader, 1);
    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, reader, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_ref(reader, 1);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(id.vt == VT_LPSTR, "Unexpected id type %u.\n", id.vt);
    ok(!strcmp(id.pszVal, "winetest"), "Unexpected id %s.\n", wine_dbgstr_a(id.pszVal));
    ok(value.vt == VT_LPSTR, "Unexpected value type %u.\n", value.vt);
    ok(!strcmp(value.pszVal, "value"), "Unexpected value %s.\n", wine_dbgstr_a(value.pszVal));
    PropVariantClear(&id);
    PropVariantClear(&value);

    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(id.vt == VT_LPSTR, "Unexpected id type %u.\n", id.vt);
    if (id.vt == VT_LPSTR)
        ok(!strcmp(id.pszVal, "winetest"), "Unexpected id %s.\n", wine_dbgstr_a(id.pszVal));
    todo_wine
    ok(value.vt == VT_LPSTR, "Unexpected value type %u.\n", value.vt);
    if (value.vt == VT_LPSTR)
        ok(!strcmp(value.pszVal, "value"), "Unexpected value %s.\n", wine_dbgstr_a(value.pszVal));
    PropVariantClear(&id);
    PropVariantClear(&value);

    hr = get_persist_stream(writer, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stream2 != stream, "Unexpected stream.\n");
    check_interface(stream2, &IID_IWICStream, TRUE);
    IStream_Release(stream2);

    IWICMetadataWriter_Release(writer);
    IWICMetadataReader_Release(reader);
    IStream_Release(stream);

    /* App1 reader */
    stream = create_stream((const char *)&app1_data, sizeof(app1_data));
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatApp1,
            NULL, 0, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, reader, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(id.vt == VT_UI2, "Unexpected id type %u.\n", id.vt);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %u.\n", value.vt);
    check_interface(value.punkVal, &IID_IWICMetadataReader, TRUE);
    check_interface(value.punkVal, &IID_IWICMetadataWriter, FALSE);
    PropVariantClear(&value);

    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(id.vt == VT_UI2, "Unexpected id type %u.\n", id.vt);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %u.\n", value.vt);
    check_interface(value.punkVal, &IID_IWICMetadataReader, TRUE);
    check_interface(value.punkVal, &IID_IWICMetadataWriter, TRUE);
    check_persist_options(value.punkVal, 0);
    PropVariantClear(&value);

    hr = get_persist_stream(writer, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stream2 != stream, "Unexpected stream.\n");
    check_interface(stream2, &IID_IWICStream, TRUE);
    IStream_Release(stream2);

    IWICMetadataWriter_Release(writer);
    IWICMetadataReader_Release(reader);
    IStream_Release(stream);

    /* App1, reader without stream caching. */
    stream = create_stream((const char *)&app1_data, sizeof(app1_data));
    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatApp1,
            NULL, WICPersistOptionNoCacheStream, stream, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(reader, WICPersistOptionNoCacheStream);

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, reader, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, NULL, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(id.vt == VT_UI2, "Unexpected id type %u.\n", id.vt);
    ok(value.vt == VT_UNKNOWN, "Unexpected value type %u.\n", value.vt);
    check_interface(value.punkVal, &IID_IWICMetadataReader, TRUE);
    check_interface(value.punkVal, &IID_IWICMetadataWriter, TRUE);
    PropVariantClear(&value);

    IWICMetadataWriter_Release(writer);
    IWICMetadataReader_Release(reader);
    IStream_Release(stream);

    /* Big-endian IFD */
    data = malloc(sizeof(IFD_data));
    memcpy(data, &IFD_data, sizeof(IFD_data));
    byte_swap_ifd_data(data);

    hr = IWICComponentFactory_CreateMetadataReader(factory, &GUID_MetadataFormatIfd,
            NULL, 0, NULL, &reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    load_stream(reader, data, sizeof(IFD_data), WICPersistOptionBigEndian);
    check_persist_options(reader, WICPersistOptionBigEndian);
    free(data);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    hr = IWICComponentFactory_CreateMetadataWriterFromReader(factory, reader, NULL, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(writer, WICPersistOptionBigEndian);

    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    IWICMetadataWriter_Release(writer);

    IWICMetadataReader_Release(reader);

    IWICComponentFactory_Release(factory);
}

static void test_CreateMetadataWriter(void)
{
    static const struct options_test
    {
        const GUID *clsid;
        DWORD options;
    }
    options_tests[] =
    {
        { &GUID_MetadataFormatApp1, WICPersistOptionBigEndian },
        { &GUID_MetadataFormatIfd, WICPersistOptionBigEndian },
        { &GUID_MetadataFormatChunktEXt, WICPersistOptionBigEndian },
        { &GUID_MetadataFormatApp1, WICPersistOptionNoCacheStream },
        { &GUID_MetadataFormatIfd, WICPersistOptionNoCacheStream },
        { &GUID_MetadataFormatChunktEXt, WICPersistOptionNoCacheStream },
        { &GUID_MetadataFormatApp1, 0x100 },
        { &GUID_MetadataFormatApp1, 0x1000 },
    };
    IWICStreamProvider *stream_provider;
    IWICComponentFactory *factory;
    IWICMetadataWriter *writer;
    IStream *stream;
    unsigned int i;
    GUID format;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(options_tests); ++i)
    {
        const struct options_test *test = &options_tests[i];

        writer = (void *)0xdeadbeef;
        hr = IWICComponentFactory_CreateMetadataWriter(factory, test->clsid, NULL, test->options, &writer);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok(writer == (void *)0xdeadbeef, "Unexpected value %p.\n", writer);
    }

    hr = IWICComponentFactory_CreateMetadataWriter(factory, &GUID_MetadataFormatChunktEXt, NULL, 0, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(writer, 0);

    hr = IWICMetadataWriter_QueryInterface(writer, &IID_IWICStreamProvider, (void **)&stream_provider);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = (void *)0xdeadbeef;
    hr = IWICStreamProvider_GetStream(stream_provider, &stream);
    ok(hr == WINCODEC_ERR_STREAMNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
    ok(stream == (void *)0xdeadbeef, "Unexpected stream.\n");

    IWICStreamProvider_Release(stream_provider);

    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    IWICMetadataWriter_Release(writer);

    /* Invalid format */
    hr = IWICComponentFactory_CreateMetadataWriter(factory, &IID_IUnknown, NULL, 0, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataWriter_Release(writer);

    writer = (void *)0xdeadbeef;
    hr = IWICComponentFactory_CreateMetadataWriter(factory, &IID_IUnknown, NULL, WICMetadataCreationFailUnknown, &writer);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "Unexpected hr %#lx.\n", hr);
    ok(writer == (void *)0xdeadbeef, "Unexpected pointer.\n");

    /* App1 */
    hr = IWICComponentFactory_CreateMetadataWriter(factory, &GUID_MetadataFormatApp1, NULL, 0, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(writer, 0);
    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataWriter_Release(writer);

    /* Ifd */
    hr = IWICComponentFactory_CreateMetadataWriter(factory, &GUID_MetadataFormatIfd, NULL, 0, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_persist_options(writer, 0);
    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));
    IWICMetadataWriter_Release(writer);

    IWICComponentFactory_Release(factory);
}

static void test_metadata_writer(void)
{
    IWICComponentFactory *factory;
    IWICMetadataWriter *writer;
    GUID format;
    HRESULT hr;
    UINT count;
    PROPVARIANT schema, id, value;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateMetadataWriter(factory, &GUID_MetadataFormatIfd, NULL, 0, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetMetadataFormat(writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    schema.vt = VT_I4;
    schema.lVal = 100;
    id.vt = VT_UI2;
    id.uiVal = 200;
    value.vt = VT_UI4;
    value.ulVal = 300;
    hr = IWICMetadataWriter_SetValueByIndex(writer, 0, &schema, &id, &value);
    todo_wine
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_SetValue(writer, &schema, NULL, &value);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    id.vt = VT_UI2;
    id.uiVal = 201;
    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    /* Same id with differing schema does not add a new item. */
    schema.vt = VT_I4;
    schema.lVal = 101;
    hr = IWICMetadataWriter_SetValue(writer, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataWriter_GetCount(writer, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 0, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(schema.vt == VT_EMPTY, "Unexpected type %u.\n", schema.vt);
    ok(!schema.lVal, "Unexpected value %lu.\n", schema.lVal);
    ok(id.vt == VT_UI2, "Unexpected type %u.\n", id.vt);
    ok(id.uiVal == 200, "Unexpected value %u.\n", id.uiVal);
    ok(value.vt == VT_UI4, "Unexpected type %u.\n", value.vt);
    ok(value.uiVal == 300, "Unexpected value %lu.\n", value.ulVal);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);
    hr = IWICMetadataWriter_GetValueByIndex(writer, 1, &schema, &id, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(schema.vt == VT_EMPTY, "Unexpected type %u.\n", schema.vt);
    ok(!schema.lVal, "Unexpected value %lu.\n", schema.lVal);
    ok(id.vt == VT_UI2, "Unexpected type %u.\n", id.vt);
    ok(id.uiVal == 201, "Unexpected value %u.\n", id.uiVal);
    ok(value.vt == VT_UI4, "Unexpected type %u.\n", value.vt);
    ok(value.uiVal == 300, "Unexpected value %lu.\n", value.ulVal);

    value.vt = VT_UI4;
    value.ulVal = 301;
    hr = IWICMetadataWriter_SetValueByIndex(writer, 1, &schema, &id, &value);
    todo_wine
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Unexpected hr %#lx.\n", hr);

    IWICMetadataWriter_Release(writer);

    IWICComponentFactory_Release(factory);
}

static void test_CreateQueryWriter(void)
{
    IWICMetadataQueryWriter *query_writer;
    IWICImagingFactory *factory;
    IEnumString *enum_string;
    WCHAR buff[64], *str;
    PROPVARIANT value;
    ULONG fetched;
    GUID format;
    HRESULT hr;
    UINT len;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* "Unknown" format. */
    hr = IWICImagingFactory_CreateQueryWriter(factory, &GUID_MetadataFormatUnknown, NULL, &query_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryWriter_GetLocation(query_writer, ARRAY_SIZE(buff), buff, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buff, L"/unknown"), "Unexpected location %s.\n", wine_dbgstr_w(buff));

    hr = IWICMetadataQueryWriter_GetContainerFormat(query_writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/", &value);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(value.vt == VT_BLOB, "Unexpected value type %u.\n", value.vt);
        ok(!value.blob.cbSize, "Unexpected size %lu.\n", value.blob.cbSize);
        ok(!value.blob.pBlobData, "Unexpected data pointer %p.\n", value.blob.pBlobData);
        PropVariantClear(&value);
    }

    hr = IWICMetadataQueryWriter_GetEnumerator(query_writer, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Next(enum_string, 1, &str, &fetched);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(!wcscmp(str, L"/{}"), "Unexpected string %s.\n", wine_dbgstr_w(str));
        CoTaskMemFree(str);
    }
    hr = IEnumString_Next(enum_string, 1, &str, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    IEnumString_Release(enum_string);

    IWICMetadataQueryWriter_Release(query_writer);

    /* App1 */
    hr = IWICImagingFactory_CreateQueryWriter(factory, &GUID_MetadataFormatApp1, NULL, &query_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryWriter_GetLocation(query_writer, ARRAY_SIZE(buff), buff, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buff, L"/app1"), "Unexpected location %s.\n", wine_dbgstr_w(buff));

    hr = IWICMetadataQueryWriter_GetContainerFormat(query_writer, &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatApp1), "Unexpected format %s.\n", wine_dbgstr_guid(&format));

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/", &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryWriter_GetEnumerator(query_writer, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Next(enum_string, 1, &str, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    IEnumString_Release(enum_string);

    IWICMetadataQueryWriter_Release(query_writer);

    IWICImagingFactory_Release(factory);
}

static void test_CreateQueryWriterFromReader(void)
{
    IWICMetadataBlockReader *block_reader;
    IWICMetadataQueryWriter *query_writer;
    IWICMetadataQueryWriter *query_writer2;
    IWICMetadataQueryReader *query_reader, *query_reader2;
    IWICBitmapFrameDecode *decoder_frame;
    IWICComponentFactory *factory;
    IWICBitmapDecoder *decoder;
    IWICMetadataReader *reader;
    IEnumString *enum_string;
    PROPVARIANT value;
    IStream *stream;
    WCHAR buff[64];
    UINT count, len;
    WCHAR *str;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Using query reader created with CreateQueryReaderFromBlockReader().

       It's not allowed to create the writer for top level reader. */
    hr = CoCreateInstance(&CLSID_WICApp1MetadataReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    load_stream(reader, (const char *)&app1_data, sizeof(app1_data), 0);
    hr = create_query_reader_from_metadata_reader(factory, reader, &GUID_ContainerFormatJpeg, &query_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryWriterFromReader(factory, query_reader, NULL, &query_writer);
    ok(hr == WINCODEC_ERR_UNEXPECTEDMETADATATYPE, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&value);
    hr = IWICMetadataQueryReader_GetMetadataByName(query_reader, L"/app1/ifd", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "Unexpected type %d.\n", value.vt);

    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&query_reader2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryWriterFromReader(factory, query_reader2, NULL, &query_writer);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (hr == S_OK)
{
    hr = IWICMetadataQueryWriter_GetLocation(query_writer, ARRAY_SIZE(buff), buff, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buff, L"/ifd"), "Unexpected location %s.\n", wine_dbgstr_w(buff));

    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/");
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/exif");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICMetadataQueryWriter_GetEnumerator(query_writer, &enum_string);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(str, L"/{ushort=256}"), "Unexpected query %s.\n", debugstr_w(str));
    CoTaskMemFree(str);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(str, L"/{ushort=257}"), "Unexpected query %s.\n", debugstr_w(str));
    CoTaskMemFree(str);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(str, L"/{ushort=34853}"), "Unexpected query %s.\n", debugstr_w(str));
    CoTaskMemFree(str);
    hr = IEnumString_Next(enum_string, 1, &str, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    IEnumString_Release(enum_string);

    IWICMetadataQueryWriter_Release(query_writer);
}

    IWICMetadataQueryReader_Release(query_reader);
    IWICMetadataQueryReader_Release(query_reader2);

    PropVariantClear(&value);

    /* IWICMetadataReader -> IWICMetadataWriter -> IWICMetadataQueryWriter -> CreateQueryWriterFromReader(). */
    hr = create_query_writer_from_metadata_reader(factory, reader, &GUID_ContainerFormatJpeg, &query_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryWriterFromReader(factory, (IWICMetadataQueryReader *)query_writer,
            NULL, &query_writer2);
    ok(hr == WINCODEC_ERR_UNEXPECTEDMETADATATYPE, "Unexpected hr %#lx.\n", hr);
    IWICMetadataQueryWriter_Release(query_writer);

    query_writer = NULL;
    hr = IWICComponentFactory_CreateQueryWriter(factory, &GUID_MetadataFormatChunktIME, NULL, &query_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryWriterFromReader(factory, (IWICMetadataQueryReader *)query_writer,
            NULL, &query_writer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IWICMetadataQueryWriter_Release(query_writer2);
    if (query_writer)
        IWICMetadataQueryWriter_Release(query_writer);

    /* Using decoder frame. */
    hr = CoCreateInstance(&CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICBitmapDecoder, (void **)&decoder);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = create_stream(pngimage, sizeof(pngimage));
    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IStream_Release(stream);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &decoder_frame);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IWICBitmapFrameDecode_QueryInterface(decoder_frame, &IID_IWICMetadataBlockReader, (void **)&block_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapFrameDecode_GetMetadataQueryReader(decoder_frame, &query_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryWriterFromReader(factory, query_reader, &GUID_VendorMicrosoft, &query_writer);
    ok(hr == WINCODEC_ERR_UNEXPECTEDMETADATATYPE, "Unexpected hr %#lx.\n", hr);

    IWICMetadataQueryReader_Release(query_reader);
    IWICBitmapFrameDecode_Release(decoder_frame);
    IWICBitmapDecoder_Release(decoder);

    hr = IWICMetadataBlockReader_GetCount(block_reader, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, block_reader, &query_reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryWriterFromReader(factory, query_reader, &GUID_VendorMicrosoft, &query_writer);
    ok(hr == WINCODEC_ERR_UNEXPECTEDMETADATATYPE, "Unexpected hr %#lx.\n", hr);
    IWICMetadataQueryReader_Release(query_reader);

    IWICMetadataBlockReader_Release(block_reader);
    IWICMetadataReader_Release(reader);

    IWICComponentFactory_Release(factory);
}

static void test_RemoveMetadataByName(void)
{
    IWICMetadataQueryWriter *query_writer;
    IWICComponentFactory *factory;
    IWICMetadataReader *reader;
    PROPVARIANT value;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_WICApp1MetadataReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    load_stream(reader, (const char *)&app1_data, sizeof(app1_data), 0);

    hr = create_query_writer_from_metadata_reader(factory, reader, &GUID_ContainerFormatJpeg, &query_writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/exif/{ushort=512}", &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UI2, "Unexpected value type: %u.\n", value.vt);
    ok(value.ulVal == 444, "Unexpected value %lu.\n", value.ulVal);

    /* Item does not exist. */
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1/ifd/exif/{ushort=513}");
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1/ifd/exif/ifd");
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1/ifd/exif/invalid");
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1/invalid/exif");
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);

    /* Removing terminal item. */
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1/ifd/exif/{ushort=512}");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd/exif/{ushort=512}", &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);

    /* Remove at handler level. */
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1/ifd");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&value);
    hr = IWICMetadataQueryWriter_GetMetadataByName(query_writer, L"/app1/ifd", &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "Unexpected hr %#lx.\n", hr);
    /* At root */
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/");
    ok(hr == WINCODEC_ERR_INVALIDQUERYREQUEST, "Unexpected hr %#lx.\n", hr);
    hr = IWICMetadataQueryWriter_RemoveMetadataByName(query_writer, L"/app1");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IWICMetadataQueryWriter_Release(query_writer);
    IWICMetadataReader_Release(reader);
    IWICComponentFactory_Release(factory);
}

static void test_CreateQueryReaderFromBlockReader(void)
{
    IWICMetadataQueryReader *queryreader;
    IWICMetadataBlockWriter *writer;
    IWICComponentFactory *factory;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = create_test_block_writer(&GUID_ContainerFormatPng, &writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, NULL, &queryreader);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, (IWICMetadataBlockReader *)writer, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IWICMetadataBlockWriter_Release(writer);

    IWICComponentFactory_Release(factory);
}

START_TEST(metadata)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_queryreader();
    test_WICMapGuidToShortName();
    test_WICMapShortNameToGuid();
    test_WICMapSchemaToName();
    test_metadata_unknown();
    test_metadata_tEXt();
    test_metadata_gAMA();
    test_metadata_cHRM();
    test_metadata_hIST();
    test_metadata_tIME();
    test_metadata_Ifd();
    test_metadata_Exif();
    test_metadata_Gps();
    test_create_reader_from_container();
    test_CreateMetadataReader();
    test_metadata_png();
    test_metadata_gif();
    test_metadata_LSD();
    test_metadata_IMD();
    test_metadata_GCE();
    test_metadata_APE();
    test_metadata_GIF_comment();
    test_metadata_query_writer();
    test_metadata_App1();
    test_CreateMetadataWriterFromReader();
    test_CreateMetadataWriter();
    test_metadata_writer();
    test_CreateQueryWriter();
    test_CreateQueryWriterFromReader();
    test_RemoveMetadataByName();
    test_CreateQueryReaderFromBlockReader();

    CoUninitialize();
}
