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

#include <stdio.h>
#include <windows.h>
#include <advpub.h>
#include <assert.h>
#include "wine/test.h"

#define TEST_STRING1 "\\Application Name"
#define TEST_STRING2 "%49001%\\Application Name"

static HRESULT (WINAPI *pCloseINFEngine)(HINF);
static HRESULT (WINAPI *pDelNode)(LPCSTR,DWORD);
static HRESULT (WINAPI *pGetVersionFromFile)(LPSTR,LPDWORD,LPDWORD,BOOL);
static HRESULT (WINAPI *pOpenINFEngine)(PCSTR,PCSTR,DWORD,HINF*,PVOID);
static HRESULT (WINAPI *pTranslateInfString)(LPSTR,LPSTR,LPSTR,LPSTR,LPSTR,DWORD,LPDWORD,LPVOID);
static HRESULT (WINAPI *pTranslateInfStringEx)(HINF,PCSTR,PCSTR,PCSTR,PSTR,DWORD,PDWORD,PVOID);

static CHAR PROG_FILES[MAX_PATH];
static DWORD PROG_FILES_LEN;

static void get_progfiles_dir(void)
{
    HKEY hkey;
    DWORD size = MAX_PATH;

    RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &hkey);
    RegQueryValueExA(hkey, "ProgramFilesDir", NULL, NULL, (LPBYTE)PROG_FILES, &size);
    RegCloseKey(hkey);

    lstrcatA(PROG_FILES, TEST_STRING1);
    PROG_FILES_LEN = lstrlenA(PROG_FILES) + 1;
}

static BOOL init_function_pointers(void)
{
    HMODULE hAdvPack = LoadLibraryA("advpack.dll");

    if (!hAdvPack)
        return FALSE;

    pCloseINFEngine = (void*)GetProcAddress(hAdvPack, "CloseINFEngine");
    pDelNode = (void *)GetProcAddress(hAdvPack, "DelNode");
    pGetVersionFromFile = (void *)GetProcAddress(hAdvPack, "GetVersionFromFile");
    pOpenINFEngine = (void*)GetProcAddress(hAdvPack, "OpenINFEngine");
    pTranslateInfString = (void *)GetProcAddress(hAdvPack, "TranslateInfString");
    pTranslateInfStringEx = (void*)GetProcAddress(hAdvPack, "TranslateInfStringEx");

    if (!pCloseINFEngine || !pDelNode || !pGetVersionFromFile ||
        !pOpenINFEngine || !pTranslateInfString)
        return FALSE;

    return TRUE;
}

static void version_test(void)
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

    major = minor = 0;
    hr = pGetVersionFromFile("advpack.dll", &major, &minor, FALSE);
    ok (hr == S_OK, "GetVersionFromFileEx(advpack.dll) failed, returned "
        "0x%08lx\n", hr);
    trace("advpack.dll Language ID: 0x%08lx, Codepage ID: 0x%08lx\n",
           major, minor);

    major = minor = 0;
    hr = pGetVersionFromFile("advpack.dll", &major, &minor, TRUE);
    ok (hr == S_OK, "GetVersionFromFileEx(advpack.dll) failed, returned "
        "0x%08lx\n", hr);
    trace("advpack.dll version: %d.%d.%d.%d\n", HIWORD(major), LOWORD(major),
          HIWORD(minor), LOWORD(minor));
}

