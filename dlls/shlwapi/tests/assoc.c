/* Unit test suite for SHLWAPI IQueryAssociations functions
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include "wine/test.h"
#include "shlwapi.h"

#define expect(expected, got) ok ( expected == got, "Expected %d, got %d\n", expected, got)
#define expect_hr(expected, got) ok ( expected == got, "Expected %08x, got %08x\n", expected, got)

/* Every version of Windows with IE should have this association? */
static const WCHAR dotHtml[] = { '.','h','t','m','l',0 };
static const WCHAR badBad[] = { 'b','a','d','b','a','d',0 };
static const WCHAR dotBad[] = { '.','b','a','d',0 };
static const WCHAR open[] = { 'o','p','e','n',0 };
static const WCHAR invalid[] = { 'i','n','v','a','l','i','d',0 };

static void test_getstring_bad(void)
{
    HRESULT hr;
    DWORD len;

    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, NULL, open, NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, badBad, open, NULL, &len);
    expect_hr(E_FAIL, hr);
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotBad, open, NULL, &len);
    expect_hr(E_FAIL, hr);
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, invalid, NULL,
                           &len);
    expect_hr(0x80070002, hr); /* NOT FOUND */
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open, NULL, NULL);
    expect_hr(E_UNEXPECTED, hr);

    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, NULL, open, NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, badBad, open, NULL,
                           &len);
    expect_hr(E_FAIL, hr);
    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotBad, open, NULL,
                           &len);
    expect_hr(E_FAIL, hr);
    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, invalid, NULL,
                           &len);
    todo_wine expect_hr(0x80070002, hr); /* NOT FOUND */
    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open, NULL,
                           NULL);
    expect_hr(E_UNEXPECTED, hr);
}

static void test_getstring_basic(void)
{
    HRESULT hr;
    WCHAR * friendlyName;
    WCHAR * executableName;
    DWORD len, len2, slen;

    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open, NULL, &len);
    expect_hr(S_FALSE, hr);
    if (hr != S_FALSE)
    {
        skip("failed to get initial len\n");
        return;
    }

    executableName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               len * sizeof(WCHAR));
    if (!executableName)
    {
        skip("failed to allocate memory\n");
        return;
    }

    len2 = len;
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open,
                           executableName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(executableName) + 1;
    expect(len, len2);
    expect(len, slen);

    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open, NULL,
                           &len);
    todo_wine expect_hr(S_FALSE, hr);
    if (hr != S_FALSE)
    {
        HeapFree(GetProcessHeap(), 0, executableName);
        skip("failed to get initial len\n");
        return;
    }

    friendlyName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               len * sizeof(WCHAR));
    if (!friendlyName)
    {
        HeapFree(GetProcessHeap(), 0, executableName);
        skip("failed to allocate memory\n");
        return;
    }

    len2 = len;
    hr = AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open,
                           friendlyName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(friendlyName) + 1;
    expect(len, len2);
    expect(len, slen);

    HeapFree(GetProcessHeap(), 0, executableName);
    HeapFree(GetProcessHeap(), 0, friendlyName);
}

START_TEST(assoc)
{
    test_getstring_bad();
    test_getstring_basic();
}
