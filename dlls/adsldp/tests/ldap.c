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

#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(CLSID_LDAPNamespace,0x228d9a82,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);

static void test_LDAP(void)
{
    HRESULT hr;
    IUnknown *unk;
    IADs *ads;
    IADsOpenDSObject *ads_open;
    IDispatch *disp;
    BSTR path;

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
    IADsOpenDSObject_Release(ads_open);

    path = SysAllocString(L"LDAP:");
    hr = IADsOpenDSObject_OpenDSObject(ads_open, path, NULL, NULL, ADS_SECURE_AUTHENTICATION, &disp);
    SysFreeString(path);
todo_wine
    ok(hr == S_OK, "got %#x\n", hr);
if (hr == S_OK)
    IDispatch_Release(disp);

    IUnknown_Release(unk);
}

START_TEST(ldap)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#x\n", hr);

    test_LDAP();

    CoUninitialize();
}
