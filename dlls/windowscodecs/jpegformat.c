/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifdef SONAME_LIBJPEG
/* This is a hack, so jpeglib.h does not redefine INT32 and the like*/
#define XMD_H
#define UINT8 JPEG_UINT8
#define UINT16 JPEG_UINT16
#define boolean jpeg_boolean
#undef HAVE_STDLIB_H
# include <jpeglib.h>
#undef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1
#undef UINT8
#undef UINT16
#undef boolean
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBJPEG
WINE_DECLARE_DEBUG_CHANNEL(jpeg);

static void *libjpeg_handle;

static const WCHAR wszImageQuality[] = {'I','m','a','g','e','Q','u','a','l','i','t','y',0};
static const WCHAR wszBitmapTransform[] = {'B','i','t','m','a','p','T','r','a','n','s','f','o','r','m',0};
static const WCHAR wszLuminance[] = {'L','u','m','i','n','a','n','c','e',0};
static const WCHAR wszChrominance[] = {'C','h','r','o','m','i','n','a','n','c','e',0};
static const WCHAR wszJpegYCrCbSubsampling[] = {'J','p','e','g','Y','C','r','C','b','S','u','b','s','a','m','p','l','i','n','g',0};
static const WCHAR wszSuppressApp0[] = {'S','u','p','p','r','e','s','s','A','p','p','0',0};

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(jpeg_CreateCompress);
MAKE_FUNCPTR(jpeg_CreateDecompress);
MAKE_FUNCPTR(jpeg_destroy_compress);
MAKE_FUNCPTR(jpeg_destroy_decompress);
MAKE_FUNCPTR(jpeg_finish_compress);
MAKE_FUNCPTR(jpeg_read_header);
MAKE_FUNCPTR(jpeg_read_scanlines);
MAKE_FUNCPTR(jpeg_resync_to_restart);
MAKE_FUNCPTR(jpeg_set_defaults);
MAKE_FUNCPTR(jpeg_start_compress);
MAKE_FUNCPTR(jpeg_start_decompress);
MAKE_FUNCPTR(jpeg_std_error);
MAKE_FUNCPTR(jpeg_write_scanlines);
#undef MAKE_FUNCPTR

static void *load_libjpeg(void)
{
    if((libjpeg_handle = dlopen(SONAME_LIBJPEG, RTLD_NOW)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(libjpeg_handle, #f)) == NULL) { \
        libjpeg_handle = NULL; \
        return NULL; \
    }

        LOAD_FUNCPTR(jpeg_CreateCompress);
        LOAD_FUNCPTR(jpeg_CreateDecompress);
        LOAD_FUNCPTR(jpeg_destroy_compress);
        LOAD_FUNCPTR(jpeg_destroy_decompress);
        LOAD_FUNCPTR(jpeg_finish_compress);
        LOAD_FUNCPTR(jpeg_read_header);
        LOAD_FUNCPTR(jpeg_read_scanlines);
        LOAD_FUNCPTR(jpeg_resync_to_restart);
        LOAD_FUNCPTR(jpeg_set_defaults);
        LOAD_FUNCPTR(jpeg_start_compress);
        LOAD_FUNCPTR(jpeg_start_decompress);
        LOAD_FUNCPTR(jpeg_std_error);
        LOAD_FUNCPTR(jpeg_write_scanlines);
#undef LOAD_FUNCPTR
    }
    return libjpeg_handle;
}

static void error_exit_fn(j_common_ptr cinfo)
{
    char message[JMSG_LENGTH_MAX];
    if (ERR_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        ERR_(jpeg)("%s\n", message);
    }
    longjmp(*(jmp_buf*)cinfo->client_data, 1);
}

static void emit_message_fn(j_common_ptr cinfo, int msg_level)
{
    char message[JMSG_LENGTH_MAX];

    if (msg_level < 0 && ERR_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        ERR_(jpeg)("%s\n", message);
    }
    else if (msg_level == 0 && WARN_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        WARN_(jpeg)("%s\n", message);
    }
    else if (msg_level > 0 && TRACE_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        TRACE_(jpeg)("%s\n", message);
    }
}

typedef struct jpeg_compress_format {
    const WICPixelFormatGUID *guid;
    int bpp;
    int num_components;
    J_COLOR_SPACE color_space;
    int swap_rgb;
} jpeg_compress_format;

static const jpeg_compress_format compress_formats[] = {
    { &GUID_WICPixelFormat24bppBGR, 24, 3, JCS_RGB, 1 },
    { &GUID_WICPixelFormat32bppCMYK, 32, 4, JCS_CMYK },
    { &GUID_WICPixelFormat8bppGray, 8, 1, JCS_GRAYSCALE },
    { 0 }
};

typedef struct JpegEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_destination_mgr dest_mgr;
    BOOL initialized;
    int frame_count;
    BOOL frame_initialized;
    BOOL started_compress;
    int lines_written;
    BOOL frame_committed;
    BOOL committed;
    UINT width, height;
    double xres, yres;
    const jpeg_compress_format *format;
    IStream *stream;
    WICColor palette[256];
    UINT colors;
    CRITICAL_SECTION lock;
    BYTE dest_buffer[1024];
} JpegEncoder;

