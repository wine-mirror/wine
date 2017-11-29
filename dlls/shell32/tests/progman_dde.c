/*
 * Unit test of the Program Manager DDE Interfaces
 *
 * Copyright 2009 Mikey Alexander
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

/* DDE Program Manager Tests
 * - Covers basic CreateGroup, ShowGroup, DeleteGroup, AddItem, and DeleteItem
 *   functionality
 * - Todo: Handle CommonGroupFlag
 *         Better AddItem Tests (Lots of parameters to test)
 *         Tests for Invalid Characters in Names / Invalid Parameters
 */

#include <stdio.h>
#include <wine/test.h>
#include <winbase.h>
#include "dde.h"
#include "ddeml.h"
#include "winuser.h"
#include "shlobj.h"

static HRESULT (WINAPI *pSHGetLocalizedName)(LPCWSTR, LPWSTR, UINT, int *);
static BOOL (WINAPI *pSHGetSpecialFolderPathA)(HWND, LPSTR, int, BOOL);
static BOOL (WINAPI *pReadCabinetState)(CABINETSTATE *, int);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHGetLocalizedName = (void*)GetProcAddress(hmod, "SHGetLocalizedName");
    pSHGetSpecialFolderPathA = (void*)GetProcAddress(hmod, "SHGetSpecialFolderPathA");
    pReadCabinetState = (void*)GetProcAddress(hmod, "ReadCabinetState");
    if (!pReadCabinetState)
        pReadCabinetState = (void*)GetProcAddress(hmod, (LPSTR)651);
}

static BOOL use_common(void)
{
    HMODULE hmod;
    static BOOL (WINAPI *pIsNTAdmin)(DWORD, LPDWORD);

    /* IsNTAdmin() is available on all platforms. */
    hmod = LoadLibraryA("advpack.dll");
    pIsNTAdmin = (void*)GetProcAddress(hmod, "IsNTAdmin");

    if (!pIsNTAdmin(0, NULL))
    {
        /* We are definitely not an administrator */
        FreeLibrary(hmod);
        return FALSE;
    }
    FreeLibrary(hmod);

    /* If we end up here we are on NT4+ as Win9x and WinMe don't have the
     * notion of administrators (as we need it).
     */

    /* As of Vista  we should always use the users directory. Tests with the
     * real Administrator account on Windows 7 proved this.
     *
     * FIXME: We need a better way of identifying Vista+ as currently this check
     * also covers Wine and we don't know yet which behavior we want to follow.
     */
    if (pSHGetLocalizedName)
        return FALSE;

    return TRUE;
}

static BOOL full_title(void)
{
    CABINETSTATE cs;

    memset(&cs, 0, sizeof(cs));
    if (pReadCabinetState)
    {
        pReadCabinetState(&cs, sizeof(cs));
    }
    else
    {
        HKEY key;
        DWORD size;

        win_skip("ReadCabinetState is not available, reading registry directly\n");
        RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState", &key);
        size = sizeof(cs);
        RegQueryValueExA(key, "Settings", NULL, NULL, (LPBYTE)&cs, &size);
        RegCloseKey(key);
    }

    return (cs.fFullPathTitle == -1);
}

static char ProgramsDir[MAX_PATH];

static char Group1Title[MAX_PATH]  = "Group1";
static char Group2Title[MAX_PATH]  = "Group2";
static char Group3Title[MAX_PATH]  = "Group3";
static char StartupTitle[MAX_PATH] = "Startup";

