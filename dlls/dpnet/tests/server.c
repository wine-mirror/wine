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
#include "wine/test.h"

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

START_TEST(server)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_server();

    CoUninitialize();
}
