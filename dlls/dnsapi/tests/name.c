/*
 * Tests for name handling functions
 *
 * Copyright 2006 Hans Leidekker
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"

#include "wine/test.h"

static HMODULE dnsapi;

static BOOL        (WINAPI *pDnsNameCompare_A)(LPSTR,LPSTR);

#define GETFUNCPTR(func) p##func = (void *)GetProcAddress( dnsapi, #func ); \
    if (!p##func) return FALSE;

static BOOL init_function_ptrs( void )
{
    GETFUNCPTR( DnsNameCompare_A )
    return TRUE;
}

static void test_DnsNameCompare_A( void )
{
    ok( pDnsNameCompare_A( NULL, NULL ) == TRUE, "failed unexpectedly\n" );

    ok( pDnsNameCompare_A( "", "" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( ".", "" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "", "." ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( ".", ".." ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "..", "." ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "a", "a" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "a", "A" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "A", "a" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b", "A.B" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b", "a.b" ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b.", "a.b." ) == TRUE, "failed unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b..", "a.b.." ) == TRUE, "failed unexpectedly\n" );

    ok( pDnsNameCompare_A( "", NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( NULL, "" ) == FALSE, "succeeded unexpectedly\n" );

    ok( pDnsNameCompare_A( "a", "b" ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a", "B" ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "A", "b" ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b", "B.A" ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b.", "b.a." ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b", "a.a" ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b..", "b.a.." ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b.", "b.a.." ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b..", "b.a." ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b.", "b.a" ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsNameCompare_A( "a.b", "b.a." ) == FALSE, "succeeded unexpectedly\n" );
}

START_TEST(name)
{
    dnsapi = LoadLibraryA( "dnsapi.dll" );
    if (!dnsapi) return;

    if (!init_function_ptrs())
    {
        FreeLibrary( dnsapi );
        return;
    }

    test_DnsNameCompare_A();

    FreeLibrary( dnsapi );
}
