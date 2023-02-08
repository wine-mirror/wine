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
#include "wine/test.h"

#include "dpnet_test.h"

static IDirectPlay8Server* server = NULL;
static IDirectPlay8Peer* peer = NULL;
static IDirectPlay8Client* client = NULL;
static IDirectPlay8LobbiedApplication* lobbied = NULL;
static const GUID appguid = { 0xcd0c3d4b, 0xe15e, 0x4cf2, { 0x9e, 0xa8, 0x6e, 0x1d, 0x65, 0x48, 0xc5, 0xa5 } };
static const WCHAR localhost[] = L"127.0.0.1";

static HRESULT   lastAsyncCode   = E_FAIL;
static DPNHANDLE lastAsyncHandle = 0xdeadbeef;
static HANDLE    enumevent       = NULL;
static int       handlecnt       = 0;

static HRESULT WINAPI DirectPlayServerHandler(void *context, DWORD message_id, void *buffer)
{
    switch(message_id)
    {
        case DPN_MSGID_CREATE_PLAYER:
        case DPN_MSGID_DESTROY_PLAYER:
            /* These are tested in the server test */
            break;
        default:
            trace("DirectPlayServerHandler: 0x%08lx\n", message_id);
    }
    return S_OK;
}

static HRESULT WINAPI DirectPlayMessageHandler(PVOID context, DWORD message_id, PVOID buffer)
{
    switch(message_id)
    {
        case DPN_MSGID_ENUM_HOSTS_RESPONSE:
            handlecnt++;
            if(handlecnt >= 2)
                SetEvent(enumevent);
            break;
        case DPN_MSGID_ASYNC_OP_COMPLETE:
        {
            DPNMSG_ASYNC_OP_COMPLETE *async_msg = (DPNMSG_ASYNC_OP_COMPLETE*)buffer;
            lastAsyncCode   = async_msg->hResultCode;
            lastAsyncHandle = async_msg->hAsyncOp;
            break;
        }
        default:
            trace("DirectPlayMessageHandler: 0x%08lx\n", message_id);
    }

    return S_OK;
}

static HRESULT WINAPI DirectPlayLobbyMessageHandler(PVOID context, DWORD message_id, PVOID buffer)
{
    trace("DirectPlayLobbyMessageHandler: 0x%08lx\n", message_id);
    return S_OK;
}

static HRESULT WINAPI DirectPlayLobbyClientMessageHandler(void *context, DWORD message_id, void* buffer)
{
    trace("DirectPlayLobbyClientMessageHandler: 0x%08lx\n", message_id);
    return S_OK;
}

static void create_server(void)
{
    static WCHAR sessionname[] = L"winegamesserver";
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;
    DPN_APPLICATION_DESC appdesc;

    hr = CoCreateInstance( &CLSID_DirectPlay8Server, NULL, CLSCTX_ALL, &IID_IDirectPlay8Server, (void **)&server);
    ok(hr == S_OK, "Failed to create IDirectPlay8Server object\n");

    hr = IDirectPlay8Server_Initialize(server, NULL, DirectPlayServerHandler, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL,  CLSCTX_ALL, &IID_IDirectPlay8Address, (void **)&localaddr);
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

    IDirectPlay8Address_Release(localaddr);
}

