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
#include <netfw.h>

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

static void CreateDiscoveryPublisher_XMLContext_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDXMLContext *xmlContext, *returnedContext;
    HRESULT rc;
    int ref;

    /* Test creating an XML context and supplying it to WSDCreateDiscoveryPublisher */
    rc = WSDXMLCreateContext(&xmlContext);
    ok(rc == S_OK, "WSDXMLCreateContext failed: %08x\n", rc);

    rc = WSDCreateDiscoveryPublisher(xmlContext, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(xmlContext, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(xmlContext, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, NULL);
    ok(rc == E_INVALIDARG, "GetXMLContext returned unexpected value with NULL argument: %08x\n", rc);

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, &returnedContext);
    ok(rc == S_OK, "GetXMLContext failed: %08x\n", rc);

    ok(xmlContext == returnedContext, "GetXMLContext returned unexpected value: returnedContext == %p\n", returnedContext);

    ref = IWSDXMLContext_Release(returnedContext);
    ok(ref == 2, "IWSDXMLContext_Release() has %d references, should have 2\n", ref);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);

    ref = IWSDXMLContext_Release(returnedContext);
    ok(ref == 0, "IWSDXMLContext_Release() has %d references, should have 0\n", ref);

    /* Test using a default XML context */
    publisher = NULL;
    returnedContext = NULL;

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, &returnedContext);
    ok(rc == S_OK, "GetXMLContext failed: %08x\n", rc);

    ref = IWSDXMLContext_Release(returnedContext);
    ok(ref == 1, "IWSDXMLContext_Release() has %d references, should have 1\n", ref);

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

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static BOOL is_firewall_enabled(void)
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    VARIANT_BOOL enabled = VARIANT_FALSE;

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %08x\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

static HRESULT set_firewall( enum firewall_op op )
{
    static const WCHAR testW[] = {'w','s','d','a','p','i','_','t','e','s','t',0};
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    INetFwAuthorizedApplication *app = NULL;
    INetFwAuthorizedApplications *apps = NULL;
    BSTR name, image = SysAllocStringLen( NULL, MAX_PATH );

    if (!GetModuleFileNameW( NULL, image, MAX_PATH ))
    {
        SysFreeString( image );
        return E_FAIL;
    }
    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( testW );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;

done:
    if (app) INetFwAuthorizedApplication_Release( app );
    if (apps) INetFwAuthorizedApplications_Release( apps );
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    SysFreeString( image );
    return hr;
}

START_TEST(discovery)
{
    BOOL firewall_enabled = is_firewall_enabled();
    HRESULT hr;

    if (firewall_enabled)
    {
        if (!is_process_elevated())
        {
            skip("no privileges, skipping tests to avoid firewall dialog\n");
            return;
        }
        if ((hr = set_firewall(APP_ADD)) != S_OK)
        {
            skip("can't authorize app in firewall %08x\n", hr);
            return;
        }
    }

    CoInitialize(NULL);

    CreateDiscoveryPublisher_tests();
    CreateDiscoveryPublisher_XMLContext_tests();
    Publish_tests();

    CoUninitialize();
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
