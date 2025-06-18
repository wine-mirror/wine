/*
 * Web Services on Devices
 * Address tests
 *
 * Copyright 2017 Owen Rudge for CodeWeavers
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

#define COBJMACROS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "wine/test.h"
#include "objbase.h"
#include "wsdapi.h"

static void CreateUdpAddress_tests(void)
{
    IWSDUdpAddress *udpAddress = NULL, *udpAddress2 = NULL;
    IWSDTransportAddress *transportAddress = NULL;
    IWSDAddress *address = NULL;
    IUnknown *unknown;
    HRESULT rc;
    ULONG ref;

    rc = WSDCreateUdpAddress(NULL);
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateUDPAddress(NULL) failed: %08lx\n", rc);

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUDPAddress(NULL, &udpAddress) failed: %08lx\n", rc);
    ok(udpAddress != NULL, "WSDCreateUDPAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    /* Try to query for objects */
    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IWSDUdpAddress, (LPVOID*)&udpAddress2);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IWSDUdpAddress) failed: %08lx\n", rc);

    if (rc == S_OK)
        IWSDUdpAddress_Release(udpAddress2);

    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IWSDTransportAddress, (LPVOID*)&transportAddress);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IID_WSDTransportAddress) failed: %08lx\n", rc);

    if (rc == S_OK)
        IWSDTransportAddress_Release(transportAddress);

    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IWSDAddress, (LPVOID*)&address);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IWSDAddress) failed: %08lx\n", rc);

    if (rc == S_OK)
        IWSDAddress_Release(address);

    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IID_IUnknown) failed: %08lx\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    ref = IWSDUdpAddress_Release(udpAddress);
    ok(ref == 0, "IWSDUdpAddress_Release() has %ld references, should have 0\n", ref);
}

static void GetSetTransportAddress_udp_tests(void)
{
    IWSDUdpAddress *udpAddress = NULL;
    const WCHAR *ipv4Address = L"10.20.30.40";
    const WCHAR *ipv6Address = L"aabb:cd::abc";
    const WCHAR *ipv6AddressWithPort = L"[aabb:cd::abc:567]:124";
    LPCWSTR returnedAddress = NULL;
    WSADATA wsaData;
    HRESULT rc;
    int ret;

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed: %d\n", ret);

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08lx\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08lx\n", rc);
    ok(returnedAddress == NULL, "GetTransportAddress returned unexpected address: %08lx\n", rc);

    /* Try setting a null address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, NULL);
    ok(rc == E_INVALIDARG, "SetTransportAddress(NULL) returned unexpected result: %08lx\n", rc);

    /* Try setting an invalid address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, L"not/valid");
    ok(rc == HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND), "SetTransportAddress(invalidAddress) returned unexpected result: %08lx\n", rc);

    /* Try setting an IPv4 address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, ipv4Address);
    ok(rc == S_OK, "SetTransportAddress(ipv4Address) failed: %08lx\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, NULL);
    ok(rc == E_POINTER, "GetTransportAddress(NULL) returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: '%s'\n", wine_dbgstr_w(returnedAddress));
    ok(lstrcmpW(returnedAddress, ipv4Address) == 0, "Returned address != ipv4Address (%s)\n", wine_dbgstr_w(returnedAddress));

    /* Try setting an IPv4 address with a port number */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, L"10.20.30.40:124");
    ok(rc == HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND), "SetTransportAddress(ipv4Address) failed: %08lx\n", rc);

    /* Try setting an IPv6 address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, ipv6Address);
    ok(rc == S_OK, "SetTransportAddress(ipv6Address) failed: %08lx\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: '%s'\n", wine_dbgstr_w(returnedAddress));
    ok(lstrcmpW(returnedAddress, ipv6Address) == 0, "Returned address != ipv6Address (%s)\n", wine_dbgstr_w(returnedAddress));

    /* Try setting an IPv6 address with a port number */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, ipv6AddressWithPort);
    ok(rc == S_OK, "SetTransportAddress(ipv6AddressWithPort) failed: %08lx\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: '%s'\n", wine_dbgstr_w(returnedAddress));
    todo_wine ok(lstrcmpW(returnedAddress, ipv6AddressWithPort) == 0, "Returned address != ipv6AddressWithPort (%s)\n", wine_dbgstr_w(returnedAddress));

    /* Release the object */
    ret = IWSDUdpAddress_Release(udpAddress);
    ok(ret == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ret);

    ret = WSACleanup();
    ok(ret == 0, "WSACleanup failed: %d\n", ret);
}

