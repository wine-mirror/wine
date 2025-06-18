/*
 * Copyright 2022 Hans Leidekker for CodeWeavers
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
#include "windef.h"
#include "wofapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wofutil);

HRESULT WINAPI WofIsExternalFile( const WCHAR *path, BOOL *result, ULONG *provider, void *ptr, ULONG *length )
{
    FIXME( "%s, %p, %p, %p, %p\n", debugstr_w(path), result, provider, ptr, length );
    if (result) *result = FALSE;
    if (provider) *provider = 0;
    if (length) *length = 0;
    return S_OK;
}

BOOL WINAPI WofShouldCompressBinaries( const WCHAR *volume, ULONG *alg )
{
    FIXME( "%s, %p\n", debugstr_w(volume), alg );
    return FALSE;
}
