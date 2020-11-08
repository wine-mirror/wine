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

#if 0
#pragma makedep unix
#endif

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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBJPEG
WINE_DECLARE_DEBUG_CHANNEL(jpeg);

static CRITICAL_SECTION init_jpeg_cs;
static CRITICAL_SECTION_DEBUG init_jpeg_cs_debug =
{
    0, 0, &init_jpeg_cs,
    { &init_jpeg_cs_debug.ProcessLocksList,
      &init_jpeg_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_jpeg_cs") }
};
static CRITICAL_SECTION init_jpeg_cs = { &init_jpeg_cs_debug, -1, 0, 0, 0, 0 };

static void *libjpeg_handle;

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
    void *result;

    RtlEnterCriticalSection(&init_jpeg_cs);

    if((libjpeg_handle = dlopen(SONAME_LIBJPEG, RTLD_NOW)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(libjpeg_handle, #f)) == NULL) { \
        ERR("failed to load symbol %s\n", #f); \
        libjpeg_handle = NULL; \
        RtlLeaveCriticalSection(&init_jpeg_cs); \
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
    result = libjpeg_handle;

    RtlLeaveCriticalSection(&init_jpeg_cs);

    return result;
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

struct jpeg_decoder {
    struct decoder decoder;
    struct decoder_frame frame;
    BOOL cinfo_initialized;
    IStream *stream;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr source_mgr;
    BYTE source_buffer[1024];
    UINT stride;
    BYTE *image_data;
};

static inline struct jpeg_decoder *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct jpeg_decoder, decoder);
}

static inline struct jpeg_decoder *decoder_from_decompress(j_decompress_ptr decompress)
{
    return CONTAINING_RECORD(decompress, struct jpeg_decoder, cinfo);
}

static void CDECL jpeg_decoder_destroy(struct decoder* iface)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);

    if (This->cinfo_initialized) pjpeg_destroy_decompress(&This->cinfo);
    free(This->image_data);
    RtlFreeHeap(GetProcessHeap(), 0, This);
}

static void source_mgr_init_source(j_decompress_ptr cinfo)
{
}

static jpeg_boolean source_mgr_fill_input_buffer(j_decompress_ptr cinfo)
{
    struct jpeg_decoder *This = decoder_from_decompress(cinfo);
    HRESULT hr;
    ULONG bytesread;

    hr = stream_read(This->stream, This->source_buffer, 1024, &bytesread);

    if (FAILED(hr) || bytesread == 0)
    {
        return FALSE;
    }
    else
    {
        This->source_mgr.next_input_byte = This->source_buffer;
        This->source_mgr.bytes_in_buffer = bytesread;
        return TRUE;
    }
}

static void source_mgr_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_decoder *This = decoder_from_decompress(cinfo);

    if (num_bytes > This->source_mgr.bytes_in_buffer)
    {
        stream_seek(This->stream, num_bytes - This->source_mgr.bytes_in_buffer, STREAM_SEEK_CUR, NULL);
        This->source_mgr.bytes_in_buffer = 0;
    }
    else if (num_bytes > 0)
    {
        This->source_mgr.next_input_byte += num_bytes;
        This->source_mgr.bytes_in_buffer -= num_bytes;
    }
}

static void source_mgr_term_source(j_decompress_ptr cinfo)
{
}

