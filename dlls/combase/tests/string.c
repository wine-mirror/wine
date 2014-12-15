/*
 * Unit tests for Windows String functions
 *
 * Copyright (c) 2014 Martin Storsjo
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "wine/test.h"

static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsCreateStringReference)(LPCWSTR, UINT32, HSTRING_HEADER *, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);
static HRESULT (WINAPI *pWindowsDuplicateString)(HSTRING, HSTRING *);

#define SET(x) p##x = (void*)GetProcAddress(hmod, #x)

static BOOL init_functions(void)
{
    HMODULE hmod = LoadLibraryA("combase.dll");
    if (!hmod)
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return FALSE;
    }
    SET(WindowsCreateString);
    SET(WindowsCreateStringReference);
    SET(WindowsDeleteString);
    SET(WindowsDuplicateString);
    return TRUE;
}

#undef SET


static const WCHAR input_string[] = { 'a', 'b', 'c', 'd', 'e', 'f', '\0' };
static const WCHAR input_empty_string[] = { '\0' };

static void test_create_delete(void)
{
    HSTRING str;
    HSTRING_HEADER header;

    /* Test normal creation of a string */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    /* Test error handling in WindowsCreateString */
    ok(pWindowsCreateString(input_string, 6, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCreateString(NULL, 6, &str) == E_POINTER, "Incorrect error handling\n");

    /* Test handling of a NULL string */
    ok(pWindowsDeleteString(NULL) == S_OK, "Failed to delete null string\n");

    /* Test creation of a string reference */
    ok(pWindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test error handling in WindowsCreateStringReference */
    /* Strings to CreateStringReference must be null terminated with the correct
     * length. According to MSDN this should be E_INVALIDARG, but it returns
     * 0x80000017 in practice. */
    ok(FAILED(pWindowsCreateStringReference(input_string, 5, &header, &str)), "Incorrect error handling\n");
    ok(pWindowsCreateStringReference(input_string, 6, NULL, &str) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCreateStringReference(input_string, 6, &header, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCreateStringReference(NULL, 6, &header, &str) == E_POINTER, "Incorrect error handling\n");

    /* Test creating a string without a null-termination at the specified length */
    ok(pWindowsCreateString(input_string, 3, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test an empty string */
    ok(pWindowsCreateString(input_empty_string, 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateStringReference(input_empty_string, 0, &header, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
}

static void test_duplicate(void)
{
    HSTRING str, str2;
    HSTRING_HEADER header;
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsDuplicateString(str, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsDuplicateString(str, &str2) == S_OK, "Failed to duplicate string\n");
    ok(str == str2, "Duplicated string created new string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(pWindowsDuplicateString(str, &str2) == S_OK, "Failed to duplicate string\n");
    ok(str != str2, "Duplicated string ref didn't create new string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");

    ok(pWindowsDuplicateString(NULL, &str2) == S_OK, "Failed to duplicate NULL string\n");
    ok(str2 == NULL, "Duplicated string created new string\n");
    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
}

START_TEST(string)
{
    if (!init_functions())
        return;
    test_create_delete();
    test_duplicate();
}
