/*
 * Tests for dns service functions
 *
 * Copyright 2026 Thomas Portal
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
#include "windns.h"

#include "wine/test.h"

static PDNS_SERVICE_INSTANCE (WINAPI *pDnsServiceConstructInstance)( PCWSTR, PCWSTR, PIP4_ADDRESS,
        PIP6_ADDRESS, WORD, WORD, WORD, DWORD, PCWSTR *, PCWSTR * );
static VOID (WINAPI *pDnsServiceFreeInstance)( PDNS_SERVICE_INSTANCE );

static void test_DnsServiceConstructInstance( void )
{
    static const WCHAR name[] = L"_test._tcp.local";
    static const WCHAR host[] = L"test.local";
    static const WCHAR *keys[] = { L"txtvers", L"path" };
    static const WCHAR *values[] = { L"1", L"/api" };
    IP4_ADDRESS ip4 = 0x0100007f; /* 127.0.0.1 */
    DNS_SERVICE_INSTANCE *instance;

    instance = pDnsServiceConstructInstance( name, host, &ip4, NULL, 8080, 1, 2, 2, keys, values );
    ok( instance != NULL, "DnsServiceConstructInstance failed\n" );
    if (!instance) return;

    ok( !lstrcmpW( instance->pszInstanceName, name ), "got name %s\n",
        wine_dbgstr_w( instance->pszInstanceName ) );
    ok( instance->pszInstanceName != name, "name was not copied\n" );
    ok( !lstrcmpW( instance->pszHostName, host ), "got host %s\n",
        wine_dbgstr_w( instance->pszHostName ) );
    ok( instance->ip4Address != NULL, "expected an ip4Address\n" );
    if (instance->ip4Address)
        ok( *instance->ip4Address == ip4, "got ip4 %08lx\n", *instance->ip4Address );
    ok( instance->ip6Address == NULL, "expected no ip6Address\n" );
    ok( instance->wPort == 8080, "got port %u\n", instance->wPort );
    ok( instance->wPriority == 1, "got priority %u\n", instance->wPriority );
    ok( instance->wWeight == 2, "got weight %u\n", instance->wWeight );
    ok( instance->dwPropertyCount == 2, "got count %lu\n", instance->dwPropertyCount );
    ok( instance->keys != NULL, "expected keys\n" );
    ok( instance->values != NULL, "expected values\n" );
    if (instance->keys)
        ok( !lstrcmpW( instance->keys[0], keys[0] ), "got key %s\n",
            wine_dbgstr_w( instance->keys[0] ) );
    if (instance->values)
        ok( !lstrcmpW( instance->values[1], values[1] ), "got value %s\n",
            wine_dbgstr_w( instance->values[1] ) );

    pDnsServiceFreeInstance( instance );
}

START_TEST(service)
{
    HMODULE dnsapi = GetModuleHandleA( "dnsapi.dll" );

    pDnsServiceConstructInstance = (void *)GetProcAddress( dnsapi, "DnsServiceConstructInstance" );
    pDnsServiceFreeInstance = (void *)GetProcAddress( dnsapi, "DnsServiceFreeInstance" );
    if (!pDnsServiceConstructInstance || !pDnsServiceFreeInstance)
    {
        win_skip( "DnsServiceConstructInstance is not available\n" );
        return;
    }

    test_DnsServiceConstructInstance();
}
