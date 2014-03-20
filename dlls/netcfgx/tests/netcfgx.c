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

#define COBJMACROS

#include "netcfgx.h"
#include "wine/test.h"

void create_configuration(void)
{
    static const WCHAR tcpipW[] = {'M','S','_','T','C','P','I','P',0};
    HRESULT hr;
    INetCfg *config = NULL;
    INetCfgComponent *component = NULL;

    hr = CoCreateInstance( &CLSID_CNetCfg, NULL, CLSCTX_ALL, &IID_INetCfg, (LPVOID*)&config);
    ok(hr == S_OK, "Failed to create object\n");
    if(SUCCEEDED(hr))
    {
        hr = INetCfg_Initialize(config, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr =  INetCfg_FindComponent(config, tcpipW, &component);
        todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
        if(hr == S_OK)
        {
            INetCfgComponent_Release(component);
        }

        hr = INetCfg_Uninitialize(config);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        INetCfg_Release(config);
    }
}

START_TEST(netcfgx)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_configuration();

    CoUninitialize();
}