static BOOL test_init_dp(void)
{
    HRESULT hr;
    DPN_SP_CAPS caps;
    DPNHANDLE lobbyConnection;

    enumevent = CreateEventA( NULL, TRUE, FALSE, NULL);

    hr = CoCreateInstance(&CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Client, (void **)&client);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%lx\n", hr);

    memset(&caps, 0, sizeof(DPN_SP_CAPS));
    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_UNINITIALIZED, "GetSPCaps failed with %lx\n", hr);

    hr = IDirectPlay8Client_Initialize(client, NULL, NULL, 0);
    ok(hr == DPNERR_INVALIDPARAM, "got %lx\n", hr);

    hr = IDirectPlay8Client_SetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_INVALIDPARAM, "SetSPCaps failed with %lx\n", hr);

    hr = IDirectPlay8Client_Initialize(client, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "IDirectPlay8Client_Initialize failed with %lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectPlay8LobbiedApplication, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDirectPlay8LobbiedApplication, (void **)&lobbied);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%lx\n", hr);

    hr = IDirectPlay8LobbiedApplication_Initialize(lobbied, NULL, DirectPlayLobbyMessageHandler,
                                                &lobbyConnection, 0);
    ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Initialize failed with %lx\n", hr);

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
    ok(hr == E_POINTER, "IDirectPlay8Client_EnumServiceProviders failed with %lx\n", hr);

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, NULL, NULL, &items, 0);
    ok(hr == E_POINTER, "IDirectPlay8Client_EnumServiceProviders failed with %lx\n", hr);

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Client_EnumServiceProviders failed with %lx\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Client_EnumServiceProviders(client, NULL, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Client_EnumServiceProviders failed with %lx\n", hr);
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

    hr = IDirectPlay8Client_EnumServiceProviders(client, &CLSID_DP8SP_TCPIP, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Client_EnumServiceProviders failed with %lx\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Client_EnumServiceProviders(client, &CLSID_DP8SP_TCPIP, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Client_EnumServiceProviders failed with %lx\n", hr);
    ok(items != 0, "Found unexpectedly no adapter\n");


    for (i=0;i<items;i++)
    {
        trace("Found adapter: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found adapter guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    /* Invalid GUID */
    items = 88;
    hr = IDirectPlay8Client_EnumServiceProviders(client, &appguid, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == DPNERR_DOESNOTEXIST, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(items == 88, "Found adapter %ld\n", items);

    HeapFree(GetProcessHeap(), 0, serv_prov_info);
}

static void test_enum_hosts(void)
{
    HRESULT hr;
    IDirectPlay8Client *client2;
    IDirectPlay8Address *host = NULL;
    IDirectPlay8Address *local = NULL;
    DPN_APPLICATION_DESC appdesc;
    DPNHANDLE async = 0, async2 = 0;
    DPN_SP_CAPS caps;
    char *data;

    memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
    appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
    appdesc.guidApplication  = appguid;

    hr = CoCreateInstance(&CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Client, (void **)&client2);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&local);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(local, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&host);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(host, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_HOSTNAME, localhost, sizeof(localhost),
                                                         DPNA_DATATYPE_STRING);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08lx\n", hr);

    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "got %lx\n", hr);
    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, caps.dwMaxEnumPayloadSize + 1);

    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, NULL, 0, 2, 1000, 1000, NULL,  &async, DPNENUMHOSTS_SYNC);
    ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, data, caps.dwMaxEnumPayloadSize + 1,
                                        INFINITE, 0, INFINITE, NULL,  &async, DPNENUMHOSTS_SYNC);
    ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

    async = 0;
    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, data, caps.dwMaxEnumPayloadSize + 1, INFINITE, 0,
                                        INFINITE, NULL,  &async, 0);
    ok(hr == DPNERR_ENUMQUERYTOOLARGE, "got 0x%08lx\n", hr);
    ok(!async, "Handle returned\n");

    async = 0;
    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, data, caps.dwMaxEnumPayloadSize, INFINITE, 0, INFINITE,
                                        NULL,  &async, 0);
    ok(hr == DPNSUCCESS_PENDING, "got 0x%08lx\n", hr);
    todo_wine ok(async, "No Handle returned\n");

    /* This CancelAsyncOperation doesn't generate a DPN_MSGID_ASYNC_OP_COMPLETE */
    hr = IDirectPlay8Client_CancelAsyncOperation(client, async, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    HeapFree(GetProcessHeap(), 0, data);

    /* No Initialize has been called on client2. */
    hr = IDirectPlay8Client_EnumHosts(client2, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    ok(hr == DPNERR_UNINITIALIZED, "IDirectPlay8Client_EnumHosts failed with 0x%08lx\n", hr);

    /* Since we are running asynchronously, EnumHosts returns DPNSUCCESS_PENDING. */
    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Client_EnumHosts failed with 0x%08lx\n", hr);
    todo_wine ok(async, "No Handle returned\n");

    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async2, 0);
    ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Client_EnumHosts failed with 0x%08lx\n", hr);
    todo_wine ok(async2, "No Handle returned\n");
    todo_wine ok(async2 != async, "Same handle returned.\n");

    WaitForSingleObject(enumevent, 1000);

    lastAsyncCode = E_FAIL;
    lastAsyncHandle = 0xdeadbeef;
    hr = IDirectPlay8Client_CancelAsyncOperation(client, async, 0);
    ok(hr == S_OK, "IDirectPlay8Client_CancelAsyncOperation failed with 0x%08lx\n", hr);
    todo_wine ok(lastAsyncCode == DPNERR_USERCANCEL, "got 0x%08lx\n", lastAsyncCode);
    todo_wine ok(lastAsyncHandle == async, "got 0x%08lx\n", lastAsyncHandle);

    hr = IDirectPlay8Client_Initialize(client2, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "got %lx\n", hr);

    /* Show that handlers are per object. */
    hr = IDirectPlay8Client_CancelAsyncOperation(client2, async2, 0);
    todo_wine ok(hr == DPNERR_INVALIDHANDLE, "IDirectPlay8Client_CancelAsyncOperation failed with 0x%08lx\n", hr);

    lastAsyncCode = E_FAIL;
    lastAsyncHandle = 0xdeadbeef;
    hr = IDirectPlay8Client_CancelAsyncOperation(client, async2, 0);
    ok(hr == S_OK, "IDirectPlay8Client_CancelAsyncOperation failed with 0x%08lx\n", hr);
    flaky
    todo_wine ok(lastAsyncCode == DPNERR_USERCANCEL, "got 0x%08lx\n", lastAsyncCode);
    todo_wine ok(lastAsyncHandle == async2, "got 0x%08lx\n", lastAsyncHandle);

    IDirectPlay8Address_Release(local);
    IDirectPlay8Address_Release(host);
    IDirectPlay8Client_Release(client2);
}

