/*
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

#define COBJMACROS

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#include <objbase.h>
#include <prntvpt.h>

#include "wine/test.h"

static WCHAR default_name[256];

struct hprov_data
{
    HPTPROVIDER hprov;
    HRESULT hr;
};

static DWORD WINAPI CloseProvider_proc(void *param)
{
    struct hprov_data *data = param;

    data->hr = PTCloseProvider(data->hprov);

    return 0;
}

static void test_PTOpenProvider(void)
{
    DWORD tid, i;
    HPTPROVIDER hprov;
    HRESULT hr;
    HANDLE hthread;
    struct hprov_data data;

    hr = PTOpenProvider(default_name, 1, &hprov);
    ok(hr == S_OK, "got %#lx\n", hr);

    data.hprov = hprov;
    hthread = CreateThread(NULL, 0, CloseProvider_proc, &data, 0, &tid);
    WaitForSingleObject(hthread, INFINITE);
    CloseHandle(hthread);
    ok(data.hr == E_HANDLE || data.hr == E_INVALIDARG /* XP */ || broken(data.hr == S_OK) /* Win8 */, "got %#lx\n", data.hr);

    if (data.hr != S_OK)
    {
        hr = PTCloseProvider(hprov);
        ok(hr == S_OK, "got %#lx\n", hr);
    }

    hr = PTOpenProvider(default_name, 0, &hprov);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    for (i = 2; i < 20; i++)
    {
        hr = PTOpenProvider(default_name, i, &hprov);
        ok(hr == 0x80040001 || hr == E_INVALIDARG /* Wine */, "%lu: got %#lx\n", i, hr);
    }
}

static void test_PTOpenProviderEx(void)
{
    DWORD tid, ver, i;
    HPTPROVIDER hprov;
    HRESULT hr;
    HANDLE hthread;
    struct hprov_data data;

    hr = PTOpenProviderEx(default_name, 1, 1, &hprov, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    ver = 0xdeadbeef;
    hr = PTOpenProviderEx(default_name, 1, 1, &hprov, &ver);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(ver == 1, "got %#lx\n", ver);

    data.hprov = hprov;
    hthread = CreateThread(NULL, 0, CloseProvider_proc, &data, 0, &tid);
    WaitForSingleObject(hthread, INFINITE);
    CloseHandle(hthread);
    ok(data.hr == E_HANDLE || data.hr == E_INVALIDARG /* XP */ || broken(data.hr == S_OK) /* Win8 */, "got %#lx\n", data.hr);

    if (data.hr != S_OK)
    {
        hr = PTCloseProvider(hprov);
        ok(hr == S_OK, "got %#lx\n", hr);
    }

    for (i = 1; i < 20; i++)
    {
        hr = PTOpenProviderEx(default_name, 0, i, &hprov, &ver);
        ok(hr == E_INVALIDARG, "%lu: got %#lx\n", i, hr);

        ver = 0xdeadbeef;
        hr = PTOpenProviderEx(default_name, 1, i, &hprov, &ver);
        ok(hr == S_OK, "%lu: got %#lx\n", i, hr);
        ok(ver == 1, "%lu: got %#lx\n", i, ver);
        PTCloseProvider(hprov);

        ver = 0xdeadbeef;
        hr = PTOpenProviderEx(default_name, i, i, &hprov, &ver);
        ok(hr == S_OK, "%lu: got %#lx\n", i, hr);
        ok(ver == 1, "%lu: got %#lx\n", i, ver);
        PTCloseProvider(hprov);
    }
}

START_TEST(prntvpt)
{
    DWORD size;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    size = ARRAY_SIZE(default_name);
    if (!GetDefaultPrinterW(default_name, &size))
    {
        skip("no default printer set\n");
        return;
    }

    trace("default printer: %s\n", wine_dbgstr_w(default_name));

    test_PTOpenProvider();
    test_PTOpenProviderEx();

    CoUninitialize();
}
