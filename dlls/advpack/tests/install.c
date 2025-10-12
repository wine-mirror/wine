/*
 * Unit tests for advpack.dll install functions
 *
 * Copyright (C) 2006 James Hawkins
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

#include <stdio.h>
#include <windows.h>
#include <advpub.h>
#include "wine/test.h"

static HMODULE hAdvPack;
/* function pointers */
static HRESULT (WINAPI *pRunSetupCommand)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, HANDLE*, DWORD, LPVOID);
static INT (WINAPI *pLaunchINFSection)(HWND, HINSTANCE, LPSTR, INT);
static HRESULT (WINAPI *pLaunchINFSectionEx)(HWND, HINSTANCE, LPSTR, INT);

static char CURR_DIR[MAX_PATH];

static BOOL init_function_pointers(void)
{
    hAdvPack = LoadLibraryA("advpack.dll");
    if (!hAdvPack)
        return FALSE;

    pRunSetupCommand = (void *)GetProcAddress(hAdvPack, "RunSetupCommand");
    pLaunchINFSection = (void *)GetProcAddress(hAdvPack, "LaunchINFSection");
    pLaunchINFSectionEx = (void *)GetProcAddress(hAdvPack, "LaunchINFSectionEx");

    if (!pRunSetupCommand || !pLaunchINFSection || !pLaunchINFSectionEx)
        return FALSE;

    return TRUE;
}

static BOOL is_spapi_err(DWORD err)
{
    const DWORD SPAPI_PREFIX = 0x800F0000L;
    const DWORD SPAPI_MASK = 0xFFFF0000L;

    return (((err & SPAPI_MASK) ^ SPAPI_PREFIX) == 0);
}

static void load_resource(const char *name, const char *filename)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", filename, GetLastError());

    res = FindResourceA(NULL, name, "TESTDLL");
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "couldn't write resource\n");
    CloseHandle(file);
}

