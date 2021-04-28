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
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "wine/test.h"

static void (WINAPI *pFreeAddrInfoExW)(ADDRINFOEXW *ai);
static int (WINAPI *pGetAddrInfoExW)(const WCHAR *name, const WCHAR *servname, DWORD namespace,
        GUID *namespace_id, const ADDRINFOEXW *hints, ADDRINFOEXW **result,
        struct timeval *timeout, OVERLAPPED *overlapped,
        LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine, HANDLE *handle);
static int   (WINAPI *pGetAddrInfoExOverlappedResult)(OVERLAPPED *overlapped);
static int (WINAPI *pGetHostNameW)(WCHAR *name, int len);
static const char *(WINAPI *p_inet_ntop)(int family, void *addr, char *string, ULONG size);

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

/* Tests used in both getaddrinfo and GetAddrInfoW */
static const struct addr_hint_tests
{
    int family, socktype, protocol;
    DWORD error;
}
hinttests[] =
{
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0},
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_UDP, 0},
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_IPV6,0},
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_TCP, 0},
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_UDP, 0},
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_IPV6,0},
    {AF_INET,   SOCK_STREAM, IPPROTO_TCP, 0},
    {AF_INET,   SOCK_STREAM, IPPROTO_UDP, 0},
    {AF_INET,   SOCK_STREAM, IPPROTO_IPV6,0},
    {AF_INET,   SOCK_DGRAM,  IPPROTO_TCP, 0},
    {AF_INET,   SOCK_DGRAM,  IPPROTO_UDP, 0},
    {AF_INET,   SOCK_DGRAM,  IPPROTO_IPV6,0},
    {AF_UNSPEC, 0,           IPPROTO_TCP, 0},
    {AF_UNSPEC, 0,           IPPROTO_UDP, 0},
    {AF_UNSPEC, 0,           IPPROTO_IPV6,0},
    {AF_UNSPEC, SOCK_STREAM, 0,           0},
    {AF_UNSPEC, SOCK_DGRAM,  0,           0},
    {AF_INET,   0,           IPPROTO_TCP, 0},
    {AF_INET,   0,           IPPROTO_UDP, 0},
    {AF_INET,   0,           IPPROTO_IPV6,0},
    {AF_INET,   SOCK_STREAM, 0,           0},
    {AF_INET,   SOCK_DGRAM,  0,           0},
    {AF_UNSPEC, 999,         IPPROTO_TCP, WSAESOCKTNOSUPPORT},
    {AF_UNSPEC, 999,         IPPROTO_UDP, WSAESOCKTNOSUPPORT},
    {AF_UNSPEC, 999,         IPPROTO_IPV6,WSAESOCKTNOSUPPORT},
    {AF_INET,   999,         IPPROTO_TCP, WSAESOCKTNOSUPPORT},
    {AF_INET,   999,         IPPROTO_UDP, WSAESOCKTNOSUPPORT},
    {AF_INET,   999,         IPPROTO_IPV6,WSAESOCKTNOSUPPORT},
    {AF_UNSPEC, SOCK_STREAM, 999,         0},
    {AF_UNSPEC, SOCK_STREAM, 999,         0},
    {AF_INET,   SOCK_DGRAM,  999,         0},
    {AF_INET,   SOCK_DGRAM,  999,         0},
};

