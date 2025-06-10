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

HRESULT (WINAPI *pPathAllocCanonicalize)(const WCHAR *path_in, DWORD flags, WCHAR **path_out);
HRESULT (WINAPI *pPathAllocCombine)(const WCHAR *path1, const WCHAR *path2, DWORD flags, WCHAR **out);
HRESULT (WINAPI *pPathCchAddBackslash)(WCHAR *out, SIZE_T size);
HRESULT (WINAPI *pPathCchAddBackslashEx)(WCHAR *out, SIZE_T size, WCHAR **endptr, SIZE_T *remaining);
HRESULT (WINAPI *pPathCchAddExtension)(WCHAR *path, SIZE_T size, const WCHAR *extension);
HRESULT (WINAPI *pPathCchAppend)(WCHAR *path1, SIZE_T size, const WCHAR *path2);
HRESULT (WINAPI *pPathCchAppendEx)(WCHAR *path1, SIZE_T size, const WCHAR *path2, DWORD flags);
HRESULT (WINAPI *pPathCchCanonicalize)(WCHAR *out, SIZE_T size, const WCHAR *in);
HRESULT (WINAPI *pPathCchCanonicalizeEx)(WCHAR *out, SIZE_T size, const WCHAR *in, DWORD flags);
HRESULT (WINAPI *pPathCchCombine)(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2);
HRESULT (WINAPI *pPathCchCombineEx)(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2, DWORD flags);
HRESULT (WINAPI *pPathCchFindExtension)(const WCHAR *path, SIZE_T size, const WCHAR **extension);
BOOL    (WINAPI *pPathCchIsRoot)(const WCHAR *path);
HRESULT (WINAPI *pPathCchRemoveBackslash)(WCHAR *path, SIZE_T path_size);
HRESULT (WINAPI *pPathCchRemoveBackslashEx)(WCHAR *path, SIZE_T path_size, WCHAR **path_end, SIZE_T *free_size);
HRESULT (WINAPI *pPathCchRemoveExtension)(WCHAR *path, SIZE_T size);
HRESULT (WINAPI *pPathCchRemoveFileSpec)(WCHAR *path, SIZE_T size);
HRESULT (WINAPI *pPathCchRenameExtension)(WCHAR *path, SIZE_T size, const WCHAR *extension);
HRESULT (WINAPI *pPathCchSkipRoot)(const WCHAR *path, const WCHAR **root_end);
HRESULT (WINAPI *pPathCchStripPrefix)(WCHAR *path, SIZE_T size);
HRESULT (WINAPI *pPathCchStripToRoot)(WCHAR *path, SIZE_T size);
BOOL    (WINAPI *pPathIsUNCEx)(const WCHAR *path, const WCHAR **server);

BOOL (WINAPI *pPathIsRootA)(const char *path);
BOOL (WINAPI *pPathIsRootW)(const WCHAR *path);
BOOL (WINAPI *pPathStripToRootA)(char *path);
BOOL (WINAPI *pPathStripToRootW)(WCHAR *path);
BOOL (WINAPI *pPathRemoveFileSpecA)(char *path);
BOOL (WINAPI *pPathRemoveFileSpecW)(WCHAR *path);

struct alloccanonicalize_test
{
    const CHAR *path_in;
    const CHAR *path_out;
    DWORD flags;
    HRESULT hr;
};

