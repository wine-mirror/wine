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

START_TEST(msgparams)
{
    CoInitialize(NULL);

    CreateUdpMessageParameters_tests();

    CoUninitialize();
}
