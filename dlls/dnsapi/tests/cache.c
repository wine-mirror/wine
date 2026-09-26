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
    DNS_CACHE_ENTRY *entry;
    DNS_RECORDW *rec;
    DNS_STATUS status;

    /* make sure some entries are available */
    status = DnsQuery_W( L"winehq.org", DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &rec, NULL );
    ok(status == ERROR_SUCCESS, "got %lu\n", status);
    DnsRecordListFree( rec, DnsFreeRecordList );

    ret = DnsGetCacheDataTable( NULL );
    ok( !ret, "DnsGetCacheDataTable succeeded\n" );

    entry = NULL;
    ret = DnsGetCacheDataTable( &entry );
    ok( ret, "DnsGetCacheDataTable failed\n" );
    ok( entry != NULL, "DnsGetCacheDataTable returned NULL\n" );
    DnsFree( entry, DnsFreeFlat );
}

START_TEST(cache)
{
    test_DnsGetCacheDataTable();
}
