/*
 * Copyright 2011 Louis Lenders
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
#include <dplobby8.h>
#include <winver.h>
#define COBJMACROS
#include <netfw.h>
#include "wine/test.h"

static IDirectPlay8Peer* peer = NULL;
static IDirectPlay8Client* client = NULL;
static IDirectPlay8LobbiedApplication* lobbied = NULL;
static const GUID appguid = { 0xcd0c3d4b, 0xe15e, 0x4cf2, { 0x9e, 0xa8, 0x6e, 0x1d, 0x65, 0x48, 0xc5, 0xa5 } };

static HRESULT WINAPI DirectPlayMessageHandler(PVOID context, DWORD message_id, PVOID buffer)
{
    trace("DirectPlayMessageHandler: 0x%08x\n", message_id);
    return S_OK;
}

static HRESULT WINAPI DirectPlayLobbyMessageHandler(PVOID context, DWORD message_id, PVOID buffer)
{
    trace("DirectPlayLobbyMessageHandler: 0x%08x\n", message_id);
    return S_OK;
}

static HRESULT WINAPI DirectPlayLobbyClientMessageHandler(void *context, DWORD message_id, void* buffer)
{
    trace("DirectPlayLobbyClientMessageHandler: 0x%08x\n", message_id);
    return S_OK;
}

static BOOL test_init_dp(void)
{
    HRESULT hr;
    DPN_SP_CAPS caps;
    DPNHANDLE lobbyConnection;

    hr = CoInitialize(0);
    ok(hr == S_OK, "CoInitialize failed with %x\n", hr);

    hr = CoCreateInstance(&CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Client, (void **)&client);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%x\n", hr);

    memset(&caps, 0, sizeof(DPN_SP_CAPS));
    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_UNINITIALIZED, "GetSPCaps failed with %x\n", hr);

    hr = IDirectPlay8Client_Initialize(client, NULL, NULL, 0);
    ok(hr == DPNERR_INVALIDPARAM, "got %x\n", hr);

    hr = IDirectPlay8Client_Initialize(client, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "IDirectPlay8Client_Initialize failed with %x\n", hr);

    hr = CoCreateInstance(&CLSID_DirectPlay8LobbiedApplication, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDirectPlay8LobbiedApplication, (void **)&lobbied);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%x\n", hr);

    hr = IDirectPlay8LobbiedApplication_Initialize(lobbied, NULL, DirectPlayLobbyMessageHandler,
                                                &lobbyConnection, 0);
    ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Initialize failed with %x\n", hr);

    return client != NULL;
}

static void test_enum_service_providers(void)
{
    DPN_SERVICE_PROVIDER_INFO *serv_prov_info;
    DWORD items, size;
    DWORD i;
    HRESULT hr;

    size = 0;
    items = 0;

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, NULL, &size, NULL, 0);
    todo_wine ok(hr == E_POINTER, "IDirectPlay8Client_EnumServiceProviders failed with %x\n", hr);

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, NULL, NULL, &items, 0);
    todo_wine ok(hr == E_POINTER, "IDirectPlay8Client_EnumServiceProviders failed with %x\n", hr);

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, NULL, &size, &items, 0);
    todo_wine ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Client_EnumServiceProviders failed with %x\n", hr);
    todo_wine ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Client_EnumServiceProviders failed with %x\n", hr);
    todo_wine ok(items != 0, "Found unexpectedly no service providers\n");

    trace("number of items found: %d\n", items);

    for (i=0;i<items;i++)
    {
        trace("Found Service Provider: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    ok(HeapFree(GetProcessHeap(), 0, serv_prov_info), "Failed freeing server provider info\n");

    size = 0;
    items = 0;

    hr = IDirectPlay8Client_EnumServiceProviders(client, &CLSID_DP8SP_TCPIP, NULL, NULL, &size, &items, 0);
    todo_wine ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Client_EnumServiceProviders failed with %x\n", hr);
    todo_wine ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Client_EnumServiceProviders(client, &CLSID_DP8SP_TCPIP, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Client_EnumServiceProviders failed with %x\n", hr);
    todo_wine ok(items != 0, "Found unexpectedly no adapter\n");


    for (i=0;i<items;i++)
    {
        trace("Found adapter: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found adapter guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    ok(HeapFree(GetProcessHeap(), 0, serv_prov_info), "Failed freeing server provider info\n");
}

static void test_enum_hosts(void)
{
    HRESULT hr;
    IDirectPlay8Address *host = NULL;
    IDirectPlay8Address *local = NULL;
    DPN_APPLICATION_DESC appdesc;
    DPNHANDLE async = 0;
    static const WCHAR localhost[] = {'1','2','7','.','0','.','0','.','1',0};

    memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
    appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
    appdesc.guidApplication  = appguid;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&local);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08x\n", hr);

    hr = IDirectPlay8Address_SetSP(local, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08x\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&host);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08x\n", hr);

    hr = IDirectPlay8Address_SetSP(host, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08x\n", hr);

    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_HOSTNAME, localhost, sizeof(localhost),
                                                         DPNA_DATATYPE_STRING);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08x\n", hr);

    /* Since we are running asynchronously, EnumHosts returns DPNSUCCESS_PENDING. */
    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    todo_wine ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Client_EnumServiceProviders failed with 0x%08x\n", hr);
    todo_wine ok(async, "No Handle returned\n");

    hr = IDirectPlay8Client_CancelAsyncOperation(client, async, 0);
    ok(hr == S_OK, "IDirectPlay8Client_CancelAsyncOperation failed with 0x%08x\n", hr);

    IDirectPlay8Address_Release(local);
    IDirectPlay8Address_Release(host);
}