static void delnode_test(void)
{
    HRESULT hr;
    HANDLE hn;
    CHAR currDir[MAX_PATH];
    int currDirLen;

    /* Native DelNode apparently does not support relative paths, so we use
       absolute paths for testing */
    currDirLen = GetCurrentDirectoryA(sizeof(currDir) / sizeof(CHAR), currDir);
    assert(currDirLen > 0 && currDirLen < sizeof(currDir) / sizeof(CHAR));

    if(currDir[currDirLen - 1] == '\\')
        currDir[--currDirLen] = 0;

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

static void append_str(char **str, const char *data)
{
    sprintf(*str, data);
    *str += strlen(*str);
}

static void create_inf_file()
{
    char data[1024];
    char *ptr = data;
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFile("c:\\test.inf", GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    append_str(&ptr, "[Version]\n");
    append_str(&ptr, "Signature=\"$Chicago$\"\n");
    append_str(&ptr, "[CustInstDestSection]\n");
    append_str(&ptr, "49001=ProgramFilesDir\n");
    append_str(&ptr, "[ProgramFilesDir]\n");
    append_str(&ptr, "HKLM,\"Software\\Microsoft\\Windows\\CurrentVersion\",");
    append_str(&ptr, "\"ProgramFilesDir\",,\"%%24%%\\%%LProgramF%%\"\n");
    append_str(&ptr, "[section]\n");
    append_str(&ptr, "NotACustomDestination=Version\n");
    append_str(&ptr, "CustomDestination=CustInstDestSection\n");
    append_str(&ptr, "[Options.NTx86]\n");
    append_str(&ptr, "49001=ProgramFilesDir\n");
    append_str(&ptr, "InstallDir=%%49001%%\\%%DefaultAppPath%%\n");
    append_str(&ptr, "CustomHDestination=CustInstDestSection\n");
    append_str(&ptr, "[Strings]\n");
    append_str(&ptr, "DefaultAppPath=\"Application Name\"\n");
    append_str(&ptr, "LProgramF=\"Program Files\"\n");

    WriteFile(hf, data, ptr - data, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void translateinfstring_test()
{
    HRESULT hr;
    char buffer[MAX_PATH];
    DWORD dwSize;

    create_inf_file();

    /* pass in a couple invalid parameters */
    hr = pTranslateInfString(NULL, NULL, NULL, NULL, buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08x\n", (UINT)hr);

    /* try to open an inf file that doesn't exist */
    hr = pTranslateInfString("c:\\a.inf", "Options.NTx86", "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == E_INVALIDARG || 
       hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND), 
       "Expected E_INVALIDARG, 0x80070002 or 0x8007007e, got 0x%08x\n", (UINT)hr);

    if(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND))
    {
        trace("WinNT 3.51 detected. Skipping tests for TranslateInfString()\n");
        return;
    }

    /* try a nonexistent section */
    buffer[0] = 0;
    hr = pTranslateInfString("c:\\test.inf", "idontexist", "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", (UINT)hr);
    ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
    ok(dwSize == 25, "Expected size 25, got %ld\n", dwSize);

    buffer[0] = 0;
    /* try other nonexistent section */
    hr = pTranslateInfString("c:\\test.inf", "Options.NTx86", "idontexist",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND || hr == E_INVALIDARG, 
       "Expected SPAPI_E_LINE_NOT_FOUND or E_INVALIDARG, got 0x%08x\n", (UINT)hr);

    buffer[0] = 0;
    /* try nonexistent key */
    hr = pTranslateInfString("c:\\test.inf", "Options.NTx86", "Options.NTx86",
                             "notvalid", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND || hr == E_INVALIDARG, 
       "Expected SPAPI_E_LINE_NOT_FOUND or E_INVALIDARG, got 0x%08x\n", (UINT)hr);

    buffer[0] = 0;
    /* test the behavior of pszInstallSection */
    hr = pTranslateInfString("c:\\test.inf", "section", "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == ERROR_SUCCESS || hr == E_FAIL, 
       "Expected ERROR_SUCCESS or E_FAIL, got 0x%08x\n", (UINT)hr);

    if(hr == ERROR_SUCCESS)
    {
        ok(!strcmp(buffer, PROG_FILES), "Expected '%s', got '%s'\n", PROG_FILES, buffer);
        ok(dwSize == PROG_FILES_LEN, "Expected size %ld, got %ld\n", PROG_FILES_LEN, dwSize);
    }

    buffer[0] = 0;
    /* try without a pszInstallSection */
    hr = pTranslateInfString("c:\\test.inf", NULL, "Options.NTx86",
                             "InstallDir", buffer, MAX_PATH, &dwSize, NULL);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", (UINT)hr);
    todo_wine
    {
        ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
        ok(dwSize == 25, "Expected size 25, got %ld\n", dwSize);
    }

    DeleteFile("c:\\a.inf");
    DeleteFile("c:\\test.inf");
}

static void translateinfstringex_test(void)
{
    HINF hinf;
    HRESULT hr;
    char buffer[MAX_PATH];
    DWORD size = MAX_PATH;

    create_inf_file();
    
    /* need to see if there are any flags */

    /* try a NULL filename */
    hr = pOpenINFEngine(NULL, "Options.NTx86", 0, &hinf, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try an empty filename */
    hr = pOpenINFEngine("", "Options.NTx86", 0, &hinf, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
        "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);

    /* try a NULL hinf */
    hr = pOpenINFEngine("c:\\test.inf", "Options.NTx86", 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* open the INF without the Install section specified */
    hr = pOpenINFEngine("c:\\test.inf", NULL, 0, &hinf, NULL);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);

    /* try a NULL hinf */
    hr = pTranslateInfStringEx(NULL, "c:\\test.inf", "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try a NULL filename */
    hr = pTranslateInfStringEx(hinf, NULL, "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try an empty filename */
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, "", "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", (UINT)hr);
    todo_wine
    {
        ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
        ok(size == 25, "Expected size 25, got %ld\n", size);
    }

    /* try a NULL translate section */
    hr = pTranslateInfStringEx(hinf, "c:\\test.inf", NULL, "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try an empty translate section */
    hr = pTranslateInfStringEx(hinf, "c:\\test.inf", "", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND, "Expected SPAPI_E_LINE_NOT_FOUND, got %ld\n", hr);

    /* try a NULL translate key */
    hr = pTranslateInfStringEx(hinf, "c:\\test.inf", "Options.NTx86", NULL,
                              buffer, size, &size, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try an empty translate key */
    hr = pTranslateInfStringEx(hinf, "c:\\test.inf", "Options.NTx86", "",
                              buffer, size, &size, NULL);
    ok(hr == SPAPI_E_LINE_NOT_FOUND, "Expected SPAPI_E_LINE_NOT_FOUND, got %ld\n", hr);

    /* successfully translate the string */
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, "c:\\test.inf", "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    todo_wine
    {
        ok(!strcmp(buffer, TEST_STRING2), "Expected %s, got %s\n", TEST_STRING2, buffer);
        ok(size == 25, "Expected size 25, got %ld\n", size);
    }

    /* try a NULL hinf */
    hr = pCloseINFEngine(NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* successfully close the hinf */
    hr = pCloseINFEngine(hinf);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);

    /* open the inf with the install section */
    hr = pOpenINFEngine("c:\\test.inf", "section", 0, &hinf, NULL);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);

    /* translate the string with the install section specified */
    size = MAX_PATH;
    hr = pTranslateInfStringEx(hinf, "c:\\test.inf", "Options.NTx86", "InstallDir",
                              buffer, size, &size, NULL);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(!strcmp(buffer, PROG_FILES), "Expected %s, got %s\n", PROG_FILES, buffer);
    ok(size == PROG_FILES_LEN, "Expected size %ld, got %ld\n", PROG_FILES_LEN, size);

    /* close the INF again */
    hr = pCloseINFEngine(hinf);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);

    DeleteFileA("c:\\test.inf");
}

START_TEST(advpack)
{
    if (!init_function_pointers())
        return;

    get_progfiles_dir();

    version_test();
    delnode_test();
    translateinfstring_test();
    translateinfstringex_test();
}
