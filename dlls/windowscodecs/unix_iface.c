/*
 * unix_iface.c - This is the Win32 side of the Unix interface.
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

#include <stdarg.h>

#define NONAMELESSUNION
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "winternl.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

static const struct unix_funcs *unix_funcs;

static const struct win32_funcs win32_funcs = {
    stream_getsize,
    stream_read,
    stream_seek,
    stream_write
};

static BOOL WINAPI load_unixlib( INIT_ONCE *once, void *param, void **context )
{
    __wine_init_unix_lib( windowscodecs_module, DLL_PROCESS_ATTACH, &win32_funcs, &unix_funcs );
    return TRUE;
}

static void init_unixlib(void)
{
    InitOnceExecuteOnce( &init_once, load_unixlib, NULL, NULL );
}

struct decoder_wrapper
{
    struct decoder win32_decoder;
    struct decoder *unix_decoder;
};

static inline struct decoder_wrapper *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct decoder_wrapper, win32_decoder);
}

HRESULT CDECL decoder_wrapper_initialize(struct decoder* iface, IStream* stream, struct decoder_stat *st)
{
    struct decoder_wrapper* This = impl_from_decoder(iface);
    return unix_funcs->decoder_initialize(This->unix_decoder, stream, st);
}

HRESULT CDECL decoder_wrapper_get_frame_info(struct decoder* iface, UINT frame, struct decoder_frame *info)
{
    struct decoder_wrapper* This = impl_from_decoder(iface);
    return unix_funcs->decoder_get_frame_info(This->unix_decoder, frame, info);
}

HRESULT CDECL decoder_wrapper_copy_pixels(struct decoder* iface, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct decoder_wrapper* This = impl_from_decoder(iface);
    return unix_funcs->decoder_copy_pixels(This->unix_decoder, frame, prc, stride, buffersize, buffer);
}

HRESULT CDECL decoder_wrapper_get_metadata_blocks(struct decoder* iface,
    UINT frame, UINT *count, struct decoder_block **blocks)
{
    struct decoder_wrapper* This = impl_from_decoder(iface);
    return unix_funcs->decoder_get_metadata_blocks(This->unix_decoder, frame, count, blocks);
}

HRESULT CDECL decoder_wrapper_get_color_context(struct decoder* iface,
    UINT frame, UINT num, BYTE **data, DWORD *datasize)
{
    struct decoder_wrapper* This = impl_from_decoder(iface);
    return unix_funcs->decoder_get_color_context(This->unix_decoder, frame, num, data, datasize);
}

void CDECL decoder_wrapper_destroy(struct decoder* iface)
{
    struct decoder_wrapper* This = impl_from_decoder(iface);
    unix_funcs->decoder_destroy(This->unix_decoder);
    HeapFree(GetProcessHeap(), 0, This);
}

static const struct decoder_funcs decoder_wrapper_vtable = {
    decoder_wrapper_initialize,
    decoder_wrapper_get_frame_info,
    decoder_wrapper_copy_pixels,
    decoder_wrapper_get_metadata_blocks,
    decoder_wrapper_get_color_context,
    decoder_wrapper_destroy
};

HRESULT get_unix_decoder(const CLSID *decoder_clsid, struct decoder_info *info, struct decoder **result)
{
    HRESULT hr;
    struct decoder_wrapper *wrapper;
    struct decoder *unix_decoder;

    init_unixlib();

    hr = unix_funcs->decoder_create(decoder_clsid, info, &unix_decoder);

    if (SUCCEEDED(hr))
    {
        wrapper = HeapAlloc(GetProcessHeap(), 0, sizeof(*wrapper));

        if (!wrapper)
        {
            unix_funcs->decoder_destroy(unix_decoder);
            return E_OUTOFMEMORY;
        }

        wrapper->win32_decoder.vtable = &decoder_wrapper_vtable;
        wrapper->unix_decoder = unix_decoder;
        *result = &wrapper->win32_decoder;
    }

    return hr;
}

struct encoder_wrapper
{
    struct encoder win32_encoder;
    struct encoder *unix_encoder;
};

static inline struct encoder_wrapper *impl_from_encoder(struct encoder* iface)
{
    return CONTAINING_RECORD(iface, struct encoder_wrapper, win32_encoder);
}

HRESULT CDECL encoder_wrapper_initialize(struct encoder* iface, IStream* stream)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    return unix_funcs->encoder_initialize(This->unix_encoder, stream);
}

HRESULT CDECL encoder_wrapper_get_supported_format(struct encoder* iface, GUID *pixel_format, DWORD *bpp, BOOL *indexed)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    return unix_funcs->encoder_get_supported_format(This->unix_encoder, pixel_format, bpp, indexed);
}

HRESULT CDECL encoder_wrapper_create_frame(struct encoder* iface, const struct encoder_frame *frame)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    return unix_funcs->encoder_create_frame(This->unix_encoder, frame);
}

HRESULT CDECL encoder_wrapper_write_lines(struct encoder* iface, BYTE *data, DWORD line_count, DWORD stride)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    return unix_funcs->encoder_write_lines(This->unix_encoder, data, line_count, stride);
}

HRESULT CDECL encoder_wrapper_commit_frame(struct encoder* iface)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    return unix_funcs->encoder_commit_frame(This->unix_encoder);
}

HRESULT CDECL encoder_wrapper_commit_file(struct encoder* iface)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    return unix_funcs->encoder_commit_file(This->unix_encoder);
}

void CDECL encoder_wrapper_destroy(struct encoder* iface)
{
    struct encoder_wrapper* This = impl_from_encoder(iface);
    unix_funcs->encoder_destroy(This->unix_encoder);
    HeapFree(GetProcessHeap(), 0, This);
}

static const struct encoder_funcs encoder_wrapper_vtable = {
    encoder_wrapper_initialize,
    encoder_wrapper_get_supported_format,
    encoder_wrapper_create_frame,
    encoder_wrapper_write_lines,
    encoder_wrapper_commit_frame,
    encoder_wrapper_commit_file,
    encoder_wrapper_destroy
};

HRESULT get_unix_encoder(const CLSID *encoder_clsid, struct encoder_info *info, struct encoder **result)
{
    HRESULT hr;
    struct encoder_wrapper *wrapper;
    struct encoder *unix_encoder;

    init_unixlib();

    hr = unix_funcs->encoder_create(encoder_clsid, info, &unix_encoder);

    if (SUCCEEDED(hr))
    {
        wrapper = HeapAlloc(GetProcessHeap(), 0, sizeof(*wrapper));

        if (!wrapper)
        {
            unix_funcs->encoder_destroy(unix_encoder);
            return E_OUTOFMEMORY;
        }

        wrapper->win32_encoder.vtable = &encoder_wrapper_vtable;
        wrapper->unix_encoder = unix_encoder;
        *result = &wrapper->win32_encoder;
    }

    return hr;
}
