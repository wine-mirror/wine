/*
 * URL Cache Tests
 *
 * Copyright 2008 Robert Shearman for CodeWeavers
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/test.h"

#define TEST_URL    "http://urlcachetest.winehq.org/index.html"

static void test_urlcacheA(void)
{
    BOOL ret;
    char filename[MAX_PATH + 1];
    HANDLE hFile;
    DWORD written;
    BYTE zero_byte = 0;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;
    DWORD cbCacheEntryInfo;
    static const FILETIME filetime_zero;

    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filename, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed with error %d\n", GetLastError());

    ret = WriteFile(hFile, &zero_byte, sizeof(zero_byte), &written, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    CloseHandle(hFile);

    ret = CommitUrlCacheEntry(TEST_URL, filename, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, NULL, NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    ret = RetrieveUrlCacheEntryFile(TEST_URL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = RetrieveUrlCacheEntryFile(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo, 0);
    ok(ret, "RetrieveUrlCacheEntryFile failed with error %d\n", GetLastError());

    ok(lpCacheEntryInfo->dwStructSize == sizeof(*lpCacheEntryInfo), "lpCacheEntryInfo->dwStructSize was %d\n", lpCacheEntryInfo->dwStructSize);
    ok(!strcmp(lpCacheEntryInfo->lpszSourceUrlName, TEST_URL), "lpCacheEntryInfo->lpszSourceUrlName should be %s instead of %s\n", TEST_URL, lpCacheEntryInfo->lpszSourceUrlName);

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    ret = UnlockUrlCacheEntryFile(TEST_URL, 0);
    ok(ret, "UnlockUrlCacheEntryFile failed with error %d\n", GetLastError());

    ret = DeleteUrlCacheEntry(TEST_URL);
    ok(ret, "DeleteUrlCacheEntry failed with error %d\n", GetLastError());

    ret = DeleteFile(filename);
    todo_wine
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "local file should no longer exist\n");
}

START_TEST(urlcache)
{
    test_urlcacheA();
}
