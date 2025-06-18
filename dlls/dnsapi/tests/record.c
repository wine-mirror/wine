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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"

#include "wine/test.h"

static WCHAR name1[] = L"localhost";
static WCHAR name2[] = L"LOCALHOST";

static DNS_RECORDW r1 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };
static DNS_RECORDW r2 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };
static DNS_RECORDW r3 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };

static void test_DnsRecordCompare( void )
{
    ok( DnsRecordCompare( &r1, &r1 ) == TRUE, "failed unexpectedly\n" );

    r2.pName = name2;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetUnicode;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetAnsi;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r1.Flags.S.CharSet = DnsCharSetAnsi;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r1.dwTtl = 0;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Data.A.IpAddress = 0;
    ok( DnsRecordCompare( &r1, &r2 ) == FALSE, "succeeded unexpectedly\n" );
}

static void test_DnsRecordSetCompare( void )
{
    DNS_RECORDW *diff1;
    DNS_RECORDW *diff2;
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

    ok( DnsRecordSetCompare( NULL, NULL, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsRecordSetCompare( rr1.pFirstRR, NULL, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsRecordSetCompare( NULL, rr2.pFirstRR, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );

    diff1 = NULL;
    diff2 = NULL;

    ok( DnsRecordSetCompare( NULL, NULL, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 == NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    ok( DnsRecordSetCompare( rr1.pFirstRR, NULL, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 != NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );
    DnsRecordListFree( diff1, DnsFreeRecordList );

    ok( DnsRecordSetCompare( NULL, rr2.pFirstRR, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 == NULL && diff2 != NULL, "unexpected result: %p, %p\n", diff1, diff2 );
    DnsRecordListFree( diff2, DnsFreeRecordList );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, NULL, &diff2 ) == TRUE, "failed unexpectedly\n" );
    ok( diff2 == NULL, "unexpected result: %p\n", diff2 );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, NULL ) == TRUE, "failed unexpectedly\n" );
    ok( diff1 == NULL, "unexpected result: %p\n", diff1 );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, &diff2 ) == TRUE, "failed unexpectedly\n" );
    ok( diff1 == NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    r2.Data.A.IpAddress = 0;

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, NULL, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    DnsRecordListFree( diff2, DnsFreeRecordList );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, NULL ) == FALSE, "succeeded unexpectedly\n" );
    DnsRecordListFree( diff1, DnsFreeRecordList );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    DnsRecordListFree( diff1, DnsFreeRecordList );
    DnsRecordListFree( diff2, DnsFreeRecordList );
}

static void test_DnsRecordSetDetach( void )
{
    DNS_RRSET rr;
    DNS_RECORDW *r, *s;

    DNS_RRSET_INIT( rr );
    DNS_RRSET_ADD( rr, &r1 );
    DNS_RRSET_ADD( rr, &r2 );
    DNS_RRSET_ADD( rr, &r3 );
    DNS_RRSET_TERMINATE( rr );

    ok( !DnsRecordSetDetach( NULL ), "succeeded unexpectedly\n" );

    r = rr.pFirstRR;
    s = DnsRecordSetDetach( r );

    ok( s == &r3, "failed unexpectedly: got %p, expected %p\n", s, &r3 );
    ok( r == &r1, "failed unexpectedly: got %p, expected %p\n", r, &r1 );
    ok( !r2.pNext, "failed unexpectedly\n" );
}