static void init_strings(void)
{
    char startup[MAX_PATH];
    char commonprograms[MAX_PATH];
    char programs[MAX_PATH];

    if (pSHGetSpecialFolderPathA)
    {
        pSHGetSpecialFolderPathA(NULL, programs, CSIDL_PROGRAMS, FALSE);
        pSHGetSpecialFolderPathA(NULL, commonprograms, CSIDL_COMMON_PROGRAMS, FALSE);
        pSHGetSpecialFolderPathA(NULL, startup, CSIDL_STARTUP, FALSE);
    }
    else
    {
        HKEY key;
        DWORD size;

        /* Older Win9x and NT4 */

        RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", &key);
        size = sizeof(programs);
        RegQueryValueExA(key, "Programs", NULL, NULL, (LPBYTE)&programs, &size);
        size = sizeof(startup);
        RegQueryValueExA(key, "Startup", NULL, NULL, (LPBYTE)&startup, &size);
        RegCloseKey(key);

        RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", &key);
        size = sizeof(commonprograms);
        RegQueryValueExA(key, "Common Programs", NULL, NULL, (LPBYTE)&commonprograms, &size);
        RegCloseKey(key);
    }

    /* ProgramsDir on Vista+ is always the users one (CSIDL_PROGRAMS). Before Vista
     * it depends on whether the user is an administrator (CSIDL_COMMON_PROGRAMS) or
     * not (CSIDL_PROGRAMS).
     */
    if (use_common())
        lstrcpyA(ProgramsDir, commonprograms);
    else
        lstrcpyA(ProgramsDir, programs);

    if (full_title())
    {
        lstrcpyA(Group1Title, ProgramsDir);
        lstrcatA(Group1Title, "\\Group1");
        lstrcpyA(Group2Title, ProgramsDir);
        lstrcatA(Group2Title, "\\Group2");
        lstrcpyA(Group3Title, ProgramsDir);
        lstrcatA(Group3Title, "\\Group3");

        lstrcpyA(StartupTitle, startup);
    }
    else
    {
        /* Vista has the nice habit of displaying the full path in English
         * and the short one localized. CSIDL_STARTUP on Vista gives us the
         * English version so we have to 'translate' this one.
         *
         * MSDN claims it should be used for files not folders but this one
         * suits our purposes just fine.
         */
        if (pSHGetLocalizedName)
        {
            WCHAR startupW[MAX_PATH];
            WCHAR module[MAX_PATH];
            WCHAR module_expanded[MAX_PATH];
            WCHAR localized[MAX_PATH];
            HRESULT hr;
            int id;

            MultiByteToWideChar(CP_ACP, 0, startup, -1, startupW, sizeof(startupW)/sizeof(WCHAR));
            hr = pSHGetLocalizedName(startupW, module, MAX_PATH, &id);
            todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
            /* check to be removed when SHGetLocalizedName is implemented */
            if (hr == S_OK)
            {
                ExpandEnvironmentStringsW(module, module_expanded, MAX_PATH);
                LoadStringW(GetModuleHandleW(module_expanded), id, localized, MAX_PATH);

                WideCharToMultiByte(CP_ACP, 0, localized, -1, StartupTitle, sizeof(StartupTitle), NULL, NULL);
            }
            else
                lstrcpyA(StartupTitle, (strrchr(startup, '\\') + 1));
        }
        else
        {
            lstrcpyA(StartupTitle, (strrchr(startup, '\\') + 1));
        }
    }
}

static HDDEDATA CALLBACK DdeCallback(UINT type, UINT format, HCONV hConv, HSZ hsz1, HSZ hsz2,
                                     HDDEDATA hDDEData, ULONG_PTR data1, ULONG_PTR data2)
{
    trace("Callback: type=%i, format=%i\n", type, format);
    return NULL;
}

static UINT dde_execute(DWORD instance, HCONV hconv, const char *command_str)
{
    HDDEDATA command, hdata;
    DWORD result;
    UINT ret;

    command = DdeCreateDataHandle(instance, (BYTE *)command_str, strlen(command_str)+1, 0, 0, 0, 0);
    ok(command != NULL, "DdeCreateDataHandle() failed: %u\n", DdeGetLastError(instance));

    hdata = DdeClientTransaction((BYTE *)command, -1, hconv, 0, 0, XTYP_EXECUTE, 2000, &result);
    ret = DdeGetLastError(instance);
    /* PROGMAN always returns 1 on success */
    ok((UINT_PTR)hdata == !ret, "expected %u, got %p\n", !ret, hdata);

    return ret;
}

static BOOL check_window_exists(const char *name)
{
    HWND window = NULL;
    int i;

    for (i = 0; i < 20; i++)
    {
        Sleep(100);
        if ((window = FindWindowA("ExplorerWClass", name)) ||
            (window = FindWindowA("CabinetWClass", name)))
        {
            SendMessageA(window, WM_SYSCOMMAND, SC_CLOSE, 0);
            break;
        }
    }

    return (window != NULL);
}

