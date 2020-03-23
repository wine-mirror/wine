/*
 * Copyright 2020 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "iads.h"
#include "adshlp.h"
#include "adserr.h"

#include "wine/test.h"

static void test_ADsBuildVarArrayStr(void)
{
    const WCHAR *props[] = { L"prop1", L"prop2" };
    HRESULT hr;
    VARIANT var, item, *data;
    LONG start, end, idx;

    hr = ADsBuildVarArrayStr(NULL, 0, NULL);
    ok(hr == E_ADS_BAD_PARAMETER || hr == E_FAIL /* XP */, "got %#x\n", hr);

    hr = ADsBuildVarArrayStr(NULL, 0, &var);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(V_VT(&var) == (VT_ARRAY | VT_VARIANT), "got %d\n", V_VT(&var));
    start = 0xdeadbeef;
    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, &start);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(start == 0, "got %d\n", start);
    end = 0xdeadbeef;
    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, &end);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(end == -1, "got %d\n", end);
    VariantClear(&var);

    hr = ADsBuildVarArrayStr((LPWSTR *)props, ARRAY_SIZE(props), &var);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(V_VT(&var) == (VT_ARRAY | VT_VARIANT), "got %d\n", V_VT(&var));
    start = 0xdeadbeef;
    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, &start);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(start == 0, "got %d\n", start);
    end = 0xdeadbeef;
    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, &end);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(end == 1, "got %d\n", end);
    idx = 0;
    hr = SafeArrayGetElement(V_ARRAY(&var), &idx, &item);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(V_VT(&item) == VT_BSTR, "got %d\n", V_VT(&item));
    ok(!lstrcmpW(V_BSTR(&item), L"prop1"), "got %s\n", wine_dbgstr_w(V_BSTR(&item)));
    VariantClear(&item);
    hr = SafeArrayAccessData(V_ARRAY(&var), (void *)&data);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(V_VT(&data[0]) == VT_BSTR, "got %d\n", V_VT(&data[0]));
    ok(!lstrcmpW(V_BSTR(&data[0]), L"prop1"), "got %s\n", wine_dbgstr_w(V_BSTR(&data[0])));
    ok(V_VT(&data[0]) == VT_BSTR, "got %d\n", V_VT(&data[0]));
    ok(!lstrcmpW(V_BSTR(&data[1]), L"prop2"), "got %s\n", wine_dbgstr_w(V_BSTR(&data[1])));
    hr = SafeArrayUnaccessData(V_ARRAY(&var));
    ok(hr == S_OK, "got %#x\n", hr);
    VariantClear(&var);
}

START_TEST(activeds)
{
    test_ADsBuildVarArrayStr();
}
