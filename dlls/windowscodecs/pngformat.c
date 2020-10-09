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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#ifdef SONAME_LIBPNG
#include <png.h>
#endif

#define NONAMELESSUNION
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

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
    result[0].id.u.pszVal = name;
    result[0].value.vt = VT_LPSTR;
    result[0].value.u.pszVal = value;

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
    static const WCHAR ImageGamma[] = {'I','m','a','g','e','G','a','m','m','a',0};
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
    name = HeapAlloc(GetProcessHeap(), 0, sizeof(ImageGamma));
    if (!result || !name)
    {
        HeapFree(GetProcessHeap(), 0, result);
        HeapFree(GetProcessHeap(), 0, name);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result[0].schema);
    PropVariantInit(&result[0].id);
    PropVariantInit(&result[0].value);

    memcpy(name, ImageGamma, sizeof(ImageGamma));

    result[0].id.vt = VT_LPWSTR;
    result[0].id.u.pwszVal = name;
    result[0].value.vt = VT_UI4;
    result[0].value.u.ulVal = gamma;

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
        {'W','h','i','t','e','P','o','i','n','t','X',0},
        {'W','h','i','t','e','P','o','i','n','t','Y',0},
        {'R','e','d','X',0},
        {'R','e','d','Y',0},
        {'G','r','e','e','n','X',0},
        {'G','r','e','e','n','Y',0},
        {'B','l','u','e','X',0},
        {'B','l','u','e','Y',0},
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
        result[i].id.u.pwszVal = dyn_names[i];
        lstrcpyW(dyn_names[i], names[i]);

        PropVariantInit(&result[i].value);
        result[i].value.vt = VT_UI4;
        result[i].value.u.ulVal = read_ulong_be(&data[i*4]);
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

#ifdef SONAME_LIBPNG

static void *libpng_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(png_create_read_struct);
MAKE_FUNCPTR(png_create_info_struct);
MAKE_FUNCPTR(png_create_write_struct);
MAKE_FUNCPTR(png_destroy_read_struct);
MAKE_FUNCPTR(png_destroy_write_struct);
MAKE_FUNCPTR(png_error);
MAKE_FUNCPTR(png_get_bit_depth);
MAKE_FUNCPTR(png_get_color_type);
MAKE_FUNCPTR(png_get_error_ptr);
MAKE_FUNCPTR(png_get_iCCP);
MAKE_FUNCPTR(png_get_image_height);
MAKE_FUNCPTR(png_get_image_width);
MAKE_FUNCPTR(png_get_io_ptr);
MAKE_FUNCPTR(png_get_pHYs);
MAKE_FUNCPTR(png_get_PLTE);
MAKE_FUNCPTR(png_get_tRNS);
MAKE_FUNCPTR(png_set_bgr);
MAKE_FUNCPTR(png_set_crc_action);
MAKE_FUNCPTR(png_set_error_fn);
MAKE_FUNCPTR(png_set_filler);
MAKE_FUNCPTR(png_set_filter);
MAKE_FUNCPTR(png_set_gray_to_rgb);
MAKE_FUNCPTR(png_set_interlace_handling);
MAKE_FUNCPTR(png_set_IHDR);
MAKE_FUNCPTR(png_set_pHYs);
MAKE_FUNCPTR(png_set_PLTE);
MAKE_FUNCPTR(png_set_read_fn);
MAKE_FUNCPTR(png_set_strip_16);
MAKE_FUNCPTR(png_set_tRNS);
MAKE_FUNCPTR(png_set_tRNS_to_alpha);
MAKE_FUNCPTR(png_set_write_fn);
MAKE_FUNCPTR(png_set_swap);
MAKE_FUNCPTR(png_read_end);
MAKE_FUNCPTR(png_read_image);
MAKE_FUNCPTR(png_read_info);
MAKE_FUNCPTR(png_write_end);
MAKE_FUNCPTR(png_write_info);
MAKE_FUNCPTR(png_write_rows);
#undef MAKE_FUNCPTR

