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

#include "wine/test.h"

#define NS_MAXDNAME 1025

static void test_DnsQuery(void)
{
    WCHAR domain[MAX_PATH];
    WCHAR name[NS_MAXDNAME];
    DWORD ret, size;
    DNS_RECORDW *rec;
    DNS_STATUS status;

    rec = NULL;
    status = DnsQuery_W(L"winehq.org", DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    if (status == ERROR_TIMEOUT)
    {
        skip("query timed out\n");
        return;
    }
    ok(status == ERROR_SUCCESS, "got %d\n", status);
    DnsRecordListFree(rec, DnsFreeRecordList);

    status = DnsQuery_W(L"", DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == DNS_ERROR_RCODE_NAME_ERROR || status == DNS_INFO_NO_RECORDS || status == ERROR_INVALID_NAME /* XP */, "got %u\n", status);

    wcscpy(domain, L"_ldap._tcp.deadbeef");
    status = DnsQuery_W(domain, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == DNS_ERROR_RCODE_NAME_ERROR || status == DNS_INFO_NO_RECORDS || status == ERROR_INVALID_NAME /* XP */, "got %u\n", status);

    wcscpy(domain, L"_ldap._tcp.dc._msdcs.");
    size = ARRAY_SIZE(domain) - wcslen(domain);
    ret = GetComputerNameExW(ComputerNameDnsDomain, domain + wcslen(domain), &size);
    ok(ret, "GetComputerNameEx error %u\n", GetLastError());
    if (!size)
    {
        skip("computer is not in a domain\n");
        return;
    }

    status = DnsQuery_W(domain, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    trace("DnsQuery_W(%s) => %d\n", wine_dbgstr_w(domain), status);
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
    ok(status == ERROR_SUCCESS || status == DNS_ERROR_RCODE_NAME_ERROR, "DnsQuery_W(%s) => %d\n", wine_dbgstr_w(name), status);
    if (status == ERROR_SUCCESS)
    {
        SOCKADDR_IN addr;
        WCHAR buf[IP4_ADDRESS_STRING_LENGTH];

        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        addr.sin_addr.s_addr = rec->Data.A.IpAddress;
        size = sizeof(buf);
        ret = WSAAddressToStringW((SOCKADDR *)&addr, sizeof(addr), NULL, buf, &size);
        ok(!ret, "WSAAddressToStringW error %d\n", ret);
        trace("WSAAddressToStringW => %s\n", wine_dbgstr_w(buf));

        DnsRecordListFree(rec, DnsFreeRecordList);
    }

    /* IPv6 */
    status = DnsQuery_W(name, DNS_TYPE_AAAA, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    ok(status == ERROR_SUCCESS || status == DNS_ERROR_RCODE_NAME_ERROR, "DnsQuery_W(%s) => %d\n", wine_dbgstr_w(name), status);
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
        ok(!ret, "WSAAddressToStringW error %d\n", ret);
        trace("WSAAddressToStringW => %s\n", wine_dbgstr_w(buf));

        DnsRecordListFree(rec, DnsFreeRecordList);
    }
}

START_TEST(query)
{
    WSADATA data;

    WSAStartup(MAKEWORD(2, 2), &data);

    test_DnsQuery();
}
