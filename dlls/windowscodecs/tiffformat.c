/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBTIFF

/* Workaround for broken libtiff 4.x headers on some 64-bit hosts which
 * define TIFF_UINT64_T/toff_t as 32-bit for 32-bit builds, while they
 * are supposed to be always 64-bit.
 * TIFF_UINT64_T doesn't exist in libtiff 3.x, it was introduced in 4.x.
 */
#ifdef TIFF_UINT64_T
# undef toff_t
# define toff_t UINT64
#endif

static CRITICAL_SECTION init_tiff_cs;
static CRITICAL_SECTION_DEBUG init_tiff_cs_debug =
{
    0, 0, &init_tiff_cs,
    { &init_tiff_cs_debug.ProcessLocksList,
      &init_tiff_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_tiff_cs") }
};
static CRITICAL_SECTION init_tiff_cs = { &init_tiff_cs_debug, -1, 0, 0, 0, 0 };

static const WCHAR wszTiffCompressionMethod[] = {'T','i','f','f','C','o','m','p','r','e','s','s','i','o','n','M','e','t','h','o','d',0};
static const WCHAR wszCompressionQuality[] = {'C','o','m','p','r','e','s','s','i','o','n','Q','u','a','l','i','t','y',0};

static void *libtiff_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(TIFFClientOpen);
MAKE_FUNCPTR(TIFFClose);
MAKE_FUNCPTR(TIFFCurrentDirOffset);
MAKE_FUNCPTR(TIFFGetField);
MAKE_FUNCPTR(TIFFIsByteSwapped);
MAKE_FUNCPTR(TIFFNumberOfDirectories);
MAKE_FUNCPTR(TIFFReadDirectory);
MAKE_FUNCPTR(TIFFReadEncodedStrip);
MAKE_FUNCPTR(TIFFReadEncodedTile);
MAKE_FUNCPTR(TIFFSetDirectory);
MAKE_FUNCPTR(TIFFSetField);
MAKE_FUNCPTR(TIFFWriteDirectory);
MAKE_FUNCPTR(TIFFWriteScanline);
#undef MAKE_FUNCPTR

static void *load_libtiff(void)
{
    void *result;

    EnterCriticalSection(&init_tiff_cs);

    if (!libtiff_handle &&
        (libtiff_handle = dlopen(SONAME_LIBTIFF, RTLD_NOW)) != NULL)
    {
        void * (*pTIFFSetWarningHandler)(void *);
        void * (*pTIFFSetWarningHandlerExt)(void *);

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(libtiff_handle, #f)) == NULL) { \
        ERR("failed to load symbol %s\n", #f); \
        libtiff_handle = NULL; \
        LeaveCriticalSection(&init_tiff_cs); \
        return NULL; \
    }
        LOAD_FUNCPTR(TIFFClientOpen);
        LOAD_FUNCPTR(TIFFClose);
        LOAD_FUNCPTR(TIFFCurrentDirOffset);
        LOAD_FUNCPTR(TIFFGetField);
        LOAD_FUNCPTR(TIFFIsByteSwapped);
        LOAD_FUNCPTR(TIFFNumberOfDirectories);
        LOAD_FUNCPTR(TIFFReadDirectory);
        LOAD_FUNCPTR(TIFFReadEncodedStrip);
        LOAD_FUNCPTR(TIFFReadEncodedTile);
        LOAD_FUNCPTR(TIFFSetDirectory);
        LOAD_FUNCPTR(TIFFSetField);
        LOAD_FUNCPTR(TIFFWriteDirectory);
        LOAD_FUNCPTR(TIFFWriteScanline);
#undef LOAD_FUNCPTR

        if ((pTIFFSetWarningHandler = dlsym(libtiff_handle, "TIFFSetWarningHandler")))
            pTIFFSetWarningHandler(NULL);
        if ((pTIFFSetWarningHandlerExt = dlsym(libtiff_handle, "TIFFSetWarningHandlerExt")))
            pTIFFSetWarningHandlerExt(NULL);
    }

    result = libtiff_handle;

    LeaveCriticalSection(&init_tiff_cs);
    return result;
}

static tsize_t tiff_stream_read(thandle_t client_data, tdata_t data, tsize_t size)
{
    IStream *stream = (IStream*)client_data;
    ULONG bytes_read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &bytes_read);
    if (FAILED(hr)) bytes_read = 0;
    return bytes_read;
}

static tsize_t tiff_stream_write(thandle_t client_data, tdata_t data, tsize_t size)
{
    IStream *stream = (IStream*)client_data;
    ULONG bytes_written;
    HRESULT hr;

    hr = IStream_Write(stream, data, size, &bytes_written);
    if (FAILED(hr)) bytes_written = 0;
    return bytes_written;
}

static toff_t tiff_stream_seek(thandle_t client_data, toff_t offset, int whence)
{
    IStream *stream = (IStream*)client_data;
    LARGE_INTEGER move;
    DWORD origin;
    ULARGE_INTEGER new_position;
    HRESULT hr;

    move.QuadPart = offset;
    switch (whence)
    {
        case SEEK_SET:
            origin = STREAM_SEEK_SET;
            break;
        case SEEK_CUR:
            origin = STREAM_SEEK_CUR;
            break;
        case SEEK_END:
            origin = STREAM_SEEK_END;
            break;
        default:
            ERR("unknown whence value %i\n", whence);
            return -1;
    }

    hr = IStream_Seek(stream, move, origin, &new_position);
    if (SUCCEEDED(hr)) return new_position.QuadPart;
    else return -1;
}

static int tiff_stream_close(thandle_t client_data)
{
    /* Caller is responsible for releasing the stream object. */
    return 0;
}

static toff_t tiff_stream_size(thandle_t client_data)
{
    IStream *stream = (IStream*)client_data;
    STATSTG statstg;
    HRESULT hr;

    hr = IStream_Stat(stream, &statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr)) return statstg.cbSize.QuadPart;
    else return -1;
}