static const struct alloccanonicalize_test alloccanonicalize_tests[] =
{
    /* Malformed path */
    {"C:a", "C:a", 0, S_OK},
    {"\\\\?\\C:", "C:\\", 0, S_OK},
    {"\\\\?C:\\a", "\\\\?C:\\a", 0, S_OK},
    {"\\\\?UNC\\a", "\\\\?UNC\\a", 0, S_OK},
    {"\\\\?\\UNCa", "\\\\?\\UNCa", 0, S_OK},
    {"\\\\?C:a", "\\\\?C:a", 0, S_OK},

    /* No . */
    {"*", "*", 0, S_OK},
    {"", "\\", 0, S_OK},
    {"C:", "C:", 0, S_OK},
    {"C:\\", "C:\\", 0, S_OK},
    {"\\\\?\\C:\\a", "C:\\a", 0, S_OK},
    {"\\\\?\\UNC\\a", "\\\\a", 0, S_OK},

    /* . */
    {".", "\\", 0, S_OK},
    {"..", "\\", 0, S_OK},
    {"...", "\\", 0, S_OK},
    {"*.", "*.", 0, S_OK},
    {"*.\\", "*.\\", 0, S_OK},
    {"*.\\", "*.\\", 0, S_OK},
    {"*..", "*.", 0, S_OK},
    {"*..\\", "*..\\", 0, S_OK},
    {"*...", "*.", 0, S_OK},
    {"*...\\", "*...\\", 0, S_OK},
    {"*....", "*.", 0, S_OK},
    {"*....\\", "*....\\", 0, S_OK},
    {".a", ".a", 0, S_OK},
    {".a\\", ".a\\", 0, S_OK},
    {"a.", "a", 0, S_OK},
    {"a.\\", "a.\\", 0, S_OK},
    {".a.", ".a", 0, S_OK},
    {"a.b", "a.b", 0, S_OK},
    {".a.b.", ".a.b", 0, S_OK},
    {"a\\.", "a", 0, S_OK},
    {"a\\.\\b", "a\\b", 0, S_OK},
    {"a\\.b", "a\\.b", 0, S_OK},
    {"a\\.b\\", "a\\.b\\", 0, S_OK},
    {":.", ":", 0, S_OK},
    {"::.", "::\\", 0, S_OK},
    {":::.", ":::", 0, S_OK},
    {"C:.", "C:\\", 0, S_OK},
    {"C:.\\", "C:.\\", 0, S_OK},
    {"C:.\\.", "C:\\", 0, S_OK},
    {"C:\\.", "C:\\", 0, S_OK},
    {"C:\\.\\", "C:\\", 0, S_OK},
    {"C:\\a.", "C:\\a", 0, S_OK},
    {"C:\\a.\\", "C:\\a.\\", 0, S_OK},
    {"C:\\.a", "C:\\.a", 0, S_OK},
    {"C:\\.a\\", "C:\\.a\\", 0, S_OK},
    {"C:\\a\\.", "C:\\a", 0, S_OK},
    {"C:\\a\\\\.", "C:\\a\\", 0, S_OK},
    {"C:\\a\\\\\\.", "C:\\a\\\\", 0, S_OK},
    {".\\", "\\", 0, S_OK},
    {"\\.", "\\", 0, S_OK},
    {"\\\\.", "\\\\", 0, S_OK},
    {"\\\\.\\", "\\\\", 0, S_OK},
    {"\\\\\\.", "\\\\", 0, S_OK},
    {"\\\\.\\\\", "\\\\\\", 0, S_OK},
    {"\\\\\\\\.", "\\\\\\", 0, S_OK},
    {"\\?\\.", "\\?", 0, S_OK},
    {"\\\\?\\.", "\\\\?", 0, S_OK},
    {"\\192.168.1.1\\a", "\\192.168.1.1\\a", 0, S_OK},
    {"\\a.168.1.1\\a", "\\a.168.1.1\\a", 0, S_OK},
    {"\\\\192.168.1.1\\a", "\\\\192.168.1.1\\a", 0, S_OK},
    {"\\\\a.168.1.1\\b", "\\\\a.168.1.1\\b", 0, S_OK},
    {"\\\\?\\C:.", "C:\\", 0, S_OK},
    {"\\\\?\\C:\\.", "C:\\", 0, S_OK},
    {"\\\\?\\UNC\\.", "\\\\", 0, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\.",
      "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", 0, S_OK},

    /* .. */
    {"..a", "..a", 0, S_OK},
    {"..a\\", "..a\\", 0, S_OK},
    {"...a", "...a", 0, S_OK},
    {"...a\\", "...a\\", 0, S_OK},
    {"....a", "....a", 0, S_OK},
    {"a..", "a", 0, S_OK},
    {"a..\\", "a..\\", 0, S_OK},
    {"a...", "a", 0, S_OK},
    {"a...\\", "a...\\", 0, S_OK},
    {"a....", "a", 0, S_OK},
    {"a....\\", "a....\\", 0, S_OK},
    {"..a..", "..a", 0, S_OK},
    {"a..b", "a..b", 0, S_OK},
    {"..a..b..", "..a..b", 0, S_OK},
    {"a\\..", "\\", 0, S_OK},
    {"a\\..\\", "\\", 0, S_OK},
    {"a\\..\\b", "\\b", 0, S_OK},
    {":..", ":", 0, S_OK},
    {"::..", "::\\", 0, S_OK},
    {":::..", ":::", 0, S_OK},
    {"C:..", "C:\\", 0, S_OK},
    {"C:...", "C:\\", 0, S_OK},
    {"C:..\\", "C:..\\", 0, S_OK},
    {"C:..\\\\", "C:..\\\\", 0, S_OK},
    {"C:...\\", "C:...\\", 0, S_OK},
    {"C:\\..", "C:\\", 0, S_OK},
    {"C:\\..a", "C:\\..a", 0, S_OK},
    {"C:\\..a\\", "C:\\..a\\", 0, S_OK},
    {"C:\\...a", "C:\\...a", 0, S_OK},
    {"C:\\...a\\", "C:\\...a\\", 0, S_OK},
    {"C:\\....a", "C:\\....a", 0, S_OK},
    {"C:\\....a\\", "C:\\....a\\", 0, S_OK},
    {"C:\\a..", "C:\\a", 0, S_OK},
    {"C:\\a..\\", "C:\\a..\\", 0, S_OK},
    {"C:\\\\..", "C:\\", 0, S_OK},
    {"C:\\..\\", "C:\\", 0, S_OK},
    {"C:\\...\\", "C:\\...\\", 0, S_OK},
    {"C:\\a\\..", "C:\\", 0, S_OK},
    {"C:\\a\\b..", "C:\\a\\b", 0, S_OK},
    {"C:\\a\\\\..", "C:\\a", 0, S_OK},
    {"C:\\a\\\\\\..", "C:\\a\\", 0, S_OK},
    {"C:\\a\\..\\b", "C:\\b", 0, S_OK},
    {"C:\\a\\..\\\\b", "C:\\\\b", 0, S_OK},
    {"..\\", "\\", 0, S_OK},
    {"...\\", "...\\", 0, S_OK},
    {"\\..", "\\", 0, S_OK},
    {"\\...", "\\", 0, S_OK},
    {"\\\\..", "\\\\", 0, S_OK},
    {"\\\\\\..", "\\", 0, S_OK},
    {"\\\\..\\", "\\\\", 0, S_OK},
    {"\\\\\\..", "\\", 0, S_OK},
    {"\\\\..\\\\", "\\\\\\", 0, S_OK},
    {"\\\\\\\\..", "\\\\", 0, S_OK},
    {"\\?\\..", "\\", 0, S_OK},
    {"\\a\\..", "\\", 0, S_OK},
    {"\\\\?\\..", "\\", 0, S_OK},
    {"\\\\a\\..", "\\", 0, S_OK},
    {"\\a\\..\\b", "\\b", 0, S_OK},
    {"\\a\\b\\..", "\\a", 0, S_OK},
    {"\\?\\UNC\\..", "\\?", 0, S_OK},
    {"\\?\\C:\\..", "\\?", 0, S_OK},
    {"\\\\?\\C:..", "C:\\", 0, S_OK},
    {"\\\\?\\C:\\..", "C:\\", 0, S_OK},
    {"\\\\?\\UNC\\..", "\\\\", 0, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}..",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", 0, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\..",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", 0, S_OK},
    {"\\\\?\\UNC\\a\\b\\..", "\\\\a", 0, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\b\\..",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", 0, S_OK},

    /* . and .. */
    {"C:\\a\\.\\b\\..\\", "C:\\a\\", 0, S_OK},
    {"\\a\\.\\b\\..\\", "\\a\\", 0, S_OK},
    {"\\?\\a\\.\\b\\..\\", "\\?\\a\\", 0, S_OK},
    {"\\\\.\\a\\.\\b\\..\\", "\\\\a\\", 0, S_OK},
    {"\\\\?\\a\\.\\b\\..\\", "\\\\?\\a\\", 0, S_OK},
    {"\\\\.\\..", "\\\\", 0, S_OK},

    /* PATHCCH_ALLOW_LONG_PATHS */
    /* Input path with prefix \\?\ and length of MAXPATH + 1, HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) = 0x800700ce */
    {"\\\\?\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", NULL, 0, 0x800700ce},
    /* Input path with prefix C:\ and length of MAXPATH + 1 */
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", NULL, 0, 0x800700ce},
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", PATHCCH_ALLOW_LONG_PATHS, S_OK},
    /* Input path with prefix C: and length of MAXPATH + 1  */
    {"C:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\\\\?\\C:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", PATHCCH_ALLOW_LONG_PATHS, S_OK},
    /* Input path with prefix C:\ and length of MAXPATH + 1 and with .. */
    {"C:\\..\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", PATHCCH_ALLOW_LONG_PATHS, S_OK},
    /* Input path with prefix \\?\ and length of MAXPATH + 1 */
    {"\\\\?\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\\\\?\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", PATHCCH_ALLOW_LONG_PATHS, S_OK},
    /* Input path with prefix \ and length of MAXPATH + 1 */
    {"\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", PATHCCH_ALLOW_LONG_PATHS, S_OK},
    /* Input path with length of MAXPATH with PATHCCH_ALLOW_LONG_PATHS disabled*/
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0, S_OK},
    /* Input path with length of MAXPATH */
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", PATHCCH_ALLOW_LONG_PATHS, S_OK},

    /* Flags added after Windows 10 1709 */
    /* PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS */
    /* PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS without PATHCCH_ALLOW_LONG_PATHS */
    {"", NULL, PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS, E_INVALIDARG},
    /* Input path with prefix C:\ and length of MAXPATH + 1 and PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS */
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     PATHCCH_ALLOW_LONG_PATHS | PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS, S_OK},

    /* PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS */
    /* PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS without PATHCCH_ALLOW_LONG_PATHS */
    {"", NULL, PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS, E_INVALIDARG},
    /* Both PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS and PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS */
    {"", "\\", PATHCCH_ALLOW_LONG_PATHS | PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS,
     E_INVALIDARG},
    /* Input path with prefix C:\ and length of MAXPATH + 1 and PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS */
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     PATHCCH_ALLOW_LONG_PATHS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS, S_OK},
    /* Input path with prefix C:\ and length of MAXPATH and PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS */
    {"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    PATHCCH_ALLOW_LONG_PATHS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS, S_OK},

    /* PATHCCH_DO_NOT_NORMALIZE_SEGMENTS */
    /* No effect for spaces */
    {"a ", "a ", 0, S_OK},
    {"C:\\a ", "C:\\a ", 0, S_OK},
    {"C:\\a \\", "C:\\a \\", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a\\ ", "C:\\a\\ ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a ", "C:\\a ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a  ", "C:\\a  ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a. ", "C:\\a. ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"\\a \\", "\\a \\", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"\\a\\ ", "\\a\\ ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"\\\\a \\", "\\\\a \\", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"\\\\a\\ ", "\\\\a\\ ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"\\\\?\\ ", "\\\\?\\ ", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    /* Keep trailing dot */
    {"*..", "*..", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {".", "\\", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"..", "\\", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {":.", ":.", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"::.", "::.", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {":::.", ":::.", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {":..", ":..", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"::..", "::..", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {":::..", ":::..", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:.", "C:.", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:..", "C:..", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:...", "C:...", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a\\.", "C:\\a", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a\\..", "C:\\", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a.", "C:\\a.", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},
    {"C:\\a..", "C:\\a..", PATHCCH_DO_NOT_NORMALIZE_SEGMENTS, S_OK},

    /* PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH */
    {"C:\\a\\", "\\\\?\\C:\\a\\", PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH, S_OK},
    {"", NULL, PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH | PATHCCH_ALLOW_LONG_PATHS, E_INVALIDARG},
    {"\\a\\", "\\a\\", PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH, S_OK},
    {"\\\\?\\C:\\a\\", "\\\\?\\C:\\a\\", PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH, S_OK},
    /* Implication of PATHCCH_DO_NOT_NORMALIZE_SEGMENTS by PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH */
    {"\\a.", "\\a.", PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH, S_OK},

    /* PATHCCH_ENSURE_TRAILING_SLASH */
    {"\\", "\\", PATHCCH_ENSURE_TRAILING_SLASH, S_OK},
    {"C:\\", "C:\\", PATHCCH_ENSURE_TRAILING_SLASH, S_OK},
    {"C:\\a\\.", "C:\\a\\", PATHCCH_ENSURE_TRAILING_SLASH, S_OK},
    {"C:\\a", "C:\\a\\", PATHCCH_ENSURE_TRAILING_SLASH, S_OK}
};

static void test_PathAllocCanonicalize(void)
{
    WCHAR path_inW[1024], path_maxW[PATHCCH_MAX_CCH + 1];
    WCHAR *path_outW;
    CHAR path_outA[1024];
    BOOL skip_new_flags = TRUE;
    HRESULT hr;
    INT i;

    if (!pPathAllocCanonicalize)
    {
        win_skip("PathAllocCanonicalize() is not available.\n");
        return;
    }

    /* No NULL check for path on Windows */
    if (0)
    {
        hr = pPathAllocCanonicalize(NULL, 0, &path_outW);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    }

    MultiByteToWideChar(CP_ACP, 0, "C:\\", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathAllocCanonicalize(path_inW, 0, NULL);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    /* Test longest path */
    for (i = 0; i < ARRAY_SIZE(path_maxW) - 1; i++) path_maxW[i] = 'a';
    path_maxW[PATHCCH_MAX_CCH] = '\0';
    path_outW = (WCHAR *)0xdeadbeef;
    hr = pPathAllocCanonicalize(path_maxW, PATHCCH_ALLOW_LONG_PATHS, &path_outW);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), "expect hr %#lx, got %#lx\n",
       HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), hr);
    ok(path_outW == NULL, "expect path_outW null, got %p\n", path_outW);

    path_maxW[PATHCCH_MAX_CCH - 1] = '\0';
    hr = pPathAllocCanonicalize(path_maxW, PATHCCH_ALLOW_LONG_PATHS, &path_outW);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);
    LocalFree(path_outW);

    /* Check if flags added after Windows 10 1709 are supported */
    MultiByteToWideChar(CP_ACP, 0, "C:\\", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathAllocCanonicalize(path_inW, PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS, &path_outW);
    if (hr == E_INVALIDARG) skip_new_flags = FALSE;

    for (i = 0; i < ARRAY_SIZE(alloccanonicalize_tests); i++)
    {
        const struct alloccanonicalize_test *t = alloccanonicalize_tests + i;

        if (((PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS
              | PATHCCH_DO_NOT_NORMALIZE_SEGMENTS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH
              | PATHCCH_ENSURE_TRAILING_SLASH)
             & t->flags)
            && skip_new_flags)
        {
            win_skip("Skip testing new flags added after Windows 10 1709\n");
            return;
        }

        MultiByteToWideChar(CP_ACP, 0, t->path_in, -1, path_inW, ARRAY_SIZE(path_inW));
        hr = pPathAllocCanonicalize(path_inW, t->flags, &path_outW);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path_in, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
            ok(!lstrcmpA(path_outA, t->path_out), "path \"%s\" expect output path \"%s\", got \"%s\"\n", t->path_in,
               t->path_out, path_outA);
            LocalFree(path_outW);
        }
    }
}

struct combine_test
{
    const CHAR *path1;
    const CHAR *path2;
    const CHAR *result;
};

static const struct combine_test combine_tests[] =
{
    /* normal paths */
    {"C:\\",  "a",     "C:\\a" },
    {"C:\\b", "..\\a", "C:\\a" },
    {"C:",    "a",     "C:\\a" },
    {"C:\\",  ".",     "C:\\" },
    {"C:\\",  "..",    "C:\\" },
    {"C:\\a", "",      "C:\\a" },
    {"\\",    "a",     "\\a"},
    {"\\a",   "b",     "\\a\\b" },

    /* normal UNC paths */
    {"\\\\192.168.1.1\\test", "a",  "\\\\192.168.1.1\\test\\a" },
    {"\\\\192.168.1.1\\test", "..", "\\\\192.168.1.1" },
    {"\\\\", "a", "\\\\a"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", "a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a"},

    /* NT paths */
    {"\\\\?\\C:\\", "a",  "C:\\a" },
    {"\\\\?\\C:\\", "..", "C:\\" },
    {"\\\\?\\C:", "a", "C:\\a"},

    /* NT UNC path */
    {"\\\\?\\UNC\\", "a", "\\\\a"},
    {"\\\\?\\UNC\\192.168.1.1\\test", "a",  "\\\\192.168.1.1\\test\\a" },
    {"\\\\?\\UNC\\192.168.1.1\\test", "..", "\\\\192.168.1.1" },

    /* Second path begins with a single backslash */
    {"C:a\\b", "\\1", "C:\\1"},
    {"C:\\a\\b", "\\1", "C:\\1"},
    {"\\a\\b", "\\1", "\\1"},
    {"\\\\a\\b", "\\1", "\\\\a\\b\\1"},
    {"\\\\a\\b\\c", "\\1", "\\\\a\\b\\1"},
    {"\\\\?\\UNC\\a", "\\1", "\\\\a\\1"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a", "\\1",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", "\\1",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1"},
    {"C:\\a\\b", "\\", "C:\\"},

    /* Second path is fully qualified */
    {"X:\\", "C:", "C:\\"},
    {"X:\\", "C:\\", "C:\\"},
    {"X:\\", "C:\\a", "C:\\a"},
    {"X:\\", "\\\\", "\\\\"},
    {"X:\\", "\\\\a", "\\\\a"},
    {"X:\\", "\\\\?\\C:", "C:\\"},
    {"X:\\", "\\\\?\\C:\\", "C:\\"},
    {"X:\\", "\\\\?\\C:\\a", "C:\\a"},
    {"X:\\", "\\\\?\\UNC", "\\\\?\\UNC"},
    {"X:\\", "\\\\?\\UNC\\", "\\\\"},
    {"X:\\", "\\\\?\\UNC\\a", "\\\\a"},
    {"X:\\", "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}"},
    {"X:\\", "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\"},
    {"X:\\", "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a"},

    /* Canonicalization */
    {"C:\\a", ".\\b", "C:\\a\\b"},
    {"C:\\a", "..\\b", "C:\\b"},

    /* Other */
    {"", "", "\\"},
    {"a", "b", "a\\b"}
};

static void test_PathAllocCombine(void)
{
    WCHAR path1W[PATHCCH_MAX_CCH];
    WCHAR path2W[PATHCCH_MAX_CCH];
    WCHAR *resultW;
    CHAR resultA[PATHCCH_MAX_CCH];
    HRESULT hr;
    INT i;

    if (!pPathAllocCombine)
    {
        win_skip("PathAllocCombine() is not available.\n");
        return;
    }

    resultW = (WCHAR *)0xdeadbeef;
    hr = pPathAllocCombine(NULL, NULL, 0, &resultW);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(resultW == NULL, "expect resultW null, got %p\n", resultW);

    MultiByteToWideChar(CP_ACP, 0, "\\a", -1, path1W, ARRAY_SIZE(path1W));
    hr = pPathAllocCombine(path1W, NULL, 0, &resultW);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);
    WideCharToMultiByte(CP_ACP, 0, resultW, -1, resultA, ARRAY_SIZE(resultA), NULL, NULL);
    ok(!lstrcmpA(resultA, "\\a"), "expect \\a, got %s\n", resultA);
    LocalFree(resultW);

    MultiByteToWideChar(CP_ACP, 0, "\\b", -1, path2W, ARRAY_SIZE(path2W));
    hr = pPathAllocCombine(NULL, path2W, 0, &resultW);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);
    WideCharToMultiByte(CP_ACP, 0, resultW, -1, resultA, ARRAY_SIZE(resultA), NULL, NULL);
    ok(!lstrcmpA(resultA, "\\b"), "expect \\b, got %s\n", resultA);
    LocalFree(resultW);

    hr = pPathAllocCombine(path1W, path2W, 0, NULL);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(combine_tests); i++)
    {
        const struct combine_test *t = combine_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path1, -1, path1W, ARRAY_SIZE(path1W));
        MultiByteToWideChar(CP_ACP, 0, t->path2, -1, path2W, ARRAY_SIZE(path2W));
        hr = pPathAllocCombine(path1W, path2W, 0, &resultW);
        ok(hr == S_OK, "combine %s %s expect hr %#lx, got %#lx\n", t->path1, t->path2, S_OK, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, resultW, -1, resultA, ARRAY_SIZE(resultA), NULL, NULL);
            ok(!lstrcmpA(resultA, t->result), "combine %s %s expect result %s, got %s\n", t->path1, t->path2, t->result,
               resultA);
            LocalFree(resultW);
        }
    }
}

static void test_PathCchCombine(void)
{
    WCHAR expected[PATHCCH_MAX_CCH] = L"C:\\a";
    WCHAR p1[PATHCCH_MAX_CCH] = L"C:\\";
    WCHAR p2[PATHCCH_MAX_CCH] = L"a";
    WCHAR output[PATHCCH_MAX_CCH];
    HRESULT hr;
    INT i;

    if (!pPathCchCombine)
    {
        win_skip("PathCchCombine() is not available.\n");
        return;
    }

    hr = pPathCchCombine(output, 5, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    hr = pPathCchCombine(NULL, 2, p1, p2);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombine(output, 0, p1, p2);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(output[0] == 0xffff, "Expected output buffer to be unchanged\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombine(output, 1, p1, p2);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08lx\n", hr);
    ok(output[0] == 0, "Expected output buffer to contain NULL string\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombine(output, 4, p1, p2);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08lx\n", hr);
    ok(output[0] == 0x0, "Expected output buffer to contain NULL string\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombine(output, 5, p1, p2);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!lstrcmpW(output, expected), "Combination of %s + %s returned %s, expected %s\n", wine_dbgstr_w(p1),
       wine_dbgstr_w(p2), wine_dbgstr_w(output), wine_dbgstr_w(expected));

    hr = pPathCchCombine(output, PATHCCH_MAX_CCH + 1, p1, p2);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    hr = pPathCchCombine(output, PATHCCH_MAX_CCH, p1, p2);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(combine_tests); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, combine_tests[i].path1, -1, p1, ARRAY_SIZE(p1));
        MultiByteToWideChar(CP_ACP, 0, combine_tests[i].path2, -1, p2, ARRAY_SIZE(p2));
        MultiByteToWideChar(CP_ACP, 0, combine_tests[i].result, -1, expected, ARRAY_SIZE(expected));

        hr = pPathCchCombine(output, ARRAY_SIZE(output), p1, p2);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(!lstrcmpW(output, expected), "Combining %s with %s returned %s, expected %s\n", wine_dbgstr_w(p1),
           wine_dbgstr_w(p2), wine_dbgstr_w(output), wine_dbgstr_w(expected));
    }
}