static inline JpegEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, JpegEncoder, IWICBitmapEncoder_iface);
}

static inline JpegEncoder *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, JpegEncoder, IWICBitmapFrameEncode_iface);
}

static inline JpegEncoder *encoder_from_compress(j_compress_ptr compress)
{
    return CONTAINING_RECORD(compress, JpegEncoder, cinfo);
}

static void dest_mgr_init_destination(j_compress_ptr cinfo)
{
    JpegEncoder *This = encoder_from_compress(cinfo);

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);
}

static jpeg_boolean dest_mgr_empty_output_buffer(j_compress_ptr cinfo)
{
    JpegEncoder *This = encoder_from_compress(cinfo);
    HRESULT hr;
    ULONG byteswritten;

    hr = IStream_Write(This->stream, This->dest_buffer,
        sizeof(This->dest_buffer), &byteswritten);

    if (hr != S_OK || byteswritten == 0)
    {
        ERR("Failed writing data, hr=%x\n", hr);
        return FALSE;
    }

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);
    return TRUE;
}

static void dest_mgr_term_destination(j_compress_ptr cinfo)
{
    JpegEncoder *This = encoder_from_compress(cinfo);
    ULONG byteswritten;
    HRESULT hr;

    if (This->dest_mgr.free_in_buffer != sizeof(This->dest_buffer))
    {
        hr = IStream_Write(This->stream, This->dest_buffer,
            sizeof(This->dest_buffer) - This->dest_mgr.free_in_buffer, &byteswritten);

        if (hr != S_OK || byteswritten == 0)
            ERR("Failed writing data, hr=%x\n", hr);
    }
}

static HRESULT WINAPI JpegEncoder_Frame_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
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

static ULONG WINAPI JpegEncoder_Frame_AddRef(IWICBitmapFrameEncode *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_AddRef(&This->IWICBitmapEncoder_iface);
}

static ULONG WINAPI JpegEncoder_Frame_Release(IWICBitmapFrameEncode *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);
}

static HRESULT WINAPI JpegEncoder_Frame_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->frame_initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->started_compress)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->width = uiWidth;
    This->height = uiHeight;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->started_compress)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->xres = dpiX;
    This->yres = dpiY;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->started_compress)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    for (i=0; compress_formats[i].guid; i++)
    {
        if (memcmp(compress_formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!compress_formats[i].guid) i = 0;

    This->format = &compress_formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI JpegEncoder_Frame_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *palette)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
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

static HRESULT WINAPI JpegEncoder_Frame_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegEncoder_Frame_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    jmp_buf jmpbuf;
    BYTE *swapped_data = NULL, *current_row;
    UINT line;
    int row_size;
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
        HeapFree(GetProcessHeap(), 0, swapped_data);
        return E_FAIL;
    }
    This->cinfo.client_data = jmpbuf;

    if (!This->started_compress)
    {
        This->cinfo.image_width = This->width;
        This->cinfo.image_height = This->height;
        This->cinfo.input_components = This->format->num_components;
        This->cinfo.in_color_space = This->format->color_space;

        pjpeg_set_defaults(&This->cinfo);

        if (This->xres != 0.0 && This->yres != 0.0)
        {
            This->cinfo.density_unit = 1; /* dots per inch */
            This->cinfo.X_density = This->xres;
            This->cinfo.Y_density = This->yres;
        }

        pjpeg_start_compress(&This->cinfo, TRUE);

        This->started_compress = TRUE;
    }

    row_size = This->format->bpp / 8 * This->width;

    if (This->format->swap_rgb)
    {
        swapped_data = HeapAlloc(GetProcessHeap(), 0, row_size);
        if (!swapped_data)
        {
            LeaveCriticalSection(&This->lock);
            return E_OUTOFMEMORY;
        }
    }

    for (line=0; line < lineCount; line++)
    {
        if (This->format->swap_rgb)
        {
            UINT x;

            memcpy(swapped_data, pbPixels + (cbStride * line), row_size);

            for (x=0; x < This->width; x++)
            {
                BYTE b;

                b = swapped_data[x*3];
                swapped_data[x*3] = swapped_data[x*3+2];
                swapped_data[x*3+2] = b;
            }

            current_row = swapped_data;
        }
        else
            current_row = pbPixels + (cbStride * line);

        if (!pjpeg_write_scanlines(&This->cinfo, &current_row, 1))
        {
            ERR("failed writing scanlines\n");
            LeaveCriticalSection(&This->lock);
            HeapFree(GetProcessHeap(), 0, swapped_data);
            return E_FAIL;
        }

        This->lines_written++;
    }

    LeaveCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, swapped_data);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
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
            This->format->guid, This->format->bpp, FALSE,
            This->width, This->height);
    }

    return hr;
}

