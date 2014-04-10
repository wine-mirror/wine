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

/* {6733C6E8-A0D6-450E-8C18-CEACF331DC27} */
static const GUID IID_Random = {0x6733c6e8, 0xa0d6, 0x450e, { 0x8c, 0x18, 0xce, 0xac, 0xf3, 0x31, 0xdc, 0x27 } };

static void create_directplay_address(void)
{
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object");
    if(SUCCEEDED(hr))
    {
        GUID guidsp;

        hr = IDirectPlay8Address_GetSP(localaddr, NULL);
        ok(hr == DPNERR_INVALIDPOINTER, "GetSP failed 0x%08x\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08x\n", hr);

        hr = IDirectPlay8Address_SetSP(localaddr, &GUID_NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&guidsp, &GUID_NULL), "wrong guid: %s\n", wine_dbgstr_guid(&guidsp));

        hr = IDirectPlay8Address_SetSP(localaddr, &IID_Random);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&guidsp, &IID_Random), "wrong guid: %s\n", wine_dbgstr_guid(&guidsp));

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&guidsp, &CLSID_DP8SP_TCPIP), "wrong guid: %s\n", wine_dbgstr_guid(&guidsp));

        IDirectPlay8Address_Release(localaddr);
    }
}

START_TEST(address)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    create_directplay_address();

    CoUninitialize();
}
