/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include <stdio.h>
#include "windows.h"
#include "objidl.h"
#include "wbemcli.h"
#include "wine/test.h"

static void test_IClientSecurity(void)
{
    HRESULT hr;
    IWbemLocator *locator;
    IWbemServices *services;
    IClientSecurity *security;
    BSTR path = SysAllocString( L"ROOT\\CIMV2" );
    ULONG refs;

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    if (hr != S_OK)
    {
        win_skip("can't create instance of WbemLocator\n");
        return;
    }
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    refs = IWbemLocator_Release( locator );
    ok( refs == 0, "unexpected refcount %u\n", refs );

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    refs = IWbemServices_Release( services );
    ok( refs == 0, "unexpected refcount %u\n", refs );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    hr = IWbemServices_QueryInterface( services, &IID_IClientSecurity, (void **)&security );
    ok( hr == S_OK, "failed to query IClientSecurity interface %08x\n", hr );
    ok( (void *)services != (void *)security, "expected pointers to be different\n" );

    refs = IClientSecurity_Release( security );
    ok( refs == 1, "unexpected refcount %u\n", refs );

    refs = IWbemServices_Release( services );
    ok( refs == 0, "unexpected refcount %u\n", refs );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    hr = IWbemServices_QueryInterface( services, &IID_IClientSecurity, (void **)&security );
    ok( hr == S_OK, "failed to query IClientSecurity interface %08x\n", hr );
    ok( (void *)services != (void *)security, "expected pointers to be different\n" );

    refs = IWbemServices_Release( services );
    todo_wine ok( refs == 1, "unexpected refcount %u\n", refs );

    refs = IClientSecurity_Release( security );
    todo_wine ok( refs == 0, "unexpected refcount %u\n", refs );

    IWbemLocator_Release( locator );
    SysFreeString( path );
}

static void test_IWbemLocator(void)
{
    static const struct
    {
        const WCHAR *path;
        HRESULT      result;
        BOOL         todo;
        HRESULT      result_broken;
    }
    test[] =
    {
        { L"", WBEM_E_INVALID_NAMESPACE },
        { L"\\", WBEM_E_INVALID_NAMESPACE },
        { L"\\\\", WBEM_E_INVALID_NAMESPACE },
        { L"\\\\.", WBEM_E_INVALID_NAMESPACE },
        { L"\\\\.\\", WBEM_E_INVALID_NAMESPACE, FALSE, WBEM_E_INVALID_PARAMETER },
        { L"\\ROOT", WBEM_E_INVALID_NAMESPACE },
        { L"\\\\ROOT", 0x800706ba, TRUE },
        { L"\\\\.ROOT", 0x800706ba, TRUE },
        { L"\\\\.\\NONE", WBEM_E_INVALID_NAMESPACE },
        { L"\\\\.\\ROOT", S_OK },
        { L"\\\\\\.\\ROOT", WBEM_E_INVALID_PARAMETER },
        { L"\\/.\\ROOT", S_OK, FALSE, WBEM_E_INVALID_PARAMETER },
        { L"//.\\ROOT", S_OK },
        { L"\\\\./ROOT", S_OK },
        { L"//./ROOT", S_OK },
        { L"NONE", WBEM_E_INVALID_NAMESPACE },
        { L"ROOT", S_OK },
        { L"ROOT\\NONE", WBEM_E_INVALID_NAMESPACE },
        { L"ROOT\\CIMV2", S_OK },
        { L"ROOT\\\\CIMV2", WBEM_E_INVALID_NAMESPACE, FALSE, WBEM_E_INVALID_PARAMETER },
        { L"ROOT\\CIMV2\\", WBEM_E_INVALID_NAMESPACE, FALSE, WBEM_E_INVALID_PARAMETER },
        { L"ROOT/CIMV2", S_OK },
        { L"root\\default", S_OK },
        { L"root\\cimv0", WBEM_E_INVALID_NAMESPACE },
        { L"root\\cimv1", WBEM_E_INVALID_NAMESPACE },
        { L"\\\\localhost\\ROOT", S_OK },
        { L"\\\\LOCALHOST\\ROOT", S_OK }
    };
    IWbemLocator *locator;
    IWbemServices *services;
    unsigned int i;
    HRESULT hr;
    BSTR resource;

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    if (hr != S_OK)
    {
        win_skip("can't create instance of WbemLocator\n");
        return;
    }
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    for (i = 0; i < ARRAY_SIZE( test ); i++)
    {
        resource = SysAllocString( test[i].path );
        hr = IWbemLocator_ConnectServer( locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services );
        todo_wine_if (test[i].todo)
            ok( hr == test[i].result || broken(hr == test[i].result_broken),
                "%u: expected %08x got %08x\n", i, test[i].result, hr );
        SysFreeString( resource );
        if (hr == S_OK) IWbemServices_Release( services );
    }
    IWbemLocator_Release( locator );
}

START_TEST(services)
{
    CoInitialize( NULL );
    test_IClientSecurity();
    test_IWbemLocator();
    CoUninitialize();
}