static BYTE msg_empty[] =  /* empty message */
{
    0x12, 0x34, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00
};
static BYTE msg_empty_answer[] =  /* marked as answer but contains only question */
{
    0x12, 0x34, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01
};
static BYTE msg_question[] =  /* question only */
{
    0x12, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01
};
static BYTE msg_winehq[] =  /* valid answer for winehq.org */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 'w', 'i', 'n', 'e', 'h', 'q', 0x03, 'o', 'r', 'g', 0x00, 0x00, 0x01, 0x00, 0x01,
    0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x00, 0x04, 0x04, 0x04, 0x51, 0x7c
};
static BYTE msg_invchars[] =  /* invalid characters in name */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 'a', '$', 0x02, 'b', '\\', 0x02, 'c', '.', 0x02, 0x09, 'd', 0x00, 0x00, 0x01, 0x00, 0x01,
    0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x04, 0x04, 0x51, 0x7c
};
static BYTE msg_root[] =  /* root name */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01,
    0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x04, 0x04, 0x51, 0x7c
};
static BYTE msg_label[] =  /* name with binary label, not supported on Windows */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x03, 'a', 'b', 'c', 0x41, 0x01, 0x34, 0x00, 0x00, 0x01, 0x00, 0x01,
    0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x04, 0x04, 0x51, 0x7c
};
static BYTE msg_loop[] =  /* name with loop */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01,
    0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x04, 0x04, 0x51, 0x7c
};
static BYTE msg_types[] =  /* various record types */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 'w', 'i', 'n', 'e', 'h', 'q', 0x03, 'o', 'r', 'g', 0x00, 0x00, 0x01, 0x00, 0x01,
/* CNAME */ 0xc0, 0x0c, 0x00, 0x05, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x00, 0x05, 0x01, 'a', 0x01, 'b', 0x00,
/* MX */    0xc0, 0x0c, 0x00, 0x0f, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x00, 0x07, 0x03, 0x04, 0x01, 'c', 0x01, 'd', 0x00,
/* AAAA */  0xc0, 0x0c, 0x00, 0x1c, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x00, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00,
/* KEY */   0xc0, 0x0c, 0x00, 0x19, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x00, 0x06, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
/* TXT */   0x01, 't', 0x01, 'x', 0x00, 0x00, 0x10, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x00, 0x09, 0x02, 'z', 'y', 0x00, 0x04, 'X', 'Y', 0xc3, 0xa9
};
static BYTE msg_question_srv[] =  /* SRV question only */
{
    0x12, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    5,'_','l','d','a','p',4,'_','t','c','p',2,'d','c',6,'_','m','s','d','c','s',6,'w','i','n','e','h','q',3,'o','r','g',0x00,0x00,0x00,0x21,0x00
};
static BYTE msg_answer_srv[] =  /* SRV answer only */
{
    0x12, 0x34, 0x81, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    5,'_','l','d','a','p',4,'_','t','c','p',2,'d','c',6,'_','m','s','d','c','s',6,'w','i','n','e','h','q',3,'o','r','g',0x00,
    0x00,0x21,0x00,0x01,0x04,0x05,0x06,0x07,
    0x00,0x15,0x00,0x00,0x00,0x00,0x01,0x85,
    2,'d','c',6,'w','i','n','e','h','q',3,'o','r','g',0x00
};
static BYTE msg_full_srv[] =  /* SRV question + answer */
{
    0x12, 0x34, 0x81, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    5,'_','l','d','a','p',4,'_','t','c','p',2,'d','c',6,'_','m','s','d','c','s',6,'w','i','n','e','h','q',3,'o','r','g',0x00,
    0x00,0x21,0x00,0x01,
    0xc0,0x0c,0x00,0x21,0x00,0x01,0x04,0x05,0x06,0x07,
    0x00,0x15,0x00,0x00,0x00,0x00,0x01,0x85,
    2,'d','c',6,'w','i','n','e','h','q',3,'o','r','g',0x00
};

