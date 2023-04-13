/*
 * Copyright 2020 Dmitry Timoshkov
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
#include "winsock2.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"

#include "wine/test.h"

#define NS_MAXDNAME 1025

static void test_DnsQuery(void)
{
    WCHAR domain[MAX_PATH];
    WCHAR name[NS_MAXDNAME];
    DWORD ret, size;
    DNS_RECORDW *rec, *ptr;
    DNS_STATUS status;
    WORD type;

    rec = NULL;
    status = DnsQuery_W(L"winehq.org", DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    if (status == ERROR_TIMEOUT)
    {
        skip("query timed out\n");
        return;
    }
    ok(status == ERROR_SUCCESS, "got %ld\n", status);
    DnsRecordListFree(rec, DnsFreeRecordList);

    /* Show that DNS_TYPE_A returns CNAMEs too */
    rec = NULL;
    wcscpy(domain, L"test.winehq.org"); /* should be a CNAME */
    status = DnsQuery_W(domain, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    if (status == ERROR_TIMEOUT)
    {
        skip("query timed out\n");
        return;
    }
    ok(status == ERROR_SUCCESS, "got %ld\n", status);
    type = DNS_TYPE_CNAME; /* CNAMEs come first */
    for (ptr = rec; ptr; ptr = ptr->pNext)
    {
        ok(!wcscmp(domain, ptr->pName), "expected record name %s, got %s\n", wine_dbgstr_w(domain), wine_dbgstr_w(ptr->pName));
        ok(type == ptr->wType, "expected record type %d, got %d\n", type, ptr->wType);
        if (ptr->wType == DNS_TYPE_CNAME)
        {
            /* CNAME chains are bad practice so assume A for the remainder */
            type = DNS_TYPE_A;
            wcscpy(domain, L"winehq.org");
            ok(!wcscmp(domain, ptr->Data.CNAME.pNameHost), "expected CNAME target %s, got %s\n", wine_dbgstr_w(domain), wine_dbgstr_w(ptr->Data.CNAME.pNameHost));
        }
    }
    DnsRecordListFree(rec, DnsFreeRecordList);

    /* But DNS_TYPE_CNAME does not return A records! */
    rec = NULL;
    wcscpy(domain, L"test.winehq.org");
    status = DnsQuery_W(L"test.winehq.org", DNS_TYPE_CNAME, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    if (status == ERROR_TIMEOUT)
    {
        skip("query timed out\n");
        return;
    }
    ok(status == ERROR_SUCCESS, "got %ld\n", status);
    ptr = rec;
    ok(!wcscmp(domain, ptr->pName), "expected record name %s, got %s\n", wine_dbgstr_w(domain), wine_dbgstr_w(ptr->pName));
    ok(DNS_TYPE_CNAME == ptr->wType, "expected record type %d, got %d\n", DNS_TYPE_CNAME, ptr->wType);
    if (ptr->wType == DNS_TYPE_CNAME)
    {
        wcscpy(domain, L"winehq.org");
        ok(!wcscmp(domain, ptr->Data.CNAME.pNameHost), "expected CNAME target %s, got %s\n", wine_dbgstr_w(domain), wine_dbgstr_w(ptr->Data.CNAME.pNameHost));
    }
    ok(ptr->pNext == NULL, "unexpected CNAME chain\n");
    DnsRecordListFree(rec, DnsFreeRecordList);

    status = DnsQuery_W(L"", DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == DNS_ERROR_RCODE_NAME_ERROR || status == DNS_INFO_NO_RECORDS || status == ERROR_INVALID_NAME /* XP */,
       "got %ld\n", status);

    wcscpy(domain, L"_ldap._tcp.deadbeef");
    status = DnsQuery_W(domain, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == DNS_ERROR_RCODE_NAME_ERROR || status == DNS_INFO_NO_RECORDS || status == ERROR_INVALID_NAME /* XP */,
       "got %ld\n", status);

    wcscpy(domain, L"_ldap._tcp.dc._msdcs.");
    size = ARRAY_SIZE(domain) - wcslen(domain);
    ret = GetComputerNameExW(ComputerNameDnsDomain, domain + wcslen(domain), &size);
    ok(ret, "GetComputerNameEx error %lu\n", GetLastError());
    if (!size)
    {
        skip("computer is not in a domain\n");
        return;
    }

    status = DnsQuery_W(domain, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    trace("DnsQuery_W(%s) => %ld\n", wine_dbgstr_w(domain), status);
    if (status != ERROR_SUCCESS)
    {
        skip("domain %s doesn't have an SRV entry\n", wine_dbgstr_w(domain));
        return;
    }

    trace("target %s, port %d\n", wine_dbgstr_w(rec->Data.Srv.pNameTarget), rec->Data.Srv.wPort);

    lstrcpynW(name, rec->Data.Srv.pNameTarget, ARRAY_SIZE(name));
    DnsRecordListFree(rec, DnsFreeRecordList);

    /* IPv4 */
    status = DnsQuery_W(name, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == ERROR_SUCCESS || status == DNS_ERROR_RCODE_NAME_ERROR, "DnsQuery_W(%s) => %ld\n",
       wine_dbgstr_w(name), status);
    if (status == ERROR_SUCCESS)
    {
        SOCKADDR_IN addr;
        WCHAR buf[IP4_ADDRESS_STRING_LENGTH];

        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        addr.sin_addr.s_addr = rec->Data.A.IpAddress;
        size = sizeof(buf);
        ret = WSAAddressToStringW((SOCKADDR *)&addr, sizeof(addr), NULL, buf, &size);
        ok(!ret, "WSAAddressToStringW error %lu\n", ret);
        trace("WSAAddressToStringW => %s\n", wine_dbgstr_w(buf));

        DnsRecordListFree(rec, DnsFreeRecordList);
    }

    /* IPv6 */
    status = DnsQuery_W(name, DNS_TYPE_AAAA, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == ERROR_SUCCESS || status == DNS_ERROR_RCODE_NAME_ERROR, "DnsQuery_W(%s) => %ld\n",
       wine_dbgstr_w(name), status);
    if (status == ERROR_SUCCESS)
    {
        SOCKADDR_IN6 addr;
        WCHAR buf[IP6_ADDRESS_STRING_LENGTH];

        addr.sin6_family = AF_INET6;
        addr.sin6_port = 0;
        addr.sin6_scope_id = 0;
        memcpy(addr.sin6_addr.s6_addr, &rec->Data.AAAA.Ip6Address, sizeof(rec->Data.AAAA.Ip6Address));
        size = sizeof(buf);
        ret = WSAAddressToStringW((SOCKADDR *)&addr, sizeof(addr), NULL, buf, &size);
        ok(!ret, "WSAAddressToStringW error %lu\n", ret);
        trace("WSAAddressToStringW => %s\n", wine_dbgstr_w(buf));

        DnsRecordListFree(rec, DnsFreeRecordList);
    }
}

static IP_ADAPTER_ADDRESSES *get_adapters(void)
{
    ULONG err, size = 1024;
    IP_ADAPTER_ADDRESSES *ret = malloc( size );
    for (;;)
    {
        err = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                               GAA_FLAG_SKIP_FRIENDLY_NAME,
                                    NULL, ret, &size );
        if (err != ERROR_BUFFER_OVERFLOW) break;
        ret = realloc( ret, size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static void test_DnsQueryConfig( void )
{
    DNS_STATUS err;
    DWORD size, i, ipv6_count;
    WCHAR name[MAX_ADAPTER_NAME_LENGTH + 1];
    IP_ADAPTER_ADDRESSES *adapters, *ptr;
    DNS_ADDR_ARRAY *ipv4, *ipv6, *unspec;
    IP4_ARRAY *ip4_array;

    if (!(adapters = get_adapters())) return;

    for (ptr = adapters; ptr; ptr = ptr->Next)
    {
        MultiByteToWideChar( CP_ACP, 0, ptr->AdapterName, -1, name, ARRAY_SIZE(name) );
        if (ptr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        size = 0;
        err = DnsQueryConfig( DnsConfigDnsServersIpv4, 0, name, NULL, NULL, &size );
        if (err) continue;
        ipv4 = malloc( size );
        size--;
        err = DnsQueryConfig( DnsConfigDnsServersIpv4, 0, name, NULL, ipv4, &size );
        ok( err == ERROR_MORE_DATA, "got %ld\n", err );
        size++;
        err = DnsQueryConfig( DnsConfigDnsServersIpv4, 0, name, NULL, ipv4, &size );
        ok( !err, "got %ld\n", err );

        ok( ipv4->AddrCount == ipv4->MaxCount, "got %lu vs %lu\n", ipv4->AddrCount, ipv4->MaxCount );
        ok( !ipv4->Tag, "got %#lx\n", ipv4->Tag );
        ok( !ipv4->Family, "got %d\n", ipv4->Family );
        ok( !ipv4->WordReserved, "got %#x\n", ipv4->WordReserved );
        ok( !ipv4->Flags, "got %#lx\n", ipv4->Flags );
        ok( !ipv4->MatchFlag, "got %#lx\n", ipv4->MatchFlag );
        ok( !ipv4->Reserved1, "got %#lx\n", ipv4->Reserved1 );
        ok( !ipv4->Reserved2, "got %#lx\n", ipv4->Reserved2 );

        size = 0;
        err = DnsQueryConfig( DnsConfigDnsServerList, 0, name, NULL, NULL, &size );
        ok( !err, "got %ld\n", err );
        ip4_array = malloc( size );
        err = DnsQueryConfig( DnsConfigDnsServerList, 0, name, NULL, ip4_array, &size );
        ok( !err, "got %ld\n", err );

        ok( ipv4->AddrCount == ip4_array->AddrCount, "got %lu vs %lu\n", ipv4->AddrCount, ip4_array->AddrCount );

        for (i = 0; i < ipv4->AddrCount; i++)
        {
            SOCKADDR_IN *sa = (SOCKADDR_IN *)ipv4->AddrArray[i].MaxSa;

            ok( sa->sin_family == AF_INET, "got %d\n", sa->sin_family );
            ok( sa->sin_addr.s_addr == ip4_array->AddrArray[i], "got %#lx vs %#lx\n",
                sa->sin_addr.s_addr, ip4_array->AddrArray[i] );
            ok( ipv4->AddrArray[i].Data.DnsAddrUserDword[0] == sizeof(*sa), "got %lu\n",
                ipv4->AddrArray[i].Data.DnsAddrUserDword[0] );
        }

        size = 0;
        err = DnsQueryConfig( DnsConfigDnsServersIpv6, 0, name, NULL, NULL, &size );
        ok( !err || err == DNS_ERROR_NO_DNS_SERVERS, "got %ld\n", err );
        ipv6_count = 0;
        ipv6 = NULL;
        if (!err)
        {
            ipv6 = malloc( size );
            err = DnsQueryConfig( DnsConfigDnsServersIpv6, 0, name, NULL, ipv6, &size );
            ok( !err, "got %ld\n", err );
            ipv6_count = ipv6->AddrCount;
        }

        size = 0;
        err = DnsQueryConfig( DnsConfigDnsServersUnspec, 0, name, NULL, NULL, &size );
        ok( !err, "got %ld\n", err );
        unspec = malloc( size );
        err = DnsQueryConfig( DnsConfigDnsServersUnspec, 0, name, NULL, unspec, &size );
        ok( !err, "got %ld\n", err );

        ok( unspec->AddrCount == ipv4->AddrCount + ipv6_count, "got %lu vs %lu + %lu\n",
            unspec->AddrCount, ipv4->AddrCount, ipv6_count );

        free( ip4_array );
        free( unspec );
        free( ipv6 );
        free( ipv4 );
    }

    free( adapters );
}

START_TEST(query)
{
    WSADATA data;

    WSAStartup(MAKEWORD(2, 2), &data);

    test_DnsQuery();
    test_DnsQueryConfig();
}