static CRITICAL_SECTION init_png_cs;
static CRITICAL_SECTION_DEBUG init_png_cs_debug =
{
    0, 0, &init_png_cs,
    { &init_png_cs_debug.ProcessLocksList,
      &init_png_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_png_cs") }
};
static CRITICAL_SECTION init_png_cs = { &init_png_cs_debug, -1, 0, 0, 0, 0 };

static const WCHAR wszPngInterlaceOption[] = {'I','n','t','e','r','l','a','c','e','O','p','t','i','o','n',0};
static const WCHAR wszPngFilterOption[] = {'F','i','l','t','e','r','O','p','t','i','o','n',0};

static void *load_libpng(void)
{
    void *result;

    EnterCriticalSection(&init_png_cs);

    if(!libpng_handle && (libpng_handle = dlopen(SONAME_LIBPNG, RTLD_NOW)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(libpng_handle, #f)) == NULL) { \
        libpng_handle = NULL; \
        LeaveCriticalSection(&init_png_cs); \
        return NULL; \
    }
        LOAD_FUNCPTR(png_create_read_struct);
        LOAD_FUNCPTR(png_create_info_struct);
        LOAD_FUNCPTR(png_create_write_struct);
        LOAD_FUNCPTR(png_destroy_read_struct);
        LOAD_FUNCPTR(png_destroy_write_struct);
        LOAD_FUNCPTR(png_error);
        LOAD_FUNCPTR(png_get_bit_depth);
        LOAD_FUNCPTR(png_get_color_type);
        LOAD_FUNCPTR(png_get_error_ptr);
        LOAD_FUNCPTR(png_get_iCCP);
        LOAD_FUNCPTR(png_get_image_height);
        LOAD_FUNCPTR(png_get_image_width);
        LOAD_FUNCPTR(png_get_io_ptr);
        LOAD_FUNCPTR(png_get_pHYs);
        LOAD_FUNCPTR(png_get_PLTE);
        LOAD_FUNCPTR(png_get_tRNS);
        LOAD_FUNCPTR(png_set_bgr);
        LOAD_FUNCPTR(png_set_crc_action);
        LOAD_FUNCPTR(png_set_error_fn);
        LOAD_FUNCPTR(png_set_filler);
        LOAD_FUNCPTR(png_set_filter);
        LOAD_FUNCPTR(png_set_gray_to_rgb);
        LOAD_FUNCPTR(png_set_interlace_handling);
        LOAD_FUNCPTR(png_set_IHDR);
        LOAD_FUNCPTR(png_set_pHYs);
        LOAD_FUNCPTR(png_set_PLTE);
        LOAD_FUNCPTR(png_set_read_fn);
        LOAD_FUNCPTR(png_set_strip_16);
        LOAD_FUNCPTR(png_set_tRNS);
        LOAD_FUNCPTR(png_set_tRNS_to_alpha);
        LOAD_FUNCPTR(png_set_write_fn);
        LOAD_FUNCPTR(png_set_swap);
        LOAD_FUNCPTR(png_read_end);
        LOAD_FUNCPTR(png_read_image);
        LOAD_FUNCPTR(png_read_info);
        LOAD_FUNCPTR(png_write_end);
        LOAD_FUNCPTR(png_write_info);
        LOAD_FUNCPTR(png_write_rows);

#undef LOAD_FUNCPTR
    }

    result = libpng_handle;

    LeaveCriticalSection(&init_png_cs);

    return result;
}

static void user_error_fn(png_structp png_ptr, png_const_charp error_message)
{
    jmp_buf *pjmpbuf;

    /* This uses setjmp/longjmp just like the default. We can't use the
     * default because there's no way to access the jmp buffer in the png_struct
     * that works in 1.2 and 1.4 and allows us to dynamically load libpng. */
    WARN("PNG error: %s\n", debugstr_a(error_message));
    pjmpbuf = ppng_get_error_ptr(png_ptr);
    longjmp(*pjmpbuf, 1);
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_message)
{
    WARN("PNG warning: %s\n", debugstr_a(warning_message));
}

struct png_pixelformat {
    const WICPixelFormatGUID *guid;
    UINT bpp;
    int bit_depth;
    int color_type;
    BOOL remove_filler;
    BOOL swap_rgb;
};

static const struct png_pixelformat formats[] = {
    {&GUID_WICPixelFormat32bppBGRA, 32, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 1},
    {&GUID_WICPixelFormat24bppBGR, 24, 8, PNG_COLOR_TYPE_RGB, 0, 1},
    {&GUID_WICPixelFormatBlackWhite, 1, 1, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat2bppGray, 2, 2, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat4bppGray, 4, 4, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat8bppGray, 8, 8, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat16bppGray, 16, 16, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat32bppBGR, 32, 8, PNG_COLOR_TYPE_RGB, 1, 1},
    {&GUID_WICPixelFormat48bppRGB, 48, 16, PNG_COLOR_TYPE_RGB, 0, 0},
    {&GUID_WICPixelFormat64bppRGBA, 64, 16, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0},
    {&GUID_WICPixelFormat1bppIndexed, 1, 1, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {&GUID_WICPixelFormat2bppIndexed, 2, 2, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {&GUID_WICPixelFormat4bppIndexed, 4, 4, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {&GUID_WICPixelFormat8bppIndexed, 8, 8, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {NULL},
};

typedef struct PngEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    IStream *stream;
    png_structp png_ptr;
    png_infop info_ptr;
    UINT frame_count;
    BOOL frame_initialized;
    const struct png_pixelformat *format;
    BOOL info_written;
    UINT width, height;
    double xres, yres;
    UINT lines_written;
    BOOL frame_committed;
    BOOL committed;
    CRITICAL_SECTION lock;
    BOOL interlace;
    WICPngFilterOption filter;
    BYTE *data;
    UINT stride;
    UINT passes;
    WICColor palette[256];
    UINT colors;
} PngEncoder;

static inline PngEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, PngEncoder, IWICBitmapEncoder_iface);
}

static inline PngEncoder *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, PngEncoder, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI PngFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &This->IWICBitmapFrameEncode_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PngFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_AddRef(&This->IWICBitmapEncoder_iface);
}

static ULONG WINAPI PngFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);
}

