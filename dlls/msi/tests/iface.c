/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2007 Mike McCormack for CodeWeavers
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
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <ole2.h>

#include "wine/test.h"

static GUID CLSID_IInstaller = { 0xc1090, 0, 0, {0xc0,0,0,0,0,0,0,0x46} };
static GUID IID_IMsiServer = { 0xc101c, 0, 0, {0xc0,0,0,0,0,0,0,0x46} };

static DISPID get_dispid( IDispatch *disp, const char *name )
{
    LPOLESTR str;
    UINT len;
    DISPID id;
    HRESULT r;

    len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0 );
    str = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR) );
    len = MultiByteToWideChar(CP_ACP, 0, name, -1, str, len );

    r = IDispatch_GetIDsOfNames( disp, &IID_IMsiServer, &str, 1, 0, &id );
    if (r != S_OK)
        return -1;

    return id;
}

static void test_msi_dispid(void)
{
    HRESULT r;
    IDispatch *disp = NULL;

    r = CoCreateInstance( &CLSID_IInstaller, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDispatch, (LPVOID*) &disp );
    todo_wine ok( r == S_OK, "failed to create an installer instance %08x\n", r);
    if (r != S_OK)
    {
        skip( "IMsiServer interface not present\n");
        return;
    }

    ok( get_dispid( disp, "OpenPackage" ) == 2, "dispid wrong\n");
    ok( get_dispid( disp, "OpenProduct" ) == 3, "dispid wrong\n");
    ok( get_dispid( disp, "OpenDatabase" ) == 4, "dispid wrong\n");
    ok( get_dispid( disp, "SummaryInformation" ) == 5, "dispid wrong\n");
    ok( get_dispid( disp, "UILevel" ) == 6, "dispid wrong\n");
    ok( get_dispid( disp, "EnableLog" ) == 7, "dispid wrong\n");
    ok( get_dispid( disp, "InstallProduct" ) == 8, "dispid wrong\n");
    ok( get_dispid( disp, "Version" ) == 9, "dispid wrong\n");
    ok( get_dispid( disp, "LastErrorRecord" ) == 10, "dispid wrong\n");
    ok( get_dispid( disp, "RegistryValue" ) == 11, "dispid wrong\n");
    ok( get_dispid( disp, "Environment" ) == 12, "dispid wrong\n");
    ok( get_dispid( disp, "FileAttributes" ) == 13, "dispid wrong\n");

    ok( get_dispid( disp, "FileSize" ) == 15, "dispid wrong\n");
    ok( get_dispid( disp, "FileVersion" ) == 16, "dispid wrong\n");
    ok( get_dispid( disp, "ProductState" ) == 17, "dispid wrong\n");
    ok( get_dispid( disp, "ProductInfo" ) == 18, "dispid wrong\n");
    ok( get_dispid( disp, "ConfigureProduct" ) == 19, "dispid wrong\n");
    ok( get_dispid( disp, "ReinstallProduct" ) == 20 , "dispid wrong\n");
    ok( get_dispid( disp, "CollectUserInfo" ) == 21, "dispid wrong\n");
    ok( get_dispid( disp, "ApplyPatch" ) == 22, "dispid wrong\n");
    ok( get_dispid( disp, "FeatureParent" ) == 23, "dispid wrong\n");
    ok( get_dispid( disp, "FeatureState" ) == 24, "dispid wrong\n");
    ok( get_dispid( disp, "UseFeature" ) == 25, "dispid wrong\n");
    ok( get_dispid( disp, "FeatureUsageCount" ) == 26, "dispid wrong\n");
    ok( get_dispid( disp, "FeatureUsageDate" ) == 27, "dispid wrong\n");
    ok( get_dispid( disp, "ConfigureFeature" ) == 28, "dispid wrong\n");
    ok( get_dispid( disp, "ReinstallFeature" ) == 29, "dispid wrong\n");
    ok( get_dispid( disp, "ProvideComponent" ) == 30, "dispid wrong\n");
    ok( get_dispid( disp, "ComponentPath" ) == 31, "dispid wrong\n");
    ok( get_dispid( disp, "ProvideQualifiedComponent" ) == 32, "dispid wrong\n");
    ok( get_dispid( disp, "QualifierDescription" ) == 33, "dispid wrong\n");
    ok( get_dispid( disp, "ComponentQualifiers" ) == 34, "dispid wrong\n");
    ok( get_dispid( disp, "Products" ) == 35, "dispid wrong\n");
    ok( get_dispid( disp, "Features" ) == 36, "dispid wrong\n");
    ok( get_dispid( disp, "Components" ) == 37, "dispid wrong\n");
    ok( get_dispid( disp, "ComponentClients" ) == 38, "dispid wrong\n");
    ok( get_dispid( disp, "Patches" ) == 39, "dispid wrong\n");
    ok( get_dispid( disp, "RelatedProducts" ) == 40, "dispid wrong\n");
    ok( get_dispid( disp, "PatchInfo" ) == 41, "dispid wrong\n");
    ok( get_dispid( disp, "PatchTransforms" ) == 42, "dispid wrong\n");
    ok( get_dispid( disp, "AddSource" ) == 43, "dispid wrong\n");
    ok( get_dispid( disp, "ClearSourceList" ) == 44, "dispid wrong\n");
    ok( get_dispid( disp, "ForceSourceListResolution" ) == 45, "dispid wrong\n");
    ok( get_dispid( disp, "ShortcutTarget" ) == 46, "dispid wrong\n");
    ok( get_dispid( disp, "FileHash" ) == 47, "dispid wrong\n");
    ok( get_dispid( disp, "FileSignatureInfo" ) == 48, "dispid wrong\n");
    ok( get_dispid( disp, "RemovePatches" ) == 49, "dispid wrong\n");

    ok( get_dispid( disp, "ApplyMultiplePatches" ) == 51, "dispid wrong\n");
    ok( get_dispid( disp, "ProductsEx" ) ==  52, "dispid wrong\n");

    ok( get_dispid( disp, "PatchesEx" ) == 55, "dispid wrong\n");

    ok( get_dispid( disp, "ExtractPatchXMLData" ) == 57, "dispid wrong\n");

    /* MSDN claims the following functions exist but IDispatch->GetIDsOfNames disagrees */
    if (0)
    {
        get_dispid( disp, "ProductElevated" );
        get_dispid( disp, "ProductInfoFromScript" );
        get_dispid( disp, "ProvideAssembly" );
        get_dispid( disp, "CreateAdvertiseScript" );
        get_dispid( disp, "AdvertiseProduct" );
        get_dispid( disp, "PatchFiles" );
    }

    IDispatch_Release( disp );
}

