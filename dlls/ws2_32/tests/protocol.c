/*
 * Unit test suite for protocol functions
 *
 * Copyright 2004 Hans Leidekker
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

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>

#include "wine/test.h"

/* TCP and UDP over IP fixed set of service flags */
#define TCPIP_SERVICE_FLAGS (XP1_GUARANTEED_DELIVERY \
                           | XP1_GUARANTEED_ORDER    \
                           | XP1_GRACEFUL_CLOSE      \
                           | XP1_EXPEDITED_DATA      \
                           | XP1_IFS_HANDLES)

#define UDPIP_SERVICE_FLAGS (XP1_CONNECTIONLESS      \
                           | XP1_MESSAGE_ORIENTED    \
                           | XP1_SUPPORT_BROADCAST   \
                           | XP1_SUPPORT_MULTIPOINT  \
                           | XP1_IFS_HANDLES)

static void test_service_flags(int family, int version, int socktype, int protocol, DWORD testflags)
{
    DWORD expectedflags = 0;
    if (socktype == SOCK_STREAM && protocol == IPPROTO_TCP)
        expectedflags = TCPIP_SERVICE_FLAGS;
    if (socktype == SOCK_DGRAM && protocol == IPPROTO_UDP)
        expectedflags = UDPIP_SERVICE_FLAGS;

    /* check if standard TCP and UDP protocols are offering the correct service flags */
    if ((family == AF_INET || family == AF_INET6) && version == 2 && expectedflags)
    {
        /* QOS may or may not be installed */
        testflags &= ~XP1_QOS_SUPPORTED;
        ok(expectedflags == testflags,
           "Incorrect flags, expected 0x%x, received 0x%x\n",
           expectedflags, testflags);
    }
}

