/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012,2016 Dmitry Timoshkov
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

#include "ungif.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct gif_decoder
{
    struct decoder decoder;
    GifFileType *gif;
};

static inline struct gif_decoder *impl_from_decoder(struct decoder *iface)
{
    return CONTAINING_RECORD(iface, struct gif_decoder, decoder);
}

#include "pshpack1.h"

struct logical_screen_descriptor
{
    char signature[6];
    USHORT width;
    USHORT height;
    BYTE packed;
    /* global_color_table_flag : 1;
     * color_resolution : 3;
     * sort_flag : 1;
     * global_color_table_size : 3;
     */
    BYTE background_color_index;
    BYTE pixel_aspect_ratio;
};

struct image_descriptor
{
    USHORT left;
    USHORT top;
    USHORT width;
    USHORT height;
    BYTE packed;
    /* local_color_table_flag : 1;
     * interlace_flag : 1;
     * sort_flag : 1;
     * reserved : 2;
     * local_color_table_size : 3;
     */
};

#include "poppack.h"

static HRESULT load_LSD_metadata(MetadataHandler *handler, IStream *stream, const GUID *vendor, DWORD options)
{
    struct logical_screen_descriptor lsd_data;
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    hr = IStream_Read(stream, &lsd_data, sizeof(lsd_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(lsd_data)) return S_OK;

    result = calloc(9, sizeof(MetadataItem));
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 9; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Signature", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI1|VT_VECTOR;
    result[0].value.caub.cElems = sizeof(lsd_data.signature);
    result[0].value.caub.pElems = CoTaskMemAlloc(sizeof(lsd_data.signature));
    memcpy(result[0].value.caub.pElems, lsd_data.signature, sizeof(lsd_data.signature));

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"Width", &result[1].id.pwszVal);
    result[1].value.vt = VT_UI2;
    result[1].value.uiVal = lsd_data.width;

    result[2].id.vt = VT_LPWSTR;
    SHStrDupW(L"Height", &result[2].id.pwszVal);
    result[2].value.vt = VT_UI2;
    result[2].value.uiVal = lsd_data.height;

    result[3].id.vt = VT_LPWSTR;
    SHStrDupW(L"GlobalColorTableFlag", &result[3].id.pwszVal);
    result[3].value.vt = VT_BOOL;
    result[3].value.boolVal = (lsd_data.packed >> 7) & 1;

    result[4].id.vt = VT_LPWSTR;
    SHStrDupW(L"ColorResolution", &result[4].id.pwszVal);
    result[4].value.vt = VT_UI1;
    result[4].value.bVal = (lsd_data.packed >> 4) & 7;

    result[5].id.vt = VT_LPWSTR;
    SHStrDupW(L"SortFlag", &result[5].id.pwszVal);
    result[5].value.vt = VT_BOOL;
    result[5].value.boolVal = (lsd_data.packed >> 3) & 1;

    result[6].id.vt = VT_LPWSTR;
    SHStrDupW(L"GlobalColorTableSize", &result[6].id.pwszVal);
    result[6].value.vt = VT_UI1;
    result[6].value.bVal = lsd_data.packed & 7;

    result[7].id.vt = VT_LPWSTR;
    SHStrDupW(L"BackgroundColorIndex", &result[7].id.pwszVal);
    result[7].value.vt = VT_UI1;
    result[7].value.bVal = lsd_data.background_color_index;

    result[8].id.vt = VT_LPWSTR;
    SHStrDupW(L"PixelAspectRatio", &result[8].id.pwszVal);
    result[8].value.vt = VT_UI1;
    result[8].value.bVal = lsd_data.pixel_aspect_ratio;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 9;

    return S_OK;
}

static const MetadataHandlerVtbl LSDReader_Vtbl = {
    0,
    &CLSID_WICLSDMetadataReader,
    load_LSD_metadata
};

HRESULT LSDReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&LSDReader_Vtbl, iid, ppv);
}

static HRESULT load_IMD_metadata(MetadataHandler *handler, IStream *stream, const GUID *vendor, DWORD options)
{
    struct image_descriptor imd_data;
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    hr = IStream_Read(stream, &imd_data, sizeof(imd_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(imd_data)) return S_OK;

    result = calloc(8, sizeof(MetadataItem));
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 8; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Left", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI2;
    result[0].value.uiVal = imd_data.left;

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"Top", &result[1].id.pwszVal);
    result[1].value.vt = VT_UI2;
    result[1].value.uiVal = imd_data.top;

    result[2].id.vt = VT_LPWSTR;
    SHStrDupW(L"Width", &result[2].id.pwszVal);
    result[2].value.vt = VT_UI2;
    result[2].value.uiVal = imd_data.width;

    result[3].id.vt = VT_LPWSTR;
    SHStrDupW(L"Height", &result[3].id.pwszVal);
    result[3].value.vt = VT_UI2;
    result[3].value.uiVal = imd_data.height;

    result[4].id.vt = VT_LPWSTR;
    SHStrDupW(L"LocalColorTableFlag", &result[4].id.pwszVal);
    result[4].value.vt = VT_BOOL;
    result[4].value.boolVal = (imd_data.packed >> 7) & 1;

    result[5].id.vt = VT_LPWSTR;
    SHStrDupW(L"InterlaceFlag", &result[5].id.pwszVal);
    result[5].value.vt = VT_BOOL;
    result[5].value.boolVal = (imd_data.packed >> 6) & 1;

    result[6].id.vt = VT_LPWSTR;
    SHStrDupW(L"SortFlag", &result[6].id.pwszVal);
    result[6].value.vt = VT_BOOL;
    result[6].value.boolVal = (imd_data.packed >> 5) & 1;

    result[7].id.vt = VT_LPWSTR;
    SHStrDupW(L"LocalColorTableSize", &result[7].id.pwszVal);
    result[7].value.vt = VT_UI1;
    result[7].value.bVal = imd_data.packed & 7;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 8;

    return S_OK;
}