static void test_enum_hosts_sync(void)
{
    HRESULT hr;
    IDirectPlay8Address *host = NULL;
    IDirectPlay8Address *local = NULL;
    DPN_APPLICATION_DESC appdesc;
    DWORD port = 5445, type = 0, size;

    memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
    appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
    appdesc.guidApplication  = appguid;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void**)&local);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(local, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void**)&host);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(host, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_HOSTNAME, localhost, sizeof(localhost),
                                                         DPNA_DATATYPE_STRING);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    handlecnt = 0;
    lastAsyncCode = 0xdeadbeef;
    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, NULL, 0, 3, 1500, 1000,
                                        NULL,  NULL, DPNENUMHOSTS_SYNC);
    ok(hr == DPN_OK, "got 0x%08lx\n", hr);
    ok(lastAsyncCode == 0xdeadbeef, "got 0x%08lx\n", lastAsyncCode);
    todo_wine ok(handlecnt == 2, "message handler not called\n");

    size = sizeof(port);
    hr = IDirectPlay8Address_GetComponentByName(host, DPNA_KEY_PORT, &port, &size, &type);
    ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

    size = sizeof(port);
    hr = IDirectPlay8Address_GetComponentByName(local, DPNA_KEY_PORT, &port, &size, &type);
    ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

    /* Try with specific port */
    port = 5445;
    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_PORT, &port, sizeof(port),
                                                         DPNA_DATATYPE_DWORD);

    handlecnt = 0;
    lastAsyncCode = 0xbeefdead;
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);
    hr = IDirectPlay8Client_EnumHosts(client, &appdesc, host, local, NULL, 0, 1, 1500, 1000,
                                        NULL,  NULL, DPNENUMHOSTS_SYNC);
    ok(hr == DPN_OK, "got 0x%08lx\n", hr);
    ok(lastAsyncCode == 0xbeefdead, "got 0x%08lx\n", lastAsyncCode);
    ok(handlecnt == 0, "message handler called\n");

    IDirectPlay8Address_Release(local);
    IDirectPlay8Address_Release(host);
}