static int tiff_stream_map(thandle_t client_data, tdata_t *addr, toff_t *size)
{
    /* Cannot mmap streams */
    return 0;
}

static void tiff_stream_unmap(thandle_t client_data, tdata_t addr, toff_t size)
{
    /* No need to ever do this, since we can't map things. */
}

static TIFF* tiff_open_stream(IStream *stream, const char *mode)
{
    LARGE_INTEGER zero;

    zero.QuadPart = 0;
    IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

    return pTIFFClientOpen("<IStream object>", mode, stream, tiff_stream_read,
        tiff_stream_write, (void *)tiff_stream_seek, tiff_stream_close,
        (void *)tiff_stream_size, (void *)tiff_stream_map, (void *)tiff_stream_unmap);
}

struct tiff_encode_format {
    const WICPixelFormatGUID *guid;
    int photometric;
    int bps;
    int samples;
    int bpp;
    int extra_sample;
    int extra_sample_type;
    int reverse_bgr;
};

static const struct tiff_encode_format formats[] = {
    {&GUID_WICPixelFormat24bppBGR, 2, 8, 3, 24, 0, 0, 1},
    {&GUID_WICPixelFormat24bppRGB, 2, 8, 3, 24, 0, 0, 0},
    {&GUID_WICPixelFormatBlackWhite, 1, 1, 1, 1, 0, 0, 0},
    {&GUID_WICPixelFormat4bppGray, 1, 4, 1, 4, 0, 0, 0},
    {&GUID_WICPixelFormat8bppGray, 1, 8, 1, 8, 0, 0, 0},
    {&GUID_WICPixelFormat32bppBGRA, 2, 8, 4, 32, 1, 2, 1},
    {&GUID_WICPixelFormat32bppPBGRA, 2, 8, 4, 32, 1, 1, 1},
    {&GUID_WICPixelFormat48bppRGB, 2, 16, 3, 48, 0, 0, 0},
    {&GUID_WICPixelFormat64bppRGBA, 2, 16, 4, 64, 1, 2, 0},
    {&GUID_WICPixelFormat64bppPRGBA, 2, 16, 4, 64, 1, 1, 0},
    {&GUID_WICPixelFormat1bppIndexed, 3, 1, 1, 1, 0, 0, 0},
    {&GUID_WICPixelFormat4bppIndexed, 3, 4, 1, 4, 0, 0, 0},
    {&GUID_WICPixelFormat8bppIndexed, 3, 8, 1, 8, 0, 0, 0},
    {0}
};