static const MetadataHandlerVtbl IMDReader_Vtbl = {
    0,
    &CLSID_WICIMDMetadataReader,
    load_IMD_metadata
};

HRESULT IMDReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&IMDReader_Vtbl, iid, ppv);
}

static HRESULT load_GCE_metadata(MetadataHandler *handler, IStream *stream, const GUID *vendor, DWORD options)
{
#include "pshpack1.h"
    struct graphic_control_extension
    {
        BYTE packed;
        /* reservred: 3;
         * disposal : 3;
         * user_input_flag : 1;
         * transparency_flag : 1;
         */
         USHORT delay;
         BYTE transparent_color_index;
    } gce_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    hr = IStream_Read(stream, &gce_data, sizeof(gce_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(gce_data)) return S_OK;

    result = calloc(5, sizeof(MetadataItem));
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 5; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Disposal", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI1;
    result[0].value.bVal = (gce_data.packed >> 2) & 7;

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"UserInputFlag", &result[1].id.pwszVal);
    result[1].value.vt = VT_BOOL;
    result[1].value.boolVal = (gce_data.packed >> 1) & 1;

    result[2].id.vt = VT_LPWSTR;
    SHStrDupW(L"TransparencyFlag", &result[2].id.pwszVal);
    result[2].value.vt = VT_BOOL;
    result[2].value.boolVal = gce_data.packed & 1;

    result[3].id.vt = VT_LPWSTR;
    SHStrDupW(L"Delay", &result[3].id.pwszVal);
    result[3].value.vt = VT_UI2;
    result[3].value.uiVal = gce_data.delay;

    result[4].id.vt = VT_LPWSTR;
    SHStrDupW(L"TransparentColorIndex", &result[4].id.pwszVal);
    result[4].value.vt = VT_UI1;
    result[4].value.bVal = gce_data.transparent_color_index;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 5;

    return S_OK;
}

static const MetadataHandlerVtbl GCEReader_Vtbl = {
    0,
    &CLSID_WICGCEMetadataReader,
    load_GCE_metadata
};

HRESULT GCEReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&GCEReader_Vtbl, iid, ppv);
}

static HRESULT load_APE_metadata(MetadataHandler *handler, IStream *stream, const GUID *vendor, DWORD options)
{
#include "pshpack1.h"
    struct application_extension
    {
        BYTE extension_introducer;
        BYTE extension_label;
        BYTE block_size;
        BYTE application[11];
    } ape_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, data_size, i;
    MetadataItem *result;
    BYTE subblock_size;
    BYTE *data;

    hr = IStream_Read(stream, &ape_data, sizeof(ape_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(ape_data)) return S_OK;
    if (ape_data.extension_introducer != 0x21 ||
        ape_data.extension_label != APPLICATION_EXT_FUNC_CODE ||
        ape_data.block_size != 11)
        return S_OK;

    data = NULL;
    data_size = 0;

    for (;;)
    {
        hr = IStream_Read(stream, &subblock_size, sizeof(subblock_size), &bytesread);
        if (FAILED(hr) || bytesread != sizeof(subblock_size))
        {
            CoTaskMemFree(data);
            return S_OK;
        }
        if (!subblock_size) break;

        if (!data)
            data = CoTaskMemAlloc(subblock_size + 1);
        else
        {
            BYTE *new_data = CoTaskMemRealloc(data, data_size + subblock_size + 1);
            if (!new_data)
            {
                CoTaskMemFree(data);
                return S_OK;
            }
            data = new_data;
        }
        data[data_size] = subblock_size;
        hr = IStream_Read(stream, data + data_size + 1, subblock_size, &bytesread);
        if (FAILED(hr) || bytesread != subblock_size)
        {
            CoTaskMemFree(data);
            return S_OK;
        }
        data_size += subblock_size + 1;
    }

    result = calloc(2, sizeof(MetadataItem));
    if (!result)
    {
        CoTaskMemFree(data);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < 2; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Application", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI1|VT_VECTOR;
    result[0].value.caub.cElems = sizeof(ape_data.application);
    result[0].value.caub.pElems = CoTaskMemAlloc(sizeof(ape_data.application));
    memcpy(result[0].value.caub.pElems, ape_data.application, sizeof(ape_data.application));

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"Data", &result[1].id.pwszVal);
    result[1].value.vt = VT_UI1|VT_VECTOR;
    result[1].value.caub.cElems = data_size;
    result[1].value.caub.pElems = data;

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 2;

    return S_OK;
}

static const MetadataHandlerVtbl APEReader_Vtbl = {
    0,
    &CLSID_WICAPEMetadataReader,
    load_APE_metadata
};

HRESULT APEReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&APEReader_Vtbl, iid, ppv);
}

static HRESULT create_gifcomment_item(char *data, MetadataItem **item)
{
    HRESULT hr;

    if (!(*item = calloc(1, sizeof(**item))))
        return E_OUTOFMEMORY;

    hr = init_propvar_from_string(L"TextEntry", &(*item)->id);
    if (FAILED(hr))
    {
        free(*item);
        return hr;
    }

    (*item)->value.vt = VT_LPSTR;
    (*item)->value.pszVal = data;

    return S_OK;
}

