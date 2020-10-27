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

#include "config.h"
#include "wine/port.h"

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
    stream_seek
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
