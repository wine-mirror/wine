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
#include "shldisp.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "winsvc.h"

#include "wine/test.h"

#include "initguid.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08lx, expected 0x%08lx\n", hr, hr_exp)

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown *)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %ld, expected %ld\n", rc, ref);
}

static HRESULT (WINAPI *pSHGetNameFromIDList)(PCIDLIST_ABSOLUTE,SIGDN,PWSTR*);

/* Updated Windows 7 has a new IShellDispatch6 in its typelib */
DEFINE_GUID(IID_IWin7ShellDispatch6, 0x34936ba1, 0x67ad, 0x4c41, 0x99,0xb8, 0x8c,0x12,0xdf,0xf1,0xe9,0x74);

static void variant_set_string(VARIANT *v, const WCHAR *s)
{
    V_VT(v) = VT_BSTR;
    V_BSTR(v) = SysAllocString(s);
}

static void init_function_pointers(void)
{
    HMODULE hshell32;

    hshell32 = GetModuleHandleA("shell32.dll");
    pSHGetNameFromIDList = (void*)GetProcAddress(hshell32, "SHGetNameFromIDList");
}

static void test_namespace(void)
{
    static const ShellSpecialFolderConstants special_folders[] =
    {
        ssfDESKTOP,
        ssfPROGRAMS,
        ssfCONTROLS,
        ssfPRINTERS,
        ssfPERSONAL,
        ssfFAVORITES,
        ssfSTARTUP,
        ssfRECENT,
        ssfSENDTO,
        ssfBITBUCKET,
        ssfSTARTMENU,
        ssfDESKTOPDIRECTORY,
        ssfDRIVES,
        ssfNETWORK,
        ssfNETHOOD,
        ssfFONTS,
        ssfTEMPLATES,
        ssfCOMMONSTARTMENU,
        ssfCOMMONPROGRAMS,
        ssfCOMMONSTARTUP,
        ssfCOMMONDESKTOPDIR,
        ssfAPPDATA,
        ssfPRINTHOOD,
        ssfLOCALAPPDATA,
        ssfALTSTARTUP,
        ssfCOMMONALTSTARTUP,
        ssfCOMMONFAVORITES,
        ssfINTERNETCACHE,
        ssfCOOKIES,
        ssfHISTORY,
        ssfCOMMONAPPDATA,
        ssfWINDOWS,
        ssfSYSTEM,
        ssfPROGRAMFILES,
        ssfMYPICTURES,
        ssfPROFILE,
        ssfSYSTEMx86,
        ssfPROGRAMFILESx86,
    };

    static const WCHAR clsidW[] = L"::{645FF040-5081-101B-9F08-00AA002F954E}";

    static WCHAR tempW[MAX_PATH], curW[MAX_PATH];
    WCHAR *long_pathW = NULL;
    HRESULT r;
    IShellDispatch *sd;
    Folder *folder;
    Folder2 *folder2;
    FolderItem *item;
    VARIANT var;
    BSTR title, item_path;
    IDispatch *disp;
    int len, i;

    r = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, &IID_IShellDispatch, (void **)&sd);
    ok(SUCCEEDED(r), "Failed to create ShellDispatch object: %#lx.\n", r);

    disp = NULL;
    r = IShellDispatch_get_Application(sd, &disp);
    ok(r == S_OK, "Failed to get application pointer, hr %#lx.\n", r);
    ok(disp == (IDispatch *)sd, "Unexpected application pointer %p.\n", disp);
    IDispatch_Release(disp);

    disp = NULL;
    r = IShellDispatch_get_Parent(sd, &disp);
    ok(r == S_OK, "Failed to get Shell object parent, hr %#lx.\n", r);
    ok(disp == (IDispatch *)sd, "Unexpected parent pointer %p.\n", disp);
    IDispatch_Release(disp);

    VariantInit(&var);
    folder = (void*)0xdeadbeef;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    ok(folder == NULL, "expected NULL, got %p\n", folder);

    /* test valid folder ids */
    for (i = 0; i < ARRAY_SIZE(special_folders); i++)
    {
        V_VT(&var) = VT_I4;
        V_I4(&var) = special_folders[i];
        folder = (void*)0xdeadbeef;
        r = IShellDispatch_NameSpace(sd, var, &folder);
        if (special_folders[i] == ssfALTSTARTUP || special_folders[i] == ssfCOMMONALTSTARTUP)
        todo_wine
            ok(r == S_OK || broken(r == S_FALSE) /* winxp */, "Failed to get folder for index %#x, got %08lx\n", special_folders[i], r);
        else
            ok(r == S_OK, "Failed to get folder for index %#x, got %08lx\n", special_folders[i], r);
        if (folder)
            Folder_Release(folder);
    }

    V_VT(&var) = VT_I4;
    V_I4(&var) = -1;
    folder = (void *)0xdeadbeef;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_FALSE, "Unexpected hr %#lx.\n", r);
    ok(folder == NULL, "Unexpected folder instance %p\n", folder);

    V_VT(&var) = VT_I4;
    V_I4(&var) = ssfPROGRAMFILES;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08lx\n", r);
    if (r == S_OK)
    {
        static WCHAR path[MAX_PATH];

        r = SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, path);
        ok(r == S_OK, "Failed to get folder path: %#lx.\n", r);

        r = Folder_get_Title(folder, &title);
        ok(r == S_OK, "Folder::get_Title failed: %08lx\n", r);
        if (r == S_OK)
        {
            /* On Win2000-2003 title is equal to program files directory name in
               HKLM\Software\Microsoft\Windows\CurrentVersion\ProgramFilesDir.
               On newer Windows it seems constant and is not changed
               if the program files directory name is changed */
            if (pSHGetNameFromIDList)
            {
                LPITEMIDLIST pidl;
                PWSTR name;

                r = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl);
                ok(r == S_OK, "SHGetSpecialFolderLocation failed: %08lx\n", r);
                r = pSHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &name);
                ok(r == S_OK, "SHGetNameFromIDList failed: %08lx\n", r);
                ok(!lstrcmpW(title, name), "expected %s, got %s\n", wine_dbgstr_w(name), wine_dbgstr_w(title));
                CoTaskMemFree(name);
                CoTaskMemFree(pidl);
            }
            else
            {
                WCHAR *p;

                p = path + lstrlenW(path);
                while (path < p && *(p - 1) != '\\')
                    p--;
                ok(!lstrcmpiW(title, p), "expected %s, got %s\n",
                 wine_dbgstr_w(p), wine_dbgstr_w(title));
            }
            SysFreeString(title);
        }
        r = Folder_QueryInterface(folder, &IID_Folder2, (LPVOID*)&folder2);
        ok(r == S_OK, "Folder::QueryInterface failed: %08lx\n", r);
        if (r == S_OK)
        {
            r = Folder2_get_Self(folder2, &item);
            ok(r == S_OK, "Folder::get_Self failed: %08lx\n", r);
            if (r == S_OK)
            {
                r = FolderItem_get_Path(item, &item_path);
                ok(r == S_OK, "FolderItem::get_Path failed: %08lx\n", r);
                ok(!lstrcmpiW(item_path, path), "expected %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(item_path));
                SysFreeString(item_path);
                FolderItem_Release(item);
            }
            Folder2_Release(folder2);
        }
        Folder_Release(folder);
    }

    V_VT(&var) = VT_I4;
    V_I4(&var) = ssfBITBUCKET;
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08lx\n", r);

    r = Folder_QueryInterface(folder, &IID_Folder2, (void **)&folder2);
    ok(r == S_OK, "Failed to get Folder2 interface: %#lx.\n", r);
    r = Folder2_get_Self(folder2, &item);
    ok(r == S_OK, "Folder::get_Self failed: %08lx\n", r);
    r = FolderItem_get_Path(item, &item_path);
    ok(r == S_OK, "FolderItem::get_Path failed: %08lx\n", r);
    /* TODO: we return lowercase GUID here */
    ok(!lstrcmpiW(item_path, clsidW), "expected %s, got %s\n", wine_dbgstr_w(clsidW), wine_dbgstr_w(item_path));

    SysFreeString(item_path);
    FolderItem_Release(item);
    Folder2_Release(folder2);
    Folder_Release(folder);

    GetTempPathW(MAX_PATH, tempW);
    GetCurrentDirectoryW(MAX_PATH, curW);
    SetCurrentDirectoryW(tempW);
    CreateDirectoryW(L"winetest", NULL);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"winetest");
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    SysFreeString(V_BSTR(&var));

    GetFullPathNameW(L"winetest", MAX_PATH, tempW, NULL);

    len = GetLongPathNameW(tempW, NULL, 0);
    long_pathW = malloc(len * sizeof(WCHAR));
    GetLongPathNameW(tempW, long_pathW, len);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(tempW);
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08lx\n", r);

    disp = (void *)0xdeadbeef;
    r = Folder_get_Parent(folder, &disp);
    ok(r == E_NOTIMPL, "Unexpected hr %#lx.\n", r);
    ok(disp == NULL, "Unexpected parent pointer %p.\n", disp);

    r = Folder_get_Title(folder, &title);
    ok(r == S_OK, "Failed to get folder title: %#lx.\n", r);
    ok(!lstrcmpW(title, L"winetest"), "Unexpected title: %s\n",  wine_dbgstr_w(title));
    SysFreeString(title);

    r = Folder_QueryInterface(folder, &IID_Folder2, (void **)&folder2);
    ok(r == S_OK, "Failed to get Folder2 interface: %#lx.\n", r);
    r = Folder2_get_Self(folder2, &item);
    ok(r == S_OK, "Folder::get_Self failed: %08lx\n", r);
    r = FolderItem_get_Path(item, &item_path);
    ok(r == S_OK, "Failed to get item path: %#lx.\n", r);
    ok(!lstrcmpW(item_path, long_pathW), "Unexpected path %s, got %s\n", wine_dbgstr_w(item_path), wine_dbgstr_w(long_pathW));
    SysFreeString(item_path);
    FolderItem_Release(item);
    Folder2_Release(folder2);

    Folder_Release(folder);
    VariantClear(&var);

    len = lstrlenW(tempW);
    if (len < MAX_PATH - 1)
    {
        lstrcatW(tempW, L"\\");
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(tempW);
        r = IShellDispatch_NameSpace(sd, var, &folder);
        ok(r == S_OK, "IShellDispatch::NameSpace failed: %08lx\n", r);
        if (r == S_OK)
        {
            r = Folder_get_Title(folder, &title);
            ok(r == S_OK, "Folder::get_Title failed: %08lx\n", r);
            if (r == S_OK)
            {
                ok(!lstrcmpW(title, L"winetest"), "bad title: %s\n",
                 wine_dbgstr_w(title));
                SysFreeString(title);
            }
            r = Folder_QueryInterface(folder, &IID_Folder2, (LPVOID*)&folder2);
            ok(r == S_OK, "Failed to get Folder2 interface: %#lx.\n", r);
            if (r == S_OK)
            {
                r = Folder2_get_Self(folder2, &item);
                ok(r == S_OK, "Folder::get_Self failed: %08lx\n", r);
                if (r == S_OK)
                {
                    r = FolderItem_get_Path(item, &item_path);
                    ok(r == S_OK, "FolderItem::get_Path failed: %08lx\n", r);
                    ok(!lstrcmpW(item_path, long_pathW), "Unexpected path %s, got %s\n", wine_dbgstr_w(item_path),
                        wine_dbgstr_w(long_pathW));
                    SysFreeString(item_path);
                    FolderItem_Release(item);
                }
                Folder2_Release(folder2);
            }
            Folder_Release(folder);
        }
        SysFreeString(V_BSTR(&var));
    }

    free(long_pathW);
    RemoveDirectoryW(L"winetest");
    SetCurrentDirectoryW(curW);
    IShellDispatch_Release(sd);
}

