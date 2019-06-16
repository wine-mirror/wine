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
static WCHAR sessionname[] = {'w','i','n','e','g','a','m','e','s','s','e','r','v','e','r',0};

static BOOL nCreatePlayer;
static BOOL nDestroyPlayer;

static HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
    trace("msgid: 0x%08x\n", dwMessageId);

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
        todo_wine ok(hr == DPNERR_UNINITIALIZED, "got 0x%08x\n", hr);

        hr = IDirectPlay8Server_Initialize(server, NULL, NULL, 0);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08x\n", hr);

        hr = IDirectPlay8Server_Initialize(server, NULL, DirectPlayMessageHandler, 0);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        if(hr == S_OK)
        {
            IDirectPlay8Address *localaddr = NULL;
            DPN_APPLICATION_DESC appdesc;

            hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL,  CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
            ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");

            hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
            appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
            appdesc.dwFlags = DPNSESSION_CLIENT_SERVER;
            appdesc.guidApplication  = appguid;
            appdesc.pwszSessionName  = sessionname;

            hr = IDirectPlay8Server_Host(server, &appdesc, &localaddr, 1, NULL, NULL, NULL, 0);
            todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

            todo_wine ok(nCreatePlayer, "No DPN_MSGID_CREATE_PLAYER Message\n");
            ok(!nDestroyPlayer, "Received DPN_MSGID_DESTROY_PLAYER Message\n");

            hr = IDirectPlay8Server_Close(server, 0);
            todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

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
    WCHAR name[] = {'w','i','n','e',0};
    WCHAR name2[] = {'w','i','n','e','2',0};
    WCHAR data[] = {'X','X','X','X',0};
    IDirectPlay8Server *server = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Server, NULL, CLSCTX_ALL, &IID_IDirectPlay8Server, (LPVOID*)&server);
    ok(hr == S_OK, "Failed to create IDirectPlay8Server object\n");
    if( SUCCEEDED(hr)  )
    {
        ZeroMemory( &info, sizeof(DPN_PLAYER_INFO) );
        info.dwSize = sizeof(DPN_PLAYER_INFO);
        info.dwInfoFlags = DPNINFO_NAME;

        hr = IDirectPlay8Server_SetServerInfo(server, NULL, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == E_POINTER, "got %x\n", hr);

        info.pwszName = name;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == DPNERR_UNINITIALIZED, "got %x\n", hr);

        hr = IDirectPlay8Server_Initialize(server, NULL, DirectPlayMessageHandler, 0);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDirectPlay8Server_SetServerInfo(server, NULL, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == E_POINTER, "got %x\n", hr);

        info.pwszName = NULL;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        info.pwszName = name;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        info.dwInfoFlags = DPNINFO_NAME;
        info.pwszName = name2;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        info.dwInfoFlags = DPNINFO_DATA;
        info.pwszName = NULL;
        info.pvData = NULL;
        info.dwDataSize = sizeof(data);
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == E_POINTER, "got %x\n", hr);

        info.dwInfoFlags = DPNINFO_DATA;
        info.pwszName = NULL;
        info.pvData = data;
        info.dwDataSize = 0;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        info.dwInfoFlags = DPNINFO_DATA;
        info.pwszName = NULL;
        info.pvData = data;
        info.dwDataSize = sizeof(data);
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
        info.pwszName = name;
        info.pvData = data;
        info.dwDataSize = sizeof(data);
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
        info.pwszName = name;
        info.pvData = NULL;
        info.dwDataSize = 0;
        hr = IDirectPlay8Server_SetServerInfo(server, &info, NULL, NULL, DPNSETSERVERINFO_SYNC);
        ok(hr == S_OK, "got %x\n", hr);

        IDirectPlay8Server_Release(server);
    }
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

HRESULT set_firewall( enum firewall_op op )
{
    static const WCHAR dpnsvrW[] =
        {'d','p','n','s','v','r','.','e','x','e',0};
    static const WCHAR separator[] = {'\\',0};
    static const WCHAR clientW[] =
        {'d','p','n','e','t','_','c','l','i','e','n','t',0};
    static const WCHAR serverW[] =
        {'d','p','n','e','t','_','s','e','r','v','e','r',0};
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
    lstrcatW(path, separator);
    lstrcatW(path, dpnsvrW);

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

    name = SysAllocString( clientW );
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
    if (hr != S_OK) goto done;

    INetFwAuthorizedApplication_Release( app );
    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    SysFreeString( image );
    image = SysAllocString( path );
    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( serverW );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08x\n", hr );
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
    DWORD size, ver;
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
            skip("can't authorize app in firewall %08x\n", hr);
            return;
        }
    }

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        goto done;

    create_server();
    test_server_info();

    CoUninitialize();

done:
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
