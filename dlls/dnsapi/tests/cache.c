/*
 * Tests for dns cache functions
 *
 * Copyright 2019 Remi Bernon for CodeWeavers
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"

#include "wine/test.h"

static void test_DnsGetCacheDataTable( void )
{
    BOOL ret;
    PDNS_CACHE_ENTRY entry = NULL;

    ret = DnsGetCacheDataTable( NULL );
    ok( !ret, "DnsGetCacheDataTable succeeded\n" );

    ret = DnsGetCacheDataTable( &entry );
    todo_wine
    ok( ret, "DnsGetCacheDataTable failed\n" );
    todo_wine
    ok( entry != NULL, "DnsGetCacheDataTable returned NULL\n" );
}

START_TEST(cache)
{
    test_DnsGetCacheDataTable();
}
