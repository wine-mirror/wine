/*
 * LDAPNamespace Tests
 *
 * Copyright 2019 Dmitry Timoshkov
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "iads.h"
#include "adserr.h"
#include "adshlp.h"

#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(CLSID_LDAP,0x228d9a81,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);
DEFINE_GUID(CLSID_LDAPNamespace,0x228d9a82,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);
DEFINE_OLEGUID(CLSID_PointerMoniker,0x306,0,0);

static const struct
{
    const WCHAR *path;
    HRESULT hr, hr_ads_open, hr_ads_get;
    const WCHAR *user, *password;
    LONG flags;
} test[] =
{
    { L"invalid", MK_E_SYNTAX, E_ADS_BAD_PATHNAME, E_FAIL },
    { L"LDAP", MK_E_SYNTAX, E_ADS_BAD_PATHNAME, E_FAIL },
    { L"LDAP:", S_OK },
    { L"LDAP:/", E_ADS_BAD_PATHNAME },
    { L"LDAP://", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com", S_OK },
    { L"LDAP:///ldap.forumsys.com", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com:389", S_OK },
    { L"LDAP://ldap.forumsys.com:389/DC=example,DC=com", S_OK },
    { L"LDAP://ldap.forumsys.com/", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com/rootDSE", S_OK },
    { L"LDAP://ldap.forumsys.com/rootDSE/", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com/rootDSE/invalid", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com/rootDSE", S_OK, S_OK, S_OK, NULL, NULL, 0 },
    { L"LDAP://ldap.forumsys.com/rootDSE", S_OK, S_OK, S_OK, L"CN=read-only-admin,DC=example,DC=com", L"password", 0 },

    /*{ L"LDAP://invalid", __HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX) }, takes way too much time */
};

static void test_LDAP(void)
{
    HRESULT hr;
    IUnknown *unk;
    IADs *ads;
    IADsOpenDSObject *ads_open;
    IDispatch *disp;
    BSTR path, user, password;
    int i;

    hr = CoCreateInstance(&CLSID_LDAPNamespace, 0, CLSCTX_INPROC_SERVER, &IID_IADs, (void **)&ads);
    ok(hr == S_OK, "got %#x\n", hr);
    IADs_Release(ads);

    hr = CoCreateInstance(&CLSID_LDAPNamespace, 0, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IDispatch, (void **)&disp);
    ok(hr == S_OK, "got %#x\n", hr);
    IDispatch_Release(disp);

    hr = IUnknown_QueryInterface(unk, &IID_IADsOpenDSObject, (void **)&ads_open);
    ok(hr == S_OK, "got %#x\n", hr);

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        path = SysAllocString(test[i].path);
        user = test[i].user ? SysAllocString(test[i].user) : NULL;
        password = test[i].password ? SysAllocString(test[i].password) : NULL;

        hr = IADsOpenDSObject_OpenDSObject(ads_open, path, user, password, test[i].flags, &disp);
        ok(hr == test[i].hr || hr == test[i].hr_ads_open, "%d: got %#x, expected %#x\n", i, hr, test[i].hr);
        if (hr == S_OK)
            IDispatch_Release(disp);

        hr = ADsOpenObject(path, user, password, test[i].flags, &IID_IADs, (void **)&ads);
        ok(hr == test[i].hr || hr == test[i].hr_ads_get, "%d: got %#x, expected %#x\n", i, hr, test[i].hr);
        if (hr == S_OK)
            IADs_Release(ads);

        hr = ADsGetObject(path, &IID_IDispatch, (void **)&disp);
        ok(hr == test[i].hr || hr == test[i].hr_ads_get, "%d: got %#x, expected %#x\n", i, hr, test[i].hr);
        if (hr == S_OK)
            IDispatch_Release(disp);

        SysFreeString(path);
        SysFreeString(user);
        SysFreeString(password);
    }


    IADsOpenDSObject_Release(ads_open);
    IUnknown_Release(unk);
}

static void test_ParseDisplayName(void)
{
    HRESULT hr;
    IBindCtx *bc;
    IParseDisplayName *parse;
    IMoniker *mk;
    IUnknown *unk;
    CLSID clsid;
    BSTR path;
    ULONG count;
    int i;

    hr = CoCreateInstance(&CLSID_LDAP, 0, CLSCTX_INPROC_SERVER, &IID_IParseDisplayName, (void **)&parse);
    ok(hr == S_OK, "got %#x\n", hr);
    IParseDisplayName_Release(parse);

    hr = CoCreateInstance(&CLSID_LDAP, 0, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "got %#x\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IParseDisplayName, (void **)&parse);
    ok(hr == S_OK, "got %#x\n", hr);
    IUnknown_Release(unk);

    hr = CreateBindCtx(0, &bc);
    ok(hr == S_OK, "got %#x\n", hr);

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        path = SysAllocString(test[i].path);

        count = 0xdeadbeef;
        hr = IParseDisplayName_ParseDisplayName(parse, bc, path, &count, &mk);
        ok(hr == test[i].hr || hr == test[i].hr_ads_open, "%d: got %#x, expected %#x\n", i, hr, test[i].hr);
        if (hr == S_OK)
        {
            ok(count == lstrlenW(test[i].path), "%d: got %d\n", i, count);

            hr = IMoniker_GetClassID(mk, &clsid);
            ok(hr == S_OK, "got %#x\n", hr);
            ok(IsEqualGUID(&clsid, &CLSID_PointerMoniker), "%d: got %s\n", i, wine_dbgstr_guid(&clsid));

            IMoniker_Release(mk);
        }

        SysFreeString(path);

        count = 0xdeadbeef;
        hr = MkParseDisplayName(bc, test[i].path, &count, &mk);
todo_wine_if(i == 0 || i == 1 || i == 11 || i == 12)
        ok(hr == test[i].hr, "%d: got %#x, expected %#x\n", i, hr, test[i].hr);
        if (hr == S_OK)
        {
            ok(count == lstrlenW(test[i].path), "%d: got %d\n", i, count);

            hr = IMoniker_GetClassID(mk, &clsid);
            ok(hr == S_OK, "got %#x\n", hr);
            ok(IsEqualGUID(&clsid, &CLSID_PointerMoniker), "%d: got %s\n", i, wine_dbgstr_guid(&clsid));

            IMoniker_Release(mk);
        }
    }

    IBindCtx_Release(bc);
    IParseDisplayName_Release(parse);
}

START_TEST(ldap)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#x\n", hr);

    test_LDAP();
    test_ParseDisplayName();

    CoUninitialize();
}
