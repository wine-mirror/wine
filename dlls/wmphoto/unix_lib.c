/*
 * unix_lib.c - This is the Unix side of the Unix interface.
 *
 * Copyright 2020 Esme Povirk
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

#include <stdarg.h>

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "initguid.h"
#ifdef SONAME_LIBJXRGLUE
#define ERR JXR_ERR
#include <JXRGlue.h>
#undef ERR
#endif

#include "wincodecs_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#include "wincodecs_common.h"

#ifdef SONAME_LIBJXRGLUE
static void *libjxrglue;
static typeof(PKImageDecode_Create_WMP) *pPKImageDecode_Create_WMP;

static const struct
{
    const WICPixelFormatGUID *format;
    UINT bpp;
} pixel_format_bpp[] =
{
    {&GUID_PKPixelFormat128bppRGBAFixedPoint, 128},
    {&GUID_PKPixelFormat128bppRGBAFloat, 128},
    {&GUID_PKPixelFormat128bppRGBFloat, 128},
    {&GUID_PKPixelFormat16bppRGB555, 16},
    {&GUID_PKPixelFormat16bppRGB565, 16},
    {&GUID_PKPixelFormat16bppGray, 16},
    {&GUID_PKPixelFormat16bppGrayFixedPoint, 16},
    {&GUID_PKPixelFormat16bppGrayHalf, 16},
    {&GUID_PKPixelFormat24bppBGR, 24},
    {&GUID_PKPixelFormat24bppRGB, 24},
    {&GUID_PKPixelFormat32bppBGR, 32},
    {&GUID_PKPixelFormat32bppRGB101010, 32},
    {&GUID_PKPixelFormat32bppBGRA, 32},
    {&GUID_PKPixelFormat32bppCMYK, 32},
    {&GUID_PKPixelFormat32bppGrayFixedPoint, 32},
    {&GUID_PKPixelFormat32bppGrayFloat, 32},
    {&GUID_PKPixelFormat32bppRGBE, 32},
    {&GUID_PKPixelFormat40bppCMYKAlpha, 40},
    {&GUID_PKPixelFormat48bppRGB, 48},
    {&GUID_PKPixelFormat48bppRGBFixedPoint, 48},
    {&GUID_PKPixelFormat48bppRGBHalf, 48},
    {&GUID_PKPixelFormat64bppCMYK, 64},
    {&GUID_PKPixelFormat64bppRGBA, 64},
    {&GUID_PKPixelFormat64bppRGBAFixedPoint, 64},
    {&GUID_PKPixelFormat64bppRGBAHalf, 64},
    {&GUID_PKPixelFormat80bppCMYKAlpha, 80},
    {&GUID_PKPixelFormat8bppGray, 8},
    {&GUID_PKPixelFormat96bppRGBFixedPoint, 96},
    {&GUID_PKPixelFormatBlackWhite, 1},
};

static inline UINT pixel_format_get_bpp(const WICPixelFormatGUID *format)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(pixel_format_bpp); ++i)
        if (IsEqualGUID(format, pixel_format_bpp[i].format)) return pixel_format_bpp[i].bpp;
    return 0;
}

struct wmp_decoder
{
    struct decoder decoder_iface;
    struct WMPStream WMPStream_iface;
    PKImageDecode *decoder;
    IStream *stream;
    struct decoder_frame frame;
    UINT frame_stride;
    BYTE *frame_data;
};

static inline struct wmp_decoder *impl_from_decoder(struct decoder *iface)
{
    return CONTAINING_RECORD(iface, struct wmp_decoder, decoder_iface);
}

static inline struct wmp_decoder *impl_from_WMPStream(struct WMPStream *iface)
{
    return CONTAINING_RECORD(iface, struct wmp_decoder, WMPStream_iface);
}

static JXR_ERR wmp_stream_Close(struct WMPStream **piface)
{
    TRACE("iface %p\n", piface);
    return WMP_errSuccess;
}

static Bool wmp_stream_EOS(struct WMPStream *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return FALSE;
}

static JXR_ERR wmp_stream_Read(struct WMPStream *iface, void *buf, size_t len)
{
    struct wmp_decoder *This = impl_from_WMPStream(iface);
    ULONG count;
    if (FAILED(stream_read(This->stream, buf, len, &count)) || count != len)
    {
        WARN("Failed to read data!\n");
        return WMP_errFileIO;
    }
    return WMP_errSuccess;
}

static JXR_ERR wmp_stream_Write(struct WMPStream *iface, const void *buf, size_t len)
{
    struct wmp_decoder *This = impl_from_WMPStream(iface);
    ULONG count;
    if (FAILED(stream_write(This->stream, buf, len, &count)) || count != len)
    {
        WARN("Failed to write data!\n");
        return WMP_errFileIO;
    }
    return WMP_errSuccess;
}

static JXR_ERR wmp_stream_SetPos(struct WMPStream *iface, size_t pos)
{
    struct wmp_decoder *This = impl_from_WMPStream(iface);
    if (FAILED(stream_seek(This->stream, pos, STREAM_SEEK_SET, NULL)))
    {
        WARN("Failed to set stream pos!\n");
        return WMP_errFileIO;
    }
    return WMP_errSuccess;
}

static JXR_ERR wmp_stream_GetPos(struct WMPStream *iface, size_t *pos)
{
    struct wmp_decoder *This = impl_from_WMPStream(iface);
    ULONGLONG ofs;
    if (FAILED(stream_seek(This->stream, 0, STREAM_SEEK_CUR, &ofs)))
    {
        WARN("Failed to get stream pos!\n");
        return WMP_errFileIO;
    }
    *pos = ofs;
    return WMP_errSuccess;
}

static HRESULT CDECL wmp_decoder_initialize(struct decoder *iface, IStream *stream, struct decoder_stat *st)
{
    struct wmp_decoder *This = impl_from_decoder(iface);
    HRESULT hr;
    Float dpix, dpiy;
    I32 width, height;
    U32 count;

    TRACE("iface %p, stream %p, st %p\n", iface, stream, st);

    if (This->stream)
        return WINCODEC_ERR_WRONGSTATE;

    This->stream = stream;
    if (FAILED(hr = stream_seek(This->stream, 0, STREAM_SEEK_SET, NULL)))
        return hr;
    if (This->decoder->Initialize(This->decoder, &This->WMPStream_iface))
    {
        ERR("Failed to initialize jxrlib decoder!\n");
        return E_FAIL;
    }

    if (This->decoder->GetFrameCount(This->decoder, &st->frame_count))
    {
        ERR("Failed to get frame count!\n");
        return E_FAIL;
    }

    if (st->frame_count > 1) FIXME("multi frame JPEG-XR not implemented\n");
    st->frame_count = 1;
    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata;

    if (This->decoder->SelectFrame(This->decoder, 0))
    {
        ERR("Failed to select frame 0!\n");
        return E_FAIL;
    }
    if (This->decoder->GetPixelFormat(This->decoder, &This->frame.pixel_format))
    {
        ERR("Failed to get frame pixel format!\n");
        return E_FAIL;
    }
    if (This->decoder->GetSize(This->decoder, &width, &height))
    {
        ERR("Failed to get frame size!\n");
        return E_FAIL;
    }
    if (This->decoder->GetResolution(This->decoder, &dpix, &dpiy))
    {
        ERR("Failed to get frame resolution!\n");
        return E_FAIL;
    }
    if (This->decoder->GetColorContext(This->decoder, NULL, &count))
    {
        ERR("Failed to get frame color context size!\n");
        return E_FAIL;
    }

    if (!(This->frame.bpp = pixel_format_get_bpp(&This->frame.pixel_format))) return E_FAIL;
    This->frame.width = width;
    This->frame.height = height;
    This->frame.dpix = dpix;
    This->frame.dpiy = dpiy;
    This->frame.num_colors = 0;
    if (count) This->frame.num_color_contexts = 1;
    else This->frame.num_color_contexts = 0;

    return S_OK;
}

static HRESULT CDECL wmp_decoder_get_frame_info(struct decoder *iface, UINT frame, struct decoder_frame *info)
{
    struct wmp_decoder *This = impl_from_decoder(iface);

    TRACE("iface %p, frame %d, info %p\n", iface, frame, info);

    if (frame > 0)
    {
        FIXME("multi frame JPEG-XR not implemented\n");
        return E_NOTIMPL;
    }

    *info = This->frame;
    return S_OK;
}

static HRESULT CDECL wmp_decoder_copy_pixels(struct decoder *iface, UINT frame, const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct wmp_decoder *This = impl_from_decoder(iface);
    PKRect pkrect;
    U8 *frame_data;

    TRACE("iface %p, frame %d, rect %p, stride %d, buffersize %d, buffer %p\n", iface, frame, prc, stride, buffersize, buffer);

    if (frame > 0)
    {
        FIXME("multi frame JPEG-XR not implemented\n");
        return E_NOTIMPL;
    }

    if (!This->frame_data)
    {
        pkrect.X = 0;
        pkrect.Y = 0;
        pkrect.Width = This->frame.width;
        pkrect.Height = This->frame.height;
        This->frame_stride = (This->frame.width * This->frame.bpp + 7) / 8;
        if (!(frame_data = RtlAllocateHeap(GetProcessHeap(), 0, This->frame.height * This->frame_stride)))
            return E_FAIL;
        if (This->decoder->Copy(This->decoder, &pkrect, frame_data, stride))
        {
            ERR("Failed to copy frame data!\n");
            RtlFreeHeap(GetProcessHeap(), 0, frame_data);
            return E_FAIL;
        }

        This->frame_data = frame_data;
    }

    return copy_pixels(This->frame.bpp, This->frame_data,
        This->frame.width, This->frame.height, This->frame_stride,
        prc, stride, buffersize, buffer);
}

static HRESULT CDECL wmp_decoder_get_metadata_blocks(struct decoder* iface, UINT frame, UINT *count, struct decoder_block **blocks)
{
    TRACE("iface %p, frame %d, count %p, blocks %p\n", iface, frame, count, blocks);

    *count = 0;
    *blocks = NULL;
    return S_OK;
}

static HRESULT CDECL wmp_decoder_get_color_context(struct decoder* iface, UINT frame, UINT num, BYTE **data, DWORD *datasize)
{
    struct wmp_decoder *This = impl_from_decoder(iface);
    U32 count;
    U8 *bytes;

    TRACE("iface %p, frame %d, num %u, data %p, datasize %p\n", iface, frame, num, data, datasize);

    *datasize = 0;
    *data = NULL;

    if (This->decoder->GetColorContext(This->decoder, NULL, &count))
    {
        ERR("Failed to get frame color context size!\n");
        return E_FAIL;
    }
    *datasize = count;

    bytes = RtlAllocateHeap(GetProcessHeap(), 0, count);
    if (!bytes)
        return E_OUTOFMEMORY;

    if (This->decoder->GetColorContext(This->decoder, bytes, &count))
    {
        ERR("Failed to get frame color context!\n");
        RtlFreeHeap(GetProcessHeap(), 0, bytes);
        return E_FAIL;
    }

    *data = bytes;
    return S_OK;
}

static void CDECL wmp_decoder_destroy(struct decoder* iface)
{
    struct wmp_decoder *This = impl_from_decoder(iface);

    TRACE("iface %p\n", iface);

    This->decoder->Release(&This->decoder);
    RtlFreeHeap(GetProcessHeap(), 0, This->frame_data);
    RtlFreeHeap(GetProcessHeap(), 0, This);
}

static const struct decoder_funcs wmp_decoder_vtable = {
    wmp_decoder_initialize,
    wmp_decoder_get_frame_info,
    wmp_decoder_copy_pixels,
    wmp_decoder_get_metadata_blocks,
    wmp_decoder_get_color_context,
    wmp_decoder_destroy
};

HRESULT CDECL wmp_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct wmp_decoder *This;
    PKImageDecode *decoder;

    if (!pPKImageDecode_Create_WMP || pPKImageDecode_Create_WMP(&decoder)) return E_FAIL;
    This = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->decoder_iface.vtable = &wmp_decoder_vtable;
    This->WMPStream_iface.Close = &wmp_stream_Close;
    This->WMPStream_iface.EOS = &wmp_stream_EOS;
    This->WMPStream_iface.Read = &wmp_stream_Read;
    This->WMPStream_iface.Write = &wmp_stream_Write;
    This->WMPStream_iface.SetPos = &wmp_stream_SetPos;
    This->WMPStream_iface.GetPos = &wmp_stream_GetPos;

    This->decoder = decoder;
    This->stream = NULL;
    memset(&This->frame, 0, sizeof(This->frame));
    This->frame_stride = 0;
    This->frame_data = NULL;

    *result = &This->decoder_iface;

    info->container_format = GUID_ContainerFormatWmp;
    info->block_format = GUID_ContainerFormatWmp;
    info->clsid = CLSID_WICWmpDecoder;

    return S_OK;
}
#endif

static const struct win32_funcs *win32_funcs;

HRESULT CDECL stream_getsize(IStream *stream, ULONGLONG *size)
{
    return win32_funcs->stream_getsize(stream, size);
}

HRESULT CDECL stream_read(IStream *stream, void *buffer, ULONG read, ULONG *bytes_read)
{
    return win32_funcs->stream_read(stream, buffer, read, bytes_read);
}

HRESULT CDECL stream_seek(IStream *stream, LONGLONG ofs, DWORD origin, ULONGLONG *new_position)
{
    return win32_funcs->stream_seek(stream, ofs, origin, new_position);
}

HRESULT CDECL stream_write(IStream *stream, const void *buffer, ULONG write, ULONG *bytes_written)
{
    return win32_funcs->stream_write(stream, buffer, write, bytes_written);
}

HRESULT CDECL decoder_create(const CLSID *decoder_clsid, struct decoder_info *info, struct decoder **result)
{
    if (IsEqualGUID(decoder_clsid, &CLSID_WICWmpDecoder))
#ifdef SONAME_LIBJXRGLUE
        return wmp_decoder_create(info, result);
#else
    {
        WARN("jxrlib support not compiled in, returning E_NOINTERFACE.\n");
        return E_NOINTERFACE;
    }
#endif

    FIXME("encoder_clsid %s, info %p, result %p, stub!\n", debugstr_guid(decoder_clsid), info, result);
    return E_NOTIMPL;
}

HRESULT CDECL encoder_create(const CLSID *encoder_clsid, struct encoder_info *info, struct encoder **result)
{
    FIXME("encoder_clsid %s, info %p, result %p, stub!\n", debugstr_guid(encoder_clsid), info, result);
    return E_NOTIMPL;
}

static const struct unix_funcs unix_funcs = {
    decoder_create,
    decoder_initialize,
    decoder_get_frame_info,
    decoder_copy_pixels,
    decoder_get_metadata_blocks,
    decoder_get_color_context,
    decoder_destroy,
    encoder_create,
    encoder_initialize,
    encoder_get_supported_format,
    encoder_create_frame,
    encoder_write_lines,
    encoder_commit_frame,
    encoder_commit_file,
    encoder_destroy
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;

    win32_funcs = ptr_in;

#ifdef SONAME_LIBJXRGLUE
    if (!(libjxrglue = dlopen(SONAME_LIBJXRGLUE, RTLD_NOW)))
        ERR("failed to load %s\n", SONAME_LIBJXRGLUE);
    else if (!(pPKImageDecode_Create_WMP = dlsym(libjxrglue, "PKImageDecode_Create_WMP")))
        ERR("unable to find PKImageDecode_Create_WMP in %s!\n", SONAME_LIBJXRGLUE);
#else
    ERR("jxrlib support not compiled in!\n");
#endif

    *(const struct unix_funcs **)ptr_out = &unix_funcs;
    return STATUS_SUCCESS;
}