static BOOL check_exists(const char *name)
{
    char path[MAX_PATH];

    strcpy(path, ProgramsDir);
    strcat(path, "\\");
    strcat(path, name);
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

/* 1st set of tests */
static void test_progman_dde(DWORD instance, HCONV hConv)
{
    UINT error;
    char temppath[MAX_PATH];
    char f1g1[MAX_PATH], f2g1[MAX_PATH], f3g1[MAX_PATH], f1g3[MAX_PATH], f2g3[MAX_PATH];
    char itemtext[MAX_PATH + 20];
    char comptext[2 * (MAX_PATH + 20) + 21];

    /* Invalid Command */
    error = dde_execute(instance, hConv, "[InvalidCommand()]");
    ok(error == DMLERR_NOTPROCESSED, "expected DMLERR_NOTPROCESSED, got %u\n", error);

    /* On Vista+ the files have to exist when adding a link */
    GetTempPathA(MAX_PATH, temppath);
    GetTempFileNameA(temppath, "dde", 0, f1g1);
    GetTempFileNameA(temppath, "dde", 0, f2g1);
    GetTempFileNameA(temppath, "dde", 0, f3g1);
    GetTempFileNameA(temppath, "dde", 0, f1g3);
    GetTempFileNameA(temppath, "dde", 0, f2g3);

    /* CreateGroup Tests (including AddItem, DeleteItem) */
    error = dde_execute(instance, hConv, "[CreateGroup(Group1)]");
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group1"), "directory not created\n");
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(Group1Title), "window not created\n");
    }

    sprintf(itemtext, "[AddItem(%s,f1g1Name)]", f1g1);
    error = dde_execute(instance, hConv, itemtext);
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group1/f1g1Name.lnk"), "link not created\n");
    }

    sprintf(itemtext, "[AddItem(%s,f2g1Name)]", f2g1);
    error = dde_execute(instance, hConv, itemtext);
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group1/f2g1Name.lnk"), "link not created\n");
    }

    error = dde_execute(instance, hConv, "[DeleteItem(f2g1Name)]");
    todo_wine
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(!check_exists("Group1/f2g1Name.lnk"), "link should not exist\n");

    sprintf(itemtext, "[AddItem(%s,f3g1Name)]", f3g1);
    error = dde_execute(instance, hConv, itemtext);
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group1/f3g1Name.lnk"), "link not created\n");
    }

    error = dde_execute(instance, hConv, "[CreateGroup(Group2)]");
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group2"), "directory not created\n");
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(Group2Title), "window not created\n");
    }

    /* Create Group that already exists - same instance */
    error = dde_execute(instance, hConv, "[CreateGroup(Group1)]");
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group1"), "directory not created\n");
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(Group1Title), "window not created\n");
    }

    /* ShowGroup Tests */
    error = dde_execute(instance, hConv, "[ShowGroup(Group1)]");
    ok(error == DMLERR_NOTPROCESSED, "expected DMLERR_NOTPROCESSED, got %u\n", error);

    error = dde_execute(instance, hConv, "[DeleteItem(f3g1Name)]");
    todo_wine
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(!check_exists("Group1/f3g1Name.lnk"), "link should not exist\n");

    error = dde_execute(instance, hConv, "[ShowGroup(Startup, 0)]");
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(StartupTitle), "window not created\n");
    }

    error = dde_execute(instance, hConv, "[ShowGroup(Group1, 0)]");
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(Group1Title), "window not created\n");
    }

    /* DeleteGroup Test */
    error = dde_execute(instance, hConv, "[DeleteGroup(Group1)]");
    todo_wine
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(!check_exists("Group1"), "directory should not exist\n");

    /* Compound Execute String Command */
    sprintf(comptext, "[CreateGroup(Group3)][AddItem(%s,f1g3Name)][AddItem(%s,f2g3Name)]", f1g3, f2g3);
    error = dde_execute(instance, hConv, comptext);
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group3"), "directory not created\n");
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(Group3Title), "window not created\n");
    ok(check_exists("Group3/f1g3Name.lnk"), "link not created\n");
    ok(check_exists("Group3/f2g3Name.lnk"), "link not created\n");
    }

    error = dde_execute(instance, hConv, "[DeleteGroup(Group3)]");
    todo_wine
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(!check_exists("Group3"), "directory should not exist\n");

    /* Full Parameters of Add Item */
    /* AddItem(CmdLine[,Name[,IconPath[,IconIndex[,xPos,yPos[,DefDir[,HotKey[,fMinimize[fSeparateSpace]]]]]]]) */

    DeleteFileA(f1g1);
    DeleteFileA(f2g1);
    DeleteFileA(f3g1);
    DeleteFileA(f1g3);
    DeleteFileA(f2g3);
}