static HRESULT load_GifComment_metadata(MetadataHandler *handler, IStream *stream, const GUID *vendor, DWORD options)
{
#include "pshpack1.h"
    struct gif_extension
    {
        BYTE extension_introducer;
        BYTE extension_label;
    } ext_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, data_size;
    MetadataItem *result;
    BYTE subblock_size;
    char *data;

    hr = IStream_Read(stream, &ext_data, sizeof(ext_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(ext_data)) return S_OK;
    if (ext_data.extension_introducer != 0x21 ||
        ext_data.extension_label != COMMENT_EXT_FUNC_CODE)
        return S_OK;

    data = NULL;
    data_size = 0;

    for (;;)
    {
        hr = IStream_Read(stream, &subblock_size, sizeof(subblock_size), &bytesread);
        if (FAILED(hr) || bytesread != sizeof(subblock_size))
        {
            CoTaskMemFree(data);
            return S_OK;
        }
        if (!subblock_size) break;

        if (!data)
            data = CoTaskMemAlloc(subblock_size + 1);
        else
        {
            char *new_data = CoTaskMemRealloc(data, data_size + subblock_size + 1);
            if (!new_data)
            {
                CoTaskMemFree(data);
                return S_OK;
            }
            data = new_data;
        }
        hr = IStream_Read(stream, data + data_size, subblock_size, &bytesread);
        if (FAILED(hr) || bytesread != subblock_size)
        {
            CoTaskMemFree(data);
            return S_OK;
        }
        data_size += subblock_size;
    }

    data[data_size] = 0;

    if (FAILED(hr = create_gifcomment_item(data, &result)))
    {
        CoTaskMemFree(data);
        return hr;
    }

    MetadataHandler_FreeItems(handler);
    handler->items = result;
    handler->item_count = 1;

    return S_OK;
}

static HRESULT CreateGifCommentHandler(MetadataHandler *handler)
{
    MetadataItem *item;
    char *data;
    HRESULT hr;

    if (!(data = CoTaskMemAlloc(1)))
        return E_OUTOFMEMORY;
    *data = 0;

    if (FAILED(hr = create_gifcomment_item(data, &item)))
    {
        CoTaskMemFree(data);
        return hr;
    }

    handler->items = item;
    handler->item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl GifCommentReader_Vtbl = {
    0,
    &CLSID_WICGifCommentMetadataReader,
    load_GifComment_metadata,
    CreateGifCommentHandler,
};

HRESULT GifCommentReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&GifCommentReader_Vtbl, iid, ppv);
}

static void copy_palette(ColorMapObject *cm, Extensions *extensions, int count, WICColor *colors)
{
    int i;

    if (cm)
    {
        for (i = 0; i < count; i++)
        {
            colors[i] = 0xff000000 | /* alpha */
                        cm->Colors[i].Red << 16 |
                        cm->Colors[i].Green << 8 |
                        cm->Colors[i].Blue;
        }
    }
    else
    {
        colors[0] = 0xff000000;
        colors[1] = 0xffffffff;
        for (i = 2; i < count; i++)
            colors[i] = 0xff000000;
    }

    /* look for the transparent color extension */
    for (i = 0; i < extensions->ExtensionBlockCount; i++)
    {
        ExtensionBlock *eb = extensions->ExtensionBlocks + i;
        if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
            eb->ByteCount == 8 && eb->Bytes[3] & 1)
        {
            int trans = (unsigned char)eb->Bytes[6];
            colors[trans] &= 0xffffff; /* set alpha to 0 */
            break;
        }
    }
}

static HRESULT copy_interlaced_pixels(const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride, const WICRect *rc,
    UINT dststride, UINT dstbuffersize, BYTE *dstbuffer)
{
    UINT row_offset; /* number of bytes into the source rows where the data starts */
    const BYTE *src;
    BYTE *dst;
    UINT y;
    WICRect rect;

    if (!rc)
    {
        rect.X = 0;
        rect.Y = 0;
        rect.Width = srcwidth;
        rect.Height = srcheight;
        rc = &rect;
    }
    else
    {
        if (rc->X < 0 || rc->Y < 0 || rc->X+rc->Width > srcwidth || rc->Y+rc->Height > srcheight)
            return E_INVALIDARG;
    }

    if (dststride < rc->Width)
        return E_INVALIDARG;

    if ((dststride * rc->Height) > dstbuffersize)
        return E_INVALIDARG;

    row_offset = rc->X;

    dst = dstbuffer;
    for (y=rc->Y; y-rc->Y < rc->Height; y++)
    {
        if (y%8 == 0)
            src = srcbuffer + srcstride * (y/8);
        else if (y%4 == 0)
            src = srcbuffer + srcstride * ((srcheight+7)/8 + y/8);
        else if (y%2 == 0)
            src = srcbuffer + srcstride * ((srcheight+3)/4 + y/4);
        else /* y%2 == 1 */
            src = srcbuffer + srcstride * ((srcheight+1)/2 + y/2);
        src += row_offset;
        memcpy(dst, src, rc->Width);
        dst += dststride;
    }
    return S_OK;
}

static int _gif_inputfunc(GifFileType *gif, GifByteType *data, int len) {
    IStream *stream = gif->UserData;
    ULONG bytesread;
    HRESULT hr;

    if (!stream)
    {
        ERR("attempting to read file after initialization\n");
        return 0;
    }

    hr = IStream_Read(stream, data, len, &bytesread);
    if (FAILED(hr)) bytesread = 0;
    return bytesread;
}

static HRESULT CDECL gif_decoder_initialize(struct decoder *iface, IStream *stream, struct decoder_stat *st)
{
    struct gif_decoder *decoder = impl_from_decoder(iface);
    LARGE_INTEGER seek;
    int ret;

    /* seek to start of stream */
    seek.QuadPart = 0;
    IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);

    /* read all data from the stream */
    decoder->gif = DGifOpen((void *)stream, _gif_inputfunc);
    if (!decoder->gif)
        return E_FAIL;

    ret = DGifSlurp(decoder->gif);
    if (ret == GIF_ERROR)
        return E_FAIL;

    /* make sure we don't use the stream after this method returns */
    decoder->gif->UserData = NULL;

    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata |
                DECODER_FLAGS_UNSUPPORTED_COLOR_CONTEXT |
                DECODER_FLAGS_METADATA_AT_DECODER;
    st->frame_count = decoder->gif->ImageCount;

    return S_OK;
}

