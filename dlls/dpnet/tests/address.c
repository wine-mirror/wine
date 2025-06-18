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

#include "dpnet_test.h"

/* {6733C6E8-A0D6-450E-8C18-CEACF331DC27} */
static const GUID IID_Random = {0x6733c6e8, 0xa0d6, 0x450e, { 0x8c, 0x18, 0xce, 0xac, 0xf3, 0x31, 0xdc, 0x27 } };
static const WCHAR localhost[] = L"localhost";

static void create_directplay_address(void)
{
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");
    if(SUCCEEDED(hr))
    {
        GUID guidsp;

        hr = IDirectPlay8Address_GetSP(localaddr, NULL);
        ok(hr == DPNERR_INVALIDPOINTER, "GetSP failed 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_SetSP(localaddr, &GUID_NULL);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&guidsp, &GUID_NULL), "wrong guid: %s\n", wine_dbgstr_guid(&guidsp));

        hr = IDirectPlay8Address_SetSP(localaddr, &IID_Random);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&guidsp, &IID_Random), "wrong guid: %s\n", wine_dbgstr_guid(&guidsp));

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetSP(localaddr, &guidsp);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&guidsp, &CLSID_DP8SP_TCPIP), "wrong guid: %s\n", wine_dbgstr_guid(&guidsp));

        IDirectPlay8Address_Release(localaddr);
    }
}

static void address_addcomponents(void)
{
    static const char testing[] = "testing";
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");
    if(SUCCEEDED(hr))
    {
        GUID compguid;
        DWORD size, type;
        DWORD components;
        DWORD i;
        DWORD namelen = 0;
        DWORD bufflen = 0;
        DWORD port = 8888;
        WCHAR buffer[256];
        WCHAR *name = NULL;

        /* We can add any Component to the Address interface not just the predefined ones. */
        hr = IDirectPlay8Address_AddComponent(localaddr, L"unknown", &IID_Random, sizeof(GUID), DPNA_DATATYPE_GUID);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, L"unknown", &IID_Random, sizeof(GUID)+1, DPNA_DATATYPE_GUID);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhost, sizeof(localhost)+2, DPNA_DATATYPE_STRING);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhost, sizeof(localhost)/2, DPNA_DATATYPE_STRING);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, testing, sizeof(testing)+2, DPNA_DATATYPE_STRING_ANSI);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        /* Show that on error, nothing is added. */
        size = sizeof(buffer);
        hr = IDirectPlay8Address_GetComponentByName(localaddr, DPNA_KEY_HOSTNAME, buffer, &size, &type);
        ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, testing, sizeof(testing), DPNA_DATATYPE_STRING_ANSI);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_PORT, &port, sizeof(DWORD)+2, DPNA_DATATYPE_DWORD);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhost, sizeof(localhost), DPNA_DATATYPE_STRING);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        /* The information doesn't get removed when invalid parameters are used.*/
        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhost, sizeof(localhost)+2, DPNA_DATATYPE_STRING);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        size = 0;
        hr = IDirectPlay8Address_GetComponentByName(localaddr, DPNA_KEY_HOSTNAME, NULL, &size, &type);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);
        ok(size == sizeof(localhost), "Invalid string length: %ld\n", size);

        size = 1;
        hr = IDirectPlay8Address_GetComponentByName(localaddr, DPNA_KEY_HOSTNAME, NULL, &size, &type);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        size = sizeof(buffer);
        hr = IDirectPlay8Address_GetComponentByName(localaddr, DPNA_KEY_HOSTNAME, buffer, &size, &type);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(type == DPNA_DATATYPE_STRING, "incorrect type %ld\n", type);
        ok(!lstrcmpW(buffer, localhost), "Invalid string: %s\n", wine_dbgstr_w(buffer));

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_PORT, &port, sizeof(DWORD)+2, DPNA_DATATYPE_DWORD);
        ok(hr == DPNERR_INVALIDPARAM, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_PORT, &port, sizeof(DWORD), DPNA_DATATYPE_DWORD);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetComponentByName(localaddr, NULL, &compguid, &size, &type);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        size = sizeof(GUID)-1;
        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", NULL, &size, &type);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        size = sizeof(GUID);
        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", NULL, &size, &type);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", &compguid, NULL, &type);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        size = sizeof(GUID)-1;
        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", &compguid, &size, NULL);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        size = sizeof(GUID);
        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", &compguid, &size, NULL);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);

        size = sizeof(GUID)-1;
        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", &compguid, &size, &type);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);
        ok(size == sizeof(GUID), "got %ld\n", size);

        size = sizeof(GUID);
        hr = IDirectPlay8Address_GetComponentByName(localaddr, L"unknown", &compguid, &size, &type);
        ok(IsEqualGUID(&compguid, &IID_Random), "incorrect guid\n");
        ok(size == sizeof(GUID), "incorrect size got %ld\n", size);
        ok(type == DPNA_DATATYPE_GUID, "incorrect type\n");
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetNumComponents(localaddr, NULL);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetNumComponents(localaddr, &components);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 100, NULL, &namelen, NULL, &bufflen, &type);
        ok(hr == DPNERR_DOESNOTEXIST, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 1, NULL, &namelen, NULL, &bufflen, NULL);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        bufflen = 100;
        namelen = 0;
        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 1, name, &namelen, buffer, &bufflen, &type);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        namelen = 100;
        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 1, NULL, &namelen, NULL, &bufflen, &type);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 100, NULL, NULL, NULL, &bufflen, &type);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 100, NULL, &namelen, NULL, NULL, &type);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        bufflen = 0;
        namelen = 0;
        type = 0;
        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 0, NULL, &namelen, NULL, &bufflen, &type);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);
        ok(namelen == 8, "namelen expected 8 got %ld\n", namelen);
        ok(bufflen == 16, "bufflen expected 16 got %ld\n", bufflen);
        ok(type == DPNA_DATATYPE_GUID, "type expected DPNA_DATATYPE_GUID got %ld\n", type);

        trace("GetNumComponents=%ld\n", components);
        for(i=0; i < components; i++)
        {
            void *buffer;

            bufflen = 0;
            namelen = 0;

            hr = IDirectPlay8Address_GetComponentByIndex(localaddr, i, NULL, &namelen, NULL, &bufflen, &type);
            ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

            name =  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, namelen * sizeof(WCHAR));
            buffer =  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufflen);

            hr = IDirectPlay8Address_GetComponentByIndex(localaddr, i, name, &namelen, buffer, &bufflen, &type);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            if(hr == S_OK)
            {
                switch(type)
                {
                    case DPNA_DATATYPE_STRING:
                        trace("%ld: %s: %s\n", i, wine_dbgstr_w(name), wine_dbgstr_w(buffer));
                        break;
                    case DPNA_DATATYPE_DWORD:
                        trace("%ld: %s: %ld\n", i, wine_dbgstr_w(name), *(DWORD*)buffer);
                        break;
                    case DPNA_DATATYPE_GUID:
                        trace("%ld: %s: %s\n", i, wine_dbgstr_w(name), wine_dbgstr_guid( (GUID*)buffer));
                        break;
                    case DPNA_DATATYPE_BINARY:
                        trace("%ld: %s: Binary Data %ld\n", i, wine_dbgstr_w(name), bufflen);
                        break;
                    default:
                        trace(" Unknown\n");
                        break;
                }
            }

            HeapFree(GetProcessHeap(), 0, name);
            HeapFree(GetProcessHeap(), 0, buffer);
        }

        IDirectPlay8Address_Release(localaddr);
    }
}

