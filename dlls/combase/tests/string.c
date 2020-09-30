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

static HRESULT (WINAPI *pWindowsCompareStringOrdinal)(HSTRING, HSTRING, INT32 *);
static HRESULT (WINAPI *pWindowsConcatString)(HSTRING, HSTRING, HSTRING *);
static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsCreateStringReference)(LPCWSTR, UINT32, HSTRING_HEADER *, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);
static HRESULT (WINAPI *pWindowsDeleteStringBuffer)(HSTRING_BUFFER);
static HRESULT (WINAPI *pWindowsDuplicateString)(HSTRING, HSTRING *);
static UINT32  (WINAPI *pWindowsGetStringLen)(HSTRING);
static LPCWSTR (WINAPI *pWindowsGetStringRawBuffer)(HSTRING, UINT32 *);
static BOOL    (WINAPI *pWindowsIsStringEmpty)(HSTRING);
static HRESULT (WINAPI *pWindowsPreallocateStringBuffer)(UINT32, WCHAR **, HSTRING_BUFFER *);
static HRESULT (WINAPI *pWindowsPromoteStringBuffer)(HSTRING_BUFFER, HSTRING *);
static HRESULT (WINAPI *pWindowsStringHasEmbeddedNull)(HSTRING, BOOL *);
static HRESULT (WINAPI *pWindowsSubstring)(HSTRING, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsSubstringWithSpecifiedLength)(HSTRING, UINT32, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsTrimStringEnd)(HSTRING, HSTRING, HSTRING *);
static HRESULT (WINAPI *pWindowsTrimStringStart)(HSTRING, HSTRING, HSTRING *);

#define SET(x) p##x = (void*)GetProcAddress(hmod, #x)

static BOOL init_functions(void)
{
    HMODULE hmod = LoadLibraryA("combase.dll");
    if (!hmod)
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return FALSE;
    }
    SET(WindowsCompareStringOrdinal);
    SET(WindowsConcatString);
    SET(WindowsCreateString);
    SET(WindowsCreateStringReference);
    SET(WindowsDeleteString);
    SET(WindowsDeleteStringBuffer);
    SET(WindowsDuplicateString);
    SET(WindowsGetStringLen);
    SET(WindowsGetStringRawBuffer);
    SET(WindowsIsStringEmpty);
    SET(WindowsPreallocateStringBuffer);
    SET(WindowsPromoteStringBuffer);
    SET(WindowsStringHasEmbeddedNull);
    SET(WindowsSubstring);
    SET(WindowsSubstringWithSpecifiedLength);
    SET(WindowsTrimStringEnd);
    SET(WindowsTrimStringStart);
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