static void test_get_sp_caps(void)
{
    DPN_SP_CAPS caps;
    HRESULT hr;

    memset(&caps, 0, sizeof(DPN_SP_CAPS));

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_INVALIDPARAM, "GetSPCaps unexpectedly returned %lx\n", hr);

    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "GetSPCaps failed with %lx\n", hr);

    ok(caps.dwSize == sizeof(DPN_SP_CAPS), "got %ld\n", caps.dwSize);
    ok((caps.dwFlags &
        (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS)) ==
       (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS),
       "unexpected flags %lx\n", caps.dwFlags);
    ok(caps.dwNumThreads >= 3, "got %ld\n", caps.dwNumThreads);
    ok(caps.dwDefaultEnumCount == 5, "expected 5, got %ld\n", caps.dwDefaultEnumCount);
    ok(caps.dwDefaultEnumRetryInterval == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumRetryInterval);
    ok(caps.dwDefaultEnumTimeout == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumTimeout);
    ok(caps.dwMaxEnumPayloadSize == 983, "expected 983, got %ld\n", caps.dwMaxEnumPayloadSize);
    ok(caps.dwBuffersPerThread == 1, "expected 1, got %ld\n", caps.dwBuffersPerThread);
    ok(caps.dwSystemBufferSize == 0x10000 || broken(caps.dwSystemBufferSize == 0x2000 /* before Win8 */),
       "expected 0x10000, got 0x%lx\n", caps.dwSystemBufferSize);

    caps.dwNumThreads = 2;
    caps.dwDefaultEnumCount = 3;
    caps.dwDefaultEnumRetryInterval = 1400;
    caps.dwDefaultEnumTimeout = 1400;
    caps.dwMaxEnumPayloadSize = 900;
    caps.dwBuffersPerThread = 2;
    caps.dwSystemBufferSize = 0x0ffff;
    hr = IDirectPlay8Client_SetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "SetSPCaps failed with %lx\n", hr);

    hr = IDirectPlay8Client_GetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "GetSPCaps failed with %lx\n", hr);

    ok(caps.dwSize == sizeof(DPN_SP_CAPS), "got %ld\n", caps.dwSize);
    ok(caps.dwNumThreads >= 3, "got %ld\n", caps.dwNumThreads);
    ok(caps.dwDefaultEnumCount == 5, "expected 5, got %ld\n", caps.dwDefaultEnumCount);
    ok(caps.dwDefaultEnumRetryInterval == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumRetryInterval);
    ok(caps.dwDefaultEnumTimeout == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumTimeout);
    ok(caps.dwMaxEnumPayloadSize == 983, "expected 983, got %ld\n", caps.dwMaxEnumPayloadSize);
    ok(caps.dwBuffersPerThread == 1, "expected 1, got %ld\n", caps.dwBuffersPerThread);
    ok(caps.dwSystemBufferSize == 0x0ffff, "expected 0x0ffff, got 0x%lx\n", caps.dwSystemBufferSize);

    /* Reset the System setting back to its default. */
    caps.dwSystemBufferSize = 0x10000;
    hr = IDirectPlay8Client_SetSPCaps(client, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "SetSPCaps failed with %lx\n", hr);
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
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        hr = IDirectPlay8LobbyClient_Initialize(client, NULL, DirectPlayLobbyClientMessageHandler, 0);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8LobbyClient_Close(client, 0);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);

        IDirectPlay8LobbyClient_Release(client);
    }
}

static void test_player_info(void)
{
    HRESULT hr;
    DPN_PLAYER_INFO info;
    WCHAR name[] = L"wine";
    WCHAR name2[] = L"wine2";
    WCHAR data[] = L"XXXX";

    ZeroMemory( &info, sizeof(DPN_PLAYER_INFO) );
    info.dwSize = sizeof(DPN_PLAYER_INFO);
    info.dwInfoFlags = DPNINFO_NAME;

    hr = IDirectPlay8Client_SetClientInfo(client, NULL, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == E_POINTER, "got %lx\n", hr);

    info.pwszName = NULL;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.pwszName = name;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_NAME;
    info.pwszName = name2;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = NULL;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == E_POINTER, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = 0;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    /* Leave ClientInfo with only the name set. */
    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = NULL;
    info.dwDataSize = 0;
    hr = IDirectPlay8Client_SetClientInfo(client, &info, NULL, NULL, DPNSETCLIENTINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);
}