static HRESULT WINAPI PngFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    WICPngFilterOption filter;
    BOOL interlace;
    PROPBAG2 opts[2]= {{0}};
    VARIANT opt_values[2];
    HRESULT opt_hres[2];
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    opts[0].pstrName = (LPOLESTR)wszPngInterlaceOption;
    opts[0].vt = VT_BOOL;
    opts[1].pstrName = (LPOLESTR)wszPngFilterOption;
    opts[1].vt = VT_UI1;

    if (pIEncoderOptions)
    {
        hr = IPropertyBag2_Read(pIEncoderOptions, ARRAY_SIZE(opts), opts, NULL, opt_values, opt_hres);

        if (FAILED(hr))
            return hr;

        if (V_VT(&opt_values[0]) == VT_EMPTY)
            interlace = FALSE;
        else
            interlace = (V_BOOL(&opt_values[0]) != 0);

        filter = V_UI1(&opt_values[1]);
        if (filter > WICPngFilterAdaptive)
        {
            WARN("Unrecognized filter option value %u.\n", filter);
            filter = WICPngFilterUnspecified;
        }
    }
    else
    {
        interlace = FALSE;
        filter = WICPngFilterUnspecified;
    }

    EnterCriticalSection(&This->lock);

    if (This->frame_initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->interlace = interlace;
    This->filter = filter;

    This->frame_initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->info_written)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->width = uiWidth;
    This->height = uiHeight;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->info_written)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->xres = dpiX;
    This->yres = dpiY;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->info_written)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!formats[i].guid) i = 0;

    This->format = &formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *palette)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->frame_initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI PngFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    png_byte **row_pointers=NULL;
    UINT i;
    jmp_buf jmpbuf;
    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || !This->width || !This->height || !This->format)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    if (lineCount == 0 || lineCount + This->lines_written > This->height)
    {
        LeaveCriticalSection(&This->lock);
        return E_INVALIDARG;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, row_pointers);
        return E_FAIL;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);

    if (!This->info_written)
    {
        if (This->interlace)
        {
            /* libpng requires us to write all data multiple times in this case. */
            This->stride = (This->format->bpp * This->width + 7)/8;
            This->data = HeapAlloc(GetProcessHeap(), 0, This->height * This->stride);
            if (!This->data)
            {
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }

        /* Tell PNG we need to byte swap if writing a >8-bpp image */
        if (This->format->bit_depth > 8)
            ppng_set_swap(This->png_ptr);

        ppng_set_IHDR(This->png_ptr, This->info_ptr, This->width, This->height,
            This->format->bit_depth, This->format->color_type,
            This->interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        if (This->xres != 0.0 && This->yres != 0.0)
        {
            ppng_set_pHYs(This->png_ptr, This->info_ptr, (This->xres+0.0127) / 0.0254,
                (This->yres+0.0127) / 0.0254, PNG_RESOLUTION_METER);
        }

        if (This->format->color_type == PNG_COLOR_TYPE_PALETTE && This->colors)
        {
            png_color png_palette[256];
            png_byte trans[256];
            UINT i, num_trans = 0, colors;

            /* Newer libpng versions don't accept larger palettes than the declared
             * bit depth, so we need to generate the palette of the correct length.
             */
            colors = min(This->colors, 1 << This->format->bit_depth);

            for (i = 0; i < colors; i++)
            {
                png_palette[i].red = (This->palette[i] >> 16) & 0xff;
                png_palette[i].green = (This->palette[i] >> 8) & 0xff;
                png_palette[i].blue = This->palette[i] & 0xff;
                trans[i] = (This->palette[i] >> 24) & 0xff;
                if (trans[i] != 0xff)
                    num_trans = i+1;
            }

            ppng_set_PLTE(This->png_ptr, This->info_ptr, png_palette, colors);

            if (num_trans)
                ppng_set_tRNS(This->png_ptr, This->info_ptr, trans, num_trans, NULL);
        }

        ppng_write_info(This->png_ptr, This->info_ptr);

        if (This->format->remove_filler)
            ppng_set_filler(This->png_ptr, 0, PNG_FILLER_AFTER);

        if (This->format->swap_rgb)
            ppng_set_bgr(This->png_ptr);

        if (This->interlace)
            This->passes = ppng_set_interlace_handling(This->png_ptr);

        if (This->filter != WICPngFilterUnspecified)
        {
            static const int png_filter_map[] =
            {
                /* WICPngFilterUnspecified */ PNG_NO_FILTERS,
                /* WICPngFilterNone */        PNG_FILTER_NONE,
                /* WICPngFilterSub */         PNG_FILTER_SUB,
                /* WICPngFilterUp */          PNG_FILTER_UP,
                /* WICPngFilterAverage */     PNG_FILTER_AVG,
                /* WICPngFilterPaeth */       PNG_FILTER_PAETH,
                /* WICPngFilterAdaptive */    PNG_ALL_FILTERS,
            };

            ppng_set_filter(This->png_ptr, 0, png_filter_map[This->filter]);
        }

        This->info_written = TRUE;
    }

    if (This->interlace)
    {
        /* Just store the data so we can write it in multiple passes in Commit. */
        for (i=0; i<lineCount; i++)
            memcpy(This->data + This->stride * (This->lines_written + i),
                   pbPixels + cbStride * i,
                   This->stride);

        This->lines_written += lineCount;

        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    row_pointers = HeapAlloc(GetProcessHeap(), 0, lineCount * sizeof(png_byte*));
    if (!row_pointers)
    {
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    for (i=0; i<lineCount; i++)
        row_pointers[i] = pbPixels + cbStride * i;

    ppng_write_rows(This->png_ptr, row_pointers, lineCount);
    This->lines_written += lineCount;

    LeaveCriticalSection(&This->lock);

    HeapFree(GetProcessHeap(), 0, row_pointers);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;
    TRACE("(%p,%p,%s)\n", iface, pIBitmapSource, debug_wic_rect(prc));

    if (!This->frame_initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        This->format ? This->format->guid : NULL, This->width, This->height,
        This->xres, This->yres);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            This->format->guid, This->format->bpp,
            !This->colors && This->format->color_type == PNG_COLOR_TYPE_PALETTE,
            This->width, This->height);
    }

    return hr;
}

static HRESULT WINAPI PngFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    png_byte **row_pointers=NULL;
    jmp_buf jmpbuf;
    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->info_written || This->lines_written != This->height || This->frame_committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, row_pointers);
        return E_FAIL;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);

    if (This->interlace)
    {
        int i;

        row_pointers = HeapAlloc(GetProcessHeap(), 0, This->height * sizeof(png_byte*));
        if (!row_pointers)
        {
            LeaveCriticalSection(&This->lock);
            return E_OUTOFMEMORY;
        }

        for (i=0; i<This->height; i++)
            row_pointers[i] = This->data + This->stride * i;

        for (i=0; i<This->passes; i++)
            ppng_write_rows(This->png_ptr, row_pointers, This->height);
    }

    ppng_write_end(This->png_ptr, This->info_ptr);

    This->frame_committed = TRUE;

    HeapFree(GetProcessHeap(), 0, row_pointers);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl PngEncoder_FrameVtbl = {
    PngFrameEncode_QueryInterface,
    PngFrameEncode_AddRef,
    PngFrameEncode_Release,
    PngFrameEncode_Initialize,
    PngFrameEncode_SetSize,
    PngFrameEncode_SetResolution,
    PngFrameEncode_SetPixelFormat,
    PngFrameEncode_SetColorContexts,
    PngFrameEncode_SetPalette,
    PngFrameEncode_SetThumbnail,
    PngFrameEncode_WritePixels,
    PngFrameEncode_WriteSource,
    PngFrameEncode_Commit,
    PngFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI PngEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        *ppv = &This->IWICBitmapEncoder_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PngEncoder_AddRef(IWICBitmapEncoder *iface)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PngEncoder_Release(IWICBitmapEncoder *iface)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->png_ptr)
            ppng_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        if (This->stream)
            IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This->data);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    PngEncoder *This = ppng_get_io_ptr(png_ptr);
    HRESULT hr;
    ULONG byteswritten;

    hr = IStream_Write(This->stream, data, length, &byteswritten);
    if (FAILED(hr) || byteswritten != length)
    {
        ppng_error(png_ptr, "failed writing data");
    }
}

