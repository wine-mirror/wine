/*
 * Path tests for kernelbase.dll
 *
 * Copyright 2017 Michael Müller
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
#include <windef.h>
#include <winbase.h>
#include <stdlib.h>
#include <winerror.h>
#include <winnls.h>
#include <pathcch.h>
#include <strsafe.h>

#include "wine/test.h"

HRESULT (WINAPI *pPathCchCombineEx)(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2, DWORD flags);

static const struct
{
    const char *path1;
    const char *path2;
    const char *result;
}
combine_test[] =
{
    /* normal paths */
    {"C:\\",  "a",     "C:\\a" },
    {"C:\\b", "..\\a", "C:\\a" },
    {"C:",    "a",     "C:\\a" },
    {"C:\\",  ".",     "C:\\" },
    {"C:\\",  "..",    "C:\\" },
    {"\\a",   "b",      "\\a\\b" },

    /* normal UNC paths */
    {"\\\\192.168.1.1\\test", "a",  "\\\\192.168.1.1\\test\\a" },
    {"\\\\192.168.1.1\\test", "..", "\\\\192.168.1.1" },

    /* NT paths */
    {"\\\\?\\C:\\", "a",  "C:\\a" },
    {"\\\\?\\C:\\", "..", "C:\\" },

    /* NT UNC path */
    {"\\\\?\\UNC\\192.168.1.1\\test", "a",  "\\\\192.168.1.1\\test\\a" },
    {"\\\\?\\UNC\\192.168.1.1\\test", "..", "\\\\192.168.1.1" },
};

static void test_PathCchCombineEx(void)
{
    WCHAR expected[MAX_PATH] = {'C',':','\\','a',0};
    WCHAR p1[MAX_PATH] = {'C',':','\\',0};
    WCHAR p2[MAX_PATH] = {'a',0};
    WCHAR output[MAX_PATH];
    HRESULT hr;
    int i;

    if (!pPathCchCombineEx)
    {
        skip("PathCchCombineEx() is not available.\n");
        return;
    }

    hr = pPathCchCombineEx(NULL, 2, p1, p2, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 0, p1, p2, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok(output[0] == 0xffff, "Expected output buffer to be unchanged\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 1, p1, p2, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08x\n", hr);
    ok(output[0] == 0, "Expected output buffer to contain NULL string\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 4, p1, p2, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08x\n", hr);
    ok(output[0] == 0x0, "Expected output buffer to contain NULL string\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 5, p1, p2, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(output, expected),
        "Combination of %s + %s returned %s, expected %s\n",
        wine_dbgstr_w(p1), wine_dbgstr_w(p2), wine_dbgstr_w(output), wine_dbgstr_w(expected));

    for (i = 0; i < sizeof(combine_test)/sizeof(combine_test[0]); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, combine_test[i].path1, -1, p1, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, combine_test[i].path2, -1, p2, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, combine_test[i].result, -1, expected, MAX_PATH);

        hr = pPathCchCombineEx(output, MAX_PATH, p1, p2, 0);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
        ok(!lstrcmpW(output, expected), "Combining %s with %s returned %s, expected %s\n",
            wine_dbgstr_w(p1), wine_dbgstr_w(p2), wine_dbgstr_w(output), wine_dbgstr_w(expected));
    }
}

START_TEST(path)
{
    HMODULE hmod = LoadLibraryA("kernelbase.dll");

    pPathCchCombineEx = (void *)GetProcAddress(hmod, "PathCchCombineEx");

    test_PathCchCombineEx();
}
