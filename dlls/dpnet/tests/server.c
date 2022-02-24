/*
 * Copyright 2014 Alistair Leslie-Hughes
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

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>

#include <dplay8.h>
#define COBJMACROS
#include <netfw.h>
#include "wine/test.h"

#include "dpnet_test.h"

/* {CD0C3D4B-E15E-4CF2-9EA8-6E1D6548C5A5} */
static const GUID appguid = { 0xcd0c3d4b, 0xe15e, 0x4cf2, { 0x9e, 0xa8, 0x6e, 0x1d, 0x65, 0x48, 0xc5, 0xa5 } };
static WCHAR sessionname[] = L"winegamesserver";

static BOOL nCreatePlayer;
static BOOL nDestroyPlayer;

static HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
    trace("msgid: 0x%08lx\n", dwMessageId);

    switch(dwMessageId)
    {
        case DPN_MSGID_CREATE_PLAYER:
            nCreatePlayer = TRUE;
            break;
        case DPN_MSGID_DESTROY_PLAYER:
            nDestroyPlayer = TRUE;
            break;
    }

    return S_OK;
}

static void create_server(void)
{
    HRESULT hr;
    IDirectPlay8Server *server = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Server, NULL, CLSCTX_ALL, &IID_IDirectPlay8Server, (LPVOID*)&server);
    ok(hr == S_OK, "Failed to create IDirectPlay8Server object\n");
    if( SUCCEEDED(hr)  )
    {
        hr = IDirectPlay8Server_Close(server, 0);
        todo_wine ok(hr == DPNERR_UNINITIALIZED, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Server_Initialize(server, NULL, NULL, 0);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Server_Initialize(server, NULL, DirectPlayMessageHandler, 0);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        if(hr == S_OK)
        {
            IDirectPlay8Address *localaddr = NULL;
            DPN_APPLICATION_DESC appdesc;

            hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL,  CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
            ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");

            hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
            ok(hr == S_OK, "got 0x%08lx\n", hr);

            memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
            appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
            appdesc.dwFlags = DPNSESSION_CLIENT_SERVER;
            appdesc.guidApplication  = appguid;
            appdesc.pwszSessionName  = sessionname;

            hr = IDirectPlay8Server_Host(server, &appdesc, &localaddr, 1, NULL, NULL, NULL, 0);
            todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);

            todo_wine ok(nCreatePlayer, "No DPN_MSGID_CREATE_PLAYER Message\n");
            ok(!nDestroyPlayer, "Received DPN_MSGID_DESTROY_PLAYER Message\n");

            hr = IDirectPlay8Server_Close(server, 0);
            todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);

            todo_wine ok(nDestroyPlayer, "No DPN_MSGID_DESTROY_PLAYER Message\n");

            IDirectPlay8Address_Release(localaddr);
        }

        IDirectPlay8Server_Release(server);
    }
}

static void test_server_info(void)
{
    HRESULT hr;
    DPN_PLAYER_INFO info;
    WCHAR name[] = L"wine";
    WCHAR name2[] = L"wine2";
    WCHAR data[] = L"XXXX";
    IDirectPlay8Server *server = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Server, NULL, CLSCTX_ALL, &IID_IDirectPlay8Server, (LPVOID*)&server);
    ok(hr == S_OK, "Failed to create IDirectPlay8Server object\n");
    if( SUCCEEDED(hr)  )
    {
        ZeroMemory( &info, sizeof(DPN_PLAYER_INFO) );
        info.dwSize = sizeof(DPN_PLAYER_INFO);
        info.dwInfoFlags = DPNINFO_NAME;

        hr = IDirectPlay8Server_SetServerInfo(server, NULL, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == E_POINTER, "got %lx\n", hr);

        info.pwszName = name;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == DPNERR_UNINITIALIZED, "got %lx\n", hr);

        hr = IDirectPlay8Server_Initialize(server, NULL, DirectPlayMessageHandler, 0);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Server_SetServerInfo(server, NULL, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == E_POINTER, "got %lx\n", hr);

        info.pwszName = NULL;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        info.pwszName = name;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        info.dwInfoFlags = DPNINFO_NAME;
        info.pwszName = name2;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        info.dwInfoFlags = DPNINFO_DATA;
        info.pwszName = NULL;
        info.pvData = NULL;
        info.dwDataSize = sizeof(data);
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == E_POINTER, "got %lx\n", hr);

        info.dwInfoFlags = DPNINFO_DATA;
        info.pwszName = NULL;
        info.pvData = data;
        info.dwDataSize = 0;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        info.dwInfoFlags = DPNINFO_DATA;
        info.pwszName = NULL;
        info.pvData = data;
        info.dwDataSize = sizeof(data);
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
        info.pwszName = name;
        info.pvData = data;
        info.dwDataSize = sizeof(data);
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
        info.pwszName = name;
        info.pvData = NULL;
        info.dwDataSize = 0;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %lx\n", hr);

        IDirectPlay8Server_Release(server);
    }
}