static void address_setsp(void)
{
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");
    if(SUCCEEDED(hr))
    {
        DWORD components;
        WCHAR *name;
        GUID  guid = IID_Random;
        DWORD type;
        DWORD namelen = 0;
        DWORD bufflen = 0;

        hr = IDirectPlay8Address_GetNumComponents(localaddr, &components);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(components == 0, "components=%ld\n", components);

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetNumComponents(localaddr, &components);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(components == 1, "components=%ld\n", components);

        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 0, NULL, &namelen, NULL, &bufflen, &type);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        name =  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, namelen * sizeof(WCHAR));

        hr = IDirectPlay8Address_GetComponentByIndex(localaddr, 0, name, &namelen, (void*)&guid, &bufflen, &type);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(type == DPNA_DATATYPE_GUID, "wrong datatype: %ld\n", type);
        ok(IsEqualGUID(&guid, &CLSID_DP8SP_TCPIP), "wrong guid\n");

        HeapFree(GetProcessHeap(), 0, name);

        IDirectPlay8Address_Release(localaddr);
    }
}

static void address_urlW(void)
{
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;
    WCHAR buffer[1024];

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");
    if(SUCCEEDED(hr))
    {
        DWORD bufflen = 0, bufflen2;
        DWORD port = 4321;

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetURLW(localaddr, NULL, NULL);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        bufflen = 10;
        hr = IDirectPlay8Address_GetURLW(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        bufflen = 0;
        hr = IDirectPlay8Address_GetURLW(localaddr, NULL, &bufflen);
        ok(bufflen == 66, "got %ld\n", bufflen);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetURLW(localaddr, buffer, &bufflen);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpW(L"x-directplay:/provider=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D", buffer), "got %s\n", wine_dbgstr_w(buffer));

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_PORT, &port, sizeof(DWORD), DPNA_DATATYPE_DWORD);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        bufflen = 0;
        hr = IDirectPlay8Address_GetURLW(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        bufflen2 = bufflen/2;
        memset( buffer, 0xcc, sizeof(buffer) );
        hr = IDirectPlay8Address_GetURLW(localaddr, buffer, &bufflen2);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);
        ok(buffer[0] == 0xcccc, "buffer modified\n");

        hr = IDirectPlay8Address_GetURLW(localaddr, buffer, &bufflen);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpW(L"x-directplay:/provider=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D;port=4321", buffer), "got %s\n", wine_dbgstr_w(buffer));

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhost, sizeof(localhost), DPNA_DATATYPE_STRING);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        bufflen = 0;
        hr = IDirectPlay8Address_GetURLW(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetURLW(localaddr, buffer, &bufflen);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpW(L"x-directplay:/provider=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D;port=4321;hostname=localhost", buffer), "got %s\n", wine_dbgstr_w(buffer));

        IDirectPlay8Address_Release(localaddr);
    }
}