static void test_cleanup_dp(void)
{
    HRESULT hr;

    hr = IDirectPlay8Client_Close(client, 0);
    ok(hr == S_OK, "IDirectPlay8Client_Close failed with %lx\n", hr);

    if(lobbied)
    {
        hr = IDirectPlay8LobbiedApplication_Close(lobbied, 0);
        ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Close failed with %lx\n", hr);

        IDirectPlay8LobbiedApplication_Release(lobbied);
    }

    IDirectPlay8Client_Release(client);
}

static void test_close(void)
{
    HRESULT hr;
    static IDirectPlay8Client* client2;
    DPN_SP_CAPS caps;

    hr = CoCreateInstance(&CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Client, (void **)&client2);
    ok(hr == S_OK, "got 0x%lx\n", hr);

    memset(&caps, 0, sizeof(DPN_SP_CAPS));
    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Client_Initialize(client2, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "got %lx\n", hr);

    hr = IDirectPlay8Client_GetSPCaps(client2, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "got %lx\n", hr);

    hr = IDirectPlay8Client_Close(client2, 0);
    ok(hr == DPN_OK, "got %lx\n", hr);

    hr = IDirectPlay8Client_GetSPCaps(client2, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_UNINITIALIZED, "got %lx\n", hr);

    IDirectPlay8Client_Release(client2);
}

