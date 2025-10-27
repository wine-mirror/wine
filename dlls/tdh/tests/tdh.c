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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "tdh.h"

#include "wine/test.h"

static void test_providers(void)
{
    PROVIDER_ENUMERATION_INFO *info;
    ULONG size, size2;
    DWORD err;

    err = TdhEnumerateProviders( NULL, NULL );
    ok( err == ERROR_INVALID_PARAMETER, "got %lu.\n", err );

    size = 1;
    err = TdhEnumerateProviders( NULL, &size );
    ok( err == ERROR_INVALID_PARAMETER, "got %lu.\n", err );
    ok( size == 1, "got %lu.\n", size );

    size = 0;
    err = TdhEnumerateProviders( NULL, &size );
    ok( err == ERROR_INSUFFICIENT_BUFFER, "got %lu.\n", err );
    ok( size >= offsetof( PROVIDER_ENUMERATION_INFO, TraceProviderInfoArray ), "got %lu.\n", size );
    info = malloc( size );
    size2 = 1;
    err = TdhEnumerateProviders( info, &size2 );
    ok( err == ERROR_INSUFFICIENT_BUFFER, "got %lu.\n", err );
    ok( size2 == size, "got %lu, %lu.\n", size2, size );

    size2 = size;
    err = TdhEnumerateProviders( info, &size2 );
    ok( !err, "got %lu.\n", err );
    ok( size2 == size, "got %lu, %lu.\n", size2, size );
    size2 = offsetof( PROVIDER_ENUMERATION_INFO, TraceProviderInfoArray[info->NumberOfProviders] );
    ok( size >= size2, "got %lu, expected %lu.\n", size, size2 );
}

START_TEST(tdh)
{
    test_providers();
}