static HRESULT CDECL jpeg_decoder_initialize(struct decoder* iface, IStream *stream, struct decoder_stat *st)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);
    int ret;
    jmp_buf jmpbuf;
    UINT data_size, i;

    if (This->cinfo_initialized)
        return WINCODEC_ERR_WRONGSTATE;

    pjpeg_std_error(&This->jerr);

    This->jerr.error_exit = error_exit_fn;
    This->jerr.emit_message = emit_message_fn;

    This->cinfo.err = &This->jerr;

    This->cinfo.client_data = jmpbuf;

    if (setjmp(jmpbuf))
        return E_FAIL;

    pjpeg_CreateDecompress(&This->cinfo, JPEG_LIB_VERSION, sizeof(struct jpeg_decompress_struct));

    This->cinfo_initialized = TRUE;

    This->stream = stream;

    stream_seek(This->stream, 0, STREAM_SEEK_SET, NULL);

    This->source_mgr.bytes_in_buffer = 0;
    This->source_mgr.init_source = source_mgr_init_source;
    This->source_mgr.fill_input_buffer = source_mgr_fill_input_buffer;
    This->source_mgr.skip_input_data = source_mgr_skip_input_data;
    This->source_mgr.resync_to_restart = pjpeg_resync_to_restart;
    This->source_mgr.term_source = source_mgr_term_source;

    This->cinfo.src = &This->source_mgr;

    ret = pjpeg_read_header(&This->cinfo, TRUE);

    if (ret != JPEG_HEADER_OK) {
        WARN("Jpeg image in stream has bad format, read header returned %d.\n",ret);
        return E_FAIL;
    }

    switch (This->cinfo.jpeg_color_space)
    {
    case JCS_GRAYSCALE:
        This->cinfo.out_color_space = JCS_GRAYSCALE;
        This->frame.bpp = 8;
        This->frame.pixel_format = GUID_WICPixelFormat8bppGray;
        break;
    case JCS_RGB:
    case JCS_YCbCr:
        This->cinfo.out_color_space = JCS_RGB;
        This->frame.bpp = 24;
        This->frame.pixel_format = GUID_WICPixelFormat24bppBGR;
        break;
    case JCS_CMYK:
    case JCS_YCCK:
        This->cinfo.out_color_space = JCS_CMYK;
        This->frame.bpp = 32;
        This->frame.pixel_format = GUID_WICPixelFormat32bppCMYK;
        break;
    default:
        ERR("Unknown JPEG color space %i\n", This->cinfo.jpeg_color_space);
        return E_FAIL;
    }

    if (!pjpeg_start_decompress(&This->cinfo))
    {
        ERR("jpeg_start_decompress failed\n");
        return E_FAIL;
    }

    This->frame.width = This->cinfo.output_width;
    This->frame.height = This->cinfo.output_height;

    switch (This->cinfo.density_unit)
    {
    case 2: /* pixels per centimeter */
        This->frame.dpix = This->cinfo.X_density * 2.54;
        This->frame.dpiy = This->cinfo.Y_density * 2.54;
        break;

    case 1: /* pixels per inch */
        This->frame.dpix = This->cinfo.X_density;
        This->frame.dpiy = This->cinfo.Y_density;
        break;

    case 0: /* unknown */
    default:
        This->frame.dpix = This->frame.dpiy = 96.0;
        break;
    }

    This->frame.num_color_contexts = 0;
    This->frame.num_colors = 0;

    This->stride = (This->frame.bpp * This->cinfo.output_width + 7) / 8;
    data_size = This->stride * This->cinfo.output_height;

    This->image_data = malloc(data_size);
    if (!This->image_data)
        return E_OUTOFMEMORY;

    while (This->cinfo.output_scanline < This->cinfo.output_height)
    {
        UINT first_scanline = This->cinfo.output_scanline;
        UINT max_rows;
        JSAMPROW out_rows[4];
        JDIMENSION ret;

        max_rows = min(This->cinfo.output_height-first_scanline, 4);
        for (i=0; i<max_rows; i++)
            out_rows[i] = This->image_data + This->stride * (first_scanline+i);

        ret = pjpeg_read_scanlines(&This->cinfo, out_rows, max_rows);
        if (ret == 0)
        {
            ERR("read_scanlines failed\n");
            return E_FAIL;
        }
    }

    if (This->frame.bpp == 24)
    {
        /* libjpeg gives us RGB data and we want BGR, so byteswap the data */
        reverse_bgr8(3, This->image_data,
            This->cinfo.output_width, This->cinfo.output_height,
            This->stride);
    }

    if (This->cinfo.out_color_space == JCS_CMYK && This->cinfo.saw_Adobe_marker)
    {
        /* Adobe JPEG's have inverted CMYK data. */
        for (i=0; i<data_size; i++)
            This->image_data[i] ^= 0xff;
    }

    st->frame_count = 1;
    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata |
                DECODER_FLAGS_UNSUPPORTED_COLOR_CONTEXT;
    return S_OK;
}

static HRESULT CDECL jpeg_decoder_get_frame_info(struct decoder* iface, UINT frame, struct decoder_frame *info)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);
    *info = This->frame;
    return S_OK;
}

static HRESULT CDECL jpeg_decoder_copy_pixels(struct decoder* iface, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);
    return copy_pixels(This->frame.bpp, This->image_data,
        This->frame.width, This->frame.height, This->stride,
        prc, stride, buffersize, buffer);
}

static HRESULT CDECL jpeg_decoder_get_metadata_blocks(struct decoder* iface, UINT frame,
    UINT *count, struct decoder_block **blocks)
{
    FIXME("stub\n");
    *count = 0;
    *blocks = NULL;
    return S_OK;
}

static HRESULT CDECL jpeg_decoder_get_color_context(struct decoder* This, UINT frame, UINT num,
    BYTE **data, DWORD *datasize)
{
    /* This should never be called because we report 0 color contexts and the unsupported flag. */
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const struct decoder_funcs jpeg_decoder_vtable = {
    jpeg_decoder_initialize,
    jpeg_decoder_get_frame_info,
    jpeg_decoder_copy_pixels,
    jpeg_decoder_get_metadata_blocks,
    jpeg_decoder_get_color_context,
    jpeg_decoder_destroy
};

HRESULT CDECL jpeg_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct jpeg_decoder *This;

    if (!load_libjpeg())
    {
        ERR("Failed reading JPEG because unable to find %s\n", SONAME_LIBJPEG);
        return E_FAIL;
    }

    This = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(struct jpeg_decoder));
    if (!This) return E_OUTOFMEMORY;

    This->decoder.vtable = &jpeg_decoder_vtable;
    This->cinfo_initialized = FALSE;
    This->stream = NULL;
    This->image_data = NULL;
    *result = &This->decoder;

    info->container_format = GUID_ContainerFormatJpeg;
    info->block_format = GUID_ContainerFormatJpeg;
    info->clsid = CLSID_WICJpegDecoder;

    return S_OK;
}

#else /* !defined(SONAME_LIBJPEG) */

HRESULT CDECL jpeg_decoder_create(struct decoder_info *info, struct decoder **result)
{
    ERR("Trying to load JPEG picture, but JPEG support is not compiled in.\n");
    return E_FAIL;
}

#endif
