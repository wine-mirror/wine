/*
 * Copyright 2017 Austin English
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "tdh.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tdh);

ULONG WINAPI TdhLoadManifest(LPWSTR manifest)
{
    FIXME("(%s): stub\n", debugstr_w(manifest));
    return STATUS_SUCCESS;
}

ULONG WINAPI TdhLoadManifestFromBinary(LPWSTR binary)
{
    FIXME("(%s): stub\n", debugstr_w(binary));
    return STATUS_SUCCESS;
}

ULONG WINAPI TdhEnumerateProviders( PROVIDER_ENUMERATION_INFO *buffer, ULONG *buffer_size )
{
    FIXME( "%p %p semi-stub.\n", buffer, buffer_size );

    if (!buffer_size) return ERROR_INVALID_PARAMETER;
    if (*buffer_size && !buffer) return ERROR_INVALID_PARAMETER;

    if (*buffer_size < offsetof(PROVIDER_ENUMERATION_INFO, TraceProviderInfoArray))
    {
        *buffer_size = offsetof(PROVIDER_ENUMERATION_INFO, TraceProviderInfoArray);
        return ERROR_INSUFFICIENT_BUFFER;
    }
    buffer->NumberOfProviders = 0;
    return ERROR_SUCCESS;
}