static void test_get_sp_caps(void)
{
    DPN_SP_CAPS caps;
    HRESULT hr;

    memset(&caps, 0, sizeof(DPN_SP_CAPS));

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_INVALIDPARAM, "GetSPCaps unexpectedly returned %x\n", hr);

    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "GetSPCaps failed with %x\n", hr);

    ok(caps.dwSize == sizeof(DPN_SP_CAPS), "got %d\n", caps.dwSize);
    ok((caps.dwFlags &
        (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS)) ==
       (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS),
       "unexpected flags %x\n", caps.dwFlags);
    ok(caps.dwNumThreads >= 3, "got %d\n", caps.dwNumThreads);
    ok(caps.dwDefaultEnumCount == 5, "expected 5, got %d\n", caps.dwDefaultEnumCount);
    ok(caps.dwDefaultEnumRetryInterval == 1500, "expected 1500, got %d\n", caps.dwDefaultEnumRetryInterval);
    ok(caps.dwDefaultEnumTimeout == 1500, "expected 1500, got %d\n", caps.dwDefaultEnumTimeout);
    ok(caps.dwMaxEnumPayloadSize == 983, "expected 983, got %d\n", caps.dwMaxEnumPayloadSize);
    ok(caps.dwBuffersPerThread == 1, "expected 1, got %d\n", caps.dwBuffersPerThread);
    ok(caps.dwSystemBufferSize == 0x10000 || broken(caps.dwSystemBufferSize == 0x2000 /* before Win8 */),
       "expected 0x10000, got 0x%x\n", caps.dwSystemBufferSize);
}

static void test_lobbyclient(void)
{
    HRESULT hr;
    IDirectPlay8LobbyClient *client = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8LobbyClient, NULL, CLSCTX_ALL, &IID_IDirectPlay8LobbyClient, (void**)&client);
    ok(hr == S_OK, "Failed to create object\n");
    if(SUCCEEDED(hr))
    {
        hr = IDirectPlay8LobbyClient_Initialize(client, NULL, NULL, 0);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);

        hr = IDirectPlay8LobbyClient_Initialize(client, NULL, DirectPlayLobbyClientMessageHandler, 0);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDirectPlay8LobbyClient_Close(client, 0);
        todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

        IDirectPlay8LobbyClient_Release(client);
    }
}

static void test_player_info(void)
{
    HRESULT hr;
    DPN_PLAYER_INFO info;
    WCHAR name[] = {'w','i','n','e',0};
    WCHAR name2[] = {'w','i','n','e','2',0};
    WCHAR data[] = {'X','X','X','X',0};

    ZeroMemory( &info, sizeof(DPN_PLAYER_INFO) );
    info.dwSize = sizeof(DPN_PLAYER_INFO);
    info.dwInfoFlags = DPNINFO_NAME;

    hr = IDirectPlay8Client_SetClientInfo(client, NULL, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == E_POINTER, "got %x\n", hr);

    info.pwszName = NULL;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.pwszName = name;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_NAME;
    info.pwszName = name2;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = NULL;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == E_POINTER, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = 0;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    /* Leave ClientInfo with only the name set. */
    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = NULL;
    info.dwDataSize = 0;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);
}

static void test_cleanup_dp(void)
{
    HRESULT hr;

    hr = IDirectPlay8Client_Close(client, 0);
    ok(hr == S_OK, "IDirectPlay8Client_Close failed with %x\n", hr);

    if(lobbied)
    {
        hr = IDirectPlay8LobbiedApplication_Close(lobbied, 0);
        ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Close failed with %x\n", hr);

        IDirectPlay8LobbiedApplication_Release(lobbied);
    }

    IDirectPlay8Client_Release(client);

    CoUninitialize();
}