static void test_PathCchCombineEx(void)
{
    WCHAR expected[MAX_PATH] = L"C:\\a";
    WCHAR p1[MAX_PATH] = L"C:\\";
    WCHAR p2[MAX_PATH] = L"a";
    WCHAR output[MAX_PATH];
    HRESULT hr;
    int i;

    if (!pPathCchCombineEx)
    {
        win_skip("PathCchCombineEx() is not available.\n");
        return;
    }

    output[0] = 0xff;
    hr = pPathCchCombineEx(output, 5, NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(output[0] == 0, "Expected output buffer to be empty\n");

    output[0] = 0xff;
    hr = pPathCchCombineEx(NULL, 2, p1, p2, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(output[0] == 0xff, "Expected output buffer to be unchanged\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 0, p1, p2, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(output[0] == 0xffff, "Expected output buffer to be unchanged\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 1, p1, p2, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08lx\n", hr);
    ok(output[0] == 0, "Expected output buffer to contain NULL string\n");

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 4, p1, p2, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08lx\n", hr);
    ok(output[0] == 0x0, "Expected output buffer to contain NULL string\n");

    output[0] = 0xff;
    hr = pPathCchCombineEx(output, PATHCCH_MAX_CCH + 1, p1, p2, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(output[0] == 0xff, "Expected output buffer to be 0xff\n");

    hr = pPathCchCombineEx(output, PATHCCH_MAX_CCH, p1, p2, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    memset(output, 0xff, sizeof(output));
    hr = pPathCchCombineEx(output, 5, p1, p2, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!lstrcmpW(output, expected),
        "Combination of %s + %s returned %s, expected %s\n",
        wine_dbgstr_w(p1), wine_dbgstr_w(p2), wine_dbgstr_w(output), wine_dbgstr_w(expected));

    for (i = 0; i < ARRAY_SIZE(combine_tests); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, combine_tests[i].path1, -1, p1, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, combine_tests[i].path2, -1, p2, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, combine_tests[i].result, -1, expected, MAX_PATH);

        hr = pPathCchCombineEx(output, MAX_PATH, p1, p2, 0);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
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
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    pathW[0] = 0;
    hr = pPathCchAddBackslash(pathW, 1);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    pathW[0] = 0;
    hr = pPathCchAddBackslash(pathW, 2);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    for (i = 0; i < ARRAY_SIZE(addbackslash_tests); i++)
    {
        const struct addbackslash_test *test = &addbackslash_tests[i];
        char path[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, test->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchAddBackslash(pathW, test->size);
        ok(hr == test->hr, "%u: unexpected return value %#lx.\n", i, hr);

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
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    pathW[0] = 0;
    ptrW = (void *)0xdeadbeef;
    remaining = 123;
    hr = pPathCchAddBackslashEx(pathW, 1, &ptrW, &remaining);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");
    ok(ptrW == pathW, "Unexpected endptr %p.\n", ptrW);
    ok(remaining == 1, "Unexpected remaining size.\n");

    pathW[0] = 0;
    hr = pPathCchAddBackslashEx(pathW, 2, NULL, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(pathW[0] == 0, "Unexpected path.\n");

    for (i = 0; i < ARRAY_SIZE(addbackslash_tests); i++)
    {
        const struct addbackslash_test *test = &addbackslash_tests[i];
        char path[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, test->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchAddBackslashEx(pathW, test->size, NULL, NULL);
        ok(hr == test->hr, "%u: unexpected return value %#lx.\n", i, hr);

        WideCharToMultiByte(CP_ACP, 0, pathW, -1, path, ARRAY_SIZE(path), NULL, NULL);
        ok(!strcmp(path, test->result), "%u: unexpected resulting path %s.\n", i, path);

        ptrW = (void *)0xdeadbeef;
        remaining = 123;
        MultiByteToWideChar(CP_ACP, 0, test->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchAddBackslashEx(pathW, test->size, &ptrW, &remaining);
        ok(hr == test->hr, "%u: unexpected return value %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(ptrW == (pathW + lstrlenW(pathW)), "%u: unexpected end pointer.\n", i);
            ok(remaining == test->remaining, "%u: unexpected remaining buffer length.\n", i);
        }
        else
        {
            ok(ptrW == NULL, "%u: unexpected end pointer.\n", i);
            ok(remaining == 0, "%u: unexpected remaining buffer length.\n", i);
        }
    }
}

struct addextension_test
{
    const CHAR *path;
    const CHAR *extension;
    const CHAR *expected;
    HRESULT hr;
};

static const struct addextension_test addextension_tests[] =
{
    /* Normal */
    {"", ".exe", ".exe", S_OK},
    {"C:\\", "", "C:\\", S_OK},
    {"C:", ".exe", "C:.exe", S_OK},
    {"C:\\", ".exe", "C:\\.exe", S_OK},
    {"\\", ".exe", "\\.exe", S_OK},
    {"\\\\", ".exe", "\\\\.exe", S_OK},
    {"\\\\?\\C:", ".exe", "\\\\?\\C:.exe", S_OK},
    {"\\\\?\\C:\\", ".exe", "\\\\?\\C:\\.exe", S_OK},
    {"\\\\?\\UNC\\", ".exe", "\\\\?\\UNC\\.exe", S_OK},
    {"\\\\?\\UNC\\192.168.1.1\\", ".exe", "\\\\?\\UNC\\192.168.1.1\\.exe", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", ".exe",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\.exe", S_OK},
    {"C:\\", "exe", "C:\\.exe", S_OK},
    {"C:\\", ".", "C:\\", S_OK},
    {"C:\\1.exe", ".txt", "C:\\1.exe", S_FALSE},

    /* Extension contains invalid characters but valid for PathCchAddExtension */
    {"C:\\", "./", "C:\\./", S_OK},
    {"C:\\", ".?", "C:\\.?", S_OK},
    {"C:\\", ".%", "C:\\.%", S_OK},
    {"C:\\", ".*", "C:\\.*", S_OK},
    {"C:\\", ".:", "C:\\.:", S_OK},
    {"C:\\", ".|", "C:\\.|", S_OK},
    {"C:\\", ".\"", "C:\\.\"", S_OK},
    {"C:\\", ".<", "C:\\.<", S_OK},
    {"C:\\", ".>", "C:\\.>", S_OK},

    /* Invalid argument for extension */
    {"C:\\", " exe", NULL, E_INVALIDARG},
    {"C:\\", ". exe", NULL, E_INVALIDARG},
    {"C:\\", " ", NULL, E_INVALIDARG},
    {"C:\\", "\\", NULL, E_INVALIDARG},
    {"C:\\", "..", NULL, E_INVALIDARG},
    {"C:\\", ". ", NULL, E_INVALIDARG},
    {"C:\\", ".\\", NULL, E_INVALIDARG},
    {"C:\\", ".a.", NULL, E_INVALIDARG},
    {"C:\\", ".a ", NULL, E_INVALIDARG},
    {"C:\\", ".a\\", NULL, E_INVALIDARG},
    {"C:\\1.exe", " ", NULL, E_INVALIDARG}
};

struct append_test
{
    const CHAR *path1;
    const CHAR *path2;
    const CHAR *result;
};

static const struct append_test append_tests[] =
{
    /* normal paths */
    {"C:\\", "a", "C:\\a"},
    {"C:\\b", "..\\a", "C:\\a"},
    {"C:", "a", "C:\\a"},
    {"C:\\", ".", "C:\\"},
    {"C:\\", "..", "C:\\"},
    {"C:\\a", "", "C:\\a"},

    /* normal UNC paths */
    {"\\\\192.168.1.1\\test", "a", "\\\\192.168.1.1\\test\\a"},
    {"\\\\192.168.1.1\\test", "..", "\\\\192.168.1.1"},
    {"\\a", "b", "\\a\\b"},
    {"\\", "a", "\\a"},
    {"\\\\", "a", "\\\\a"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", "a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a"},

    /* NT paths */
    {"\\\\?\\C:\\", "a", "C:\\a"},
    {"\\\\?\\C:\\", "..", "C:\\"},
    {"\\\\?\\C:", "a", "C:\\a"},

    /* NT UNC path */
    {"\\\\?\\UNC\\", "a", "\\\\a"},
    {"\\\\?\\UNC\\192.168.1.1\\test", "a", "\\\\192.168.1.1\\test\\a"},
    {"\\\\?\\UNC\\192.168.1.1\\test", "..", "\\\\192.168.1.1"},

    /* Second path begins with a single backslash */
    {"C:a\\b", "\\1", "C:a\\b\\1"},
    {"C:\\a\\b", "\\1", "C:\\a\\b\\1"},
    {"\\a\\b", "\\1", "\\a\\b\\1"},
    {"\\\\a\\b", "\\1", "\\\\a\\b\\1"},
    {"\\\\a\\b\\c", "\\1", "\\\\a\\b\\c\\1"},
    {"\\\\?\\UNC\\a", "\\1", "\\\\a\\1"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a", "\\1",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a\\1"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", "\\1",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\1"},
    {"C:\\a\\b", "\\", "C:\\a\\b"},

    /* Second path is fully qualified */
    {"X:\\", "C:", "C:\\"},
    {"X:\\", "C:\\", "C:\\"},
    {"X:\\", "\\\\", "\\\\"},
    {"X:\\", "\\\\?\\C:", "C:\\"},
    {"X:\\", "\\\\?\\C:\\", "C:\\"},
    {"X:\\", "\\\\?\\UNC\\", "\\\\"},
    {"X:\\", "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\"},

    /* Canonicalization */
    {"C:\\a", ".\\b", "C:\\a\\b"},
    {"C:\\a", "..\\b", "C:\\b"},

    /* Other */
    {"", "", "\\"},
    {"a", "b", "a\\b"}
};

static void test_PathCchAppend(void)
{
    WCHAR path1W[PATHCCH_MAX_CCH];
    WCHAR path2W[PATHCCH_MAX_CCH];
    CHAR path1A[PATHCCH_MAX_CCH];
    HRESULT hr;
    INT i;

    if (!pPathCchAppend)
    {
        win_skip("PathCchAppend() is not available.\n");
        return;
    }

    MultiByteToWideChar(CP_ACP, 0, "\\a", -1, path1W, ARRAY_SIZE(path1W));
    MultiByteToWideChar(CP_ACP, 0, "\\b", -1, path2W, ARRAY_SIZE(path2W));
    hr = pPathCchAppend(NULL, PATHCCH_MAX_CCH, path2W);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAppend(path1W, 0, path2W);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAppend(path1W, PATHCCH_MAX_CCH + 1, path2W);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAppend(path1W, PATHCCH_MAX_CCH, NULL);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);
    WideCharToMultiByte(CP_ACP, 0, path1W, -1, path1A, ARRAY_SIZE(path1A), NULL, NULL);
    ok(!lstrcmpA(path1A, "\\a"), "expect \\a, got %s\n", path1A);

    for (i = 0; i < ARRAY_SIZE(append_tests); i++)
    {
        const struct append_test *t = append_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path1, -1, path1W, ARRAY_SIZE(path1W));
        MultiByteToWideChar(CP_ACP, 0, t->path2, -1, path2W, ARRAY_SIZE(path2W));
        hr = pPathCchAppend(path1W, PATHCCH_MAX_CCH, path2W);
        ok(hr == S_OK, "append \"%s\" \"%s\" expect hr %#lx, got %#lx\n", t->path1, t->path2, S_OK, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, path1W, -1, path1A, ARRAY_SIZE(path1A), NULL, NULL);
            ok(!lstrcmpA(path1A, t->result), "append \"%s\" \"%s\" expect result \"%s\", got \"%s\"\n", t->path1,
               t->path2, t->result, path1A);
        }
    }
}

static void test_PathCchAppendEx(void)
{
    WCHAR path1W[PATHCCH_MAX_CCH];
    WCHAR path2W[PATHCCH_MAX_CCH];
    CHAR path1A[PATHCCH_MAX_CCH];
    HRESULT hr;
    INT i;

    if (!pPathCchAppendEx)
    {
        win_skip("PathCchAppendEx() is not available.\n");
        return;
    }

    MultiByteToWideChar(CP_ACP, 0, "\\a", -1, path1W, ARRAY_SIZE(path1W));
    MultiByteToWideChar(CP_ACP, 0, "\\b", -1, path2W, ARRAY_SIZE(path2W));
    hr = pPathCchAppendEx(NULL, ARRAY_SIZE(path1W), path2W, 0);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAppendEx(path1W, 0, path2W, 0);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(path1W[0] == '\\', "expect path1 unchanged\n");

    hr = pPathCchAppendEx(path1W, PATHCCH_MAX_CCH + 1, path2W, 0);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(path1W[0] == '\\', "expect path1 unchanged\n");

    hr = pPathCchAppendEx(path1W,  ARRAY_SIZE(path1W), NULL, 0);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);
    WideCharToMultiByte(CP_ACP, 0, path1W, -1, path1A, ARRAY_SIZE(path1A), NULL, NULL);
    ok(!lstrcmpA(path1A, "\\a"), "expect \\a, got %s\n", path1A);

    hr = pPathCchAppendEx(path1W, PATHCCH_MAX_CCH, path2W, 0);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);

    for (i = 0; i < ARRAY_SIZE(append_tests); i++)
    {
        const struct append_test *t = append_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path1, -1, path1W, ARRAY_SIZE(path1W));
        MultiByteToWideChar(CP_ACP, 0, t->path2, -1, path2W, ARRAY_SIZE(path2W));
        hr = pPathCchAppendEx(path1W, PATHCCH_MAX_CCH, path2W, 0);
        ok(hr == S_OK, "append \"%s\" \"%s\" expect hr %#lx, got %#lx\n", t->path1, t->path2, S_OK, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, path1W, -1, path1A, ARRAY_SIZE(path1A), NULL, NULL);
            ok(!lstrcmpA(path1A, t->result), "append \"%s\" \"%s\" expect result \"%s\", got \"%s\"\n", t->path1,
               t->path2, t->result, path1A);
        }
    }
}

static void test_PathCchAddExtension(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH + 1];
    CHAR pathA[PATHCCH_MAX_CCH + 1];
    WCHAR extensionW[MAX_PATH];
    HRESULT hr;
    INT i;

    if (!pPathCchAddExtension)
    {
        win_skip("PathCchAddExtension() is not available.\n");
        return;
    }

    /* Arguments check */
    MultiByteToWideChar(CP_ACP, 0, "C:\\", -1, pathW, ARRAY_SIZE(pathW));
    MultiByteToWideChar(CP_ACP, 0, ".exe", -1, extensionW, ARRAY_SIZE(extensionW));

    hr = pPathCchAddExtension(NULL, PATHCCH_MAX_CCH, extensionW);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAddExtension(pathW, 0, extensionW);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAddExtension(pathW, PATHCCH_MAX_CCH, NULL);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    /* Path length check */
    hr = pPathCchAddExtension(pathW, ARRAY_SIZE("C:\\.exe") - 1, extensionW);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "expect result %#lx, got %#lx\n", STRSAFE_E_INSUFFICIENT_BUFFER, hr);

    hr = pPathCchAddExtension(pathW, PATHCCH_MAX_CCH + 1, extensionW);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchAddExtension(pathW, PATHCCH_MAX_CCH, extensionW);
    ok(hr == S_OK, "expect result %#lx, got %#lx\n", S_OK, hr);

    for (i = 0; i < ARRAY_SIZE(addextension_tests); i++)
    {
        const struct addextension_test *t = addextension_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        MultiByteToWideChar(CP_ACP, 0, t->extension, -1, extensionW, ARRAY_SIZE(extensionW));
        hr = pPathCchAddExtension(pathW, PATHCCH_MAX_CCH, extensionW);
        ok(hr == t->hr, "path %s extension %s expect result %#lx, got %#lx\n", t->path, t->extension, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, ARRAY_SIZE(pathA), NULL, NULL);
            ok(!lstrcmpA(pathA, t->expected), "path %s extension %s expect output path %s, got %s\n", t->path,
               t->extension, t->expected, pathA);
        }
    }
}

static void test_PathCchCanonicalize(void)
{
    WCHAR path_inW[MAX_PATH + 16], path_outW[MAX_PATH];
    CHAR path_outA[MAX_PATH];
    HRESULT hr;
    INT i;

    if (!pPathCchCanonicalize)
    {
        win_skip("PathCchCanonicalize() is not available.\n");
        return;
    }

    /* No NULL check for path pointers on Windows */
    if (0)
    {
        hr = pPathCchCanonicalize(NULL, ARRAY_SIZE(path_outW), path_inW);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

        /* MSDN says NULL path_in result in a backslash added to path_out, but the fact is that it would crash */
        hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), NULL);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    }

    path_inW[0] = 0;
    hr = pPathCchCanonicalize(path_outW, 0, path_inW);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    /* Test path length */
    for (i = 0; i < MAX_PATH - 3; i++) path_inW[i] = 'a';
    path_inW[MAX_PATH - 3] = '\0';
    memset(path_outW, 0, sizeof(path_outW));
    hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), path_inW);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), "expect hr %#lx, got %#lx %s\n",
       HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), hr, wine_dbgstr_w(path_outW));
    ok(!*path_outW, "got %d\n", lstrlenW(path_outW));

    path_inW[0] = 'C';
    path_inW[1] = ':';
    path_inW[2] = '\\';
    hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), path_inW);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);

    path_inW[MAX_PATH - 4] = '\0';
    hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), path_inW);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);

    /* Insufficient buffer size handling */
    hr = pPathCchCanonicalize(path_outW, 1, path_inW);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "expect hr %#lx, got %#lx\n", STRSAFE_E_INSUFFICIENT_BUFFER, hr);

    for (i = 0; i < ARRAY_SIZE(alloccanonicalize_tests); i++)
    {
        const struct alloccanonicalize_test *t = alloccanonicalize_tests + i;

        /* Skip testing X: path input, this case is different compared to PathAllocCanonicalize */
        /* Skip test cases where a flag is used */
        if (!lstrcmpA("C:", t->path_in) || t->flags) continue;

        hr = MultiByteToWideChar(CP_ACP, 0, t->path_in, -1, path_inW, ARRAY_SIZE(path_inW));
        ok(hr, "MultiByteToWideChar failed %#lx\n", GetLastError());
        hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), path_inW);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path_in, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
            ok(!lstrcmpA(path_outA, t->path_out), "path \"%s\" expect output path \"%s\", got \"%s\"\n", t->path_in,
               t->path_out, path_outA);
        }
    }

    /* X: path input */
    /* Fill a \ at the end of X: if there is enough space */
    MultiByteToWideChar(CP_ACP, 0, "C:", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), path_inW);
    ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", "C:", S_OK, hr);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
        ok(!lstrcmpA(path_outA, "C:\\"), "path \"%s\" expect output path \"%s\", got \"%s\"\n", "C:", "C:\\",
           path_outA);
    }

    /* Don't fill a \ at the end of X: if there isn't enough space */
    MultiByteToWideChar(CP_ACP, 0, "C:", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalize(path_outW, 3, path_inW);
    ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", "C:", S_OK, hr);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
        ok(!lstrcmpA(path_outA, "C:"), "path \"%s\" expect output path \"%s\", got \"%s\"\n", "C:", "C:\\", path_outA);
    }

    /* Don't fill a \ at the end of X: if there is character following X: */
    MultiByteToWideChar(CP_ACP, 0, "C:a", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalize(path_outW, ARRAY_SIZE(path_outW), path_inW);
    ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", "C:a", S_OK, hr);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
        ok(!lstrcmpA(path_outA, "C:a"), "path \"%s\" expect output path \"%s\", got \"%s\"\n", "C:a", "C:a", path_outA);
    }
}