static void compare_addrinfow(ADDRINFOW *a, ADDRINFOW *b)
{
    for (; a && b; a = a->ai_next, b = b->ai_next)
    {
        ok(a->ai_flags == b->ai_flags,
           "Wrong flags %d != %d\n", a->ai_flags, b->ai_flags);
        ok(a->ai_family == b->ai_family,
           "Wrong family %d != %d\n", a->ai_family, b->ai_family);
        ok(a->ai_socktype == b->ai_socktype,
           "Wrong socktype %d != %d\n", a->ai_socktype, b->ai_socktype);
        ok(a->ai_protocol == b->ai_protocol,
           "Wrong protocol %d != %d\n", a->ai_protocol, b->ai_protocol);
        ok(a->ai_addrlen == b->ai_addrlen,
           "Wrong addrlen %lu != %lu\n", a->ai_addrlen, b->ai_addrlen);
        ok(!memcmp(a->ai_addr, b->ai_addr, min(a->ai_addrlen, b->ai_addrlen)),
           "Wrong address data\n");
        if (a->ai_canonname && b->ai_canonname)
        {
            ok(!lstrcmpW(a->ai_canonname, b->ai_canonname), "Wrong canonical name '%s' != '%s'\n",
               wine_dbgstr_w(a->ai_canonname), wine_dbgstr_w(b->ai_canonname));
        }
        else
            ok(!a->ai_canonname && !b->ai_canonname, "Expected both names absent (%p != %p)\n",
               a->ai_canonname, b->ai_canonname);
    }
    ok(!a && !b, "Expected both addresses null (%p != %p)\n", a, b);
}