static void test_items(void)
{
    static const struct
    {
        WCHAR name[32];
        enum
        {
            DIRECTORY,
            EMPTY_FILE,
        }
        type;
    }
    file_defs[] =
    {
        { L"00-Myfolder",        DIRECTORY  },
        { L"01-empty.bin",       EMPTY_FILE },
    };
    WCHAR path[MAX_PATH], cur_dir[MAX_PATH], orig_dir[MAX_PATH];
    HRESULT r;
    IShellDispatch *sd = NULL;
    Folder *folder = NULL;
    FolderItems *items;
    FolderItems2 *items2 = NULL;
    FolderItems3 *items3 = NULL;
    FolderItem *item = (FolderItem*)0xdeadbeef, *item2;
    FolderItemVerbs *verbs = (FolderItemVerbs*)0xdeadbeef;
    VARIANT var, var2, int_index, str_index, str_index2;
    IDispatch *disp, *disp2;
    LONG count = -1;
    IUnknown *unk;
    HANDLE file;
    BSTR bstr;
    WCHAR cstr[64];
    BOOL ret;
    int i;

    r = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, &IID_IShellDispatch, (void**)&sd);
    ok(SUCCEEDED(r), "CoCreateInstance failed: %08lx\n", r);
    ok(!!sd, "sd is null\n");

    /* create and enter a temporary directory and a folder object for it */
    GetTempPathW(MAX_PATH, path);
    GetCurrentDirectoryW(MAX_PATH, orig_dir);
    SetCurrentDirectoryW(path);
    ret = CreateDirectoryW(L"winetest", NULL);
    ok(ret, "CreateDirectory failed: %08lx\n", GetLastError());
    GetFullPathNameW(L"winetest", MAX_PATH, path, NULL);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(path);

    EXPECT_REF(sd, 1);
    r = IShellDispatch_NameSpace(sd, var, &folder);
    ok(r == S_OK, "IShellDispatch::NameSpace failed: %08lx\n", r);
    ok(!!folder, "folder is null\n");
    EXPECT_REF(folder, 1);
    EXPECT_REF(sd, 1);

    VariantClear(&var);
    SetCurrentDirectoryW(L"winetest");
    GetCurrentDirectoryW(MAX_PATH, path);
    GetLongPathNameW(path, cur_dir, MAX_PATH);

    /* FolderItems grabs its Folder reference */
    items = NULL;
    r = Folder_Items(folder, &items);
    ok(r == S_OK, "Folder::Items failed: %08lx\n", r);
    ok(!!items, "items is null\n");
    EXPECT_REF(folder, 2);
    EXPECT_REF(items, 1);

    unk = NULL;
    r = Folder_Items(folder, (FolderItems **)&unk);
    ok(r == S_OK, "Folder::Items failed: %08lx\n", r);
    EXPECT_REF(folder, 3);
    IUnknown_Release(unk);
    EXPECT_REF(folder, 2);

    FolderItems_AddRef(items);
    EXPECT_REF(folder, 2);
    FolderItems_Release(items);

    /* Application property */
    disp = NULL;
    EXPECT_REF(sd, 1);
    r = Folder_get_Application(folder, &disp);
    ok(r == S_OK, "Failed to get application %#lx.\n", r);
    ok(disp != (IDispatch *)sd, "Unexpected application pointer\n");
    EXPECT_REF(sd, 1);

    disp2 = NULL;
    r = Folder_get_Application(folder, &disp2);
    ok(r == S_OK, "Failed to get application %#lx.\n", r);
    ok(disp2 == disp, "Unexpected application pointer\n");
    IDispatch_Release(disp2);

    r = IDispatch_QueryInterface(disp, &IID_IShellDispatch, (void **)&disp2);
    ok(r == S_OK, "Wrong instance, hr %#lx.\n", r);
    IDispatch_Release(disp2);
    IDispatch_Release(disp);

    if (0) /* crashes on all versions of Windows */
        r = FolderItems_get_Count(items, NULL);

    r = FolderItems_get_Count(items, &count);
    ok(r == S_OK, "FolderItems::get_Count failed: %08lx\n", r);
    ok(!count, "expected 0 files, got %ld\n", count);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 0;

    if (0) /* crashes on all versions of Windows */
        r = FolderItems_Item(items, var, NULL);

    r = FolderItems_Item(items, var, &item);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    ok(!item, "item is not null\n");

    /* create test files */
    for (i = 0; i < ARRAY_SIZE(file_defs); i++)
    {
        switch (file_defs[i].type)
        {
            case DIRECTORY:
                r = CreateDirectoryW(file_defs[i].name, NULL);
                ok(r, "CreateDirectory failed: %08lx\n", GetLastError());
                PathCombineW(cstr, file_defs[i].name, L"foo.txt");
                file = CreateFileW(cstr, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %08lx\n", GetLastError());
                CloseHandle(file);
                break;

            case EMPTY_FILE:
                file = CreateFileW(file_defs[i].name, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %08lx\n", GetLastError());
                CloseHandle(file);
                break;
        }
    }

    /* test that get_Count is not aware of the newly created files */
    count = -1;
    r = FolderItems_get_Count(items, &count);
    ok(r == S_OK, "FolderItems::get_Count failed: %08lx\n", r);
    ok(!count, "expected 0 files, got %ld\n", count);

    /* test that the newly created files CAN be retrieved by string index */
    variant_set_string(&var, file_defs[0].name);
    item = NULL;
    r = FolderItems_Item(items, var, &item);
    ok(r == S_OK, "FolderItems::Item failed: %08lx\n", r);
    ok(!!item, "item is null\n");

    disp = (void *)0xdeadbeef;
    r = FolderItems_get_Parent(items, &disp);
    ok(r == E_NOTIMPL, "Unexpected hr %#lx.\n", r);
    ok(disp == NULL, "Unexpected parent pointer %p.\n", disp);

    r = FolderItem_get_Parent(item, &disp);
    ok(r == S_OK, "Failed to get parent pointer, hr %#lx.\n", r);
    ok(disp == (IDispatch *)folder, "Unexpected parent pointer %p.\n", disp);
    IDispatch_Release(disp);

    if (item) FolderItem_Release(item);
    VariantClear(&var);

    /* recreate the items object */
    FolderItems_Release(items);
    items = NULL;
    r = Folder_Items(folder, &items);
    ok(r == S_OK, "Folder::Items failed: %08lx\n", r);
    ok(!!items, "items is null\n");
    r = FolderItems_QueryInterface(items, &IID_FolderItems2, (void**)&items2);
    ok(r == S_OK || broken(r == E_NOINTERFACE) /* xp and later */, "FolderItems::QueryInterface failed: %08lx\n", r);
    if (r == S_OK)
    {
        ok(!!items2, "items2 is null\n");
        FolderItems2_Release(items2);
    }
    r = FolderItems_QueryInterface(items, &IID_FolderItems3, (void**)&items3);
    ok(r == S_OK, "FolderItems::QueryInterface failed: %08lx\n", r);
    ok(!!items3, "items3 is null\n");

    count = -1;
    r = FolderItems_get_Count(items, &count);
    ok(r == S_OK, "FolderItems::get_Count failed: %08lx\n", r);
    ok(count == ARRAY_SIZE(file_defs), "got %ld files\n", count);

    /* VT_EMPTY */
    V_VT(&var) = VT_EMPTY;
    item = (FolderItem*)0xdeadbeef;
    r = FolderItems_Item(items, var, &item);
    ok(r == E_NOTIMPL, "expected E_NOTIMPL, got %08lx\n", r);
    ok(!item, "item is not null\n");

    /* VT_I2 */
    V_VT(&var) = VT_I2;
    V_I2(&var) = 0;

    EXPECT_REF(folder, 2);
    EXPECT_REF(items, 2);
    item = NULL;
    r = FolderItems_Item(items, var, &item);
    ok(r == S_OK, "FolderItems::Item failed: %08lx\n", r);
    ok(!!item, "item is null\n");
    EXPECT_REF(folder, 3);
    EXPECT_REF(items, 2);

    r = Folder_get_Application(folder, &disp);
    ok(r == S_OK, "Failed to get application pointer %#lx.\n", r);
    r = FolderItem_get_Application(item, &disp2);
    ok(r == S_OK, "Failed to get application pointer %#lx.\n", r);
    ok(disp == disp2, "Unexpected application pointer.\n");
    IDispatch_Release(disp2);
    IDispatch_Release(disp);

    FolderItem_Release(item);

    /* VT_VARIANT | VT_BYREF */
    V_VT(&var2) = VT_I2;
    V_I2(&var2) = 0;

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VARIANTREF(&var) = &var2;

    item = NULL;
    r = FolderItems_Item(items, var, &item);
    ok(r == S_OK, "FolderItems::Item failed: %08lx\n", r);
    ok(!!item, "item is null\n");
    FolderItem_Release(item);

    /* VT_I4 */
    V_VT(&var) = VT_I4;
    V_I4(&var) = 0;
    item = NULL;
    r = FolderItems_Item(items, var, &item);
    ok(r == S_OK, "FolderItems::Item failed: %08lx\n", r);
    ok(!!item, "item is null\n");
    if (item) FolderItem_Release(item);

    V_I4(&var) = -1;
    item = (FolderItem*)0xdeadbeef;
    r = FolderItems_Item(items, var, &item);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    ok(!item, "item is not null\n");

    V_VT(&var) = VT_ERROR;
    V_ERROR(&var) = 0;
    item = NULL;
    r = FolderItems_Item(items, var, &item);
    ok(r == S_OK, "expected S_OK, got %08lx\n", r);
    ok(!!item, "item is null\n");
    if (item)
    {
        bstr = NULL;
        r = FolderItem_get_Path(item, &bstr);
        ok(r == S_OK, "FolderItem::get_Path failed: %08lx\n", r);
        ok(!lstrcmpW(bstr, cur_dir),
           "expected %s, got %s\n", wine_dbgstr_w(cur_dir), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
        FolderItem_Release(item);
    }

    V_VT(&int_index) = VT_I4;

    /* test the folder item corresponding to each file */
    for (i = 0; i < ARRAY_SIZE(file_defs); i++)
    {
        VARIANT_BOOL b;
        BSTR name;

        V_I4(&int_index) = i;
        variant_set_string(&str_index, file_defs[i].name);

        item = NULL;
        r = FolderItems_Item(items, int_index, &item);
        ok(r == S_OK, "file_defs[%d]: FolderItems::Item failed: %08lx\n", i, r);
        ok(!!item, "file_defs[%d]: item is null\n", i);

        item2 = NULL;
        r = FolderItems_Item(items, int_index, &item2);
        ok(r == S_OK, "file_defs[%d]: FolderItems::Item failed: %08lx\n", i, r);
        ok(item2 != item, "file_defs[%d]: item and item2 are the same\n", i);
        FolderItem_Release(item2);

        bstr = NULL;
        r = FolderItem_get_Path(item, &bstr);
        ok(r == S_OK, "file_defs[%d]: FolderItem::get_Path failed: %08lx\n", i, r);
        PathCombineW(path, cur_dir, V_BSTR(&str_index));
        ok(!lstrcmpW(bstr, path),
           "file_defs[%d]: expected %s, got %s\n", i, wine_dbgstr_w(path), wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        bstr = SysAllocString(file_defs[i].name);
        r = FolderItem_get_Name(item, &name);
        ok(r == S_OK, "Failed to get item name, hr %#lx.\n", r);
        /* Returned display name does not have to strictly match file name, e.g. extension could be omitted. */
        ok(lstrlenW(name) <= lstrlenW(bstr), "file_defs[%d]: unexpected name length.\n", i);
        ok(!memcmp(bstr, name, lstrlenW(name) * sizeof(WCHAR)), "file_defs[%d]: unexpected name %s.\n", i, wine_dbgstr_w(name));
        SysFreeString(name);
        SysFreeString(bstr);

        FolderItem_Release(item);

        item = NULL;
        r = FolderItems_Item(items, str_index, &item);
        ok(r == S_OK, "file_defs[%d]: FolderItems::Item failed: %08lx\n", i, r);
        ok(!!item, "file_defs[%d]: item is null\n", i);

        bstr = NULL;
        r = FolderItem_get_Path(item, &bstr);
        ok(r == S_OK, "file_defs[%d]: FolderItem::get_Path failed: %08lx\n", i, r);
        PathCombineW(path, cur_dir, V_BSTR(&str_index));
        ok(!lstrcmpW(bstr, path),
           "file_defs[%d]: expected %s, got %s\n", i, wine_dbgstr_w(path), wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        b = 0xdead;
        r = FolderItem_get_IsFolder(item, &b);
        ok(r == S_OK, "Failed to get IsFolder property, %#lx.\n", r);
        ok(file_defs[i].type == DIRECTORY ? b == VARIANT_TRUE : b == VARIANT_FALSE, "Unexpected prop value %#x.\n", b);

        FolderItem_Release(item);

        if (file_defs[i].type == DIRECTORY)
        {
            /* test that getting an item object for a file in a subdirectory succeeds */
            PathCombineW(cstr, file_defs[i].name, L"foo.txt");
            variant_set_string(&str_index2, cstr);
            item2 = NULL;
            r = FolderItems_Item(items, str_index2, &item2);
            ok(r == S_OK, "file_defs[%d]: FolderItems::Item failed: %08lx\n", i, r);
            ok(!!item2, "file_defs[%d]: item is null\n", i);
            if (item2) FolderItem_Release(item2);
            VariantClear(&str_index2);

            /* delete the file in the subdirectory */
            ret = DeleteFileW(cstr);
            ok(ret, "file_defs[%d]: DeleteFile failed: %08lx\n", i, GetLastError());

            /* test that getting an item object via a relative path fails */
            wcscpy(cstr, file_defs[i].name);
            wcscat(cstr, L"\\..\\");
            wcscat(cstr, file_defs[i].name);
            variant_set_string(&str_index2, cstr);
            item2 = (FolderItem*)0xdeadbeef;
            r = FolderItems_Item(items, str_index2, &item2);
            ok(r == S_FALSE, "file_defs[%d]: expected S_FALSE, got %08lx\n", i, r);
            ok(!item2, "file_defs[%d]: item is not null\n", i);
            VariantClear(&str_index2);

            /* remove the directory */
            ret = RemoveDirectoryW(file_defs[i].name);
            ok(ret, "file_defs[%d]: RemoveDirectory failed: %08lx\n", i, GetLastError());
        }
        else
        {
            ret = DeleteFileW(file_defs[i].name);
            ok(ret, "file_defs[%d]: DeleteFile failed: %08lx\n", i, GetLastError());
        }

        /* test that the folder item is still accessible by integer index */
        item = NULL;
        r = FolderItems_Item(items, int_index, &item);
        ok(r == S_OK, "file_defs[%d]: FolderItems::Item failed: %08lx\n", i, r);
        ok(!!item, "file_defs[%d]: item is null\n", i);

        bstr = NULL;
        r = FolderItem_get_Path(item, &bstr);
        ok(r == S_OK, "file_defs[%d]: FolderItem::get_Path failed: %08lx\n", i, r);
        PathCombineW(path, cur_dir, V_BSTR(&str_index));
        ok(!lstrcmpW(bstr, path),
           "file_defs[%d]: expected %s, got %s\n", i, wine_dbgstr_w(path), wine_dbgstr_w(bstr));
        SysFreeString(bstr);

        FolderItem_Release(item);

        /* test that the folder item is no longer accessible by string index */
        item = (FolderItem*)0xdeadbeef;
        r = FolderItems_Item(items, str_index, &item);
        ok(r == S_FALSE, "file_defs[%d]: expected S_FALSE, got %08lx\n", i, r);
        ok(!item, "file_defs[%d]: item is not null\n", i);

        VariantClear(&str_index);
    }

    /* test that there are only as many folder items as there were files */
    V_I4(&int_index) = ARRAY_SIZE(file_defs);
    item = (FolderItem*)0xdeadbeef;
    r = FolderItems_Item(items, int_index, &item);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    ok(!item, "item is not null\n");

    if (0) /* crashes on xp */
    {
        r = FolderItems_get_Application(items, NULL);
        ok(r == E_INVALIDARG, "expected E_INVALIDARG, got %08lx\n", r);
    }

    r = FolderItems_get_Application(items, &disp);
    ok(r == S_OK, "FolderItems::get_Application failed: %08lx\n", r);

    r = Folder_get_Application(folder, &disp2);
    ok(r == S_OK, "Failed to get application pointer, hr %#lx.\n", r);
    ok(disp == disp2, "Unexpected application pointer.\n");
    IDispatch_Release(disp2);
    IDispatch_Release(disp);

    if (0) /* crashes on xp */
    {
        r = FolderItems_get_Parent(items, NULL);
        ok(r == E_NOTIMPL, "expected E_NOTIMPL, got %08lx\n", r);
    }

    disp = (IDispatch*)0xdeadbeef;
    r = FolderItems_get_Parent(items, &disp);
    ok(r == E_NOTIMPL, "expected E_NOTIMPL, got %08lx\n", r);
    ok(!disp, "disp is not null\n");

    if (0) /* crashes on xp */
    {
        r = FolderItems__NewEnum(items, NULL);
        ok(r == E_INVALIDARG, "expected E_INVALIDARG, got %08lx\n", r);
    }

    r = FolderItems__NewEnum(items, &unk);
    todo_wine
    ok(r == S_OK, "FolderItems::_NewEnum failed: %08lx\n", r);
    todo_wine
    ok(!!unk, "unk is null\n");
    if (unk) IUnknown_Release(unk);

    if (items3)
    {
        r = FolderItems3_Filter(items3, 0, NULL);
        todo_wine
        ok(r == S_OK, "expected S_OK, got %08lx\n", r);

        if (0) /* crashes on xp */
        {
            r = FolderItems3_get_Verbs(items3, NULL);
            ok(r == E_INVALIDARG, "expected E_INVALIDARG, got %08lx\n", r);
        }

        r = FolderItems3_get_Verbs(items3, &verbs);
        todo_wine
        ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
        ok(!verbs, "verbs is not null\n");
    }

    /* remove the temporary directory and restore the original working directory */
    GetTempPathW(MAX_PATH, path);
    SetCurrentDirectoryW(path);
    ret = RemoveDirectoryW(L"winetest");
    ok(ret, "RemoveDirectory failed: %08lx\n", GetLastError());
    SetCurrentDirectoryW(orig_dir);

    /* test that everything stops working after the directory has been removed */
    count = -1;
    r = FolderItems_get_Count(items, &count);
    ok(r == S_OK, "FolderItems::get_Count failed: %08lx\n", r);
    ok(!count, "expected 0 files, got %ld\n", count);

    item = NULL;
    V_I4(&int_index) = 0;
    item = (FolderItem*)0xdeadbeef;
    r = FolderItems_Item(items, int_index, &item);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    ok(!item, "item is not null\n");

    variant_set_string(&str_index, file_defs[0].name);
    item = (FolderItem*)0xdeadbeef;
    r = FolderItems_Item(items, str_index, &item);
    ok(r == S_FALSE, "expected S_FALSE, got %08lx\n", r);
    ok(!item, "item is not null\n");
    VariantClear(&str_index);

    FolderItems_Release(items);
    Folder_Release(folder);
    if (items3) FolderItems3_Release(items3);
    IShellDispatch_Release(sd);
}

static void test_service(void)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    IShellDispatch2 *sd;
    DWORD dummy;
    HRESULT hr;
    BSTR name;
    VARIANT v;

    hr = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
        &IID_IShellDispatch2, (void**)&sd);
    if (hr != S_OK)
    {
        win_skip("IShellDispatch2 not supported\n");
        return;
    }

    V_VT(&v) = VT_I2;
    V_I2(&v) = 10;
    hr = IShellDispatch2_IsServiceRunning(sd, NULL, &v);
    ok(V_VT(&v) == VT_BOOL, "got %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_FALSE, "got %d\n", V_BOOL(&v));
    EXPECT_HR(hr, S_OK);

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    service = OpenServiceW(scm, L"Spooler", SERVICE_QUERY_STATUS);
    QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(SERVICE_STATUS_PROCESS), &dummy);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    /* service should exist */
    name = SysAllocString(L"Spooler");
    V_VT(&v) = VT_I2;
    hr = IShellDispatch2_IsServiceRunning(sd, name, &v);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_BOOL, "got %d\n", V_VT(&v));
    if (status.dwCurrentState == SERVICE_RUNNING)
        ok(V_BOOL(&v) == VARIANT_TRUE, "got %d\n", V_BOOL(&v));
    else
        ok(V_BOOL(&v) == VARIANT_FALSE, "got %d\n", V_BOOL(&v));
    SysFreeString(name);

    /* service doesn't exist */
    name = SysAllocString(L"dummy");
    V_VT(&v) = VT_I2;
    hr = IShellDispatch2_IsServiceRunning(sd, name, &v);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_BOOL, "got %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_FALSE, "got %d\n", V_BOOL(&v));
    SysFreeString(name);

    IShellDispatch2_Release(sd);
}

static void test_dispatch_typeinfo(IDispatch *disp, REFIID *riid)
{
    ITypeInfo *typeinfo;
    TYPEATTR *typeattr;
    UINT count;
    HRESULT hr;

    count = 10;
    hr = IDispatch_GetTypeInfoCount(disp, &count);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(count == 1, "got %u\n", count);

    hr = IDispatch_GetTypeInfo(disp, 0, LOCALE_SYSTEM_DEFAULT, &typeinfo);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    while (!IsEqualGUID(*riid, &IID_NULL)) {
        if (IsEqualGUID(&typeattr->guid, *riid))
            break;
        riid++;
    }
    ok(IsEqualGUID(&typeattr->guid, *riid), "unexpected type guid %s\n", wine_dbgstr_guid(&typeattr->guid));

    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);
}