static void test_msi_invoke(void)
{
    IDispatch *installer = NULL, *record = NULL;
    DISPPARAMS param;
    VARIANTARG varg;
    VARIANT result;
    DISPID dispid;
    DISPID arg;
    HRESULT r;

    r = CoCreateInstance( &CLSID_IInstaller, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IDispatch, (LPVOID*) &installer );
    todo_wine ok( r == S_OK, "failed to create an installer instance %08x\n", r);
    if (r != S_OK)
    {
        skip( "IMsiServer interface not present\n");
        return;
    }

    arg = 0;

    V_UINT(&varg) = 1;
    V_VT(&varg) = VT_I4;

    param.cArgs = 1;
    param.cNamedArgs = 0;
    param.rgvarg = &varg;
    param.rgdispidNamedArgs = &arg;

    dispid = get_dispid( installer, "CreateRecord" );

    r = IDispatch_Invoke( installer, dispid, &IID_NULL, 0,
                          DISPATCH_METHOD, &param, &result, NULL, NULL);
    ok( r == S_OK, "dispatch failed %08x\n", r);

    ok( V_VT(&result) == VT_DISPATCH, "type wrong\n");

    record = V_DISPATCH(&result);

    memset( &result, 0, sizeof result );
    dispid = get_dispid( record, "FieldCount" );

    param.cArgs = 0;
    param.cNamedArgs = 0;
    param.rgvarg = &varg;
    param.rgdispidNamedArgs = &arg;

    r = IDispatch_Invoke( record, dispid, &IID_NULL, 0,
                          DISPATCH_PROPERTYGET, &param, &result, NULL, NULL );
    ok( r == S_OK, "dispatch failed %08x\n", r);

    ok( V_VT(&result) == VT_I4, "type wrong\n");
    ok( V_I4(&result) == 1, "field count wrong\n");

    IDispatch_Release( record );
    IDispatch_Release( installer );
}

START_TEST(iface)
{
    CoInitialize( NULL );

    test_msi_dispid();
    test_msi_invoke();

    CoUninitialize();
}
