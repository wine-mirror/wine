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

typedef struct IWSDiscoveryPublisherNotifyImpl {
    IWSDiscoveryPublisherNotify IWSDiscoveryPublisherNotify_iface;
    LONG                  ref;
} IWSDiscoveryPublisherNotifyImpl;

static inline IWSDiscoveryPublisherNotifyImpl *impl_from_IWSDiscoveryPublisherNotify(IWSDiscoveryPublisherNotify *iface)
{
    return CONTAINING_RECORD(iface, IWSDiscoveryPublisherNotifyImpl, IWSDiscoveryPublisherNotify_iface);
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_QueryInterface(IWSDiscoveryPublisherNotify *iface, REFIID riid, void **ppv)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);

    if (!ppv)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWSDiscoveryPublisherNotify))
    {
        *ppv = &This->IWSDiscoveryPublisherNotify_iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IWSDiscoveryPublisherNotifyImpl_AddRef(IWSDiscoveryPublisherNotify *iface)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    trace("IWSDiscoveryPublisherNotifyImpl_AddRef called (%p, ref = %d)\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDiscoveryPublisherNotifyImpl_Release(IWSDiscoveryPublisherNotify *iface)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    trace("IWSDiscoveryPublisherNotifyImpl_Release called (%p, ref = %d)\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_ProbeHandler(IWSDiscoveryPublisherNotify *This, const WSD_SOAP_MESSAGE *pSoap, IWSDMessageParameters *pMessageParameters)
{
    trace("IWSDiscoveryPublisherNotifyImpl_ProbeHandler called (%p, %p, %p)\n", This, pSoap, pMessageParameters);
    return S_OK;
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_ResolveHandler(IWSDiscoveryPublisherNotify *This, const WSD_SOAP_MESSAGE *pSoap, IWSDMessageParameters *pMessageParameters)
{
    trace("IWSDiscoveryPublisherNotifyImpl_ResolveHandler called (%p, %p, %p)\n", This, pSoap, pMessageParameters);
    return S_OK;
}

static const IWSDiscoveryPublisherNotifyVtbl publisherNotify_vtbl =
{
    IWSDiscoveryPublisherNotifyImpl_QueryInterface,
    IWSDiscoveryPublisherNotifyImpl_AddRef,
    IWSDiscoveryPublisherNotifyImpl_Release,
    IWSDiscoveryPublisherNotifyImpl_ProbeHandler,
    IWSDiscoveryPublisherNotifyImpl_ResolveHandler
};

static BOOL create_discovery_publisher_notify(IWSDiscoveryPublisherNotify **publisherNotify)
{
    IWSDiscoveryPublisherNotifyImpl *obj;

    *publisherNotify = NULL;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));

    if (!obj)
    {
        trace("Out of memory creating IWSDiscoveryPublisherNotify\n");
        return FALSE;
    }

    obj->IWSDiscoveryPublisherNotify_iface.lpVtbl = &publisherNotify_vtbl;
    obj->ref = 1;

    *publisherNotify = &obj->IWSDiscoveryPublisherNotify_iface;

    return TRUE;
}

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
    IWSDiscoveryPublisherNotify *sink1 = NULL, *sink2 = NULL;
    IWSDiscoveryPublisherNotifyImpl *sink1Impl = NULL, *sink2Impl = NULL;

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

    /* Create notification sinks */
    ok(create_discovery_publisher_notify(&sink1) == TRUE, "create_discovery_publisher_notify failed\n");
    ok(create_discovery_publisher_notify(&sink2) == TRUE, "create_discovery_publisher_notify failed\n");

    /* Get underlying implementation so we can check the ref count */
    sink1Impl = impl_from_IWSDiscoveryPublisherNotify(sink1);
    sink2Impl = impl_from_IWSDiscoveryPublisherNotify(sink2);

    /* Attempt to unregister sink before registering it */
    rc = IWSDiscoveryPublisher_UnRegisterNotificationSink(publisher, sink1);
    ok(rc == E_FAIL, "IWSDiscoveryPublisher_UnRegisterNotificationSink returned unexpected result: %08x\n", rc);

    /* Register notification sinks */
    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08x\n", rc);
    ok(sink1Impl->ref == 2, "Ref count for sink 1 is not as expected: %d\n", sink1Impl->ref);

    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink2);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08x\n", rc);
    ok(sink2Impl->ref == 2, "Ref count for sink 2 is not as expected: %d\n", sink2Impl->ref);

    /* Unregister the first sink */
    rc = IWSDiscoveryPublisher_UnRegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_UnRegisterNotificationSink failed: %08x\n", rc);
    ok(sink1Impl->ref == 1, "Ref count for sink 1 is not as expected: %d\n", sink1Impl->ref);

    /* TODO: Publish */

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);

    /* Check that the sinks have been released by the publisher */
    ok(sink1Impl->ref == 1, "Ref count for sink 1 is not as expected: %d\n", sink1Impl->ref);
    ok(sink2Impl->ref == 1, "Ref count for sink 2 is not as expected: %d\n", sink2Impl->ref);

    /* Release the sinks */
    IWSDiscoveryPublisherNotify_Release(sink1);
    IWSDiscoveryPublisherNotify_Release(sink2);
}

START_TEST(discovery)
{
    CoInitialize(NULL);

    CreateDiscoveryPublisher_tests();
    Publish_tests();

    CoUninitialize();
}
