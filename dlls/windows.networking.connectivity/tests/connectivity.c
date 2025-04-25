/*
 * Copyright (C) 2024 Mohamad Al-Jaf
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
#include "initguid.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winstring.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Networking_Connectivity
#include "windows.networking.connectivity.h"

#include "netlistmgr.h"

#include "wine/test.h"

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    IUnknown_Release( unk );
}

static void test_NetworkInformationStatics(void)
{
    static const WCHAR *network_information_statics_name = L"Windows.Networking.Connectivity.NetworkInformation";
    INetworkInformationStatics *network_information_statics = (void *)0xdeadbeef;
    INetworkListManager *network_list_manager = (void *)0xdeadbeef;
    IConnectionProfile *connection_profile = (void *)0xdeadbeef;
    IConnectionProfile2 *connection_profile2;
    NetworkConnectivityLevel network_connectivity_level;
    IActivationFactory *factory = (void *)0xdeadbeef;
    NLM_CONNECTIVITY connectivity;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( network_information_statics_name, wcslen( network_information_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( network_information_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_INetworkInformationStatics, (void **)&network_information_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = CoCreateInstance( &CLSID_NetworkListManager, NULL, CLSCTX_INPROC_SERVER, &IID_INetworkListManager, (void **)&network_list_manager );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (FAILED(hr))
    {
        skip( "couldn't create an instance of NetworkListManager\n" );
        goto error;
    }

    hr = INetworkInformationStatics_GetInternetConnectionProfile( network_information_statics, &connection_profile );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    connectivity = 0xdeadbeef;
    hr = INetworkListManager_GetConnectivity( network_list_manager, &connectivity );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( connectivity != 0xdeadbeef, "failed to get connectivity information\n" );
    trace( "GetConnectivity: %08x\n", connectivity );
    if (connectivity == NLM_CONNECTIVITY_DISCONNECTED)
    {
        ok( !connection_profile, "expected NULL, got connection_profile %p.\n", connection_profile );
        skip( "Internet connection unavailable, skipping tests.\n" );
        goto cleanup;
    }

    hr = IConnectionProfile_QueryInterface( connection_profile, &IID_IConnectionProfile2, (void **)&connection_profile2 );

    if (hr == E_NOINTERFACE)
    {
        skip ( "IConnectionProfile2 not available, skipping those tests\n" );
    }
    else
    {
        IUnknown *unknown1 = 0, *unknown2 = 0;
        IInspectable *inspectable1 = 0, *inspectable2 = 0;
        IAgileObject *agile1 = 0, *agile2 = 0;
        IConnectionProfile *profile1 = 0;
        IConnectionProfile2 *profile2 = 0;

        hr = IUnknown_QueryInterface( connection_profile, &IID_IUnknown, (void **)&unknown1 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile, &IID_IInspectable, (void **)&inspectable1 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile, &IID_IAgileObject, (void **)&agile1 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile2, &IID_IUnknown, (void **)&unknown2 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile2, &IID_IInspectable, (void **)&inspectable2 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile2, &IID_IAgileObject, (void **)&agile2 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile2, &IID_IConnectionProfile, (void **)&profile1 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IUnknown_QueryInterface( connection_profile2, &IID_IConnectionProfile2, (void **)&profile2 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        ok( (void*)connection_profile != (void*)connection_profile2, "Interfaces should not be equal\n");

        ok( (void*)connection_profile == (void*)unknown1, "Interfaces not equal\n");
        ok( (void*)connection_profile == (void*)inspectable1, "Interfaces not equal\n");
        todo_wine ok( (void*)connection_profile != (void*)agile1, "Interfaces should not be equal\n");

        ok( (void*)unknown1 == (void*)unknown2, "Interfaces not equal\n");
        ok( (void*)inspectable1 == (void*)inspectable2, "Interfaces not equal\n");
        ok( (void*)agile1 == (void*)agile2, "Interfaces not equal\n");

        ok( (void*)connection_profile2 == (void*)profile2, "Interfaces not equal\n");
        ok( (void*)connection_profile == (void*)profile1, "Interfaces not equal\n");

        if (unknown1) IUnknown_Release( unknown1 );
        if (unknown2) IUnknown_Release( unknown2 );
        if (inspectable1) IInspectable_Release( inspectable1 );
        if (inspectable2) IInspectable_Release( inspectable2 );
        if (agile1) IAgileObject_Release( agile1 );
        if (agile2) IAgileObject_Release( agile2 );
        if (profile1) IConnectionProfile_Release( profile1 );
        if (profile2) IConnectionProfile2_Release( profile2 );
        IConnectionProfile2_Release( connection_profile2 );
    }

    hr = IConnectionProfile_GetNetworkConnectivityLevel( connection_profile, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    network_connectivity_level = 0xdeadbeef;
    hr = IConnectionProfile_GetNetworkConnectivityLevel( connection_profile, &network_connectivity_level );
    ok( hr == S_OK || hr == 0x8007023e /* Internet disconnect after GetInternetConnectionProfile */, "got hr %#lx.\n", hr );
    ok( network_connectivity_level != 0xdeadbeef, "failed to get network_connectivity_level\n" );

    connectivity = 0xdeadbeef;
    hr = INetworkListManager_GetConnectivity( network_list_manager, &connectivity );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( connectivity != 0xdeadbeef, "failed to get connectivity information\n" );
    trace( "GetConnectivity: %08x\n", connectivity );

    if (connectivity == NLM_CONNECTIVITY_DISCONNECTED)
        ok( network_connectivity_level == NetworkConnectivityLevel_None, "got network_connectivity_level %d.\n", network_connectivity_level );
    else if (connectivity & ( NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET ))
        ok( network_connectivity_level == NetworkConnectivityLevel_InternetAccess, "got network_connectivity_level %d.\n", network_connectivity_level );
    else if (connectivity & ( NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_LOCALNETWORK | NLM_CONNECTIVITY_IPV4_NOTRAFFIC | NLM_CONNECTIVITY_IPV6_NOTRAFFIC ))
        ok( network_connectivity_level == NetworkConnectivityLevel_LocalAccess, "got network_connectivity_level %d.\n", network_connectivity_level );
    else
        ok( network_connectivity_level == NetworkConnectivityLevel_ConstrainedInternetAccess, "got network_connectivity_level %d.\n", network_connectivity_level );

    ref = IConnectionProfile_Release( connection_profile );
    ok( ref == 0, "got ref %ld.\n", ref );
cleanup:
    INetworkListManager_Release( network_list_manager );
error:
    ref = INetworkInformationStatics_Release( network_information_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(connectivity)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_NetworkInformationStatics();

    RoUninitialize();
}
