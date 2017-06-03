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

START_TEST(address)
{
    CoInitialize(NULL);

    CreateUdpAddress_tests();

    CoUninitialize();
}