static void test_init_dp_peer(void)
{
    HRESULT hr;
    DPN_SP_CAPS caps;
    DPNHANDLE lobbyConnection;

    hr = CoCreateInstance(&CLSID_DirectPlay8Peer, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Peer, (void **)&peer);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%lx\n", hr);

    memset(&caps, 0, sizeof(DPN_SP_CAPS));
    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Peer_SetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_INVALIDPARAM, "SetSPCaps failed with %lx\n", hr);

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_UNINITIALIZED, "GetSPCaps failed with %lx\n", hr);

    hr = IDirectPlay8Peer_Initialize(peer, NULL, NULL, 0);
    ok(hr == DPNERR_INVALIDPARAM, "got %lx\n", hr);

    hr = IDirectPlay8Peer_Initialize(peer, NULL, DirectPlayMessageHandler, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_Initialize failed with %lx\n", hr);

    hr = CoCreateInstance(&CLSID_DirectPlay8LobbiedApplication, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDirectPlay8LobbiedApplication, (void **)&lobbied);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%lx\n", hr);

    hr = IDirectPlay8LobbiedApplication_Initialize(lobbied, NULL, NULL,
                                                &lobbyConnection, 0);
    ok(hr == DPNERR_INVALIDPOINTER, "Failed with %lx\n", hr);

    hr = IDirectPlay8LobbiedApplication_Initialize(lobbied, NULL, DirectPlayLobbyMessageHandler,
                                                &lobbyConnection, 0);
    ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Initialize failed with %lx\n", hr);
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
    ok(hr == E_POINTER, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, NULL, NULL, &items, 0);
    ok(hr == E_POINTER, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, NULL, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(items != 0, "Found unexpectedly no service providers\n");

    trace("number of items found: %ld\n", items);

    for (i=0;i<items;i++)
    {
        trace("Found Service Provider: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    HeapFree(GetProcessHeap(), 0, serv_prov_info);

    size = 0;
    items = 0;

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, &CLSID_DP8SP_TCPIP, NULL, NULL, &size, &items, 0);
    ok(hr == DPNERR_BUFFERTOOSMALL, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(size != 0, "size is unexpectedly 0\n");

    serv_prov_info = HeapAlloc(GetProcessHeap(), 0, size);

    hr = IDirectPlay8Peer_EnumServiceProviders(peer, &CLSID_DP8SP_TCPIP, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(items != 0, "Found unexpectedly no adapter\n");


    for (i=0;i<items;i++)
    {
        trace("Found adapter: %s\n", wine_dbgstr_w(serv_prov_info[i].pwszName));
        trace("Found adapter guid: %s\n", wine_dbgstr_guid(&serv_prov_info[i].guid));
    }

    /* Invalid GUID */
    items = 88;
    hr = IDirectPlay8Peer_EnumServiceProviders(peer, &appguid, NULL, serv_prov_info, &size, &items, 0);
    ok(hr == DPNERR_DOESNOTEXIST, "IDirectPlay8Peer_EnumServiceProviders failed with %lx\n", hr);
    ok(items == 88, "Found adapter %ld\n", items);

    HeapFree(GetProcessHeap(), 0, serv_prov_info);
}

static void test_enum_hosts_peer(void)
{
    HRESULT hr;
    IDirectPlay8Peer *peer2;
    IDirectPlay8Address *host = NULL;
    IDirectPlay8Address *local = NULL;
    DPN_APPLICATION_DESC appdesc;
    DPNHANDLE async = 0, async2 = 0;

    ResetEvent(enumevent);
    handlecnt = 0;

    memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
    appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
    appdesc.guidApplication  = appguid;

    hr = CoCreateInstance(&CLSID_DirectPlay8Peer, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Peer, (void **)&peer2);
    ok(hr == S_OK, "CoCreateInstance failed with 0x%lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&local);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(local, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&host);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(host, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "IDirectPlay8Address_SetSP failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_HOSTNAME, localhost, sizeof(localhost),
                                                         DPNA_DATATYPE_STRING);
    ok(hr == S_OK, "IDirectPlay8Address failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Peer_EnumHosts(peer, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Peer_EnumServiceProviders failed with 0x%08lx\n", hr);
    todo_wine ok(async, "No Handle returned\n");

    hr = IDirectPlay8Peer_CancelAsyncOperation(peer, async, 0);
    todo_wine ok(hr == S_OK, "IDirectPlay8Peer_CancelAsyncOperation failed with 0x%08lx\n", hr);

    /* No Initialize has been called on peer2. */
    hr = IDirectPlay8Peer_EnumHosts(peer2, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    ok(hr == DPNERR_UNINITIALIZED, "IDirectPlay8Peer_EnumHosts failed with 0x%08lx\n", hr);

    /* Since we are running asynchronously, EnumHosts returns DPNSUCCESS_PENDING. */
    hr = IDirectPlay8Peer_EnumHosts(peer, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async, 0);
    ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Peer_EnumHosts failed with 0x%08lx\n", hr);
    todo_wine ok(async, "No Handle returned\n");

    hr = IDirectPlay8Peer_EnumHosts(peer, &appdesc, host, local, NULL, 0, INFINITE, 0, INFINITE, NULL,  &async2, 0);
    ok(hr == DPNSUCCESS_PENDING, "IDirectPlay8Peer_EnumHosts failed with 0x%08lx\n", hr);
    todo_wine ok(async2, "No Handle returned\n");
    todo_wine ok(async2 != async, "Same handle returned.\n");

    WaitForSingleObject(enumevent, 1000);

    lastAsyncCode = E_FAIL;
    lastAsyncHandle = 0xdeadbeef;
    hr = IDirectPlay8Peer_CancelAsyncOperation(peer, async, 0);
    todo_wine ok(hr == S_OK, "IDirectPlay8Peer_CancelAsyncOperation failed with 0x%08lx\n", hr);
    todo_wine ok(lastAsyncCode == DPNERR_USERCANCEL, "got 0x%08lx\n", lastAsyncCode);
    todo_wine ok(lastAsyncHandle == async, "got 0x%08lx\n", lastAsyncHandle);

    lastAsyncCode = E_FAIL;
    lastAsyncHandle = 0xdeadbeef;
    hr = IDirectPlay8Peer_CancelAsyncOperation(peer, async2, 0);
    todo_wine ok(hr == S_OK, "IDirectPlay8Peer_CancelAsyncOperation failed with 0x%08lx\n", hr);
    flaky
    todo_wine ok(lastAsyncCode == DPNERR_USERCANCEL, "got 0x%08lx\n", lastAsyncCode);
    flaky
    todo_wine ok(lastAsyncHandle == async2, "got 0x%08lx\n", lastAsyncHandle);

    IDirectPlay8Peer_Release(peer2);
    IDirectPlay8Address_Release(local);
    IDirectPlay8Address_Release(host);
}

static void test_enum_hosts_sync_peer(void)
{
    HRESULT hr;
    IDirectPlay8Address *host = NULL;
    IDirectPlay8Address *local = NULL;
    DPN_APPLICATION_DESC appdesc;
    DWORD port = 5445, type = 0, size;

    memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC) );
    appdesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
    appdesc.guidApplication  = appguid;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void**)&local);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(local, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void**)&host);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_SetSP(host, &CLSID_DP8SP_TCPIP);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_HOSTNAME, localhost, sizeof(localhost),
                                                         DPNA_DATATYPE_STRING);
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);

    handlecnt = 0;
    lastAsyncCode = 0xdeadbeef;
    hr = IDirectPlay8Peer_EnumHosts(peer, &appdesc, host, local, NULL, 0, 3, 1500, 1000,
                                        NULL,  NULL, DPNENUMHOSTS_SYNC);
    ok(hr == DPN_OK, "got 0x%08lx\n", hr);
    ok(lastAsyncCode == 0xdeadbeef, "got 0x%08lx\n", lastAsyncCode);
    todo_wine ok(handlecnt == 2, "wrong handle cnt\n");

    size = sizeof(port);
    hr = IDirectPlay8Address_GetComponentByName(host, DPNA_KEY_PORT, &port, &size, &type);
    ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

    size = sizeof(port);
    hr = IDirectPlay8Address_GetComponentByName(local, DPNA_KEY_PORT, &port, &size, &type);
    ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

    /* Try with specific port */
    port = 5445;
    hr = IDirectPlay8Address_AddComponent(host, DPNA_KEY_PORT, &port, sizeof(port),
                                                         DPNA_DATATYPE_DWORD);

    handlecnt = 0;
    lastAsyncCode = 0xbeefdead;
    ok(hr == S_OK, "Failed with 0x%08lx\n", hr);
    hr = IDirectPlay8Peer_EnumHosts(peer, &appdesc, host, local, NULL, 0, 1, 1500, 1000,
                                        NULL,  NULL, DPNENUMHOSTS_SYNC);
    ok(hr == DPN_OK, "got 0x%08lx\n", hr);
    ok(lastAsyncCode == 0xbeefdead, "got 0x%08lx\n", lastAsyncCode);
    ok(handlecnt == 0, "message handler called\n");

    IDirectPlay8Address_Release(local);
    IDirectPlay8Address_Release(host);
}

