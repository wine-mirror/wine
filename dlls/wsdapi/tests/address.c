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
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateUDPAddress(NULL) failed: %08x\n", rc);

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUDPAddress(NULL, &udpAddress) failed: %08x\n", rc);
    ok(udpAddress != NULL, "WSDCreateUDPAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    /* Try to query for objects */
    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IWSDUdpAddress, (LPVOID*)&udpAddress2);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IWSDUdpAddress) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDUdpAddress_Release(udpAddress2);

    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IWSDTransportAddress, (LPVOID*)&transportAddress);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IID_WSDTransportAddress) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDTransportAddress_Release(transportAddress);

    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IWSDAddress, (LPVOID*)&address);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IWSDAddress) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDAddress_Release(address);

    rc = IWSDUdpAddress_QueryInterface(udpAddress, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK, "IWSDUdpAddress_QueryInterface(IID_IUnknown) failed: %08x\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    ref = IWSDUdpAddress_Release(udpAddress);
    ok(ref == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ref);
}

static void GetSetTransportAddress_udp_tests(void)
{
    IWSDUdpAddress *udpAddress = NULL;
    const WCHAR ipv4Address[] = {'1','0','.','2','0','.','3','0','.','4','0',0};
    const WCHAR ipv6Address[] = {'a','a','b','b',':','c','d',':',':','a','b','c',0};
    const WCHAR invalidAddress[] = {'n','o','t','/','v','a','l','i','d',0};
    LPCWSTR returnedAddress = NULL;
    WSADATA wsaData;
    HRESULT rc;
    int ret;

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed: %d\n", ret);

    rc = WSDCreateUdpAddress(&udpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08x\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    todo_wine ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08x\n", rc);
    ok(returnedAddress == NULL, "GetTransportAddress returned unexpected address: %08x\n", rc);

    /* Try setting a null address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, NULL);
    todo_wine ok(rc == E_INVALIDARG, "SetTransportAddress(NULL) returned unexpected result: %08x\n", rc);

    /* Try setting an invalid address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, invalidAddress);
    todo_wine ok(rc == HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND), "SetTransportAddress(invalidAddress) returned unexpected result: %08x\n", rc);

    /* Try setting an IPv4 address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, ipv4Address);
    todo_wine ok(rc == S_OK, "SetTransportAddress(ipv4Address) failed: %08x\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, NULL);
    todo_wine ok(rc == E_POINTER, "GetTransportAddress(NULL) returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    todo_wine ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08x\n", rc);
    todo_wine ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: '%s'\n", wine_dbgstr_w(returnedAddress));
    todo_wine ok(lstrcmpW(returnedAddress, ipv4Address) == 0, "Returned address != ipv4Address (%s)\n", wine_dbgstr_w(returnedAddress));

    /* Try setting an IPv6 address */
    rc = IWSDUdpAddress_SetTransportAddress(udpAddress, ipv6Address);
    todo_wine ok(rc == S_OK, "SetTransportAddress(ipv6Address) failed: %08x\n", rc);

    rc = IWSDUdpAddress_GetTransportAddress(udpAddress, &returnedAddress);
    todo_wine ok(rc == S_OK, "GetTransportAddress returned unexpected result: %08x\n", rc);
    todo_wine ok(returnedAddress != NULL, "GetTransportAddress returned unexpected address: '%s'\n", wine_dbgstr_w(returnedAddress));
    todo_wine ok(lstrcmpW(returnedAddress, ipv6Address) == 0, "Returned address != ipv6Address (%s)\n", wine_dbgstr_w(returnedAddress));

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
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08x\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    /* No test for GetPort(NULL) as this causes an access violation exception on Windows */

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    todo_wine ok(rc == S_OK, "GetPort returned unexpected result: %08x\n", rc);
    ok(actualPort == 0, "GetPort returned unexpected port: %d\n", actualPort);

    /* Try setting a zero port */
    rc = IWSDUdpAddress_SetPort(udpAddress, 0);
    todo_wine ok(rc == S_OK, "SetPort returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    todo_wine ok(rc == S_OK, "GetPort returned unexpected result: %08x\n", rc);
    ok(actualPort == 0, "GetPort returned unexpected port: %d\n", actualPort);

    /* Set a real port */
    rc = IWSDUdpAddress_SetPort(udpAddress, expectedPort1);
    todo_wine ok(rc == S_OK, "SetPort returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    todo_wine ok(rc == S_OK, "GetPort returned unexpected result: %08x\n", rc);
    todo_wine ok(actualPort == expectedPort1, "GetPort returned unexpected port: %d\n", actualPort);

    /* Now set a different port */
    rc = IWSDUdpAddress_SetPort(udpAddress, expectedPort2);
    todo_wine ok(rc == S_OK, "SetPort returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetPort(udpAddress, &actualPort);
    todo_wine ok(rc == S_OK, "GetPort returned unexpected result: %08x\n", rc);
    todo_wine ok(actualPort == expectedPort2, "GetPort returned unexpected port: %d\n", actualPort);

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
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &udpAddress) failed: %08x\n", rc);
    ok(udpAddress != NULL, "WSDCreateUdpAddress(NULL, &udpAddress) failed: udpAddress == NULL\n");

    rc = IWSDUdpAddress_GetMessageType(udpAddress, NULL);
    todo_wine ok(rc == E_POINTER, "GetMessageType returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetMessageType(udpAddress, &actualMessageType);
    todo_wine ok(rc == S_OK, "GetMessageType returned unexpected result: %08x\n", rc);
    ok(actualMessageType == 0, "GetMessageType returned unexpected message type: %d\n", actualMessageType);

    /* Try setting a message type */
    rc = IWSDUdpAddress_SetMessageType(udpAddress, expectedMessageType1);
    todo_wine ok(rc == S_OK, "SetMessageType returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetMessageType(udpAddress, &actualMessageType);
    todo_wine ok(rc == S_OK, "GetMessageType returned unexpected result: %08x\n", rc);
    todo_wine ok(actualMessageType == expectedMessageType1, "GetMessageType returned unexpected message type: %d\n", actualMessageType);

    /* Set another one */
    rc = IWSDUdpAddress_SetMessageType(udpAddress, expectedMessageType2);
    todo_wine ok(rc == S_OK, "SetMessageType returned unexpected result: %08x\n", rc);

    rc = IWSDUdpAddress_GetMessageType(udpAddress, &actualMessageType);
    todo_wine ok(rc == S_OK, "GetMessageType returned unexpected result: %08x\n", rc);
    ok(actualMessageType == expectedMessageType2, "GetMessageType returned unexpected message type: %d\n", actualMessageType);

    /* Release the object */
    ret = IWSDUdpAddress_Release(udpAddress);
    ok(ret == 0, "IWSDUdpAddress_Release() has %d references, should have 0\n", ret);
}

START_TEST(address)
{
    CoInitialize(NULL);

    CreateUdpAddress_tests();
    GetSetTransportAddress_udp_tests();
    GetSetPort_udp_tests();
    GetSetMessageType_udp_tests();

    CoUninitialize();
}