static void create_inf_file(LPCSTR filename)
{
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    static const char data[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "AdvancedINF=2.5\n"
        "[DefaultInstall]\n"
        "CheckAdminRights=1\n"

        "[OcxInstallGood]\n"
        "RegisterOCXs=GoodOCX\n"
        "[OcxUninstallGood]\n"
        "UnregisterOCXs=GoodOCX\n"
        "[GoodOCX]\n"
        "winetest_selfreg.ocx\n"

        "[OcxInstallAtGood]\n"
        "RegisterOCXs=AtGoodOCX\n"
        "[AtGoodOCX]\n"
        "@winetest_selfreg.ocx\n"

        "[OcxInstallBad]\n"
        "RegisterOCXs=BadOCXsToRegister\n"
        "[BadOCXsToRegister]\n"
        "nonexistent.ocx\n"

        "[OcxInstallAtBad]\n"
        "RegisterOCXs=AtBadOCX\n"
        "[AtBadOCX]\n"
        "@nonexistent.ocx\n";

    WriteFile(hf, data, sizeof(data) - 1, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void test_RunSetupCommand(void)
{
    HRESULT hr;
    HANDLE hexe;
    char path[MAX_PATH];
    char dir[MAX_PATH];
    char systemdir[MAX_PATH];

    GetSystemDirectoryA(systemdir, sizeof(systemdir));

    /* try an invalid cmd name */
    hr = pRunSetupCommand(NULL, NULL, "Install", "Dir", "Title", NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try an invalid directory */
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", NULL, "Title", NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try to run a nonexistent exe */
    hexe = (HANDLE)0xdeadbeef;
    hr = pRunSetupCommand(NULL, "idontexist.exe", "Install", systemdir, "Title", &hexe, 0, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);
    ok(hexe == NULL, "Expected hexe to be NULL\n");
    ok(!TerminateProcess(hexe, 0), "Expected TerminateProcess to fail\n");

    /* try a bad directory */
    hexe = (HANDLE)0xdeadbeef;
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", "non\\existent\\directory", "Title", &hexe, 0, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_DIRECTORY),
       "Expected HRESULT_FROM_WIN32(ERROR_DIRECTORY), got %ld\n", hr);
    ok(hexe == NULL, "Expected hexe to be NULL\n");
    ok(!TerminateProcess(hexe, 0), "Expected TerminateProcess to fail\n");

    /* try to run an exe with the RSC_FLAG_INF flag */
    hexe = (HANDLE)0xdeadbeef;
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", systemdir, "Title", &hexe, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(is_spapi_err(hr), "Expected a setupapi error, got %ld\n", hr);
    ok(hexe == (HANDLE)0xdeadbeef, "Expected hexe to be 0xdeadbeef\n");
    ok(!TerminateProcess(hexe, 0), "Expected TerminateProcess to fail\n");

    /* run winver.exe */
    hexe = (HANDLE)0xdeadbeef;
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", systemdir, "Title", &hexe, 0, NULL);
    ok(hr == S_ASYNCHRONOUS, "Expected S_ASYNCHRONOUS, got %ld\n", hr);
    ok(hexe != NULL, "Expected hexe to be non-NULL\n");
    ok(TerminateProcess(hexe, 0), "Expected TerminateProcess to succeed\n");

    CreateDirectoryA("one", NULL);
    create_inf_file("one\\test.inf");

    /* try a full path to the INF, with working dir provided */
    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\one\\test.inf");
    lstrcpyA(dir, CURR_DIR);
    lstrcatA(dir, "\\one");
    hr = pRunSetupCommand(NULL, path, "DefaultInstall", dir, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", hr);

    /* try a full path to the INF, NULL working dir */
    hr = pRunSetupCommand(NULL, path, "DefaultInstall", NULL, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
       "Expected HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got %ld\n", hr);

    /* try a full path to the INF, empty working dir */
    hr = pRunSetupCommand(NULL, path, "DefaultInstall", "", "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", hr);

    /* try a relative path to the INF, with working dir provided */
    hr = pRunSetupCommand(NULL, "one\\test.inf", "DefaultInstall", dir, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", hr);

    /* try a relative path to the INF, NULL working dir */
    hr = pRunSetupCommand(NULL, "one\\test.inf", "DefaultInstall", NULL, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
       "Expected HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got %ld\n", hr);

    /* try a relative path to the INF, empty working dir */
    hr = pRunSetupCommand(NULL, "one\\test.inf", "DefaultInstall", "", "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", hr);

    /* try only the INF filename, with working dir provided */
    hr = pRunSetupCommand(NULL, "test.inf", "DefaultInstall", dir, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);

    /* try only the INF filename, NULL working dir */
    hr = pRunSetupCommand(NULL, "test.inf", "DefaultInstall", NULL, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
       "Expected HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got %ld\n", hr);

    /* try only the INF filename, empty working dir */
    hr = pRunSetupCommand(NULL, "test.inf", "DefaultInstall", "", "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);

    DeleteFileA("one\\test.inf");
    RemoveDirectoryA("one");

    create_inf_file("test.inf");

    /* try INF file in the current directory, working directory provided */
    hr = pRunSetupCommand(NULL, "test.inf", "DefaultInstall", CURR_DIR, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);

    /* try INF file in the current directory, NULL working directory */
    hr = pRunSetupCommand(NULL, "test.inf", "DefaultInstall", NULL, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
       "Expected HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got %ld\n", hr);

    /* try INF file in the current directory, empty working directory */
    hr = pRunSetupCommand(NULL, "test.inf", "DefaultInstall", CURR_DIR, "Title", NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);
}

static void test_LaunchINFSection(void)
{
    HRESULT hr;
    char cmdline[MAX_PATH];
    static char file[] = "test.inf,DefaultInstall,4,0";
    static char file2[] = "test.inf,,1,0";

    /* The 'No UI' flag seems to have no effect whatsoever on Windows.
     * So only do this test in interactive mode.
     */
    if (winetest_interactive)
    {
        /* try an invalid cmdline */
        hr = pLaunchINFSection(NULL, NULL, NULL, 0);
        ok(hr == 1, "Expected 1, got %ld\n", hr);
    }

    CreateDirectoryA("one", NULL);
    create_inf_file("one\\test.inf");

    /* try a full path to the INF */
    lstrcpyA(cmdline, CURR_DIR);
    lstrcatA(cmdline, "\\");
    lstrcatA(cmdline, "one\\test.inf,DefaultInstall,,4");
    hr = pLaunchINFSection(NULL, NULL, cmdline, 0);
    ok(hr == 0, "Expected 0, got %ld\n", hr);

    DeleteFileA("one\\test.inf");
    RemoveDirectoryA("one");

    create_inf_file("test.inf");

    /* try just the INF filename */
    hr = pLaunchINFSection(NULL, NULL, file, 0);
    ok(hr == 0, "Expected 0, got %ld\n", hr);

    hr = pLaunchINFSection(NULL, NULL, file2, 0);
    ok(hr == 0, "Expected 0, got %ld\n", hr);

    DeleteFileA("test.inf");
}

static void test_LaunchINFSectionEx(void)
{
    HRESULT hr;
    char cmdline[MAX_PATH];

    create_inf_file("test.inf");

    /* try an invalid CAB filename with an absolute INF name */
    lstrcpyA(cmdline, CURR_DIR);
    lstrcatA(cmdline, "\\");
    lstrcatA(cmdline, "test.inf,DefaultInstall,c:imacab.cab,4");
    hr = pLaunchINFSectionEx(NULL, NULL, cmdline, 0);
    ok(hr == 0, "Expected 0, got %ld\n", hr);

    /* try quoting the parameters */
    lstrcpyA(cmdline, "\"");
    lstrcatA(cmdline, CURR_DIR);
    lstrcatA(cmdline, "\\test.inf\",\"DefaultInstall\",\"c:,imacab.cab\",\"4\"");
    hr = pLaunchINFSectionEx(NULL, NULL, cmdline, 0);
    ok(hr == 0, "Expected 0, got %ld\n", hr);

    /* The 'No UI' flag seems to have no effect whatsoever on Windows.
     * So only do this test in interactive mode.
     */
    if (winetest_interactive)
    {
        /* try an invalid CAB filename with a relative INF name */
        lstrcpyA(cmdline, "test.inf,DefaultInstall,c:imacab.cab,4");
        hr = pLaunchINFSectionEx(NULL, NULL, cmdline, 0);
        ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);
    }

    DeleteFileA("test.inf");
}

static LRESULT CALLBACK hide_window_hook(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HCBT_CREATEWND)
    {
        /* Suppress the "Error registering the OCX" dialog */
        return 1;
    }

    return CallNextHookEx(NULL, code, wparam, lparam);
}

static void test_RegisterOCXs(void)
{
    static char install_good_section[] = "test.inf,OcxInstallGood,4,0";
    static char uninstall_good_section[] = "test.inf,OcxUninstallGood,4,0";
    static char install_at_good_section[] = "test.inf,OcxInstallAtGood,4,0";
    static char install_bad_section[] = "test.inf,OcxInstallBad,4,0";
    static char install_at_bad_section[] = "test.inf,OcxInstallAtBad,4,0";
    HHOOK hook;
    HKEY key;
    INT res;

    load_resource("selfreg.dll", "winetest_selfreg.ocx");
    create_inf_file("test.inf");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "selfreg_test");
    res = pLaunchINFSection(NULL, NULL, install_good_section, 0);
    ok(res == 0, "Expected 0, got %d\n", res);
    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    ok(res == 0, "Expected 0, got %d\n", res);
    RegCloseKey(key);

    res = pLaunchINFSection(NULL, NULL, uninstall_good_section, 0);
    ok(res == 0, "Expected 0, got %d\n", res);
    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    todo_wine ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "selfreg_test");

    res = pLaunchINFSection(NULL, NULL, install_at_good_section, 0);
    ok(res == 0, "Expected 0, got %d\n", res);
    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    hook = SetWindowsHookExW(WH_CBT, hide_window_hook, NULL, GetCurrentThreadId());
    res = pLaunchINFSection(NULL, NULL, install_bad_section, 0);
    ok(res == 1, "Expected 1, got %d\n", res);
    UnhookWindowsHookEx(hook);

    res = pLaunchINFSection(NULL, NULL, install_at_bad_section, 0);
    ok(res == 0, "Expected 0, got %d\n", res);

    DeleteFileA("winetest_selfreg.ocx");
    DeleteFileA("test.inf");
}

START_TEST(install)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];

    if (!init_function_pointers())
        return;

    if (!IsNTAdmin(0, NULL))
    {
        skip("Most tests need admin rights\n");
        return;
    }

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    test_RunSetupCommand();
    test_LaunchINFSection();
    test_LaunchINFSectionEx();
    test_RegisterOCXs();

    FreeLibrary(hAdvPack);
    SetCurrentDirectoryA(prev_path);
}
