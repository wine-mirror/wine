/*
 * Web Services on Devices
 * Discovery tests
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
#include "initguid.h"
#include "objbase.h"
#include "wsdapi.h"

static void CreateDiscoveryPublisher_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDiscoveryPublisher *publisher2;
    IUnknown *unknown;
    HRESULT rc;
    ULONG ref;

    rc = WSDCreateDiscoveryPublisher(NULL, NULL);
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateDiscoveryPublisher(NULL, NULL) failed: %08x\n", rc);

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    /* Try to query for objects */
    rc = IWSDiscoveryPublisher_QueryInterface(publisher, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK,"IWSDiscoveryPublisher_QueryInterface(IID_IUnknown) failed: %08x\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    rc = IWSDiscoveryPublisher_QueryInterface(publisher, &IID_IWSDiscoveryPublisher, (LPVOID*)&publisher2);
    ok(rc == S_OK,"IWSDiscoveryPublisher_QueryInterface(IID_IWSDiscoveryPublisher) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDiscoveryPublisher_Release(publisher2);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);
}

static void Publish_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    HRESULT rc;
    ULONG ref;

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    /* Test SetAddressFamily */
    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, 12345);
    ok(rc == E_INVALIDARG, "IWSDiscoveryPublisher_SetAddressFamily(12345) returned unexpected result: %08x\n", rc);

    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV4);
    ok(rc == S_OK, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV4) failed: %08x\n", rc);

    /* Try to update the address family after already setting it */
    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV6);
    ok(rc == STG_E_INVALIDFUNCTION, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV6) returned unexpected result: %08x\n", rc);

    /* TODO: Register notification sink */

    /* TODO: Publish */

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);
}

START_TEST(discovery)
{
    CoInitialize(NULL);

    CreateDiscoveryPublisher_tests();
    Publish_tests();

    CoUninitialize();
}