static void test_ShellFolderViewDual(void)
{
    static const IID *shelldisp_riids[] = {
        &IID_IShellDispatch6,
        &IID_IShellDispatch5,
        &IID_IShellDispatch4,
        &IID_IShellDispatch2,
        &IID_IWin7ShellDispatch6,
        &IID_NULL
    };
    IShellFolderViewDual *viewdual;
    IShellFolder *desktop, *tmpdir;
    IShellView *view, *view2;
    IDispatch *disp, *disp2;
    WCHAR pathW[MAX_PATH];
    LPITEMIDLIST pidl;
    HRESULT hr;

    /* IShellFolderViewDual is not an IShellView extension */
    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellView_QueryInterface(view, &IID_IShellFolderViewDual, (void**)&viewdual);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IDispatch, (void**)&disp);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IDispatch, (void**)&disp2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(disp2 == disp, "got %p, %p\n", disp2, disp);
    IDispatch_Release(disp2);

    hr = IDispatch_QueryInterface(disp, &IID_IShellFolderViewDual, (void**)&viewdual);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(disp == (IDispatch*)viewdual, "got %p, expected %p\n", viewdual, disp);

    hr = IShellFolderViewDual_QueryInterface(viewdual, &IID_IShellView, (void**)&view2);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    /* get_Application() */

