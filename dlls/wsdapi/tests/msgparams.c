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
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateUdpMessageParameters(NULL) failed: %08lx\n", rc);

    rc = WSDCreateUdpMessageParameters(&udpMessageParams);
    ok(rc == S_OK, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: %08lx\n", rc);
    ok(udpMessageParams != NULL, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: udpMessageParams == NULL\n");

    /* Try to query for objects */
    rc = IWSDUdpMessageParameters_QueryInterface(udpMessageParams, &IID_IWSDUdpMessageParameters, (LPVOID*)&udpMessageParams2);
    ok(rc == S_OK, "IWSDUdpMessageParams_QueryInterface(IID_IWSDUdpMessageParameters) failed: %08lx\n", rc);

    if (rc == S_OK)
        IWSDUdpMessageParameters_Release(udpMessageParams2);

    rc = IWSDUdpMessageParameters_QueryInterface(udpMessageParams, &IID_IWSDMessageParameters, (LPVOID*)&messageParams);
    ok(rc == S_OK, "IWSDUdpMessageParams_QueryInterface(IID_WSDMessageParameters) failed: %08lx\n", rc);

    if (rc == S_OK)
        IWSDMessageParameters_Release(messageParams);

    rc = IWSDUdpMessageParameters_QueryInterface(udpMessageParams, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK, "IWSDUdpMessageParams_QueryInterface(IID_IUnknown) failed: %08lx\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    ref = IWSDUdpMessageParameters_Release(udpMessageParams);
    ok(ref == 0, "IWSDUdpMessageParameters_Release() has %ld references, should have 0\n", ref);
}

static void LocalAddress_tests(void)
{
    IWSDUdpAddress *origUdpAddress = NULL;
    IWSDAddress *returnedAddress = NULL;
    IWSDUdpMessageParameters *udpMessageParams = NULL;
    WSADATA wsaData;
    HRESULT rc;
    int ret;

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed: %d\n", ret);

    rc = WSDCreateUdpMessageParameters(&udpMessageParams);
    ok(rc == S_OK, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: %08lx\n", rc);
    ok(udpMessageParams != NULL, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: udpMessageParams == NULL\n");

    rc = IWSDUdpMessageParameters_GetLocalAddress(udpMessageParams, NULL);
    ok(rc == E_POINTER, "GetLocalAddress failed: %08lx\n", rc);
    ok(returnedAddress == NULL, "GetLocalAddress returned %p\n", returnedAddress);

    rc = IWSDUdpMessageParameters_GetLocalAddress(udpMessageParams, &returnedAddress);
    ok(rc == E_ABORT, "GetLocalAddress failed: %08lx\n", rc);
    ok(returnedAddress == NULL, "GetLocalAddress returned %p\n", returnedAddress);

    rc = WSDCreateUdpAddress(&origUdpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &origUdpAddress) failed: %08lx\n", rc);
    ok(origUdpAddress != NULL, "WSDCreateUdpMessageParameters(NULL, &origUdpAddress) failed: origUdpAddress == NULL\n");

    rc = IWSDUdpAddress_SetTransportAddress(origUdpAddress, L"1.2.3.4");
    ok(rc == S_OK, "SetTransportAddress failed: %08lx\n", rc);

    rc = IWSDUdpMessageParameters_SetLocalAddress(udpMessageParams, (IWSDAddress *)origUdpAddress);
    ok(rc == S_OK, "SetLocalAddress failed: %08lx\n", rc);

    rc = IWSDUdpMessageParameters_GetLocalAddress(udpMessageParams, &returnedAddress);
    ok(rc == S_OK, "GetLocalAddress failed: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetLocalAddress returned NULL\n");

    /* Check if GetLocalAddress returns the same object */
    ok(returnedAddress == (IWSDAddress *)origUdpAddress, "returnedAddress != origUdpAddress\n");

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

static void RemoteAddress_tests(void)
{
    IWSDUdpAddress *origUdpAddress = NULL;
    IWSDAddress *returnedAddress = NULL;
    IWSDUdpMessageParameters *udpMessageParams = NULL;
    WSADATA wsaData;
    HRESULT rc;
    int ret;

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed: %d\n", ret);

    rc = WSDCreateUdpMessageParameters(&udpMessageParams);
    ok(rc == S_OK, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: %08lx\n", rc);
    ok(udpMessageParams != NULL, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: udpMessageParams == NULL\n");

    rc = IWSDUdpMessageParameters_GetRemoteAddress(udpMessageParams, NULL);
    ok(rc == E_POINTER, "GetRemoteAddress failed: %08lx\n", rc);
    ok(returnedAddress == NULL, "GetRemoteAddress returned %p\n", returnedAddress);

    rc = IWSDUdpMessageParameters_GetRemoteAddress(udpMessageParams, &returnedAddress);
    ok(rc == E_ABORT, "GetRemoteAddress failed: %08lx\n", rc);
    ok(returnedAddress == NULL, "GetRemoteAddress returned %p\n", returnedAddress);

    rc = WSDCreateUdpAddress(&origUdpAddress);
    ok(rc == S_OK, "WSDCreateUdpAddress(NULL, &origUdpAddress) failed: %08lx\n", rc);
    ok(origUdpAddress != NULL, "WSDCreateUdpMessageParameters(NULL, &origUdpAddress) failed: origUdpAddress == NULL\n");

    rc = IWSDUdpAddress_SetTransportAddress(origUdpAddress, L"1.2.3.4");
    ok(rc == S_OK, "SetTransportAddress failed: %08lx\n", rc);

    rc = IWSDUdpMessageParameters_SetRemoteAddress(udpMessageParams, (IWSDAddress *)origUdpAddress);
    ok(rc == S_OK, "SetRemoteAddress failed: %08lx\n", rc);

    rc = IWSDUdpMessageParameters_GetRemoteAddress(udpMessageParams, &returnedAddress);
    ok(rc == S_OK, "GetRemoteAddress failed: %08lx\n", rc);
    ok(returnedAddress != NULL, "GetLocalAddress returned NULL\n");

    /* Check if GetRemoteAddress returns the same object */
    ok(returnedAddress == (IWSDAddress *)origUdpAddress, "returnedAddress != origUdpAddress\n");

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

static void RetransmitParams_tests(void)
{
    WSDUdpRetransmitParams origParams, returnedParams;
    IWSDUdpMessageParameters *udpMessageParams = NULL;
    HRESULT rc;
    int ret;

    ZeroMemory(&origParams, sizeof(WSDUdpRetransmitParams));
    ZeroMemory(&returnedParams, sizeof(WSDUdpRetransmitParams));

    rc = WSDCreateUdpMessageParameters(&udpMessageParams);
    ok(rc == S_OK, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: %08lx\n", rc);
    ok(udpMessageParams != NULL, "WSDCreateUdpMessageParameters(NULL, &udpMessageParams) failed: udpMessageParams == NULL\n");

    rc = IWSDUdpMessageParameters_GetRetransmitParams(udpMessageParams, NULL);
    ok(rc == E_POINTER, "GetRetransmitParams returned unexpected result: %08lx\n", rc);

    /* Check if the default values are returned */
    rc = IWSDUdpMessageParameters_GetRetransmitParams(udpMessageParams, &returnedParams);
    ok(rc == S_OK, "GetRetransmitParams failed: %08lx\n", rc);

    ok(returnedParams.ulSendDelay == 0, "ulSendDelay = %ld\n", returnedParams.ulSendDelay);
    ok(returnedParams.ulRepeat == 1, "ulRepeat = %ld\n", returnedParams.ulRepeat);
    ok(returnedParams.ulRepeatMinDelay == 50, "ulRepeatMinDelay = %ld\n", returnedParams.ulRepeatMinDelay);
    ok(returnedParams.ulRepeatMaxDelay == 250, "ulRepeatMaxDelay = %ld\n", returnedParams.ulRepeatMaxDelay);
    ok(returnedParams.ulRepeatUpperDelay == 450, "ulRepeatUpperDelay = %ld\n", returnedParams.ulRepeatUpperDelay);

    /* Now try setting some custom parameters */
    origParams.ulSendDelay = 100;
    origParams.ulRepeat = 7;
    origParams.ulRepeatMinDelay = 125;
    origParams.ulRepeatMaxDelay = 175;
    origParams.ulRepeatUpperDelay = 500;

    rc = IWSDUdpMessageParameters_SetRetransmitParams(udpMessageParams, &origParams);
    ok(rc == S_OK, "SetRetransmitParams failed: %08lx\n", rc);

    ZeroMemory(&returnedParams, sizeof(WSDUdpRetransmitParams));

    rc = IWSDUdpMessageParameters_GetRetransmitParams(udpMessageParams, &returnedParams);
    ok(rc == S_OK, "GetRetransmitParams failed: %08lx\n", rc);

    ok(origParams.ulSendDelay == returnedParams.ulSendDelay, "ulSendDelay = %ld\n", returnedParams.ulSendDelay);
    ok(origParams.ulRepeat == returnedParams.ulRepeat, "ulRepeat = %ld\n", returnedParams.ulRepeat);
    ok(origParams.ulRepeatMinDelay == returnedParams.ulRepeatMinDelay, "ulRepeatMinDelay = %ld\n", returnedParams.ulRepeatMinDelay);
    ok(origParams.ulRepeatMaxDelay == returnedParams.ulRepeatMaxDelay, "ulRepeatMaxDelay = %ld\n", returnedParams.ulRepeatMaxDelay);
    ok(origParams.ulRepeatUpperDelay == returnedParams.ulRepeatUpperDelay, "ulRepeatUpperDelay = %ld\n", returnedParams.ulRepeatUpperDelay);

    /* Try setting a null parameter */
    rc = IWSDUdpMessageParameters_SetRetransmitParams(udpMessageParams, NULL);
    ok(rc == E_INVALIDARG, "SetRetransmitParams returned unexpected result: %08lx\n", rc);

    /* Now attempt to set some invalid parameters - these appear to be accepted */
    origParams.ulSendDelay = INFINITE;
    origParams.ulRepeat = 500;
    origParams.ulRepeatMinDelay = 250;
    origParams.ulRepeatMaxDelay = 125;
    origParams.ulRepeatUpperDelay = 100;

    rc = IWSDUdpMessageParameters_SetRetransmitParams(udpMessageParams, &origParams);
    ok(rc == S_OK, "SetRetransmitParams failed: %08lx\n", rc);

    ZeroMemory(&returnedParams, sizeof(WSDUdpRetransmitParams));

    rc = IWSDUdpMessageParameters_GetRetransmitParams(udpMessageParams, &returnedParams);
    ok(rc == S_OK, "GetRetransmitParams failed: %08lx\n", rc);

    ok(origParams.ulSendDelay == returnedParams.ulSendDelay, "ulSendDelay = %ld\n", returnedParams.ulSendDelay);
    ok(origParams.ulRepeat == returnedParams.ulRepeat, "ulRepeat = %ld\n", returnedParams.ulRepeat);
    ok(origParams.ulRepeatMinDelay == returnedParams.ulRepeatMinDelay, "ulRepeatMinDelay = %ld\n", returnedParams.ulRepeatMinDelay);
    ok(origParams.ulRepeatMaxDelay == returnedParams.ulRepeatMaxDelay, "ulRepeatMaxDelay = %ld\n", returnedParams.ulRepeatMaxDelay);
    ok(origParams.ulRepeatUpperDelay == returnedParams.ulRepeatUpperDelay, "ulRepeatUpperDelay = %ld\n", returnedParams.ulRepeatUpperDelay);

    ret = IWSDUdpMessageParameters_Release(udpMessageParams);
    ok(ret == 0, "IWSDUdpMessageParameters_Release() has %d references, should have 0\n", ret);
}

START_TEST(msgparams)
{
    CoInitialize(NULL);

    CreateUdpMessageParameters_tests();
    LocalAddress_tests();
    RemoteAddress_tests();
    RetransmitParams_tests();

    CoUninitialize();
}