static void test_PathCchCanonicalizeEx(void)
{
    WCHAR path_inW[PATHCCH_MAX_CCH + 1], path_outW[PATHCCH_MAX_CCH];
    CHAR path_outA[4096];
    BOOL skip_new_flags = TRUE;
    HRESULT hr;
    INT i;

    if (!pPathCchCanonicalizeEx)
    {
        win_skip("PathCchCanonicalizeEx() is not available.\n");
        return;
    }

    /* No NULL check for path pointers on Windows */
    if (0)
    {
        hr = pPathCchCanonicalizeEx(NULL, ARRAY_SIZE(path_outW), path_inW, 0);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

        /* MSDN says NULL path_in result in a backslash added to path_out, but the fact is that it would crash */
        hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), NULL, 0);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    }

    for (i = 0; i < ARRAY_SIZE(path_inW) - 1; i++) path_inW[i] = 'a';
    path_inW[PATHCCH_MAX_CCH] = '\0';

    path_outW[0] = 0xff;
    hr = pPathCchCanonicalizeEx(path_outW, 0, path_inW, 0);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(path_outW[0] == 0xff, "expect path_outW unchanged\n");

    /* Test path length */
    hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), path_inW, PATHCCH_ALLOW_LONG_PATHS);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), "expect hr %#lx, got %#lx\n",
       HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), hr);

    path_inW[PATHCCH_MAX_CCH - 1] = '\0';
    hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), path_inW, PATHCCH_ALLOW_LONG_PATHS);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);

    hr = pPathCchCanonicalizeEx(path_outW, 1, path_inW, PATHCCH_ALLOW_LONG_PATHS);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), "expect hr %#lx, got %#lx\n",
       HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), hr);

    /* No root and path > MAX_PATH - 4, return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) */
    path_inW[MAX_PATH - 3] = '\0';
    hr = pPathCchCanonicalizeEx(path_outW, 1, path_inW, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), "expect hr %#lx, got %#lx\n",
       HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), hr);

    /* Has root and path > MAX_PATH - 4 */
    path_inW[0] = 'C';
    path_inW[1] = ':';
    path_inW[2] = '\\';
    hr = pPathCchCanonicalizeEx(path_outW, 1, path_inW, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "expect hr %#lx, got %#lx\n", STRSAFE_E_INSUFFICIENT_BUFFER, hr);

    path_inW[0] = '\\';
    path_inW[1] = path_inW[2] = 'a';
    hr = pPathCchCanonicalizeEx(path_outW, 1, path_inW, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "expect hr %#lx, got %#lx\n", STRSAFE_E_INSUFFICIENT_BUFFER, hr);

    path_inW[0] = path_inW[1] = '\\';
    path_inW[2] = 'a';
    hr = pPathCchCanonicalizeEx(path_outW, 1, path_inW, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "expect hr %#lx, got %#lx\n", STRSAFE_E_INSUFFICIENT_BUFFER, hr);

    /* path <= MAX_PATH - 4 */
    path_inW[0] = path_inW[1] = path_inW[2] = 'a';
    path_inW[MAX_PATH - 4] = '\0';
    hr = pPathCchCanonicalizeEx(path_outW, 1, path_inW, 0);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "expect hr %#lx, got %#lx\n", STRSAFE_E_INSUFFICIENT_BUFFER, hr);

    /* Check if flags added after Windows 10 1709 are supported */
    MultiByteToWideChar(CP_ACP, 0, "C:\\", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), path_inW, PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS);
    if (hr == E_INVALIDARG) skip_new_flags = FALSE;

    for (i = 0; i < ARRAY_SIZE(alloccanonicalize_tests); i++)
    {
        const struct alloccanonicalize_test *t = alloccanonicalize_tests + i;

        /* Skip testing X: path input, this case is different compared to PathAllocCanonicalize */
        if (!lstrcmpA("C:", t->path_in)) continue;

        if (((PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS
              | PATHCCH_DO_NOT_NORMALIZE_SEGMENTS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH
              | PATHCCH_ENSURE_TRAILING_SLASH)
             & t->flags)
            && skip_new_flags)
        {
            win_skip("Skip testing new flags added after Windows 10 1709\n");
            return;
        }

        MultiByteToWideChar(CP_ACP, 0, t->path_in, -1, path_inW, ARRAY_SIZE(path_inW));
        hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), path_inW, t->flags);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path_in, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
            ok(!lstrcmpA(path_outA, t->path_out), "path \"%s\" expect output path \"%s\", got \"%s\"\n", t->path_in,
               t->path_out, path_outA);
        }
    }

    /* X: path input */
    /* Fill a \ at the end of X: if there is enough space */
    MultiByteToWideChar(CP_ACP, 0, "C:", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), path_inW, 0);
    ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", "C:", S_OK, hr);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
        ok(!lstrcmpA(path_outA, "C:\\"), "path \"%s\" expect output path \"%s\", got \"%s\"\n", "C:", "C:\\",
           path_outA);
    }

    /* Don't fill a \ at the end of X: if there isn't enough space */
    MultiByteToWideChar(CP_ACP, 0, "C:", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalizeEx(path_outW, 3, path_inW, 0);
    ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", "C:", S_OK, hr);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
        ok(!lstrcmpA(path_outA, "C:"), "path \"%s\" expect output path \"%s\", got \"%s\"\n", "C:", "C:\\", path_outA);
    }

    /* Don't fill a \ at the end of X: if there is character following X: */
    MultiByteToWideChar(CP_ACP, 0, "C:a", -1, path_inW, ARRAY_SIZE(path_inW));
    hr = pPathCchCanonicalizeEx(path_outW, ARRAY_SIZE(path_outW), path_inW, 0);
    ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", "C:a", S_OK, hr);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, path_outW, -1, path_outA, ARRAY_SIZE(path_outA), NULL, NULL);
        ok(!lstrcmpA(path_outA, "C:a"), "path \"%s\" expect output path \"%s\", got \"%s\"\n", "C:a", "C:a", path_outA);
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
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(extension == NULL, "Expect extension null, got %p\n", extension);

    extension = (const WCHAR *)0xdeadbeef;
    hr = pPathCchFindExtension(pathW, 0, &extension);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(extension == NULL, "Expect extension null, got %p\n", extension);

    /* Crashed on Windows */
    if (0)
    {
        hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, NULL);
        ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);
    }

    /* Path length check */
    /* size == PATHCCH_MAX_CCH + 1 */
    MultiByteToWideChar(CP_ACP, 0, "C:\\1.exe", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH + 1, &extension);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    /* Size == path length + 1*/
    hr = pPathCchFindExtension(pathW, ARRAY_SIZE("C:\\1.exe"), &extension);
    ok(hr == S_OK, "expect result %#lx, got %#lx\n", S_OK, hr);
    ok(*extension == '.', "wrong extension value\n");

    /* Size < path length + 1 */
    extension = (const WCHAR *)0xdeadbeef;
    hr = pPathCchFindExtension(pathW, ARRAY_SIZE("C:\\1.exe") - 1, &extension);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(extension == NULL, "Expect extension null, got %p\n", extension);

    /* Size == PATHCCH_MAX_CCH */
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
    ok(hr == S_OK, "expect result %#lx, got %#lx\n", S_OK, hr);

    /* Path length + 1 > PATHCCH_MAX_CCH */
    for (i = 0; i < ARRAY_SIZE(pathW) - 1; i++) pathW[i] = 'a';
    pathW[PATHCCH_MAX_CCH] = 0;
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    /* Path length + 1 == PATHCCH_MAX_CCH */
    pathW[PATHCCH_MAX_CCH - 1] = 0;
    hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
    ok(hr == S_OK, "expect result %#lx, got %#lx\n", S_OK, hr);

    for (i = 0; i < ARRAY_SIZE(findextension_tests); i++)
    {
        const struct findextension_test *t = findextension_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchFindExtension(pathW, PATHCCH_MAX_CCH, &extension);
        ok(hr == S_OK, "path %s expect result %#lx, got %#lx\n", t->path, S_OK, hr);
        if (SUCCEEDED(hr))
            ok(extension - pathW == t->extension_offset, "path %s expect extension offset %d, got %Id\n", t->path,
               t->extension_offset, (UINT_PTR)(extension - pathW));
    }
}

struct isroot_test
{
    const CHAR *path;
    BOOL ret;
};

static const struct isroot_test isroot_tests[] =
{
    {"", FALSE},
    {"a", FALSE},
    {"C:", FALSE},
    {"C:\\", TRUE},
    {"C:\\a", FALSE},
    {"\\\\?\\C:\\", TRUE},
    {"\\\\?\\C:", FALSE},
    {"\\\\?\\C:\\a", FALSE},
    {"\\", TRUE},
    {"\\a\\", FALSE},
    {"\\a\\b", FALSE},
    {"\\\\", TRUE},
    {"\\\\a", TRUE},
    {"\\\\a\\", FALSE},
    {"\\\\a\\b", TRUE},
    {"\\\\a\\b\\", FALSE},
    {"\\\\a\\b\\c", FALSE},
    {"\\\\?\\UNC\\", TRUE},
    {"\\\\?\\UNC\\a", TRUE},
    {"\\\\?\\UNC\\a\\", FALSE},
    {"\\\\?\\UNC\\a\\b", TRUE},
    {"\\\\?\\UNC\\a\\b\\", FALSE},
    {"\\\\?\\UNC\\a\\b\\c", FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", TRUE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", FALSE},
    {"..\\a", FALSE},
    {"\\\\\\\\", FALSE},
    {"\\\\a\\\\b", FALSE},

    /* Wrong MSDN examples */
    {"\\a", FALSE},
    {"X:", FALSE},
    {"\\server", FALSE}
};

