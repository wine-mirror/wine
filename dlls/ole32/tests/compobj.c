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

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08lx\n", hr)

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

START_TEST(compobj)
{
    test_ProgIDFromCLSID();
    test_CLSIDFromProgID();
    test_CLSIDFromString();
}