typedef struct TiffEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock; /* Must be held when tiff is used or fields below are set */
    TIFF *tiff;
    BOOL initialized;
    BOOL committed;
    ULONG num_frames;
    ULONG num_frames_committed;
} TiffEncoder;

static inline TiffEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, TiffEncoder, IWICBitmapEncoder_iface);
}

typedef struct TiffFrameEncode {
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    TiffEncoder *parent;
    /* fields below are protected by parent->lock */
    BOOL initialized;
    BOOL info_written;
    BOOL committed;
    const struct tiff_encode_format *format;
    UINT width, height;
    double xres, yres;
    UINT lines_written;
    WICColor palette[256];
    UINT colors;
} TiffFrameEncode;

static inline TiffFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, TiffFrameEncode, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI TiffFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
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

static ULONG WINAPI TiffFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapEncoder_Release(&This->parent->IWICBitmapEncoder_iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TiffFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    EnterCriticalSection(&This->parent->lock);

    if (This->initialized)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->initialized = TRUE;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->info_written)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->width = uiWidth;
    This->height = uiHeight;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->info_written)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->xres = dpiX;
    This->yres = dpiY;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;

    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->info_written)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    if (IsEqualGUID(pPixelFormat, &GUID_WICPixelFormat2bppIndexed))
        *pPixelFormat = GUID_WICPixelFormat4bppIndexed;

    for (i=0; formats[i].guid; i++)
    {
        if (IsEqualGUID(formats[i].guid, pPixelFormat))
            break;
    }

    if (!formats[i].guid) i = 0;

    This->format = &formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *palette)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->parent->lock);

    if (This->initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->parent->lock);
    return hr;
}

