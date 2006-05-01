/*
 * Tests for record handling functions
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

static BOOL        (WINAPI *pDnsRecordCompare)(PDNS_RECORD,PDNS_RECORD);

#define GETFUNCPTR(func) p##func = (void *)GetProcAddress( dnsapi, #func ); \
    if (!p##func) return FALSE;

static BOOL init_function_ptrs( void )
{
    GETFUNCPTR( DnsRecordCompare )
    return TRUE;
}

static void test_DnsRecordCompare( void )
{
    char name1[] = "localhost";
    char name2[] = "LOCALHOST";
    static DNS_RECORDA r1, r2;
           
    r1.pName = name1;
    r1.wType = DNS_TYPE_A;
    r1.wDataLength = sizeof(DNS_A_DATA);
    r1.Data.A.IpAddress = 0xffffffff;

    r2.pName = name1;
    r2.wType = DNS_TYPE_A;
    r2.wDataLength = sizeof(DNS_A_DATA);
    r2.Data.A.IpAddress = 0xffffffff;

    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r1 ) == TRUE, "failed unexpectedly\n" );

    r2.pName = name2;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetUnicode;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == FALSE, "succeeded unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetAnsi;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == FALSE, "succeeded unexpectedly\n" );

    r1.Flags.S.CharSet = DnsCharSetAnsi;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Data.A.IpAddress = 0;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == FALSE, "succeeded unexpectedly\n" );
}

START_TEST(record)
{
    dnsapi = LoadLibraryA( "dnsapi.dll" );
    if (!dnsapi) return;

    if (!init_function_ptrs())
    {
        FreeLibrary( dnsapi );
        return;
    }

    test_DnsRecordCompare();

    FreeLibrary( dnsapi );
}
