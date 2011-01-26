/*
 * Unit tests for IShellDispatch
 *
 * Copyright 2010 Alexander Morozov for Etersoft
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "shldisp.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "wine/test.h"

static HRESULT (WINAPI *pSHGetFolderPathW)(HWND, int, HANDLE, DWORD, LPWSTR);
static HRESULT (WINAPI *pSHGetNameFromIDList)(PCIDLIST_ABSOLUTE,SIGDN,PWSTR*);
static HRESULT (WINAPI *pSHGetSpecialFolderLocation)(HWND, int, LPITEMIDLIST *);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHGetFolderPathW = (void*)GetProcAddress(hmod, "SHGetFolderPathW");
    pSHGetNameFromIDList = (void*)GetProcAddress(hmod, "SHGetNameFromIDList");
    pSHGetSpecialFolderLocation = (void*)GetProcAddress(hmod,
     "SHGetSpecialFolderLocation");
}

static void test_namespace(void)
{
    static const WCHAR winetestW[] = {'w','i','n','e','t','e','s','t',0};
    static const WCHAR backslashW[] = {'\\',0};

    static WCHAR tempW[MAX_PATH], curW[MAX_PATH];
    HRESULT r;
    IShellDispatch *sd;
    Folder *folder;
    VARIANT var;
    BSTR title;
    int len;

    r = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
     &IID_IShellDispatch, (LPVOID*)&sd);
    if (r == REGDB_E_CLASSNOTREG) /* NT4 */
    {
        win_skip("skipping IShellDispatch tests\n");
        return;
    }
    ok(SUCCEEDED(r), "CoCreateInstance failed: %08x\n", r);
    if (FAILED(r))
        return;

    VariantInit(&var);
    folder = (void*)0xdeadbeef;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    todo_wine
    ok(r == S_FALSE, "expected S_FALSE, got %08x\n", r);
    ok(folder == NULL, "expected NULL, got %p\n", folder);

    V_VT(&var) = VT_I4;
    V_I4(&var) = -1;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    todo_wine
    ok(r == S_FALSE, "expected S_FALSE, got %08x\n", r);

    V_VT(&var) = VT_I4;
    V_I4(&var) = ssfPROGRAMFILES;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    todo_wine
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08x\n", r);
    if (r == S_OK)
    {
        r = Folder_get_Title(folder, &title);
        todo_wine
        ok(r == S_OK, "Folder::get_Title failed: %08x\n", r);
        if (r == S_OK)
        {
            /* On Win2000-2003 title is equal to program files directory name in
               HKLM\Software\Microsoft\Windows\CurrentVersion\ProgramFilesDir.
               On newer Windows it seems constant and is not changed
               if the program files directory name is changed */
            if (pSHGetSpecialFolderLocation && pSHGetNameFromIDList)
            {
                LPITEMIDLIST pidl;
                PWSTR name;

                r = pSHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl);
                ok(r == S_OK, "SHGetSpecialFolderLocation failed: %08x\n", r);
                r = pSHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &name);
                ok(r == S_OK, "SHGetNameFromIDList failed: %08x\n", r);
                todo_wine
                ok(!lstrcmpW(title, name), "expected %s, got %s\n",
                 wine_dbgstr_w(name), wine_dbgstr_w(title));
                CoTaskMemFree(name);
                CoTaskMemFree(pidl);
            }
            else if (pSHGetFolderPathW)
            {
                static WCHAR path[MAX_PATH];
                WCHAR *p;

                r = pSHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL,
                 SHGFP_TYPE_CURRENT, path);
                ok(r == S_OK, "SHGetFolderPath failed: %08x\n", r);
                p = path + lstrlenW(path);
                while (path < p && *(p - 1) != '\\')
                    p--;
                ok(!lstrcmpW(title, p), "expected %s, got %s\n",
                 wine_dbgstr_w(p), wine_dbgstr_w(title));
            }
            else skip("skipping Folder::get_Title test\n");
            SysFreeString(title);
        }
        Folder_Release(folder);
    }

    GetTempPathW(MAX_PATH, tempW);
    GetCurrentDirectoryW(MAX_PATH, curW);
    SetCurrentDirectoryW(tempW);
    CreateDirectoryW(winetestW, NULL);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(winetestW);
    r = IShellDispatch_NameSpace(sd, var, &folder);
    todo_wine
    ok(r == S_FALSE, "expected S_FALSE, got %08x\n", r);
    SysFreeString(V_BSTR(&var));

    GetFullPathNameW(winetestW, MAX_PATH, tempW, NULL);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(tempW);
    r = IShellDispatch_NameSpace(sd, var, &folder);
    todo_wine
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08x\n", r);
    if (r == S_OK)
    {
        r = Folder_get_Title(folder, &title);
        todo_wine
        ok(r == S_OK, "Folder::get_Title failed: %08x\n", r);
        if (r == S_OK)
        {
            todo_wine
            ok(!lstrcmpW(title, winetestW), "bad title: %s\n",
             wine_dbgstr_w(title));
            SysFreeString(title);
        }
        Folder_Release(folder);
    }
    SysFreeString(V_BSTR(&var));

    len = lstrlenW(tempW);
    if (len < MAX_PATH - 1)
    {
        lstrcatW(tempW, backslashW);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(tempW);
        r = IShellDispatch_NameSpace(sd, var, &folder);
        todo_wine
        ok(r == S_OK, "IShellDispatch::NameSpace failed: %08x\n", r);
        if (r == S_OK)
        {
            r = Folder_get_Title(folder, &title);
            todo_wine
            ok(r == S_OK, "Folder::get_Title failed: %08x\n", r);
            if (r == S_OK)
            {
                todo_wine
                ok(!lstrcmpW(title, winetestW), "bad title: %s\n",
                 wine_dbgstr_w(title));
                SysFreeString(title);
            }
            Folder_Release(folder);
        }
        SysFreeString(V_BSTR(&var));
    }

    RemoveDirectoryW(winetestW);
    SetCurrentDirectoryW(curW);
    IShellDispatch_Release(sd);
}

START_TEST(shelldispatch)
{
    HRESULT r;

    r = CoInitialize(NULL);
    ok(SUCCEEDED(r), "CoInitialize failed: %08x\n", r);
    if (FAILED(r))
        return;

    init_function_pointers();
    test_namespace();

    CoUninitialize();
}
