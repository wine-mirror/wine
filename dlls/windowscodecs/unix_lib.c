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
#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#include "wincodecs_common.h"

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

HRESULT CDECL decoder_create(const CLSID *decoder_clsid, struct decoder_info *info, struct decoder **result)
{
    if (IsEqualGUID(decoder_clsid, &CLSID_WICPngDecoder))
        return png_decoder_create(info, result);

    if (IsEqualGUID(decoder_clsid, &CLSID_WICTiffDecoder))
        return tiff_decoder_create(info, result);

    if (IsEqualGUID(decoder_clsid, &CLSID_WICJpegDecoder))
        return jpeg_decoder_create(info, result);

    return E_NOTIMPL;
}

static const struct unix_funcs unix_funcs = {
    decoder_create,
    decoder_initialize,
    decoder_get_frame_info,
    decoder_copy_pixels,
    decoder_get_metadata_blocks,
    decoder_get_color_context,
    decoder_destroy
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;

    win32_funcs = ptr_in;

    *(const struct unix_funcs **)ptr_out = &unix_funcs;
    return STATUS_SUCCESS;
}