static void address_urlA(void)
{
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;
    char buffer[1024];
    static const char urlA1[] = "x-directplay:/provider=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D";
    static const char urlA2[] = "x-directplay:/provider=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D;port=4321";
    static const char urlA3[] = "x-directplay:/provider=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D;port=4321;hostname=localhost";
    static const char localhostA[] = "localhost";

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");
    if(SUCCEEDED(hr))
    {
        DWORD bufflen = 0, bufflen2;
        DWORD port = 4321;

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetURLA(localaddr, NULL, NULL);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        bufflen = 10;
        hr = IDirectPlay8Address_GetURLA(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_INVALIDPOINTER, "got 0x%08lx\n", hr);

        bufflen = 0;
        hr = IDirectPlay8Address_GetURLA(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        bufflen2 = bufflen/2;
        memset( buffer, 0x55, sizeof(buffer) );
        hr = IDirectPlay8Address_GetURLA(localaddr, buffer, &bufflen2);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);
        ok(buffer[0] == 0x55, "buffer modified\n");

        hr = IDirectPlay8Address_GetURLA(localaddr, buffer, &bufflen);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpA(urlA1, buffer), "got %s\n", buffer);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_PORT, &port, sizeof(DWORD), DPNA_DATATYPE_DWORD);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        bufflen = 0;
        hr = IDirectPlay8Address_GetURLA(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetURLA(localaddr, buffer, &bufflen);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpA(urlA2, buffer), "got %s\n", buffer);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhostA, sizeof(localhostA), DPNA_DATATYPE_STRING_ANSI);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        bufflen = 0;
        hr = IDirectPlay8Address_GetURLA(localaddr, NULL, &bufflen);
        ok(hr == DPNERR_BUFFERTOOSMALL, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetURLA(localaddr, buffer, &bufflen);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpA(urlA3, buffer), "got %s\n", buffer);

        IDirectPlay8Address_Release(localaddr);
    }
}

static void address_duplicate(void)
{
    HRESULT hr;
    IDirectPlay8Address *localaddr = NULL;
    IDirectPlay8Address *duplicate = NULL;
    DWORD components, dupcomps;
    GUID  guid = IID_Random;

    hr = CoCreateInstance( &CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (LPVOID*)&localaddr);
    ok(hr == S_OK, "Failed to create IDirectPlay8Address object\n");
    if(SUCCEEDED(hr))
    {
        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, &localhost, sizeof(localhost), DPNA_DATATYPE_STRING);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IDirectPlay8Address_GetNumComponents(localaddr, &components);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(components == 2, "components=%ld\n", components);

        hr = IDirectPlay8Address_Duplicate(localaddr, &duplicate);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            DWORD size, type;
            WCHAR buffer[256];

            hr = IDirectPlay8Address_GetSP(duplicate, &guid);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            ok(IsEqualGUID(&guid, &CLSID_DP8SP_TCPIP), "wrong guid\n");

            hr = IDirectPlay8Address_GetNumComponents(duplicate, &dupcomps);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            ok(components == dupcomps, "expected %ld got %ld\n", components, dupcomps);

            size = sizeof(buffer);
            hr = IDirectPlay8Address_GetComponentByName(duplicate, DPNA_KEY_HOSTNAME, buffer, &size, &type);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            ok(type == DPNA_DATATYPE_STRING, "incorrect type %ld\n", type);
            ok(!lstrcmpW(buffer, localhost), "Invalid string: %s\n", wine_dbgstr_w(buffer));

            IDirectPlay8Address_Release(duplicate);
        }

        IDirectPlay8Address_Release(localaddr);
    }
}

START_TEST(address)
{
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

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    create_directplay_address();
    address_addcomponents();
    address_setsp();
    address_duplicate();
    address_urlW();
    address_urlA();

    CoUninitialize();
}