static void test_PathCchIsRoot(void)
{
    WCHAR pathW[MAX_PATH];
    BOOL ret;
    INT i;

    if (!pPathCchIsRoot)
    {
        win_skip("PathCchIsRoot() is not available.\n");
        return;
    }

    ret = pPathCchIsRoot(NULL);
    ok(ret == FALSE, "expect return FALSE\n");

    for (i = 0; i < ARRAY_SIZE(isroot_tests); i++)
    {
        const struct isroot_test *t = isroot_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        ret = pPathCchIsRoot(pathW);
        ok(ret == t->ret, "path %s expect return %d, got %d\n", t->path, t->ret, ret);
    }
}

struct removebackslashex_test
{
    const CHAR *path_in;
    const CHAR *path_out;
    int end_offset;
    SIZE_T free_size;
    HRESULT hr;
};

static const struct removebackslashex_test removebackslashex_tests [] =
{
    {"", "", 0, 1, S_FALSE},
    {"C", "C", 1, 1, S_FALSE},
    {"C\\", "C", 1, 2, S_OK},
    {"C:", "C:", 2, 1, S_FALSE},
    {"C:\\", "C:\\", 2, 2, S_FALSE},
    {"C:\\\\", "C:\\", 3, 2, S_OK},
    {"C:\\a\\", "C:\\a", 4, 2, S_OK},
    {"C:\\a\\\\", "C:\\a\\", 5, 2, S_OK},
    {"\\", "\\", 0, 2, S_FALSE},
    {"\\\\", "\\\\", 1, 2, S_FALSE},
    {"\\?\\", "\\?", 2, 2, S_OK},
    {"\\?\\\\", "\\?\\", 3, 2, S_OK},
    {"\\a\\", "\\a", 2, 2, S_OK},
    {"\\a\\\\", "\\a\\", 3, 2, S_OK},
    {"\\\\a\\", "\\\\a", 3, 2, S_OK},
    {"\\\\a\\b\\", "\\\\a\\b", 5, 2, S_OK},
    {"\\\\a\\\\", "\\\\a\\", 4, 2, S_OK},
    {"\\\\?\\", "\\\\?", 3, 2, S_OK},
    {"\\\\?\\\\", "\\\\?\\", 4, 2, S_OK},
    {"\\\\?\\C:", "\\\\?\\C:", 6, 1, S_FALSE},
    {"\\\\?\\C:\\", "\\\\?\\C:\\", 6, 2, S_FALSE},
    {"\\?\\UNC\\", "\\?\\UNC", 6, 2, S_OK},
    {"\\\\?\\UNC", "\\\\?\\UNC", 7, 1, S_FALSE},
    {"\\\\?\\UNC\\", "\\\\?\\UNC\\", 7, 2, S_FALSE},
    {"\\\\?\\UNC\\a", "\\\\?\\UNC\\a", 9, 1, S_FALSE},
    {"\\\\?\\UNC\\a\\", "\\\\?\\UNC\\a", 9, 2, S_OK},
    {"\\\\?\\UNC\\a\\b\\", "\\\\?\\UNC\\a\\b", 11, 2, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", 48, 1, S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", 48, 2, S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", 50, 1, S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", 50, 2, S_OK}
};

static void test_PathCchRemoveBackslash(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH];
    CHAR pathA[PATHCCH_MAX_CCH];
    HRESULT hr;
    SIZE_T path_size;
    INT i;

    if (!pPathCchRemoveBackslash)
    {
        win_skip("PathCchRemoveBackslash() is not available.\n");
        return;
    }

    /* No NULL check for path on Windows */
    if (0)
    {
        hr = pPathCchRemoveBackslash(NULL, PATHCCH_MAX_CCH);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    }

    MultiByteToWideChar(CP_ACP, 0, "C:\\a\\", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchRemoveBackslash(pathW, 0);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRemoveBackslash(pathW, PATHCCH_MAX_CCH + 1);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);

    hr = pPathCchRemoveBackslash(pathW, PATHCCH_MAX_CCH);
    ok(hr == S_FALSE, "expect hr %#lx, got %#lx\n", S_FALSE, hr);

    for (i = 0; i < ARRAY_SIZE(removebackslashex_tests); i++)
    {
        const struct removebackslashex_test *t = removebackslashex_tests + i;
        path_size = MultiByteToWideChar(CP_ACP, 0, t->path_in, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchRemoveBackslash(pathW, path_size);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path_in, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, ARRAY_SIZE(pathA), NULL, NULL);
            ok(!lstrcmpA(pathA, t->path_out), "path %s expect output path %s, got %s\n", t->path_in, t->path_out,
               pathA);
        }
    }
}

static void test_PathCchRemoveBackslashEx(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH];
    CHAR pathA[PATHCCH_MAX_CCH];
    WCHAR *path_end;
    SIZE_T path_size, free_size;
    HRESULT hr;
    INT i;

    if (!pPathCchRemoveBackslashEx)
    {
        win_skip("PathCchRemoveBackslashEx() is not available.\n");
        return;
    }

    /* No NULL check for path on Windows */
    if (0)
    {
        hr = pPathCchRemoveBackslashEx(NULL, 0, &path_end, &path_size);
        ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    }

    path_size = MultiByteToWideChar(CP_ACP, 0, "C:\\a\\", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchRemoveBackslashEx(pathW, 0, &path_end, &path_size);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);

    free_size = 0xdeadbeef;
    hr = pPathCchRemoveBackslashEx(pathW, path_size, NULL, &free_size);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(free_size == 0, "expect %d, got %Iu\n", 0, free_size);

    path_end = (WCHAR *)0xdeadbeef;
    hr = pPathCchRemoveBackslashEx(pathW, path_size, &path_end, NULL);
    ok(hr == E_INVALIDARG, "expect hr %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(path_end == NULL, "expect null, got %p\n", path_end);

    hr = pPathCchRemoveBackslashEx(pathW, PATHCCH_MAX_CCH + 1, &path_end, &free_size);
    ok(hr == S_OK, "expect hr %#lx, got %#lx\n", S_OK, hr);

    hr = pPathCchRemoveBackslashEx(pathW, PATHCCH_MAX_CCH, &path_end, &free_size);
    ok(hr == S_FALSE, "expect hr %#lx, got %#lx\n", S_FALSE, hr);

    /* Size < original path length + 1, don't read beyond size */
    MultiByteToWideChar(CP_ACP, 0, "C:\\a", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchRemoveBackslashEx(pathW, ARRAY_SIZE("C:\\a") - 1, &path_end, &free_size);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(removebackslashex_tests); i++)
    {
        const struct removebackslashex_test *t = removebackslashex_tests + i;
        path_size = MultiByteToWideChar(CP_ACP, 0, t->path_in, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchRemoveBackslashEx(pathW, path_size, &path_end, &free_size);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path_in, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            ok(path_end - pathW == t->end_offset, "path %s expect end offset %d, got %Id\n", t->path_in, t->end_offset,
               (INT_PTR)(path_end - pathW));
            ok(free_size == t->free_size, "path %s expect free size %Iu, got %Iu\n", t->path_in, t->free_size, free_size);
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, ARRAY_SIZE(pathA), NULL, NULL);
            ok(!lstrcmpA(pathA, t->path_out), "path %s expect output path %s, got %s\n", t->path_in, t->path_out,
               pathA);
        }
    }
}

struct removeextension_test
{
    const CHAR *path;
    const CHAR *expected;
    HRESULT hr;
};

static const struct removeextension_test removeextension_tests[] =
{
    {"1.exe", "1", S_OK},
    {"C:1.exe", "C:1", S_OK},
    {"C:\\1.exe", "C:\\1", S_OK},
    {"\\1.exe", "\\1", S_OK},
    {"\\\\1.exe", "\\\\1", S_OK},
    {"\\\\?\\C:1.exe", "\\\\?\\C:1", S_OK},
    {"\\\\?\\C:\\1.exe", "\\\\?\\C:\\1", S_OK},
    {"\\\\?\\UNC\\1.exe", "\\\\?\\UNC\\1", S_OK},
    {"\\\\?\\UNC\\192.168.1.1\\1.exe", "\\\\?\\UNC\\192.168.1.1\\1", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1.exe",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1", S_OK},

    /* Malformed */
    {"", "", S_FALSE},
    {" ", " ", S_FALSE},
    {".", "", S_OK},
    {"..", ".", S_OK},
    {"a", "a", S_FALSE},
    {"a.", "a", S_OK},
    {".a.b.", ".a.b", S_OK},
    {"a. ", "a. ", S_FALSE},
    {"a.\\", "a.\\", S_FALSE},
    {"\\\\?\\UNC\\192.168.1.1", "\\\\?\\UNC\\192.168.1", S_OK},
    {"\\\\?\\UNC\\192.168.1.1\\", "\\\\?\\UNC\\192.168.1.1\\", S_FALSE},
    {"\\\\?\\UNC\\192.168.1.1\\a", "\\\\?\\UNC\\192.168.1.1\\a", S_FALSE}
};

static void test_PathCchRemoveExtension(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH] = {0};
    CHAR pathA[PATHCCH_MAX_CCH];
    HRESULT hr;
    INT i;

    if (!pPathCchRemoveExtension)
    {
        win_skip("PathCchRemoveExtension() is not available.\n");
        return;
    }

    /* Arguments check */
    hr = pPathCchRemoveExtension(NULL, PATHCCH_MAX_CCH);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRemoveExtension(pathW, 0);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRemoveExtension(pathW, PATHCCH_MAX_CCH + 1);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRemoveExtension(pathW, PATHCCH_MAX_CCH);
    ok(hr == S_FALSE, "expect %#lx, got %#lx\n", S_FALSE, hr);

    /* Size < original path length + 1 */
    MultiByteToWideChar(CP_ACP, 0, "C:\\1.exe", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchRemoveExtension(pathW, ARRAY_SIZE("C:\\1.exe") - 1);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(removeextension_tests); i++)
    {
        const struct removeextension_test *t = removeextension_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchRemoveExtension(pathW, ARRAY_SIZE(pathW));
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, ARRAY_SIZE(pathA), NULL, NULL);
            ok(!lstrcmpA(pathA, t->expected), "path %s expect stripped path %s, got %s\n", t->path, t->expected, pathA);
        }
    }
}

struct removefilespec_test
{
    const CHAR *path;
    const CHAR *expected;
    HRESULT hr;
    SIZE_T size;
};

static const struct removefilespec_test removefilespec_tests[] =
{
    {"", "", S_FALSE},
    {"a", "", S_OK},
    {"a\\", "a", S_OK},
    {"a\\b", "a", S_OK},

    {"\\", "\\", S_FALSE},
    {"\\a", "\\", S_OK},
    {"\\a\\", "\\a", S_OK},
    {"\\a\\b", "\\a", S_OK},

    {"\\\\", "\\\\", S_FALSE},
    {"\\\\a", "\\\\a", S_FALSE},
    {"\\\\a\\", "\\\\a", S_OK},
    {"\\\\a\\b", "\\\\a\\b", S_FALSE},
    {"\\\\a\\b\\", "\\\\a\\b", S_OK},
    {"\\\\a\\b\\c", "\\\\a\\b", S_OK},
    {"\\\\\\\\\\\\", "\\\\\\\\", S_OK},

    {"C:", "C:", S_FALSE},
    {"C:a", "C:", S_OK},
    {"C:a\\", "C:a", S_OK},
    {"C:a\\b", "C:a", S_OK},
    {"C:\\a\\\\b", "C:\\a", S_OK},

    {"C:\\", "C:\\", S_FALSE},
    {"C:\\a", "C:\\", S_OK},
    {"C:\\a\\", "C:\\a", S_OK},
    {"C:\\a\\b", "C:\\a", S_OK},

    {"\\\\?\\", "\\\\?", S_OK},
    {"\\\\?\\a", "\\\\?", S_OK},
    {"\\\\?\\a\\", "\\\\?\\a", S_OK},
    {"\\\\?\\a\\b", "\\\\?\\a", S_OK},

    {"\\\\?\\C:", "\\\\?\\C:", S_FALSE},
    {"\\\\?\\C:a", "\\\\?\\C:", S_OK},
    {"\\\\?\\C:a\\", "\\\\?\\C:a", S_OK},
    {"\\\\?\\C:a\\b", "\\\\?\\C:a", S_OK},

    {"\\\\?\\C:\\", "\\\\?\\C:\\", S_FALSE},
    {"\\\\?\\C:\\a", "\\\\?\\C:\\", S_OK},
    {"\\\\?\\C:\\a\\", "\\\\?\\C:\\a", S_OK},
    {"\\\\?\\C:\\a\\b", "\\\\?\\C:\\a", S_OK},

    {"\\\\?\\UNC\\", "\\\\?\\UNC\\", S_FALSE},
    {"\\\\?\\UNC\\a", "\\\\?\\UNC\\a", S_FALSE},
    {"\\\\?\\UNC\\a\\", "\\\\?\\UNC\\a", S_OK},
    {"\\\\?\\UNC\\a\\b", "\\\\?\\UNC\\a\\b", S_FALSE},
    {"\\\\?\\UNC\\a\\b\\", "\\\\?\\UNC\\a\\b", S_OK},
    {"\\\\?\\UNC\\a\\b\\c", "\\\\?\\UNC\\a\\b", S_OK},

    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a\\b",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a", S_OK},

    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\b",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", S_OK},

    /* Size tests */
    {"C:\\a", NULL, E_INVALIDARG, PATHCCH_MAX_CCH + 1},
    {"C:\\a", "C:\\", S_OK, PATHCCH_MAX_CCH},
    /* Size < original path length + 1, read beyond size */
    {"C:\\a", "C:\\", S_OK, ARRAY_SIZE("C:\\a") - 1},
    /* Size < result path length + 1 */
    {"C:\\a", NULL, E_INVALIDARG, ARRAY_SIZE("C:\\") - 1}
};