static HRESULT CDECL gif_decoder_get_frame_info(struct decoder *iface, UINT frame, struct decoder_frame *info)
{
    struct gif_decoder *decoder = impl_from_decoder(iface);
    ColorMapObject *colormap;
    GifWord aspect_word;
    SavedImage *image;
    DWORD num_colors;
    double aspect;

    if (frame >= decoder->gif->ImageCount)
        return WINCODEC_ERR_FRAMEMISSING;

    aspect_word = decoder->gif->SAspectRatio;
    aspect = (aspect_word > 0) ? ((aspect_word + 15.0) / 64.0) : 1.0;

    image = &decoder->gif->SavedImages[frame];

    colormap = image->ImageDesc.ColorMap;
    if (colormap)
    {
        num_colors = colormap->ColorCount;
    }
    else
    {
        colormap = decoder->gif->SColorMap;
        num_colors = decoder->gif->SColorTableSize;
    }

    memcpy(&info->pixel_format, &GUID_WICPixelFormat8bppIndexed, sizeof(GUID));
    info->width = image->ImageDesc.Width;
    info->height = image->ImageDesc.Height;
    info->bpp = 8;
    info->dpix = 96.0 / aspect;
    info->dpiy = 96.0;
    info->num_color_contexts = 0;
    info->num_colors = num_colors;
    copy_palette(colormap, &image->Extensions, num_colors, info->palette);

    return S_OK;
}

static HRESULT CDECL gif_decoder_get_decoder_palette(struct decoder *iface, UINT frame, WICColor *colors,
        UINT *num_colors)
{
    struct gif_decoder *decoder = impl_from_decoder(iface);
    ColorMapObject *cm;
    int count;

    if (!decoder->gif)
        return WINCODEC_ERR_WRONGSTATE;

    if (frame >= decoder->gif->ImageCount)
        return WINCODEC_ERR_FRAMEMISSING;

    cm = decoder->gif->SColorMap;
    count = decoder->gif->SColorTableSize;

    if (count > 256)
    {
        ERR("GIF contains invalid number of colors: %d\n", count);
        return E_FAIL;
    }

    copy_palette(cm, &decoder->gif->SavedImages[frame].Extensions, count, colors);

    *num_colors = count;

    return S_OK;
}

static HRESULT CDECL gif_decoder_copy_pixels(struct decoder *iface, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct gif_decoder *decoder = impl_from_decoder(iface);
    SavedImage *image;

    if (frame >= decoder->gif->ImageCount)
        return E_INVALIDARG;

    image = &decoder->gif->SavedImages[frame];

    if (image->ImageDesc.Interlace)
        return copy_interlaced_pixels(image->RasterBits, image->ImageDesc.Width,
                image->ImageDesc.Height, image->ImageDesc.Width, prc, stride, buffersize, buffer);

    return copy_pixels(8, image->RasterBits, image->ImageDesc.Width, image->ImageDesc.Height,
            image->ImageDesc.Width, prc, stride, buffersize, buffer);
}

static HRESULT CDECL gif_decoder_get_metadata_blocks(struct decoder *iface,
        UINT frame, UINT *count, struct decoder_block **blocks)
{
    struct gif_decoder *decoder = impl_from_decoder(iface);
    struct decoder_block *result;
    UINT block_count, index = 0;
    ExtensionBlock *ext;
    SavedImage *image;

    if (frame == ~0u)
    {
        block_count = decoder->gif->Extensions.ExtensionBlockCount + 1;

        if (!(result = calloc(block_count, sizeof(*result))))
            return E_OUTOFMEMORY;

        result[index].offset = 0;
        result[index].length = sizeof(struct logical_screen_descriptor);
        result[index].options = DECODER_BLOCK_READER_CLSID;
        result[index].reader_clsid = CLSID_WICLSDMetadataReader;

        for (index = 1; index < block_count; ++index)
        {
            ext = decoder->gif->Extensions.ExtensionBlocks + index - 1;

            result[index].offset = (ULONG_PTR)ext->Bytes;
            result[index].length = ext->ByteCount;
            result[index].options = DECODER_BLOCK_OFFSET_IS_PTR;

            if (ext->Function == APPLICATION_EXT_FUNC_CODE)
            {
                result[index].reader_clsid = CLSID_WICAPEMetadataReader;
                result[index].options |= DECODER_BLOCK_READER_CLSID;
            }
            else if (ext->Function == COMMENT_EXT_FUNC_CODE)
            {
                result[index].reader_clsid = CLSID_WICGifCommentMetadataReader;
                result[index].options |= DECODER_BLOCK_READER_CLSID;
            }
            else
            {
                result[index].options |= WICMetadataCreationAllowUnknown;
            }
        }
    }
    else if (frame < decoder->gif->ImageCount)
    {
        image = &decoder->gif->SavedImages[frame];

        block_count = image->Extensions.ExtensionBlockCount + 1;

        if (!(result = calloc(block_count, sizeof(*result))))
            return E_OUTOFMEMORY;

        result[index].offset = image->ImageDescOffset;
        result[index].length = 9;
        result[index].options = DECODER_BLOCK_READER_CLSID;
        result[index].reader_clsid = CLSID_WICIMDMetadataReader;

        for (index = 1; index < block_count; ++index)
        {
            ext = image->Extensions.ExtensionBlocks + index - 1;

            if (ext->Function == GRAPHICS_EXT_FUNC_CODE)
            {
                result[index].offset = (ULONG_PTR)(ext->Bytes + 3);
                result[index].length = ext->ByteCount - 4;
                result[index].options = DECODER_BLOCK_OFFSET_IS_PTR | DECODER_BLOCK_READER_CLSID;
                result[index].reader_clsid = CLSID_WICGCEMetadataReader;
            }
            else if (ext->Function == COMMENT_EXT_FUNC_CODE)
            {
                result[index].offset = (ULONG_PTR)ext->Bytes;
                result[index].length = ext->ByteCount;
                result[index].options = DECODER_BLOCK_OFFSET_IS_PTR | DECODER_BLOCK_READER_CLSID;
                result[index].reader_clsid = CLSID_WICGifCommentMetadataReader;
            }
            else
            {
                result[index].offset = (ULONG_PTR)ext->Bytes;
                result[index].length = ext->ByteCount;
                result[index].options = DECODER_BLOCK_OFFSET_IS_PTR | WICMetadataCreationAllowUnknown;
            }
        }
    }
    else
        return E_INVALIDARG;

    *count = block_count;
    *blocks = result;

    return S_OK;
}