if (0) /* crashes on pre-vista */ {
    hr = IShellFolderViewDual_get_Application(viewdual, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}
    hr = IShellFolderViewDual_get_Application(viewdual, &disp2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(disp2 != (IDispatch*)viewdual, "got %p, %p\n", disp2, viewdual);
    test_dispatch_typeinfo(disp2, shelldisp_riids);
    IDispatch_Release(disp2);

    IShellFolderViewDual_Release(viewdual);
    IDispatch_Release(disp);

    disp = (void*)0xdeadbeef;
    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IShellFolderViewDual, (void**)&disp);
    ok(hr == E_NOINTERFACE || broken(hr == E_NOTIMPL) /* win2k */, "got 0x%08lx\n", hr);
    ok(disp == NULL, "got %p\n", disp);
    IShellView_Release(view);

    /* Try with some other folder, that's not a desktop */
    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, pathW, NULL, &pidl, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellFolder_BindToObject(desktop, pidl, NULL, &IID_IShellFolder, (void**)&tmpdir);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    CoTaskMemFree(pidl);

    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IShellView, (void**)&view);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellView_QueryInterface(view, &IID_IShellFolderViewDual, (void**)&viewdual);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IDispatch, (void**)&disp);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    IDispatch_Release(disp);
    IShellView_Release(view);

    IShellFolder_Release(tmpdir);
    IShellFolder_Release(desktop);
}