static void test_enum_service_providers(void)
{
    DPN_SERVICE_PROVIDER_INFO *serv_prov_info = NULL;
    IDirectPlay8Server *server = NULL;
    DWORD items, size;
    DWORD i;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay8Server, NULL, CLSCTX_ALL, &IID_IDirectPlay8Server, (LPVOID*)&server);
    ok(hr == S_OK, "Failed to create IDirectPlay8Server object\n");
    if (FAILED(hr))
        return;

    size = 0;
    items = 0;
    hr = IDirectPlay8Server_EnumServiceProviders(server, NULL, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == DPNERR_UNINITIALIZED, "got %lx\n", hr);

    hr = IDirectPlay8Server_Initialize(server, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        IDirectPlay8Server_Release(server);
        return;
    }

    size = 0;
    items = 0;

    hr = IDirectPlay8Server_EnumServiceProviders(server, NULL, NULL, NULL, &size, NULL, 0);
    ok(hr == E_POINTER, "IDirectPlay8Server_EnumServiceProviders failed with %lx\n", hr);

    hr = IDirectPlay8Server_EnumServiceProviders(server, NULL, NULL, NULL, NULL, &items, 0);
    ok(hr == E_POINTER, "IDirectPlay8Server_EnumServiceProviders failed with %lx\n", hr);

    hr = IDirectPlay8Server_EnumServiceProviders(server, NULL, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Server_EnumServiceProviders failed with %lx\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Server_EnumServiceProviders(server, NULL, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Server_EnumServiceProviders failed with %lx\n", hr);
    ok(items != 0, "Found unexpectedly no service providers\n");

    trace("number of items found: %ld\n", items);

    for (i=0;i<items;i++)
    {
        trace("Found Service Provider: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    ok(HeapFree(GetProcessHeap(), 0, serv_prov_info), "Failed freeing server provider info\n");

    size = 0;
    items = 0;

    hr = IDirectPlay8Server_EnumServiceProviders(server, &CLSID_DP8SP_TCPIP, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Server_EnumServiceProviders failed with %lx\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Server_EnumServiceProviders(server, &CLSID_DP8SP_TCPIP, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Server_EnumServiceProviders failed with %lx\n", hr);
    ok(items != 0, "Found unexpectedly no adapter\n");


    for (i=0;i<items;i++)
    {
        trace("Found adapter: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found adapter guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    /* Invalid GUID */
    items = 88;
    hr = IDirectPlay8Server_EnumServiceProviders(server, &appguid, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == DPNERR_DOESNOTEXIST, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(items == 88, "Found adapter %ld\n", items);

    HeapFree(GetProcessHeap(), 0, serv_prov_info);
    IDirectPlay8Server_Release(server);
}

BOOL is_process_elevated(void)
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

BOOL is_firewall_enabled(void)
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    VARIANT_BOOL enabled = VARIANT_FALSE;

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %08lx\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

HRESULT set_firewall( enum firewall_op op )
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    INetFwAuthorizedApplication *app = NULL;
    INetFwAuthorizedApplications *apps = NULL;
    BSTR name, image = SysAllocStringLen( NULL, MAX_PATH );
    WCHAR path[MAX_PATH];

    if (!GetModuleFileNameW( NULL, image, MAX_PATH ))
    {
        SysFreeString( image );
        return E_FAIL;
    }

    if(!GetSystemDirectoryW(path, MAX_PATH))
    {
        SysFreeString( image );
        return E_FAIL;
    }
    lstrcatW(path, L"\\dpnsvr.exe");

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( L"dpnet_client" );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;
    if (hr != S_OK) goto done;

    INetFwAuthorizedApplication_Release( app );
    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    SysFreeString( image );
    image = SysAllocString( path );
    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( L"dpnet_server" );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );

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

/* taken from programs/winetest/main.c */
BOOL is_stub_dll(const char *filename)
{
    UINT size;
    DWORD ver;
    BOOL isstub = FALSE;
    char *p, *data;

    size = GetFileVersionInfoSizeA(filename, &ver);
    if (!size) return FALSE;

    data = HeapAlloc(GetProcessHeap(), 0, size);
    if (!data) return FALSE;

    if (GetFileVersionInfoA(filename, ver, size, data))
    {
        char buf[256];

        sprintf(buf, "\\StringFileInfo\\%04x%04x\\OriginalFilename", MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 1200);
        if (VerQueryValueA(data, buf, (void**)&p, &size))
            isstub = !lstrcmpiA("wcodstub.dll", p);
    }
    HeapFree(GetProcessHeap(), 0, data);

    return isstub;
}

START_TEST(server)
{
    HRESULT hr;
    BOOL firewall_enabled;
    char path[MAX_PATH];

    if(!GetSystemDirectoryA(path, MAX_PATH))
    {
        skip("Failed to get systems directory\n");
        return;
    }
    strcat(path, "\\dpnet.dll");

    if (!winetest_interactive && is_stub_dll(path))
    {
        win_skip("dpnet is a stub dll, skipping tests\n");
        return;
    }

    if ((firewall_enabled = is_firewall_enabled()) && !is_process_elevated())
    {
        skip("no privileges, skipping tests to avoid firewall dialog\n");
        return;
    }

    if (firewall_enabled)
    {
        HRESULT hr = set_firewall(APP_ADD);
        if (hr != S_OK)
        {
            skip("can't authorize app in firewall %08lx\n", hr);
            return;
        }
    }

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        goto done;

    create_server();
    test_server_info();
    test_enum_service_providers();

    CoUninitialize();

done:
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
