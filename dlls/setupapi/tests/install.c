/*
 * Unit test for setupapi.dll install functions
 *
 * Copyright 2007 Misha Koshelev
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
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

static const char inffile[] = "test.inf";
static char CURR_DIR[MAX_PATH];

/* Notes on InstallHinfSectionA/W:
 * - InstallHinfSectionW on Win98 and InstallHinfSectionA on WinXP seem to be stubs - they do not do anything
 *   and simply return without displaying any error message or setting last error. We test whether
 *   InstallHinfSectionA sets last error, and if it doesn't we set it to NULL and use the W version if available.
 * - These functions do not return a value and do not always set last error to ERROR_SUCCESS when installation still
 *   occurs (e.g., unquoted inf file with spaces, registry keys are written but last error is 6). Also, on Win98 last error
 *   is set to ERROR_SUCCESS even if install fails (e.g., quoted inf file with spaces, no registry keys set, MessageBox with
 *   "Installation Error" displayed). Thus, we must use functional tests (e.g., is registry key created) to determine whether
 *   or not installation occured.
 * - On installation problems, a MessageBox() is displayed and a Beep() is issued. The MessageBox() is disabled with a
 *   CBT hook.
 */

static void (WINAPI *pInstallHinfSectionA)(HWND, HINSTANCE, LPCSTR, INT);
static void (WINAPI *pInstallHinfSectionW)(HWND, HINSTANCE, LPCWSTR, INT);

/*
 * Helpers
 */

static void create_inf_file(LPCSTR filename, const char *data)
{
    DWORD res;
    HANDLE handle = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(handle != INVALID_HANDLE_VALUE);
    assert(WriteFile(handle, data, strlen(data), &res, NULL));
    CloseHandle(handle);
}

/* CBT hook to ensure a window (e.g., MessageBox) cannot be created */
static HHOOK hhook;
static LRESULT CALLBACK cbt_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    return nCode == HCBT_CREATEWND ? 1: CallNextHookEx(hhook, nCode, wParam, lParam);
}

/*
 * Tests
 */

static const char *cmdline_inf = "[Version]\n"
    "Signature=\"$Chicago$\"\n"
    "[DefaultInstall]\n"
    "AddReg=Add.Settings\n"
    "[Add.Settings]\n"
    "HKCU,Software\\Wine\\setupapitest,,\n";

static void ok_cmdline(LPCSTR section, int mode, LPCSTR path, BOOL expectsuccess)
{
    CHAR cmdline[MAX_PATH * 2];
    LONG ret;

    sprintf(cmdline, "%s %d %s", section, mode, path);
    if (pInstallHinfSectionA) pInstallHinfSectionA(NULL, NULL, cmdline, 0);
    else
    {
        WCHAR cmdlinew[MAX_PATH * 2];
        MultiByteToWideChar(CP_ACP, 0, cmdline, -1, cmdlinew, MAX_PATH*2);
        pInstallHinfSectionW(NULL, NULL, cmdlinew, 0);
    }

    /* Functional tests for success of install and clean up */
    ret = RegDeleteKey(HKEY_CURRENT_USER, "Software\\Wine\\setupapitest");
    ok((expectsuccess && ret == ERROR_SUCCESS) ||
       (!expectsuccess && ret == ERROR_FILE_NOT_FOUND),
       "Expected registry key Software\\Wine\\setupapitest to %s, RegDeleteKey returned %d\n",
       expectsuccess ? "exist" : "not exist",
       ret);
}

/* Test command line processing */
static void test_cmdline(void)
{
    static const char infwithspaces[] = "test file.inf";
    char path[MAX_PATH];

    create_inf_file(inffile, cmdline_inf);
    sprintf(path, "%s\\%s", CURR_DIR, inffile);
    ok_cmdline("DefaultInstall", 128, path, TRUE);
    ok(DeleteFile(inffile), "Expected source inf to exist, last error was %d\n", GetLastError());

    /* Test handling of spaces in path, unquoted and quoted */
    create_inf_file(infwithspaces, cmdline_inf);

    sprintf(path, "%s\\%s", CURR_DIR, infwithspaces);
    ok_cmdline("DefaultInstall", 128, path, TRUE);

    sprintf(path, "\"%s\\%s\"", CURR_DIR, infwithspaces);
    ok_cmdline("DefaultInstall", 128, path, FALSE);

    ok(DeleteFile(infwithspaces), "Expected source inf to exist, last error was %d\n", GetLastError());
}

START_TEST(install)
{
    HMODULE hsetupapi = GetModuleHandle("setupapi.dll");
    char temp_path[MAX_PATH], prev_path[MAX_PATH];
    DWORD len;

    GetCurrentDirectory(MAX_PATH, prev_path);
    GetTempPath(MAX_PATH, temp_path);
    SetCurrentDirectory(temp_path);

    strcpy(CURR_DIR, temp_path);
    len = strlen(CURR_DIR);
    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    pInstallHinfSectionA = (void *)GetProcAddress(hsetupapi, "InstallHinfSectionA");
    pInstallHinfSectionW = (void *)GetProcAddress(hsetupapi, "InstallHinfSectionW");
    if (pInstallHinfSectionA)
    {
        /* Check if pInstallHinfSectionA sets last error or is a stub (as on WinXP) */
        static const char *minimal_inf = "[Version]\nSignature=\"$Chicago$\"\n";
        char cmdline[MAX_PATH*2];
        create_inf_file(inffile, minimal_inf);
        sprintf(cmdline, "DefaultInstall 128 %s\\%s", CURR_DIR, inffile);
        SetLastError(0xdeadbeef);
        pInstallHinfSectionA(NULL, NULL, cmdline, 0);
        if (GetLastError() == 0xdeadbeef)
        {
            skip("InstallHinfSectionA is broken (stub)\n");
            pInstallHinfSectionA = NULL;
        }
        ok(DeleteFile(inffile), "Expected source inf to exist, last error was %d\n", GetLastError());
    }
    if (!pInstallHinfSectionW && !pInstallHinfSectionA)
        skip("InstallHinfSectionA and InstallHinfSectionW are not available\n");
    else
    {
        /* Set CBT hook to disallow MessageBox creation in current thread */
        hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
        assert(hhook != 0);

        test_cmdline();

        UnhookWindowsHookEx(hhook);
    }

    SetCurrentDirectory(prev_path);
}