/* 2nd set of tests - 2nd connection */
static void test_progman_dde2(DWORD instance, HCONV hConv)
{
    UINT error;

    /* Create Group that already exists on a separate connection */
    error = dde_execute(instance, hConv, "[CreateGroup(Group2)]");
    todo_wine {
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(check_exists("Group2"), "directory not created\n");
    if (error == DMLERR_NO_ERROR)
        ok(check_window_exists(Group2Title), "window not created\n");
    }

    error = dde_execute(instance, hConv, "[DeleteGroup(Group2)]");
    todo_wine
    ok(error == DMLERR_NO_ERROR, "expected DMLERR_NO_ERROR, got %u\n", error);
    ok(!check_exists("Group2"), "directory should not exist\n");
}

START_TEST(progman_dde)
{
    DWORD instance = 0;
    UINT err;
    HSZ hszProgman;
    HCONV hConv;

    init_function_pointers();
    init_strings();

    /* Initialize DDE Instance */
    err = DdeInitializeA(&instance, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok(err == DMLERR_NO_ERROR, "DdeInitialize() failed: %u\n", err);

    /* Create Connection */
    hszProgman = DdeCreateStringHandleA(instance, "PROGMAN", CP_WINANSI);
    ok(hszProgman != NULL, "DdeCreateStringHandle() failed: %u\n", DdeGetLastError(instance));
    hConv = DdeConnect(instance, hszProgman, hszProgman, NULL);
    ok(DdeFreeStringHandle(instance, hszProgman), "DdeFreeStringHandle() failed: %u\n", DdeGetLastError(instance));
    /* Seeing failures on early versions of Windows Connecting to progman, exit if connection fails */
    if (hConv == NULL)
    {
        ok (DdeUninitialize(instance), "DdeUninitialize failed\n");
        return;
    }

    test_progman_dde(instance, hConv);

    /* Cleanup & Exit */
    ok(DdeDisconnect(hConv), "DdeDisonnect() failed: %u\n", DdeGetLastError(instance));
    ok(DdeUninitialize(instance), "DdeUninitialize() failed: %u\n", DdeGetLastError(instance));

    /* 2nd Instance (Followup Tests) */
    /* Initialize DDE Instance */
    instance = 0;
    err = DdeInitializeA(&instance, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok (err == DMLERR_NO_ERROR, "DdeInitialize() failed: %u\n", err);

    /* Create Connection */
    hszProgman = DdeCreateStringHandleA(instance, "PROGMAN", CP_WINANSI);
    ok(hszProgman != NULL, "DdeCreateStringHandle() failed: %u\n", DdeGetLastError(instance));
    hConv = DdeConnect(instance, hszProgman, hszProgman, NULL);
    ok(hConv != NULL, "DdeConnect() failed: %u\n", DdeGetLastError(instance));
    ok(DdeFreeStringHandle(instance, hszProgman), "DdeFreeStringHandle() failed: %u\n", DdeGetLastError(instance));

    /* Run Tests */
    test_progman_dde2(instance, hConv);

    /* Cleanup & Exit */
    ok(DdeDisconnect(hConv), "DdeDisonnect() failed: %u\n", DdeGetLastError(instance));
    ok(DdeUninitialize(instance), "DdeUninitialize() failed: %u\n", DdeGetLastError(instance));
}