static void test_ShellWindows(void)
{
    IShellWindows *shellwindows;
    LONG cookie, cookie2, ret;
    ITEMIDLIST *pidl;
    IDispatch *disp;
    VARIANT v, v2;
    HRESULT hr;
    HWND hwnd;

    hr = CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER,
        &IID_IShellWindows, (void**)&shellwindows);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    /* TODO: remove when explorer startup with clean prefix is fixed */
    if (hr != S_OK)
        return;

    hr = IShellWindows_Register(shellwindows, NULL, 0, SWC_EXPLORER, NULL);
    ok(hr == HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER), "got 0x%08lx\n", hr);

    hr = IShellWindows_Register(shellwindows, NULL, 0, SWC_EXPLORER, &cookie);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hr = IShellWindows_Register(shellwindows, (IDispatch*)shellwindows, 0, SWC_EXPLORER, &cookie);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hr = IShellWindows_Register(shellwindows, (IDispatch*)shellwindows, 0, SWC_EXPLORER, &cookie);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hwnd = CreateWindowExA(0, "button", "test", BS_CHECKBOX | WS_VISIBLE | WS_POPUP,
                           0, 0, 50, 14, 0, 0, 0, NULL);
    ok(hwnd != NULL, "got %p, error %ld\n", hwnd, GetLastError());

    cookie = 0;
    hr = IShellWindows_Register(shellwindows, NULL, HandleToLong(hwnd), SWC_EXPLORER, &cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cookie != 0, "got %ld\n", cookie);

    cookie2 = 0;
    hr = IShellWindows_Register(shellwindows, NULL, HandleToLong(hwnd), SWC_EXPLORER, &cookie2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cookie2 != 0 && cookie2 != cookie, "got %ld\n", cookie2);

    pidl = ILCreateFromPathA("C:\\");
    V_VT(&v) = VT_ARRAY | VT_UI1;
    V_ARRAY(&v) = SafeArrayCreateVector(VT_UI1, 0, ILGetSize(pidl));
    memcpy(V_ARRAY(&v)->pvData, pidl, ILGetSize(pidl));

    VariantInit(&v2);
    hr = IShellWindows_FindWindowSW(shellwindows, &v, &v2, SWC_EXPLORER, &ret, 0, &disp);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!ret, "Got window %#lx.\n", ret);
    ok(!disp, "Got IDispatch %p.\n", &disp);

    hr = IShellWindows_OnNavigate(shellwindows, 0, &v);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = IShellWindows_OnNavigate(shellwindows, cookie, &v);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IShellWindows_FindWindowSW(shellwindows, &v, &v2, SWC_EXPLORER, &ret, 0, &disp);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(ret == (LONG)(LONG_PTR)hwnd, "Expected %p, got %#lx.\n", hwnd, ret);
    ok(!disp, "Got IDispatch %p.\n", &disp);

    hr = IShellWindows_Revoke(shellwindows, cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellWindows_FindWindowSW(shellwindows, &v, &v2, SWC_EXPLORER, &ret, 0, &disp);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    ok(!ret, "Got window %#lx.\n", ret);
    ok(!disp, "Got IDispatch %p.\n", &disp);

    hr = IShellWindows_Revoke(shellwindows, cookie2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellWindows_Revoke(shellwindows, 0);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    /* we can register ourselves as desktop, but FindWindowSW still returns real desktop window */
    cookie = 0;
    hr = IShellWindows_Register(shellwindows, NULL, HandleToLong(hwnd), SWC_DESKTOP, &cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cookie != 0, "got %ld\n", cookie);

    disp = (void*)0xdeadbeef;
    ret = 0xdead;
    VariantInit(&v);
    hr = IShellWindows_FindWindowSW(shellwindows, &v, &v, SWC_DESKTOP, &ret, SWFO_NEEDDISPATCH, &disp);
    ok(hr == S_OK || broken(hr == S_FALSE), "got 0x%08lx\n", hr);
    if (hr == S_FALSE) /* winxp and earlier */ {
        win_skip("SWC_DESKTOP is not supported, some tests will be skipped.\n");
        /* older versions allowed to register SWC_DESKTOP and access it with FindWindowSW */
        ok(disp == NULL, "got %p\n", disp);
        ok(ret == 0, "got %ld\n", ret);
    }
    else {
        static const IID *browser_riids[] = {
            &IID_IWebBrowser2,
            &IID_NULL
        };

        static const IID *viewdual_riids[] = {
            &IID_IShellFolderViewDual3,
            &IID_NULL
        };

        IShellFolderViewDual *view;
        IShellBrowser *sb, *sb2;
        IServiceProvider *sp;
        IDispatch *doc, *app;
        IWebBrowser2 *wb;
        IShellView *sv;
        IUnknown *unk;

        ok(disp != NULL, "got %p\n", disp);
        ok(ret != HandleToUlong(hwnd), "got %ld\n", ret);

        /* IDispatch-related tests */
        test_dispatch_typeinfo(disp, browser_riids);

        /* IWebBrowser2 */
        hr = IDispatch_QueryInterface(disp, &IID_IWebBrowser2, (void**)&wb);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IWebBrowser2_Refresh(wb);
        todo_wine
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IWebBrowser2_get_Application(wb, &app);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(disp == app, "got %p, %p\n", app, disp);
        IDispatch_Release(app);

        hr = IWebBrowser2_get_Document(wb, &doc);
        todo_wine
        ok(hr == S_OK, "got 0x%08lx\n", hr);
if (hr == S_OK) {
        test_dispatch_typeinfo(doc, viewdual_riids);
}
        IWebBrowser2_Release(wb);

        /* IServiceProvider */
        hr = IDispatch_QueryInterface(disp, &IID_IShellFolderViewDual, (void**)&view);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        hr = IDispatch_QueryInterface(disp, &IID_IServiceProvider, (void**)&sp);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IServiceProvider_QueryService(sp, &SID_STopLevelBrowser, &IID_IShellBrowser, (void**)&sb);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IServiceProvider_QueryService(sp, &SID_STopLevelBrowser, &IID_IShellBrowser, (void**)&sb2);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(sb == sb2, "got %p, %p\n", sb, sb2);

        hr = IServiceProvider_QueryService(sp, &SID_STopLevelBrowser, &IID_IOleWindow, (void**)&unk);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        IUnknown_Release(unk);

        hr = IServiceProvider_QueryService(sp, &SID_STopLevelBrowser, &IID_IExplorerBrowser, (void**)&unk);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        hr = IShellBrowser_QueryInterface(sb, &IID_IExplorerBrowser, (void**)&unk);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        hr = IShellBrowser_QueryInterface(sb, &IID_IWebBrowser2, (void**)&unk);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        hr = IShellBrowser_QueryInterface(sb, &IID_IDispatch, (void**)&unk);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        hr = IShellBrowser_QueryActiveShellView(sb, &sv);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        IShellView_Release(sv);

        IShellBrowser_Release(sb2);
        IShellBrowser_Release(sb);

        hr = IServiceProvider_QueryService(sp, &SID_STopLevelBrowser, &IID_IUnknown, (void**)&unk);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IUnknown_QueryInterface(unk, &IID_IShellBrowser, (void**)&sb2);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        IShellBrowser_Release(sb2);
        IUnknown_Release(unk);

        hr = IServiceProvider_QueryService(sp, &SID_STopLevelBrowser, &IID_IShellView, (void**)&sv);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        IServiceProvider_Release(sp);
        IDispatch_Release(disp);
    }

    disp = (void*)0xdeadbeef;
    ret = 0xdead;
    VariantInit(&v);
    hr = IShellWindows_FindWindowSW(shellwindows, &v, &v, SWC_DESKTOP, &ret, 0, &disp);
    ok(hr == S_OK || broken(hr == S_FALSE) /* winxp */, "got 0x%08lx\n", hr);
    ok(disp == NULL, "got %p\n", disp);
    ok(ret != HandleToUlong(hwnd), "got %ld\n", ret);

    disp = (void*)0xdeadbeef;
    ret = 0xdead;
    V_VT(&v) = VT_I4;
    V_I4(&v) = cookie;
    VariantInit(&v2);
    hr = IShellWindows_FindWindowSW(shellwindows, &v, &v2, SWC_BROWSER, &ret, SWFO_COOKIEPASSED, &disp);
    todo_wine
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);
    ok(disp == NULL, "got %p\n", disp);
    ok(ret == 0, "got %ld\n", ret);

    hr = IShellWindows_Revoke(shellwindows, cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    DestroyWindow(hwnd);
    IShellWindows_Release(shellwindows);
}

static void test_ParseName(void)
{
    WCHAR pathW[MAX_PATH];
    IShellDispatch *sd;
    FolderItem *item;
    Folder *folder;
    HRESULT hr;
    VARIANT v;
    BSTR str;

    hr = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
        &IID_IShellDispatch, (void**)&sd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pathW);
    hr = IShellDispatch_NameSpace(sd, v, &folder);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    VariantClear(&v);

    item = (void*)0xdeadbeef;
    hr = Folder_ParseName(folder, NULL, &item);
    ok(hr == S_FALSE || broken(hr == E_INVALIDARG) /* win2k */, "got 0x%08lx\n", hr);
    ok(item == NULL, "got %p\n", item);

    /* empty name */
    str = SysAllocStringLen(NULL, 0);
    item = (void*)0xdeadbeef;
    hr = Folder_ParseName(folder, str, &item);
    ok(hr == S_FALSE || broken(hr == E_INVALIDARG) /* win2k */, "got 0x%08lx\n", hr);
    ok(item == NULL, "got %p\n", item);
    SysFreeString(str);

    /* path doesn't exist */
    str = SysAllocString(L"cadabra");
    item = (void*)0xdeadbeef;
    hr = Folder_ParseName(folder, str, &item);
    ok(hr == S_FALSE || broken(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) /* win2k */,
        "got 0x%08lx\n", hr);
    ok(item == NULL, "got %p\n", item);
    SysFreeString(str);

    lstrcatW(pathW, L"cadabra");
    CreateDirectoryW(pathW, NULL);

    str = SysAllocString(L"cadabra");
    item = NULL;
    hr = Folder_ParseName(folder, str, &item);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(item != NULL, "got %p\n", item);
    SysFreeString(str);

    hr = FolderItem_get_Path(item, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(str[0] != 0, "path %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    RemoveDirectoryW(pathW);
    FolderItem_Release(item);
    Folder_Release(folder);
    IShellDispatch_Release(sd);
}

static void test_Verbs(void)
{
    FolderItemVerbs *verbs, *verbs2;
    WCHAR pathW[MAX_PATH];
    FolderItemVerb *verb;
    IShellDispatch *sd;
    FolderItem *item;
    Folder2 *folder2;
    IDispatch *disp;
    Folder *folder;
    HRESULT hr;
    LONG count, i;
    VARIANT v;
    BSTR str;

    hr = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
        &IID_IShellDispatch, (void**)&sd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pathW);
    hr = IShellDispatch_NameSpace(sd, v, &folder);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    VariantClear(&v);

    hr = Folder_QueryInterface(folder, &IID_Folder2, (void**)&folder2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    Folder_Release(folder);

    hr = Folder2_get_Self(folder2, &item);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    Folder2_Release(folder2);

if (0) { /* crashes on some systems */
    hr = FolderItem_Verbs(item, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}
    hr = FolderItem_Verbs(item, &verbs);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = FolderItem_Verbs(item, &verbs2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(verbs2 != verbs, "Unexpected verbs pointer.\n");
    FolderItemVerbs_Release(verbs2);

    disp = (void *)0xdeadbeef;
    hr = FolderItemVerbs_get_Application(verbs, &disp);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(disp == NULL, "Unexpected application pointer.\n");

    disp = (void *)0xdeadbeef;
    hr = FolderItemVerbs_get_Parent(verbs, &disp);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(disp == NULL, "Unexpected parent pointer %p.\n", disp);

if (0) { /* crashes on winxp/win2k3 */
    hr = FolderItemVerbs_get_Count(verbs, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}
    count = 0;
    hr = FolderItemVerbs_get_Count(verbs, &count);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(count > 0, "got count %ld\n", count);

if (0) { /* crashes on winxp/win2k3 */
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0;
    hr = FolderItemVerbs_Item(verbs, v, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}
    /* there's always one item more, so you can access [0,count],
       instead of actual [0,count) */
    for (i = 0; i <= count; i++) {
        V_VT(&v) = VT_I4;
        V_I4(&v) = i;
        hr = FolderItemVerbs_Item(verbs, v, &verb);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        hr = FolderItemVerb_get_Name(verb, &str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(str != NULL, "%ld: name %s\n", i, wine_dbgstr_w(str));
        if (i == count)
            ok(str[0] == 0, "%ld: got terminating item %s\n", i, wine_dbgstr_w(str));

        disp = (void *)0xdeadbeef;
        hr = FolderItemVerb_get_Parent(verb, &disp);
        ok(hr == E_NOTIMPL, "got %#lx.\n", hr);
        ok(disp == NULL, "Unexpected parent pointer %p.\n", disp);

        disp = (void *)0xdeadbeef;
        hr = FolderItemVerb_get_Application(verb, &disp);
        ok(hr == E_NOTIMPL, "got %#lx.\n", hr);
        ok(disp == NULL, "Unexpected parent pointer %p.\n", disp);

        SysFreeString(str);
        FolderItemVerb_Release(verb);
    }

    V_VT(&v) = VT_I4;
    V_I4(&v) = count+1;
    verb = NULL;
    hr = FolderItemVerbs_Item(verbs, v, &verb);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(verb == NULL, "got %p\n", verb);

    FolderItemVerbs_Release(verbs);
    FolderItem_Release(item);
    IShellDispatch_Release(sd);
}

static void test_ShellLinkObject(void)
{
    HRESULT hr;
    IShellDispatch *sd;
    WCHAR path[MAX_PATH],
        empty_path[MAX_PATH],
        link_path[MAX_PATH];
    VARIANT v;
    Folder2 *folder2;
    Folder *folder;
    FolderItem *item;
    IDispatch *dispatch;
    IShellLinkW *sl;
    IShellLinkDual2* sld;
    IPersistFile *pf;
    BOOL ret;
    BSTR str;
    HANDLE file;
    int hk;

    hr = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
        &IID_IShellDispatch, (void**)&sd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    GetTempPathW(MAX_PATH, path);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(path);
    hr = IShellDispatch_NameSpace(sd, v, &folder);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    VariantClear(&v);

    hr = Folder_QueryInterface(folder, &IID_Folder2, (void**)&folder2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    Folder_Release(folder);

    hr = Folder2_get_Self(folder2, &item);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    dispatch = (IDispatch*)0xdeadbeef;
    hr = FolderItem_get_GetLink(item, &dispatch);
    ok(hr == E_NOTIMPL, "got 0x%08lx\n", hr);
    ok(dispatch == NULL, "got %p\n", dispatch);

    FolderItem_Release(item);

    PathCombineW(empty_path, path, L"winetest_empty_file.txt");
    file = CreateFileW(empty_path, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %08lx\n", GetLastError());
    CloseHandle(file);

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID*)&sl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_SetPath(sl, empty_path);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_GetPath(sl, empty_path, MAX_PATH, NULL, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_SetDescription(sl, L"description");
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_SetWorkingDirectory(sl, L"working directory");
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_SetArguments(sl, L"arguments");
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_SetHotkey(sl, 1234);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellLinkW_SetShowCmd(sl, 1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellLinkW_QueryInterface(sl, &IID_IPersistFile, (LPVOID*)&pf);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    PathCombineW(link_path, path, L"winetest_filled.lnk");
    hr = IPersistFile_Save(pf, link_path, TRUE);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IPersistFile_Release(pf);
    IShellLinkW_Release(sl);

    str = SysAllocString(L"winetest_filled.lnk");
    hr = Folder2_ParseName(folder2, str, &item);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    dispatch = NULL;
    hr = FolderItem_get_GetLink(item, &dispatch);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(dispatch != NULL, "got %p\n", dispatch);

    if (dispatch) {
        sld = (IShellLinkDual2*)dispatch;

        str = NULL;
        hr = IShellLinkDual2_get_Path(sld, &str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        if (hr == S_OK) {
            ok(!wcscmp(str, empty_path), "got %s (wanted %s)\n",
               wine_dbgstr_w(str), wine_dbgstr_w(empty_path));
            SysFreeString(str);
        }

        str = NULL;
        hr = IShellLinkDual2_get_Description(sld, &str);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
        if (hr == S_OK) {
            ok(!wcscmp(str, L"description"), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);
        }

        str = NULL;
        hr = IShellLinkDual2_get_WorkingDirectory(sld, &str);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
        if (hr == S_OK) {
            ok(!wcscmp(str, L"working directory"), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);
        }

        str = NULL;
        hr = IShellLinkDual2_get_Arguments(sld, &str);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
        if (hr == S_OK) {
            ok(!wcscmp(str, L"arguments"), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);
        }

        hk = 0;
        hr = IShellLinkDual2_get_Hotkey(sld, &hk);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
        todo_wine ok(hk == 1234, "got %i\n", hk);

        hk = 0;
        hr = IShellLinkDual2_get_ShowCommand(sld, &hk);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
        todo_wine ok(hk == 1, "got %i\n", hk);

        IShellLinkDual2_Release(sld);
    }

    FolderItem_Release(item);

    ret = DeleteFileW(link_path);
    ok(ret, "DeleteFile failed: %08lx\n", GetLastError());
    ret = DeleteFileW(empty_path);
    ok(ret, "DeleteFile failed: %08lx\n", GetLastError());

    Folder2_Release(folder2);

    IShellDispatch_Release(sd);
}

static void test_ShellExecute(void)
{
    HRESULT hr;
    IShellDispatch2 *sd;
    BSTR name;
    VARIANT args, dir, op, show;

    hr = CoCreateInstance(&CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
        &IID_IShellDispatch2, (void**)&sd);
    if (hr != S_OK)
    {
        win_skip("IShellDispatch2 not supported\n");
        return;
    }

    VariantInit(&args);
    VariantInit(&dir);
    VariantInit(&op);
    VariantInit(&show);

    V_VT(&show) = VT_I4;
    V_I4(&show) = 0;

    name = SysAllocString(L"reg");

    hr = IShellDispatch2_ShellExecute(sd, name, args, dir, op, show);
    ok(hr == S_OK, "ShellExecute failed: %08lx\n", hr);

    /* test invalid value for show */
    V_VT(&show) = VT_BSTR;
    V_BSTR(&show) = name;

    hr = IShellDispatch2_ShellExecute(sd, name, args, dir, op, show);
    ok(hr == S_OK, "ShellExecute failed: %08lx\n", hr);

    SysFreeString(name);
    IShellDispatch2_Release(sd);
}

START_TEST(shelldispatch)
{
    HRESULT r;

    r = CoInitialize(NULL);
    ok(SUCCEEDED(r), "CoInitialize failed: %08lx\n", r);
    if (FAILED(r))
        return;

    init_function_pointers();
    test_namespace();
    test_items();
    test_service();
    test_ShellFolderViewDual();
    test_ShellWindows();
    test_ParseName();
    test_Verbs();
    test_ShellLinkObject();
    test_ShellExecute();

    CoUninitialize();
}
