/*
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "wincodec.h"
#include "wine/test.h"

#define IFD_BYTE 1
#define IFD_ASCII 2
#define IFD_SHORT 3
#define IFD_LONG 4
#define IFD_RATIONAL 5
#define IFD_SBYTE 6
#define IFD_UNDEFINED 7
#define IFD_SSHORT 8
#define IFD_SLONG 9
#define IFD_SRATIONAL 10
#define IFD_FLOAT 11
#define IFD_DOUBLE 12

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

static const struct tiff_1bpp_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[13];
    ULONG next_IFD;
    struct IFD_rational res;
    BYTE pixel_data[4];
} tiff_1bpp_data =
{
#ifdef WORDS_BIGENDIAN
    'M' | 'M' << 8,
#else
    'I' | 'I' << 8,
#endif
    42,
    FIELD_OFFSET(struct tiff_1bpp_data, number_of_entries),
    13,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 1 }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 1 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_1bpp_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 1 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 1 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 1 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1bpp_data, res) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1bpp_data, res) },
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
    },
    0,
    { 900, 3 },
    { 0x11, 0x22, 0x33, 0 }
};
#include "poppack.h"

static const char *debugstr_guid(const GUID *guid)
{
    static char buf[50];

    if (!guid) return "(null)";
    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
            guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
            guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return buf;
}

static IWICBitmapDecoder *create_decoder(IWICImagingFactory *factory,
                                         const void *image_data, UINT image_size)
{
    HGLOBAL hmem;
    BYTE *data;
    HRESULT hr;
    IWICBitmapDecoder *decoder = NULL;
    IStream *stream;
    GUID guid;

    hmem = GlobalAlloc(0, image_size);
    data = GlobalLock(hmem);
    memcpy(data, image_data, image_size);
    GlobalUnlock(hmem);

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal error %#x\n", hr);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#x\n", hr);

    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guid);
    ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_ContainerFormatTiff), "container format is not TIFF\n");

    IStream_Release(stream);

    return decoder;
}

static void test_tiff_palette(void)
{
    HRESULT hr;
    IWICImagingFactory *factory;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    decoder = create_decoder(factory, &tiff_1bpp_data, sizeof(tiff_1bpp_data));
    ok(decoder != 0, "Failed to load TIFF image data\n");
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormatBlackWhite),
       "got wrong format %s\n", debugstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE,
       "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %#x\n", hr);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
    IWICImagingFactory_Release(factory);
}

START_TEST(tiffformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_tiff_palette();

    CoUninitialize();
}
