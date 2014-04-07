/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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

#include <stdio.h>
#include "windows.h"
#define COBJMACROS
#include "initguid.h"
#include "objbase.h"
#include "netlistmgr.h"
#include "wine/test.h"

static void test_INetworkListManager( void )
{
    INetworkListManager *mgr;
    NLM_CONNECTIVITY connectivity;
    VARIANT_BOOL connected;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_NetworkListManager, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetworkListManager, (void **)&mgr );
    if (hr != S_OK)
    {
        win_skip( "can't create instance of NetworkListManager\n" );
        return;
    }

    connectivity = 0xdeadbeef;
    hr = INetworkListManager_GetConnectivity( mgr, &connectivity );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( connectivity != 0xdeadbeef, "unchanged value\n" );
    trace( "GetConnectivity: %08x\n", connectivity );

    connected = 0xdead;
    hr = INetworkListManager_IsConnected( mgr, &connected );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( connected == VARIANT_TRUE || connected == VARIANT_FALSE, "expected boolean value\n" );

    connected = 0xdead;
    hr = INetworkListManager_IsConnectedToInternet( mgr, &connected );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( connected == VARIANT_TRUE || connected == VARIANT_FALSE, "expected boolean value\n" );

    INetworkListManager_Release( mgr );
}

START_TEST( list )
{
    CoInitialize( NULL );
    test_INetworkListManager();
    CoUninitialize();
}