static void test_close(void)
{
    HRESULT hr;
    static IDirectPlay8Client* client2;
    DPN_SP_CAPS caps;

    hr = CoCreateInstance(&CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Client, (void **)&client2);
    ok(hr == S_OK, "got 0x%x\n", hr);

    memset(&caps, 0, sizeof(DPN_SP_CAPS));
    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_Initialize(client2, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "got %x\n", hr);

    hr = IDirectPlay8Client_GetSPCaps(client2, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "got %x\n", hr);

    hr = IDirectPlay8Client_Close(client2, 0);
    ok(hr == DPN_OK, "got %x\n", hr);

    hr = IDirectPlay8Client_GetSPCaps(client2, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_UNINITIALIZED, "got %x\n", hr);

    IDirectPlay8Client_Release(client2);
}

static void test_init_dp_peer(void)
{
    HRESULT hr;
    DPN_SP_CAPS caps;
    DPNHANDLE lobbyConnection;

    hr = CoInitialize(0);
    ok(hr == S_OK, "CoInitialize failed with %x\n", hr);

    hr = CoCreateInstance(&CLSID_DirectPlay8Peer, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Peer, (void **)&peer);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%x\n", hr);

    memset(&caps, 0, sizeof(DPN_SP_CAPS));
    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_UNINITIALIZED, "GetSPCaps failed with %x\n", hr);

    hr = IDirectPlay8Peer_Initialize(peer, NULL, NULL, 0);
    ok(hr == DPNERR_INVALIDPARAM, "got %x\n", hr);

    hr = IDirectPlay8Peer_Initialize(peer, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_Initialize failed with %x\n", hr);

    hr = CoCreateInstance(&CLSID_DirectPlay8LobbiedApplication, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDirectPlay8LobbiedApplication, (void **)&lobbied);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%x\n", hr);

    hr = IDirectPlay8LobbiedApplication_Initialize(lobbied, NULL, NULL,
                                                &lobbyConnection, 0);
    ok(hr == DPNERR_INVALIDPOINTER, "Failed with %x\n", hr);

    hr = IDirectPlay8LobbiedApplication_Initialize(lobbied, NULL, DirectPlayLobbyMessageHandler,
                                                &lobbyConnection, 0);
    ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Initialize failed with %x\n", hr);
}

static void test_enum_service_providers_peer(void)
{
    DPN_SERVICE_PROVIDER_INFO *serv_prov_info;
    DWORD items, size;
    DWORD i;
    HRESULT hr;

    size = 0;
    items = 0;

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, NULL, &size, NULL, 0);
    ok(hr == E_POINTER, "IDirectPlay8Peer_EnumServiceProviders failed with %x\n", hr);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, NULL, NULL, &items, 0);
    ok(hr == E_POINTER, "IDirectPlay8Peer_EnumServiceProviders failed with %x\n", hr);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Peer_EnumServiceProviders failed with %x\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_EnumServiceProviders failed with %x\n", hr);
    ok(items != 0, "Found unexpectedly no service providers\n");

    trace("number of items found: %d\n", items);

    for (i=0;i<items;i++)
    {
        trace("Found Service Provider: %s\n", wine_dbgstr_w(serv_prov_info->pwszName));
        trace("Found guid: %s\n", wine_dbgstr_guid(&serv_prov_info->guid));

        serv_prov_info++;
    }

    serv_prov_info -= items; /* set pointer back */
    ok(HeapFree(GetProcessHeap(), 0, serv_prov_info), "Failed freeing server provider info\n");

    size = 0;
    items = 0;

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, &CLSID_DP8SP_TCPIP, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Peer_EnumServiceProviders failed with %x\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, &CLSID_DP8SP_TCPIP, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_EnumServiceProviders failed with %x\n", hr);
    ok(items != 0, "Found unexpectedly no adapter\n");


    for (i=0;i<items;i++)
    {
        trace("Found adapter: %s\n", wine_dbgstr_w(serv_prov_info->pwszName));
        trace("Found adapter guid: %s\n", wine_dbgstr_guid(&serv_prov_info->guid));

        serv_prov_info++;
    }

    serv_prov_info -= items; /* set pointer back */
    ok(HeapFree(GetProcessHeap(), 0, serv_prov_info), "Failed freeing server provider info\n");
}

