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
#include "wine/test.h"


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

START_TEST(client)
{
    if(!test_init_dp())
        return;

    test_enum_service_providers();
    test_enum_hosts();
    test_get_sp_caps();
    test_cleanup_dp();
}
