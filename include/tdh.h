/*
 * Copyright (C) 2025 Paul Gofman for CodeWeavers
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

#ifndef __WINE_TDH_H
#define __WINE_TDH_H

ULONG WINAPI TdhLoadManifest( WCHAR *manifest );
ULONG WINAPI TdhLoadManifestFromBinary( WCHAR *binary );

typedef struct _TRACE_PROVIDER_INFO
{
    GUID ProviderGuid;
    ULONG SchemaSource;
    ULONG ProviderNameOffset;
}
TRACE_PROVIDER_INFO;

typedef struct _PROVIDER_ENUMERATION_INFO
{
    ULONG NumberOfProviders;
    ULONG Reserved;
    TRACE_PROVIDER_INFO TraceProviderInfoArray[ANYSIZE_ARRAY];
}
PROVIDER_ENUMERATION_INFO;

ULONG WINAPI TdhEnumerateProviders( PROVIDER_ENUMERATION_INFO *buffer, ULONG *buffer_size );
#endif