static void test_PathCchRemoveFileSpec(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH] = {0};
    CHAR pathA[PATHCCH_MAX_CCH];
    SIZE_T size;
    HRESULT hr;
    INT i;

    if (!pPathCchRemoveFileSpec)
    {
        win_skip("PathCchRemoveFileSpec() is not available.\n");
        return;
    }

    /* Null arguments */
    hr = pPathCchRemoveFileSpec(NULL, ARRAY_SIZE(pathW));
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRemoveFileSpec(pathW, 0);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(removefilespec_tests); i++)
    {
        const struct removefilespec_test *t = removefilespec_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        size = t->size ? t->size : ARRAY_SIZE(pathW);
        hr = pPathCchRemoveFileSpec(pathW, size);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, ARRAY_SIZE(pathA), NULL, NULL);
            ok(!lstrcmpA(pathA, t->expected), "path %s expect stripped path %s, got %s\n", t->path, t->expected, pathA);
        }
    }
}

struct renameextension_test
{
    const CHAR *path;
    const CHAR *extension;
    const CHAR *expected;
};

static const struct renameextension_test renameextension_tests[] =
{
    {"1.exe", ".txt", "1.txt"},
    {"C:1.exe", ".txt", "C:1.txt"},
    {"C:\\1.exe", ".txt", "C:\\1.txt"},
    {"\\1.exe", ".txt", "\\1.txt"},
    {"\\\\1.exe", ".txt", "\\\\1.txt"},
    {"\\\\?\\C:1.exe", ".txt", "\\\\?\\C:1.txt"},
    {"\\\\?\\C:\\1.exe", ".txt", "\\\\?\\C:\\1.txt"},
    {"\\\\?\\UNC\\1.exe", ".txt", "\\\\?\\UNC\\1.txt"},
    {"\\\\?\\UNC\\192.168.1.1\\1.exe", ".txt", "\\\\?\\UNC\\192.168.1.1\\1.txt"},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1.exe", ".txt",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\1.txt"},
    {"C:\\1.exe", "", "C:\\1"},
    {"C:\\1.exe", "txt", "C:\\1.txt"}
};

static void test_PathCchRenameExtension(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH + 1];
    CHAR pathA[PATHCCH_MAX_CCH + 1];
    WCHAR extensionW[MAX_PATH];
    HRESULT hr;
    INT i;

    if (!pPathCchRenameExtension)
    {
        win_skip("PathCchRenameExtension() is not available.\n");
        return;
    }

    /* Invalid arguments */
    MultiByteToWideChar(CP_ACP, 0, "C:\\1.txt", -1, pathW, ARRAY_SIZE(pathW));
    MultiByteToWideChar(CP_ACP, 0, ".exe", -1, extensionW, ARRAY_SIZE(extensionW));

    hr = pPathCchRenameExtension(NULL, PATHCCH_MAX_CCH, extensionW);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRenameExtension(pathW, 0, extensionW);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRenameExtension(pathW, PATHCCH_MAX_CCH, NULL);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    /* Path length */
    hr = pPathCchRenameExtension(pathW, ARRAY_SIZE("C:\\1.exe") - 1, extensionW);
    ok(E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRenameExtension(pathW, PATHCCH_MAX_CCH + 1, extensionW);
    ok(hr == E_INVALIDARG, "expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchRenameExtension(pathW, PATHCCH_MAX_CCH, extensionW);
    ok(hr == S_OK, "expect result %#lx, got %#lx\n", S_OK, hr);

    for (i = 0; i < ARRAY_SIZE(renameextension_tests); i++)
    {
        const struct renameextension_test *t = renameextension_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        MultiByteToWideChar(CP_ACP, 0, t->extension, -1, extensionW, ARRAY_SIZE(extensionW));
        hr = pPathCchRenameExtension(pathW, PATHCCH_MAX_CCH, extensionW);
        ok(hr == S_OK, "path %s extension %s expect result %#lx, got %#lx\n", t->path, t->extension, S_OK, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, pathA, ARRAY_SIZE(pathA), NULL, NULL);
            ok(!lstrcmpA(pathA, t->expected), "path %s extension %s expect output path %s, got %s\n", t->path,
               t->extension, t->expected, pathA);
        }
    }
}

struct skiproot_test
{
    const char *path;
    int root_offset;
    HRESULT hr;
};

static const struct skiproot_test skiproot_tests [] =
{
    /* Basic combination */
    {"", 0, E_INVALIDARG},
    {"C:\\", 3, S_OK},
    {"\\", 1, S_OK},
    {"\\\\.\\", 4, S_OK},
    {"\\\\?\\UNC\\", 8, S_OK},
    {"\\\\?\\C:\\", 7, S_OK},

    /* Basic + \ */
    {"C:\\\\", 3, S_OK},
    {"\\\\", 2, S_OK},
    {"\\\\.\\\\", 4, S_OK},
    {"\\\\?\\UNC\\\\", 9, S_OK},
    {"\\\\?\\C:\\\\", 7, S_OK},

    /* Basic + a */
    {"a", 0, E_INVALIDARG},
    {"C:\\a", 3, S_OK},
    {"\\a", 1, S_OK},
    {"\\\\.\\a", 5, S_OK},
    {"\\\\?\\UNC\\a", 9, S_OK},

    /* Basic + \a */
    {"\\a", 1, S_OK},
    {"C:\\\\a", 3, S_OK},
    {"\\\\a", 3, S_OK},
    {"\\\\.\\\\a", 4, S_OK},
    {"\\\\?\\UNC\\\\a", 10, S_OK},
    {"\\\\?\\C:\\\\a", 7, S_OK},

    /* Basic + a\ */
    {"a\\", 0, E_INVALIDARG},
    {"C:\\a\\", 3, S_OK},
    {"\\a\\", 1, S_OK},
    {"\\\\.\\a\\", 6, S_OK},
    {"\\\\?\\UNC\\a\\", 10, S_OK},
    {"\\\\?\\C:\\a\\", 7, S_OK},

    /* Network share */
    {"\\\\\\\\", 3, S_OK},
    {"\\\\a\\", 4, S_OK},
    {"\\\\a\\b", 5, S_OK},
    {"\\\\a\\b\\", 6, S_OK},
    {"\\\\a\\b\\\\", 6, S_OK},
    {"\\\\a\\b\\\\c", 6, S_OK},
    {"\\\\a\\b\\c", 6, S_OK},
    {"\\\\a\\b\\c\\", 6, S_OK},
    {"\\\\a\\b\\c\\d", 6, S_OK},
    {"\\\\a\\\\b\\c\\", 4, S_OK},
    {"\\\\aa\\bb\\cc\\", 8, S_OK},

    /* UNC */
    {"\\\\?\\UNC\\\\", 9, S_OK},
    {"\\\\?\\UNC\\a\\b", 11, S_OK},
    {"\\\\?\\UNC\\a\\b", 11, S_OK},
    {"\\\\?\\UNC\\a\\b\\", 12, S_OK},
    {"\\\\?\\UNC\\a\\b\\c", 12, S_OK},
    {"\\\\?\\UNC\\a\\b\\c\\", 12, S_OK},
    {"\\\\?\\UNC\\a\\b\\c\\d", 12, S_OK},
    {"\\\\?\\C:", 6, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", 48, S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", 49, S_OK},
    {"\\\\?\\unc\\a\\b", 11, S_OK},
    {"\\\\?\\volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", 49, S_OK},
    {"\\\\?\\volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a", 49, S_OK},

    /* Malformed */
    {"C:", 2, S_OK},
    {":", 0, E_INVALIDARG},
    {":\\", 0, E_INVALIDARG},
    {"C\\", 0, E_INVALIDARG},
    {"\\?", 1, S_OK},
    {"\\?\\UNC", 1, S_OK},
    {"\\\\?\\", 0, E_INVALIDARG},
    {"\\\\?\\UNC", 0, E_INVALIDARG},
    {"\\\\?\\::\\", 0, E_INVALIDARG},
    {"\\\\?\\Volume", 0, E_INVALIDARG},
    {"\\.", 1, S_OK},
    {"\\\\..", 4, S_OK},
    {"\\\\..a", 5, S_OK}
};

static void test_PathCchSkipRoot(void)
{
    WCHAR pathW[MAX_PATH];
    const WCHAR *root_end;
    HRESULT hr;
    INT i;

    if (!pPathCchSkipRoot)
    {
        win_skip("PathCchSkipRoot() is not available.\n");
        return;
    }

    root_end = (const WCHAR *)0xdeadbeef;
    hr = pPathCchSkipRoot(NULL, &root_end);
    ok(hr == E_INVALIDARG, "Expect result %#lx, got %#lx\n", E_INVALIDARG, hr);
    ok(root_end == (const WCHAR *)0xdeadbeef, "Expect root_end 0xdeadbeef, got %p\n", root_end);

    MultiByteToWideChar(CP_ACP, 0, "C:\\", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchSkipRoot(pathW, NULL);
    ok(hr == E_INVALIDARG, "Expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(skiproot_tests); i++)
    {
        const struct skiproot_test *t = skiproot_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        hr = pPathCchSkipRoot(pathW, &root_end);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path, t->hr, hr);
        if (SUCCEEDED(hr))
            ok(root_end - pathW == t->root_offset, "path %s expect root offset %d, got %Id\n", t->path, t->root_offset,
               (INT_PTR)(root_end - pathW));
    }
}

struct stripprefix_test
{
    const CHAR *path;
    const CHAR *stripped_path;
    HRESULT hr;
    SIZE_T size;
};

static const struct stripprefix_test stripprefix_tests[] =
{
    {"\\\\?\\UNC\\", "\\\\", S_OK},
    {"\\\\?\\UNC\\a", "\\\\a", S_OK},
    {"\\\\?\\C:", "C:", S_OK},
    {"\\\\?\\C:\\", "C:\\", S_OK},
    {"\\\\?\\C:\\a", "C:\\a", S_OK},
    {"\\\\?\\unc\\", "\\\\", S_OK},
    {"\\\\?\\c:\\", "c:\\", S_OK},

    {"\\", "\\", S_FALSE},
    {"\\\\", "\\\\", S_FALSE},
    {"\\\\a", "\\\\a", S_FALSE},
    {"\\\\a\\", "\\\\a\\", S_FALSE},
    {"\\\\?\\a", "\\\\?\\a", S_FALSE},
    {"\\\\?\\UNC", "\\\\?\\UNC", S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_FALSE},

    /* Size Tests */
    {"C:\\", NULL, E_INVALIDARG, PATHCCH_MAX_CCH + 1},
    {"C:\\", "C:\\", S_FALSE, PATHCCH_MAX_CCH},
    /* Size < original path actual length + 1, read beyond size */
    {"\\\\?\\C:\\", "C:\\", S_OK, ARRAY_SIZE("\\\\?\\C:\\") - 1},
    /* Size < stripped path length + 1 */
    {"\\\\?\\C:\\", NULL, E_INVALIDARG, ARRAY_SIZE("C:\\") - 1},
    {"\\\\?\\UNC\\", NULL, E_INVALIDARG, ARRAY_SIZE("\\\\") - 1}
};

static void test_PathCchStripPrefix(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH + 1] = {0};
    CHAR stripped_pathA[PATHCCH_MAX_CCH];
    SIZE_T size;
    HRESULT hr;
    INT i;

    if (!pPathCchStripPrefix)
    {
        win_skip("PathCchStripPrefix(() is not available.\n");
        return;
    }

    /* Null arguments */
    hr = pPathCchStripPrefix(NULL, PATHCCH_MAX_CCH);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    hr = pPathCchStripPrefix(pathW, 0);
    ok(hr == E_INVALIDARG, "expect %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(stripprefix_tests); i++)
    {
        const struct stripprefix_test *t = stripprefix_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        size = t->size ? t->size : PATHCCH_MAX_CCH;
        hr = pPathCchStripPrefix(pathW, size);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, stripped_pathA, ARRAY_SIZE(stripped_pathA), NULL, NULL);
            ok(!lstrcmpA(stripped_pathA, t->stripped_path), "path %s expect stripped path %s, got %s\n", t->path,
               t->stripped_path, stripped_pathA);
        }
    }
}

struct striptoroot_test
{
    const CHAR *path;
    const CHAR *root;
    HRESULT hr;
    SIZE_T size;
};

static const struct striptoroot_test striptoroot_tests[] =
{
    /* Invalid */
    {"", "", E_INVALIDARG},
    {"C", NULL, E_INVALIDARG},
    {"\\\\?\\UNC", NULL, E_INVALIDARG},

    /* Size */
    {"C:\\", NULL, E_INVALIDARG, PATHCCH_MAX_CCH + 1},
    {"C:\\", "C:\\", S_FALSE, PATHCCH_MAX_CCH},
    /* Size < original path length + 1, read beyond size */
    {"C:\\a", "C:\\", S_OK, ARRAY_SIZE("C:\\a") - 1},
    /* Size < stripped path length + 1 */
    {"C:\\a", "C:\\", E_INVALIDARG, ARRAY_SIZE("C:\\") - 1},
    {"\\\\a\\b\\c", NULL, E_INVALIDARG, ARRAY_SIZE("\\\\a\\b") - 1},

    /* X: */
    {"C:", "C:", S_FALSE},
    {"C:a", "C:", S_OK},
    {"C:a\\b", "C:", S_OK},
    {"C:a\\b\\c", "C:", S_OK},

    /* X:\ */
    {"C:\\", "C:\\", S_FALSE},
    {"C:\\a", "C:\\", S_OK},
    {"C:\\a\\b", "C:\\", S_OK},
    {"C:\\a\\b\\c", "C:\\", S_OK},

    /* \ */
    {"\\", "\\", S_FALSE},
    {"\\a", "\\", S_OK},
    {"\\a\\b", "\\", S_OK},
    {"\\a\\b\\c", "\\", S_OK},

    /* \\ */
    {"\\\\", "\\\\", S_FALSE},
    {"\\\\a", "\\\\a", S_FALSE},
    {"\\\\a\\b", "\\\\a\\b", S_FALSE},
    {"\\\\a\\b\\c", "\\\\a\\b", S_OK},
    {"\\\\\\", "\\\\", S_OK},

    /* UNC */
    {"\\\\?\\UNC\\", "\\\\?\\UNC\\", S_FALSE},
    {"\\\\?\\UNC\\a", "\\\\?\\UNC\\a", S_FALSE},
    {"\\\\?\\UNC\\a\\b", "\\\\?\\UNC\\a\\b", S_FALSE},
    {"\\\\?\\UNC\\a\\b\\", "\\\\?\\UNC\\a\\b", S_OK},
    {"\\\\?\\UNC\\a\\b\\c", "\\\\?\\UNC\\a\\b", S_OK},

    /* Prefixed X: */
    {"\\\\?\\C:", "\\\\?\\C:", S_FALSE},
    {"\\\\?\\C:a", "\\\\?\\C:", S_OK},
    {"\\\\?\\C:a\\b", "\\\\?\\C:", S_OK},
    {"\\\\?\\C:a\\b\\c", "\\\\?\\C:", S_OK},

    /* Prefixed X:\ */
    {"\\\\?\\C:\\", "\\\\?\\C:\\", S_FALSE},
    {"\\\\?\\C:\\a", "\\\\?\\C:\\", S_OK},
    {"\\\\?\\C:\\a\\b", "\\\\?\\C:\\", S_OK},
    {"\\\\?\\C:\\a\\b\\c", "\\\\?\\C:\\", S_OK},

    /* UNC Volume */
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a\\b",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}a\\b\\c",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}", S_OK},

    /* UNC Volume with backslash */
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\b",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_OK},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\a\\b\\c",
     "\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", S_OK},
};

