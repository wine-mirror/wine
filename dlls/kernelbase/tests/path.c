/*
 * Path tests for kernelbase.dll
 *
 * Copyright 2017 Michael Müller
 * Copyright 2018 Zhiyi Zhang
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

HRESULT (WINAPI *pPathCchAddBackslash)(WCHAR *out, SIZE_T size);
HRESULT (WINAPI *pPathCchAddBackslashEx)(WCHAR *out, SIZE_T size, WCHAR **endptr, SIZE_T *remaining);
HRESULT (WINAPI *pPathCchCombineEx)(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2, DWORD flags);
HRESULT (WINAPI *pPathCchFindExtension)(const WCHAR *path, SIZE_T size, const WCHAR **extension);

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

    for (i = 0; i < ARRAY_SIZE(combine_test); i++)
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

struct addbackslash_test
{
    const char *path;
    const char *result;
    HRESULT hr;
    SIZE_T size;
    SIZE_T remaining;
};

static const struct addbackslash_test addbackslash_tests[] =
{
    { "C:",    "C:\\",    S_OK, MAX_PATH, MAX_PATH - 3 },
    { "a.txt", "a.txt\\", S_OK, MAX_PATH, MAX_PATH - 6 },
    { "a/b",   "a/b\\",   S_OK, MAX_PATH, MAX_PATH - 4 },

    { "C:\\",  "C:\\",    S_FALSE, MAX_PATH, MAX_PATH - 3 },
    { "C:\\",  "C:\\",    S_FALSE, 4, 1 },

    { "C:",    "C:",      STRSAFE_E_INSUFFICIENT_BUFFER, 2, 0 },
    { "C:",    "C:",      STRSAFE_E_INSUFFICIENT_BUFFER, 3, 1 },
    { "C:\\",  "C:\\",    STRSAFE_E_INSUFFICIENT_BUFFER, 2, 0 },
    { "C:\\",  "C:\\",    STRSAFE_E_INSUFFICIENT_BUFFER, 3, 0 },
    { "C:\\",  "C:\\",    STRSAFE_E_INSUFFICIENT_BUFFER, 0, 0 },
    { "C:",    "C:",      STRSAFE_E_INSUFFICIENT_BUFFER, 0, 0 },
};

static void test_PathCchAddBackslash(void)
{
    WCHAR pathW[MAX_PATH];
    unsigned int i;
    HRESULT hr;

    if (!pPathCchAddBackslash)
    {
        win_skip("PathCchAddBackslash() is not available.\n");
        return;
    }

    pathW[0] = 0;
    hr = pPathCchAddBackslash(pathW, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#x.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    pathW[0] = 0;
    hr = pPathCchAddBackslash(pathW, 1);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    pathW[0] = 0;
    hr = pPathCchAddBackslash(pathW, 2);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    for (i = 0; i < ARRAY_SIZE(addbackslash_tests); i++)
    {
        const struct addbackslash_test *test = &addbackslash_tests[i];
        char path[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, test->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchAddBackslash(pathW, test->size);
        ok(hr == test->hr, "%u: unexpected return value %#x.\n", i, hr);

        WideCharToMultiByte(CP_ACP, 0, pathW, -1, path, ARRAY_SIZE(path), NULL, NULL);
        ok(!strcmp(path, test->result), "%u: unexpected resulting path %s.\n", i, path);
    }
}

static void test_PathCchAddBackslashEx(void)
{
    WCHAR pathW[MAX_PATH];
    SIZE_T remaining;
    unsigned int i;
    HRESULT hr;
    WCHAR *ptrW;

    if (!pPathCchAddBackslashEx)
    {
        win_skip("PathCchAddBackslashEx() is not available.\n");
        return;
    }

    pathW[0] = 0;
    hr = pPathCchAddBackslashEx(pathW, 0, NULL, NULL);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#x.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    pathW[0] = 0;
    ptrW = (void *)0xdeadbeef;
    remaining = 123;
    hr = pPathCchAddBackslashEx(pathW, 1, &ptrW, &remaining);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");
    ok(ptrW == pathW, "Unexpected endptr %p.\n", ptrW);
    ok(remaining == 1, "Unexpected remaining size.\n");

    pathW[0] = 0;
    hr = pPathCchAddBackslashEx(pathW, 2, NULL, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    for (i = 0; i < ARRAY_SIZE(addbackslash_tests); i++)
    {
        const struct addbackslash_test *test = &addbackslash_tests[i];
        char path[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, test->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchAddBackslashEx(pathW, test->size, NULL, NULL);
        ok(hr == test->hr, "%u: unexpected return value %#x.\n", i, hr);

        WideCharToMultiByte(CP_ACP, 0, pathW, -1, path, ARRAY_SIZE(path), NULL, NULL);
        ok(!strcmp(path, test->result), "%u: unexpected resulting path %s.\n", i, path);

        ptrW = (void *)0xdeadbeef;
        remaining = 123;
        MultiByteToWideChar(CP_ACP, 0, test->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchAddBackslashEx(pathW, test->size, &ptrW, &remaining);
        ok(hr == test->hr, "%u: unexpected return value %#x.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(ptrW == (pathW + lstrlenW(pathW)), "%u: unexpected end pointer.\n", i);
            ok(remaining == test->remaining, "%u: unexpected remaining buffer length.\n", i);
        }
        else
        {
            ok(ptrW == NULL, "%u: unexpecred end pointer.\n", i);
            ok(remaining == 0, "%u: unexpected remaining buffer length.\n", i);
        }
    }
}

struct findextension_test
{
    const CHAR *path;
    INT extension_offset;
};

static const struct findextension_test findextension_tests[] =
{
    /* Normal */
    {"1.exe", 1},
    {"C:1.exe", 3},
    {"C:\\1.exe", 4},
    {"\\1.exe", 2},
    {"\\\\1.exe", 3},
    {"\\\\?\\C:1.exe", 7},
    {"\\\\?\\C:\\1.exe", 8},
    {"\\\\?\\UNC\\1.exe", 9},
    {"\\\\?\\UNC\\192.168.1.1\\1.exe", 21},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1.exe", 50},

    /* Contains forward slash */
    {"C:\\a/1.exe", 6},
    {"/1.exe", 2},
    {"//1.exe", 3},
    {"C:\\a/b/1.exe", 8},
    {"/a/1.exe", 4},
    {"/a/1.exe", 4},

    /* Malformed */
    {"", 0},
    {" ", 1},
    {".", 0},
    {"..", 1},
    {"a", 1},
    {"a.", 1},
    {".a.b.", 4},
    {"a. ", 3},
    {"a.\\", 3},
    {"\\\\?\\UNC\\192.168.1.1", 17},
    {"\\\\?\\UNC\\192.168.1.1\\", 20},
    {"\\\\?\\UNC\\192.168.1.1\\a", 21}
};