static void test_WSAEnumProtocolsA(void)
{
    INT ret, i, j, found;
    DWORD len = 0, error;
    WSAPROTOCOL_INFOA info, *buffer;
    INT ptest[] = {0xdead, IPPROTO_TCP, 0xcafe, IPPROTO_UDP, 0xbeef, 0};

    ret = WSAEnumProtocolsA( NULL, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsA() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %d\n", error);

    len = 0;

    ret = WSAEnumProtocolsA( NULL, &info, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsA() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %d\n", error);

    buffer = HeapAlloc( GetProcessHeap(), 0, len );

    if (buffer)
    {
        ret = WSAEnumProtocolsA( NULL, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsA() failed unexpectedly: %d\n",
            WSAGetLastError() );

        for (i = 0; i < ret; i++)
        {
            ok( strlen( buffer[i].szProtocol ), "No protocol name found\n" );
            test_service_flags( buffer[i].iAddressFamily, buffer[i].iVersion,
                                buffer[i].iSocketType, buffer[i].iProtocol,
                                buffer[i].dwServiceFlags1);
        }

        HeapFree( GetProcessHeap(), 0, buffer );
    }

    /* Test invalid protocols in the list */
    ret = WSAEnumProtocolsA( ptest, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsA() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS || broken(error == WSAEFAULT) /* NT4 */,
       "Expected 10055, received %d\n", error);

    buffer = HeapAlloc( GetProcessHeap(), 0, len );

    if (buffer)
    {
        ret = WSAEnumProtocolsA( ptest, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsA() failed unexpectedly: %d\n",
            WSAGetLastError() );
        ok( ret >= 2, "Expected at least 2 items, received %d\n", ret);

        for (i = found = 0; i < ret; i++)
            for (j = 0; j < ARRAY_SIZE(ptest); j++)
                if (buffer[i].iProtocol == ptest[j])
                {
                    found |= 1 << j;
                    break;
                }
        ok(found == 0x0A, "Expected 2 bits represented as 0xA, received 0x%x\n", found);

        HeapFree( GetProcessHeap(), 0, buffer );
    }
}

static void test_WSAEnumProtocolsW(void)
{
    INT ret, i, j, found;
    DWORD len = 0, error;
    WSAPROTOCOL_INFOW info, *buffer;
    INT ptest[] = {0xdead, IPPROTO_TCP, 0xcafe, IPPROTO_UDP, 0xbeef, 0};

    ret = WSAEnumProtocolsW( NULL, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsW() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %d\n", error);

    len = 0;

    ret = WSAEnumProtocolsW( NULL, &info, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsW() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %d\n", error);

    buffer = HeapAlloc( GetProcessHeap(), 0, len );

    if (buffer)
    {
        ret = WSAEnumProtocolsW( NULL, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsW() failed unexpectedly: %d\n",
            WSAGetLastError() );

        for (i = 0; i < ret; i++)
        {
            ok( lstrlenW( buffer[i].szProtocol ), "No protocol name found\n" );
            test_service_flags( buffer[i].iAddressFamily, buffer[i].iVersion,
                                buffer[i].iSocketType, buffer[i].iProtocol,
                                buffer[i].dwServiceFlags1);
        }

        HeapFree( GetProcessHeap(), 0, buffer );
    }

    /* Test invalid protocols in the list */
    ret = WSAEnumProtocolsW( ptest, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsW() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS || broken(error == WSAEFAULT) /* NT4 */,
       "Expected 10055, received %d\n", error);

    buffer = HeapAlloc( GetProcessHeap(), 0, len );

    if (buffer)
    {
        ret = WSAEnumProtocolsW( ptest, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsW() failed unexpectedly: %d\n",
            WSAGetLastError() );
        ok( ret >= 2, "Expected at least 2 items, received %d\n", ret);

        for (i = found = 0; i < ret; i++)
            for (j = 0; j < ARRAY_SIZE(ptest); j++)
                if (buffer[i].iProtocol == ptest[j])
                {
                    found |= 1 << j;
                    break;
                }
        ok(found == 0x0A, "Expected 2 bits represented as 0xA, received 0x%x\n", found);

        HeapFree( GetProcessHeap(), 0, buffer );
    }
}

struct protocol
{
    int prot;
    const char *names[2];
    BOOL missing_from_xp;
};

static const struct protocol protocols[] =
{
    {   0, { "ip", "IP" }},
    {   1, { "icmp", "ICMP" }},
    {   3, { "ggp", "GGP" }},
    {   6, { "tcp", "TCP" }},
    {   8, { "egp", "EGP" }},
    {  12, { "pup", "PUP" }},
    {  17, { "udp", "UDP" }},
    {  20, { "hmp", "HMP" }},
    {  22, { "xns-idp", "XNS-IDP" }},
    {  27, { "rdp", "RDP" }},
    {  41, { "ipv6", "IPv6" }, TRUE},
    {  43, { "ipv6-route", "IPv6-Route" }, TRUE},
    {  44, { "ipv6-frag", "IPv6-Frag" }, TRUE},
    {  50, { "esp", "ESP" }, TRUE},
    {  51, { "ah", "AH" }, TRUE},
    {  58, { "ipv6-icmp", "IPv6-ICMP" }, TRUE},
    {  59, { "ipv6-nonxt", "IPv6-NoNxt" }, TRUE},
    {  60, { "ipv6-opts", "IPv6-Opts" }, TRUE},
    {  66, { "rvd", "RVD" }},
};

static const struct protocol *find_protocol(int number)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(protocols); i++)
    {
        if (protocols[i].prot == number)
            return &protocols[i];
    }
    return NULL;
}

static void test_getprotobyname(void)
{
    struct protoent *ent;
    char all_caps_name[16];
    int i, j;

    for (i = 0; i < ARRAY_SIZE(protocols); i++)
    {
        for (j = 0; j < ARRAY_SIZE(protocols[0].names); j++)
        {
            ent = getprotobyname(protocols[i].names[j]);
            ok((ent && ent->p_proto == protocols[i].prot) || broken(!ent && protocols[i].missing_from_xp),
               "Expected %s to be protocol number %d, got %d\n",
               wine_dbgstr_a(protocols[i].names[j]), protocols[i].prot, ent ? ent->p_proto : -1);
        }

        for (j = 0; protocols[i].names[0][j]; j++)
            all_caps_name[j] = toupper(protocols[i].names[0][j]);
        all_caps_name[j] = 0;
        ent = getprotobyname(all_caps_name);
        ok((ent && ent->p_proto == protocols[i].prot) || broken(!ent && protocols[i].missing_from_xp),
           "Expected %s to be protocol number %d, got %d\n",
           wine_dbgstr_a(all_caps_name), protocols[i].prot, ent ? ent->p_proto : -1);
    }
}

static void test_getprotobynumber(void)
{
    struct protoent *ent;
    const struct protocol *ref;
    int i;

    for (i = -1; i <= 256; i++)
    {
        ent = getprotobynumber(i);
        ref = find_protocol(i);

        if (!ref)
        {
            ok(!ent, "Expected protocol number %d to be undefined, got %s\n",
               i, wine_dbgstr_a(ent ? ent->p_name : NULL));
            continue;
        }

        ok((ent && ent->p_name && strcmp(ent->p_name, ref->names[0]) == 0) ||
           broken(!ent && ref->missing_from_xp),
           "Expected protocol number %d to be %s, got %s\n",
           i, ref->names[0], wine_dbgstr_a(ent ? ent->p_name : NULL));

        ok((ent && ent->p_aliases && ent->p_aliases[0] &&
            strcmp(ent->p_aliases[0], ref->names[1]) == 0) ||
           broken(!ent && ref->missing_from_xp),
           "Expected protocol number %d alias 0 to be %s, got %s\n",
           i, ref->names[0], wine_dbgstr_a(ent && ent->p_aliases ? ent->p_aliases[0] : NULL));
    }
}

START_TEST( protocol )
{
    WSADATA data;
    WORD version = MAKEWORD( 2, 2 );
 
    if (WSAStartup( version, &data )) return;

    test_WSAEnumProtocolsA();
    test_WSAEnumProtocolsW();
    test_getprotobyname();
    test_getprotobynumber();
}
