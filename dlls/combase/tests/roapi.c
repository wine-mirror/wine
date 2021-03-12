/*
 * Copyright (c) 2018 Alistair Leslie-Hughes
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"


#include "wine/test.h"

static HRESULT (WINAPI *pRoActivateInstance)(HSTRING, IInspectable **);
static HRESULT (WINAPI *pRoInitialize)(RO_INIT_TYPE);
static void    (WINAPI *pRoUninitialize)(void);
static HRESULT (WINAPI *pRoGetActivationFactory)(HSTRING, REFIID, void **);

static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);

#define SET(x) p##x = (void*)GetProcAddress(hmod, #x)

static BOOL init_functions(void)
{
    HMODULE hmod = LoadLibraryA("combase.dll");
    if (!hmod)
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return FALSE;
    }
    SET(RoActivateInstance);
    SET(RoInitialize);
    SET(RoUninitialize);
    SET(RoGetActivationFactory);

    SET(WindowsCreateString);
    SET(WindowsDeleteString);

    return TRUE;
}

static void test_ActivationFactories(void)
{
    HRESULT hr;
    HSTRING str, str2;
    IActivationFactory *factory = NULL;
    IInspectable *inspect = NULL;

    if(!pRoGetActivationFactory || !pRoActivateInstance)
    {
        win_skip("RoGetActivationFactory not available\n");
        return;
    }

    hr = pWindowsCreateString(L"Windows.Data.Xml.Dom.XmlDocument",
                              ARRAY_SIZE(L"Windows.Data.Xml.Dom.XmlDocument") - 1, &str);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = pWindowsCreateString(L"Does.Not.Exist", ARRAY_SIZE(L"Does.Not.Exist") - 1, &str2);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = pRoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = pRoGetActivationFactory(str2, &IID_IActivationFactory, (void **)&factory);
    ok(hr == REGDB_E_CLASSNOTREG, "got %08x\n", hr);

    hr = pRoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);
    if(factory)
        IActivationFactory_Release(factory);

    hr = pRoActivateInstance(str2, &inspect);
    ok(hr == REGDB_E_CLASSNOTREG, "got %08x\n", hr);

    hr = pRoActivateInstance(str, &inspect);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);
    if(inspect)
        IInspectable_Release(inspect);

    pWindowsDeleteString(str2);
    pWindowsDeleteString(str);
    pRoUninitialize();
}

START_TEST(roapi)
{
    if (!init_functions())
        return;

    test_ActivationFactories();
}