static HRESULT WINAPI JpegEncoder_Frame_Commit(IWICBitmapFrameEncode *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    jmp_buf jmpbuf;
    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->started_compress || This->lines_written != This->height || This->frame_committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }
    This->cinfo.client_data = jmpbuf;

    pjpeg_finish_compress(&This->cinfo);

    This->frame_committed = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl JpegEncoder_FrameVtbl = {
    JpegEncoder_Frame_QueryInterface,
    JpegEncoder_Frame_AddRef,
    JpegEncoder_Frame_Release,
    JpegEncoder_Frame_Initialize,
    JpegEncoder_Frame_SetSize,
    JpegEncoder_Frame_SetResolution,
    JpegEncoder_Frame_SetPixelFormat,
    JpegEncoder_Frame_SetColorContexts,
    JpegEncoder_Frame_SetPalette,
    JpegEncoder_Frame_SetThumbnail,
    JpegEncoder_Frame_WritePixels,
    JpegEncoder_Frame_WriteSource,
    JpegEncoder_Frame_Commit,
    JpegEncoder_Frame_GetMetadataQueryWriter
};

static HRESULT WINAPI JpegEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static ULONG WINAPI JpegEncoder_AddRef(IWICBitmapEncoder *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI JpegEncoder_Release(IWICBitmapEncoder *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->initialized) pjpeg_destroy_compress(&This->cinfo);
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI JpegEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    jmp_buf jmpbuf;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    pjpeg_std_error(&This->jerr);

    This->jerr.error_exit = error_exit_fn;
    This->jerr.emit_message = emit_message_fn;

    This->cinfo.err = &This->jerr;

    This->cinfo.client_data = jmpbuf;

    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    pjpeg_CreateCompress(&This->cinfo, JPEG_LIB_VERSION, sizeof(struct jpeg_compress_struct));

    This->stream = pIStream;
    IStream_AddRef(pIStream);

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);

    This->dest_mgr.init_destination = dest_mgr_init_destination;
    This->dest_mgr.empty_output_buffer = dest_mgr_empty_output_buffer;
    This->dest_mgr.term_destination = dest_mgr_term_destination;

    This->cinfo.dest = &This->dest_mgr;

    This->initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    TRACE("(%p,%p)\n", iface, format);

    if (!format)
        return E_INVALIDARG;

    memcpy(format, &GUID_ContainerFormatJpeg, sizeof(*format));
    return S_OK;
}

static HRESULT WINAPI JpegEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICJpegEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI JpegEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI JpegEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pIPalette);

    EnterCriticalSection(&This->lock);

    hr = This->initialized ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI JpegEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;
    static const PROPBAG2 opts[6] =
    {
        { PROPBAG2_TYPE_DATA, VT_R4,            0, 0, (LPOLESTR)wszImageQuality },
        { PROPBAG2_TYPE_DATA, VT_UI1,           0, 0, (LPOLESTR)wszBitmapTransform },
        { PROPBAG2_TYPE_DATA, VT_I4 | VT_ARRAY, 0, 0, (LPOLESTR)wszLuminance },
        { PROPBAG2_TYPE_DATA, VT_I4 | VT_ARRAY, 0, 0, (LPOLESTR)wszChrominance },
        { PROPBAG2_TYPE_DATA, VT_UI1,           0, 0, (LPOLESTR)wszJpegYCrCbSubsampling },
        { PROPBAG2_TYPE_DATA, VT_BOOL,          0, 0, (LPOLESTR)wszSuppressApp0 },
    };

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_count != 0)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    if (!This->initialized)
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

static HRESULT WINAPI JpegEncoder_Commit(IWICBitmapEncoder *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static HRESULT WINAPI JpegEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl JpegEncoder_Vtbl = {
    JpegEncoder_QueryInterface,
    JpegEncoder_AddRef,
    JpegEncoder_Release,
    JpegEncoder_Initialize,
    JpegEncoder_GetContainerFormat,
    JpegEncoder_GetEncoderInfo,
    JpegEncoder_SetColorContexts,
    JpegEncoder_SetPalette,
    JpegEncoder_SetThumbnail,
    JpegEncoder_SetPreview,
    JpegEncoder_CreateNewFrame,
    JpegEncoder_Commit,
    JpegEncoder_GetMetadataQueryWriter
};

HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv)
{
    JpegEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!libjpeg_handle && !load_libjpeg())
    {
        ERR("Failed writing JPEG because unable to find %s\n",SONAME_LIBJPEG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(JpegEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &JpegEncoder_Vtbl;
    This->IWICBitmapFrameEncode_iface.lpVtbl = &JpegEncoder_FrameVtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->frame_count = 0;
    This->frame_initialized = FALSE;
    This->started_compress = FALSE;
    This->lines_written = 0;
    This->frame_committed = FALSE;
    This->committed = FALSE;
    This->width = This->height = 0;
    This->xres = This->yres = 0.0;
    This->format = NULL;
    This->stream = NULL;
    This->colors = 0;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JpegEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !defined(SONAME_LIBJPEG) */

HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save JPEG picture, but JPEG support is not compiled in.\n");
    return E_FAIL;
}

#endif

HRESULT JpegDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct decoder *decoder;
    struct decoder_info decoder_info;

    hr = get_unix_decoder(&CLSID_WICJpegDecoder, &decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}