static void GetSetPort_udp_tests(void)
{
    IWSDUdpAddress *udpAddress = NULL;
    WORD expectedPort1 = 12345;
    WORD expectedPort2 = 8080;
    WORD actualPort = 0;
    HRESULT rc;
    int ret;

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08lx\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    /* No test for GetPort(NULL) as this causes an access violation exception on Windows */

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    ok(rc == S_OK, "GetPort returned unexpected result: %08lx\n", rc);
    ok(actualPort == 0, "GetPort returned unexpected port: %d\n", actualPort);

    /* Try setting a zero port */
    rc = IWSDUdpAddress_SetPort(udpAddress, 0);
    ok(rc == S_OK, "SetPort returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    ok(rc == S_OK, "GetPort returned unexpected result: %08lx\n", rc);
    ok(actualPort == 0, "GetPort returned unexpected port: %d\n", actualPort);

    /* Set a real port */
    rc = IWSDUdpAddress_SetPort(udpAddress, expectedPort1);
    ok(rc == S_OK, "SetPort returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    ok(rc == S_OK, "GetPort returned unexpected result: %08lx\n", rc);
    ok(actualPort == expectedPort1, "GetPort returned unexpected port: %d\n", actualPort);

    /* Now set a different port */
    rc = IWSDUdpAddress_SetPort(udpAddress, expectedPort2);
    ok(rc == S_OK, "SetPort returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    ok(rc == S_OK, "GetPort returned unexpected result: %08lx\n", rc);
    ok(actualPort == expectedPort2, "GetPort returned unexpected port: %d\n", actualPort);

    /* Release the object */
    ret = IWSDUdpAddress_Release(udpAddress);
    ok(ret == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ret);
}

static void GetSetMessageType_udp_tests(void)
{
    WSDUdpMessageType expectedMessageType1 = TWO_WAY;
    WSDUdpMessageType expectedMessageType2 = ONE_WAY;
    WSDUdpMessageType actualMessageType = 0;
    IWSDUdpAddress *udpAddress = NULL;
    HRESULT rc;
    int ret;

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08lx\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    rc = IWSDUdpAddress_GetMessageType(udpAddress, NULL);
    ok(rc == E_POINTER, "GetMessageType returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetMessageType(udpAddress, &actualMessageType);
    ok(rc == S_OK, "GetMessageType returned unexpected result: %08lx\n", rc);
    ok(actualMessageType == 0, "GetMessageType returned unexpected message type: %d\n", actualMessageType);

    /* Try setting a message type */
    rc = IWSDUdpAddress_SetMessageType(udpAddress, expectedMessageType1);
    ok(rc == S_OK, "SetMessageType returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetMessageType(udpAddress, &actualMessageType);
    ok(rc == S_OK, "GetMessageType returned unexpected result: %08lx\n", rc);
    ok(actualMessageType == expectedMessageType1, "GetMessageType returned unexpected message type: %d\n", actualMessageType);

    /* Set another one */
    rc = IWSDUdpAddress_SetMessageType(udpAddress, expectedMessageType2);
    ok(rc == S_OK, "SetMessageType returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetMessageType(udpAddress, &actualMessageType);
    ok(rc == S_OK, "GetMessageType returned unexpected result: %08lx\n", rc);
    ok(actualMessageType == expectedMessageType2, "GetMessageType returned unexpected message type: %d\n", actualMessageType);

    /* Release the object */
    ret = IWSDUdpAddress_Release(udpAddress);
    ok(ret == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ret);
}

static void GetSetSockaddr_udp_tests(void)
{
    SOCKADDR_STORAGE storage1, storage2;
    SOCKADDR_STORAGE returnedStorage;
    struct sockaddr_in6 *sockAddr6Ptr;
    struct sockaddr_in *sockAddrPtr;
    IWSDUdpAddress *udpAddress = NULL;
    LPCWSTR returnedAddress = NULL;
    char addressBuffer[MAX_PATH];
    const char *cret;
    WSADATA wsaData;
    WORD port = 0;
    HRESULT rc;
    int ret;

    const char *ipv4Address = "1.2.3.4";
    const short ipv4Port = 1234;

    const char *ipv6Address = "2a00:1234:5678:dead:beef::aaaa";
    const short ipv6Port = 2345;
    const WCHAR *expectedIpv6TransportAddr = L"[2a00:1234:5678:dead:beef::aaaa]:2345";

    ZeroMemory(&storage1, sizeof(SOCKADDR_STORAGE));
    ZeroMemory(&storage2, sizeof(SOCKADDR_STORAGE));
    ZeroMemory(&returnedStorage, sizeof(SOCKADDR_STORAGE));

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed: %d\n", ret);

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08lx\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    rc = IWSDUdpAddress_GetSockaddr(udpAddress, NULL);
    ok(rc == E_POINTER, "GetSockaddr returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetSockaddr(udpAddress, &returnedStorage);
    ok(rc == E_FAIL, "GetSockaddr returned unexpected result: %08lx\n", rc);

    /* Try setting a transport address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, expectedIpv6TransportAddr);
    ok(rc == S_OK, "SetTransportAddress failed: %08lx\n", rc);

    /* A socket address should be returned */
    rc = IWSDUdpAddress_GetSockaddr(udpAddress, &returnedStorage);
    ok(rc == S_OK, "GetSockaddr returned unexpected result: %08lx\n", rc);
    ok(returnedStorage.ss_family == AF_INET6, "returnedStorage.ss_family != AF_INET6 (%d)\n", returnedStorage.ss_family);

    sockAddr6Ptr = (struct sockaddr_in6 *) &returnedStorage;

    /* Windows however doesn't set the port number */
    ok(sockAddr6Ptr->sin6_port == 0, "returnedStorage.sin6_port != 0 (%d)\n", sockAddr6Ptr->sin6_port);

    cret = inet_ntop(returnedStorage.ss_family, &sockAddr6Ptr->sin6_addr, addressBuffer, MAX_PATH);
    ok(cret != NULL, "inet_ntop failed (%d)\n", WSAGetLastError());
    ok(strcmp(addressBuffer, ipv6Address) == 0, "returnedStorage.sin6_addr != '%s' ('%s')\n", ipv6Address, addressBuffer);

    /* Release the object and create a new one */
    ret = IWSDUdpAddress_Release(udpAddress);
    ok(ret == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ret);

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08lx\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    /* Try setting an IPv4 address */
    sockAddrPtr = (struct sockaddr_in *) &storage1;
    sockAddrPtr->sin_family = AF_INET;
    sockAddrPtr->sin_port = htons(ipv4Port);

    ret = inet_pton(AF_INET, ipv4Address, &sockAddrPtr->sin_addr);
    ok(ret == 1, "inet_pton(ipv4) failed: %d\n", WSAGetLastError());

    rc = IWSDUdpAddress_SetSockaddr(udpAddress, &storage1);
    ok(rc == S_OK, "SetSockaddr returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetSockaddr(udpAddress, &returnedStorage);
    ok(rc == S_OK, "GetSockaddr returned unexpected result: %08lx\n", rc);

    ok(returnedStorage.ss_family == storage1.ss_family, "returnedStorage.ss_family != storage1.ss_family (%d)\n", returnedStorage.ss_family);
    ok(memcmp(&returnedStorage, &storage1, sizeof(struct sockaddr_in)) == 0, "returnedStorage != storage1\n");

    /* Check that GetTransportAddress returns the address set via the socket */
    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress failed: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: %p\n", returnedAddress);
    ok(lstrcmpW(returnedAddress, L"1.2.3.4:1234") == 0, "GetTransportAddress returned unexpected address: %s\n", wine_dbgstr_w(returnedAddress));

    /* Check that GetPort doesn't return the port set via the socket */
    rc = IWSDUdpAddress_GetPort(udpAddress, &port);
    ok(rc == S_OK, "GetPort returned unexpected result: %08lx\n", rc);
    ok(port == 0, "GetPort returned unexpected port: %d\n", port);

    /* Try setting an IPv4 address without a port */
    sockAddrPtr->sin_port = 0;

    rc = IWSDUdpAddress_SetSockaddr(udpAddress, &storage1);
    ok(rc == S_OK, "SetSockaddr returned unexpected result: %08lx\n", rc);

    /* Check that GetTransportAddress returns the address set via the socket */
    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress failed: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: %p\n", returnedAddress);
    ok(lstrcmpW(returnedAddress, L"1.2.3.4") == 0, "GetTransportAddress returned unexpected address: %s\n", wine_dbgstr_w(returnedAddress));

    /* Try setting an IPv6 address */
    sockAddr6Ptr = (struct sockaddr_in6 *) &storage2;
    sockAddr6Ptr->sin6_family = AF_INET6;
    sockAddr6Ptr->sin6_port = htons(ipv6Port);

    ret = inet_pton(AF_INET6, ipv6Address, &sockAddr6Ptr->sin6_addr);
    ok(ret == 1, "inet_pton(ipv6) failed: %d\n", WSAGetLastError());

    rc = IWSDUdpAddress_SetSockaddr(udpAddress, &storage2);
    ok(rc == S_OK, "SetSockaddr returned unexpected result: %08lx\n", rc);

    rc = IWSDUdpAddress_GetSockaddr(udpAddress, &returnedStorage);
    ok(rc == S_OK, "GetSockaddr returned unexpected result: %08lx\n", rc);

    ok(returnedStorage.ss_family == storage2.ss_family, "returnedStorage.ss_family != storage2.ss_family (%d)\n", returnedStorage.ss_family);
    ok(memcmp(&returnedStorage, &storage2, sizeof(struct sockaddr_in6)) == 0, "returnedStorage != storage2\n");

    /* Check that GetTransportAddress returns the address set via the socket */
    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress failed: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: %p\n", returnedAddress);
    ok(lstrcmpW(returnedAddress, expectedIpv6TransportAddr) == 0, "GetTransportAddress returned unexpected address: %s\n", wine_dbgstr_w(returnedAddress));

    /* Check that GetPort doesn't return the port set via the socket */
    rc = IWSDUdpAddress_GetPort(udpAddress, &port);
    ok(rc == S_OK, "GetPort returned unexpected result: %08lx\n", rc);
    ok(port == 0, "GetPort returned unexpected port: %d\n", port);

    /* Try setting an IPv6 address without a port */
    sockAddr6Ptr->sin6_port = 0;

    rc = IWSDUdpAddress_SetSockaddr(udpAddress, &storage2);
    ok(rc == S_OK, "SetSockaddr returned unexpected result: %08lx\n", rc);

    /* Check that GetTransportAddress returns the address set via the socket */
    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    ok(rc == S_OK, "GetTransportAddress failed: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: %p\n", returnedAddress);
    ok(lstrcmpW(returnedAddress, L"2a00:1234:5678:dead:beef::aaaa") == 0, "GetTransportAddress returned unexpected address: %s\n", wine_dbgstr_w(returnedAddress));

    rc = IWSDUdpAddress_SetSockaddr(udpAddress, &storage2);
    ok(rc == S_OK, "SetSockaddr returned unexpected result: %08lx\n", rc);

    /* Release the object */
    ret = IWSDUdpAddress_Release(udpAddress);
    ok(ret == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ret);

    ret = WSACleanup();
    ok(ret == 0, "WSACleanup failed: %d\n", ret);
}

START_TEST(address)
{
    CoInitialize(NULL);

    CreateUdpAddress_tests();
    GetSetTransportAddress_udp_tests();
    GetSetPort_udp_tests();
    GetSetMessageType_udp_tests();
    GetSetSockaddr_udp_tests();

    CoUninitialize();
}
