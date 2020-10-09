/*
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

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBPNG

struct png_decoder
{
    struct decoder decoder;
};

static inline struct png_decoder *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct png_decoder, decoder);
}

void CDECL png_decoder_destroy(struct decoder* iface)
{
    struct png_decoder *This = impl_from_decoder(iface);

    RtlFreeHeap(GetProcessHeap(), 0, This);
}

static const struct decoder_funcs png_decoder_vtable = {
    png_decoder_destroy
};

HRESULT CDECL png_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct png_decoder *This;

    TRACE("\n");

    This = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(*This));

    if (!This)
    {
        return E_OUTOFMEMORY;
    }

    This->decoder.vtable = &png_decoder_vtable;
    *result = &This->decoder;

    info->container_format = GUID_ContainerFormatPng;
    info->block_format = GUID_ContainerFormatPng;
    info->clsid = CLSID_WICPngDecoder;

    return S_OK;
}

#else

HRESULT CDECL png_decoder_create(struct decoder_info *info, struct decoder **result)
{
    ERR("Trying to load PNG picture, but PNG support is not compiled in.\n");
    return E_FAIL;
}

#endif
