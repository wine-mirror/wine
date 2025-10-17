/*
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "winstring.h"

#include "wine/test.h"

HRESULT WINAPI GetFontFallbackLanguageList(const WCHAR *, size_t, WCHAR *, size_t *);
DWORD WINAPI GetUserLanguages(WCHAR, HSTRING *);

static void test_GetFontFallbackLanguageList(void)
{
    size_t required_size;
    WCHAR buffer[128];
    HRESULT hr;

    /* Parameter checks */
    required_size = 0xdeadbeef;
    hr = GetFontFallbackLanguageList(NULL, ARRAY_SIZE(buffer), buffer, &required_size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(required_size != 0xdeadbeef, "Got unexpected size %Iu.\n", required_size);

    hr = GetFontFallbackLanguageList(L"en-US", 0, buffer, &required_size);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    required_size = 0;
    hr = GetFontFallbackLanguageList(L"en-US", 1, buffer, &required_size);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Got unexpected hr %#lx.\n", hr);
    ok(required_size > 0, "Got unexpected size %Iu.\n", required_size);

    hr = GetFontFallbackLanguageList(L"en-US", ARRAY_SIZE(buffer), NULL, &required_size);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    if (0) /* Crashes on Windows */
    {
    hr = GetFontFallbackLanguageList(L"en-US", ARRAY_SIZE(buffer), buffer, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    }

    hr = GetFontFallbackLanguageList(L"deadbeef", ARRAY_SIZE(buffer), buffer, &required_size);
    todo_wine
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Normal call */
    required_size = 0;
    hr = GetFontFallbackLanguageList(L"en-US", ARRAY_SIZE(buffer), buffer, &required_size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wcsncmp(buffer, L"en-US", wcslen(L"en-US")), "Got unexpected %s.\n", wine_dbgstr_w(buffer));
    ok(required_size >= (wcslen(L"en-US") + 1), "Got unexpected size %Iu.\n", required_size);
}

static void test_GetUserLanguages(void)
{
    HSTRING usrlangs = NULL;
    const WCHAR *langs;
    int count = 0;
    HRESULT hr;

    hr = GetUserLanguages(';', &usrlangs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!!usrlangs, "Unexpected pointer.\n");
    if (!usrlangs) return;

    langs = WindowsGetStringRawBuffer(usrlangs, NULL);

    for (WCHAR *p = wcstok(wcsdup(langs), L";"); p; p = wcstok(NULL, L";")) count++;
    todo_wine ok(count > 0, "Got count=%d.\n", count);
    WindowsDeleteString(usrlangs);
}

START_TEST(bcp47langs)
{
    test_GetFontFallbackLanguageList();
    test_GetUserLanguages();
}