static void test_get_sp_caps_peer(void)
{
    DPN_SP_CAPS caps;
    HRESULT hr;

    memset(&caps, 0, sizeof(DPN_SP_CAPS));

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPNERR_INVALIDPARAM, "GetSPCaps unexpectedly returned %lx\n", hr);

    caps.dwSize = sizeof(DPN_SP_CAPS);

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "GetSPCaps failed with %lx\n", hr);

    ok(caps.dwSize == sizeof(DPN_SP_CAPS), "got %ld\n", caps.dwSize);
    ok((caps.dwFlags &
        (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS)) ==
       (DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST | DPNSPCAPS_SUPPORTSALLADAPTERS),
       "unexpected flags %lx\n", caps.dwFlags);
    ok(caps.dwNumThreads >= 3, "got %ld\n", caps.dwNumThreads);
    ok(caps.dwDefaultEnumCount == 5, "expected 5, got %ld\n", caps.dwDefaultEnumCount);
    ok(caps.dwDefaultEnumRetryInterval == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumRetryInterval);
    ok(caps.dwDefaultEnumTimeout == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumTimeout);
    ok(caps.dwMaxEnumPayloadSize == 983, "expected 983, got %ld\n", caps.dwMaxEnumPayloadSize);
    ok(caps.dwBuffersPerThread == 1, "expected 1, got %ld\n", caps.dwBuffersPerThread);
    ok(caps.dwSystemBufferSize == 0x10000 || broken(caps.dwSystemBufferSize == 0x2000 /* before Win8 */),
       "expected 0x10000, got 0x%lx\n", caps.dwSystemBufferSize);

    caps.dwNumThreads = 2;
    caps.dwDefaultEnumCount = 3;
    caps.dwDefaultEnumRetryInterval = 1400;
    caps.dwDefaultEnumTimeout = 1400;
    caps.dwMaxEnumPayloadSize = 900;
    caps.dwBuffersPerThread = 2;
    caps.dwSystemBufferSize = 0x0ffff;
    hr = IDirectPlay8Peer_SetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "SetSPCaps failed with %lx\n", hr);

    hr = IDirectPlay8Peer_GetSPCaps(peer, &CLSID_DP8SP_TCPIP, &caps, 0);
    ok(hr == DPN_OK, "GetSPCaps failed with %lx\n", hr);

    ok(caps.dwSize == sizeof(DPN_SP_CAPS), "got %ld\n", caps.dwSize);
    ok(caps.dwNumThreads >= 3, "got %ld\n", caps.dwNumThreads);
    ok(caps.dwDefaultEnumCount == 5, "expected 5, got %ld\n", caps.dwDefaultEnumCount);
    ok(caps.dwDefaultEnumRetryInterval == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumRetryInterval);
    ok(caps.dwDefaultEnumTimeout == 1500, "expected 1500, got %ld\n", caps.dwDefaultEnumTimeout);
    ok(caps.dwMaxEnumPayloadSize == 983, "expected 983, got %ld\n", caps.dwMaxEnumPayloadSize);
    ok(caps.dwBuffersPerThread == 1, "expected 1, got %ld\n", caps.dwBuffersPerThread);
    ok(caps.dwSystemBufferSize == 0x0ffff, "expected 0x0ffff, got 0x%lx\n", caps.dwSystemBufferSize);
}