static void test_DnsExtractRecordsFromMessage(void)
{
    DNS_STATUS ret;
    DNS_RECORDA *rec, *r;
    DNS_RECORDW *recW, *rW;

    rec = NULL;
    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_full_srv, sizeof(msg_full_srv), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( rec != NULL, "record not set\n" );
    ok( !strcmp( rec->pName, "_ldap._tcp.dc._msdcs.winehq.org" ), "wrong name %s\n", rec->pName );
    ok( rec->Flags.S.Section == DnsSectionAnswer, "wrong section %u\n", rec->Flags.S.Section );
    ok( rec->Flags.S.CharSet == DnsCharSetUtf8, "wrong charset %u\n", rec->Flags.S.CharSet );
    ok( rec->wType == DNS_TYPE_SRV, "wrong type %u\n", rec->wType );
    ok( rec->wDataLength == sizeof(DNS_SRV_DATAA) + strlen( "dc.winehq.org" ) + 1, "wrong len %u\n", rec->wDataLength );
    ok( rec->dwTtl == 0x04050607, "wrong ttl %#lx\n", rec->dwTtl );
    ok( !strcmp( rec->Data.SRV.pNameTarget, "dc.winehq.org"), "wrong target %s\n", rec->Data.SRV.pNameTarget );
    ok( !rec->pNext, "next record %p\n", rec->pNext );
    DnsRecordListFree( (DNS_RECORD *)rec, DnsFreeRecordList );

    rec = (void *)0xdeadbeef;
    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_question_srv, sizeof(msg_question_srv), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( !rec, "record %p\n", rec );

    rec = NULL;
    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_answer_srv, sizeof(msg_answer_srv), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( rec != NULL, "record not set\n" );
    ok( !strcmp( rec->pName, "_ldap._tcp.dc._msdcs.winehq.org" ), "wrong name %s\n", rec->pName );
    ok( rec->Flags.S.Section == DnsSectionAnswer, "wrong section %u\n", rec->Flags.S.Section );
    ok( rec->Flags.S.CharSet == DnsCharSetUtf8, "wrong charset %u\n", rec->Flags.S.CharSet );
    ok( rec->wType == DNS_TYPE_SRV, "wrong type %u\n", rec->wType );
    ok( rec->wDataLength == sizeof(DNS_SRV_DATAA) + strlen( "dc.winehq.org" ) + 1, "wrong len %u\n", rec->wDataLength );
    ok( rec->dwTtl == 0x04050607, "wrong ttl %#lx\n", rec->dwTtl );
    ok( !strcmp( rec->Data.SRV.pNameTarget, "dc.winehq.org"), "wrong target %s\n", rec->Data.SRV.pNameTarget );
    ok( !rec->pNext, "next record %p\n", rec->pNext );
    DnsRecordListFree( (DNS_RECORD *)rec, DnsFreeRecordList );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_empty, sizeof(msg_empty) - 1, &rec );
    ok( ret == ERROR_INVALID_PARAMETER || broken(ret == DNS_ERROR_BAD_PACKET) /* win7 */,
        "failed %ld\n", ret );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_empty, sizeof(msg_empty), &rec );
    ok( ret == DNS_ERROR_BAD_PACKET, "failed %ld\n", ret );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_empty_answer, sizeof(msg_empty_answer), &rec );
    ok( ret == DNS_ERROR_BAD_PACKET, "failed %ld\n", ret );

    rec = (void *)0xdeadbeef;
    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_question, sizeof(msg_question), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( !rec, "record %p\n", rec );

    rec = (void *)0xdeadbeef;
    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_winehq, sizeof(msg_winehq), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( rec != NULL, "record not set\n" );
    ok( !strcmp( rec->pName, "winehq.org" ), "wrong name %s\n", rec->pName );
    ok( rec->Flags.S.Section == DnsSectionAnswer, "wrong section %u\n", rec->Flags.S.Section );
    ok( rec->Flags.S.CharSet == DnsCharSetUtf8, "wrong charset %u\n", rec->Flags.S.CharSet );
    ok( rec->wType == DNS_TYPE_A, "wrong type %u\n", rec->wType );
    ok( rec->wDataLength == sizeof(DNS_A_DATA), "wrong len %u\n", rec->wDataLength );
    ok( rec->dwTtl == 0x04050607, "wrong ttl %#lx\n", rec->dwTtl );
    ok( rec->Data.A.IpAddress == 0x7c510404, "wrong addr %#lx\n", rec->Data.A.IpAddress );
    ok( !rec->pNext, "next record %p\n", rec->pNext );
    DnsRecordListFree( (DNS_RECORD *)rec, DnsFreeRecordList );

    recW = (void *)0xdeadbeef;
    ret = DnsExtractRecordsFromMessage_W( (DNS_MESSAGE_BUFFER *)msg_winehq, sizeof(msg_winehq), &recW );
    ok( !ret, "failed %ld\n", ret );
    ok( recW != NULL, "record not set\n" );
    ok( !wcscmp( recW->pName, L"winehq.org" ), "wrong name %s\n", debugstr_w(recW->pName) );
    DnsRecordListFree( (DNS_RECORD *)recW, DnsFreeRecordList );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_invchars, sizeof(msg_invchars), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( !strcmp( rec->pName, "a$.b\\.c..\td" ), "wrong name %s\n", rec->pName );
    DnsRecordListFree( (DNS_RECORD *)rec, DnsFreeRecordList );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_root, sizeof(msg_root), &rec );
    ok( !ret, "failed %ld\n", ret );
    ok( !strcmp( rec->pName, "." ), "wrong name %s\n", rec->pName );
    DnsRecordListFree( (DNS_RECORD *)rec, DnsFreeRecordList );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_label, sizeof(msg_label), &rec );
    ok( ret == DNS_ERROR_BAD_PACKET, "failed %ld\n", ret );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_loop, sizeof(msg_loop), &rec );
    ok( ret == DNS_ERROR_BAD_PACKET, "failed %ld\n", ret );

    ret = DnsExtractRecordsFromMessage_UTF8( (DNS_MESSAGE_BUFFER *)msg_types, sizeof(msg_types), &rec );
    ok( !ret, "failed %ld\n", ret );
    r = rec;
    ok( r != NULL, "record not set\n" );
    ok( !strcmp( r->pName, "winehq.org" ), "wrong name %s\n", r->pName );
    ok( r->wType == DNS_TYPE_CNAME, "wrong type %u\n", r->wType );
    ok( !strcmp( r->Data.CNAME.pNameHost, "a.b" ), "wrong cname %s\n", r->Data.CNAME.pNameHost );
    ok( r->wDataLength == sizeof(DNS_PTR_DATAA) + 4, "wrong len %x\n", r->wDataLength );
    r = r->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !strcmp( r->pName, "winehq.org" ), "wrong name %s\n", r->pName );
    ok( r->wType == DNS_TYPE_MX, "wrong type %u\n", r->wType );
    ok( r->Data.MX.wPreference == 0x0304, "wrong pref %x\n", r->Data.MX.wPreference );
    ok( !strcmp( r->Data.MX.pNameExchange, "c.d" ), "wrong mx %s\n", r->Data.MX.pNameExchange );
    ok( r->wDataLength == sizeof(DNS_MX_DATAA) + 4, "wrong len %x\n", r->wDataLength );
    r = r->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !strcmp( r->pName, "winehq.org" ), "wrong name %s\n", r->pName );
    ok( r->wType == DNS_TYPE_AAAA, "wrong type %u\n", r->wType );
    ok( r->Data.AAAA.Ip6Address.IP6Dword[0] == 0x04030201, "wrong addr %#lx\n",
        r->Data.AAAA.Ip6Address.IP6Dword[0] );
    ok( r->Data.AAAA.Ip6Address.IP6Dword[1] == 0x08070605, "wrong addr %#lx\n",
        r->Data.AAAA.Ip6Address.IP6Dword[1] );
    ok( r->Data.AAAA.Ip6Address.IP6Dword[2] == 0x0c0b0a09, "wrong addr %#lx\n",
        r->Data.AAAA.Ip6Address.IP6Dword[2] );
    ok( r->Data.AAAA.Ip6Address.IP6Dword[3] == 0x000f0e0d, "wrong addr %#lx\n",
        r->Data.AAAA.Ip6Address.IP6Dword[3] );
    ok( r->wDataLength == sizeof(DNS_AAAA_DATA), "wrong len %u\n", r->wDataLength );
    r = r->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !strcmp( r->pName, "winehq.org" ), "wrong name %s\n", r->pName );
    ok( r->wType == DNS_TYPE_KEY, "wrong type %u\n", r->wType );
    ok( r->Data.KEY.wFlags == 0x1122, "wrong flags %x\n", r->Data.KEY.wFlags );
    ok( r->Data.KEY.chProtocol == 0x33, "wrong protocol %x\n", r->Data.KEY.chProtocol );
    ok( r->Data.KEY.chAlgorithm == 0x44, "wrong algorithm %x\n", r->Data.KEY.chAlgorithm );
    ok( r->Data.KEY.wKeyLength == 0x02, "wrong len %x\n", r->Data.KEY.wKeyLength );
    ok( r->Data.KEY.Key[0] == 0x55, "wrong key %x\n", r->Data.KEY.Key[0] );
    ok( r->Data.KEY.Key[1] == 0x66, "wrong key %x\n", r->Data.KEY.Key[1] );
    ok( r->wDataLength == offsetof( DNS_KEY_DATA, Key[r->Data.KEY.wKeyLength] ),
        "wrong len %x\n", r->wDataLength );
    r = r->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !strcmp( r->pName, "t.x" ), "wrong name %s\n", r->pName );
    ok( r->wType == DNS_TYPE_TEXT, "wrong type %u\n", r->wType );
    ok( r->Data.TXT.dwStringCount == 3, "wrong count %lu\n", r->Data.TXT.dwStringCount );
    ok( !strcmp( r->Data.TXT.pStringArray[0], "zy" ), "wrong string %s\n", r->Data.TXT.pStringArray[0] );
    ok( !strcmp( r->Data.TXT.pStringArray[1], "" ), "wrong string %s\n", r->Data.TXT.pStringArray[1] );
    ok( !strcmp( r->Data.TXT.pStringArray[2], "XY\xc3\xa9" ), "wrong string %s\n", r->Data.TXT.pStringArray[2] );
    ok( r->wDataLength == offsetof( DNS_TXT_DATAA, pStringArray[r->Data.TXT.dwStringCount] ),
        "wrong len %x\n", r->wDataLength );
    ok( !r->pNext, "next record %p\n", r->pNext );
    DnsRecordListFree( (DNS_RECORD *)rec, DnsFreeRecordList );

    ret = DnsExtractRecordsFromMessage_W( (DNS_MESSAGE_BUFFER *)msg_types, sizeof(msg_types), &recW );
    ok( !ret, "failed %ld\n", ret );
    rW = recW;
    ok( rW != NULL, "record not set\n" );
    ok( !wcscmp( rW->pName, L"winehq.org" ), "wrong name %s\n", debugstr_w(rW->pName) );
    ok( rW->wType == DNS_TYPE_CNAME, "wrong type %u\n", rW->wType );
    ok( !wcscmp( rW->Data.CNAME.pNameHost, L"a.b" ), "wrong cname %s\n", debugstr_w(rW->Data.CNAME.pNameHost) );
    rW = rW->pNext;
    ok( rW != NULL, "record not set\n" );
    ok( !wcscmp( rW->pName, L"winehq.org" ), "wrong name %s\n", debugstr_w(rW->pName) );
    ok( rW->wType == DNS_TYPE_MX, "wrong type %u\n", rW->wType );
    ok( !wcscmp( rW->Data.MX.pNameExchange, L"c.d" ), "wrong mx %s\n", debugstr_w(rW->Data.MX.pNameExchange) );
    rW = rW->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !wcscmp( rW->pName, L"winehq.org" ), "wrong name %s\n", debugstr_w(rW->pName) );
    ok( rW->wType == DNS_TYPE_AAAA, "wrong type %u\n", rW->wType );
    rW = rW->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !wcscmp( rW->pName, L"winehq.org" ), "wrong name %s\n", debugstr_w(rW->pName) );
    rW = rW->pNext;
    ok( r != NULL, "record not set\n" );
    ok( !wcscmp( rW->pName, L"t.x" ), "wrong name %s\n", debugstr_w(rW->pName) );
    ok( rW->wType == DNS_TYPE_TEXT, "wrong type %u\n", rW->wType );
    ok( rW->Data.TXT.dwStringCount == 3, "wrong count %lu\n", rW->Data.TXT.dwStringCount );
    ok( !wcscmp( rW->Data.TXT.pStringArray[0], L"zy" ), "wrong string %s\n", debugstr_w(rW->Data.TXT.pStringArray[0]) );
    ok( !wcscmp( rW->Data.TXT.pStringArray[1], L"" ), "wrong string %s\n", debugstr_w(rW->Data.TXT.pStringArray[1]) );
    ok( !wcscmp( rW->Data.TXT.pStringArray[2], L"XY\x00e9" ), "wrong string %s\n", debugstr_w(rW->Data.TXT.pStringArray[2]) );
    ok( !rW->pNext, "next record %p\n", rW->pNext );
    DnsRecordListFree( (DNS_RECORD *)recW, DnsFreeRecordList );
}

START_TEST(record)
{
    test_DnsRecordCompare();
    test_DnsRecordSetCompare();
    test_DnsRecordSetDetach();
    test_DnsExtractRecordsFromMessage();
}
