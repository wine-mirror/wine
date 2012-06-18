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
#include "ocidl.h"
#include "initguid.h"
#include "objidl.h"
#include "wbemcli.h"
#include "wine/test.h"

static const WCHAR wqlW[] = {'w','q','l',0};

static HRESULT exec_query( IWbemServices *services, const WCHAR *str, IEnumWbemClassObject **result )
{
    static const WCHAR captionW[] = {'C','a','p','t','i','o','n',0};
    static const WCHAR descriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    HRESULT hr;
    IWbemClassObject *obj;
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( str );
    LONG flags = WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
    ULONG count;

    hr = IWbemServices_ExecQuery( services, wql, query, flags, NULL, result );
    if (hr == S_OK)
    {
        trace("%s\n", wine_dbgstr_w(str));
        for (;;)
        {
            VARIANT var;

            IEnumWbemClassObject_Next( *result, WBEM_INFINITE, 1, &obj, &count );
            if (!count) break;

            if (IWbemClassObject_Get( obj, captionW, 0, &var, NULL, NULL ) == WBEM_S_NO_ERROR)
            {
                trace("caption: %s\n", wine_dbgstr_w(V_BSTR(&var)));
                VariantClear( &var );
            }
            if (IWbemClassObject_Get( obj, descriptionW, 0, &var, NULL, NULL ) == WBEM_S_NO_ERROR)
            {
                trace("description: %s\n", wine_dbgstr_w(V_BSTR(&var)));
                VariantClear( &var );
            }
            IWbemClassObject_Release( obj );
        }
    }
    SysFreeString( wql );
    SysFreeString( query );
    return hr;
}

static void test_select(void)
{
    static const WCHAR cimv2W[] = {'R','O','O','T','\\','C','I','M','V','2',0};
    static const WCHAR sqlW[] = {'S','Q','L',0};
    static const WCHAR query1[] =
        {'S','E','L','E','C','T',' ','H','O','T','F','I','X','I','D',' ','F','R','O','M',' ',
         'W','i','n','3','2','_','Q','u','i','c','k','F','i','x','E','n','g','i','n','e','e','r','i','n','g',0};
    static const WCHAR query2[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','B','I','O','S',0};
    static const WCHAR query3[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k',' ','W','H','E','R','E',' ',
         '\"','N','T','F','S','\"',' ','=',' ','F','i','l','e','S','y','s','t','e','m',0};
    static const WCHAR query4[] =
        {'S','E','L','E','C','T',' ','a',' ','F','R','O','M',' ','b',0};
    static const WCHAR query5[] =
        {'S','E','L','E','C','T',' ','a',' ','F','R','O','M',' ','W','i','n','3','2','_','B','i','o','s',0};
    static const WCHAR query6[] =
        {'S','E','L','E','C','T',' ','D','e','s','c','r','i','p','t','i','o','n',' ','F','R','O','M',' ',
         'W','i','n','3','2','_','B','i','o','s',0};
    static const WCHAR query7[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','r','o','c','e','s','s',' ','W','H','E','R','E',' ','C','a','p','t','i','o','n',' ',
         'L','I','K','E',' ','\'','%','%','R','E','G','E','D','I','T','%','\'',0};
    static const WCHAR *test[] = { query1, query2, query3, query4, query5, query6, query7 };
    HRESULT hr;
    IWbemLocator *locator;
    IWbemServices *services;
    IEnumWbemClassObject *result;
    BSTR path = SysAllocString( cimv2W );
    BSTR wql = SysAllocString( wqlW );
    BSTR sql = SysAllocString( sqlW );
    BSTR query = SysAllocString( query1 );
    UINT i;

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                           (void **)&locator );
    if (hr != S_OK)
    {
        win_skip("can't create instance of WbemLocator\n");
        return;
    }
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    hr = CoSetProxyBlanket( (IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );
    ok( hr == S_OK, "failed to set proxy blanket %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, NULL, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, NULL, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, wql, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, sql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_QUERY_TYPE, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, sql, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    for (i = 0; i < sizeof(test)/sizeof(test[0]); i++)
    {
        hr = exec_query( services, test[i], &result );
        ok( hr == S_OK, "query %u failed: %08x\n", i, hr );
        if (result) IEnumWbemClassObject_Release( result );
    }

    IWbemServices_Release( services );
    IWbemLocator_Release( locator );
    SysFreeString( path );
    SysFreeString( wql );
    SysFreeString( sql );
    SysFreeString( query );
}

START_TEST(query)
{
    CoInitialize( NULL );
    CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                          RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL );
    test_select();
    CoUninitialize();
}
