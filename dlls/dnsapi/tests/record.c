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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"

#include "wine/test.h"

static HMODULE dnsapi;

static BOOL        (WINAPI *pDnsRecordCompare)(PDNS_RECORD,PDNS_RECORD);
static BOOL        (WINAPI *pDnsRecordSetCompare)(PDNS_RECORD,PDNS_RECORD,PDNS_RECORD*,PDNS_RECORD*);

#define GETFUNCPTR(func) p##func = (void *)GetProcAddress( dnsapi, #func ); \
    if (!p##func) return FALSE;

static BOOL init_function_ptrs( void )
{
    GETFUNCPTR( DnsRecordCompare )
    GETFUNCPTR( DnsRecordSetCompare )
    return TRUE;
}

static char name1[] = "localhost";
static char name2[] = "LOCALHOST";

static DNS_RECORDA r1 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };
static DNS_RECORDA r2 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };

static void test_DnsRecordCompare( void )
{
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r1 ) == TRUE, "failed unexpectedly\n" );

    r2.pName = name2;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetUnicode;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == FALSE, "succeeded unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetAnsi;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == FALSE, "succeeded unexpectedly\n" );

    r1.Flags.S.CharSet = DnsCharSetAnsi;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == TRUE, "failed unexpectedly\n" );

    r1.dwTtl = 0;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Data.A.IpAddress = 0;
    ok( pDnsRecordCompare( (PDNS_RECORD)&r1, (PDNS_RECORD)&r2 ) == FALSE, "succeeded unexpectedly\n" );
}

static void test_DnsRecordSetCompare( void )
{
    DNS_RECORD *diff1;
    DNS_RECORD *diff2;
    DNS_RRSET rr1, rr2;

    r1.Flags.DW = 0x2019;
    r2.Flags.DW = 0x2019;
    r2.Data.A.IpAddress = 0xffffffff;

    DNS_RRSET_INIT( rr1 );
    DNS_RRSET_INIT( rr2 );

    DNS_RRSET_ADD( rr1, &r1 );
    DNS_RRSET_ADD( rr2, &r2 );

    DNS_RRSET_TERMINATE( rr1 );
    DNS_RRSET_TERMINATE( rr2 );

    ok( pDnsRecordSetCompare( NULL, NULL, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsRecordSetCompare( rr1.pFirstRR, NULL, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsRecordSetCompare( NULL, rr2.pFirstRR, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );

    diff1 = NULL;
    diff2 = NULL;

    ok( pDnsRecordSetCompare( NULL, NULL, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 == NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    ok( pDnsRecordSetCompare( rr1.pFirstRR, NULL, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 != NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    ok( pDnsRecordSetCompare( NULL, rr2.pFirstRR, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 == NULL && diff2 != NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    ok( pDnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, NULL, &diff2 ) == TRUE, "failed unexpectedly\n" );
    ok( diff2 == NULL, "unexpected result: %p\n", diff2 );

    ok( pDnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, NULL ) == TRUE, "failed unexpectedly\n" );
    ok( diff1 == NULL, "unexpected result: %p\n", diff1 );

    ok( pDnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, &diff2 ) == TRUE, "failed unexpectedly\n" );
    ok( diff1 == NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    r2.Data.A.IpAddress = 0;

    ok( pDnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, NULL, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( pDnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
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
    test_DnsRecordSetCompare();

    FreeLibrary( dnsapi );
}
