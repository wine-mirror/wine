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

static HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
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
            hr = IDirectPlay8Server_Close(server, 0);
            todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
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
