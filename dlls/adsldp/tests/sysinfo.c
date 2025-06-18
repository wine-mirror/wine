/*
 * ADSystemInfo Tests
 *
 * Copyright 2018 Dmitry Timoshkov
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
#include "initguid.h"
#include "iads.h"
#define SECURITY_WIN32
#include "security.h"

#include "wine/test.h"

static void test_ComputerName(void)
{
    static const WCHAR cnW[] = { 'C','N','=' };
    static const WCHAR ComputersW[] = { 'C','N','=','C','o','m','p','u','t','e','r','s' };
    BOOL ret;
    ULONG size, name_size;
    WCHAR name[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR buf[1024];
    IADsADSystemInfo *sysinfo;
    BSTR bstr;
    HRESULT hr;

    name_size = MAX_COMPUTERNAME_LENGTH + 1;
    SetLastError(0xdeadbeef);
    ret = GetComputerNameW(name, &name_size);
    ok(ret, "GetComputerName error %lu\n", GetLastError());

    /* Distinguished name (rfc1779) is supposed to look like this:
     * "CN=COMPNAME,CN=Computers,DC=domain,DC=com"
     */

    size = 1024;
    SetLastError(0xdeadbeef);
    ret = GetComputerObjectNameW(NameFullyQualifiedDN, buf, &size);
    ok(ret || GetLastError() == ERROR_CANT_ACCESS_DOMAIN_INFO, "GetComputerObjectName error %lu\n", GetLastError());
    if (ret)
    {
        const WCHAR *p;
        ok(size == lstrlenW(buf) + 1, "got %lu, expected %u\n", size, lstrlenW(buf) + 1);
        ok(!memcmp(buf, cnW, sizeof(cnW)), "got %s\n", wine_dbgstr_w(buf));
        ok(!memcmp(buf + 3, name, name_size), "got %s (name_size = %lu)\n", wine_dbgstr_w(buf), name_size);
        p = wcschr(buf, ',');
        ok(p != NULL, "delimiter was not found\n");
        ok(p && !memcmp(p + 1, ComputersW, sizeof(ComputersW)), "got %s\n", p ? wine_dbgstr_w(p + 1) : wine_dbgstr_w(buf));
    }

    hr = CoCreateInstance(&CLSID_ADSystemInfo, 0, CLSCTX_ALL, &IID_IADsADSystemInfo, (void **)&sysinfo);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IADsADSystemInfo_get_ComputerName(sysinfo, &bstr);
    ok(hr == S_OK || hr == HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO), "got %#lx\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpW(bstr, buf), "got %s, expected %s\n", wine_dbgstr_w(bstr), wine_dbgstr_w(buf));
        SysFreeString(bstr);
    }

    IADsADSystemInfo_Release(sysinfo);
}

static void test_UserName(void)
{
    static const WCHAR cnW[] = { 'C','N','=' };
    static const WCHAR UsersW[] = { 'C','N','=','U','s','e','r','s' };
    BOOL ret;
    ULONG size, name_size;
    WCHAR name[256];
    WCHAR buf[1024];
    IADsADSystemInfo *sysinfo;
    BSTR bstr;
    HRESULT hr;

    name_size = 256;
    SetLastError(0xdeadbeef);
    ret = GetUserNameW(name, &name_size);
    ok(ret, "GetUserName error %lu\n", GetLastError());

    /* Distinguished name (rfc1779) is supposed to look like this:
     * "CN=username,CN=Users,DC=domain,DC=com"
     */

    size = 1024;
    SetLastError(0xdeadbeef);
    ret = GetUserNameExW(NameFullyQualifiedDN, buf, &size);
    ok(ret || GetLastError() == ERROR_NONE_MAPPED, "GetUserNameEx error %lu\n", GetLastError());
    if (ret)
    {
        const WCHAR *p;
        ok(size == lstrlenW(buf), "got %lu, expected %u\n", size, lstrlenW(buf));
        ok(!memcmp(buf, cnW, sizeof(cnW)), "got %s\n", wine_dbgstr_w(buf));
        ok(!memcmp(buf + 3, name, name_size), "got %s (name_size = %lu)\n", wine_dbgstr_w(buf), name_size);
        p = wcschr(buf, ',');
        ok(p != NULL, "delimiter was not found\n");
        ok(p && !memcmp(p + 1, UsersW, sizeof(UsersW)), "got %s\n", p ? wine_dbgstr_w(p + 1) : wine_dbgstr_w(buf));
    }

    hr = CoCreateInstance(&CLSID_ADSystemInfo, 0, CLSCTX_ALL, &IID_IADsADSystemInfo, (void **)&sysinfo);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IADsADSystemInfo_get_UserName(sysinfo, &bstr);
    todo_wine
    ok(hr == S_OK || hr == HRESULT_FROM_WIN32(ERROR_NONE_MAPPED), "got %#lx\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpW(bstr, buf), "got %s, expected %s\n", wine_dbgstr_w(bstr), wine_dbgstr_w(buf));
        SysFreeString(bstr);
    }

    IADsADSystemInfo_Release(sysinfo);
}

static void test_sysinfo(void)
{

    IADsADSystemInfo *sysinfo;
    IDispatch *dispatch;
    BSTR bstr;
    VARIANT_BOOL value;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ADSystemInfo, 0, CLSCTX_ALL, &IID_IUnknown, (void **)&sysinfo);
    ok(hr == S_OK, "got %#lx\n", hr);
    IADsADSystemInfo_Release(sysinfo);

    hr = CoCreateInstance(&CLSID_ADSystemInfo, 0, CLSCTX_ALL, &IID_IADsADSystemInfo, (void **)&sysinfo);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IADsADSystemInfo_QueryInterface(sysinfo, &IID_IDispatch, (void **)&dispatch);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDispatch_Release(dispatch);

    hr = IADsADSystemInfo_get_ComputerName(sysinfo, &bstr);
    ok(hr == S_OK || hr == HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO), "got %#lx\n", hr);
    if (hr != S_OK)
    {
        skip("Computer is not part of a domain, skipping the tests\n");
        goto done;
    }
    SysFreeString(bstr);

    hr = IADsADSystemInfo_get_UserName(sysinfo, &bstr);
    todo_wine
    ok(hr == S_OK || hr == HRESULT_FROM_WIN32(ERROR_NONE_MAPPED), "got %#lx\n", hr);
    if (hr != S_OK) goto done;
    SysFreeString(bstr);

    hr = IADsADSystemInfo_get_SiteName(sysinfo, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) SysFreeString(bstr);

    hr = IADsADSystemInfo_get_DomainShortName(sysinfo, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) SysFreeString(bstr);

    hr = IADsADSystemInfo_get_DomainDNSName(sysinfo, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) SysFreeString(bstr);

    hr = IADsADSystemInfo_get_ForestDNSName(sysinfo, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) SysFreeString(bstr);

    hr = IADsADSystemInfo_get_PDCRoleOwner(sysinfo, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) SysFreeString(bstr);

    hr = IADsADSystemInfo_get_IsNativeMode(sysinfo, &value);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) ok(value == VARIANT_TRUE, "IsNativeMode: %s\n", value == VARIANT_TRUE ? "yes" : "no");

    hr = IADsADSystemInfo_GetAnyDCName(sysinfo, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK) SysFreeString(bstr);
done:
    IADsADSystemInfo_Release(sysinfo);
}

START_TEST(sysinfo)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    test_ComputerName();
    test_UserName();
    test_sysinfo();

    CoUninitialize();
}