static void test_GetAddrInfoW(void)
{
    static const WCHAR port[] = {'8','0',0};
    static const WCHAR empty[] = {0};
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
    static const WCHAR nxdomain[] =
        {'n','x','d','o','m','a','i','n','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};
    static const WCHAR zero[] = {'0',0};
    int i, ret;
    ADDRINFOW *result, *result2, *p, hint;
    WCHAR name[256];
    DWORD size = ARRAY_SIZE(name);
    /* te su to.winehq.org written in katakana */
    static const WCHAR idn_domain[] =
        {0x30C6,0x30B9,0x30C8,'.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR idn_punycode[] =
        {'x','n','-','-','z','c','k','z','a','h','.','w','i','n','e','h','q','.','o','r','g',0};

    memset(&hint, 0, sizeof(ADDRINFOW));
    name[0] = 0;
    GetComputerNameExW( ComputerNamePhysicalDnsHostname, name, &size );

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(NULL, NULL, NULL, &result);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(empty, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(NULL, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    result2 = NULL;
    ret = GetAddrInfoW(NULL, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    result = NULL;
    ret = GetAddrInfoW(empty, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    result2 = NULL;
    ret = GetAddrInfoW(empty, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    result = NULL;
    ret = GetAddrInfoW(localhost, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, empty, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, port, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, NULL, &hint, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    SetLastError(0xdeadbeef);
    ret = GetAddrInfoW(localhost, port, &hint, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    /* try to get information from the computer name, result is the same
     * as if requesting with an empty host name. */
    ret = GetAddrInfoW(name, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = GetAddrInfoW(empty, NULL, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    ret = GetAddrInfoW(name, empty, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = GetAddrInfoW(empty, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(NULL, NULL, NULL, &result);
    if (ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(nxdomain, NULL, NULL, &result);
    if (ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    for (i = 0; i < ARRAY_SIZE(hinttests); i++)
    {
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        SetLastError(0xdeadbeef);
        ret = GetAddrInfoW(localhost, NULL, &hint, &result);
        todo_wine_if (hinttests[i].error) ok(ret == hinttests[i].error, "test %d: wrong ret %d\n", i, ret);
        if (!ret)
        {
            for (p = result; p; p = p->ai_next)
            {
                /* when AF_UNSPEC is used the return will be either AF_INET or AF_INET6 */
                if (hinttests[i].family == AF_UNSPEC)
                    ok(p->ai_family == AF_INET || p->ai_family == AF_INET6,
                       "test %d: expected AF_INET or AF_INET6, got %d\n",
                       i, p->ai_family);
                else
                    ok(p->ai_family == hinttests[i].family,
                       "test %d: expected family %d, got %d\n",
                       i, hinttests[i].family, p->ai_family);

                ok(p->ai_socktype == hinttests[i].socktype,
                   "test %d: expected type %d, got %d\n",
                   i, hinttests[i].socktype, p->ai_socktype);
                ok(p->ai_protocol == hinttests[i].protocol,
                   "test %d: expected protocol %d, got %d\n",
                   i, hinttests[i].protocol, p->ai_protocol);
            }
            FreeAddrInfoW(result);
        }
        else
        {
            ok(WSAGetLastError() == hinttests[i].error, "test %d: wrong error %d\n", i, WSAGetLastError());
        }
    }

    /* Test IDN resolution (Internationalized Domain Names) present since Windows 8 */
    result = NULL;
    ret = GetAddrInfoW(idn_punycode, NULL, NULL, &result);
    ok(!ret, "got %d expected success\n", ret);
    ok(result != NULL, "got %p\n", result);
    FreeAddrInfoW(result);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result = NULL;
    ret = GetAddrInfoW(idn_punycode, NULL, &hint, &result);
    ok(!ret, "got %d expected success\n", ret);
    ok(result != NULL, "got %p\n", result);

    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, NULL, &result2);
    if (broken(ret == WSAHOST_NOT_FOUND))
    {
        FreeAddrInfoW(result);
        win_skip("IDN resolution not supported in Win <= 7\n");
        return;
    }

    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);
    FreeAddrInfoW(result2);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);

    /* ensure manually resolved punycode and unicode hosts result in same data */
    compare_addrinfow(result, result2);

    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);
    FreeAddrInfoW(result2);

    /* Disable IDN resolution and test again*/
    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = AI_DISABLE_IDN_ENCODING;

    SetLastError(0xdeadbeef);
    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result2 == NULL, "got %p\n", result2);
}

static struct completion_routine_test
{
    WSAOVERLAPPED  *overlapped;
    DWORD           error;
    ADDRINFOEXW   **result;
    HANDLE          event;
    DWORD           called;
} completion_routine_test;

static void CALLBACK completion_routine(DWORD error, DWORD byte_count, WSAOVERLAPPED *overlapped)
{
    struct completion_routine_test *test = &completion_routine_test;

    ok(error == test->error, "got %u\n", error);
    ok(!byte_count, "got %u\n", byte_count);
    ok(overlapped == test->overlapped, "got %p\n", overlapped);
    ok(overlapped->Internal == test->error, "got %lu\n", overlapped->Internal);
    ok(overlapped->Pointer == test->result, "got %p\n", overlapped->Pointer);
    ok(overlapped->hEvent == NULL, "got %p\n", overlapped->hEvent);

    test->called++;
    SetEvent(test->event);
}

static void test_GetAddrInfoExW(void)
{
    static const WCHAR empty[] = {0};
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
    static const WCHAR winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR nxdomain[] = {'n','x','d','o','m','a','i','n','.','w','i','n','e','h','q','.','o','r','g',0};
    ADDRINFOEXW *result;
    OVERLAPPED overlapped;
    HANDLE event;
    int ret;

    if (!pGetAddrInfoExW || !pGetAddrInfoExOverlappedResult)
    {
        win_skip("GetAddrInfoExW and/or GetAddrInfoExOverlappedResult not present\n");
        return;
    }

    event = WSACreateEvent();

    result = (ADDRINFOEXW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(NULL, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(empty, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(!ret, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pFreeAddrInfoExW(result);

    result = NULL;
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(!ret, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoExW(result);

    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    overlapped.hEvent = event;
    ResetEvent(event);
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(!result, "result != NULL\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    pFreeAddrInfoExW(result);

    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(winehq, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == ERROR_IO_PENDING, "expected 11001, got %d\n", WSAGetLastError());
    ret = overlapped.Internal;
    ok(ret == WSAEINPROGRESS || ret == ERROR_SUCCESS, "overlapped.Internal = %u\n", ret);
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == event, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == ERROR_SUCCESS, "overlapped.Internal = %lx\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(result != NULL, "result == NULL\n");
    ok(!result->ai_blob, "ai_blob != NULL\n");
    ok(!result->ai_bloblen, "ai_bloblen != 0\n");
    ok(!result->ai_provider, "ai_provider = %s\n", wine_dbgstr_guid(result->ai_provider));
    pFreeAddrInfoExW(result);

    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    ret = pGetAddrInfoExW(NULL, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    todo_wine
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    todo_wine
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);
    ret = WaitForSingleObject(event, 0);
    todo_wine_if(ret != WAIT_TIMEOUT) /* Remove when abowe todo_wines are fixed */
    ok(ret == WAIT_TIMEOUT, "wait failed\n");

    /* event + completion routine */
    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    overlapped.hEvent = event;
    ResetEvent(event);
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, completion_routine, NULL);
    ok(ret == WSAEINVAL, "GetAddrInfoExW failed with %d\n", WSAGetLastError());

    /* completion routine, existing domain */
    result = (void *)0xdeadbeef;
    overlapped.hEvent = NULL;
    completion_routine_test.overlapped = &overlapped;
    completion_routine_test.error = ERROR_SUCCESS;
    completion_routine_test.result = &result;
    completion_routine_test.event = event;
    completion_routine_test.called = 0;
    ResetEvent(event);
    ret = pGetAddrInfoExW(winehq, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, completion_routine, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(!result, "result != NULL\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == NULL, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == ERROR_SUCCESS, "overlapped.Internal = %lx\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(completion_routine_test.called == 1, "got %u\n", completion_routine_test.called);
    pFreeAddrInfoExW(result);

    /* completion routine, non-existing domain */
    result = (void *)0xdeadbeef;
    completion_routine_test.overlapped = &overlapped;
    completion_routine_test.error = WSAHOST_NOT_FOUND;
    completion_routine_test.called = 0;
    ResetEvent(event);
    ret = pGetAddrInfoExW(nxdomain, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, completion_routine, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(!result, "result != NULL\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(ret == WSAHOST_NOT_FOUND, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == NULL, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == WSAHOST_NOT_FOUND, "overlapped.Internal = %lx\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(completion_routine_test.called == 1, "got %u\n", completion_routine_test.called);
    ok(result == NULL, "got %p\n", result);

    WSACloseEvent(event);
}

static void verify_ipv6_addrinfo(ADDRINFOA *result, const char *expect)
{
    SOCKADDR_IN6 *sockaddr6;
    char buffer[256];
    const char *ret;

    ok(result->ai_family == AF_INET6, "ai_family == %d\n", result->ai_family);
    ok(result->ai_addrlen >= sizeof(struct sockaddr_in6), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");

    sockaddr6 = (SOCKADDR_IN6 *)result->ai_addr;
    ok(sockaddr6->sin6_family == AF_INET6, "ai_addr->sin6_family == %d\n", sockaddr6->sin6_family);
    ok(sockaddr6->sin6_port == 0, "ai_addr->sin6_port == %d\n", sockaddr6->sin6_port);

    memset(buffer, 0, sizeof(buffer));
    ret = p_inet_ntop(AF_INET6, &sockaddr6->sin6_addr, buffer, sizeof(buffer));
    ok(ret != NULL, "inet_ntop failed (%d)\n", WSAGetLastError());
    ok(!strcmp(buffer, expect), "ai_addr->sin6_addr == '%s' (expected '%s')\n", buffer, expect);
}

static void compare_addrinfo(ADDRINFO *a, ADDRINFO *b)
{
    for (; a && b ; a = a->ai_next, b = b->ai_next)
    {
        ok(a->ai_flags == b->ai_flags,
           "Wrong flags %d != %d\n", a->ai_flags, b->ai_flags);
        ok(a->ai_family == b->ai_family,
           "Wrong family %d != %d\n", a->ai_family, b->ai_family);
        ok(a->ai_socktype == b->ai_socktype,
           "Wrong socktype %d != %d\n", a->ai_socktype, b->ai_socktype);
        ok(a->ai_protocol == b->ai_protocol,
           "Wrong protocol %d != %d\n", a->ai_protocol, b->ai_protocol);
        ok(a->ai_addrlen == b->ai_addrlen,
           "Wrong addrlen %lu != %lu\n", a->ai_addrlen, b->ai_addrlen);
        ok(!memcmp(a->ai_addr, b->ai_addr, min(a->ai_addrlen, b->ai_addrlen)),
           "Wrong address data\n");
        if (a->ai_canonname && b->ai_canonname)
        {
            ok(!strcmp(a->ai_canonname, b->ai_canonname), "Wrong canonical name '%s' != '%s'\n",
               a->ai_canonname, b->ai_canonname);
        }
        else
            ok(!a->ai_canonname && !b->ai_canonname, "Expected both names absent (%p != %p)\n",
               a->ai_canonname, b->ai_canonname);
    }
    ok(!a && !b, "Expected both addresses null (%p != %p)\n", a, b);
}

static void test_getaddrinfo(void)
{
    int i, ret;
    ADDRINFOA *result, *result2, *p, hint;
    SOCKADDR_IN *sockaddr;
    CHAR name[256], *ip;
    DWORD size = sizeof(name);

    memset(&hint, 0, sizeof(ADDRINFOA));
    GetComputerNameExA( ComputerNamePhysicalDnsHostname, name, &size );

    result = (ADDRINFOA *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo(NULL, NULL, NULL, &result);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo(NULL, "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");

    result2 = NULL;
    ret = getaddrinfo(NULL, "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "getaddrinfo failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("", "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");

    result2 = NULL;
    ret = getaddrinfo("", "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "getaddrinfo failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    result = NULL;
    ret = getaddrinfo("localhost", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", "", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", "80", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", NULL, &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("localhost", "80", &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    freeaddrinfo(result);

    hint.ai_flags = AI_NUMERICHOST;
    result = (void *)0xdeadbeef;
    ret = getaddrinfo("localhost", "80", &hint, &result);
    ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected WSAHOST_NOT_FOUND, got %d\n", WSAGetLastError());
    ok(!result, "result = %p\n", result);
    hint.ai_flags = 0;

    /* try to get information from the computer name, result is the same
     * as if requesting with an empty host name. */
    ret = getaddrinfo(name, NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = getaddrinfo("", NULL, NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    ret = getaddrinfo(name, "", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = getaddrinfo("", "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    result = (ADDRINFOA *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("nxdomain.codeweavers.com", NULL, NULL, &result);
    if (ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    /* Test IPv4 address conversion */
    result = NULL;
    ret = getaddrinfo("192.168.1.253", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", ret);
    ok(result->ai_family == AF_INET, "ai_family == %d\n", result->ai_family);
    ok(result->ai_addrlen >= sizeof(struct sockaddr_in), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");
    sockaddr = (SOCKADDR_IN *)result->ai_addr;
    ok(sockaddr->sin_family == AF_INET, "ai_addr->sin_family == %d\n", sockaddr->sin_family);
    ok(sockaddr->sin_port == 0, "ai_addr->sin_port == %d\n", sockaddr->sin_port);

    ip = inet_ntoa(sockaddr->sin_addr);
    ok(strcmp(ip, "192.168.1.253") == 0, "sockaddr->ai_addr == '%s'\n", ip);
    freeaddrinfo(result);

    /* Test IPv4 address conversion with port */
    result = NULL;
    hint.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo("192.168.1.253:1024", NULL, &hint, &result);
    hint.ai_flags = 0;
    ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo returned unexpected result: %d\n", ret);
    ok(result == NULL, "expected NULL, got %p\n", result);

    /* Test IPv6 address conversion */
    result = NULL;
    SetLastError(0xdeadbeef);
    ret = getaddrinfo("2a00:2039:dead:beef:cafe::6666", NULL, NULL, &result);

    if (result != NULL)
    {
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "2a00:2039:dead:beef:cafe::6666");
        freeaddrinfo(result);

        /* Test IPv6 address conversion with brackets */
        result = NULL;
        ret = getaddrinfo("[beef::cafe]", NULL, NULL, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        freeaddrinfo(result);

        /* Test IPv6 address conversion with brackets and hints */
        memset(&hint, 0, sizeof(ADDRINFOA));
        hint.ai_flags = AI_NUMERICHOST;
        hint.ai_family = AF_INET6;
        result = NULL;
        ret = getaddrinfo("[beef::cafe]", NULL, &hint, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        freeaddrinfo(result);

        memset(&hint, 0, sizeof(ADDRINFOA));
        hint.ai_flags = AI_NUMERICHOST;
        hint.ai_family = AF_INET;
        result = NULL;
        ret = getaddrinfo("[beef::cafe]", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);

        /* Test IPv6 address conversion with brackets and port */
        result = NULL;
        ret = getaddrinfo("[beef::cafe]:10239", NULL, NULL, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        freeaddrinfo(result);

        /* Test IPv6 address conversion with unmatched brackets */
        result = NULL;
        hint.ai_flags = AI_NUMERICHOST;
        ret = getaddrinfo("[beef::cafe", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);

        ret = getaddrinfo("beef::cafe]", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);
    }
    else
    {
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);
        win_skip("getaddrinfo does not support IPV6\n");
    }

    hint.ai_flags = 0;

    for (i = 0; i < ARRAY_SIZE(hinttests); i++)
    {
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        SetLastError(0xdeadbeef);
        ret = getaddrinfo("localhost", NULL, &hint, &result);
        todo_wine_if (hinttests[i].error) ok(ret == hinttests[i].error, "test %d: wrong ret %d\n", i, ret);
        if (!ret)
        {
            for (p = result; p; p = p->ai_next)
            {
                /* when AF_UNSPEC is used the return will be either AF_INET or AF_INET6 */
                if (hinttests[i].family == AF_UNSPEC)
                    ok(p->ai_family == AF_INET || p->ai_family == AF_INET6,
                       "test %d: expected AF_INET or AF_INET6, got %d\n",
                       i, p->ai_family);
                else
                    ok(p->ai_family == hinttests[i].family,
                       "test %d: expected family %d, got %d\n",
                       i, hinttests[i].family, p->ai_family);

                ok(p->ai_socktype == hinttests[i].socktype,
                   "test %d: expected type %d, got %d\n",
                   i, hinttests[i].socktype, p->ai_socktype);
                ok(p->ai_protocol == hinttests[i].protocol,
                   "test %d: expected protocol %d, got %d\n",
                   i, hinttests[i].protocol, p->ai_protocol);
            }
            freeaddrinfo(result);
        }
        else
        {
            ok(WSAGetLastError() == hinttests[i].error, "test %d: wrong error %d\n", i, WSAGetLastError());
        }
    }

    memset(&hint, 0, sizeof(hint));
    ret = getaddrinfo(NULL, "nonexistentservice", &hint, &result);
    ok(ret == WSATYPE_NOT_FOUND, "got %d\n", ret);
}

static void test_dns(void)
{
    struct hostent *h;
    union
    {
        char *chr;
        void *mem;
    } addr;
    char **ptr;
    int count;

    h = gethostbyname("");
    ok(h != NULL, "gethostbyname(\"\") failed with %d\n", h_errno);

    /* Use an address with valid alias names if possible */
    h = gethostbyname("source.winehq.org");
    if (!h)
    {
        skip("Can't test the hostent structure because gethostbyname failed\n");
        return;
    }

    /* The returned struct must be allocated in a very strict way. First we need to
     * count how many aliases there are because they must be located right after
     * the struct hostent size. Knowing the amount of aliases we know the exact
     * location of the first IP returned. Rule valid for >= XP, for older OS's
     * it's somewhat the opposite. */
    addr.mem = h + 1;
    if (h->h_addr_list == addr.mem) /* <= W2K */
    {
        win_skip("Skipping hostent tests since this OS is unsupported\n");
        return;
    }

    ok(h->h_aliases == addr.mem,
       "hostent->h_aliases should be in %p, it is in %p\n", addr.mem, h->h_aliases);

    for (ptr = h->h_aliases, count = 1; *ptr; ptr++) count++;
    addr.chr += sizeof(*ptr) * count;
    ok(h->h_addr_list == addr.mem,
       "hostent->h_addr_list should be in %p, it is in %p\n", addr.mem, h->h_addr_list);

    for (ptr = h->h_addr_list, count = 1; *ptr; ptr++) count++;

    addr.chr += sizeof(*ptr) * count;
    ok(h->h_addr_list[0] == addr.mem,
       "hostent->h_addr_list[0] should be in %p, it is in %p\n", addr.mem, h->h_addr_list[0]);
}

static void test_gethostbyname(void)
{
    struct hostent *he;
    struct in_addr **addr_list;
    char name[256], first_ip[16];
    int ret, i, count;
    MIB_IPFORWARDTABLE *routes = NULL;
    IP_ADAPTER_INFO *adapters = NULL, *k;
    DWORD adap_size = 0, route_size = 0;
    BOOL found_default = FALSE;
    BOOL local_ip = FALSE;

    ret = gethostname(name, sizeof(name));
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());

    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());
    addr_list = (struct in_addr **)he->h_addr_list;
    strcpy(first_ip, inet_ntoa(*addr_list[0]));

    if (winetest_debug > 1) trace("List of local IPs:\n");
    for (count = 0; addr_list[count] != NULL; count++)
    {
        char *ip = inet_ntoa(*addr_list[count]);
        if (!strcmp(ip, "127.0.0.1"))
            local_ip = TRUE;
        if (winetest_debug > 1) trace("%s\n", ip);
    }

    if (local_ip)
    {
        ok(count == 1, "expected 127.0.0.1 to be the only IP returned\n");
        skip("Only the loopback address is present, skipping tests\n");
        return;
    }

    ret = GetAdaptersInfo(NULL, &adap_size);
    ok(ret  == ERROR_BUFFER_OVERFLOW, "GetAdaptersInfo failed with a different error: %d\n", ret);
    ret = GetIpForwardTable(NULL, &route_size, FALSE);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "GetIpForwardTable failed with a different error: %d\n", ret);

    adapters = HeapAlloc(GetProcessHeap(), 0, adap_size);
    routes = HeapAlloc(GetProcessHeap(), 0, route_size);

    ret = GetAdaptersInfo(adapters, &adap_size);
    ok(ret  == NO_ERROR, "GetAdaptersInfo failed, error: %d\n", ret);
    ret = GetIpForwardTable(routes, &route_size, FALSE);
    ok(ret == NO_ERROR, "GetIpForwardTable failed, error: %d\n", ret);

    /* This test only has meaning if there is more than one IP configured */
    if (adapters->Next == NULL && count == 1)
    {
        skip("Only one IP is present, skipping tests\n");
        goto cleanup;
    }

    for (i = 0; !found_default && i < routes->dwNumEntries; i++)
    {
        /* default route (ip 0.0.0.0) ? */
        if (routes->table[i].dwForwardDest) continue;

        for (k = adapters; k != NULL; k = k->Next)
        {
            char *ip;

            if (k->Index != routes->table[i].dwForwardIfIndex) continue;

            /* the first IP returned from gethostbyname must be a default route */
            ip = k->IpAddressList.IpAddress.String;
            if (!strcmp(first_ip, ip))
            {
                found_default = TRUE;
                break;
            }
        }
    }
    ok(found_default, "failed to find the first IP from gethostbyname!\n");

cleanup:
    HeapFree(GetProcessHeap(), 0, adapters);
    HeapFree(GetProcessHeap(), 0, routes);
}

static void test_gethostbyname_hack(void)
{
    struct hostent *he;
    char name[256];
    static BYTE loopback[] = {127, 0, 0, 1};
    static BYTE magic_loopback[] = {127, 12, 34, 56};
    int ret;

    ret = gethostname(name, 256);
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());

    he = gethostbyname("localhost");
    ok(he != NULL, "gethostbyname(\"localhost\") failed: %d\n", h_errno);
    if (he->h_length != 4)
    {
        skip("h_length is %d, not IPv4, skipping test.\n", he->h_length);
        return;
    }
    ok(!memcmp(he->h_addr_list[0], loopback, he->h_length),
       "gethostbyname(\"localhost\") returned %u.%u.%u.%u\n",
       he->h_addr_list[0][0], he->h_addr_list[0][1], he->h_addr_list[0][2],
       he->h_addr_list[0][3]);

    if (!strcmp(name, "localhost"))
    {
        skip("hostname seems to be \"localhost\", skipping test.\n");
        return;
    }

    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, h_errno);
    if (he->h_length != 4)
    {
        skip("h_length is %d, not IPv4, skipping test.\n", he->h_length);
        return;
    }

    if (he->h_addr_list[0][0] == 127)
    {
        ok(memcmp(he->h_addr_list[0], magic_loopback, he->h_length) == 0,
           "gethostbyname(\"%s\") returned %u.%u.%u.%u not 127.12.34.56\n",
           name, he->h_addr_list[0][0], he->h_addr_list[0][1],
           he->h_addr_list[0][2], he->h_addr_list[0][3]);
    }

    gethostbyname("nonexistent.winehq.org");
    /* Don't check for the return value, as some braindead ISPs will kindly
     * resolve nonexistent host names to addresses of the ISP's spam pages. */
}

static void test_gethostname(void)
{
    struct hostent *he;
    char name[256];
    int ret, len;

    WSASetLastError(0xdeadbeef);
    ret = gethostname(NULL, 256);
    ok(ret == -1, "gethostname() returned %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "gethostname with null buffer "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    ret = gethostname(name, sizeof(name));
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());
    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());

    len = strlen(name);
    WSASetLastError(0xdeadbeef);
    strcpy(name, "deadbeef");
    ret = gethostname(name, len);
    ok(ret == -1, "gethostname() returned %d\n", ret);
    ok(!strcmp(name, "deadbeef"), "name changed unexpected!\n");
    ok(WSAGetLastError() == WSAEFAULT, "gethostname with insufficient length "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    len++;
    ret = gethostname(name, len);
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());
    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());
}

static void test_GetHostNameW(void)
{
    WCHAR name[256];
    int ret, len;

    if (!pGetHostNameW)
    {
        win_skip("GetHostNameW() not present\n");
        return;
    }

    WSASetLastError(0xdeadbeef);
    ret = pGetHostNameW(NULL, 256);
    ok(ret == -1, "GetHostNameW() returned %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "GetHostNameW with null buffer "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    ret = pGetHostNameW(name, sizeof(name));
    ok(ret == 0, "GetHostNameW() call failed: %d\n", WSAGetLastError());

    len = wcslen(name);
    WSASetLastError(0xdeadbeef);
    wcscpy(name, L"deadbeef");
    ret = pGetHostNameW(name, len);
    ok(ret == -1, "GetHostNameW() returned %d\n", ret);
    ok(!wcscmp(name, L"deadbeef"), "name changed unexpected!\n");
    ok(WSAGetLastError() == WSAEFAULT, "GetHostNameW with insufficient length "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    len++;
    ret = pGetHostNameW(name, len);
    ok(ret == 0, "GetHostNameW() call failed: %d\n", WSAGetLastError());
}

START_TEST( protocol )
{
    WSADATA data;
    WORD version = MAKEWORD( 2, 2 );

    pFreeAddrInfoExW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "FreeAddrInfoExW");
    pGetAddrInfoExOverlappedResult = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "GetAddrInfoExOverlappedResult");
    pGetAddrInfoExW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "GetAddrInfoExW");
    pGetHostNameW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "GetHostNameW");
    p_inet_ntop = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "inet_ntop");

    if (WSAStartup( version, &data )) return;

    test_WSAEnumProtocolsA();
    test_WSAEnumProtocolsW();
    test_getprotobyname();
    test_getprotobynumber();

    test_GetAddrInfoW();
    test_GetAddrInfoExW();
    test_getaddrinfo();

    test_dns();
    test_gethostbyname();
    test_gethostbyname_hack();
    test_gethostname();
    test_GetHostNameW();
}