static const WCHAR input_string[] = L"abcdef\0";
static const WCHAR input_string1[] = L"abc";
static const WCHAR input_string2[] = L"def";
static const WCHAR input_embed_null[] = L"a\0c\0ef";
static const WCHAR output_substring[] = L"cdef";

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
    /* If the input string is non-null, it must be null-terminated even if the
     * length is zero. */
    ok(FAILED(pWindowsCreateStringReference(input_string, 0, &header, &str)), "Incorrect error handling\n");
    ok(pWindowsCreateStringReference(input_string, 6, NULL, &str) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCreateStringReference(input_string, 6, &header, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCreateStringReference(NULL, 6, &header, &str) == E_POINTER, "Incorrect error handling\n");

    /* Test creating a string without a null-termination at the specified length */
    ok(pWindowsCreateString(input_string, 3, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_string, 3, FALSE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test an empty string */
    ok(pWindowsCreateString(L"", 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateString(input_string, 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateStringReference(L"", 0, &header, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateString(NULL, 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsCreateStringReference(NULL, 0, &header, &str) == S_OK, "Failed to create string\n");
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

static void test_string_buffer(void)
{
    /* Initialize ptr to NULL to make sure it actually is set in the first
     * test below. */
    HSTRING_BUFFER buf = NULL;
    WCHAR *ptr = NULL;
    HSTRING str;

    /* Test creation of an empty buffer */
    ok(pWindowsPreallocateStringBuffer(0, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ok(ptr != NULL, "Empty string didn't return a buffer pointer\n");
    ok(pWindowsPromoteStringBuffer(buf, &str) == S_OK, "Failed to promote string buffer\n");
    ok(str == NULL, "Empty string isn't a null string\n");
    check_string(str, L"", 0, FALSE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteStringBuffer(NULL) == S_OK, "Failed to delete null string buffer\n");

    /* Test creation and deletion of string buffers */
    ok(pWindowsPreallocateStringBuffer(6, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ok(pWindowsDeleteStringBuffer(buf) == S_OK, "Failed to delete string buffer\n");

    /* Test creation and promotion of string buffers */
    ok(pWindowsPreallocateStringBuffer(6, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ok(ptr[6] == '\0', "Preallocated string buffer didn't have null termination\n");
    memcpy(ptr, input_string, 6 * sizeof(*input_string));
    ok(pWindowsPromoteStringBuffer(buf, NULL) == E_POINTER, "Incorrect error handling\n");
    ok(pWindowsPromoteStringBuffer(buf, &str) == S_OK, "Failed to promote string buffer\n");
    check_string(str, input_string, 6, FALSE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test error handling in preallocation */
    ok(pWindowsPreallocateStringBuffer(6, NULL, &buf) == E_POINTER, "Incorrect error handling\n");
    ok(pWindowsPreallocateStringBuffer(6, &ptr, NULL) == E_POINTER, "Incorrect error handling\n");

    ok(pWindowsPreallocateStringBuffer(6, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ptr[6] = 'a'; /* Overwrite the buffer's null termination, promotion should fail */
    ok(pWindowsPromoteStringBuffer(buf, &str) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsDeleteStringBuffer(buf) == S_OK, "Failed to delete string buffer\n");

    /* Test strings with trailing null chars */
    ok(pWindowsPreallocateStringBuffer(7, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    memcpy(ptr, input_string, 7 * sizeof(*input_string));
    ok(pWindowsPromoteStringBuffer(buf, &str) == S_OK, "Failed to promote string buffer\n");
    check_string(str, input_string, 7, TRUE);
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
}

static void test_substring(void)
{
    HSTRING str, substr;
    HSTRING_HEADER header;

    /* Test substring of string buffers */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsSubstring(str, 2, &substr) == S_OK, "Failed to create substring\n");
    check_string(substr, output_substring, 4, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 2, 3, &substr) == S_OK, "Failed to create substring\n");
    check_string(substr, output_substring, 3, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test duplication of string using substring */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsSubstring(str, 0, &substr) == S_OK, "Failed to create substring\n");
    ok(str != substr, "Duplicated string didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 0, 6, &substr) == S_OK, "Failed to create substring\n");
    ok(str != substr, "Duplicated string didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test substring of string reference */
    ok(pWindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(pWindowsSubstring(str, 2, &substr) == S_OK, "Failed to create substring of string ref\n");
    check_string(substr, output_substring, 4, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 2, 3, &substr) == S_OK, "Failed to create substring of string ref\n");
    check_string(substr, output_substring, 3, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test duplication of string reference using substring */
    ok(pWindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(pWindowsSubstring(str, 0, &substr) == S_OK, "Failed to create substring of string ref\n");
    ok(str != substr, "Duplicated string ref didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 0, 6, &substr) == S_OK, "Failed to create substring of string ref\n");
    ok(str != substr, "Duplicated string ref didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(pWindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test get substring of empty string */
    ok(pWindowsSubstring(NULL, 0, &substr) == S_OK, "Failed to duplicate NULL string\n");
    ok(substr == NULL, "Substring created new string\n");
    ok(pWindowsSubstringWithSpecifiedLength(NULL, 0, 0, &substr) == S_OK, "Failed to duplicate NULL string\n");
    ok(substr == NULL, "Substring created new string\n");

    /* Test get empty substring of string */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsSubstring(str, 6, &substr) == S_OK, "Failed to create substring\n");
    ok(substr == NULL, "Substring created new string\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 6, 0, &substr) == S_OK, "Failed to create substring\n");
    ok(substr == NULL, "Substring created new string\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test handling of using too high start index or length */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsSubstring(str, 7, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 7, 0, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 6, 1, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 7, ~0U, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test handling of a NULL string  */
    ok(pWindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(pWindowsSubstring(str, 7, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsSubstringWithSpecifiedLength(str, 7, 0, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsDeleteString(str) == S_OK, "Failed to delete string\n");
}

static void test_concat(void)
{
    HSTRING str1, str2, concat;
    HSTRING_HEADER header1, header2;

    /* Test concatenation of string buffers */
    ok(pWindowsCreateString(input_string1, 3, &str1) == S_OK, "Failed to create string\n");
    ok(pWindowsCreateString(input_string2, 3, &str2) == S_OK, "Failed to create string\n");

    ok(pWindowsConcatString(str1, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsConcatString(str1, NULL, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str1 == concat, "Concatenate created new string\n");
    check_string(concat, input_string1, 3, FALSE);
    ok(pWindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(pWindowsConcatString(NULL, str2, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsConcatString(NULL, str2, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str2 == concat, "Concatenate created new string\n");
    check_string(concat, input_string2, 3, FALSE);
    ok(pWindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(pWindowsConcatString(str1, str2, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsConcatString(str1, str2, &concat) == S_OK, "Failed to concatenate string\n");
    check_string(concat, input_string, 6, FALSE);
    ok(pWindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string\n");

    /* Test concatenation of string references */
    ok(pWindowsCreateStringReference(input_string1, 3, &header1, &str1) == S_OK, "Failed to create string ref\n");
    ok(pWindowsCreateStringReference(input_string2, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(pWindowsConcatString(str1, NULL, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str1 != concat, "Concatenate string ref didn't create new string\n");
    check_string(concat, input_string1, 3, FALSE);
    ok(pWindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(pWindowsConcatString(NULL, str2, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str2 != concat, "Concatenate string ref didn't create new string\n");
    check_string(concat, input_string2, 3, FALSE);
    ok(pWindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(pWindowsConcatString(str1, str2, &concat) == S_OK, "Failed to concatenate string\n");
    check_string(concat, input_string, 6, FALSE);
    ok(pWindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string ref\n");

    /* Test concatenation of two empty strings */
    ok(pWindowsConcatString(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsConcatString(NULL, NULL, &concat) == S_OK, "Failed to concatenate string\n");
    ok(concat == NULL, "Concatenate created new string\n");
}

static void test_compare(void)
{
    HSTRING str1, str2;
    HSTRING_HEADER header1, header2;
    INT32 res;

    /* Test comparison of string buffers */
    ok(pWindowsCreateString(input_string1, 3, &str1) == S_OK, "Failed to create string\n");
    ok(pWindowsCreateString(input_string2, 3, &str2) == S_OK, "Failed to create string\n");

    ok(pWindowsCompareStringOrdinal(str1, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str1, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str2, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str2, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str1, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(NULL, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str2, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(NULL, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string\n");

    /* Test comparison of string references */
    ok(pWindowsCreateStringReference(input_string1, 3, &header1, &str1) == S_OK, "Failed to create string ref\n");
    ok(pWindowsCreateStringReference(input_string2, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(pWindowsCompareStringOrdinal(str1, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str1, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str2, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str2, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str1, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(NULL, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(str2, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(pWindowsCompareStringOrdinal(NULL, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string ref\n");

    /* Test comparison of two empty strings */
    ok(pWindowsCompareStringOrdinal(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsCompareStringOrdinal(NULL, NULL, &res) == S_OK, "Failed to compare NULL string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
}

static void test_trim(void)
{
    HSTRING str1, str2, trimmed;
    HSTRING_HEADER header1, header2;

    /* Test trimming of string buffers */
    ok(pWindowsCreateString(input_string, 6, &str1) == S_OK, "Failed to create string\n");
    ok(pWindowsCreateString(input_string1, 3, &str2) == S_OK, "Failed to create string\n");

    ok(pWindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string2, 3, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(pWindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed == str1, "Trimmed string created new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(pWindowsCreateString(input_string2, 3, &str2) == S_OK, "Failed to create string\n");

    ok(pWindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed == str1, "Trimmed string created new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(pWindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string1, 3, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string\n");

    /* Test trimming of string references */
    ok(pWindowsCreateStringReference(input_string, 6, &header1, &str1) == S_OK, "Failed to create string ref\n");
    ok(pWindowsCreateStringReference(input_string1, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(pWindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string2, 3, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(pWindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed != str1, "Trimmed string ref didn't create new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(pWindowsCreateStringReference(input_string2, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(pWindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed != str1, "Trimmed string ref didn't create new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(pWindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string1, 3, FALSE);
    ok(pWindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(pWindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string ref\n");

    /* Test handling of NULL strings */
    ok(pWindowsCreateString(input_string, 6, &str1) == S_OK, "Failed to create string\n");
    ok(pWindowsTrimStringStart(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsTrimStringStart(NULL, str1, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsTrimStringStart(NULL, NULL, &trimmed) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsTrimStringStart(NULL, str1, &trimmed) == S_OK, "Failed to trim empty string\n");
    ok(trimmed == NULL, "Trimming created new string\n");
    ok(pWindowsTrimStringEnd(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsTrimStringEnd(NULL, str1, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsTrimStringEnd(NULL, NULL, &trimmed) == E_INVALIDARG, "Incorrect error handling\n");
    ok(pWindowsTrimStringEnd(NULL, str1, &trimmed) == S_OK, "Failed to trim empty string\n");
    ok(trimmed == NULL, "Trimming created new string\n");
    ok(pWindowsDeleteString(str1) == S_OK, "Failed to delete string\n");
}

START_TEST(string)
{
    if (!init_functions())
        return;
    test_create_delete();
    test_duplicate();
    test_access();
    test_string_buffer();
    test_substring();
    test_concat();
    test_compare();
    test_trim();
}