static void test_PathCchStripToRoot(void)
{
    WCHAR pathW[PATHCCH_MAX_CCH];
    CHAR rootA[PATHCCH_MAX_CCH];
    SIZE_T size;
    HRESULT hr;
    INT i;

    if (!pPathCchStripToRoot)
    {
        win_skip("PathCchStripToRoot() is not available.\n");
        return;
    }

    /* Null arguments */
    hr = pPathCchStripToRoot(NULL, ARRAY_SIZE(pathW));
    ok(hr == E_INVALIDARG, "Expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    MultiByteToWideChar(CP_ACP, 0, "C:\\a", -1, pathW, ARRAY_SIZE(pathW));
    hr = pPathCchStripToRoot(pathW, 0);
    ok(hr == E_INVALIDARG, "Expect result %#lx, got %#lx\n", E_INVALIDARG, hr);

    for (i = 0; i < ARRAY_SIZE(striptoroot_tests); i++)
    {
        const struct striptoroot_test *t = striptoroot_tests + i;
        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        size = t->size ? t->size : ARRAY_SIZE(pathW);
        hr = pPathCchStripToRoot(pathW, size);
        ok(hr == t->hr, "path %s expect result %#lx, got %#lx\n", t->path, t->hr, hr);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pathW, -1, rootA, ARRAY_SIZE(rootA), NULL, NULL);
            ok(!lstrcmpA(rootA, t->root), "path %s expect stripped path %s, got %s\n", t->path, t->root, rootA);
        }
    }
}

struct isuncex_test
{
    const CHAR *path;
    INT server_offset;
    BOOL ret;
};

static const struct isuncex_test isuncex_tests[] =
{
    {"\\\\", 2, TRUE},
    {"\\\\a\\", 2, TRUE},
    {"\\\\.\\", 2, TRUE},
    {"\\\\?\\UNC\\", 8, TRUE},
    {"\\\\?\\UNC\\a", 8, TRUE},
    {"\\\\?\\unc\\", 8, TRUE},
    {"\\\\?\\unc\\a", 8, TRUE},

    {"", 0, FALSE},
    {"\\", 0, FALSE},
    {"C:\\", 0, FALSE},
    {"\\??\\", 0, FALSE},
    {"\\\\?\\", 0, FALSE},
    {"\\\\?\\UNC", 0, FALSE},
    {"\\\\?\\C:", 0, FALSE},
    {"\\\\?\\C:\\", 0, FALSE},
    {"\\\\?\\C:\\a", 0, FALSE},
    {"\\\\?\\Volume{e51a1864-6f2d-4019-b73d-f4e60e600c26}\\", 0, FALSE}
};

static void test_PathIsUNCEx(void)
{
    WCHAR pathW[MAX_PATH];
    const WCHAR *server;
    BOOL ret;
    INT i;

    if (!pPathIsUNCEx)
    {
        win_skip("PathIsUNCEx(() is not available.\n");
        return;
    }

    /* No NULL check for path pointers on Windows */
    if (0)
    {
        ret = pPathIsUNCEx(NULL, &server);
        ok(ret == FALSE, "expect FALSE\n");
    }

    MultiByteToWideChar(CP_ACP, 0, "C:\\", -1, pathW, ARRAY_SIZE(pathW));
    ret = pPathIsUNCEx(pathW, NULL);
    ok(ret == FALSE, "expect FALSE\n");

    for (i = 0; i < ARRAY_SIZE(isuncex_tests); i++)
    {
        const struct isuncex_test *t = isuncex_tests + i;

        MultiByteToWideChar(CP_ACP, 0, t->path, -1, pathW, ARRAY_SIZE(pathW));
        server = (const WCHAR *)0xdeadbeef;
        ret = pPathIsUNCEx(pathW, &server);
        ok(ret == t->ret, "path \"%s\" expect return %d, got %d\n", t->path, t->ret, ret);
        if (ret)
            ok(server == pathW + t->server_offset, "path \"%s\" expect server offset %d, got %Id\n", t->path,
               t->server_offset, (INT_PTR)(server - pathW));
        else
            ok(!server, "expect server is null, got %p\n", server);
    }
}

