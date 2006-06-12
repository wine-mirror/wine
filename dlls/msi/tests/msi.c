/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

typedef INSTALLSTATE (WINAPI *fnMsiUseFeatureExA)(LPCSTR, LPCSTR ,DWORD, DWORD );
fnMsiUseFeatureExA pMsiUseFeatureExA;
typedef UINT (WINAPI *fnMsiOpenPackageExA)(LPCSTR, DWORD, MSIHANDLE*);
fnMsiOpenPackageExA pMsiOpenPackageExA;
typedef UINT (WINAPI *fnMsiOpenPackageExW)(LPCWSTR, DWORD, MSIHANDLE*);
fnMsiOpenPackageExW pMsiOpenPackageExW;

static void test_usefeature(void)
{
    UINT r;

    if (!pMsiUseFeatureExA)
        return;

    r = MsiQueryFeatureState(NULL,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiQueryFeatureState("{9085040-6000-11d3-8cfe-0150048383c9}" ,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL,NULL,0,0);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL, "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}", 
                         NULL, -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{9085040-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{0085040-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");
}

static void test_null(void)
{
    MSIHANDLE hpkg;
    UINT r;

    r = pMsiOpenPackageExW(NULL, 0, &hpkg);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiQueryProductStateW(NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return\n");

    r = MsiEnumFeaturesW(NULL,0,NULL,NULL);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");
}

START_TEST(msi)
{
    HMODULE hmod = GetModuleHandle("msi.dll");
    pMsiUseFeatureExA = (fnMsiUseFeatureExA) 
        GetProcAddress(hmod, "MsiUseFeatureExA");
    pMsiOpenPackageExA = (fnMsiOpenPackageExA) 
        GetProcAddress(hmod, "MsiOpenPackageExA");
    pMsiOpenPackageExW = (fnMsiOpenPackageExW) 
        GetProcAddress(hmod, "MsiOpenPackageExW");

    test_usefeature();
    test_null();
}