static void user_flush(png_structp png_ptr)
{
}

static HRESULT WINAPI PngEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    jmp_buf jmpbuf;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->png_ptr)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* initialize libpng */
    This->png_ptr = ppng_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!This->png_ptr)
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    This->info_ptr = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_write_struct(&This->png_ptr, NULL);
        This->png_ptr = NULL;
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        ppng_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        This->png_ptr = NULL;
        IStream_Release(This->stream);
        This->stream = NULL;
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);

    /* set up custom i/o handling */
    ppng_set_write_fn(This->png_ptr, This, user_write_data, user_flush);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    TRACE("(%p,%p)\n", iface, format);

    if (!format)
        return E_INVALIDARG;

    memcpy(format, &GUID_ContainerFormatPng, sizeof(*format));
    return S_OK;
}

static HRESULT WINAPI PngEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICPngEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI PngEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    EnterCriticalSection(&This->lock);

    hr = This->stream ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI PngEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;
    static const PROPBAG2 opts[2] =
    {
        { PROPBAG2_TYPE_DATA, VT_BOOL, 0, 0, (LPOLESTR)wszPngInterlaceOption },
        { PROPBAG2_TYPE_DATA, VT_UI1,  0, 0, (LPOLESTR)wszPngFilterOption },
    };

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_count != 0)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    if (!This->stream)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    if (ppIEncoderOptions)
    {
        hr = CreatePropertyBag2(opts, ARRAY_SIZE(opts), ppIEncoderOptions);
        if (FAILED(hr))
        {
            LeaveCriticalSection(&This->lock);
            return hr;
        }
    }

    This->frame_count = 1;

    LeaveCriticalSection(&This->lock);

    IWICBitmapEncoder_AddRef(iface);
    *ppIFrameEncode = &This->IWICBitmapFrameEncode_iface;

    return S_OK;
}

