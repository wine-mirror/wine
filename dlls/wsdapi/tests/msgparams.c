/*
 * Web Services on Devices
 * Message Parameters tests
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

static void CreateUdpMessageParameters_tests(void)
{
    IWSDUdpMessageParameters *udpMessageParams = NULL, *udpMessageParams2 = NULL;
    IWSDMessageParameters *messageParams = NULL;
    IUnknown *unknown;
    HRESULT rc;
    ULONG ref;

    rc = WSDCreateUdpMessageParameters(NULL);
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateUdpMessageParameters(NULL) failed: %08x\n", rc);

    rc = WSDCreateUdpMessageParameters(&udpMessageParams);
    ok(rc == S_OK, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: %08x\n", rc);
    ok(udpMessageParams != NULL, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: udpMessageParams == NULL\n");

    /* Try to query for objects */
    rc = IWSDUdpMessageParameters_QueryInterface(udpMessageParams, &IID_IWSDUdpMessageParameters, (LPVOID*)&udpMessageParams2);
    ok(rc == S_OK, "IWSDUdpMessageParams_QueryInterface(IID_IWSDUdpMessageParameters) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDUdpMessageParameters_Release(udpMessageParams2);

    rc = IWSDUdpMessageParameters_QueryInterface(udpMessageParams, &IID_IWSDMessageParameters, (LPVOID*)&messageParams);
    ok(rc == S_OK, "IWSDUdpMessageParams_QueryInterface(IID_WSDMessageParameters) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDMessageParameters_Release(messageParams);

    rc = IWSDUdpMessageParameters_QueryInterface(udpMessageParams, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK, "IWSDUdpMessageParams_QueryInterface(IID_IUnknown) failed: %08x\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    ref = IWSDUdpMessageParameters_Release(udpMessageParams);
    ok(ref == 0, "IWSDUdpMessageParameters_Release() has %d references, should have 0\n", ref);
}

static void LocalAddress_tests(void)
{
    IWSDUdpAddress *origUdpAddress = NULL;
    IWSDAddress *returnedAddress = NULL;
    IWSDUdpMessageParameters *udpMessageParams = NULL;
    WCHAR address[] = {'1','.','2','.','3','.','4',0};
    WSADATA wsaData;
    HRESULT rc;
    int ret;

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed: %d\n", ret);

    rc = WSDCreateUdpMessageParameters(&udpMessageParams);
    ok(rc == S_OK, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: %08x\n", rc);
    ok(udpMessageParams != NULL, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: udpMessageParams == NULL\n");

    rc = IWSDUdpMessageParameters_GetLocalAddress(udpMessageParams, NULL);
    todo_wine ok(rc == E_POINTER, "GetLocalAddress failed: %08x\n", rc);
    ok(returnedAddress == NULL, "GetLocalAddress returned %p\n", returnedAddress);

    rc = IWSDUdpMessageParameters_GetLocalAddress(udpMessageParams, &returnedAddress);
    todo_wine ok(rc == E_ABORT, "GetLocalAddress failed: %08x\n", rc);
    ok(returnedAddress == NULL, "GetLocalAddress returned %p\n", returnedAddress);

    rc = WSDCreateUdpAddress(&origUdpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &origUdpAddress) failed: %08x\n", rc);
    ok(origUdpAddress != NULL, "WSDCreateUdpMessageParameters(NULL, &origUdpAddress) failed: origUdpAddress == NULL\n");

    rc = IWSDUdpAddress_SetTransportAddress(origUdpAddress, address);
    todo_wine ok(rc == S_OK, "SetTransportAddress failed: %08x\n", rc);

    rc = IWSDUdpMessageParameters_SetLocalAddress(udpMessageParams, (IWSDAddress *)origUdpAddress);
    todo_wine ok(rc == S_OK, "SetLocalAddress failed: %08x\n", rc);

    rc = IWSDUdpMessageParameters_GetLocalAddress(udpMessageParams, &returnedAddress);
    todo_wine ok(rc == S_OK, "GetLocalAddress failed: %08x\n", rc);
    todo_wine ok(returnedAddress != NULL, "GetLocalAddress returned NULL\n");

    /* Check if GetLocalAddress returns the same object */
    todo_wine ok(returnedAddress == (IWSDAddress *)origUdpAddress, "returnedAddress != origUdpAddress\n");

    ret = IWSDUdpMessageParameters_Release(udpMessageParams);
    ok(ret == 0, "IWSDUdpMessageParameters_Release() has %d references, should have 0\n", ret);

    if (returnedAddress != NULL)
    {
        ret = IWSDAddress_Release(returnedAddress);
        ok(ret == 1, "IWSDAddress_Release() has %d references, should have 1\n", ret);
    }

    ret = IWSDUdpAddress_Release(origUdpAddress);
    ok(ret == 0, "IWSDUdpMessageParameters_Release() has %d references, should have 0\n", ret);

    ret = WSACleanup();
    ok(ret == 0, "WSACleanup failed: %d\n", ret);
}

START_TEST(msgparams)
{
    CoInitialize(NULL);

    CreateUdpMessageParameters_tests();
    LocalAddress_tests();

    CoUninitialize();
}
