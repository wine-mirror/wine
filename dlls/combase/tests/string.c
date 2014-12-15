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
static UINT32  (WINAPI *pWindowsGetStringLen)(HSTRING);
static LPCWSTR (WINAPI *pWindowsGetStringRawBuffer)(HSTRING, UINT32 *);
static BOOL    (WINAPI *pWindowsIsStringEmpty)(HSTRING);
static HRESULT (WINAPI *pWindowsStringHasEmbeddedNull)(HSTRING, BOOL *);

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
    SET(WindowsGetStringLen);
    SET(WindowsGetStringRawBuffer);
    SET(WindowsIsStringEmpty);
    SET(WindowsStringHasEmbeddedNull);
    return TRUE;
}

#undef SET


#define check_string(str, content, length, has_null) _check_string(__LINE__, str, content, length, has_null)
static void _check_string(int line, HSTRING str, LPCWSTR content, UINT32 length, BOOL has_null)
{
    BOOL out_null;
    BOOL empty = length == 0;
    UINT32 out_length;
    LPCWSTR ptr;

    ok_(__FILE__, line)(pWindowsIsStringEmpty(str) == empty, "WindowsIsStringEmpty failed\n");
    ok_(__FILE__, line)(pWindowsStringHasEmbeddedNull(str, &out_null) == S_OK, "pWindowsStringHasEmbeddedNull failed\n");
    ok_(__FILE__, line)(out_null == has_null, "WindowsStringHasEmbeddedNull failed\n");
    ok_(__FILE__, line)(pWindowsGetStringLen(str) == length, "WindowsGetStringLen failed\n");
    ptr = pWindowsGetStringRawBuffer(str, &out_length);
    /* WindowsGetStringRawBuffer should return a non-null, null terminated empty string
     * even if str is NULL. */
    ok_(__FILE__, line)(ptr != NULL, "WindowsGetStringRawBuffer returned null\n");
    ok_(__FILE__, line)(out_length == length, "WindowsGetStringRawBuffer returned incorrect length\n");
    ptr = pWindowsGetStringRawBuffer(str, NULL);
    ok_(__FILE__, line)(ptr != NULL, "WindowsGetStringRawBuffer returned null\n");
    ok_(__FILE__, line)(ptr[length] == '\0', "WindowsGetStringRawBuffer doesn't return a null terminated buffer\n");
    ok_(__FILE__, line)(memcmp(ptr, content, sizeof(*content) * length) == 0, "Incorrect string content\n");
}

static const WCHAR input_string[] = { 'a', 'b', 'c', 'd', 'e', 'f', '\0', '\0' };
static const WCHAR input_empty_string[] = { '\0' };
static const WCHAR input_embed_null[] = { 'a', '\0', 'c', '\0', 'e', 'f', '\0' };

static void test_create_delete(void)
{
    HSTRING str;
    HSTRING_HEADER header;

    /* Test normal creation of a string */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_string, 6, FALSE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    /* Test error handling in WindowsCreateString */
    ok(pWindowsCreateString(input_string, 6, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCreateString(NULL, 6, &str) == E_POINTER, "Incorrect error handling\n");

    /* Test handling of a NULL string */
    ok(pWindowsDeleteString(NULL) == S_OK, "Failed to delete null string\n");

    /* Test creation of a string reference */
    ok(pWindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    check_string(str, input_string, 6, FALSE);
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
    check_string(str, input_string, 3, FALSE);
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

static void test_access(void)
{
    HSTRING str;
    HSTRING_HEADER header;

    /* Test handling of a NULL string */
    check_string(NULL, NULL, 0, FALSE);

    /* Test strings with embedded null chars */
    ok(pWindowsCreateString(input_embed_null, 6, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_embed_null, 6, TRUE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateStringReference(input_embed_null, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    check_string(str, input_embed_null, 6, TRUE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test normal creation of a string with trailing null */
    ok(pWindowsCreateString(input_string, 7, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_string, 7, TRUE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateStringReference(input_string, 7, &header, &str) == S_OK, "Failed to create string ref\n");
    check_string(str, input_string, 7, TRUE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");
}

START_TEST(string)
{
    if (!init_functions())
        return;
    test_create_delete();
    test_duplicate();
    test_access();
}