static HRESULT WINAPI PngEncoder_Commit(IWICBitmapEncoder *iface)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->frame_committed || This->committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->committed = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl PngEncoder_Vtbl = {
    PngEncoder_QueryInterface,
    PngEncoder_AddRef,
    PngEncoder_Release,
    PngEncoder_Initialize,
    PngEncoder_GetContainerFormat,
    PngEncoder_GetEncoderInfo,
    PngEncoder_SetColorContexts,
    PngEncoder_SetPalette,
    PngEncoder_SetThumbnail,
    PngEncoder_SetPreview,
    PngEncoder_CreateNewFrame,
    PngEncoder_Commit,
    PngEncoder_GetMetadataQueryWriter
};

HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv)
{
    PngEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!load_libpng())
    {
        ERR("Failed writing PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PngEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &PngEncoder_Vtbl;
    This->IWICBitmapFrameEncode_iface.lpVtbl = &PngEncoder_FrameVtbl;
    This->ref = 1;
    This->png_ptr = NULL;
    This->info_ptr = NULL;
    This->stream = NULL;
    This->frame_count = 0;
    This->frame_initialized = FALSE;
    This->format = NULL;
    This->info_written = FALSE;
    This->width = 0;
    This->height = 0;
    This->xres = 0.0;
    This->yres = 0.0;
    This->lines_written = 0;
    This->frame_committed = FALSE;
    This->committed = FALSE;
    This->data = NULL;
    This->colors = 0;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PngEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !SONAME_LIBPNG */

HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save PNG picture, but PNG support is not compiled in.\n");
    return E_FAIL;
}

#endif

HRESULT PngDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct decoder *decoder;
    struct decoder_info decoder_info;

    hr = get_unix_decoder(&CLSID_WICPngDecoder, &decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}
