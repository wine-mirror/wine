/*
 * Unit tests for advpack.dll
 *
 * Copyright (C) 2005 Robert Reif
 * Copyright (C) 2005 Sami Aario
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <advpub.h>
#include <assert.h>
#include "wine/test.h"

static HRESULT (WINAPI *pGetVersionFromFile)(LPSTR,LPDWORD,LPDWORD,BOOL);
static HRESULT (WINAPI *pDelNode)(LPCSTR,DWORD);

static void version_test()
{
    HRESULT hr;
    DWORD major, minor;

    major = minor = 0;
    hr = pGetVersionFromFile("kernel32.dll", &major, &minor, FALSE);
    ok (hr == S_OK, "GetVersionFromFileEx(kernel32.dll) failed, returned "
        "0x%08lx\n", hr);

    trace("kernel32.dll Language ID: 0x%08lx, Codepage ID: 0x%08lx\n",
           major, minor);

    major = minor = 0;
    hr = pGetVersionFromFile("kernel32.dll", &major, &minor, TRUE);
    ok (hr == S_OK, "GetVersionFromFileEx(kernel32.dll) failed, returned "
        "0x%08lx\n", hr);

    trace("kernel32.dll version: %d.%d.%d.%d\n", HIWORD(major), LOWORD(major),
          HIWORD(minor), LOWORD(minor));
}

static void delnode_test()
{
    HRESULT hr;
    HANDLE hn;
    CHAR currDir[MAX_PATH];
    int currDirLen;

    /* Native DelNode apparently does not support relative paths, so we use
       absolute paths for testing */
    currDirLen = GetCurrentDirectoryA(sizeof(currDir) / sizeof(CHAR), currDir);
    assert(currDirLen > 0 && currDirLen < sizeof(currDir) / sizeof(CHAR));

    /* Simple tests; these should fail. */
    hr = pDelNode(NULL, 0);
    ok (hr == E_FAIL, "DelNode called with NULL pathname should return E_FAIL\n");
    hr = pDelNode("", 0);
    ok (hr == E_FAIL, "DelNode called with empty pathname should return E_FAIL\n");

    /* Test deletion of a file. */
    hn = CreateFile("DelNodeTestFile1", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hr = pDelNode(lstrcat(currDir, "\\DelNodeTestFile1"), 0);
    ok (hr == S_OK, "DelNode failed deleting a single file\n");
    currDir[currDirLen] = '\0';

    /* Test deletion of an empty directory. */
    CreateDirectoryA("DelNodeTestDir", NULL);
    hr = pDelNode(lstrcat(currDir, "\\DelNodeTestDir"), 0);
    ok (hr == S_OK, "DelNode failed deleting an empty directory\n");
    currDir[currDirLen] = '\0';

    /* Test deletion of a directory containing one file. */
    CreateDirectoryA("DelNodeTestDir", NULL);
    hn = CreateFile("DelNodeTestDir\\DelNodeTestFile1", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hr = pDelNode(lstrcat(currDir, "\\DelNodeTestDir"), 0);
    ok (hr == S_OK, "DelNode failed deleting a directory containing one file\n");
    currDir[currDirLen] = '\0';

    /* Test deletion of a directory containing multiple files. */
    CreateDirectoryA("DelNodeTestDir", NULL);
    hn = CreateFile("DelNodeTestDir\\DelNodeTestFile1", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hn = CreateFile("DelNodeTestDir\\DelNodeTestFile2", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hn = CreateFile("DelNodeTestDir\\DelNodeTestFile3", GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hn != INVALID_HANDLE_VALUE);
    CloseHandle(hn);
    hr = pDelNode(lstrcat(currDir, "\\DelNodeTestDir"), 0);
    ok (hr == S_OK, "DelNode failed deleting a directory containing multiple files\n");
    currDir[currDirLen] = '\0';
}

START_TEST(advpack)
{
    HMODULE hdll;

    hdll = LoadLibraryA("advpack.dll");
    if (!hdll)
        return;

    pGetVersionFromFile = (void*)GetProcAddress(hdll, "GetVersionFromFile");
    pDelNode = (void*)GetProcAddress(hdll, "DelNode");
    if (!pGetVersionFromFile || !pDelNode)
        return;

    version_test();
    delnode_test();

    FreeLibrary(hdll);
}
