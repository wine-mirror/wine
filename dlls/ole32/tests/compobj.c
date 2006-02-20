/*
 * Component Object Tests
 *
 * Copyright 2005 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlguid.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08lx\n", hr)

static const CLSID CLSID_non_existent =   { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const CLSID CLSID_CDeviceMoniker = { 0x4315d437, 0x5b8c, 0x11d0, { 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86 } };
static const WCHAR devicedotone[] = {'d','e','v','i','c','e','.','1',0};
static const WCHAR wszCLSID_CDeviceMoniker[] =
{
    '{',
    '4','3','1','5','d','4','3','7','-',
    '5','b','8','c','-',
    '1','1','d','0','-',
    'b','d','3','b','-',
    '0','0','a','0','c','9','1','1','c','e','8','6',
    '}',0
};

static void test_ProgIDFromCLSID(void)
{
    LPWSTR progid;
    HRESULT hr = ProgIDFromCLSID(&CLSID_CDeviceMoniker, &progid);
    ok(hr == S_OK, "ProgIDFromCLSID failed with error 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpiW(progid, devicedotone), "Didn't get expected prog ID\n");
        CoTaskMemFree(progid);
    }

    progid = (LPWSTR)0xdeadbeef;
    hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
    ok(hr == REGDB_E_CLASSNOTREG, "ProgIDFromCLSID returned %08lx\n", hr);
    ok(progid == NULL, "ProgIDFromCLSID returns with progid %p\n", progid);
}

static void test_CLSIDFromProgID(void)
{
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(devicedotone, &clsid);
    ok(hr == S_OK, "CLSIDFromProgID failed with error 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");

    hr = CLSIDFromString((LPOLESTR)devicedotone, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");
}

static void test_CLSIDFromString(void)
{
    CLSID clsid;
    HRESULT hr = CLSIDFromString((LPOLESTR)wszCLSID_CDeviceMoniker, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");
}

static void test_CoCreateInstance(void)
{
    REFCLSID rclsid = &CLSID_MyComputer;
    IUnknown *pUnk = (IUnknown *)0xdeadbeef;
    HRESULT hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have return CO_E_NOTINITIALIZED instead of 0x%08lx", hr);
    todo_wine {
    ok(pUnk == NULL, "CoCreateInstance should have changed the passed in pointer to NULL, instead of %p\n", pUnk);
    }

    OleInitialize(NULL);
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok_ole_success(hr, "CoCreateInstance");
    IUnknown_Release(pUnk);
    OleUninitialize();

    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have return CO_E_NOTINITIALIZED instead of 0x%08lx", hr);
}

START_TEST(compobj)
{
    test_ProgIDFromCLSID();
    test_CLSIDFromProgID();
    test_CLSIDFromString();
    test_CoCreateInstance();
}