static HRESULT WINAPI TiffFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    BYTE *row_data, *swapped_data = NULL;
    UINT i, j, line_size;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || !This->width || !This->height || !This->format)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    if (lineCount == 0 || lineCount + This->lines_written > This->height)
    {
        LeaveCriticalSection(&This->parent->lock);
        return E_INVALIDARG;
    }

    line_size = ((This->width * This->format->bpp)+7)/8;

    if (This->format->reverse_bgr)
    {
        swapped_data = HeapAlloc(GetProcessHeap(), 0, line_size);
        if (!swapped_data)
        {
            LeaveCriticalSection(&This->parent->lock);
            return E_OUTOFMEMORY;
        }
    }

    if (!This->info_written)
    {
        pTIFFSetField(This->parent->tiff, TIFFTAG_PHOTOMETRIC, (uint16)This->format->photometric);
        pTIFFSetField(This->parent->tiff, TIFFTAG_PLANARCONFIG, (uint16)1);
        pTIFFSetField(This->parent->tiff, TIFFTAG_BITSPERSAMPLE, (uint16)This->format->bps);
        pTIFFSetField(This->parent->tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)This->format->samples);

        if (This->format->extra_sample)
        {
            uint16 extra_samples;
            extra_samples = This->format->extra_sample_type;

            pTIFFSetField(This->parent->tiff, TIFFTAG_EXTRASAMPLES, (uint16)1, &extra_samples);
        }

        pTIFFSetField(This->parent->tiff, TIFFTAG_IMAGEWIDTH, (uint32)This->width);
        pTIFFSetField(This->parent->tiff, TIFFTAG_IMAGELENGTH, (uint32)This->height);

        if (This->xres != 0.0 && This->yres != 0.0)
        {
            pTIFFSetField(This->parent->tiff, TIFFTAG_RESOLUTIONUNIT, (uint16)2); /* Inch */
            pTIFFSetField(This->parent->tiff, TIFFTAG_XRESOLUTION, (float)This->xres);
            pTIFFSetField(This->parent->tiff, TIFFTAG_YRESOLUTION, (float)This->yres);
        }

        if (This->format->bpp <= 8 && This->colors && !IsEqualGUID(This->format->guid, &GUID_WICPixelFormatBlackWhite))
        {
            uint16 red[256], green[256], blue[256];
            UINT i;

            for (i = 0; i < This->colors; i++)
            {
                red[i] = (This->palette[i] >> 8) & 0xff00;
                green[i] = This->palette[i] & 0xff00;
                blue[i] = (This->palette[i] << 8) & 0xff00;
            }

            pTIFFSetField(This->parent->tiff, TIFFTAG_COLORMAP, red, green, blue);
        }

        This->info_written = TRUE;
    }

    for (i=0; i<lineCount; i++)
    {
        row_data = pbPixels + i * cbStride;

        if (This->format->reverse_bgr && This->format->bps == 8)
        {
            memcpy(swapped_data, row_data, line_size);
            for (j=0; j<line_size; j += This->format->samples)
            {
                BYTE temp;
                temp = swapped_data[j];
                swapped_data[j] = swapped_data[j+2];
                swapped_data[j+2] = temp;
            }
            row_data = swapped_data;
        }

        pTIFFWriteScanline(This->parent->tiff, (tdata_t)row_data, i+This->lines_written, 0);
    }

    This->lines_written += lineCount;

    LeaveCriticalSection(&This->parent->lock);

    HeapFree(GetProcessHeap(), 0, swapped_data);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p,%s)\n", iface, pIBitmapSource, debug_wic_rect(prc));

    if (!This->initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        This->format ? This->format->guid : NULL, This->width, This->height,
        This->xres, This->yres);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            This->format->guid, This->format->bpp,
            !This->colors && This->format->bpp <= 8 && !IsEqualGUID(This->format->guid, &GUID_WICPixelFormatBlackWhite),
            This->width, This->height);
    }

    return hr;
}

static HRESULT WINAPI TiffFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->parent->lock);

    if (!This->info_written || This->lines_written != This->height || This->committed)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* libtiff will commit the data when creating a new frame or closing the file */

    This->committed = TRUE;
    This->parent->num_frames_committed++;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl TiffFrameEncode_Vtbl = {
    TiffFrameEncode_QueryInterface,
    TiffFrameEncode_AddRef,
    TiffFrameEncode_Release,
    TiffFrameEncode_Initialize,
    TiffFrameEncode_SetSize,
    TiffFrameEncode_SetResolution,
    TiffFrameEncode_SetPixelFormat,
    TiffFrameEncode_SetColorContexts,
    TiffFrameEncode_SetPalette,
    TiffFrameEncode_SetThumbnail,
    TiffFrameEncode_WritePixels,
    TiffFrameEncode_WriteSource,
    TiffFrameEncode_Commit,
    TiffFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI TiffEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static ULONG WINAPI TiffEncoder_AddRef(IWICBitmapEncoder *iface)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffEncoder_Release(IWICBitmapEncoder *iface)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->tiff) pTIFFClose(This->tiff);
        if (This->stream) IStream_Release(This->stream);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TiffEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TIFF *tiff;
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->initialized || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto exit;
    }

    tiff = tiff_open_stream(pIStream, "w");

    if (!tiff)
    {
        hr = E_FAIL;
        goto exit;
    }

    This->tiff = tiff;
    This->stream = pIStream;
    IStream_AddRef(pIStream);
    This->initialized = TRUE;