static void test_PathCchFindExtension(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH + 1] = {0};
    const WCHAR *extension;
    HRESULT hr;
    INT i;

    if (!pPathCchFindExtension)
    {
        win_skip("PathCchFindExtension() is not available.\n");
        return;
    }

    /* Arguments check */
    extension = (const WCHAR *)0xdeadbeef;
    hr = pPathCchFindExtension(NULL, PATHCCH_MAX_CCH, &extension);
    ok(hr == E_INVALIDARG, "expect result %#x, got %#x\n", E_INVALIDARG, hr);
    ok(extension == NULL, "Expect extension null, got %p\n", extension);

    extension = (const WCHAR *)0xdeadbeef;
    hr = pPathCchFindExtension(pathW, 0, &extension);
    ok(hr == E_INVALIDARG, "expect result %#x, got %#x\n", E_INVALIDARG, hr);
    ok(extension == NULL, "Expect extension null, got %p\n", extension);

    /* Crashed on Windows */
    if (0)
    {
        hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, NULL);
        ok(hr == E_INVALIDARG, "expect result %#x, got %#x\n", E_INVALIDARG, hr);
    }

    /* Path length check */
    /* size == PATHCCH_MAX_CCH + 1 */
    MultiByteToWideChar(CP_ACP, 0, "C:\\1.exe", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH + 1, &extension);
    ok(hr == E_INVALIDARG, "expect result %#x, got %#x\n", E_INVALIDARG, hr);

    /* Size == path length + 1*/
    hr = pPathCchFindExtension(pathW, ARRAY_SIZE("C:\\1.exe"), &extension);
    ok(hr == S_OK, "expect result %#x, got %#x\n", S_OK, hr);
    ok(*extension == '.', "wrong extension value\n");

    /* Size < path length + 1 */
    extension = (const WCHAR *)0xdeadbeef;
    hr = pPathCchFindExtension(pathW, ARRAY_SIZE("C:\\1.exe") - 1, &extension);
    ok(hr == E_INVALIDARG, "expect result %#x, got %#x\n", E_INVALIDARG, hr);
    ok(extension == NULL, "Expect extension null, got %p\n", extension);

    /* Size == PATHCCH_MAX_CCH */
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
    ok(hr == S_OK, "expect result %#x, got %#x\n", S_OK, hr);

    /* Path length + 1 > PATHCCH_MAX_CCH */
    for (i = 0; i < ARRAY_SIZE(pathW) - 1; i++) pathW[i] = 'a';
    pathW[PATHCCH_MAX_CCH] = 0;
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
    ok(hr == E_INVALIDARG, "expect result %#x, got %#x\n", E_INVALIDARG, hr);

    /* Path length + 1 == PATHCCH_MAX_CCH */
    pathW[PATHCCH_MAX_CCH - 1] = 0;
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
    ok(hr == S_OK, "expect result %#x, got %#x\n", S_OK, hr);

    for (i = 0; i < ARRAY_SIZE(findextension_tests); i++)
    {
        const struct findextension_test *t = findextension_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
        ok(hr == S_OK, "path %s expect result %#x, got %#x\n", t->path, S_OK, hr);
        if (SUCCEEDED(hr))
            ok(extension - pathW == t->extension_offset, "path %s expect extension offset %d, got %ld\n", t->path,
               t->extension_offset, (UINT_PTR)(extension - pathW));
    }
}

START_TEST(path)
{
    HMODULE hmod = LoadLibraryA("kernelbase.dll");

    pPathCchCombineEx = (void *)GetProcAddress(hmod, "PathCchCombineEx");
    pPathCchAddBackslash = (void *)GetProcAddress(hmod, "PathCchAddBackslash");
    pPathCchAddBackslashEx = (void *)GetProcAddress(hmod, "PathCchAddBackslashEx");
    pPathCchFindExtension = (void *)GetProcAddress(hmod, "PathCchFindExtension");

    test_PathCchCombineEx();
    test_PathCchAddBackslash();
    test_PathCchAddBackslashEx();
    test_PathCchFindExtension();
}