static HRESULT CDECL gif_decoder_get_color_context(struct decoder *iface, UINT frame, UINT num,
        BYTE **data, DWORD *datasize)
{
    return E_NOTIMPL;
}

static void CDECL gif_decoder_destroy(struct decoder *iface)
{
    struct gif_decoder *decoder = impl_from_decoder(iface);

    DGifCloseFile(decoder->gif);
    free(decoder);
}

static const struct decoder_funcs gif_decoder_vtable =
{
    gif_decoder_initialize,
    gif_decoder_get_frame_info,
    gif_decoder_get_decoder_palette,
    gif_decoder_copy_pixels,
    gif_decoder_get_metadata_blocks,
    gif_decoder_get_color_context,
    gif_decoder_destroy
};

HRESULT CDECL gif_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct gif_decoder *gif_decoder;

    if (!(gif_decoder = calloc(1, sizeof(*gif_decoder))))
        return E_OUTOFMEMORY;

    gif_decoder->decoder.vtable = &gif_decoder_vtable;
    *result = &gif_decoder->decoder;

    info->container_format = GUID_ContainerFormatGif;
    info->block_format = GUID_ContainerFormatGif;
    info->clsid = CLSID_WICGifDecoder;

    return S_OK;
}

HRESULT GifDecoder_CreateInstance(REFIID iid, void **ppv)
{
    struct decoder_info decoder_info;
    struct decoder *decoder;
    HRESULT hr;

    hr = gif_decoder_create(&decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}

typedef struct GifEncoder
{
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock;
    BOOL initialized, info_written, committed;
    UINT n_frames;
    WICColor palette[256];
    UINT colors;
} GifEncoder;

static inline GifEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, GifEncoder, IWICBitmapEncoder_iface);
}

typedef struct GifFrameEncode
{
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    IWICMetadataBlockWriter IWICMetadataBlockWriter_iface;
    LONG ref;
    GifEncoder *encoder;
    BOOL initialized, interlace, committed;
    UINT width, height, lines;
    double xres, yres;
    WICColor palette[256];
    UINT colors;
    BYTE *image_data;
} GifFrameEncode;

static inline GifFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, GifFrameEncode, IWICBitmapFrameEncode_iface);
}

static inline GifFrameEncode *impl_from_IWICMetadataBlockWriter(IWICMetadataBlockWriter *iface)
{
    return CONTAINING_RECORD(iface, GifFrameEncode, IWICMetadataBlockWriter_iface);
}