static void test_path_manipulation(void)
{
    static const struct
    {
        const char *path;
        BOOL is_root;
        BOOL stript_to_root_ret;
        HRESULT cch_strip_to_root_hr;
        const char *strip_to_root;
        BOOL remove_file_ret;
        const char *remove_file;
        BOOL aw_differ, w_version;
    }
    tests[] =
    {
        {"Q:\\", TRUE, TRUE, S_FALSE, "Q:\\", FALSE, "Q:\\"},
        {"Q:/", FALSE, TRUE, S_OK,  "Q:", TRUE, "Q:"},
        {"Q:", FALSE, TRUE, S_FALSE, "Q:", FALSE, "Q:"},
        {"|:", FALSE, FALSE, E_INVALIDARG, "", TRUE, ""},
        {"Q:\\\\", FALSE, TRUE, S_OK,  "Q:\\", TRUE, "Q:\\"},
        {"Q:\\\\test1", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\"},
        {"Q:\\test1\\test2", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\test1"},
        {"Q:\\test1\\\\test2", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\test1"},
        {"Q:\\test1\\\\\\test2", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\test1\\"},
        {"Q:\\test1\\test2\\", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\test1\\test2"},
        {"Q:\\test1\\\\test2\\", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\test1\\\\test2"},
        {"Q:\\test1\\test2\\\\", FALSE, TRUE, S_OK, "Q:\\", TRUE, "Q:\\test1\\test2"},
        {"Q:/test1/test2", FALSE, TRUE, S_OK, "Q:", TRUE, "Q:"},
        {"Q:/test1\\test2", FALSE, TRUE, S_OK, "Q:", TRUE, "Q:/test1"},
        {"0:/test1/test2", FALSE, FALSE, E_INVALIDARG, "", TRUE, ""},
        {"test", FALSE, FALSE, E_INVALIDARG, "", TRUE, ""},
        {"\\\\?\\Q:\\", FALSE, FALSE, S_OK, "", TRUE, "\\\\?\\Q:", TRUE, FALSE},
        {"\\\\?\\Q:\\", TRUE, TRUE, S_FALSE, "\\\\?\\Q:\\", FALSE, "\\\\?\\Q:\\", TRUE, TRUE},
        {"\\\\?\\Q:\\a", FALSE, TRUE, S_OK, "\\\\?\\Q:\\", TRUE, "\\\\?\\Q:\\", TRUE, TRUE},
        {"\\\\?\\Q:\\\\", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\Q:", TRUE, FALSE},
        {"\\\\?\\Q:\\\\", FALSE, TRUE, S_OK, "\\\\?\\Q:\\", TRUE, "\\\\?\\Q:\\", TRUE, TRUE},
        {"\\\\?\\Q:", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?", TRUE, FALSE},
        {"\\\\?\\Q:", FALSE, TRUE, S_FALSE, "\\\\?\\Q:", FALSE, "\\\\?\\Q:", TRUE, TRUE},
        {"\\\\?\\aa\\", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?\\aa"},
        {"\\\\?\\aa", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?"},
        {"\\\\.\\Q:\\", FALSE, TRUE, S_OK, "\\\\.\\Q:", TRUE, "\\\\.\\Q:"},
        {"\\\\.\\Q:", TRUE, TRUE, S_FALSE, "\\\\.\\Q:", TRUE, "\\\\."},
        {"\\\\.\\", FALSE, TRUE, S_OK, "\\\\.", TRUE, "\\\\."},
        {"\\\\?\\", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?"},
        {"\\\\?\\\\", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?"},
        {"?", FALSE, FALSE, E_INVALIDARG, "", TRUE, ""},
        {"\\?", FALSE, TRUE, S_OK, "\\", TRUE, "\\"},
        {"\\\\?", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\"},
        {"\\\\*", TRUE, TRUE, S_FALSE, "\\\\*", TRUE, "\\\\"},
        {"\\\\.", TRUE, TRUE, S_FALSE, "\\\\.", TRUE, "\\\\"},
        {"\\\\\\.", TRUE, TRUE, S_FALSE, "\\\\\\.", TRUE, "\\\\\\"},
        {"\\\\\\a", TRUE, TRUE, S_FALSE, "\\\\\\a", TRUE, "\\\\\\"},
        {"\\\\\\\\.", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\"},
        {"\\\\!", TRUE, TRUE, S_FALSE, "\\\\!", TRUE, "\\\\"},
        {"\\\\|", TRUE, TRUE, S_FALSE, "\\\\|", TRUE, "\\\\"},
        {"\\\\a", TRUE, TRUE, S_FALSE, "\\\\a", TRUE, "\\\\"},
        {"\\\\a\\", FALSE, TRUE, S_OK, "\\\\a", TRUE, "\\\\a"},
        {"\\\\a\\Q:\\", FALSE, TRUE, S_OK, "\\\\a\\Q:", TRUE, "\\\\a\\Q:"},
        {"\\\\a\\Q:", TRUE, TRUE, S_FALSE, "\\\\a\\Q:", TRUE, "\\\\a"},
        {"\\\\aa\\Q:\\", FALSE, TRUE, S_OK, "\\\\aa\\Q:", TRUE, "\\\\aa\\Q:"},
        {"\\\\aa\\Q:", TRUE, TRUE, S_FALSE, "\\\\aa\\Q:", TRUE, "\\\\aa"},
        {"\\\\aa", TRUE, TRUE, S_FALSE, "\\\\aa", TRUE, "\\\\"},
        {"\\\\aa\\", FALSE, TRUE, S_OK, "\\\\aa", TRUE, "\\\\aa"},
        {"\\\\aa\\b", TRUE, TRUE, S_FALSE, "\\\\aa\\b", TRUE, "\\\\aa"},
        {"\\\\aa\\|", TRUE, TRUE, S_FALSE, "\\\\aa\\|", TRUE, "\\\\aa"},
        {"\\\\aa\\b\\c", FALSE, TRUE, S_OK, "\\\\aa\\b", TRUE, "\\\\aa\\b"},
        {"\\\\Q:", TRUE, TRUE, S_FALSE, "\\\\Q:", FALSE, "\\\\Q:"},
        {"\\\\\\Q:", TRUE, TRUE, S_FALSE, "\\\\\\Q:", TRUE, "\\\\\\"},
        {"\\\\Q:\\", FALSE, TRUE, S_OK, "\\\\Q:", TRUE, "\\\\Q:"},
        {"\\\\?Q:\\", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?Q:"},
        {"\\??\\Q:\\", FALSE, TRUE, S_OK, "\\", TRUE, "\\??\\Q:"},
        {"\\??\\", FALSE, TRUE, S_OK, "\\", TRUE, "\\??"},
        {"\\\\o\\Q:\\aaa", FALSE, TRUE, S_OK, "\\\\o\\Q:", TRUE, "\\\\o\\Q:"},
        {"||||Q:\\aaa", FALSE, FALSE, E_INVALIDARG, "", TRUE, "||||Q:"},
        {"\\\\\\\\Q:\\", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\\\\\Q:"},
        {"\\\\\\\\Q:", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\"},
        {"\\\\", TRUE, TRUE, S_FALSE, "\\\\", FALSE, "\\\\"},
        {"\\\\\\", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\"},
        {"\\\\\\\\", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\"},
        {"\\\\\\\\\\", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\\\"},
        {"\\\\\\\\\\\\", FALSE, TRUE, S_OK, "\\\\", TRUE, "\\\\\\\\"},
        {"\\", TRUE, TRUE, S_FALSE, "\\", FALSE, "\\"},
        {"\\a", FALSE, TRUE, S_OK, "\\", TRUE, "\\"},
        {"\\a\\b", FALSE, TRUE, S_OK, "\\", TRUE, "\\a"},
        {"\\\\a", TRUE, TRUE, S_FALSE, "\\\\a", TRUE, "\\\\"},
        {"\\\\a\\b", TRUE, TRUE, S_FALSE, "\\\\a\\b", TRUE, "\\\\a"},
        {"\\\\a\\\\b", FALSE, TRUE, S_OK, "\\\\a", TRUE, "\\\\a"},
        {"\\\\a\\b\\", FALSE, TRUE, S_OK, "\\\\a\\b", TRUE, "\\\\a\\b"},
        {"\\\\a\\b\\c", FALSE, TRUE, S_OK, "\\\\a\\b", TRUE, "\\\\a\\b"},
        {"", FALSE, FALSE, E_INVALIDARG, "", FALSE, ""},
        {"\\\\Q:\\", FALSE, TRUE, S_OK, "\\\\Q:", TRUE, "\\\\Q:"},
        {"\\Q:\\", FALSE, TRUE, S_OK, "\\", TRUE, "\\Q:"},
        {"\\Q:", FALSE, TRUE, S_OK, "\\", TRUE, "\\"},
        {"\\\\?\\UNC\\", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\UNC", TRUE, FALSE},
        {"\\\\?\\UNC\\", TRUE, TRUE, S_FALSE, "\\\\?\\UNC\\", TRUE, "\\\\?\\UNC", TRUE, TRUE},
        {"\\\\?\\UNC\\a", FALSE, FALSE, S_OK, "", TRUE, "\\\\?\\UNC", TRUE, FALSE},
        {"\\\\?\\UNC\\a", TRUE, TRUE, S_FALSE, "\\\\?\\UNC\\a", TRUE, "\\\\?\\UNC", TRUE, TRUE},
        {"\\\\?\\UNC\\a\\", FALSE, FALSE, S_OK, "", TRUE, "\\\\?\\UNC\\a", TRUE, FALSE},
        {"\\\\?\\UNC\\a\\", FALSE, TRUE, S_OK, "\\\\?\\UNC\\a", TRUE, "\\\\?\\UNC\\a", TRUE, TRUE},
        {"\\\\?\\UNC\\a\\b", FALSE, FALSE, S_OK, "", TRUE, "\\\\?\\UNC\\a", TRUE, FALSE},
        {"\\\\?\\UNC\\a\\b", TRUE, TRUE, S_FALSE, "\\\\?\\UNC\\a\\b", TRUE, "\\\\?\\UNC\\a", TRUE, TRUE},
        {"\\\\?\\UNC\\a\\b\\c", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\UNC\\a\\b", TRUE, FALSE},
        {"\\\\?\\UNC\\a\\b\\c", FALSE, TRUE, S_OK, "\\\\?\\UNC\\a\\b", TRUE, "\\\\?\\UNC\\a\\b", TRUE, TRUE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?", TRUE, FALSE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", FALSE, TRUE, S_FALSE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", FALSE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", TRUE, TRUE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", TRUE, FALSE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", TRUE, TRUE, S_FALSE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", FALSE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", TRUE, TRUE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\\\", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", TRUE, FALSE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\\\", FALSE, TRUE, S_OK, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", TRUE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", TRUE, TRUE},
        {"\\\\?\\Volume{zaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?\\Volume{zaaaaaaa-bbbb-cccc-dddd-ffffffffffff}"},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\a", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}", TRUE, FALSE},
        {"\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\a", FALSE, TRUE, S_OK, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", TRUE, "\\\\?\\Volume{aaaaaaaa-bbbb-cccc-dddd-ffffffffffff}\\", TRUE, TRUE},
        {"\\\\?\\TST\\", FALSE, FALSE, E_INVALIDARG, "", TRUE, "\\\\?\\TST"},
        {"\\\\?\\UNC\\Q:\\", FALSE, FALSE, S_FALSE, "", TRUE, "\\\\?\\UNC\\Q:", TRUE, FALSE},
        {"\\\\?\\UNC\\Q:\\", FALSE, TRUE, S_OK, "\\\\?\\UNC\\Q:", TRUE, "\\\\?\\UNC\\Q:", TRUE, TRUE},
        {"\\Device\\Harddiskvolume1", FALSE, TRUE, S_OK, "\\", TRUE, "\\Device"},
        {"\\Device\\Harddiskvolume1\\", FALSE, TRUE, S_OK, "\\", TRUE, "\\Device\\Harddiskvolume1"},
    };

    WCHAR pathW[MAX_PATH], expectedW[MAX_PATH];
    unsigned int i, expected_len;
    const WCHAR *end_pfx, *ptr;
    HRESULT hr, expected_hr;
    char path[MAX_PATH];
    BOOL ret;

    if (!pPathCchStripToRoot || !pPathCchSkipRoot || !pPathCchRemoveFileSpec || !pPathCchIsRoot)
    {
        win_skip("Functions are not available.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("%u (%s)", i, debugstr_a(tests[i].path));

        strcpy(path, tests[i].path);

        if (!tests[i].aw_differ || !tests[i].w_version)
        {
            ret = pPathIsRootA(path);
            ok(ret == tests[i].is_root, "PathIsRootA got %d, expected %d.\n", ret, tests[i].is_root);
            ret = pPathStripToRootA(path);
            ok(ret == tests[i].stript_to_root_ret, "PathStripToRootA got %d, expected %d.\n", ret, tests[i].stript_to_root_ret);
            ok(!strcmp(path, tests[i].strip_to_root), "PathStripToRootA got %s, expected %s.\n", debugstr_a(path), debugstr_a(tests[i].strip_to_root));
            strcpy(path, tests[i].path);
            ret = pPathRemoveFileSpecA(path);
            ok(ret == tests[i].remove_file_ret, "PathRemoveFileSpecA got %d, expected %d.\n", ret, tests[i].remove_file_ret);
            ok(!strcmp(path, tests[i].remove_file), "PathRemoveFileSpecA got %s, expected %s.\n", debugstr_a(path), debugstr_a(tests[i].remove_file));
        }

        if (!tests[i].aw_differ || tests[i].w_version)
        {
            MultiByteToWideChar(CP_ACP, 0, tests[i].path, -1, pathW, MAX_PATH);
            ret = pPathIsRootW(pathW);
            ok(ret == tests[i].is_root, "PathIsRootW got %d, expected %d.\n", ret, tests[i].is_root);
            ret = pPathCchIsRoot(pathW);
            ok(ret == tests[i].is_root, "pPathCchIsRoot got %d, expected %d.\n", ret, tests[i].is_root);

            ret = pPathStripToRootW(pathW);
            MultiByteToWideChar(CP_ACP, 0, tests[i].strip_to_root, -1, expectedW, MAX_PATH);
            ok(ret == tests[i].stript_to_root_ret, "PathStripToRootW got %d, expected %d.\n", ret, tests[i].stript_to_root_ret);
            ok(!wcscmp(pathW, expectedW), "PathStripToRootW got %s, expected %s.\n", debugstr_w(pathW), debugstr_w(expectedW));

            MultiByteToWideChar(CP_ACP, 0, tests[i].path, -1, pathW, MAX_PATH);
            hr = pPathCchStripToRoot(pathW, ARRAY_SIZE(pathW));
            ok(hr == tests[i].cch_strip_to_root_hr, "PathCchStripToRoot got hr %#lx, expected %#lx.\n", hr, tests[i].cch_strip_to_root_hr);
            ok(!wcscmp(pathW, expectedW), "PathCchStripToRoot got %s, expected %s.\n", debugstr_w(pathW), debugstr_w(expectedW));

            MultiByteToWideChar(CP_ACP, 0, tests[i].path, -1, pathW, MAX_PATH);
            end_pfx = NULL;
            hr = pPathCchSkipRoot(pathW, &end_pfx);
            expected_hr = SUCCEEDED(tests[i].cch_strip_to_root_hr) ? S_OK : tests[i].cch_strip_to_root_hr;
            ok(hr == expected_hr, "PathCchSkipRoot got hr %#lx, expected %#lx.\n", hr, expected_hr);
            if (SUCCEEDED(hr))
            {
                expected_len = wcslen(expectedW);
                if (end_pfx && end_pfx > pathW/* && end_pfx[-1] == '\\'*/ && pathW[expected_len] == '\\'
                    && (!wcsnicmp(pathW, L"\\\\?\\UNC\\", 8 ) || (pathW[0] == '\\' && pathW[1] == '\\' && pathW[2] && pathW[2] != '?')))
                {
                    ok(end_pfx[-1] == '\\', "PathCchSkipRoot missing trailing backslash (end_pfx %s).\n", debugstr_w(end_pfx));
                    end_pfx--;
                }
                ok(expected_len == end_pfx - pathW, "PathCchSkipRoot got %s, expected %s.\n",
                        debugstr_wn(pathW, end_pfx - pathW), debugstr_wn(pathW, expected_len));
            }
            MultiByteToWideChar(CP_ACP, 0, tests[i].path, -1, pathW, MAX_PATH);
            if (FAILED(hr))
                expected_hr = *pathW ? S_OK : S_FALSE;
            else
                expected_hr = *end_pfx ? S_OK : S_FALSE;
            wcscpy(expectedW, pathW);
            if (expected_hr != S_FALSE)
            {
                if (!end_pfx)
                    end_pfx = pathW;
                if ((ptr = wcsrchr(end_pfx, '\\')))
                {
                    if (ptr > end_pfx && ptr[-1] == '\\' && ptr[1] != '?')
                        --ptr;
                    expectedW[ptr - pathW] = 0;
                }
                else
                {
                    expectedW[end_pfx - pathW] = 0;
                }
            }
            hr = pPathCchRemoveFileSpec(pathW, ARRAY_SIZE(pathW));
            ok(hr == expected_hr, "PathCchRemoveFileSpec got hr %#lx, expected %#lx.\n", hr, expected_hr);
            ok(!wcscmp(pathW, expectedW), "PathCchRemoveFileSpec got %s, expected %s, end_pfx %s.\n", debugstr_w(pathW), debugstr_w(expectedW), debugstr_w(end_pfx));

            MultiByteToWideChar(CP_ACP, 0, tests[i].path, -1, pathW, MAX_PATH);
            ret = pPathRemoveFileSpecW(pathW);
            MultiByteToWideChar(CP_ACP, 0, tests[i].remove_file, -1, expectedW, MAX_PATH);
            ok(ret == tests[i].remove_file_ret, "PathRemoveFileSpecW got %d, expected %d.\n", ret, tests[i].remove_file_ret);
            ok(!wcscmp(pathW, expectedW), "PathRemoveFileSpecW got %s, expected %s.\n", debugstr_w(pathW), debugstr_w(expectedW));
        }

        winetest_pop_context();
    }
}

static void test_actctx(void)
{
    ACTCTX_SECTION_KEYED_DATA data = { sizeof(data) };
    WCHAR exe_path[MAX_PATH];
    char buf[1024];
    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info = (void *)buf;
    SIZE_T size;
    BOOL b;

    b = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, L"testdll.dll", &data);
    ok(b, "FindActCtxSectionString failed: %lu\n", GetLastError());

    b = QueryActCtxW(0, data.hActCtx, &data.ulAssemblyRosterIndex, AssemblyDetailedInformationInActivationContext, buf, sizeof(buf), &size);
    ok(b, "QueryActCtx failed: %lu\n", GetLastError());

    GetModuleFileNameW(NULL, exe_path, ARRAY_SIZE(exe_path));
    ok(!lstrcmpW(info->lpAssemblyManifestPath, exe_path), "lpAssemblyManifestPath = %s expected %s\n", debugstr_w(info->lpAssemblyManifestPath), debugstr_w(exe_path));
    ok(!info->lpAssemblyDirectoryName, "lpAssemblyDirectoryName = %s\n", wine_dbgstr_w(info->lpAssemblyDirectoryName));
}

START_TEST(path)
{
    HMODULE hmod = LoadLibraryA("kernelbase.dll");

    pPathAllocCanonicalize = (void *)GetProcAddress(hmod, "PathAllocCanonicalize");
    pPathAllocCombine = (void *)GetProcAddress(hmod, "PathAllocCombine");
    pPathCchAddBackslash = (void *)GetProcAddress(hmod, "PathCchAddBackslash");
    pPathCchAddBackslashEx = (void *)GetProcAddress(hmod, "PathCchAddBackslashEx");
    pPathCchAddExtension = (void *)GetProcAddress(hmod, "PathCchAddExtension");
    pPathCchAppend = (void *)GetProcAddress(hmod, "PathCchAppend");
    pPathCchAppendEx = (void *)GetProcAddress(hmod, "PathCchAppendEx");
    pPathCchCanonicalize = (void *)GetProcAddress(hmod, "PathCchCanonicalize");
    pPathCchCanonicalizeEx = (void *)GetProcAddress(hmod, "PathCchCanonicalizeEx");
    pPathCchCombine = (void *)GetProcAddress(hmod, "PathCchCombine");
    pPathCchCombineEx = (void *)GetProcAddress(hmod, "PathCchCombineEx");
    pPathCchFindExtension = (void *)GetProcAddress(hmod, "PathCchFindExtension");
    pPathCchIsRoot = (void *)GetProcAddress(hmod, "PathCchIsRoot");
    pPathCchRemoveBackslash = (void *)GetProcAddress(hmod, "PathCchRemoveBackslash");
    pPathCchRemoveBackslashEx = (void *)GetProcAddress(hmod, "PathCchRemoveBackslashEx");
    pPathCchRemoveExtension = (void *)GetProcAddress(hmod, "PathCchRemoveExtension");
    pPathCchRemoveFileSpec = (void *)GetProcAddress(hmod, "PathCchRemoveFileSpec");
    pPathCchRenameExtension = (void *)GetProcAddress(hmod, "PathCchRenameExtension");
    pPathCchSkipRoot = (void *)GetProcAddress(hmod, "PathCchSkipRoot");
    pPathCchStripPrefix = (void *)GetProcAddress(hmod, "PathCchStripPrefix");
    pPathCchStripToRoot = (void *)GetProcAddress(hmod, "PathCchStripToRoot");
    pPathIsUNCEx = (void *)GetProcAddress(hmod, "PathIsUNCEx");

    pPathIsRootA = (void *)GetProcAddress(hmod, "PathIsRootA");
    pPathIsRootW = (void *)GetProcAddress(hmod, "PathIsRootW");
    pPathStripToRootA = (void *)GetProcAddress(hmod, "PathStripToRootA");
    pPathStripToRootW = (void *)GetProcAddress(hmod, "PathStripToRootW");
    pPathRemoveFileSpecA = (void *)GetProcAddress(hmod, "PathRemoveFileSpecA");
    pPathRemoveFileSpecW = (void *)GetProcAddress(hmod, "PathRemoveFileSpecW");

    test_PathAllocCanonicalize();
    test_PathAllocCombine();
    test_PathCchAddBackslash();
    test_PathCchAddBackslashEx();
    test_PathCchAddExtension();
    test_PathCchAppend();
    test_PathCchAppendEx();
    test_PathCchCanonicalize();
    test_PathCchCanonicalizeEx();
    test_PathCchCombine();
    test_PathCchCombineEx();
    test_PathCchFindExtension();
    test_PathCchIsRoot();
    test_PathCchRemoveBackslash();
    test_PathCchRemoveBackslashEx();
    test_PathCchRemoveExtension();
    test_PathCchRemoveFileSpec();
    test_PathCchRenameExtension();
    test_PathCchSkipRoot();
    test_PathCchStripPrefix();
    test_PathCchStripToRoot();
    test_PathIsUNCEx();
    test_path_manipulation();
    test_actctx();
}