exit:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI TiffEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    TRACE("(%p,%p)\n", iface, pguidContainerFormat);

    if (!pguidContainerFormat)
        return E_INVALIDARG;

    memcpy(pguidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TiffEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICTiffEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI TiffEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    EnterCriticalSection(&This->lock);

    hr = This->stream ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI TiffEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TiffFrameEncode *result;
    static const PROPBAG2 opts[2] =
    {
        { PROPBAG2_TYPE_DATA, VT_UI1, 0, 0, (LPOLESTR)wszTiffCompressionMethod },
        { PROPBAG2_TYPE_DATA, VT_R4,  0, 0, (LPOLESTR)wszCompressionQuality },
    };
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (!This->initialized || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else if (This->num_frames != This->num_frames_committed)
    {
        FIXME("New frame created before previous frame was committed\n");
        hr = E_FAIL;
    }

    if (ppIEncoderOptions && SUCCEEDED(hr))
    {
        hr = CreatePropertyBag2(opts, ARRAY_SIZE(opts), ppIEncoderOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT v;
            VariantInit(&v);
            V_VT(&v) = VT_UI1;
            V_UI1(&v) = WICTiffCompressionDontCare;
            hr = IPropertyBag2_Write(*ppIEncoderOptions, 1, (PROPBAG2 *)opts, &v);
            VariantClear(&v);
            if (FAILED(hr))
            {
                IPropertyBag2_Release(*ppIEncoderOptions);
                *ppIEncoderOptions = NULL;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        result = HeapAlloc(GetProcessHeap(), 0, sizeof(*result));

        if (result)
        {
            result->IWICBitmapFrameEncode_iface.lpVtbl = &TiffFrameEncode_Vtbl;
            result->ref = 1;
            result->parent = This;
            result->initialized = FALSE;
            result->info_written = FALSE;
            result->committed = FALSE;
            result->format = NULL;
            result->width = 0;
            result->height = 0;
            result->xres = 0.0;
            result->yres = 0.0;
            result->lines_written = 0;
            result->colors = 0;

            IWICBitmapEncoder_AddRef(iface);
            *ppIFrameEncode = &result->IWICBitmapFrameEncode_iface;

            if (This->num_frames != 0)
                pTIFFWriteDirectory(This->tiff);

            This->num_frames++;
        }
        else
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
        {
            IPropertyBag2_Release(*ppIEncoderOptions);
            *ppIEncoderOptions = NULL;
        }
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI TiffEncoder_Commit(IWICBitmapEncoder *iface)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->initialized || This->committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    pTIFFClose(This->tiff);
    IStream_Release(This->stream);
    This->stream = NULL;
    This->tiff = NULL;

    This->committed = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI TiffEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl TiffEncoder_Vtbl = {
    TiffEncoder_QueryInterface,
    TiffEncoder_AddRef,
    TiffEncoder_Release,
    TiffEncoder_Initialize,
    TiffEncoder_GetContainerFormat,
    TiffEncoder_GetEncoderInfo,
    TiffEncoder_SetColorContexts,
    TiffEncoder_SetPalette,
    TiffEncoder_SetThumbnail,
    TiffEncoder_SetPreview,
    TiffEncoder_CreateNewFrame,
    TiffEncoder_Commit,
    TiffEncoder_GetMetadataQueryWriter
};

HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv)
{
    TiffEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!load_libtiff())
    {
        ERR("Failed writing TIFF because unable to load %s\n",SONAME_LIBTIFF);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TiffEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &TiffEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TiffEncoder.lock");
    This->tiff = NULL;
    This->initialized = FALSE;
    This->num_frames = 0;
    This->num_frames_committed = 0;
    This->committed = FALSE;

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !SONAME_LIBTIFF */

HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save TIFF picture, but Wine was compiled without TIFF support.\n");
    return E_FAIL;
}

#endif

HRESULT TiffDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct decoder *decoder;
    struct decoder_info decoder_info;

    hr = get_unix_decoder(&CLSID_WICTiffDecoder, &decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}