static HRESULT WINAPI GifFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid, void **ppv)
{
    GifFrameEncode *encoder = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockWriter, iid))
    {
        *ppv = &encoder->IWICMetadataBlockWriter_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI GifFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI GifFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);

    if (!ref)
    {
        IWICBitmapEncoder_Release(&This->encoder->IWICBitmapEncoder_iface);
        free(This->image_data);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI GifFrameEncode_Initialize(IWICBitmapFrameEncode *iface, IPropertyBag2 *options)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, options);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized)
    {
        This->initialized = TRUE;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetSize(IWICBitmapFrameEncode *iface, UINT width, UINT height)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%u,%u\n", iface, width, height);

    if (!width || !height) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        free(This->image_data);

        This->image_data = malloc(width * height);
        if (This->image_data)
        {
            This->width = width;
            This->height = height;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetResolution(IWICBitmapFrameEncode *iface, double xres, double yres)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%f,%f\n", iface, xres, yres);

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        This->xres = xres;
        This->yres = yres;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface, WICPixelFormatGUID *format)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%s\n", iface, debugstr_guid(format));

    if (!format) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        *format = GUID_WICPixelFormat8bppIndexed;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface, UINT count, IWICColorContext **context)
{
    FIXME("%p,%u,%p: stub\n", iface, count, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifFrameEncode_SetPalette(IWICBitmapFrameEncode *iface, IWICPalette *palette)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface, IWICBitmapSource *thumbnail)
{
    FIXME("%p,%p: stub\n", iface, thumbnail);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifFrameEncode_WritePixels(IWICBitmapFrameEncode *iface, UINT lines, UINT stride, UINT size, BYTE *pixels)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%u,%u,%u,%p\n", iface, lines, stride, size, pixels);

    if (!pixels) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized && This->image_data)
    {
        if (This->lines + lines <= This->height)
        {
            UINT i;
            BYTE *src, *dst;

            src = pixels;
            dst = This->image_data + This->lines * This->width;

            for (i = 0; i < lines; i++)
            {
                memcpy(dst, src, This->width);
                src += stride;
                dst += This->width;
            }

            This->lines += lines;
            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI GifFrameEncode_WriteSource(IWICBitmapFrameEncode *iface, IWICBitmapSource *source, WICRect *rc)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%p,%p\n", iface, source, rc);

    if (!source) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        const GUID *format = &GUID_WICPixelFormat8bppIndexed;

        hr = configure_write_source(iface, source, rc, format,
                                    This->width, This->height, This->xres, This->yres);
        if (hr == S_OK)
            hr = write_source(iface, source, rc, format, 8, !This->colors, This->width, This->height);
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

#define LZW_DICT_SIZE (1 << 12)

struct lzw_dict
{
    short prefix[LZW_DICT_SIZE];
    unsigned char suffix[LZW_DICT_SIZE];
};

struct lzw_state
{
    struct lzw_dict dict;
    short init_code_bits, code_bits, next_code, clear_code, eof_code;
    unsigned bits_buf;
    int bits_count;
    int (*user_write_data)(void *user_ptr, void *data, int length);
    void *user_ptr;
};

struct input_stream
{
    unsigned len;
    const BYTE *in;
};

struct output_stream
{
    struct
    {
        unsigned char len;
        char data[255];
    } gif_block;
    IStream *out;
};

static int lzw_output_code(struct lzw_state *state, short code)
{
    state->bits_buf |= code << state->bits_count;
    state->bits_count += state->code_bits;

    while (state->bits_count >= 8)
    {
        unsigned char byte = (unsigned char)state->bits_buf;
        if (state->user_write_data(state->user_ptr, &byte, 1) != 1)
            return 0;
        state->bits_buf >>= 8;
        state->bits_count -= 8;
    }

    return 1;
}

static inline int lzw_output_clear_code(struct lzw_state *state)
{
    return lzw_output_code(state, state->clear_code);
}

static inline int lzw_output_eof_code(struct lzw_state *state)
{
    return lzw_output_code(state, state->eof_code);
}

static int lzw_flush_bits(struct lzw_state *state)
{
    unsigned char byte;

    while (state->bits_count >= 8)
    {
        byte = (unsigned char)state->bits_buf;
        if (state->user_write_data(state->user_ptr, &byte, 1) != 1)
            return 0;
        state->bits_buf >>= 8;
        state->bits_count -= 8;
    }

    if (state->bits_count)
    {
        static const char mask[8] = { 0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f };

        byte = (unsigned char)state->bits_buf & mask[state->bits_count];
        if (state->user_write_data(state->user_ptr, &byte, 1) != 1)
            return 0;
    }

    state->bits_buf = 0;
    state->bits_count = 0;

    return 1;
}

static void lzw_dict_reset(struct lzw_state *state)
{
    int i;

    state->code_bits = state->init_code_bits + 1;
    state->next_code = (1 << state->init_code_bits) + 2;

    for(i = 0; i < LZW_DICT_SIZE; i++)
    {
        state->dict.prefix[i] = 1 << 12; /* impossible LZW code value */
        state->dict.suffix[i] = 0;
    }
}

static void lzw_state_init(struct lzw_state *state, short init_code_bits, void *user_write_data, void *user_ptr)
{
    state->init_code_bits = init_code_bits;
    state->clear_code = 1 << init_code_bits;
    state->eof_code = state->clear_code + 1;
    state->bits_buf = 0;
    state->bits_count = 0;
    state->user_write_data = user_write_data;
    state->user_ptr = user_ptr;

    lzw_dict_reset(state);
}

static int lzw_dict_add(struct lzw_state *state, short prefix, unsigned char suffix)
{
    if (state->next_code < LZW_DICT_SIZE)
    {
        state->dict.prefix[state->next_code] = prefix;
        state->dict.suffix[state->next_code] = suffix;

        if ((state->next_code & (state->next_code - 1)) == 0)
            state->code_bits++;

        state->next_code++;
        return state->next_code;
    }

    return -1;
}

static short lzw_dict_lookup(const struct lzw_state *state, short prefix, unsigned char suffix)
{
    short i;

    for (i = 0; i < state->next_code; i++)
    {
        if (state->dict.prefix[i] == prefix && state->dict.suffix[i] == suffix)
            return i;
    }

    return -1;
}

static inline int write_byte(struct output_stream *out, char byte)
{
    if (out->gif_block.len == 255)
    {
        if (IStream_Write(out->out, &out->gif_block, sizeof(out->gif_block), NULL) != S_OK)
            return 0;

        out->gif_block.len = 0;
    }

    out->gif_block.data[out->gif_block.len++] = byte;

    return 1;
}

static int write_data(void *user_ptr, void *user_data, int length)
{
    unsigned char *data = user_data;
    struct output_stream *out = user_ptr;
    int len = length;

    while (len-- > 0)
    {
        if (!write_byte(out, *data++)) return 0;
    }

    return length;
}

static int flush_output_data(void *user_ptr)
{
    struct output_stream *out = user_ptr;

    if (out->gif_block.len)
    {
        if (IStream_Write(out->out, &out->gif_block, out->gif_block.len + sizeof(out->gif_block.len), NULL) != S_OK)
            return 0;
    }

    /* write GIF block terminator */
    out->gif_block.len = 0;
    return IStream_Write(out->out, &out->gif_block, sizeof(out->gif_block.len), NULL) == S_OK;
}

static inline int read_byte(struct input_stream *in, unsigned char *byte)
{
    if (in->len)
    {
        in->len--;
        *byte = *in->in++;
        return 1;
    }

    return 0;
}

static HRESULT gif_compress(IStream *out_stream, const BYTE *in_data, ULONG in_size)
{
    struct input_stream in;
    struct output_stream out;
    struct lzw_state state;
    short init_code_bits, prefix, code;
    unsigned char suffix;

    in.in = in_data;
    in.len = in_size;

    out.gif_block.len = 0;
    out.out = out_stream;

    init_code_bits = suffix = 8;
    if (IStream_Write(out.out, &suffix, sizeof(suffix), NULL) != S_OK)
        return E_FAIL;

    lzw_state_init(&state, init_code_bits, write_data, &out);

    if (!lzw_output_clear_code(&state))
        return E_FAIL;

    if (read_byte(&in, &suffix))
    {
        prefix = suffix;

        while (read_byte(&in, &suffix))
        {
            code = lzw_dict_lookup(&state, prefix, suffix);
            if (code == -1)
            {
                if (!lzw_output_code(&state, prefix))
                    return E_FAIL;

                if (lzw_dict_add(&state, prefix, suffix) == -1)
                {
                    if (!lzw_output_clear_code(&state))
                        return E_FAIL;
                    lzw_dict_reset(&state);
                }

                prefix = suffix;
            }
            else
                prefix = code;
        }

        if (!lzw_output_code(&state, prefix))
            return E_FAIL;
        if (!lzw_output_eof_code(&state))
            return E_FAIL;
        if (!lzw_flush_bits(&state))
            return E_FAIL;
    }

    return flush_output_data(&out) ? S_OK : E_FAIL;
}

static HRESULT WINAPI GifFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p\n", iface);

    EnterCriticalSection(&This->encoder->lock);

    if (This->image_data && This->lines == This->height && !This->committed)
    {
        BYTE gif_palette[256][3];

        hr = S_OK;

        if (!This->encoder->info_written)
        {
            struct logical_screen_descriptor lsd;

            /* Logical Screen Descriptor */
            memcpy(lsd.signature, "GIF89a", 6);
            lsd.width = This->width;
            lsd.height = This->height;
            lsd.packed = 0;
            if (This->encoder->colors)
                lsd.packed |= 0x80; /* global color table flag */
            lsd.packed |= 0x07 << 4; /* color resolution */
            lsd.packed |= 0x07; /* global color table size */
            lsd.background_color_index = 0; /* FIXME */
            lsd.pixel_aspect_ratio = 0;
            hr = IStream_Write(This->encoder->stream, &lsd, sizeof(lsd), NULL);
            if (hr == S_OK && This->encoder->colors)
            {
                UINT i;

                /* Global Color Table */
                memset(gif_palette, 0, sizeof(gif_palette));
                for (i = 0; i < This->encoder->colors; i++)
                {
                    gif_palette[i][0] = (This->encoder->palette[i] >> 16) & 0xff;
                    gif_palette[i][1] = (This->encoder->palette[i] >> 8) & 0xff;
                    gif_palette[i][2] = This->encoder->palette[i] & 0xff;
                }
                hr = IStream_Write(This->encoder->stream, gif_palette, sizeof(gif_palette), NULL);
            }

            /* FIXME: write GCE, APE, etc. GIF extensions */

            if (hr == S_OK)
                This->encoder->info_written = TRUE;
        }

        if (hr == S_OK)
        {
            char image_separator = 0x2c;

            hr = IStream_Write(This->encoder->stream, &image_separator, sizeof(image_separator), NULL);
            if (hr == S_OK)
            {
                struct image_descriptor imd;

                /* Image Descriptor */
                imd.left = 0;
                imd.top = 0;
                imd.width = This->width;
                imd.height = This->height;
                imd.packed = 0;
                if (This->colors)
                {
                    imd.packed |= 0x80; /* local color table flag */
                    imd.packed |= 0x07; /* local color table size */
                }
                /* FIXME: interlace flag */
                hr = IStream_Write(This->encoder->stream, &imd, sizeof(imd), NULL);
                if (hr == S_OK && This->colors)
                {
                    UINT i;

                    /* Local Color Table */
                    memset(gif_palette, 0, sizeof(gif_palette));
                    for (i = 0; i < This->colors; i++)
                    {
                        gif_palette[i][0] = (This->palette[i] >> 16) & 0xff;
                        gif_palette[i][1] = (This->palette[i] >> 8) & 0xff;
                        gif_palette[i][2] = This->palette[i] & 0xff;
                    }
                    hr = IStream_Write(This->encoder->stream, gif_palette, sizeof(gif_palette), NULL);
                    if (hr == S_OK)
                    {
                        /* Image Data */
                        hr = gif_compress(This->encoder->stream, This->image_data, This->width * This->height);
                        if (hr == S_OK)
                            This->committed = TRUE;
                    }
                }
            }
        }
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI GifFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface, IWICMetadataQueryWriter **writer)
{
    GifFrameEncode *encode = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("iface, %p, writer %p.\n", iface, writer);

    if (!writer)
        return E_INVALIDARG;

    if (!encode->initialized)
        return WINCODEC_ERR_NOTINITIALIZED;

    return MetadataQueryWriter_CreateInstanceFromBlockWriter(&encode->IWICMetadataBlockWriter_iface, writer);
}

static const IWICBitmapFrameEncodeVtbl GifFrameEncode_Vtbl =
{
    GifFrameEncode_QueryInterface,
    GifFrameEncode_AddRef,
    GifFrameEncode_Release,
    GifFrameEncode_Initialize,
    GifFrameEncode_SetSize,
    GifFrameEncode_SetResolution,
    GifFrameEncode_SetPixelFormat,
    GifFrameEncode_SetColorContexts,
    GifFrameEncode_SetPalette,
    GifFrameEncode_SetThumbnail,
    GifFrameEncode_WritePixels,
    GifFrameEncode_WriteSource,
    GifFrameEncode_Commit,
    GifFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI GifEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid, void **ppv)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        IWICBitmapEncoder_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI GifEncoder_AddRef(IWICBitmapEncoder *iface)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI GifEncoder_Release(IWICBitmapEncoder *iface)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);

    if (!ref)
    {
        if (This->stream) IStream_Release(This->stream);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI GifEncoder_Initialize(IWICBitmapEncoder *iface, IStream *stream, WICBitmapEncoderCacheOption option)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p,%p,%#x\n", iface, stream, option);

    if (!stream) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized)
    {
        IStream_AddRef(stream);
        This->stream = stream;
        This->initialized = TRUE;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI GifEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    if (!format) return E_INVALIDARG;

    *format = GUID_ContainerFormatGif;
    return S_OK;
}

static HRESULT WINAPI GifEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICGifEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI GifEncoder_SetColorContexts(IWICBitmapEncoder *iface, UINT count, IWICColorContext **context)
{
    FIXME("%p,%u,%p: stub\n", iface, count, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI GifEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *thumbnail)
{
    TRACE("%p,%p\n", iface, thumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *preview)
{
    TRACE("%p,%p\n", iface, preview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifEncoderFrame_Block_QueryInterface(IWICMetadataBlockWriter *iface, REFIID iid, void **ppv)
{
    GifFrameEncode *frame_encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_QueryInterface(&frame_encoder->IWICBitmapFrameEncode_iface, iid, ppv);
}

static ULONG WINAPI GifEncoderFrame_Block_AddRef(IWICMetadataBlockWriter *iface)
{
    GifFrameEncode *frame_encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_AddRef(&frame_encoder->IWICBitmapFrameEncode_iface);
}

static ULONG WINAPI GifEncoderFrame_Block_Release(IWICMetadataBlockWriter *iface)
{
    GifFrameEncode *frame_encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_Release(&frame_encoder->IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI GifEncoderFrame_Block_GetContainerFormat(IWICMetadataBlockWriter *iface, GUID *container_format)
{
    FIXME("iface %p, container_format %p stub.\n", iface, container_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetCount(IWICMetadataBlockWriter *iface, UINT *count)
{
    FIXME("iface %p, count %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetReaderByIndex(IWICMetadataBlockWriter *iface,
        UINT index, IWICMetadataReader **metadata_reader)
{
    FIXME("iface %p, index %d, metadata_reader %p stub.\n", iface, index, metadata_reader);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetEnumerator(IWICMetadataBlockWriter *iface, IEnumUnknown **enum_metadata)
{
    FIXME("iface %p, enum_metadata %p stub.\n", iface, enum_metadata);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_InitializeFromBlockReader(IWICMetadataBlockWriter *iface,
        IWICMetadataBlockReader *block_reader)
{
    FIXME("iface %p, block_reader %p stub.\n", iface, block_reader);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter **metadata_writer)
{
    FIXME("iface %p, index %u, metadata_writer %p stub.\n", iface, index, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_AddWriter(IWICMetadataBlockWriter *iface, IWICMetadataWriter *metadata_writer)
{
    FIXME("iface %p, metadata_writer %p stub.\n", iface, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_SetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter *metadata_writer)
{
    FIXME("iface %p, index %u, metadata_writer %p stub.\n", iface, index, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_RemoveWriterByIndex(IWICMetadataBlockWriter *iface, UINT index)
{
    FIXME("iface %p, index %u stub.\n", iface, index);

    return E_NOTIMPL;
}

static const IWICMetadataBlockWriterVtbl GifFrameEncode_BlockVtbl = {
    GifEncoderFrame_Block_QueryInterface,
    GifEncoderFrame_Block_AddRef,
    GifEncoderFrame_Block_Release,
    GifEncoderFrame_Block_GetContainerFormat,
    GifEncoderFrame_Block_GetCount,
    GifEncoderFrame_Block_GetReaderByIndex,
    GifEncoderFrame_Block_GetEnumerator,
    GifEncoderFrame_Block_InitializeFromBlockReader,
    GifEncoderFrame_Block_GetWriterByIndex,
    GifEncoderFrame_Block_AddWriter,
    GifEncoderFrame_Block_SetWriterByIndex,
    GifEncoderFrame_Block_RemoveWriterByIndex,
};

static HRESULT WINAPI GifEncoder_CreateNewFrame(IWICBitmapEncoder *iface, IWICBitmapFrameEncode **frame, IPropertyBag2 **options)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p,%p,%p\n", iface, frame, options);

    if (!frame) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->initialized && !This->committed)
    {
        GifFrameEncode *ret = malloc(sizeof(*ret));
        if (ret)
        {
            This->n_frames++;

            ret->IWICBitmapFrameEncode_iface.lpVtbl = &GifFrameEncode_Vtbl;
            ret->IWICMetadataBlockWriter_iface.lpVtbl = &GifFrameEncode_BlockVtbl;

            ret->ref = 1;
            ret->encoder = This;
            ret->initialized = FALSE;
            ret->interlace = FALSE; /* FIXME: read from the properties */
            ret->committed = FALSE;
            ret->width = 0;
            ret->height = 0;
            ret->lines = 0;
            ret->xres = 0.0;
            ret->yres = 0.0;
            ret->colors = 0;
            ret->image_data = NULL;
            IWICBitmapEncoder_AddRef(iface);
            *frame = &ret->IWICBitmapFrameEncode_iface;

            hr = S_OK;

            if (options)
            {
                hr = CreatePropertyBag2(NULL, 0, options);
                if (hr != S_OK)
                {
                    IWICBitmapFrameEncode_Release(*frame);
                    *frame = NULL;
                }
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->lock);

    return hr;

}

static HRESULT WINAPI GifEncoder_Commit(IWICBitmapEncoder *iface)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p\n", iface);

    EnterCriticalSection(&This->lock);

    if (This->initialized && !This->committed)
    {
        char gif_trailer = 0x3b;

        /* FIXME: write text, comment GIF extensions */

        hr = IStream_Write(This->stream, &gif_trailer, sizeof(gif_trailer), NULL);
        if (hr == S_OK)
            This->committed = TRUE;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI GifEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface, IWICMetadataQueryWriter **writer)
{
    FIXME("%p,%p: stub\n", iface, writer);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl GifEncoder_Vtbl =
{
    GifEncoder_QueryInterface,
    GifEncoder_AddRef,
    GifEncoder_Release,
    GifEncoder_Initialize,
    GifEncoder_GetContainerFormat,
    GifEncoder_GetEncoderInfo,
    GifEncoder_SetColorContexts,
    GifEncoder_SetPalette,
    GifEncoder_SetThumbnail,
    GifEncoder_SetPreview,
    GifEncoder_CreateNewFrame,
    GifEncoder_Commit,
    GifEncoder_GetMetadataQueryWriter
};

HRESULT GifEncoder_CreateInstance(REFIID iid, void **ppv)
{
    GifEncoder *This;
    HRESULT ret;

    TRACE("%s,%p\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &GifEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": GifEncoder.lock");
    This->initialized = FALSE;
    This->info_written = FALSE;
    This->committed = FALSE;
    This->n_frames = 0;
    This->colors = 0;

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}