static void test_enum_hosts_peer(void)
{
    HRESULT hr;
    IDirectPlay8Address *host = NULL;
    IDirectPlay8Address *local = NULL;
    DPN_APPLICATION_DESC appdesc;
    DPNHANDLE async = 0;
    static const WCHAR localhost[] = {'1','2','7','.','0','.','0','.','1',0};

    memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
    appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
    appdesc.guidApplication  = appguid;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&local);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08x\n", hr);

    hr = IDirectPlay8Address_SetSP(local, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08x\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&host);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08x\n", hr);

    hr = IDirectPlay8Address_SetSP(host, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08x\n", hr);

    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_HOSTNAME, localhost, sizeof(localhost),
                                                         DPNA_DATATYPE_STRING);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08x\n", hr);

    hr = IDirectPlay8Peer_EnumHosts(peer, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    todo_wine ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Peer_EnumServiceProviders failed with 0x%08x\n", hr);
    todo_wine ok(async, "No Handle returned\n");

    hr = IDirectPlay8Peer_CancelAsyncOperation(peer, async, 0);
    todo_wine ok(hr == S_OK, "IDirectPlay8Peer_CancelAsyncOperation failed with 0x%08x\n", hr);

    IDirectPlay8Address_Release(local);
    IDirectPlay8Address_Release(host);
}

static void test_get_sp_caps_peer(void)
{
    DPN_SP_CAPS caps;
    HRESULT hr;

    memset(&caps, 0, sizeof(DPN_SP_CAPS));

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_INVALIDPARAM, "GetSPCaps unexpectedly returned %x\n", hr);

    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "GetSPCaps failed with %x\n", hr);

    ok(caps.dwSize == sizeof(DPN_SP_CAPS), "got %d\n", caps.dwSize);
    ok((caps.dwFlags &
        (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS)) ==
       (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS),
       "unexpected flags %x\n", caps.dwFlags);
    ok(caps.dwNumThreads >= 3, "got %d\n", caps.dwNumThreads);
    ok(caps.dwDefaultEnumCount == 5, "expected 5, got %d\n", caps.dwDefaultEnumCount);
    ok(caps.dwDefaultEnumRetryInterval == 1500, "expected 1500, got %d\n", caps.dwDefaultEnumRetryInterval);
    ok(caps.dwDefaultEnumTimeout == 1500, "expected 1500, got %d\n", caps.dwDefaultEnumTimeout);
    ok(caps.dwMaxEnumPayloadSize == 983, "expected 983, got %d\n", caps.dwMaxEnumPayloadSize);
    ok(caps.dwBuffersPerThread == 1, "expected 1, got %d\n", caps.dwBuffersPerThread);
    ok(caps.dwSystemBufferSize == 0x10000 || broken(caps.dwSystemBufferSize == 0x2000 /* before Win8 */),
       "expected 0x10000, got 0x%x\n", caps.dwSystemBufferSize);
}

static void test_player_info_peer(void)
{
    HRESULT hr;
    DPN_PLAYER_INFO info;
    WCHAR name[] = {'w','i','n','e',0};
    WCHAR name2[] = {'w','i','n','e','2',0};
    WCHAR data[] = {'X','X','X','X',0};

    ZeroMemory( &info, sizeof(DPN_PLAYER_INFO) );
    info.dwSize = sizeof(DPN_PLAYER_INFO);
    info.dwInfoFlags = DPNINFO_NAME;

    hr = IDirectPlay8Peer_SetPeerInfo(peer, NULL, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == E_POINTER, "got %x\n", hr);

    info.pwszName = NULL;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.pwszName = name;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_NAME;
    info.pwszName = name2;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

if(0) /* Crashes on windows */
{
    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = NULL;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);
}

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = 0;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);

    /* Leave PeerInfo with only the name set. */
    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = NULL;
    info.dwDataSize = 0;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %x\n", hr);
}

static void test_cleanup_dp_peer(void)
{
    HRESULT hr;

    hr = IDirectPlay8Peer_Close(peer, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_Close failed with %x\n", hr);

    if(lobbied)
    {
        hr = IDirectPlay8LobbiedApplication_Close(lobbied, 0);
        ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Close failed with %x\n", hr);

        IDirectPlay8LobbiedApplication_Release(lobbied);
    }

    IDirectPlay8Peer_Release(peer);

    CoUninitialize();
}

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

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

static HRESULT set_firewall( enum firewall_op op )
{
    static const WCHAR testW[] = {'d','p','n','e','t','_','t','e','s','t',0};
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

    INetFwProfile_get_AuthorizedApplications( profile, &apps );
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

/* taken from programs/winetest/main.c */
static BOOL is_stub_dll(const char *filename)
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

START_TEST(client)
{
    BOOL firewall_enabled;

    if (!winetest_interactive &&
        (is_stub_dll("c:\\windows\\system32\\dpnet.dll") ||
         is_stub_dll("c:\\windows\\syswow64\\dpnet.dll")))
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

    if (!test_init_dp()) goto done;

    test_enum_service_providers();
    test_enum_hosts();
    test_get_sp_caps();
    test_player_info();
    test_lobbyclient();
    test_close();
    test_cleanup_dp();

    test_init_dp_peer();
    test_enum_service_providers_peer();
    test_enum_hosts_peer();
    test_get_sp_caps_peer();
    test_player_info_peer();
    test_cleanup_dp_peer();

done:
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