static void test_player_info_peer(void)
{
    HRESULT hr;
    DPN_PLAYER_INFO info;
    WCHAR name[] = L"wine";
    WCHAR name2[] = L"wine2";
    WCHAR data[] = L"XXXX";

    ZeroMemory( &info, sizeof(DPN_PLAYER_INFO) );
    info.dwSize = sizeof(DPN_PLAYER_INFO);
    info.dwInfoFlags = DPNINFO_NAME;

    hr = IDirectPlay8Peer_SetPeerInfo(peer, NULL, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == E_POINTER, "got %lx\n", hr);

    info.pwszName = NULL;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.pwszName = name;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_NAME;
    info.pwszName = name2;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

if(0) /* Crashes on windows */
{
    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = NULL;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);
}

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = 0;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_DATA;
    info.pwszName = NULL;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = data;
    info.dwDataSize = sizeof(data);
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);

    /* Leave PeerInfo with only the name set. */
    info.dwInfoFlags = DPNINFO_DATA | DPNINFO_NAME;
    info.pwszName = name;
    info.pvData = NULL;
    info.dwDataSize = 0;
    hr = IDirectPlay8Peer_SetPeerInfo(peer, &info, NULL, NULL, DPNSETPEERINFO_SYNC);
    ok(hr == S_OK, "got %lx\n", hr);
}

static void test_cleanup_dp_peer(void)
{
    HRESULT hr;

    hr = IDirectPlay8Peer_Close(peer, 0);
    ok(hr == S_OK, "IDirectPlay8Peer_Close failed with %lx\n", hr);

    if(lobbied)
    {
        hr = IDirectPlay8LobbiedApplication_Close(lobbied, 0);
        ok(hr == S_OK, "IDirectPlay8LobbiedApplication_Close failed with %lx\n", hr);

        IDirectPlay8LobbiedApplication_Release(lobbied);
    }

    IDirectPlay8Peer_Release(peer);
}

START_TEST(client)
{
    BOOL firewall_enabled;
    HRESULT hr;
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
        hr = set_firewall(APP_ADD);
        if (hr != S_OK)
        {
            skip("can't authorize app in firewall %08lx\n", hr);
            return;
        }
    }

    hr = CoInitialize(0);
    ok(hr == S_OK, "CoInitialize failed with %lx\n", hr);

    if (!test_init_dp())
        goto done;

    create_server();

    test_enum_service_providers();
    test_enum_hosts();
    test_enum_hosts_sync();
    test_get_sp_caps();
    test_player_info();
    test_lobbyclient();
    test_close();
    test_cleanup_dp();

    test_init_dp_peer();
    test_enum_service_providers_peer();
    test_enum_hosts_peer();
    test_enum_hosts_sync_peer();
    test_get_sp_caps_peer();
    test_player_info_peer();
    test_cleanup_dp_peer();

    hr = IDirectPlay8Server_Close(server, 0);
    todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
    IDirectPlay8Server_Release(server);

    CloseHandle(enumevent);

done:
    if (firewall_enabled) set_firewall(APP_REMOVE);

    CoUninitialize();
}
