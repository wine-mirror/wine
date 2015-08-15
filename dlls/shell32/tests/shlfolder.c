/*
 * Unit test of the IShellFolder functions.
 *
 * Copyright 2004 Vitaliy Margolen
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

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"


#include "shlguid.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "shlwapi.h"
#include "ocidl.h"
#include "oleauto.h"

#include "wine/test.h"

#include <initguid.h>
DEFINE_GUID(CLSID_ShellDocObjView, 0xe7e4bc40, 0xe76a, 0x11ce, 0xa9,0xbb, 0x00,0xaa,0x00,0x4a,0xe8,0x37);

static HRESULT (WINAPI *pSHCreateItemFromIDList)(PCIDLIST_ABSOLUTE pidl, REFIID riid, void **ppv);
static HRESULT (WINAPI *pSHCreateItemFromParsingName)(PCWSTR,IBindCtx*,REFIID,void**);
static HRESULT (WINAPI *pSHCreateItemFromRelativeName)(IShellItem*,PCWSTR,IBindCtx*,REFIID,void**);
static HRESULT (WINAPI *pSHCreateItemInKnownFolder)(REFKNOWNFOLDERID,DWORD,PCWSTR,REFIID,void **);
static HRESULT (WINAPI *pSHCreateShellItem)(LPCITEMIDLIST,IShellFolder*,LPCITEMIDLIST,IShellItem**);
static HRESULT (WINAPI *pSHCreateShellItemArray)(LPCITEMIDLIST,IShellFolder*,UINT,LPCITEMIDLIST*,IShellItemArray**);
static HRESULT (WINAPI *pSHCreateShellItemArrayFromIDLists)(UINT, PCIDLIST_ABSOLUTE*, IShellItemArray**);
static HRESULT (WINAPI *pSHCreateShellItemArrayFromDataObject)(IDataObject*, REFIID, void **);
static HRESULT (WINAPI *pSHCreateShellItemArrayFromShellItem)(IShellItem*, REFIID, void **);
static HRESULT (WINAPI *pSHGetKnownFolderPath)(REFKNOWNFOLDERID,DWORD,HANDLE,PWSTR*);
static HRESULT (WINAPI *pSHGetNameFromIDList)(PCIDLIST_ABSOLUTE,SIGDN,PWSTR*);
static HRESULT (WINAPI *pSHGetItemFromDataObject)(IDataObject*,DATAOBJ_GET_ITEM_FLAGS,REFIID,void**);
static HRESULT (WINAPI *pSHGetIDListFromObject)(IUnknown*, PIDLIST_ABSOLUTE*);
static HRESULT (WINAPI *pSHGetItemFromObject)(IUnknown*,REFIID,void**);
static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static HRESULT (WINAPI *pSHCreateDefaultContextMenu)(const DEFCONTEXTMENU*,REFIID,void**);
static BOOL (WINAPI *pSHGetPathFromIDListEx)(PCIDLIST_ABSOLUTE,WCHAR*,DWORD,GPFIDL_FLAGS);
static HRESULT (WINAPI *pSHGetSetFolderCustomSettings)(LPSHFOLDERCUSTOMSETTINGS,PCWSTR,DWORD);
static HRESULT (WINAPI *pSHBindToFolderIDListParent)(IShellFolder*,LPCITEMIDLIST,REFIID,void **,LPCITEMIDLIST*);

static WCHAR *make_wstr(const char *str)
{
    WCHAR *ret;
    int len;

    if (!str || !str[0])
        return NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    if(!len || len < 0)
        return NULL;

    ret = malloc(len * sizeof(WCHAR));
    if(!ret)
        return NULL;

    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BOOL check_window_exists(const char *name)
{
    HWND window = NULL;
    int i;

    for (i = 0; i < 10; i++)
    {
        if ((window = FindWindowA("ExplorerWClass", name))
            || (window = FindWindowA("CabinetWClass", name)))
        {
            SendMessageA(window, WM_SYSCOMMAND, SC_CLOSE, 0);
            break;
        }

        Sleep(100);
    }

    if (!window)
        return FALSE;

    for (i = 0; i < 10; i++)
    {
        if (!IsWindow(window))
            break;

        Sleep(100);
    }

    return TRUE;
}

static void init_function_pointers(void)
{
    HMODULE hmod;
    void *ptr;

    hmod = GetModuleHandleA("shell32.dll");

#define MAKEFUNC(f) (p##f = (void*)GetProcAddress(hmod, #f))
    MAKEFUNC(SHBindToFolderIDListParent);
    MAKEFUNC(SHCreateItemFromIDList);
    MAKEFUNC(SHCreateItemFromParsingName);
    MAKEFUNC(SHCreateItemFromRelativeName);
    MAKEFUNC(SHCreateItemInKnownFolder);
    MAKEFUNC(SHCreateShellItem);
    MAKEFUNC(SHCreateShellItemArray);
    MAKEFUNC(SHCreateShellItemArrayFromIDLists);
    MAKEFUNC(SHCreateShellItemArrayFromDataObject);
    MAKEFUNC(SHCreateShellItemArrayFromShellItem);
    MAKEFUNC(SHGetKnownFolderPath);
    MAKEFUNC(SHGetNameFromIDList);
    MAKEFUNC(SHGetItemFromDataObject);
    MAKEFUNC(SHGetIDListFromObject);
    MAKEFUNC(SHGetItemFromObject);
    MAKEFUNC(SHCreateDefaultContextMenu);
    MAKEFUNC(SHGetPathFromIDListEx);
    MAKEFUNC(SHGetSetFolderCustomSettings);
#undef MAKEFUNC

    /* test named exports */
    ptr = GetProcAddress(hmod, "ILFree");
    ok(broken(ptr == 0) || ptr != 0, "expected named export for ILFree\n");
    if (ptr)
    {
#define TESTNAMED(f) \
    ptr = (void*)GetProcAddress(hmod, #f); \
    ok(ptr != 0, "expected named export for " #f "\n");

        TESTNAMED(ILAppendID);
        TESTNAMED(ILClone);
        TESTNAMED(ILCloneFirst);
        TESTNAMED(ILCombine);
        TESTNAMED(ILCreateFromPath);
        TESTNAMED(ILCreateFromPathA);
        TESTNAMED(ILCreateFromPathW);
        TESTNAMED(ILFindChild);
        TESTNAMED(ILFindLastID);
        TESTNAMED(ILGetNext);
        TESTNAMED(ILGetSize);
        TESTNAMED(ILIsEqual);
        TESTNAMED(ILIsParent);
        TESTNAMED(ILRemoveLastID);
        TESTNAMED(ILSaveToStream);
#undef TESTNAMED
    }

    hmod = GetModuleHandleA("kernel32.dll");
    pIsWow64Process = (void*)GetProcAddress(hmod, "IsWow64Process");
}

/* Based on PathAddBackslashW from dlls/shlwapi/path.c */
static LPWSTR myPathAddBackslashW( LPWSTR lpszPath )
{
  size_t iLen;

  if (!lpszPath || (iLen = lstrlenW(lpszPath)) >= MAX_PATH)
    return NULL;

  if (iLen)
  {
    lpszPath += iLen;
    if (lpszPath[-1] != '\\')
    {
      *lpszPath++ = '\\';
      *lpszPath = '\0';
    }
  }
  return lpszPath;
}

static struct
{
    WCHAR path[MAX_PATH];
    HRESULT hr;
    int todo;
} parse_tests[] = {
    {L"c:\\", S_OK},
    {L"c:\\\\", E_INVALIDARG, 1},
    {L"c:\\fake", 0x80070002}, /* ERROR_FILE_NOT_FOUND */
    {L"c:fake", E_INVALIDARG},
    {L"c:/", E_INVALIDARG},
    {L"c:\\windows", S_OK},
    {L"c:\\windows\\", S_OK},
    {L"c:\\windows\\.", E_INVALIDARG, 1},
    {L"c:\\windows\\..", E_INVALIDARG, 1},
    {L".", E_INVALIDARG, 1},
    {L"..", E_INVALIDARG, 1},
    {L"test", 0x80070002},
    {L"test\\", 0x80070002},
    {L"sub\\dir", 0x80070002},
    {L"sub/dir", E_INVALIDARG, 1},
    {L"http:", S_OK, 1},
    {L"http:test", S_OK, 1},
    {L"http:\\test", S_OK, 1},
    {L"xx:", S_OK, 1},
};

static void test_ParseDisplayName(void)
{
    static WCHAR testdirW[] = L"parsetest";
    WCHAR buffer[MAX_PATH], buffer2[MAX_PATH];
    IShellFolder *desktop;
    ITEMIDLIST *pidl;
    HRESULT hr;
    BOOL bRes;
    int i;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "Expected SHGetDesktopFolder to return S_OK, got 0x%08lx\n", hr);

    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, NULL, NULL, &pidl, NULL);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(parse_tests); i++)
    {
        hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, parse_tests[i].path, NULL, &pidl, NULL);
        todo_wine_if(parse_tests[i].todo)
        ok(hr == parse_tests[i].hr, "%s: expected %#lx, got %#lx\n",
            wine_dbgstr_w(parse_tests[i].path), parse_tests[i].hr, hr);
        if (SUCCEEDED(hr))
            CoTaskMemFree(pidl);
    }

    /* I thought that perhaps the DesktopFolder's ParseDisplayName would recognize the
     * path corresponding to CSIDL_PERSONAL and return a CLSID_MyDocuments PIDL. Turns
     * out it doesn't. The magic seems to happen in the file dialogs, then. */

    bRes = SHGetSpecialFolderPathW(NULL, buffer, CSIDL_PERSONAL, FALSE);
    ok(bRes, "SHGetSpecialFolderPath(CSIDL_PERSONAL) failed! %lu\n", GetLastError());

    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, buffer, NULL, &pidl, 0);
    ok(hr == S_OK, "DesktopFolder->ParseDisplayName failed. hr = %08lx.\n", hr);

    ok(ILFindLastID(pidl)->mkid.abID[0] == 0x31,
       "Last pidl should be of type PT_FOLDER, but is: %02x\n",
       ILFindLastID(pidl)->mkid.abID[0]);
    CoTaskMemFree(pidl);

    /* Relative paths are interpreted relative to the desktop. */
    GetTempPathW(ARRAY_SIZE(buffer), buffer);
    GetLongPathNameW(buffer, buffer, ARRAY_SIZE(buffer));
    SetCurrentDirectoryW(buffer);
    CreateDirectoryW(testdirW, NULL);

    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, testdirW, NULL, &pidl, NULL);
    ok(hr == 0x80070002, "got %#lx\n", hr);

    RemoveDirectoryW(testdirW);

    hr = SHGetSpecialFolderPathW(NULL, buffer, CSIDL_DESKTOP, FALSE);
    ok(hr == S_FALSE, "got %#lx\n", hr);
    SetCurrentDirectoryW(buffer);
    CreateDirectoryW(testdirW, NULL);

    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, testdirW, NULL, &pidl, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    ok(SHGetPathFromIDListW(pidl, buffer2), "SHGetPathFromIDList failed\n");
    lstrcatW(buffer, L"\\");
    lstrcatW(buffer, testdirW);
    ok(!lstrcmpW(buffer, buffer2), "expected %s, got %s\n", wine_dbgstr_w(buffer), wine_dbgstr_w(buffer2));

    RemoveDirectoryW(testdirW);
    CoTaskMemFree(pidl);

    IShellFolder_Release(desktop);
}

/* creates a file with the specified name for tests */
static void CreateTestFile(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
	WriteFile(file, name, strlen(name), &written, NULL);
	WriteFile(file, "\n", strlen("\n"), &written, NULL);
	CloseHandle(file);
    }
}


/* initializes the tests */
static void CreateFilesFolders(void)
{
    CreateDirectoryA(".\\testdir", NULL);
    CreateDirectoryA(".\\testdir\\test.txt", NULL);
    CreateTestFile  (".\\testdir\\test1.txt ");
    CreateTestFile  (".\\testdir\\test2.txt ");
    CreateTestFile  (".\\testdir\\test3.txt ");
    CreateDirectoryA(".\\testdir\\testdir2 ", NULL);
    CreateDirectoryA(".\\testdir\\testdir2\\subdir", NULL);
}

/* cleans after tests */
static void Cleanup(void)
{
    DeleteFileA(".\\testdir\\test1.txt");
    DeleteFileA(".\\testdir\\test2.txt");
    DeleteFileA(".\\testdir\\test3.txt");
    RemoveDirectoryA(".\\testdir\\test.txt");
    RemoveDirectoryA(".\\testdir\\testdir2\\subdir");
    RemoveDirectoryA(".\\testdir\\testdir2");
    RemoveDirectoryA(".\\testdir");
}


/* perform test */
static void test_EnumObjects(IShellFolder *iFolder)
{
    IEnumIDList *iEnumList;
    LPITEMIDLIST newPIDL, idlArr[10];
    ULONG NumPIDLs;
    int i=0, j;
    HRESULT hr;

    static const WORD iResults [5][5] =
    {
	{ 0,-1,-1,-1,-1},
	{ 1, 0,-1,-1,-1},
	{ 1, 1, 0,-1,-1},
	{ 1, 1, 1, 0,-1},
	{ 1, 1, 1, 1, 0}
    };

#define SFGAO_testfor SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_CAPABILITYMASK
    /* Don't test for SFGAO_HASSUBFOLDER since we return real state and native cached */
    static const ULONG attrs[5] =
    {
        SFGAO_DROPTARGET | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR,
        SFGAO_DROPTARGET | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR,
        SFGAO_DROPTARGET | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_FILESYSTEM,
        SFGAO_DROPTARGET | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_FILESYSTEM,
        SFGAO_DROPTARGET | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_FILESYSTEM,
    };
    static const ULONG full_attrs[5] =
    {
        SFGAO_CAPABILITYMASK | SFGAO_STORAGE | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR,
        SFGAO_CAPABILITYMASK | SFGAO_STORAGE | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR,
        SFGAO_CAPABILITYMASK | SFGAO_STREAM | SFGAO_FILESYSTEM,
        SFGAO_CAPABILITYMASK | SFGAO_STREAM | SFGAO_FILESYSTEM,
        SFGAO_CAPABILITYMASK | SFGAO_STREAM | SFGAO_FILESYSTEM,
    };

    hr = IShellFolder_EnumObjects(iFolder, NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &iEnumList);
    ok(hr == S_OK, "EnumObjects failed %08lx\n", hr);

    /* This is to show that, contrary to what is said on MSDN, on IEnumIDList::Next,
     * the filesystem shellfolders return S_OK even if less than 'celt' items are
     * returned (in contrast to S_FALSE). We have to do it in a loop since WinXP
     * only ever returns a single entry per call. */
    while (IEnumIDList_Next(iEnumList, 10-i, &idlArr[i], &NumPIDLs) == S_OK) 
        i += NumPIDLs;
    ok (i == 5, "i: %d\n", i);

    hr = IEnumIDList_Release(iEnumList);
    ok(hr == S_OK, "IEnumIDList_Release failed %08lx\n", hr);
    
    /* Sort them first in case of wrong order from system */
    for (i=0;i<5;i++) for (j=0;j<5;j++)
        if ((SHORT)IShellFolder_CompareIDs(iFolder, 0, idlArr[i], idlArr[j]) < 0)
	{
            newPIDL = idlArr[i];
            idlArr[i] = idlArr[j];
            idlArr[j] = newPIDL;
        }
	    
    for (i=0;i<5;i++) for (j=0;j<5;j++)
    {
        hr = IShellFolder_CompareIDs(iFolder, 0, idlArr[i], idlArr[j]);
        ok(hr == iResults[i][j], "Got %lx expected [%d]-[%d]=%x\n", hr, i, j, iResults[i][j]);
    }

    for (i = 0; i < ARRAY_SIZE(attrs); ++i)
    {
        SFGAOF flags;

        flags = SFGAO_CANCOPY;
        hr = IShellFolder_GetAttributesOf(iFolder, 1, (LPCITEMIDLIST*)(idlArr + i), &flags);
        flags &= SFGAO_testfor;
        ok(hr == S_OK, "Failed to get item attributes, hr %#lx.\n", hr);
        ok((flags & attrs[i]) == attrs[i], "%i: unexpected attributes got %#lx, expected %#lx.\n", i, flags, attrs[i]);

        flags = SFGAO_testfor;
        hr = IShellFolder_GetAttributesOf(iFolder, 1, (LPCITEMIDLIST*)(idlArr + i), &flags);
        flags &= SFGAO_testfor;
        ok(hr == S_OK, "Failed to get item attributes, hr %#lx.\n", hr);
        ok(flags == (attrs[i] | SFGAO_CAPABILITYMASK), "%i: unexpected attributes got %#lx, expected %#lx.\n",
                i, flags, attrs[i]);

        flags = ~0u;
        hr = IShellFolder_GetAttributesOf(iFolder, 1, (LPCITEMIDLIST*)(idlArr + i), &flags);
        ok(hr == S_OK, "Failed to get item attributes, hr %#lx.\n", hr);
        ok((flags & ~(SFGAO_HASSUBFOLDER|SFGAO_COMPRESSED)) == full_attrs[i], "%d: unexpected attributes %#lx, expected %#lx\n",
                i, flags, full_attrs[i]);
    }

    for (i=0;i<5;i++)
        ILFree(idlArr[i]);
}

static void test_BindToObject(void)
{
    HRESULT hr;
    UINT cChars;
    IShellFolder *psfDesktop, *psfChild, *psfMyComputer, *psfSystemDir;
    SHITEMID emptyitem = { 0, { 0 } };
    LPITEMIDLIST pidlMyComputer, pidlSystemDir, pidl, pidlEmpty = (LPITEMIDLIST)&emptyitem;
    WCHAR wszSystemDir[MAX_PATH];
    char szSystemDir[MAX_PATH];
    char buf[MAX_PATH];
    WCHAR path[MAX_PATH];
    CHAR pathA[MAX_PATH];
    HANDLE hfile;
    WCHAR wszMyComputer[] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
    static const CHAR filename_html[] = "winetest.html";
    static const CHAR filename_txt[] = "winetest.txt";
    static const CHAR filename_foo[] = "winetest.foo";

    /* The following tests shows that BindToObject should fail with E_INVALIDARG if called
     * with an empty pidl. This is tested for Desktop, MyComputer and the FS ShellFolder
     */
    hr = SHGetDesktopFolder(&psfDesktop);
    ok (hr == S_OK, "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;
    
    hr = IShellFolder_BindToObject(psfDesktop, pidlEmpty, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, "Desktop's BindToObject should fail, when called with empty pidl! hr = %08lx\n", hr);

    hr = IShellFolder_BindToObject(psfDesktop, NULL, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, "Desktop's BindToObject should fail, when called with NULL pidl! hr = %08lx\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyComputer, NULL, &pidlMyComputer, NULL);
    ok (hr == S_OK, "Desktop's ParseDisplayName failed to parse MyComputer's CLSID! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        return;
    }
    
    hr = IShellFolder_BindToObject(psfDesktop, pidlMyComputer, NULL, &IID_IShellFolder, (LPVOID*)&psfMyComputer);
    ok (hr == S_OK, "Desktop failed to bind to MyComputer object! hr = %08lx\n", hr);
    IShellFolder_Release(psfDesktop);
    ILFree(pidlMyComputer);
    if (hr != S_OK) return;

    hr = IShellFolder_BindToObject(psfMyComputer, pidlEmpty, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, "MyComputers's BindToObject should fail, when called with empty pidl! hr = %08lx\n", hr);

    hr = IShellFolder_BindToObject(psfMyComputer, NULL, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, "MyComputers's BindToObject should fail, when called with NULL pidl! hr = %08lx\n", hr);

    cChars = GetSystemDirectoryA(szSystemDir, MAX_PATH);
    ok (cChars > 0 && cChars < MAX_PATH, "GetSystemDirectoryA failed! LastError: %lu\n", GetLastError());
    if (cChars == 0 || cChars >= MAX_PATH) {
        IShellFolder_Release(psfMyComputer);
        return;
    }
    MultiByteToWideChar(CP_ACP, 0, szSystemDir, -1, wszSystemDir, MAX_PATH);
    
    hr = IShellFolder_ParseDisplayName(psfMyComputer, NULL, NULL, wszSystemDir, NULL, &pidlSystemDir, NULL);
    ok (hr == S_OK, "MyComputers's ParseDisplayName failed to parse the SystemDirectory! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfMyComputer);
        return;
    }

    hr = IShellFolder_BindToObject(psfMyComputer, pidlSystemDir, NULL, &IID_IShellFolder, (LPVOID*)&psfSystemDir);
    ok (hr == S_OK, "MyComputer failed to bind to a FileSystem ShellFolder! hr = %08lx\n", hr);
    IShellFolder_Release(psfMyComputer);
    ILFree(pidlSystemDir);
    if (hr != S_OK) return;

    hr = IShellFolder_BindToObject(psfSystemDir, pidlEmpty, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, 
        "FileSystem ShellFolder's BindToObject should fail, when called with empty pidl! hr = %08lx\n", hr);

    hr = IShellFolder_BindToObject(psfSystemDir, NULL, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG,
        "FileSystem ShellFolder's BindToObject should fail, when called with NULL pidl! hr = %08lx\n", hr);

    IShellFolder_Release(psfSystemDir);

    cChars = GetCurrentDirectoryA(MAX_PATH, buf);
    if(!cChars)
    {
        skip("Failed to get current directory, skipping tests.\n");
        return;
    }
    if(buf[cChars-1] != '\\') lstrcatA(buf, "\\");

    SHGetDesktopFolder(&psfDesktop);

    /* Attempt BindToObject on files. */

    /* .html */
    lstrcpyA(pathA, buf);
    lstrcatA(pathA, filename_html);
    hfile = CreateFileA(pathA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
        MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);
        hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, path, NULL, &pidl, NULL);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (void**)&psfChild);
            ok(hr == S_OK ||
               hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP, W2K3 */
               "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                IPersist *pp;
                hr = IShellFolder_QueryInterface(psfChild, &IID_IPersist, (void**)&pp);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                if(SUCCEEDED(hr))
                {
                    CLSID id;
                    hr = IPersist_GetClassID(pp, &id);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(IsEqualIID(&id, &CLSID_ShellDocObjView), "Unexpected classid %s\n", wine_dbgstr_guid(&id));
                    IPersist_Release(pp);
                }

                IShellFolder_Release(psfChild);
            }
            ILFree(pidl);
        }
        DeleteFileA(pathA);
    }
    else
        win_skip("Failed to create .html testfile.\n");

    /* .txt */
    lstrcpyA(pathA, buf);
    lstrcatA(pathA, filename_txt);
    hfile = CreateFileA(pathA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
        MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);
        hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, path, NULL, &pidl, NULL);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (void**)&psfChild);
            ok(hr == E_FAIL || /* Vista+ */
               hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP, W2K3 */
               "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IShellFolder_Release(psfChild);
            ILFree(pidl);
        }
        DeleteFileA(pathA);
    }
    else
        win_skip("Failed to create .txt testfile.\n");

    /* .foo */
    lstrcpyA(pathA, buf);
    lstrcatA(pathA, filename_foo);
    hfile = CreateFileA(pathA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
        MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);
        hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, path, NULL, &pidl, NULL);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (void**)&psfChild);
            ok(hr == E_FAIL || /* Vista+ */
               hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP, W2K3 */
               "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IShellFolder_Release(psfChild);
            ILFree(pidl);
        }
        DeleteFileA(pathA);
    }
    else
        win_skip("Failed to create .foo testfile.\n");

    /* And on the desktop */
    SHGetSpecialFolderPathA(NULL, pathA, CSIDL_DESKTOP, FALSE);
    lstrcatA(pathA, "\\");
    lstrcatA(pathA, filename_html);
    hfile = CreateFileA(pathA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    CloseHandle(hfile);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);
    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, path, NULL, &pidl, NULL);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (void **)&psfChild);
    ok(hr == S_OK ||
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP, W2K3 */
       "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) IShellFolder_Release(psfChild);
    ILFree(pidl);
    if(!DeleteFileA(pathA))
        trace("Failed to delete: %ld\n", GetLastError());

    SHGetSpecialFolderPathA(NULL, pathA, CSIDL_DESKTOP, FALSE);
    lstrcatA(pathA, "\\");
    lstrcatA(pathA, filename_foo);
    hfile = CreateFileA(pathA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    CloseHandle(hfile);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);
    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, path, NULL, &pidl, NULL);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (void **)&psfChild);
    ok(hr == E_FAIL || /* Vista+ */
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP, W2K3 */
       "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) IShellFolder_Release(psfChild);
    ILFree(pidl);
    DeleteFileA(pathA);

    IShellFolder_Release(psfDesktop);
}

static void test_GetDisplayName(void)
{
    BOOL result;
    HRESULT hr;
    HANDLE hTestFile;
    WCHAR wszTestFile[MAX_PATH], wszTestFile2[MAX_PATH];
    char szTestFile[MAX_PATH], szTestDir[MAX_PATH];
    DWORD attr;
    STRRET strret;
    LPSHELLFOLDER psfDesktop, psfPersonal;
    IUnknown *psfFile;
    SHITEMID emptyitem = { 0, { 0 } };
    LPITEMIDLIST pidlTestFile, pidlEmpty = (LPITEMIDLIST)&emptyitem;
    LPCITEMIDLIST pidlLast;

    /* It's ok to use this fixed path. Call will fail anyway. */
    WCHAR wszAbsoluteFilename[] = L"C:\\winetest";
    LPITEMIDLIST pidlNew;

    /* I'm trying to figure if there is a functional difference between calling
     * SHGetPathFromIDListW and calling GetDisplayNameOf(SHGDN_FORPARSING) after
     * binding to the shellfolder. One thing I thought of was that perhaps 
     * SHGetPathFromIDListW would be able to get the path to a file, which does
     * not exist anymore, while the other method wouldn't. It turns out there's
     * no functional difference in this respect.
     */

    /* First creating a directory in MyDocuments and a file in this directory. */
    result = SHGetSpecialFolderPathA(NULL, szTestDir, CSIDL_PERSONAL, FALSE);
    ok(result, "SHGetSpecialFolderPathA failed! Last error: %lu\n", GetLastError());
    if (!result) return;

    /* Use ANSI file functions so this works on Windows 9x */
    lstrcatA(szTestDir, "\\winetest");
    CreateDirectoryA(szTestDir, NULL);
    attr=GetFileAttributesA(szTestDir);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        ok(0, "unable to create the '%s' directory\n", szTestDir);
        return;
    }

    lstrcpyA(szTestFile, szTestDir);
    lstrcatA(szTestFile, "\\winetest.foo");
    hTestFile = CreateFileA(szTestFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok((hTestFile != INVALID_HANDLE_VALUE), "CreateFileA failed! Last error: %lu\n", GetLastError());
    if (hTestFile == INVALID_HANDLE_VALUE) return;
    CloseHandle(hTestFile);

    /* Getting an itemidlist for the file. */
    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    MultiByteToWideChar(CP_ACP, 0, szTestFile, -1, wszTestFile, MAX_PATH);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszTestFile, NULL, &pidlTestFile, NULL);
    ok(hr == S_OK, "Desktop->ParseDisplayName failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    pidlLast = ILFindLastID(pidlTestFile);
    ok(pidlLast->mkid.cb >= 76, "Expected pidl length of at least 76, got %d.\n", pidlLast->mkid.cb);
    if (pidlLast->mkid.cb >= 28) {
        ok(!lstrcmpA((char*)&pidlLast->mkid.abID[12], "winetest.foo"),
            "Filename should be stored as ansi-string at this position!\n");
    }
    /* WinXP and up store the filenames as both ANSI and UNICODE in the pidls */
    if (pidlLast->mkid.cb >= 76) {
        ok(!lstrcmpW((WCHAR*)&pidlLast->mkid.abID[46], L"winetest.foo") ||
            (pidlLast->mkid.cb >= 94 && !lstrcmpW((WCHAR*)&pidlLast->mkid.abID[64], L"winetest.foo")) ||  /* Vista */
            (pidlLast->mkid.cb >= 98 && !lstrcmpW((WCHAR*)&pidlLast->mkid.abID[68], L"winetest.foo")) ||  /* Win7 */
            (pidlLast->mkid.cb >= 102 && !lstrcmpW((WCHAR*)&pidlLast->mkid.abID[72], L"winetest.foo")),   /* Win8 */
            "Filename should be stored as wchar-string at this position!\n");
    }
    
    /* It seems as if we cannot bind to regular files on windows, but only directories. 
     */
    hr = IShellFolder_BindToObject(psfDesktop, pidlTestFile, NULL, &IID_IUnknown, (VOID**)&psfFile);
    ok (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
        hr == E_NOTIMPL, /* Vista */
        "hr = %08lx\n", hr);
    if (hr == S_OK) {
        IUnknown_Release(psfFile);
    }

    /* Some tests for IShellFolder::SetNameOf */
    hr = SHBindToParent(pidlTestFile, &IID_IShellFolder, (void **)&psfPersonal, &pidlLast);
    ok(hr == S_OK, "SHBindToParent failed! hr = %08lx\n", hr);

    /* The pidl returned through the last parameter of SetNameOf is a simple one. */
    hr = IShellFolder_SetNameOf(psfPersonal, NULL, pidlLast, L"winetest", SHGDN_NORMAL, &pidlNew);
    ok (hr == S_OK, "SetNameOf failed! hr = %08lx\n", hr);

    ok (((ITEMIDLIST *)((BYTE *)pidlNew + pidlNew->mkid.cb))->mkid.cb == 0,
        "pidl returned from SetNameOf should be simple!\n");

    /* Passing an absolute path to SetNameOf fails. The HRESULT code indicates that SetNameOf
     * is implemented on top of SHFileOperation in WinXP. */
    hr = IShellFolder_SetNameOf(psfPersonal, NULL, pidlNew, wszAbsoluteFilename, SHGDN_FORPARSING, NULL);
    ok (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED), "SetNameOf succeeded! hr = %08lx\n", hr);

    /* Rename the file back to its original name. SetNameOf ignores the fact, that the
     * SHGDN flags specify an absolute path. */
    hr = IShellFolder_SetNameOf(psfPersonal, NULL, pidlNew, L"winetest.foo", SHGDN_FORPARSING, NULL);
    ok (hr == S_OK, "SetNameOf failed! hr = %08lx\n", hr);

    ILFree(pidlNew);
    IShellFolder_Release(psfPersonal);

    /* Deleting the file and the directory */
    DeleteFileA(szTestFile);
    RemoveDirectoryA(szTestDir);

    /* SHGetPathFromIDListW still works, although the file is not present anymore. */
    result = SHGetPathFromIDListW(pidlTestFile, wszTestFile2);
    ok (result, "SHGetPathFromIDListW failed! Last error: %lu\n", GetLastError());
    ok (!lstrcmpiW(wszTestFile, wszTestFile2), "SHGetPathFromIDListW returns incorrect path!\n");

    /* SHBindToParent fails, if called with a NULL PIDL. */
    pidlLast = (LPITEMIDLIST)0xdeadbeef;
    hr = SHBindToParent(NULL, &IID_IShellFolder, (void **)&psfPersonal, &pidlLast);
    ok (hr == E_INVALIDARG || broken(hr == E_OUTOFMEMORY) /* XP */,
        "SHBindToParent(NULL) should fail! hr = %08lx\n", hr);
    ok(pidlLast == NULL, "got %p\n", pidlLast);

    /* But it succeeds with an empty PIDL. */
    hr = SHBindToParent(pidlEmpty, &IID_IShellFolder, (void **)&psfPersonal, &pidlLast);
    ok (hr == S_OK, "SHBindToParent(empty PIDL) should succeed! hr = %08lx\n", hr);
    ok (pidlLast == pidlEmpty, "The last element of an empty PIDL should be the PIDL itself!\n");
    if (hr == S_OK)
        IShellFolder_Release(psfPersonal);

    /* Binding to the folder and querying the display name of the file also works. */
    hr = SHBindToParent(pidlTestFile, &IID_IShellFolder, (void **)&psfPersonal, &pidlLast);
    ok (hr == S_OK, "SHBindToParent failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    /* This test shows that Windows doesn't allocate a new pidlLast, but returns a pointer into
     * pidlTestFile (In accordance with MSDN). */
    ok (ILFindLastID(pidlTestFile) == pidlLast,
                                "SHBindToParent doesn't return the last id of the pidl param!\n");

    hr = IShellFolder_GetDisplayNameOf(psfPersonal, pidlLast, SHGDN_FORPARSING, &strret);
    ok (hr == S_OK, "Personal->GetDisplayNameOf failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        IShellFolder_Release(psfPersonal);
        return;
    }

    hr = StrRetToBufW(&strret, pidlLast, wszTestFile2, MAX_PATH);
    ok (hr == S_OK, "StrRetToBufW failed! hr = %08lx\n", hr);
    ok (!lstrcmpiW(wszTestFile, wszTestFile2), "GetDisplayNameOf returns incorrect path!\n");

    ILFree(pidlTestFile);
    IShellFolder_Release(psfDesktop);
    IShellFolder_Release(psfPersonal);
}

static void test_CallForAttributes(void)
{
    HKEY hKey;
    LONG lResult;
    HRESULT hr;
    DWORD dwSize;
    LPSHELLFOLDER psfDesktop;
    LPITEMIDLIST pidlMyDocuments;
    DWORD dwAttributes, dwCallForAttributes, dwOrigAttributes, dwOrigCallForAttributes;
    static const WCHAR wszMyDocumentsKey[] = L"CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\ShellFolder";
    WCHAR wszMyDocuments[] = L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}";

    /* For the root of a namespace extension, the attributes are not queried by binding
     * to the object and calling GetAttributesOf. Instead, the attributes are read from 
     * the registry value HKCR/CLSID/{...}/ShellFolder/Attributes. This is documented on MSDN.
     *
     * The MyDocuments shellfolder on WinXP has a HKCR/CLSID/{...}/ShellFolder/CallForAttributes
     * value. It seems that if the folder is queried for one of the flags set in CallForAttributes,
     * the shell does bind to the folder object and calls GetAttributesOf. This is not documented
     * on MSDN. This test is meant to document the observed behaviour on WinXP SP2.
     */
    hr = SHGetDesktopFolder(&psfDesktop);
    ok (hr == S_OK, "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyDocuments, NULL,
                                       &pidlMyDocuments, NULL);
    ok (hr == S_OK,
        "Desktop's ParseDisplayName failed to parse MyDocuments's CLSID! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    dwAttributes = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 1, 
                                      (LPCITEMIDLIST*)&pidlMyDocuments, &dwAttributes);
    ok (hr == S_OK, "Desktop->GetAttributesOf(MyDocuments) failed! hr = %08lx\n", hr);

    /* We need the following setup (as observed on WinXP SP2), for the tests to make sense. */
    ok (dwAttributes & SFGAO_FILESYSTEM, "SFGAO_FILESYSTEM attribute is not set for MyDocuments!\n");
    ok (!(dwAttributes & SFGAO_ISSLOW), "SFGAO_ISSLOW attribute is set for MyDocuments!\n");
    ok (!(dwAttributes & SFGAO_GHOSTED), "SFGAO_GHOSTED attribute is set for MyDocuments!\n");

    /* We don't have the MyDocuments shellfolder in wine yet, and thus we don't have the registry
     * key. So the test will return at this point, if run on wine. 
     */
    lResult = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszMyDocumentsKey, 0, KEY_WRITE|KEY_READ, &hKey);
    ok (lResult == ERROR_SUCCESS ||
        lResult == ERROR_ACCESS_DENIED,
        "RegOpenKeyEx failed! result: %08lx\n", lResult);
    if (lResult != ERROR_SUCCESS) {
        if (lResult == ERROR_ACCESS_DENIED)
            skip("Not enough rights to open the registry key\n");
        ILFree(pidlMyDocuments);
        IShellFolder_Release(psfDesktop);
        return;
    }
    
    /* Query MyDocuments' Attributes value, to be able to restore it later. */
    dwSize = sizeof(DWORD);
    lResult = RegQueryValueExW(hKey, L"Attributes", NULL, NULL, (BYTE*)&dwOrigAttributes, &dwSize);
    ok (lResult == ERROR_SUCCESS, "RegQueryValueEx failed! result: %08lx\n", lResult);
    if (lResult != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        ILFree(pidlMyDocuments);
        IShellFolder_Release(psfDesktop);
        return;
    }

    /* Query MyDocuments' CallForAttributes value, to be able to restore it later. */
    dwSize = sizeof(DWORD);
    lResult = RegQueryValueExW(hKey, L"CallForAttributes", NULL, NULL,
                               (BYTE*)&dwOrigCallForAttributes, &dwSize);
    ok (lResult == ERROR_SUCCESS, "RegQueryValueEx failed! result: %08lx\n", lResult);
    if (lResult != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        ILFree(pidlMyDocuments);
        IShellFolder_Release(psfDesktop);
        return;
    }
    
    /* Define via the Attributes value that MyDocuments attributes are SFGAO_ISSLOW and 
     * SFGAO_GHOSTED and that MyDocuments should be called for the SFGAO_ISSLOW and
     * SFGAO_FILESYSTEM attributes. */
    dwAttributes = SFGAO_ISSLOW|SFGAO_GHOSTED;
    RegSetValueExW(hKey, L"Attributes", 0, REG_DWORD, (BYTE*)&dwAttributes, sizeof(DWORD));
    dwCallForAttributes = SFGAO_ISSLOW|SFGAO_FILESYSTEM;
    RegSetValueExW(hKey, L"CallForAttributes", 0, REG_DWORD,
                   (BYTE*)&dwCallForAttributes, sizeof(DWORD));

    /* Although it is not set in CallForAttributes, the SFGAO_GHOSTED flag is reset by 
     * GetAttributesOf. It seems that once there is a single attribute queried, for which
     * CallForAttributes is set, all flags are taken from the GetAttributesOf call and
     * the flags in Attributes are ignored. 
     */
    dwAttributes = SFGAO_ISSLOW|SFGAO_GHOSTED|SFGAO_FILESYSTEM;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 1, 
                                      (LPCITEMIDLIST*)&pidlMyDocuments, &dwAttributes);
    ok (hr == S_OK, "Desktop->GetAttributesOf(MyDocuments) failed! hr = %08lx\n", hr);
    if (hr == S_OK)
        ok (dwAttributes == SFGAO_FILESYSTEM, 
            "Desktop->GetAttributes(MyDocuments) returned unexpected attributes: %08lx\n",
            dwAttributes);

    /* Restore MyDocuments' original Attributes and CallForAttributes registry values */
    RegSetValueExW(hKey, L"Attributes", 0, REG_DWORD, (BYTE*)&dwOrigAttributes, sizeof(DWORD));
    RegSetValueExW(hKey, L"CallForAttributes", 0, REG_DWORD,
                   (BYTE*)&dwOrigCallForAttributes, sizeof(DWORD));
    RegCloseKey(hKey);
    ILFree(pidlMyDocuments);
    IShellFolder_Release(psfDesktop);
}

static void test_GetAttributesOf(void) 
{
    HRESULT hr;
    LPSHELLFOLDER psfDesktop, psfMyComputer;
    SHITEMID emptyitem = { 0, { 0 } };
    LPCITEMIDLIST pidlEmpty = (LPCITEMIDLIST)&emptyitem;
    LPITEMIDLIST pidlMyComputer;
    DWORD dwFlags;
    static const DWORD desktopFlags = SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
    static const DWORD myComputerFlags = SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET |
        SFGAO_DROPTARGET | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
    WCHAR wszMyComputer[] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
    WCHAR temp_dir[MAX_PATH], cwd[MAX_PATH], path[MAX_PATH];
    IShellFolder *IDesktopFolder, *testIShellFolder;
    ITEMIDLIST *newPIDL, *pidls[2], *abs_pidl;
    IEnumIDList *list;
    ULONG fetch;
    BOOL ret;

    static const DWORD testdir_flags = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK | SFGAO_STORAGE
            | SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_STORAGEANCESTOR
            | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;

    static const DWORD testdir_abs_flags = SFGAO_CANLINK | SFGAO_CANRENAME | SFGAO_CANDELETE
            | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER;

    static const DWORD testdir_multi_flags = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK
            | SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_FILESYSTEM;

    GetCurrentDirectoryW(ARRAY_SIZE(cwd), cwd);
    GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    SetCurrentDirectoryW(temp_dir);

    hr = SHGetDesktopFolder(&psfDesktop);
    ok (hr == S_OK, "SHGetDesktopFolder failed! hr = %08lx\n", hr);

    /* The Desktop attributes can be queried with a single empty itemidlist, .. */
    dwFlags = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 1, &pidlEmpty, &dwFlags);
    ok (hr == S_OK, "Desktop->GetAttributesOf(empty pidl) failed! hr = %08lx\n", hr);
    ok (dwFlags == desktopFlags, "Wrong Desktop attributes: %08lx\n", dwFlags);

    /* .. or with no itemidlist at all. */
    dwFlags = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 0, NULL, &dwFlags);
    ok (hr == S_OK, "Desktop->GetAttributesOf(NULL) failed! hr = %08lx\n", hr);
    ok (dwFlags == desktopFlags, "Wrong Desktop attributes: %08lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 0, NULL, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!dwFlags, "got flags %#lx\n", dwFlags);

    dwFlags = SFGAO_FOLDER;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 0, NULL, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(dwFlags == SFGAO_FOLDER, "got flags %#lx\n", dwFlags);

    /* Testing the attributes of the MyComputer shellfolder */
    dwFlags = ~0u;
    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyComputer, NULL, &pidlMyComputer, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(dwFlags == (myComputerFlags | SFGAO_CANLINK), "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyComputer, NULL, &pidlMyComputer, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!dwFlags, "got flags %#lx\n", dwFlags);

    for (unsigned int i = 0; i < 32; ++i)
    {
        dwFlags = (1u << i);
        hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyComputer, NULL, &pidlMyComputer, &dwFlags);
        ok(hr == S_OK, "got %#lx\n", hr);
        todo_wine ok(dwFlags == (myComputerFlags | SFGAO_CANLINK), "got flags %#lx\n", dwFlags);
    }

    /* Windows sets the SFGAO_CANLINK flag, when MyComputer is queried via the Desktop
     * folder object. It doesn't do this, if MyComputer is queried directly (see below).
     */
    dwFlags = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 1, (LPCITEMIDLIST*)&pidlMyComputer, &dwFlags);
    ok (hr == S_OK, "Desktop->GetAttributesOf(MyComputer) failed! hr = %08lx\n", hr);
    todo_wine
    ok (dwFlags == (myComputerFlags | SFGAO_CANLINK), "Wrong MyComputer attributes: %08lx\n", dwFlags);

    hr = IShellFolder_BindToObject(psfDesktop, pidlMyComputer, NULL, &IID_IShellFolder, (LPVOID*)&psfMyComputer);
    ok (hr == S_OK, "Desktop failed to bind to MyComputer object! hr = %08lx\n", hr);
    IShellFolder_Release(psfDesktop);
    ILFree(pidlMyComputer);

    hr = IShellFolder_GetAttributesOf(psfMyComputer, 1, &pidlEmpty, &dwFlags);
    todo_wine
    ok (hr == E_INVALIDARG, "MyComputer->GetAttributesOf(empty pidl) should fail! hr = %08lx\n", hr);

    dwFlags = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfMyComputer, 0, NULL, &dwFlags);
    ok (hr == S_OK, "MyComputer->GetAttributesOf(NULL) failed! hr = %08lx\n", hr);
    todo_wine
    ok (dwFlags == myComputerFlags, "Wrong MyComputer attributes: %08lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_GetAttributesOf(psfMyComputer, 0, NULL, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!dwFlags, "got flags %#lx\n", dwFlags);

    dwFlags = SFGAO_FOLDER;
    hr = IShellFolder_GetAttributesOf(psfMyComputer, 0, NULL, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(dwFlags == SFGAO_FOLDER, "got flags %#lx\n", dwFlags);

    IShellFolder_Release(psfMyComputer);

    ret = CreateDirectoryW(L"winetestdir", NULL);
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = CreateDirectoryW(L"winetestdir\\subdir", NULL);
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = CreateDirectoryW(L"winetestdir2", NULL);
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = CreateDirectoryW(L"winetestdir2\\subdir", NULL);
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = SetFileAttributesW(L"winetestdir2\\subdir", FILE_ATTRIBUTE_HIDDEN);
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    CreateTestFile("winetestdir2\\file");

    hr = SHGetDesktopFolder(&IDesktopFolder);
    ok(hr == S_OK, "SHGetDesktopfolder failed %08lx\n", hr);

    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, temp_dir, NULL, &newPIDL, NULL);
    ok(hr == S_OK, "ParseDisplayName failed %08lx\n", hr);

    hr = IShellFolder_BindToObject(IDesktopFolder, newPIDL, NULL, (REFIID)&IID_IShellFolder, (LPVOID *)&testIShellFolder);
    ok(hr == S_OK, "BindToObject failed %08lx\n", hr);

    ILFree(newPIDL);

    wcscpy(path, L"winetestdir");

    /* get relative PIDL */
    dwFlags = ~0u;
    hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, path, NULL, &newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(dwFlags == testdir_flags, "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, path, NULL, &newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!dwFlags, "got flags %#lx\n", dwFlags);

    /* test the shell attributes of the test directory using the relative PIDL */

    dwFlags = ~0u;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(dwFlags == testdir_flags, "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(!dwFlags, "got flags %#lx\n", dwFlags);

    for (unsigned int i = 0; i < 32; ++i)
    {
        static const DWORD set_flags = (SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_STORAGE | SFGAO_DROPTARGET
                | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM);
        DWORD input = (1u << i);
        DWORD expect = 0;

        if ((testdir_flags | SFGAO_LINK | SFGAO_READONLY | SFGAO_STREAM) & input)
        {
            expect = (set_flags | input) & testdir_flags;
            if (input == SFGAO_CANRENAME)
                expect |= SFGAO_HASPROPSHEET;
            if (input & (SFGAO_CANDELETE | SFGAO_CANMOVE))
                expect |= SFGAO_CANDELETE | SFGAO_CANMOVE;
        }

        dwFlags = input;
        hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, path, NULL, &newPIDL, &dwFlags);
        ok(hr == S_OK, "got %#lx\n", hr);
        todo_wine ok(dwFlags == expect, "got flags %#lx for input %#lx\n", dwFlags, input);

        dwFlags = input;
        hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(dwFlags == expect, "got flags %#lx for input %#lx\n", dwFlags, input);
    }

    /* Test an array. */

    hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, path, NULL, &pidls[0], &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    wcscpy(path, L"winetestdir2");
    hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, path, NULL, &pidls[1], &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    dwFlags = ~0u;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&pidls[1], &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(dwFlags == testdir_flags, "got flags %#lx\n", dwFlags);

    /* This clears a bunch of flags, for some reason, even though both folders
     * have them in common. */
    dwFlags = ~0u;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 2, (LPCITEMIDLIST *)pidls, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(dwFlags == testdir_multi_flags, "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 2, (LPCITEMIDLIST *)pidls, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(dwFlags == (SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_FILESYSTEM),
            "got flags %#lx\n", dwFlags);

    for (unsigned int i = 0; i < 32; ++i)
    {
        DWORD expect = SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_FILESYSTEM;
        DWORD input = (1u << i);

        if ((SFGAO_CANMOVE | SFGAO_CANRENAME | SFGAO_CANDELETE) & input)
            expect |= SFGAO_CANMOVE | SFGAO_CANRENAME | SFGAO_CANDELETE;

        dwFlags = input;
        hr = IShellFolder_GetAttributesOf(testIShellFolder, 2, (LPCITEMIDLIST *)pidls, &dwFlags);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(dwFlags == expect, "got flags %#lx for input %#lx\n", dwFlags, input);
    }

    ret = RemoveDirectoryW(L"winetestdir2\\subdir");
    ok(ret == TRUE, "got error %lu\n", GetLastError());

    dwFlags = ~0u;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&pidls[1], &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(dwFlags == (testdir_flags & ~SFGAO_HASSUBFOLDER), "got flags %#lx\n", dwFlags);

    /* Test an absolute PIDL. Results for ParseDisplayName() are the same as
     * the relative PIDL, but for GetAttributesOf() they are different. */

    swprintf(path, ARRAY_SIZE(path), L"%swinetestdir\\", temp_dir);

    dwFlags = ~0u;
    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, path, NULL, &abs_pidl, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(dwFlags == testdir_flags, "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, path, NULL, &abs_pidl, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!dwFlags, "got flags %#lx\n", dwFlags);

    dwFlags = SFGAO_FOLDER;
    hr = IShellFolder_GetAttributesOf(IDesktopFolder, 1, (LPCITEMIDLIST *)&abs_pidl, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(dwFlags == SFGAO_FOLDER, "got flags %#lx\n", dwFlags);

    dwFlags = ~0u;
    hr = IShellFolder_GetAttributesOf(IDesktopFolder, 1, (LPCITEMIDLIST *)&abs_pidl, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(dwFlags == testdir_abs_flags, "got flags %#lx\n", dwFlags);

    for (unsigned int i = 0; i < 32; ++i)
    {
        DWORD expect;

        expect = (1u << i);
        hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &expect);
        ok(hr == S_OK, "got hr %#lx\n", hr);

        dwFlags = (1u << i);
        hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, path, NULL, &abs_pidl, &dwFlags);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(dwFlags == expect, "expected flags %#lx for input %#x, got %#lx\n", expect, (1u << i), dwFlags);

        dwFlags = (1u << i);
        hr = IShellFolder_GetAttributesOf(IDesktopFolder, 1, (LPCITEMIDLIST *)&abs_pidl, &dwFlags);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine_if ((1u << i) == SFGAO_CANLINK)
            ok(dwFlags == (testdir_abs_flags & (1u << i)), "got flags %#lx\n", dwFlags);
    }

    ILFree(newPIDL);

    IShellFolder_Release(testIShellFolder);

    ret = DeleteFileW(L"winetestdir2\\file");
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = RemoveDirectoryW(L"winetestdir2");
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = RemoveDirectoryW(L"winetestdir\\subdir");
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    ret = RemoveDirectoryW(L"winetestdir");
    ok(ret == TRUE, "got error %lu\n", GetLastError());

    /* test Control Panel elements */

    dwFlags = ~0u;
    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL,
            (WCHAR *)L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}", NULL, &newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(dwFlags == (SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER), "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL,
            (WCHAR *)L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}", NULL, &newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!dwFlags, "got flags %#lx\n", dwFlags);

    dwFlags = ~0u;
    hr = IShellFolder_GetAttributesOf(IDesktopFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(dwFlags == (SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER), "got flags %#lx\n", dwFlags);

    dwFlags = 0;
    hr = IShellFolder_GetAttributesOf(IDesktopFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!dwFlags, "got flags %#lx\n", dwFlags);

    for (unsigned int i = 0; i < 32; ++i)
    {
        dwFlags = (1u << i);
        hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL,
                (WCHAR *)L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}", NULL, &newPIDL, &dwFlags);
        ok(hr == S_OK, "got %#lx\n", hr);
        todo_wine ok(dwFlags == (SFGAO_CANLINK | ((SFGAO_FOLDER | SFGAO_HASSUBFOLDER) & (1u << i))),
                "got flags %#lx for input %#x\n", dwFlags, 1u << i);

        dwFlags = (1u << i);
        hr = IShellFolder_GetAttributesOf(IDesktopFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine_if ((1u << i) == SFGAO_CANLINK)
            ok(dwFlags == ((SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER) & (1u << i)),
                    "got flags %#lx for input %#x\n", dwFlags, 1u << i);
    }

    hr = IShellFolder_BindToObject(IDesktopFolder, newPIDL, NULL,
            &IID_IShellFolder, (void**)&testIShellFolder);
    ok(hr == S_OK, "BindToObject failed %08lx\n", hr);
    ILFree(newPIDL);

    hr = IShellFolder_EnumObjects(testIShellFolder, NULL, SHCONTF_NONFOLDERS, &list);
    ok(hr == S_OK, "EnumObjects failed %08lx\n", hr);

    while (IEnumIDList_Next(list, 1, &newPIDL, &fetch) == S_OK)
    {
        WCHAR name[256];
        STRRET strret;
        DWORD expect;

        hr = IShellFolder_GetDisplayNameOf(testIShellFolder, newPIDL, SHGDN_FORPARSING, &strret);
        ok(hr == S_OK, "GetDisplayNameOf failed %08lx\n", hr);
        StrRetToBufW(&strret, newPIDL, name, ARRAY_SIZE(name));

        dwFlags = ~0u;
        hr = IShellFolder_GetAttributesOf(testIShellFolder, 1,
                (LPCITEMIDLIST*)&newPIDL, &dwFlags);
        ok(hr == S_OK, "ControlPanel->GetAttributesOf failed %08lx\n", hr);
        ok(dwFlags == SFGAO_CANLINK ||
                broken(!wcsncmp(L"::{26EE0668-A00A-44D7-9371-BEB064C98683}\\", name, 41)
                    && dwFlags == SFGAO_VALIDATE),
                "%s dwFlags = %08lx\n", debugstr_w(name), dwFlags);
        expect = dwFlags;

        dwFlags = 0;
        hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(!dwFlags, "got flags %#lx\n", dwFlags);

        for (unsigned int i = 0; i < 32; ++i)
        {
            dwFlags = (1u << i);
            hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(dwFlags == (expect & (1u << i)), "got flags %#lx for input %#x\n", dwFlags, 1u << i);
        }

        dwFlags = ~0u;
        hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, name, NULL, &newPIDL, &dwFlags);
        todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(dwFlags == expect, "got flags %#lx\n", dwFlags);

        dwFlags = 0;
        hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, name, NULL, &newPIDL, &dwFlags);
        todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(!dwFlags, "got flags %#lx\n", dwFlags);

        for (unsigned int i = 0; i < 32; ++i)
        {
            dwFlags = (1u << i);
            hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, name, NULL, &newPIDL, &dwFlags);
            todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
            todo_wine_if (expect != (1u << i))
                ok(dwFlags == expect, "got flags %#lx for input %#x\n", dwFlags, 1u << i);
        }

        ILFree(newPIDL);
    }
    IEnumIDList_Release(list);

    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL,
            (WCHAR*)L"c:\\", NULL, &newPIDL, 0);
    ok(hr == S_OK, "ParseDisplayName failed %08lx\n", hr);

    dwFlags = 0;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1, (LPCITEMIDLIST *)&newPIDL, &dwFlags);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!dwFlags, "got flags %#lx\n", dwFlags);

    dwFlags = ~0;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1,
            (LPCITEMIDLIST*)&newPIDL, &dwFlags);
    ok(hr == S_OK, "ControlPanel->GetAttributesOf failed %08lx\n", hr);
    ok(dwFlags == SFGAO_VALIDATE, "dwFlags = %08lx\n", dwFlags);

    dwFlags = ~0 & ~SFGAO_VALIDATE;
    hr = IShellFolder_GetAttributesOf(testIShellFolder, 1,
            (LPCITEMIDLIST*)&newPIDL, &dwFlags);
    ok(hr == S_OK, "ControlPanel->GetAttributesOf failed %08lx\n", hr);
    ok(dwFlags == SFGAO_CANLINK, "dwFlags = %08lx\n", dwFlags);

    ILFree(newPIDL);
    IShellFolder_Release(testIShellFolder);
    IShellFolder_Release(IDesktopFolder);

    SetCurrentDirectoryW(cwd);
}

static void test_SHGetPathFromIDList(void)
{
    SHITEMID emptyitem = { 0, { 0 } };
    LPCITEMIDLIST pidlEmpty = (LPCITEMIDLIST)&emptyitem;
    LPITEMIDLIST pidlMyComputer;
    WCHAR wszPath[MAX_PATH], wszDesktop[MAX_PATH];
    BOOL result;
    HRESULT hr;
    LPSHELLFOLDER psfDesktop;
    WCHAR wszMyComputer[] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
    WCHAR wszFileName[MAX_PATH];
    LPITEMIDLIST pidlTestFile;
    HANDLE hTestFile;
    STRRET strret;
    static WCHAR wszTestFile[] = L"winetest.foo";
    LPITEMIDLIST pidlPrograms;

    /* Calling SHGetPathFromIDListW with no pidl should return the empty string */
    wszPath[0] = 'a';
    wszPath[1] = '\0';
    result = SHGetPathFromIDListW(NULL, wszPath);
    ok(!result, "Expected failure\n");
    ok(!wszPath[0], "Expected empty string\n");

    /* Calling SHGetPathFromIDListW with an empty pidl should return the desktop folder's path. */
    result = SHGetSpecialFolderPathW(NULL, wszDesktop, CSIDL_DESKTOP, FALSE);
    ok(result, "SHGetSpecialFolderPathW(CSIDL_DESKTOP) failed! Last error: %lu\n", GetLastError());
    if (!result) return;

    result = SHGetPathFromIDListW(pidlEmpty, wszPath);
    ok(result, "SHGetPathFromIDListW failed! Last error: %lu\n", GetLastError());
    if (!result) return;
    ok(!lstrcmpiW(wszDesktop, wszPath), "SHGetPathFromIDListW didn't return desktop path for empty pidl!\n");

    /* MyComputer does not map to a filesystem path. SHGetPathFromIDListW should fail. */
    hr = SHGetDesktopFolder(&psfDesktop);
    ok (hr == S_OK, "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyComputer, NULL, &pidlMyComputer, NULL);
    ok (hr == S_OK, "Desktop's ParseDisplayName failed to parse MyComputer's CLSID! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    SetLastError(0xdeadbeef);
    wszPath[0] = 'a';
    wszPath[1] = '\0';
    result = SHGetPathFromIDListW(pidlMyComputer, wszPath);
    ok (!result, "SHGetPathFromIDListW succeeded where it shouldn't!\n");
    ok (GetLastError()==0xdeadbeef ||
        GetLastError()==ERROR_SUCCESS, /* Vista and higher */
        "Unexpected last error from SHGetPathFromIDListW: %lu\n", GetLastError());
    ok (!wszPath[0], "Expected empty path\n");
    if (result) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    ILFree(pidlMyComputer);

    result = SHGetSpecialFolderPathW(NULL, wszFileName, CSIDL_DESKTOPDIRECTORY, FALSE);
    ok(result, "SHGetSpecialFolderPathW failed! Last error: %lu\n", GetLastError());
    if (!result) {
        IShellFolder_Release(psfDesktop);
        return;
    }
    myPathAddBackslashW(wszFileName);
    lstrcatW(wszFileName, wszTestFile);
    hTestFile = CreateFileW(wszFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    ok(hTestFile != INVALID_HANDLE_VALUE, "CreateFileW failed! Last error: %lu\n", GetLastError());
    if (hTestFile == INVALID_HANDLE_VALUE) {
        IShellFolder_Release(psfDesktop);
        return;
    }
    CloseHandle(hTestFile);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszTestFile, NULL, &pidlTestFile, NULL);
    ok (hr == S_OK, "Desktop's ParseDisplayName failed to parse filename hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        DeleteFileW(wszFileName);
        ILFree(pidlTestFile);
        return;
    }

    /* This test is to show that the Desktop shellfolder prepends the CSIDL_DESKTOPDIRECTORY
     * path for files placed on the desktop, if called with SHGDN_FORPARSING. */
    hr = IShellFolder_GetDisplayNameOf(psfDesktop, pidlTestFile, SHGDN_FORPARSING, &strret);
    ok (hr == S_OK, "Desktop's GetDisplayNamfOf failed! hr = %08lx\n", hr);
    IShellFolder_Release(psfDesktop);
    DeleteFileW(wszFileName);
    if (hr != S_OK) {
        ILFree(pidlTestFile);
        return;
    }
    StrRetToBufW(&strret, pidlTestFile, wszPath, MAX_PATH);
    ok(0 == lstrcmpW(wszFileName, wszPath),
       "Desktop->GetDisplayNameOf(pidlTestFile, SHGDN_FORPARSING) "
       "returned incorrect path for file placed on desktop\n");

    result = SHGetPathFromIDListW(pidlTestFile, wszPath);
    ok(result, "SHGetPathFromIDListW failed! Last error: %lu\n", GetLastError());
    ok(0 == lstrcmpW(wszFileName, wszPath), "SHGetPathFromIDListW returned incorrect path for file placed on desktop\n");

    if (pSHGetPathFromIDListEx)
    {
        result = pSHGetPathFromIDListEx(pidlEmpty, wszPath, MAX_PATH, SFGAO_FILESYSTEM);
        ok(result, "SHGetPathFromIDListEx failed: %lu\n", GetLastError());
        ok(!lstrcmpiW(wszDesktop, wszPath), "Unexpected SHGetPathFromIDListEx result %s, expected %s\n",
           wine_dbgstr_w(wszPath), wine_dbgstr_w(wszDesktop));

        result = pSHGetPathFromIDListEx(pidlTestFile, wszPath, MAX_PATH, SFGAO_FILESYSTEM);
        ok(result, "SHGetPathFromIDListEx failed: %lu\n", GetLastError());
        ok(!lstrcmpiW(wszFileName, wszPath), "Unexpected SHGetPathFromIDListEx result %s, expected %s\n",
           wine_dbgstr_w(wszPath), wine_dbgstr_w(wszFileName));

        SetLastError(0xdeadbeef);
        memset(wszPath, 0x55, sizeof(wszPath));
        result = pSHGetPathFromIDListEx(pidlTestFile, wszPath, 5, SFGAO_FILESYSTEM);
        ok(!result, "SHGetPathFromIDListEx returned: %x(%lu)\n", result, GetLastError());

        SetLastError(0xdeadbeef);
        memset(wszPath, 0x55, sizeof(wszPath));
        result = pSHGetPathFromIDListEx(pidlEmpty, wszPath, 5, SFGAO_FILESYSTEM);
        ok(!result, "SHGetPathFromIDListEx returned: %x(%lu)\n", result, GetLastError());
    }
    else
        win_skip("SHGetPathFromIDListEx not available\n");

    ILFree(pidlTestFile);

    /* Test if we can get the path from the start menu "program files" PIDL. */
    hr = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidlPrograms);
    ok(hr == S_OK, "SHGetFolderLocation failed: 0x%08lx\n", hr);

    SetLastError(0xdeadbeef);
    result = SHGetPathFromIDListW(pidlPrograms, wszPath);
    ILFree(pidlPrograms);
    ok(result, "SHGetPathFromIDListW failed\n");
}

static void test_EnumObjects_and_CompareIDs(void)
{
    ITEMIDLIST *newPIDL;
    IShellFolder *IDesktopFolder, *testIShellFolder;
    char  cCurrDirA [MAX_PATH] = {0};
    static const CHAR cTestDirA[] = "\\testdir";
    WCHAR cTestDirW[MAX_PATH];
    int len;
    HRESULT hr;

    GetCurrentDirectoryA(MAX_PATH, cCurrDirA);
    len = lstrlenA(cCurrDirA);

    if(len == 0) {
        win_skip("GetCurrentDirectoryA returned empty string. Skipping test_EnumObjects_and_CompareIDs\n");
        return;
    }
    if(cCurrDirA[len-1] == '\\')
        cCurrDirA[len-1] = 0;

    lstrcatA(cCurrDirA, cTestDirA);
    MultiByteToWideChar(CP_ACP, 0, cCurrDirA, -1, cTestDirW, MAX_PATH);

    hr = SHGetDesktopFolder(&IDesktopFolder);
    ok(hr == S_OK, "SHGetDesktopfolder failed %08lx\n", hr);

    CreateFilesFolders();

    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, cTestDirW, NULL, &newPIDL, 0);
    ok(hr == S_OK, "ParseDisplayName failed %08lx\n", hr);

    hr = IShellFolder_BindToObject(IDesktopFolder, newPIDL, NULL, (REFIID)&IID_IShellFolder, (LPVOID *)&testIShellFolder);
    ok(hr == S_OK, "BindToObject failed %08lx\n", hr);

    test_EnumObjects(testIShellFolder);

    IShellFolder_Release(testIShellFolder);

    Cleanup();

    ILFree(newPIDL);

    IShellFolder_Release(IDesktopFolder);
}

/* A simple implementation of an IPropertyBag, which returns fixed values for
 * 'Target' and 'Attributes' properties.
 */
static HRESULT WINAPI InitPropertyBag_IPropertyBag_QueryInterface(IPropertyBag *iface, REFIID riid,
    void **ppvObject) 
{
    if (!ppvObject)
        return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IPropertyBag, riid)) {
        *ppvObject = iface;
    } else {
        ok (FALSE, "InitPropertyBag asked for unknown interface!\n");
        return E_NOINTERFACE;
    }

    IPropertyBag_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI InitPropertyBag_IPropertyBag_AddRef(IPropertyBag *iface) {
    return 2;
}

static ULONG WINAPI InitPropertyBag_IPropertyBag_Release(IPropertyBag *iface) {
    return 1;
}

static HRESULT WINAPI InitPropertyBag_IPropertyBag_Read(IPropertyBag *iface, LPCOLESTR pszPropName,
    VARIANT *pVar, IErrorLog *pErrorLog)
{
    if (!lstrcmpW(pszPropName, L"TargetSpecialFolder")) {
        ok(V_VT(pVar) == VT_I4, "Wrong variant type for 'TargetSpecialFolder' property!\n");
        return E_INVALIDARG;
    }

    if (!lstrcmpW(pszPropName, L"ResolveLinkFlags"))
    {
        ok(V_VT(pVar) == VT_UI4, "Wrong variant type for 'ResolveLinkFlags' property!\n");
        return E_INVALIDARG;
    }

    if (!lstrcmpW(pszPropName, L"Target")) {
        WCHAR wszPath[MAX_PATH];
        BOOL result;

        ok(V_VT(pVar) == VT_BSTR, "Wrong variant type for 'Target' property!\n");
        if (V_VT(pVar) != VT_BSTR) return E_INVALIDARG;

        result = SHGetSpecialFolderPathW(NULL, wszPath, CSIDL_DESKTOPDIRECTORY, FALSE);
        ok(result, "SHGetSpecialFolderPathW(DESKTOPDIRECTORY) failed! %lu\n", GetLastError());
        if (!result) return E_INVALIDARG;

        V_BSTR(pVar) = SysAllocString(wszPath);
        return S_OK;
    }

    if (!lstrcmpW(pszPropName, L"Attributes")) {
        ok(V_VT(pVar) == VT_UI4, "Wrong variant type for 'Attributes' property!\n");
        if (V_VT(pVar) != VT_UI4) return E_INVALIDARG;
        V_UI4(pVar) = SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR|
                      SFGAO_CANRENAME|SFGAO_FILESYSTEM;
        return S_OK;
    }

    if (!lstrcmpW(pszPropName, L"TargetKnownFolder")) {
        ok(V_VT(pVar) == VT_BSTR, "Wrong variant type for 'TargetKnownFolder' property!\n");
        /* TODO */
        return E_INVALIDARG;
    }

    if (!lstrcmpW(pszPropName, L"CLSID")) {
        ok(V_VT(pVar) == VT_EMPTY, "Wrong variant type for 'CLSID' property!\n");
        /* TODO */
        return E_INVALIDARG;
    }

    ok(FALSE, "PropertyBag was asked for unknown property %s (vt=%d)!\n", wine_dbgstr_w(pszPropName), V_VT(pVar));
    return E_INVALIDARG;
}

static HRESULT WINAPI InitPropertyBag_IPropertyBag_Write(IPropertyBag *iface, LPCOLESTR pszPropName,
    VARIANT *pVar)
{
    ok(FALSE, "Unexpected call to IPropertyBag_Write\n");
    return E_NOTIMPL;
}
    
static const IPropertyBagVtbl InitPropertyBag_IPropertyBagVtbl = {
    InitPropertyBag_IPropertyBag_QueryInterface,
    InitPropertyBag_IPropertyBag_AddRef,
    InitPropertyBag_IPropertyBag_Release,
    InitPropertyBag_IPropertyBag_Read,
    InitPropertyBag_IPropertyBag_Write
};

static struct IPropertyBag InitPropertyBag = {
    &InitPropertyBag_IPropertyBagVtbl
};

static void test_FolderShortcut(void) {
    IPersistPropertyBag *pPersistPropertyBag;
    IShellFolder *pShellFolder, *pDesktopFolder;
    IPersistFolder3 *pPersistFolder3;
    HRESULT hr;
    STRRET strret;
    WCHAR wszDesktopPath[MAX_PATH], wszBuffer[MAX_PATH];
    BOOL result;
    CLSID clsid;
    LPITEMIDLIST pidlCurrentFolder, pidlWineTestFolder, pidlSubFolder;
    HKEY hShellExtKey;
    WCHAR wszWineTestFolder[] = L"::{9B352EBF-2765-45C1-B4C6-85CC7F7ABC64}";
    WCHAR wszShellExtKey[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\"
        L"{9b352ebf-2765-45c1-b4c6-85cc7f7abc64}";

    WCHAR wszSomeSubFolder[] = L"SubFolder";
    static const GUID CLSID_UnixDosFolder =
        {0x9d20aae8, 0x0625, 0x44b0, {0x9c, 0xa7, 0x71, 0x88, 0x9c, 0x22, 0x54, 0xd9}};

    /* These tests basically show, that CLSID_FolderShortcuts are initialized
     * via their IPersistPropertyBag interface. And that the target folder
     * is taken from the IPropertyBag's 'Target' property.
     */
    hr = CoCreateInstance(&CLSID_FolderShortcut, NULL, CLSCTX_INPROC_SERVER, 
                          &IID_IPersistPropertyBag, (LPVOID*)&pPersistPropertyBag);
    if (hr == REGDB_E_CLASSNOTREG) {
        win_skip("CLSID_FolderShortcut is not implemented\n");
        return;
    }
    ok (hr == S_OK, "CoCreateInstance failed! hr = 0x%08lx\n", hr);
    if (hr != S_OK) return;

    hr = IPersistPropertyBag_Load(pPersistPropertyBag, &InitPropertyBag, NULL);
    ok(hr == S_OK, "IPersistPropertyBag_Load failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IPersistPropertyBag_Release(pPersistPropertyBag);
        return;
    }

    hr = IPersistPropertyBag_QueryInterface(pPersistPropertyBag, &IID_IShellFolder, 
                                            (LPVOID*)&pShellFolder);
    IPersistPropertyBag_Release(pPersistPropertyBag);
    ok(hr == S_OK, "IPersistPropertyBag_QueryInterface(IShellFolder) failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    hr = IShellFolder_GetDisplayNameOf(pShellFolder, NULL, SHGDN_FORPARSING, &strret);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* win10 */,
       "IShellFolder_GetDisplayNameOf(NULL) failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(pShellFolder);
        return;
    }

    result = SHGetSpecialFolderPathW(NULL, wszDesktopPath, CSIDL_DESKTOPDIRECTORY, FALSE);
    ok(result, "SHGetSpecialFolderPathW(CSIDL_DESKTOPDIRECTORY) failed! %lu\n", GetLastError());
    if (!result) return;

    StrRetToBufW(&strret, NULL, wszBuffer, MAX_PATH);
    ok(!lstrcmpiW(wszDesktopPath, wszBuffer), "FolderShortcut returned incorrect folder!\n");

    hr = IShellFolder_QueryInterface(pShellFolder, &IID_IPersistFolder3, (LPVOID*)&pPersistFolder3);
    IShellFolder_Release(pShellFolder);
    ok(hr == S_OK, "IShellFolder_QueryInterface(IID_IPersistFolder3 failed! hr = 0x%08lx\n", hr);
    if (hr != S_OK) return;

    hr = IPersistFolder3_GetClassID(pPersistFolder3, &clsid);
    ok(hr == S_OK, "IPersistFolder3_GetClassID failed! hr=0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_FolderShortcut), "Unexpected CLSID!\n");

    hr = IPersistFolder3_GetCurFolder(pPersistFolder3, &pidlCurrentFolder);
    todo_wine ok(hr == S_FALSE, "IPersistFolder3_GetCurFolder failed! hr=0x%08lx\n", hr);
    ok(!pidlCurrentFolder, "IPersistFolder3_GetCurFolder should return a NULL pidl!\n");
                    
    /* For FolderShortcut objects, the Initialize method initialized the folder's position in the
     * shell namespace. The target folder, read from the property bag above, remains untouched. 
     * The following tests show this: The itemidlist for some imaginary shellfolder object
     * is created and the FolderShortcut is initialized with it. GetCurFolder now returns this
     * itemidlist, but GetDisplayNameOf still returns the path from above.
     */
    hr = SHGetDesktopFolder(&pDesktopFolder);
    ok (hr == S_OK, "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    /* Temporarily register WineTestFolder as a shell namespace extension at the Desktop. 
     * Otherwise ParseDisplayName fails on WinXP with E_INVALIDARG */
    RegCreateKeyW(HKEY_CURRENT_USER, wszShellExtKey, &hShellExtKey);
    RegCloseKey(hShellExtKey);
    hr = IShellFolder_ParseDisplayName(pDesktopFolder, NULL, NULL, wszWineTestFolder, NULL,
                                       &pidlWineTestFolder, NULL);
    RegDeleteKeyW(HKEY_CURRENT_USER, wszShellExtKey);
    IShellFolder_Release(pDesktopFolder);
    ok (hr == S_OK, "IShellFolder::ParseDisplayName failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    hr = IPersistFolder3_Initialize(pPersistFolder3, pidlWineTestFolder);
    ok (hr == S_OK, "IPersistFolder3::Initialize failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IPersistFolder3_Release(pPersistFolder3);
        ILFree(pidlWineTestFolder);
        return;
    }

    hr = IPersistFolder3_GetCurFolder(pPersistFolder3, &pidlCurrentFolder);
    ok(hr == S_OK, "IPersistFolder3_GetCurFolder failed! hr=0x%08lx\n", hr);
    ok(ILIsEqual(pidlCurrentFolder, pidlWineTestFolder),
        "IPersistFolder3_GetCurFolder should return pidlWineTestFolder!\n");
    ILFree(pidlCurrentFolder);
    ILFree(pidlWineTestFolder);

    hr = IPersistFolder3_QueryInterface(pPersistFolder3, &IID_IShellFolder, (LPVOID*)&pShellFolder);
    IPersistFolder3_Release(pPersistFolder3);
    ok(hr == S_OK, "IPersistFolder3_QueryInterface(IShellFolder) failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    hr = IShellFolder_GetDisplayNameOf(pShellFolder, NULL, SHGDN_FORPARSING, &strret);
    ok(hr == S_OK, "IShellFolder_GetDisplayNameOf(NULL) failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(pShellFolder);
        return;
    }

    StrRetToBufW(&strret, NULL, wszBuffer, MAX_PATH);
    ok(!lstrcmpiW(wszDesktopPath, wszBuffer), "FolderShortcut returned incorrect folder!\n");

    /* Next few lines are meant to show that children of FolderShortcuts are not FolderShortcuts,
     * but ShellFSFolders. */
    myPathAddBackslashW(wszDesktopPath);
    lstrcatW(wszDesktopPath, wszSomeSubFolder);
    if (!CreateDirectoryW(wszDesktopPath, NULL)) {
        IShellFolder_Release(pShellFolder);
        return;
    }
    
    hr = IShellFolder_ParseDisplayName(pShellFolder, NULL, NULL, wszSomeSubFolder, NULL, 
                                       &pidlSubFolder, NULL);
    RemoveDirectoryW(wszDesktopPath);
    ok (hr == S_OK, "IShellFolder::ParseDisplayName failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(pShellFolder);
        return;
    }

    hr = IShellFolder_BindToObject(pShellFolder, pidlSubFolder, NULL, &IID_IPersistFolder3,
                                   (LPVOID*)&pPersistFolder3);
    IShellFolder_Release(pShellFolder);
    ILFree(pidlSubFolder);
    ok (hr == S_OK, "IShellFolder::BindToObject failed! hr = %08lx\n", hr);
    if (hr != S_OK)
        return;

    /* On windows, we expect CLSID_ShellFSFolder. On wine we relax this constraint
     * a little bit and also allow CLSID_UnixDosFolder. */
    hr = IPersistFolder3_GetClassID(pPersistFolder3, &clsid);
    ok(hr == S_OK, "IPersistFolder3_GetClassID failed! hr=0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_ShellFSFolder) || IsEqualCLSID(&clsid, &CLSID_UnixDosFolder),
        "IPersistFolder3::GetClassID returned unexpected CLSID!\n");

    IPersistFolder3_Release(pPersistFolder3);
}

#pragma pack(push,1)
struct FileStructA {
    BYTE  type;
    BYTE  dummy;
    DWORD dwFileSize;
    WORD  uFileDate;    /* In our current implementation this is */
    WORD  uFileTime;    /* FileTimeToDosDate(WIN32_FIND_DATA->ftLastWriteTime) */
    WORD  uFileAttribs;
    CHAR  szName[1];
};

struct FileStructW {
    WORD  cbLen;        /* Length of this element. */
    BYTE  abFooBar1[6]; /* Beyond any recognition. */
    WORD  uDate;        /* FileTimeToDosDate(WIN32_FIND_DATA->ftCreationTime)? */
    WORD  uTime;        /* (this is currently speculation) */
    WORD  uDate2;       /* FileTimeToDosDate(WIN32_FIND_DATA->ftLastAccessTime)? */
    WORD  uTime2;       /* (this is currently speculation) */
    BYTE  abFooBar2[4]; /* Beyond any recognition. */
    WCHAR wszName[1];   /* The long filename in unicode. */
    /* Just for documentation: Right after the unicode string: */
    WORD  cbOffset;     /* FileStructW's offset from the beginning of the SHITMEID. 
                         * SHITEMID->cb == uOffset + cbLen */
};
#pragma pack(pop)

static void test_ITEMIDLIST_format(void) {
    WCHAR wszPersonal[MAX_PATH];
    LPSHELLFOLDER psfDesktop, psfPersonal;
    LPITEMIDLIST pidlPersonal, pidlFile;
    HANDLE hFile;
    HRESULT hr;
    BOOL bResult;
    WCHAR wszFile[3][17] = { L"even_", L"odd_", L"longer_than.8_3" };
    int i;

    bResult = SHGetSpecialFolderPathW(NULL, wszPersonal, CSIDL_PERSONAL, FALSE);
    ok(bResult, "SHGetSpecialFolderPathW failed! Last error: %lu\n", GetLastError());
    if (!bResult) return;

    SetLastError(0xdeadbeef);
    bResult = SetCurrentDirectoryW(wszPersonal);
    if (!bResult && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
        win_skip("Most W-calls are not implemented\n");
        return;
    }
    ok(bResult, "SetCurrentDirectory failed! Last error: %lu\n", GetLastError());
    if (!bResult) return;

    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed! hr: %08lx\n", hr);
    if (hr != S_OK) return;

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszPersonal, NULL, &pidlPersonal, NULL);
    ok(hr == S_OK, "psfDesktop->ParseDisplayName failed! hr = %08lx\n", hr);
    if (hr != S_OK) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    hr = IShellFolder_BindToObject(psfDesktop, pidlPersonal, NULL, &IID_IShellFolder,
        (LPVOID*)&psfPersonal);
    IShellFolder_Release(psfDesktop);
    ILFree(pidlPersonal);
    ok(hr == S_OK, "psfDesktop->BindToObject failed! hr = %08lx\n", hr);
    if (hr != S_OK) return;

    for (i=0; i<3; i++) {
        CHAR szFile[MAX_PATH];
        struct FileStructA *pFileStructA;
        WORD cbOffset;

        WideCharToMultiByte(CP_ACP, 0, wszFile[i], -1, szFile, MAX_PATH, NULL, NULL);

        hFile = CreateFileW(wszFile[i], GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_FLAG_WRITE_THROUGH, NULL);
        ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed! (%lu)\n", GetLastError());
        if (hFile == INVALID_HANDLE_VALUE) {
            IShellFolder_Release(psfPersonal);
            return;
        }
        CloseHandle(hFile);

        hr = IShellFolder_ParseDisplayName(psfPersonal, NULL, NULL, wszFile[i], NULL, &pidlFile, NULL);
        DeleteFileW(wszFile[i]);
        ok(hr == S_OK, "psfPersonal->ParseDisplayName failed! hr: %08lx\n", hr);
        if (hr != S_OK) {
            IShellFolder_Release(psfPersonal);
            return;
        }

        pFileStructA = (struct FileStructA *)pidlFile->mkid.abID;
        ok(pFileStructA->type == 0x32, "PIDLTYPE should be 0x32!\n");
        ok(pFileStructA->dummy == 0x00, "Dummy Byte should be 0x00!\n");
        ok(pFileStructA->dwFileSize == 0, "Filesize should be zero!\n");

        if (i < 2) /* First two file names are already in valid 8.3 format */
            ok(!strcmp(szFile, (CHAR*)&pidlFile->mkid.abID[12]), "Wrong file name!\n");
        else
            /* WinXP stores a derived 8.3 dos name (LONGER~1.8_3) here. We probably
             * can't implement this correctly, since unix filesystems don't support
             * this nasty short/long filename stuff. So we'll probably stay with our
             * current habit of storing the long filename here, which seems to work
             * just fine. */
            todo_wine
            ok(pidlFile->mkid.abID[18] == '~', "Should be derived 8.3 name!\n");

        if (i == 0) /* First file name has an even number of chars. No need for alignment. */
            ok(pidlFile->mkid.abID[12 + strlen(szFile) + 1] != '\0',
                "Alignment byte, where there shouldn't be!\n");

        if (i == 1) /* Second file name has an uneven number of chars => alignment byte */
            ok(pidlFile->mkid.abID[12 + strlen(szFile) + 1] == '\0',
                "There should be an alignment byte, but isn't!\n");

        /* The offset of the FileStructW member is stored as a WORD at the end of the pidl. */
        cbOffset = *(WORD*)(((LPBYTE)pidlFile)+pidlFile->mkid.cb-sizeof(WORD));
        ok ((cbOffset >= sizeof(struct FileStructA) &&
            cbOffset <= pidlFile->mkid.cb - sizeof(struct FileStructW)),
            "Wrong offset value (%d) stored at the end of the PIDL\n", cbOffset);

        if (cbOffset >= sizeof(struct FileStructA) &&
            cbOffset <= pidlFile->mkid.cb - sizeof(struct FileStructW))
        {
            struct FileStructW *pFileStructW = (struct FileStructW *)(((LPBYTE)pidlFile)+cbOffset);
            WCHAR *name = pFileStructW->wszName;

            ok(pidlFile->mkid.cb == cbOffset + pFileStructW->cbLen,
                "FileStructW's offset and length should add up to the PIDL's length!\n");

            if (pidlFile->mkid.cb == cbOffset + pFileStructW->cbLen) {
                /* Since we just created the file, time of creation,
                 * time of last access and time of last write access just be the same.
                 * These tests seem to fail sometimes (on WinXP), if the test is run again shortly
                 * after the first run. I do remember something with NTFS keeping the creation time
                 * if a file is deleted and then created again within a couple of seconds or so.
                 * Might be the reason. */
                ok (pFileStructA->uFileDate == pFileStructW->uDate &&
                    pFileStructA->uFileTime == pFileStructW->uTime,
                    "Last write time should match creation time!\n");

                /* On FAT filesystems the last access time is midnight
                   local time, so the values of uDate2 and uTime2 will
                   depend on the local timezone.  If the times are exactly
                   equal then the dates should be identical for both FAT
                   and NTFS as no timezone is more than 1 day away from UTC.
                */
                if (pFileStructA->uFileTime == pFileStructW->uTime2)
                {
                    ok (pFileStructA->uFileDate == pFileStructW->uDate2,
                        "Last write date and time should match last access date and time!\n");
                }
                else
                {
                    /* Filesystem may be FAT. Check date within 1 day
                       and seconds are zero. */
                    trace ("Filesystem may be FAT. Performing less strict atime test.\n");
                    ok ((pFileStructW->uTime2 & 0x1F) == 0,
                        "Last access time on FAT filesystems should have zero seconds.\n");
                    /* TODO: Perform check for date being within one day.*/
                }

                ok (!lstrcmpW(wszFile[i], name) ||
                    !lstrcmpW(wszFile[i], name + 9)  || /* Vista */
                    !lstrcmpW(wszFile[i], name + 11) || /* Win7 */
                    !lstrcmpW(wszFile[i], name + 13),   /* Win8 */
                    "The filename should be stored in unicode at this position!\n");
            }
        }

        ILFree(pidlFile);
    }

    IShellFolder_Release(psfPersonal);
}

static void test_SHGetFolderPathA(void)
{
    static const BOOL is_win64 = sizeof(void *) > sizeof(int);
    BOOL is_wow64;
    char path[MAX_PATH];
    char path_x86[MAX_PATH];
    char path_key[MAX_PATH];
    HRESULT hr;
    HKEY key;

    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    hr = SHGetFolderPathA( 0, CSIDL_PROGRAM_FILES, 0, SHGFP_TYPE_CURRENT, path );
    ok( hr == S_OK, "SHGetFolderPathA failed %lx\n", hr );
    hr = SHGetFolderPathA( 0, CSIDL_PROGRAM_FILESX86, 0, SHGFP_TYPE_CURRENT, path_x86 );
    if (hr == E_FAIL)
    {
        win_skip( "Program Files (x86) not supported\n" );
        return;
    }
    ok( hr == S_OK, "SHGetFolderPathA failed %lx\n", hr );
    if (is_win64)
    {
        ok( lstrcmpiA( path, path_x86 ), "paths are identical '%s'\n", path );
        ok( strstr( path, "x86" ) == NULL, "64-bit path '%s' contains x86\n", path );
        ok( strstr( path_x86, "x86" ) != NULL, "32-bit path '%s' doesn't contain x86\n", path_x86 );
    }
    else
    {
        ok( !lstrcmpiA( path, path_x86 ), "paths differ '%s' != '%s'\n", path, path_x86 );
        if (is_wow64)
            ok( strstr( path, "x86" ) != NULL, "32-bit path '%s' doesn't contain x86\n", path );
        else
            ok( strstr( path, "x86" ) == NULL, "32-bit path '%s' contains x86\n", path );
    }
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &key ))
    {
        DWORD type, count = sizeof(path_x86);
        if (!RegQueryValueExA( key, "ProgramFilesDir (x86)", NULL, &type, (BYTE *)path_key, &count ))
        {
            ok( is_win64 || is_wow64, "ProgramFilesDir (x86) exists on 32-bit setup\n" );
            ok( !lstrcmpiA( path_key, path_x86 ), "paths differ '%s' != '%s'\n", path_key, path_x86 );
        }
        else ok( !is_win64 && !is_wow64, "ProgramFilesDir (x86) should exist on 64-bit setup\n" );
        RegCloseKey( key );
    }

    hr = SHGetFolderPathA( 0, CSIDL_PROGRAM_FILES_COMMON, 0, SHGFP_TYPE_CURRENT, path );
    ok( hr == S_OK, "SHGetFolderPathA failed %lx\n", hr );
    hr = SHGetFolderPathA( 0, CSIDL_PROGRAM_FILES_COMMONX86, 0, SHGFP_TYPE_CURRENT, path_x86 );
    if (hr == E_FAIL)
    {
        win_skip( "Common Files (x86) not supported\n" );
        return;
    }
    ok( hr == S_OK, "SHGetFolderPathA failed %lx\n", hr );
    if (is_win64)
    {
        ok( lstrcmpiA( path, path_x86 ), "paths are identical '%s'\n", path );
        ok( strstr( path, "x86" ) == NULL, "64-bit path '%s' contains x86\n", path );
        ok( strstr( path_x86, "x86" ) != NULL, "32-bit path '%s' doesn't contain x86\n", path_x86 );
    }
    else
    {
        ok( !lstrcmpiA( path, path_x86 ), "paths differ '%s' != '%s'\n", path, path_x86 );
        if (is_wow64)
            ok( strstr( path, "x86" ) != NULL, "32-bit path '%s' doesn't contain x86\n", path );
        else
            ok( strstr( path, "x86" ) == NULL, "32-bit path '%s' contains x86\n", path );
    }
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &key ))
    {
        DWORD type, count = sizeof(path_x86);
        if (!RegQueryValueExA( key, "CommonFilesDir (x86)", NULL, &type, (BYTE *)path_key, &count ))
        {
            ok( is_win64 || is_wow64, "CommonFilesDir (x86) exists on 32-bit setup\n" );
            ok( !lstrcmpiA( path_key, path_x86 ), "paths differ '%s' != '%s'\n", path_key, path_x86 );
        }
        else ok( !is_win64 && !is_wow64, "CommonFilesDir (x86) should exist on 64-bit setup\n" );
    }
}

static void test_SHGetFolderPathAndSubDirA(void)
{
    HRESULT ret;
    BOOL delret;
    DWORD dwret;
    int i;
    static const char wine[] = "wine";
    static const char winetemp[] = "wine\\temp";
    static char appdata[MAX_PATH];
    static char testpath[2 * MAX_PATH];
    static char toolongpath[MAX_PATH+1];

    if(FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata)))
    {
        win_skip("SHGetFolderPathA failed for CSIDL_LOCAL_APPDATA!\n");
        return;
    }

    sprintf(testpath, "%s\\%s", appdata, winetemp);
    delret = RemoveDirectoryA(testpath);
    if(!delret && (ERROR_PATH_NOT_FOUND != GetLastError()) ) {
        win_skip("RemoveDirectoryA(%s) failed with error %lu\n", testpath, GetLastError());
        return;
    }

    sprintf(testpath, "%s\\%s", appdata, wine);
    delret = RemoveDirectoryA(testpath);
    if(!delret && (ERROR_PATH_NOT_FOUND != GetLastError()) && (ERROR_FILE_NOT_FOUND != GetLastError())) {
        win_skip("RemoveDirectoryA(%s) failed with error %lu\n", testpath, GetLastError());
        return;
    }

    /* test invalid second parameter */
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | 0xff, NULL, SHGFP_TYPE_CURRENT, wine, testpath);
    ok(E_INVALIDARG == ret, "expected E_INVALIDARG, got  %lx\n", ret);

    /* test fourth parameter */
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | CSIDL_LOCAL_APPDATA, NULL, 2, winetemp, testpath);
    switch(ret) {
        case S_OK: /* winvista */
            ok(!strncmp(appdata, testpath, strlen(appdata)),
                "expected %s to start with %s\n", testpath, appdata);
            ok(!lstrcmpA(&testpath[1 + strlen(appdata)], winetemp),
                "expected %s to end with %s\n", testpath, winetemp);
            break;
        case E_INVALIDARG: /* winxp, win2k3 */
            break;
        default:
            ok(0, "expected S_OK or E_INVALIDARG, got  %lx\n", ret);
    }

    /* test fifth parameter */
    testpath[0] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, NULL, testpath);
    ok(S_OK == ret, "expected S_OK, got %lx\n", ret);
    ok(!lstrcmpA(appdata, testpath), "expected %s, got %s\n", appdata, testpath);

    testpath[0] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, "", testpath);
    ok(S_OK == ret, "expected S_OK, got %lx\n", ret);
    ok(!lstrcmpA(appdata, testpath), "expected %s, got %s\n", appdata, testpath);

    testpath[0] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, "\\", testpath);
    ok(S_OK == ret, "expected S_OK, got %lx\n", ret);
    ok(!lstrcmpA(appdata, testpath), "expected %s, got %s\n", appdata, testpath);

    for(i=0; i< MAX_PATH; i++)
        toolongpath[i] = '0' + i % 10;
    toolongpath[MAX_PATH] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, toolongpath, testpath);
    ok(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) == ret,
        "expected %lx, got %lx\n", HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE), ret);

    testpath[0] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_DONT_VERIFY | CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wine, NULL);
    ok((S_OK == ret) || (E_INVALIDARG == ret), "expected S_OK or E_INVALIDARG, got %lx\n", ret);

    /* test a not existing path */
    testpath[0] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, winetemp, testpath);
    ok(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == ret,
        "expected %lx, got %lx\n", HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), ret);

    /* create a directory inside a not existing directory */
    testpath[0] = '\0';
    ret = SHGetFolderPathAndSubDirA(NULL, CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, winetemp, testpath);
    ok(S_OK == ret, "expected S_OK, got %lx\n", ret);
    ok(!strncmp(appdata, testpath, strlen(appdata)),
        "expected %s to start with %s\n", testpath, appdata);
    ok(!lstrcmpA(&testpath[1 + strlen(appdata)], winetemp),
        "expected %s to end with %s\n", testpath, winetemp);
    dwret = GetFileAttributesA(testpath);
    ok(FILE_ATTRIBUTE_DIRECTORY | dwret, "expected %lx to contain FILE_ATTRIBUTE_DIRECTORY\n", dwret);

    /* cleanup */
    sprintf(testpath, "%s\\%s", appdata, winetemp);
    RemoveDirectoryA(testpath);
    sprintf(testpath, "%s\\%s", appdata, wine);
    RemoveDirectoryA(testpath);
}

static void test_LocalizedNames(void)
{
    static char cCurrDirA[MAX_PATH];
    WCHAR cCurrDirW[MAX_PATH], tempbufW[25];
    IShellFolder *IDesktopFolder, *testIShellFolder;
    ITEMIDLIST *newPIDL;
    int len;
    HRESULT hr;
    static char resourcefile[MAX_PATH];
    DWORD res;
    HANDLE file;
    STRRET strret;
    BOOL ret;

    static const char desktopini_contents1[] =
        "[.ShellClassInfo]\r\n"
        "LocalizedResourceName=@";
    static const char desktopini_contents2[] =
        ",-1\r\n";
    static WCHAR foldernameW[] = L"testfolder";

    /* create folder with desktop.ini and localized name in GetModuleFileNameA(NULL) */
    CreateDirectoryA(".\\testfolder", NULL);

    SetFileAttributesA(".\\testfolder", GetFileAttributesA(".\\testfolder")|FILE_ATTRIBUTE_SYSTEM);

    GetModuleFileNameA(NULL, resourcefile, MAX_PATH);

    file = CreateFileA(".\\testfolder\\desktop.ini", GENERIC_WRITE, 0, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %li\n", GetLastError());
    ret = WriteFile(file, desktopini_contents1, strlen(desktopini_contents1), &res, NULL) &&
          WriteFile(file, resourcefile, strlen(resourcefile), &res, NULL) &&
          WriteFile(file, desktopini_contents2, strlen(desktopini_contents2), &res, NULL);
    ok(ret, "WriteFile failed %li\n", GetLastError());
    CloseHandle(file);

    /* get IShellFolder for parent */
    GetCurrentDirectoryA(MAX_PATH, cCurrDirA);
    len = lstrlenA(cCurrDirA);

    if (len == 0) {
        win_skip("GetCurrentDirectoryA returned empty string. Skipping test_LocalizedNames\n");
        goto cleanup;
    }
    if(cCurrDirA[len-1] == '\\')
        cCurrDirA[len-1] = 0;

    MultiByteToWideChar(CP_ACP, 0, cCurrDirA, -1, cCurrDirW, MAX_PATH);

    hr = SHGetDesktopFolder(&IDesktopFolder);
    ok(hr == S_OK, "SHGetDesktopfolder failed %08lx\n", hr);

    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, cCurrDirW, NULL, &newPIDL, 0);
    ok(hr == S_OK, "ParseDisplayName failed %08lx\n", hr);

    hr = IShellFolder_BindToObject(IDesktopFolder, newPIDL, NULL, (REFIID)&IID_IShellFolder, (LPVOID *)&testIShellFolder);
    ok(hr == S_OK, "BindToObject failed %08lx\n", hr);

    ILFree(newPIDL);

    /* windows reads the display name from the resource */
    hr = IShellFolder_ParseDisplayName(testIShellFolder, NULL, NULL, foldernameW, NULL, &newPIDL, 0);
    ok(hr == S_OK, "ParseDisplayName failed %08lx\n", hr);

    hr = IShellFolder_GetDisplayNameOf(testIShellFolder, newPIDL, SHGDN_INFOLDER, &strret);
    ok(hr == S_OK, "GetDisplayNameOf failed %08lx\n", hr);

    hr = StrRetToBufW(&strret, newPIDL, tempbufW, ARRAY_SIZE(tempbufW));
    ok (hr == S_OK, "StrRetToBufW failed! hr = %08lx\n", hr);
    todo_wine
    ok (!lstrcmpiW(tempbufW, L"Folder Name Resource"), "GetDisplayNameOf returned %s\n", wine_dbgstr_w(tempbufW));

    /* editing name is also read from the resource */
    hr = IShellFolder_GetDisplayNameOf(testIShellFolder, newPIDL, SHGDN_INFOLDER|SHGDN_FOREDITING, &strret);
    ok(hr == S_OK, "GetDisplayNameOf failed %08lx\n", hr);

    hr = StrRetToBufW(&strret, newPIDL, tempbufW, ARRAY_SIZE(tempbufW));
    ok (hr == S_OK, "StrRetToBufW failed! hr = %08lx\n", hr);
    todo_wine
    ok (!lstrcmpiW(tempbufW, L"Folder Name Resource"), "GetDisplayNameOf returned %s\n", wine_dbgstr_w(tempbufW));

    /* parsing name is unchanged */
    hr = IShellFolder_GetDisplayNameOf(testIShellFolder, newPIDL, SHGDN_INFOLDER|SHGDN_FORPARSING, &strret);
    ok(hr == S_OK, "GetDisplayNameOf failed %08lx\n", hr);

    hr = StrRetToBufW(&strret, newPIDL, tempbufW, ARRAY_SIZE(tempbufW));
    ok (hr == S_OK, "StrRetToBufW failed! hr = %08lx\n", hr);
    ok (!lstrcmpiW(tempbufW, foldernameW), "GetDisplayNameOf returned %s\n", wine_dbgstr_w(tempbufW));

    IShellFolder_Release(IDesktopFolder);
    IShellFolder_Release(testIShellFolder);

    ILFree(newPIDL);

cleanup:
    DeleteFileA(".\\testfolder\\desktop.ini");
    SetFileAttributesA(".\\testfolder", GetFileAttributesA(".\\testfolder")&~FILE_ATTRIBUTE_SYSTEM);
    RemoveDirectoryA(".\\testfolder");
}

static void test_SHCreateShellItem(void)
{
    IShellItem *shellitem, *shellitem2;
    IPersistIDList *persistidl;
    LPITEMIDLIST pidl_cwd=NULL, pidl_testfile, pidl_abstestfile, pidl_test, pidl_desktop;
    HRESULT ret;
    char curdirA[MAX_PATH];
    WCHAR curdirW[MAX_PATH];
    WCHAR fnbufW[MAX_PATH];
    IShellFolder *desktopfolder=NULL, *currentfolder=NULL;
    static WCHAR testfileW[] = L"testfile";

    GetCurrentDirectoryA(MAX_PATH, curdirA);

    if (!pSHCreateShellItem)
    {
        win_skip("SHCreateShellItem isn't available\n");
        return;
    }

    if (!curdirA[0])
    {
        win_skip("GetCurrentDirectoryA returned empty string, skipping test_SHCreateShellItem\n");
        return;
    }

    ret = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl_desktop);
    ok(ret == S_OK, "Got 0x%08lx\n", ret);

    MultiByteToWideChar(CP_ACP, 0, curdirA, -1, curdirW, MAX_PATH);

    ret = SHGetDesktopFolder(&desktopfolder);
    ok(SUCCEEDED(ret), "SHGetShellFolder returned %lx\n", ret);

    ret = IShellFolder_ParseDisplayName(desktopfolder, NULL, NULL, curdirW, NULL, &pidl_cwd, NULL);
    ok(SUCCEEDED(ret), "ParseDisplayName returned %lx\n", ret);

    ret = IShellFolder_BindToObject(desktopfolder, pidl_cwd, NULL, &IID_IShellFolder, (void**)&currentfolder);
    ok(SUCCEEDED(ret), "BindToObject returned %lx\n", ret);

    CreateTestFile(".\\testfile");

    ret = IShellFolder_ParseDisplayName(currentfolder, NULL, NULL, testfileW, NULL, &pidl_testfile, NULL);
    ok(SUCCEEDED(ret), "ParseDisplayName returned %lx\n", ret);

    pidl_abstestfile = ILCombine(pidl_cwd, pidl_testfile);

    shellitem = (void*)0xdeadbeef;
    ret = pSHCreateShellItem(NULL, NULL, NULL, &shellitem);
    ok(ret == E_INVALIDARG, "SHCreateShellItem returned %lx\n", ret);
    ok(shellitem == 0, "Got %p\n", shellitem);

    if (0) /* crashes on Windows XP */
    {
        pSHCreateShellItem(NULL, NULL, pidl_cwd, NULL);
        pSHCreateShellItem(pidl_cwd, NULL, NULL, &shellitem);
        pSHCreateShellItem(NULL, currentfolder, NULL, &shellitem);
        pSHCreateShellItem(pidl_cwd, currentfolder, NULL, &shellitem);
    }

    ret = pSHCreateShellItem(NULL, NULL, pidl_cwd, &shellitem);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
    if (SUCCEEDED(ret))
    {
        ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
        ok(SUCCEEDED(ret), "QueryInterface returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
            ok(SUCCEEDED(ret), "GetIDList returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ok(ILIsEqual(pidl_cwd, pidl_test), "id lists are not equal\n");
                ILFree(pidl_test);
            }
            IPersistIDList_Release(persistidl);
        }
        IShellItem_Release(shellitem);
    }

    ret = pSHCreateShellItem(pidl_cwd, NULL, pidl_testfile, &shellitem);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
    if (SUCCEEDED(ret))
    {
        ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
        ok(SUCCEEDED(ret), "QueryInterface returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
            ok(SUCCEEDED(ret), "GetIDList returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ok(ILIsEqual(pidl_abstestfile, pidl_test), "id lists are not equal\n");
                ILFree(pidl_test);
            }
            IPersistIDList_Release(persistidl);
        }

        ret = IShellItem_GetParent(shellitem, &shellitem2);
        ok(SUCCEEDED(ret), "GetParent returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IShellItem_QueryInterface(shellitem2, &IID_IPersistIDList, (void**)&persistidl);
            ok(SUCCEEDED(ret), "QueryInterface returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
                ok(SUCCEEDED(ret), "GetIDList returned %lx\n", ret);
                if (SUCCEEDED(ret))
                {
                    ok(ILIsEqual(pidl_cwd, pidl_test), "id lists are not equal\n");
                    ILFree(pidl_test);
                }
                IPersistIDList_Release(persistidl);
            }
            IShellItem_Release(shellitem2);
        }

        IShellItem_Release(shellitem);
    }

    ret = pSHCreateShellItem(NULL, currentfolder, pidl_testfile, &shellitem);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
    if (SUCCEEDED(ret))
    {
        ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
        ok(SUCCEEDED(ret), "QueryInterface returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
            ok(SUCCEEDED(ret), "GetIDList returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ok(ILIsEqual(pidl_abstestfile, pidl_test), "id lists are not equal\n");
                ILFree(pidl_test);
            }
            IPersistIDList_Release(persistidl);
        }
        IShellItem_Release(shellitem);
    }

    /* if a parent pidl and shellfolder are specified, the shellfolder is ignored */
    ret = pSHCreateShellItem(pidl_cwd, desktopfolder, pidl_testfile, &shellitem);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
    if (SUCCEEDED(ret))
    {
        ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
        ok(SUCCEEDED(ret), "QueryInterface returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
            ok(SUCCEEDED(ret), "GetIDList returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ok(ILIsEqual(pidl_abstestfile, pidl_test), "id lists are not equal\n");
                ILFree(pidl_test);
            }
            IPersistIDList_Release(persistidl);
        }
        IShellItem_Release(shellitem);
    }

    ret = pSHCreateShellItem(NULL, desktopfolder, pidl_testfile, &shellitem);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
    if (SUCCEEDED(ret))
    {
        ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
        ok(SUCCEEDED(ret), "QueryInterface returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
            ok(SUCCEEDED(ret), "GetIDList returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ok(ILIsEqual(pidl_testfile, pidl_test), "id lists are not equal\n");
                ILFree(pidl_test);
            }
            IPersistIDList_Release(persistidl);
        }

        IShellItem_Release(shellitem);
    }

    ret = pSHCreateShellItem(NULL, NULL, pidl_desktop, &shellitem);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
    if (SUCCEEDED(ret))
    {
        ret = IShellItem_GetParent(shellitem, &shellitem2);
        ok(FAILED(ret), "Got 0x%08lx\n", ret);
        if(SUCCEEDED(ret)) IShellItem_Release(shellitem2);
        IShellItem_Release(shellitem);
    }

    /* SHCreateItemFromParsingName */
    if(pSHCreateItemFromParsingName)
    {
        if(0)
        {
            /* Crashes under windows 7 */
            pSHCreateItemFromParsingName(NULL, NULL, &IID_IShellItem, NULL);
        }

        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemFromParsingName(NULL, NULL, &IID_IShellItem, (void**)&shellitem);
        ok(ret == E_INVALIDARG, "SHCreateItemFromParsingName returned %lx\n", ret);
        ok(shellitem == NULL, "shellitem was %p.\n", shellitem);

        ret = pSHCreateItemFromParsingName(testfileW, NULL, &IID_IShellItem, (void**)&shellitem);
        ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
           "SHCreateItemFromParsingName returned %lx\n", ret);
        if(SUCCEEDED(ret)) IShellItem_Release(shellitem);

        lstrcpyW(fnbufW, curdirW);
        myPathAddBackslashW(fnbufW);
        lstrcatW(fnbufW, testfileW);

        ret = pSHCreateItemFromParsingName(fnbufW, NULL, &IID_IShellItem, (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemFromParsingName returned %lx\n", ret);
        if(SUCCEEDED(ret))
        {
            LPWSTR tmp_fname;
            ret = IShellItem_GetDisplayName(shellitem, SIGDN_FILESYSPATH, &tmp_fname);
            ok(ret == S_OK, "GetDisplayName returned %lx\n", ret);
            if(SUCCEEDED(ret))
            {
                ok(!lstrcmpW(fnbufW, tmp_fname), "strings not equal\n");
                CoTaskMemFree(tmp_fname);
            }
            IShellItem_Release(shellitem);
        }
    }
    else
        win_skip("No SHCreateItemFromParsingName\n");


    /* SHCreateItemFromIDList */
    if(pSHCreateItemFromIDList)
    {
        if(0)
        {
            /* Crashes under win7 */
            pSHCreateItemFromIDList(NULL, &IID_IShellItem, NULL);
        }

        ret = pSHCreateItemFromIDList(NULL, &IID_IShellItem, (void**)&shellitem);
        ok(ret == E_INVALIDARG, "SHCreateItemFromIDList returned %lx\n", ret);

        ret = pSHCreateItemFromIDList(pidl_cwd, &IID_IShellItem, (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemFromIDList returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
            ok(ret == S_OK, "QueryInterface returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
                ok(ret == S_OK, "GetIDList returned %lx\n", ret);
                if (SUCCEEDED(ret))
                {
                    ok(ILIsEqual(pidl_cwd, pidl_test), "id lists are not equal\n");
                    ILFree(pidl_test);
                }
                IPersistIDList_Release(persistidl);
            }
            IShellItem_Release(shellitem);
        }

        ret = pSHCreateItemFromIDList(pidl_testfile, &IID_IShellItem, (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemFromIDList returned %lx\n", ret);
        if (SUCCEEDED(ret))
        {
            ret = IShellItem_QueryInterface(shellitem, &IID_IPersistIDList, (void**)&persistidl);
            ok(ret == S_OK, "QueryInterface returned %lx\n", ret);
            if (SUCCEEDED(ret))
            {
                ret = IPersistIDList_GetIDList(persistidl, &pidl_test);
                ok(ret == S_OK, "GetIDList returned %lx\n", ret);
                if (SUCCEEDED(ret))
                {
                    ok(ILIsEqual(pidl_testfile, pidl_test), "id lists are not equal\n");
                    ILFree(pidl_test);
                }
                IPersistIDList_Release(persistidl);
            }
            IShellItem_Release(shellitem);
        }
    }
    else
        win_skip("No SHCreateItemFromIDList\n");

    /* SHCreateItemFromRelativeName */
    if(pSHCreateItemFromRelativeName && pSHGetKnownFolderPath)
    {
        IShellItem *shellitem_desktop = NULL;
        WCHAR *desktop_path, *displayname;
        WCHAR testfile_path[MAX_PATH] = {0};
        HANDLE file;
        LPITEMIDLIST pidl_desktop_testfile = NULL;
        int order;

        ret = pSHCreateShellItem(NULL, NULL, pidl_desktop, &shellitem_desktop);
        ok(ret == S_OK, "SHCreateShellItem failed: 0x%08lx.\n", ret);

        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemFromRelativeName(shellitem_desktop, NULL, NULL, &IID_IShellItem,
                                            (void**)&shellitem);
        ok(ret == E_INVALIDARG, "Expected 0x%08lx but SHCreateItemFromRelativeName return: 0x%08lx.\n",
           E_INVALIDARG, ret);
        ok(shellitem == NULL, "shellitem was %p.\n", shellitem);

        /* Test with a non-existent file */
        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemFromRelativeName(shellitem_desktop, testfileW, NULL, &IID_IShellItem,
                                            (void**)&shellitem);
        ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
           "Expected 0x%08lx but SHCreateItemFromRelativeName return: 0x%08lx.\n",
           HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), ret);
        ok(shellitem == NULL, "shellitem was %p.\n", shellitem);

        /* Create a file for testing in desktop folder */
        pSHGetKnownFolderPath(&FOLDERID_Desktop, 0, NULL, &desktop_path);
        lstrcatW(testfile_path, desktop_path);
        myPathAddBackslashW(testfile_path);
        lstrcatW(testfile_path, testfileW);
        file = CreateFileW(testfile_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFileW failed! Last error: 0x%08lx.\n", GetLastError());
        CloseHandle(file);

        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemFromRelativeName(shellitem_desktop, testfileW, NULL, &IID_IShellItem,
                                            (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemFromRelativeName failed: 0x%08lx.\n", ret);
        ok(shellitem != NULL, "shellitem was %p.\n", shellitem);
        if(SUCCEEDED(ret))
        {
            ret = IShellItem_GetDisplayName(shellitem, 0, &displayname);
            ok(ret == S_OK, "IShellItem_GetDisplayName failed: 0x%08lx.\n", ret);
            ok(!lstrcmpW(displayname, testfileW), "got wrong display name: %s.\n", wine_dbgstr_w(displayname));
            CoTaskMemFree(displayname);

            shellitem2 = (void*)0xdeadbeef;
            ret = pSHCreateItemFromRelativeName(shellitem_desktop, testfileW, NULL, &IID_IShellItem,
                                                (void**)&shellitem2);
            ok(ret == S_OK, "SHCreateItemFromRelativeName failed: 0x%08lx.\n", ret);
            ret = IShellItem_Compare(shellitem, shellitem2, 0, &order);
            ok(ret == S_OK, "IShellItem_Compare failed: 0x%08lx.\n", ret);
            ok(!order, "order got wrong value: %d.\n", order);
            IShellItem_Release(shellitem2);

            shellitem2 = (void*)0xdeadbeef;
            ret = IShellFolder_ParseDisplayName(desktopfolder, NULL, NULL, testfileW, NULL,
                                                &pidl_desktop_testfile, NULL);
            ok(ret == S_OK, "ParseDisplayName failed 0x%08lx.\n", ret);
            ret = pSHCreateItemFromIDList(pidl_desktop_testfile, &IID_IShellItem, (void**)&shellitem2);
            ret = IShellItem_Compare(shellitem, shellitem2, 0, &order);
            ok(ret == S_OK, "IShellItem_Compare fail: 0x%08lx.\n", ret);
            ok(!order, "order got wrong value: %d.\n", order);
            ILFree(pidl_desktop_testfile);
            IShellItem_Release(shellitem2);

            IShellItem_Release(shellitem);
        }

        DeleteFileW(testfile_path);
        CoTaskMemFree(desktop_path);
        IShellItem_Release(shellitem_desktop);
    }
    else
        win_skip("No SHCreateItemFromRelativeName or SHGetKnownFolderPath\n");

    /* SHCreateItemInKnownFolder */
    if(pSHCreateItemInKnownFolder && pSHGetKnownFolderPath)
    {
        WCHAR *desktop_path;
        WCHAR testfile_path[MAX_PATH] = {0};
        HANDLE file;
        WCHAR *displayname = NULL;
        int order;
        LPITEMIDLIST pidl_desktop_testfile = NULL;

        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemInKnownFolder(&FOLDERID_Desktop, 0, NULL, &IID_IShellItem,
                                         (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemInKnownFolder failed: 0x%08lx.\n", ret);
        ok(shellitem != NULL, "shellitem was %p.\n", shellitem);
        if(SUCCEEDED(ret))
        {
            shellitem2 = (void*)0xdeadbeef;
            ret = pSHCreateShellItem(NULL, NULL, pidl_desktop, &shellitem2);
            ok(SUCCEEDED(ret), "SHCreateShellItem returned %lx\n", ret);
            if(SUCCEEDED(ret))
            {
                ret = IShellItem_Compare(shellitem, shellitem2, 0, &order);
                ok(ret == S_OK, "IShellItem_Compare failed: 0x%08lx.\n", ret);
                ok(!order, "order got wrong value: %d.\n", order);
                IShellItem_Release(shellitem2);
            }
            IShellItem_Release(shellitem);
        }

        /* Test with a non-existent file */
        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemInKnownFolder(&FOLDERID_Desktop, 0, testfileW, &IID_IShellItem,
                                         (void**)&shellitem);
        ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
           "Expected 0x%08lx but SHCreateItemInKnownFolder return: 0x%08lx.\n",
           HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), ret);
        ok(shellitem == NULL, "shellitem was %p.\n", shellitem);

        pSHGetKnownFolderPath(&FOLDERID_Desktop, 0, NULL, &desktop_path);
        lstrcatW(testfile_path, desktop_path);
        myPathAddBackslashW(testfile_path);
        lstrcatW(testfile_path, testfileW);
        file = CreateFileW(testfile_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFileW failed! Last error: 0x%08lx.\n", GetLastError());
        CloseHandle(file);

        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemInKnownFolder(&FOLDERID_Desktop, 0, testfileW, &IID_IShellItem,
                                         (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemInKnownFolder failed: 0x%08lx.\n", ret);
        ok(shellitem != NULL, "shellitem was %p.\n", shellitem);
        if(SUCCEEDED(ret))
        {
            ret = IShellItem_GetDisplayName(shellitem, 0, &displayname);
            ok(ret == S_OK, "IShellItem_GetDisplayName failed: 0x%08lx.\n", ret);
            ok(!lstrcmpW(displayname, testfileW), "got wrong display name: %s.\n",
               wine_dbgstr_w(displayname));
            CoTaskMemFree(displayname);

            shellitem2 = (void*)0xdeadbeef;
            ret = pSHCreateItemInKnownFolder(&FOLDERID_Desktop, 0, testfileW, &IID_IShellItem,
                                             (void**)&shellitem2);
            ok(ret == S_OK, "SHCreateItemInKnownFolder failed: 0x%08lx.\n", ret);
            ok(shellitem2 != NULL, "shellitem was %p.\n", shellitem);
            ret = IShellItem_Compare(shellitem, shellitem2, 0, &order);
            ok(ret == S_OK, "IShellItem_Compare failed: 0x%08lx.\n", ret);
            ok(!order, "order got wrong value: %d.\n", order);
            IShellItem_Release(shellitem2);

            shellitem2 = (void*)0xdeadbeef;
            ret = IShellFolder_ParseDisplayName(desktopfolder, NULL, NULL, testfileW, NULL,
                                                &pidl_desktop_testfile, NULL);
            ok(SUCCEEDED(ret), "ParseDisplayName returned %lx.\n", ret);
            ret = pSHCreateItemFromIDList(pidl_desktop_testfile, &IID_IShellItem, (void**)&shellitem2);
            ret = IShellItem_Compare(shellitem, shellitem2, 0, &order);
            ok(ret == S_OK, "IShellItem_Compare failed: 0x%08lx.\n", ret);
            ok(!order, "order got wrong value: %d.\n", order);
            ILFree(pidl_desktop_testfile);
            IShellItem_Release(shellitem2);

            IShellItem_Release(shellitem);
        }

        shellitem = (void*)0xdeadbeef;
        ret = pSHCreateItemInKnownFolder(&FOLDERID_Documents, 0, NULL, &IID_IShellItem,
                                         (void**)&shellitem);
        ok(ret == S_OK, "SHCreateItemInKnownFolder failed: 0x%08lx.\n", ret);
        ok(shellitem != NULL, "shellitem was %p.\n", shellitem);
        if(SUCCEEDED(ret))
        {
            shellitem2 = (void*)0xdeadbeef;
            ret = pSHCreateItemInKnownFolder(&FOLDERID_Documents, 0, NULL, &IID_IShellItem,
                                             (void**)&shellitem2);
            ok(ret == S_OK, "SHCreateItemInKnownFolder failed: 0x%08lx.\n", ret);
            ok(shellitem2 != NULL, "shellitem was %p.\n", shellitem);
            ret = IShellItem_Compare(shellitem, shellitem2, 0, &order);
            ok(ret == S_OK, "IShellItem_Compare failed: 0x%08lx.\n", ret);
            ok(!order, "order got wrong value: %d.\n", order);
            IShellItem_Release(shellitem2);

            IShellItem_Release(shellitem);
        }
        DeleteFileW(testfile_path);
        CoTaskMemFree(desktop_path);
    }
    else
        win_skip("No SHCreateItemInKnownFolder or SHGetKnownFolderPath\n");

    DeleteFileA(".\\testfile");
    ILFree(pidl_abstestfile);
    ILFree(pidl_testfile);
    ILFree(pidl_desktop);
    ILFree(pidl_cwd);
    IShellFolder_Release(currentfolder);
    IShellFolder_Release(desktopfolder);
}

static void test_SHGetNameFromIDList(void)
{
    IShellItem *shellitem;
    LPITEMIDLIST pidl;
    LPWSTR name_string;
    HRESULT hres;
    UINT i;
    static const DWORD flags[] = {
        SIGDN_NORMALDISPLAY, SIGDN_PARENTRELATIVEPARSING,
        SIGDN_DESKTOPABSOLUTEPARSING,SIGDN_PARENTRELATIVEEDITING,
        SIGDN_DESKTOPABSOLUTEEDITING, /*SIGDN_FILESYSPATH, SIGDN_URL, */
        SIGDN_PARENTRELATIVEFORADDRESSBAR,SIGDN_PARENTRELATIVE, -1234};

    if(!pSHGetNameFromIDList)
    {
        win_skip("SHGetNameFromIDList missing.\n");
        return;
    }

    /* This should be available on any platform that passed the above test. */
    ok(pSHCreateShellItem != NULL, "SHCreateShellItem missing.\n");

    if(0)
    {
        /* Crashes under win7 */
        pSHGetNameFromIDList(NULL, 0, NULL);
    }

    hres = pSHGetNameFromIDList(NULL, 0, &name_string);
    ok(hres == E_INVALIDARG, "Got 0x%08lx\n", hres);

    /* Test the desktop */
    hres = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    hres = pSHCreateShellItem(NULL, NULL, pidl, &shellitem);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    if(SUCCEEDED(hres))
    {
        WCHAR *nameSI, *nameSH;
        WCHAR buf[MAX_PATH];
        HRESULT hrSI, hrSH, hrSF;
        STRRET strret;
        IShellFolder *psf;
        BOOL res;

        SHGetDesktopFolder(&psf);
        for(i = 0; flags[i] != -1234; i++)
        {
            hrSI = IShellItem_GetDisplayName(shellitem, flags[i], &nameSI);
            ok(hrSI == S_OK, "Got 0x%08lx\n", hrSI);
            hrSH = pSHGetNameFromIDList(pidl, flags[i], &nameSH);
            ok(hrSH == S_OK, "Got 0x%08lx\n", hrSH);
            hrSF = IShellFolder_GetDisplayNameOf(psf, pidl, flags[i] & 0xffff, &strret);
            ok(hrSF == S_OK, "Got 0x%08lx\n", hrSF);

            if(SUCCEEDED(hrSI) && SUCCEEDED(hrSH))
                ok(!lstrcmpW(nameSI, nameSH), "Strings differ.\n");

            if(SUCCEEDED(hrSF))
            {
                StrRetToBufW(&strret, NULL, buf, MAX_PATH);
                if(SUCCEEDED(hrSI))
                    ok(!lstrcmpW(nameSI, buf), "Strings differ.\n");
                if(SUCCEEDED(hrSF))
                    ok(!lstrcmpW(nameSI, buf), "Strings differ.\n");
            }
            if(SUCCEEDED(hrSI)) CoTaskMemFree(nameSI);
            if(SUCCEEDED(hrSH)) CoTaskMemFree(nameSH);
        }
        IShellFolder_Release(psf);

        hrSI = pSHGetNameFromIDList(pidl, SIGDN_FILESYSPATH, &nameSI);
        ok(hrSI == S_OK, "Got 0x%08lx\n", hrSI);
        res = SHGetPathFromIDListW(pidl, buf);
        ok(res == TRUE, "Got %d\n", res);
        if(SUCCEEDED(hrSI) && res)
            ok(!lstrcmpW(nameSI, buf), "Strings differ.\n");
        if(SUCCEEDED(hrSI)) CoTaskMemFree(nameSI);

        hres = pSHGetNameFromIDList(pidl, SIGDN_URL, &name_string);
        todo_wine ok(hres == S_OK, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) CoTaskMemFree(name_string);

        IShellItem_Release(shellitem);
    }
    ILFree(pidl);

    /* Test the control panel */
    hres = SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidl);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    hres = pSHCreateShellItem(NULL, NULL, pidl, &shellitem);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    if(SUCCEEDED(hres))
    {
        WCHAR *nameSI, *nameSH;
        WCHAR buf[MAX_PATH];
        HRESULT hrSI, hrSH, hrSF;
        STRRET strret;
        IShellFolder *psf;
        BOOL res;

        SHGetDesktopFolder(&psf);
        for(i = 0; flags[i] != -1234; i++)
        {
            hrSI = IShellItem_GetDisplayName(shellitem, flags[i], &nameSI);
            ok(hrSI == S_OK, "Got 0x%08lx\n", hrSI);
            hrSH = pSHGetNameFromIDList(pidl, flags[i], &nameSH);
            ok(hrSH == S_OK, "Got 0x%08lx\n", hrSH);
            hrSF = IShellFolder_GetDisplayNameOf(psf, pidl, flags[i] & 0xffff, &strret);
            ok(hrSF == S_OK, "Got 0x%08lx\n", hrSF);

            if(SUCCEEDED(hrSI) && SUCCEEDED(hrSH))
                ok(!lstrcmpW(nameSI, nameSH), "Strings differ.\n");

            if(SUCCEEDED(hrSF))
            {
                StrRetToBufW(&strret, NULL, buf, MAX_PATH);
                if(SUCCEEDED(hrSI))
                    ok(!lstrcmpW(nameSI, buf), "Strings differ.\n");
                if(SUCCEEDED(hrSF))
                    ok(!lstrcmpW(nameSI, buf), "Strings differ.\n");
            }
            if(SUCCEEDED(hrSI)) CoTaskMemFree(nameSI);
            if(SUCCEEDED(hrSH)) CoTaskMemFree(nameSH);
        }
        IShellFolder_Release(psf);

        hrSI = pSHGetNameFromIDList(pidl, SIGDN_FILESYSPATH, &nameSI);
        ok(hrSI == E_INVALIDARG, "Got 0x%08lx\n", hrSI);
        res = SHGetPathFromIDListW(pidl, buf);
        ok(res == FALSE, "Got %d\n", res);
        if(SUCCEEDED(hrSI) && res)
            ok(!lstrcmpW(nameSI, buf), "Strings differ.\n");
        if(SUCCEEDED(hrSI)) CoTaskMemFree(nameSI);

        hres = pSHGetNameFromIDList(pidl, SIGDN_URL, &name_string);
        todo_wine ok(hres == E_NOTIMPL /* Win7 */ || hres == S_OK /* Vista */,
                     "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) CoTaskMemFree(name_string);

        IShellItem_Release(shellitem);
    }
    ILFree(pidl);
}

static void test_SHGetItemFromDataObject(void)
{
    IShellFolder *psfdesktop;
    IShellItem *psi;
    IShellView *psv;
    HRESULT hres;

    if(!pSHGetItemFromDataObject)
    {
        win_skip("No SHGetItemFromDataObject.\n");
        return;
    }

    if(0)
    {
        /* Crashes under win7 */
        pSHGetItemFromDataObject(NULL, 0, &IID_IShellItem, NULL);
    }

    hres = pSHGetItemFromDataObject(NULL, 0, &IID_IShellItem, (void**)&psv);
    ok(hres == E_INVALIDARG, "got 0x%08lx\n", hres);

    SHGetDesktopFolder(&psfdesktop);

    hres = IShellFolder_CreateViewObject(psfdesktop, NULL, &IID_IShellView, (void**)&psv);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    if(SUCCEEDED(hres))
    {
        IEnumIDList *peidl;
        IDataObject *pdo;
        SHCONTF enum_flags;

        enum_flags = SHCONTF_NONFOLDERS | SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN;
        hres = IShellFolder_EnumObjects(psfdesktop, NULL, enum_flags, &peidl);
        ok(hres == S_OK, "got 0x%08lx\n", hres);
        if(SUCCEEDED(hres))
        {
            LPITEMIDLIST apidl[5];
            UINT count = 0, i;

            for(count = 0; count < 5; count++)
                if(IEnumIDList_Next(peidl, 1, &apidl[count], NULL) != S_OK)
                    break;

            if(count)
            {
                hres = IShellFolder_GetUIObjectOf(psfdesktop, NULL, 1, (LPCITEMIDLIST*)apidl,
                                                  &IID_IDataObject, NULL, (void**)&pdo);
                ok(hres == S_OK, "got 0x%08lx\n", hres);
                if(SUCCEEDED(hres))
                {
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_DEFAULT, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_TRAVERSE_LINK, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_NO_HDROP, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_NO_URL, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_ONLY_IF_ONE, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);

                    IDataObject_Release(pdo);
                }
            }
            else
                skip("No file(s) found - skipping single-file test.\n");

            if(count > 1)
            {
                hres = IShellFolder_GetUIObjectOf(psfdesktop, NULL, count, (LPCITEMIDLIST*)apidl,
                                                  &IID_IDataObject, NULL, (void**)&pdo);
                ok(hres == S_OK, "got 0x%08lx\n", hres);
                if(SUCCEEDED(hres))
                {
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_DEFAULT, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_TRAVERSE_LINK, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_NO_HDROP, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_NO_URL, &IID_IShellItem, (void**)&psi);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    hres = pSHGetItemFromDataObject(pdo, DOGIF_ONLY_IF_ONE, &IID_IShellItem, (void**)&psi);
                    ok(hres == E_FAIL, "got 0x%08lx\n", hres);
                    if(SUCCEEDED(hres)) IShellItem_Release(psi);
                    IDataObject_Release(pdo);
                }
            }
            else
                skip("zero or one file found - skipping multi-file test.\n");

            for(i = 0; i < count; i++)
                ILFree(apidl[i]);

            IEnumIDList_Release(peidl);
        }

        IShellView_Release(psv);
    }

    IShellFolder_Release(psfdesktop);
}

static void test_ShellItemCompare(void)
{
    IShellItem *psi[9]; /* a\a, a\b, a\c, b\a, .. */
    IShellItem *psi_a = NULL, *psi_b = NULL, *psi_c = NULL;
    IShellFolder *psf_desktop, *psf_current;
    LPITEMIDLIST pidl_cwd;
    WCHAR curdirW[MAX_PATH];
    BOOL failed;
    HRESULT hr;
    static const WCHAR filesW[][9] =
        { L"a\\a", L"a\\b", L"a\\c", L"b\\a", L"b\\b", L"b\\c", L"c\\a", L"c\\b", L"c\\c" };
    int order;
    UINT i;

    if(!pSHCreateShellItem)
    {
        win_skip("SHCreateShellItem missing.\n");
        return;
    }

    GetCurrentDirectoryW(MAX_PATH, curdirW);
    if (!curdirW[0])
    {
        skip("Failed to get current directory, skipping.\n");
        return;
    }

    CreateDirectoryA(".\\a", NULL);
    CreateDirectoryA(".\\b", NULL);
    CreateDirectoryA(".\\c", NULL);
    CreateTestFile(".\\a\\a");
    CreateTestFile(".\\a\\b");
    CreateTestFile(".\\a\\c");
    CreateTestFile(".\\b\\a");
    CreateTestFile(".\\b\\b");
    CreateTestFile(".\\b\\c");
    CreateTestFile(".\\c\\a");
    CreateTestFile(".\\c\\b");
    CreateTestFile(".\\c\\c");

    SHGetDesktopFolder(&psf_desktop);
    hr = IShellFolder_ParseDisplayName(psf_desktop, NULL, NULL, curdirW, NULL, &pidl_cwd, NULL);
    ok(SUCCEEDED(hr), "ParseDisplayName returned %lx\n", hr);
    hr = IShellFolder_BindToObject(psf_desktop, pidl_cwd, NULL, &IID_IShellFolder, (void**)&psf_current);
    ok(SUCCEEDED(hr), "BindToObject returned %lx\n", hr);
    IShellFolder_Release(psf_desktop);
    ILFree(pidl_cwd);

    /* Generate ShellItems for the files */
    memset(&psi, 0, sizeof(psi));
    failed = FALSE;
    for(i = 0; i < 9; i++)
    {
        LPITEMIDLIST pidl_testfile = NULL;

        hr = IShellFolder_ParseDisplayName(psf_current, NULL, NULL, (LPWSTR)filesW[i],
                                           NULL, &pidl_testfile, NULL);
        ok(SUCCEEDED(hr), "ParseDisplayName returned %lx\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = pSHCreateShellItem(NULL, NULL, pidl_testfile, &psi[i]);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            ILFree(pidl_testfile);
        }
        if(FAILED(hr)) failed = TRUE;
    }
    if(failed)
    {
        skip("Failed to create all shellitems.\n");
        goto cleanup;
    }

    /* Generate ShellItems for the folders */
    hr = IShellItem_GetParent(psi[0], &psi_a);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(FAILED(hr)) failed = TRUE;
    hr = IShellItem_GetParent(psi[3], &psi_b);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(FAILED(hr)) failed = TRUE;
    hr = IShellItem_GetParent(psi[6], &psi_c);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(FAILED(hr)) failed = TRUE;

    if(failed)
    {
        skip("Failed to create shellitems.\n");
        goto cleanup;
    }

    if(0)
    {
        /* Crashes on native (win7, winxp) */
        IShellItem_Compare(psi_a, NULL, 0, NULL);
        IShellItem_Compare(psi_a, psi_b, 0, NULL);
        IShellItem_Compare(psi_a, NULL, 0, &order);
    }

    /* Basics */
    for(i = 0; i < 9; i++)
    {
        hr = IShellItem_Compare(psi[i], psi[i], SICHINT_DISPLAY, &order);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        ok(order == 0, "Got order %d\n", order);
        hr = IShellItem_Compare(psi[i], psi[i], SICHINT_CANONICAL, &order);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        ok(order == 0, "Got order %d\n", order);
        hr = IShellItem_Compare(psi[i], psi[i], SICHINT_ALLFIELDS, &order);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        ok(order == 0, "Got order %d\n", order);
    }

    /* Order */
    /* a\b:a\a , a\b:a\c, a\b:a\b */
    hr = IShellItem_Compare(psi[1], psi[0], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[1], psi[2], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[1], psi[1], SICHINT_DISPLAY, &order);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(order == 0, "Got order %d\n", order);

    /* b\b:a\b, b\b:c\b, b\b:c\b */
    hr = IShellItem_Compare(psi[4], psi[1], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[4], psi[7], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[4], psi[4], SICHINT_DISPLAY, &order);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(order == 0, "Got order %d\n", order);

    /* b:a\a, b:a\c, b:a\b */
    hr = IShellItem_Compare(psi_b, psi[0], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    todo_wine ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[2], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    todo_wine ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[1], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    todo_wine ok(order == 1, "Got order %d\n", order);

    /* b:c\a, b:c\c, b:c\b */
    hr = IShellItem_Compare(psi_b, psi[6], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[8], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[7], SICHINT_DISPLAY, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);

    /* a\b:a\a , a\b:a\c, a\b:a\b */
    hr = IShellItem_Compare(psi[1], psi[0], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[1], psi[2], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[1], psi[1], SICHINT_CANONICAL, &order);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(order == 0, "Got order %d\n", order);

    /* b\b:a\b, b\b:c\b, b\b:c\b */
    hr = IShellItem_Compare(psi[4], psi[1], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[4], psi[7], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi[4], psi[4], SICHINT_CANONICAL, &order);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(order == 0, "Got order %d\n", order);

    /* b:a\a, b:a\c, b:a\b */
    hr = IShellItem_Compare(psi_b, psi[0], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    todo_wine ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[2], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    todo_wine ok(order == 1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[1], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    todo_wine ok(order == 1, "Got order %d\n", order);

    /* b:c\a, b:c\c, b:c\b */
    hr = IShellItem_Compare(psi_b, psi[6], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[8], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);
    hr = IShellItem_Compare(psi_b, psi[7], SICHINT_CANONICAL, &order);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(order == -1, "Got order %d\n", order);

cleanup:
    IShellFolder_Release(psf_current);

    DeleteFileA(".\\a\\a");
    DeleteFileA(".\\a\\b");
    DeleteFileA(".\\a\\c");
    DeleteFileA(".\\b\\a");
    DeleteFileA(".\\b\\b");
    DeleteFileA(".\\b\\c");
    DeleteFileA(".\\c\\a");
    DeleteFileA(".\\c\\b");
    DeleteFileA(".\\c\\c");
    RemoveDirectoryA(".\\a");
    RemoveDirectoryA(".\\b");
    RemoveDirectoryA(".\\c");

    if(psi_a) IShellItem_Release(psi_a);
    if(psi_b) IShellItem_Release(psi_b);
    if(psi_c) IShellItem_Release(psi_c);

    for(i = 0; i < 9; i++)
        if(psi[i]) IShellItem_Release(psi[i]);
}

/**************************************************************/
/* IUnknown implementation for counting QueryInterface calls. */
typedef struct {
    IUnknown IUnknown_iface;
    struct if_count {
        REFIID id;
        LONG count;
    } *ifaces;
    LONG unknown;
} IUnknownImpl;

static inline IUnknownImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IUnknownImpl, IUnknown_iface);
}

static HRESULT WINAPI unk_fnQueryInterface(IUnknown *iunk, REFIID riid, void** punk)
{
    IUnknownImpl *This = impl_from_IUnknown(iunk);
    UINT i;
    BOOL found = FALSE;
    for(i = 0; This->ifaces[i].id != NULL; i++)
    {
        if(IsEqualIID(This->ifaces[i].id, riid))
        {
            This->ifaces[i].count++;
            found = TRUE;
            break;
        }
    }
    if(!found)
        This->unknown++;
    return E_NOINTERFACE;
}

static ULONG WINAPI unk_fnAddRef(IUnknown *iunk)
{
    return 2;
}

static ULONG WINAPI unk_fnRelease(IUnknown *iunk)
{
    return 1;
}

static const IUnknownVtbl vt_IUnknown = {
    unk_fnQueryInterface,
    unk_fnAddRef,
    unk_fnRelease
};

static void test_SHGetIDListFromObject(void)
{
    IUnknownImpl *punkimpl;
    IShellFolder *psfdesktop;
    IShellView *psv;
    LPITEMIDLIST pidl, pidl_desktop;
    HRESULT hres;
    UINT i;
    struct if_count ifaces[] =
        { {&IID_IPersistIDList, 0},
          {&IID_IPersistFolder2, 0},
          {&IID_IDataObject, 0},
          {&IID_IParentAndItem, 0},
          {&IID_IFolderView, 0},
          {NULL, 0} };

    if(!pSHGetIDListFromObject)
    {
        win_skip("SHGetIDListFromObject missing.\n");
        return;
    }

    if(0)
    {
        /* Crashes native */
        pSHGetIDListFromObject(NULL, NULL);
        pSHGetIDListFromObject((void*)0xDEADBEEF, NULL);
    }

    hres = pSHGetIDListFromObject(NULL, &pidl);
    ok(hres == E_NOINTERFACE, "Got %lx\n", hres);

    punkimpl = malloc(sizeof(*punkimpl));
    punkimpl->IUnknown_iface.lpVtbl = &vt_IUnknown;
    punkimpl->ifaces = ifaces;
    punkimpl->unknown = 0;

    hres = pSHGetIDListFromObject(&punkimpl->IUnknown_iface, &pidl);
    ok(hres == E_NOINTERFACE, "Got %lx\n", hres);
    ok(ifaces[0].count, "interface not requested.\n");
    ok(ifaces[1].count, "interface not requested.\n");
    ok(ifaces[2].count, "interface not requested.\n");
    todo_wine
        ok(ifaces[3].count || broken(!ifaces[3].count /*vista*/),
           "interface not requested.\n");
    ok(ifaces[4].count || broken(!ifaces[4].count /*vista*/),
       "interface not requested.\n");

    ok(!punkimpl->unknown, "Got %ld unknown.\n", punkimpl->unknown);
    free(punkimpl);

    pidl_desktop = NULL;
    SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl_desktop);
    ok(pidl_desktop != NULL, "Failed to get desktop pidl.\n");

    SHGetDesktopFolder(&psfdesktop);

    /* Test IShellItem */
    if(pSHCreateShellItem)
    {
        IShellItem *shellitem;
        hres = pSHCreateShellItem(NULL, NULL, pidl_desktop, &shellitem);
        ok(hres == S_OK, "got 0x%08lx\n", hres);
        if(SUCCEEDED(hres))
        {
            hres = pSHGetIDListFromObject((IUnknown*)shellitem, &pidl);
            ok(hres == S_OK, "got 0x%08lx\n", hres);
            if(SUCCEEDED(hres))
            {
                ok(ILIsEqual(pidl_desktop, pidl), "pidl not equal.\n");
                ILFree(pidl);
            }
            IShellItem_Release(shellitem);
        }
    }
    else
        skip("no SHCreateShellItem.\n");

    /* Test IShellFolder */
    hres = pSHGetIDListFromObject((IUnknown*)psfdesktop, &pidl);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    if(SUCCEEDED(hres))
    {
        ok(ILIsEqual(pidl_desktop, pidl), "pidl not equal.\n");
        ILFree(pidl);
    }

    hres = IShellFolder_CreateViewObject(psfdesktop, NULL, &IID_IShellView, (void**)&psv);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    if(SUCCEEDED(hres))
    {
        IEnumIDList *peidl;
        IDataObject *pdo;
        SHCONTF enum_flags;

        /* Test IFolderView */
        hres = pSHGetIDListFromObject((IUnknown*)psv, &pidl);
        ok(hres == S_OK, "got 0x%08lx\n", hres);
        if(SUCCEEDED(hres))
        {
            ok(ILIsEqual(pidl_desktop, pidl), "pidl not equal.\n");
            ILFree(pidl);
        }

        /* Test IDataObject */
        enum_flags = SHCONTF_NONFOLDERS | SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN;
        hres = IShellFolder_EnumObjects(psfdesktop, NULL, enum_flags, &peidl);
        ok(hres == S_OK, "got 0x%08lx\n", hres);
        if(SUCCEEDED(hres))
        {
            LPITEMIDLIST apidl[5];
            UINT count = 0;
            for(count = 0; count < 5; count++)
                if(IEnumIDList_Next(peidl, 1, &apidl[count], NULL) != S_OK)
                    break;

            if(count)
            {
                hres = IShellFolder_GetUIObjectOf(psfdesktop, NULL, 1, (LPCITEMIDLIST*)apidl,
                                                  &IID_IDataObject, NULL, (void**)&pdo);
                ok(hres == S_OK, "got 0x%08lx\n", hres);
                if(SUCCEEDED(hres))
                {
                    pidl = (void*)0xDEADBEEF;
                    hres = pSHGetIDListFromObject((IUnknown*)pdo, &pidl);
                    ok(hres == S_OK, "got 0x%08lx\n", hres);
                    ok(pidl != NULL, "pidl is NULL.\n");
                    ok(ILIsEqual(pidl, apidl[0]), "pidl not equal.\n");
                    ILFree(pidl);

                    IDataObject_Release(pdo);
                }
            }
            else
                skip("No files found - skipping single-file test.\n");

            if(count > 1)
            {
                hres = IShellFolder_GetUIObjectOf(psfdesktop, NULL, count, (LPCITEMIDLIST*)apidl,
                                                  &IID_IDataObject, NULL, (void**)&pdo);
                ok(hres == S_OK, "got 0x%08lx\n", hres);
                if(SUCCEEDED(hres))
                {
                    pidl = (void*)0xDEADBEEF;
                    hres = pSHGetIDListFromObject((IUnknown*)pdo, &pidl);
                    ok(hres == E_NOINTERFACE || hres == E_FAIL /*Vista*/,
                       "got 0x%08lx\n", hres);
                    ok(pidl == NULL, "pidl is not NULL.\n");

                    IDataObject_Release(pdo);
                }
            }
            else
                skip("zero or one file found - skipping multi-file test.\n");

            for(i = 0; i < count; i++)
                ILFree(apidl[i]);

            IEnumIDList_Release(peidl);
        }

        IShellView_Release(psv);
    }

    IShellFolder_Release(psfdesktop);
    ILFree(pidl_desktop);
}

static void test_SHGetItemFromObject(void)
{
    IUnknownImpl *punkimpl;
    IShellFolder *psfdesktop;
    LPITEMIDLIST pidl;
    IShellItem *psi;
    IUnknown *punk;
    HRESULT hres;
    struct if_count ifaces[] =
        { {&IID_IPersistIDList, 0},
          {&IID_IPersistFolder2, 0},
          {&IID_IDataObject, 0},
          {&IID_IParentAndItem, 0},
          {&IID_IFolderView, 0},
          {NULL, 0} };

    if(!pSHGetItemFromObject)
    {
        skip("No SHGetItemFromObject.\n");
        return;
    }

    SHGetDesktopFolder(&psfdesktop);

    if(0)
    {
        /* Crashes with Windows 7 */
        pSHGetItemFromObject((IUnknown*)psfdesktop, &IID_IUnknown, NULL);
        pSHGetItemFromObject(NULL, &IID_IUnknown, NULL);
        pSHGetItemFromObject((IUnknown*)psfdesktop, NULL, (void**)&punk);
    }

    hres = pSHGetItemFromObject(NULL, &IID_IUnknown, (void**)&punk);
    ok(hres == E_NOINTERFACE, "Got 0x%08lx\n", hres);

    punkimpl = malloc(sizeof(*punkimpl));
    punkimpl->IUnknown_iface.lpVtbl = &vt_IUnknown;
    punkimpl->ifaces = ifaces;
    punkimpl->unknown = 0;

    /* The same as SHGetIDListFromObject */
    hres = pSHGetIDListFromObject(&punkimpl->IUnknown_iface, &pidl);
    ok(hres == E_NOINTERFACE, "Got %lx\n", hres);
    ok(ifaces[0].count, "interface not requested.\n");
    ok(ifaces[1].count, "interface not requested.\n");
    ok(ifaces[2].count, "interface not requested.\n");
    todo_wine
        ok(ifaces[3].count || broken(!ifaces[3].count /*vista*/),
           "interface not requested.\n");
    ok(ifaces[4].count || broken(!ifaces[4].count /*vista*/),
       "interface not requested.\n");

    ok(!punkimpl->unknown, "Got %ld unknown.\n", punkimpl->unknown);
    free(punkimpl);

    /* Test IShellItem */
    hres = pSHGetItemFromObject((IUnknown*)psfdesktop, &IID_IShellItem, (void**)&psi);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    if(SUCCEEDED(hres))
    {
        IShellItem *psi2;
        hres = pSHGetItemFromObject((IUnknown*)psi, &IID_IShellItem, (void**)&psi2);
        ok(hres == S_OK, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres))
        {
            todo_wine
                ok(psi == psi2, "Different instances (%p != %p).\n", psi, psi2);
            IShellItem_Release(psi2);
        }
        IShellItem_Release(psi);
    }

    IShellFolder_Release(psfdesktop);
}

static void test_SHCreateShellItemArray(void)
{
    IShellFolder *pdesktopsf, *psf;
    IShellItemArray *psia;
    IEnumIDList *peidl;
    HRESULT hr;
    WCHAR cTestDirW[MAX_PATH];
    LPITEMIDLIST pidl_testdir, pidl;

    if(!pSHCreateShellItemArray) {
        skip("No pSHCreateShellItemArray!\n");
        return;
    }

    if(0)
    {
        /* Crashes under native */
        pSHCreateShellItemArray(NULL, NULL, 0, NULL, NULL);
        pSHCreateShellItemArray(NULL, NULL, 1, NULL, NULL);
        pSHCreateShellItemArray(NULL, pdesktopsf, 0, NULL, NULL);
        pSHCreateShellItemArray(pidl, NULL, 0, NULL, NULL);
    }

    hr = pSHCreateShellItemArray(NULL, NULL, 0, NULL, &psia);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    SHGetDesktopFolder(&pdesktopsf);
    hr = pSHCreateShellItemArray(NULL, pdesktopsf, 0, NULL, &psia);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = pSHCreateShellItemArray(NULL, pdesktopsf, 1, NULL, &psia);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
    hr = pSHCreateShellItemArray(pidl, NULL, 0, NULL, &psia);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ILFree(pidl);

    GetCurrentDirectoryW(MAX_PATH, cTestDirW);
    myPathAddBackslashW(cTestDirW);
    lstrcatW(cTestDirW, L"testdir");

    CreateFilesFolders();

    hr = IShellFolder_ParseDisplayName(pdesktopsf, NULL, NULL, cTestDirW, NULL, &pidl_testdir, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IShellFolder_BindToObject(pdesktopsf, pidl_testdir, NULL, (REFIID)&IID_IShellFolder,
                                       (void**)&psf);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
    }
    IShellFolder_Release(pdesktopsf);

    if(FAILED(hr))
    {
        skip("Failed to set up environment for SHCreateShellItemArray tests.\n");
        ILFree(pidl_testdir);
        Cleanup();
        return;
    }

    hr = IShellFolder_EnumObjects(psf, NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &peidl);
    ok(hr == S_OK, "Got %08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        LPITEMIDLIST apidl[5];
        UINT done;
        DWORD numitems, i;

        for(done = 0; done < 5; done++)
            if(IEnumIDList_Next(peidl, 1, &apidl[done], NULL) != S_OK)
                break;
        ok(done == 5, "Got %d pidls\n", done);
        IEnumIDList_Release(peidl);

        /* Create a ShellItemArray */
        hr = pSHCreateShellItemArray(NULL, psf, done, (LPCITEMIDLIST*)apidl, &psia);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            IShellItem *psi;

            if(0)
            {
                /* Crashes in Windows 7 */
                IShellItemArray_GetCount(psia, NULL);
            }

            IShellItemArray_GetCount(psia, &numitems);
            ok(numitems == done, "Got %ld, expected %d\n", numitems, done);

            hr = IShellItemArray_GetItemAt(psia, numitems, &psi);
            ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

            /* Compare all the items */
            for(i = 0; i < numitems; i++)
            {
                LPITEMIDLIST pidl_abs;
                pidl_abs = ILCombine(pidl_testdir, apidl[i]);

                hr = IShellItemArray_GetItemAt(psia, i, &psi);
                ok(hr == S_OK, "(%ld) Failed with 0x%08lx\n", i, hr);
                if(SUCCEEDED(hr))
                {
                    hr = pSHGetIDListFromObject((IUnknown*)psi, &pidl);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    if(SUCCEEDED(hr))
                    {
                        ok(ILIsEqual(pidl_abs, pidl), "Pidl not equal.\n");
                        ILFree(pidl);
                    }
                    IShellItem_Release(psi);
                }
                ILFree(pidl_abs);
            }
            for(i = 0; i < done; i++)
                ILFree(apidl[i]);
            IShellItemArray_Release(psia);
        }
    }

    /* SHCreateShellItemArrayFromShellItem */
    if(pSHCreateShellItemArrayFromShellItem)
    {
        IShellItem *psi;

        if(0)
        {
            /* Crashes under Windows 7 */
            pSHCreateShellItemArrayFromShellItem(NULL, &IID_IShellItemArray, NULL);
            pSHCreateShellItemArrayFromShellItem(NULL, &IID_IShellItemArray, (void**)&psia);
            pSHCreateShellItemArrayFromShellItem(psi, &IID_IShellItemArray, NULL);
        }

        hr = pSHCreateItemFromIDList(pidl_testdir, &IID_IShellItem, (void**)&psi);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = pSHCreateShellItemArrayFromShellItem(psi, &IID_IShellItemArray, (void**)&psia);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                IShellItem *psi2;
                DWORD count;
                hr = IShellItemArray_GetCount(psia, &count);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(count == 1, "Got count %ld\n", count);
                hr = IShellItemArray_GetItemAt(psia, 0, &psi2);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                todo_wine
                    ok(psi != psi2, "ShellItems are of the same instance.\n");
                if(SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl1, pidl2;
                    hr = pSHGetIDListFromObject((IUnknown*)psi, &pidl1);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(pidl1 != NULL, "pidl1 was null.\n");
                    hr = pSHGetIDListFromObject((IUnknown*)psi2, &pidl2);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(pidl2 != NULL, "pidl2 was null.\n");
                    ok(ILIsEqual(pidl1, pidl2), "pidls not equal.\n");
                    ILFree(pidl1);
                    ILFree(pidl2);
                    IShellItem_Release(psi2);
                }
                hr = IShellItemArray_GetItemAt(psia, 1, &psi2);
                ok(hr == E_FAIL, "Got 0x%08lx\n", hr);
                IShellItemArray_Release(psia);
            }
            IShellItem_Release(psi);
        }
    }
    else
        skip("No SHCreateShellItemArrayFromShellItem.\n");

    if(pSHCreateShellItemArrayFromDataObject)
    {
        IShellView *psv;

        if(0)
        {
            /* Crashes under Windows 7 */
            pSHCreateShellItemArrayFromDataObject(NULL, &IID_IShellItemArray, NULL);
        }
        hr = pSHCreateShellItemArrayFromDataObject(NULL, &IID_IShellItemArray, (void**)&psia);
        ok(hr == E_INVALIDARG, "Got 0x%08lx\n", hr);

        hr = IShellFolder_CreateViewObject(psf, NULL, &IID_IShellView, (void**)&psv);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            IEnumIDList *peidl;
            IDataObject *pdo;
            SHCONTF enum_flags;

            enum_flags = SHCONTF_NONFOLDERS | SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN;
            hr = IShellFolder_EnumObjects(psf, NULL, enum_flags, &peidl);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                LPITEMIDLIST apidl[5];
                UINT count, i;

                for(count = 0; count < 5; count++)
                    if(IEnumIDList_Next(peidl, 1, &apidl[count], NULL) != S_OK)
                        break;
                ok(count == 5, "Got %d\n", count);

                if(count)
                {
                    hr = IShellFolder_GetUIObjectOf(psf, NULL, count, (LPCITEMIDLIST*)apidl,
                                                    &IID_IDataObject, NULL, (void**)&pdo);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    if(SUCCEEDED(hr))
                    {
                        hr = pSHCreateShellItemArrayFromDataObject(pdo, &IID_IShellItemArray,
                                                                   (void**)&psia);
                        ok(hr == S_OK, "Got 0x%08lx\n", hr);
                        if(SUCCEEDED(hr))
                        {
                            DWORD count_sia, i;
                            hr = IShellItemArray_GetCount(psia, &count_sia);
                            ok(hr == S_OK, "Got 0x%08lx\n", hr);
                            ok(count_sia == count, "Counts differ (%d, %ld)\n", count, count_sia);
                            for(i = 0; i < count_sia; i++)
                            {
                                LPITEMIDLIST pidl_abs = ILCombine(pidl_testdir, apidl[i]);
                                IShellItem *psi;
                                hr = IShellItemArray_GetItemAt(psia, i, &psi);
                                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                                if(SUCCEEDED(hr))
                                {
                                    LPITEMIDLIST pidl;
                                    hr = pSHGetIDListFromObject((IUnknown*)psi, &pidl);
                                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                                    ok(pidl != NULL, "pidl as NULL.\n");
                                    ok(ILIsEqual(pidl, pidl_abs), "pidls differ.\n");
                                    ILFree(pidl);
                                    IShellItem_Release(psi);
                                }
                                ILFree(pidl_abs);
                            }

                            IShellItemArray_Release(psia);
                        }

                        IDataObject_Release(pdo);
                    }
                    for(i = 0; i < count; i++)
                        ILFree(apidl[i]);
                }
                else
                    skip("No files found - skipping test.\n");

                IEnumIDList_Release(peidl);
            }
            IShellView_Release(psv);
        }
    }
    else
        skip("No SHCreateShellItemArrayFromDataObject.\n");

    if(pSHCreateShellItemArrayFromIDLists)
    {
        WCHAR test1pathW[MAX_PATH];
        LPITEMIDLIST pidltest1;
        LPCITEMIDLIST pidl_array[2];

        if(0)
        {
            /* Crashes */
            hr = pSHCreateShellItemArrayFromIDLists(0, NULL, NULL);
        }

        psia = (void*)0xdeadbeef;
        hr = pSHCreateShellItemArrayFromIDLists(0, NULL, &psia);
        ok(hr == E_INVALIDARG, "Got 0x%08lx\n", hr);
        ok(psia == NULL, "Got %p\n", psia);

        psia = (void*)0xdeadbeef;
        hr = pSHCreateShellItemArrayFromIDLists(0, pidl_array, &psia);
        ok(hr == E_INVALIDARG, "Got 0x%08lx\n", hr);
        ok(psia == NULL, "Got %p\n", psia);

        psia = (void*)0xdeadbeef;
        pidl_array[0] = NULL;
        hr = pSHCreateShellItemArrayFromIDLists(1, pidl_array, &psia);
        todo_wine ok(hr == E_OUTOFMEMORY, "Got 0x%08lx\n", hr);
        ok(psia == NULL, "Got %p\n", psia);

        psia = (void*)0xdeadbeef;
        pidl_array[0] = pidl_testdir;
        pidl_array[1] = NULL;
        hr = pSHCreateShellItemArrayFromIDLists(2, pidl_array, &psia);
        todo_wine ok(hr == S_OK || broken(hr == E_INVALIDARG) /* Vista */, "Got 0x%08lx\n", hr);
        todo_wine ok(psia != NULL || broken(psia == NULL) /* Vista */, "Got %p\n", psia);
        if(SUCCEEDED(hr))
        {
            IShellItem *psi;
            DWORD count = 0;

            hr = IShellItemArray_GetCount(psia, &count);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            ok(count == 2, "Got %ld\n", count);

            hr = IShellItemArray_GetItemAt(psia, 0, &psi);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                LPWSTR path;
                hr = IShellItem_GetDisplayName(psi, SIGDN_DESKTOPABSOLUTEPARSING, &path);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(!lstrcmpW(path, cTestDirW), "Got %s\n", wine_dbgstr_w(path));
                if(SUCCEEDED(hr))
                    CoTaskMemFree(path);

                IShellItem_Release(psi);
            }

            hr = IShellItemArray_GetItemAt(psia, 1, &psi);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                LPWSTR path;
                WCHAR desktoppath[MAX_PATH];
                BOOL result;

                result = SHGetSpecialFolderPathW(NULL, desktoppath, CSIDL_DESKTOPDIRECTORY, FALSE);
                ok(result, "SHGetSpecialFolderPathW(CSIDL_DESKTOPDIRECTORY) failed! %lu\n", GetLastError());

                hr = IShellItem_GetDisplayName(psi, SIGDN_DESKTOPABSOLUTEPARSING, &path);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(!lstrcmpW(path, desktoppath), "Got %s\n", wine_dbgstr_w(path));
                if(SUCCEEDED(hr))
                    CoTaskMemFree(path);

                IShellItem_Release(psi);
            }


            IShellItemArray_Release(psia);
        }


        /* Single pidl */
        psia = (void*)0xdeadbeef;
        pidl_array[0] = pidl_testdir;
        hr = pSHCreateShellItemArrayFromIDLists(1, pidl_array, &psia);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            IShellItem *psi;
            DWORD count = 0;

            hr = IShellItemArray_GetCount(psia, &count);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            ok(count == 1, "Got %ld\n", count);

            hr = IShellItemArray_GetItemAt(psia, 0, &psi);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                LPWSTR path;
                hr = IShellItem_GetDisplayName(psi, SIGDN_DESKTOPABSOLUTEPARSING, &path);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(!lstrcmpW(path, cTestDirW), "Got %s\n", wine_dbgstr_w(path));
                if(SUCCEEDED(hr))
                    CoTaskMemFree(path);

                IShellItem_Release(psi);
            }

            IShellItemArray_Release(psia);
        }


        lstrcpyW(test1pathW, cTestDirW);
        myPathAddBackslashW(test1pathW);
        lstrcatW(test1pathW, L"test1.txt");

        SHGetDesktopFolder(&pdesktopsf);

        hr = IShellFolder_ParseDisplayName(pdesktopsf, NULL, NULL, test1pathW, NULL, &pidltest1, NULL);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            psia = (void*)0xdeadbeef;
            pidl_array[0] = pidl_testdir;
            pidl_array[1] = pidltest1;
            hr = pSHCreateShellItemArrayFromIDLists(2, pidl_array, &psia);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                IShellItem *psi;
                DWORD count = 0;

                hr = IShellItemArray_GetCount(psia, &count);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(count == 2, "Got %ld\n", count);

                hr = IShellItemArray_GetItemAt(psia, 0, &psi);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                if(SUCCEEDED(hr))
                {
                    LPWSTR path;
                    hr = IShellItem_GetDisplayName(psi, SIGDN_DESKTOPABSOLUTEPARSING, &path);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(!lstrcmpW(path, cTestDirW), "Got %s\n", wine_dbgstr_w(path));
                    if(SUCCEEDED(hr))
                        CoTaskMemFree(path);

                    IShellItem_Release(psi);
                }

                hr = IShellItemArray_GetItemAt(psia, 1, &psi);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                if(SUCCEEDED(hr))
                {
                    LPWSTR path;
                    hr = IShellItem_GetDisplayName(psi, SIGDN_DESKTOPABSOLUTEPARSING, &path);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(!lstrcmpW(path, test1pathW), "Got %s\n", wine_dbgstr_w(path));
                    if(SUCCEEDED(hr))
                        CoTaskMemFree(path);

                    IShellItem_Release(psi);
                }


                IShellItemArray_Release(psia);
            }

            ILFree(pidltest1);
        }

        IShellFolder_Release(pdesktopsf);
    }
    else
        skip("No SHCreateShellItemArrayFromIDLists.\n");

    IShellFolder_Release(psf);
    ILFree(pidl_testdir);
    Cleanup();
}

static void test_ShellItemArrayEnumItems(void)
{
    IShellFolder *pdesktopsf, *psf;
    IEnumIDList *peidl;
    WCHAR cTestDirW[MAX_PATH];
    HRESULT hr;
    LPITEMIDLIST pidl_testdir;

    if(!pSHCreateShellItemArray)
    {
        win_skip("No SHCreateShellItemArray, skipping test...\n");
        return;
    }

    CreateFilesFolders();

    SHGetDesktopFolder(&pdesktopsf);

    GetCurrentDirectoryW(MAX_PATH, cTestDirW);
    myPathAddBackslashW(cTestDirW);
    lstrcatW(cTestDirW, L"testdir");

    hr = IShellFolder_ParseDisplayName(pdesktopsf, NULL, NULL, cTestDirW, NULL, &pidl_testdir, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IShellFolder_BindToObject(pdesktopsf, pidl_testdir, NULL, (REFIID)&IID_IShellFolder,
                                       (void**)&psf);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        ILFree(pidl_testdir);
    }
    IShellFolder_Release(pdesktopsf);
    if (FAILED(hr)) return;

    hr = IShellFolder_EnumObjects(psf, NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &peidl);
    ok(hr == S_OK, "Got %08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        IShellItemArray *psia;
        LPITEMIDLIST apidl[5];
        UINT done;
        DWORD numitems, i;

        for(done = 0; done < 5; done++)
            if(IEnumIDList_Next(peidl, 1, &apidl[done], NULL) != S_OK)
                break;
        ok(done == 5, "Got %d pidls\n", done);
        IEnumIDList_Release(peidl);

        /* Create a ShellItemArray */
        hr = pSHCreateShellItemArray(NULL, psf, done, (LPCITEMIDLIST*)apidl, &psia);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            IEnumShellItems *iesi;
            IShellItem *my_array[10];
            ULONG fetched;

            IShellItemArray_GetCount(psia, &numitems);
            ok(numitems == done, "Got %ld, expected %d\n", numitems, done);

            iesi = NULL;
            hr = IShellItemArray_EnumItems(psia, &iesi);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            ok(iesi != NULL, "Got NULL\n");
            if(SUCCEEDED(hr))
            {
                IEnumShellItems *iesi2;

                /* This should fail according to the documentation and Win7+ */
                for(i = 0; i < 10; i++) my_array[i] = (void*)0xdeadbeef;
                hr = IEnumShellItems_Next(iesi, 2, my_array, NULL);
                ok(hr == E_INVALIDARG || broken(hr == S_OK) /* Vista */, "Got 0x%08lx\n", hr);
                for(i = 0; i < 2; i++)
                {
                    ok(my_array[i] == (void*)0xdeadbeef ||
                       broken(my_array[i] != (void*)0xdeadbeef && my_array[i] != NULL), /* Vista */
                       "Got %p (%ld)\n", my_array[i], i);

                    if(my_array[i] != (void*)0xdeadbeef)
                        IShellItem_Release(my_array[i]);
                }
                ok(my_array[2] == (void*)0xdeadbeef, "Got %p\n", my_array[2]);

                IEnumShellItems_Reset(iesi);
                for(i = 0; i < 10; i++) my_array[i] = (void*)0xdeadbeef;
                hr = IEnumShellItems_Next(iesi, 1, my_array, NULL);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(my_array[0] != NULL && my_array[0] != (void*)0xdeadbeef, "Got %p\n", my_array[0]);
                if(my_array[0] != NULL && my_array[0] != (void*)0xdeadbeef)
                    IShellItem_Release(my_array[0]);
                ok(my_array[1] == (void*)0xdeadbeef, "Got %p\n", my_array[1]);

                IEnumShellItems_Reset(iesi);
                fetched = 0;
                for(i = 0; i < 10; i++) my_array[i] = (void*)0xdeadbeef;
                hr = IEnumShellItems_Next(iesi, numitems, my_array, &fetched);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                ok(fetched == numitems, "Got %ld\n", fetched);
                for(i = 0;i < numitems; i++)
                {
                    ok(my_array[i] != NULL && my_array[i] != (void*)0xdeadbeef,
                       "Got %p at %ld\n", my_array[i], i);

                    if(my_array[i] != NULL && my_array[i] != (void*)0xdeadbeef)
                        IShellItem_Release(my_array[i]);
                }
                ok(my_array[i] == (void*)0xdeadbeef, "Got %p\n", my_array[i]);

                /* Compare all the items */
                IEnumShellItems_Reset(iesi);
                for(i = 0; i < numitems; i++)
                {
                    IShellItem *psi;
                    int order;

                    hr = IShellItemArray_GetItemAt(psia, i, &psi);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    hr = IEnumShellItems_Next(iesi, 1, my_array, &fetched);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(fetched == 1, "Got %ld\n", fetched);

                    hr = IShellItem_Compare(psi, my_array[0], 0, &order);
                    ok(hr == S_OK, "Got 0x%08lx\n", hr);
                    ok(order == 0, "Got %d\n", order);

                    IShellItem_Release(psi);
                    IShellItem_Release(my_array[0]);
                }

                my_array[0] = (void*)0xdeadbeef;
                hr = IEnumShellItems_Next(iesi, 1, my_array, &fetched);
                ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
                ok(fetched == 0, "Got %ld\n", fetched);
                ok(my_array[0] == (void*)0xdeadbeef, "Got %p\n", my_array[0]);

                /* Cloning not implemented anywhere */
                iesi2 = (void*)0xdeadbeef;
                hr = IEnumShellItems_Clone(iesi, &iesi2);
                ok(hr == E_NOTIMPL, "Got 0x%08lx\n", hr);
                ok(iesi2 == NULL || broken(iesi2 == (void*)0xdeadbeef) /* Vista */, "Got %p\n", iesi2);

                IEnumShellItems_Release(iesi);
            }

            IShellItemArray_Release(psia);
        }

        for(i = 0; i < done; i++)
            ILFree(apidl[i]);
    }

    IShellFolder_Release(psf);
}


static void test_ShellItemBindToHandler(void)
{
    IShellItem *psi;
    LPITEMIDLIST pidl_desktop;
    HRESULT hr;

    if(!pSHCreateShellItem)
    {
        skip("SHCreateShellItem missing.\n");
        return;
    }

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl_desktop);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = pSHCreateShellItem(NULL, NULL, pidl_desktop, &psi);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
    }
    if(SUCCEEDED(hr))
    {
        IPersistFolder2 *ppf2;
        IUnknown *punk;

        if(0)
        {
            /* Crashes under Windows 7 */
            IShellItem_BindToHandler(psi, NULL, NULL, NULL, NULL);
            IShellItem_BindToHandler(psi, NULL, &IID_IUnknown, &IID_IUnknown, NULL);
        }
        hr = IShellItem_BindToHandler(psi, NULL, &IID_IUnknown, &IID_IUnknown, (void**)&punk);
        ok(hr == MK_E_NOOBJECT, "Got 0x%08lx\n", hr);

        /* BHID_SFObject */
        hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFObject, &IID_IShellFolder, (void**)&punk);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);
        hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFObject, &IID_IPersistFolder2, (void**)&ppf2);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl_tmp;
            hr = IPersistFolder2_GetCurFolder(ppf2, &pidl_tmp);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                ok(ILIsEqual(pidl_desktop, pidl_tmp), "Pidl not equal (%p, %p)\n", pidl_desktop, pidl_tmp);
                ILFree(pidl_tmp);
            }
            IPersistFolder2_Release(ppf2);
        }

        /* BHID_SFUIObject */
        hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFUIObject, &IID_IDataObject, (void**)&punk);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE /* XP */), "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);
        hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFUIObject, &IID_IContextMenu, (void**)&punk);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE /* XP */), "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);

        /* BHID_DataObject */
        hr = IShellItem_BindToHandler(psi, NULL, &BHID_DataObject, &IID_IDataObject, (void**)&punk);
        ok(hr == S_OK || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);

        todo_wine
        {
            /* BHID_SFViewObject */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFViewObject, &IID_IShellView, (void**)&punk);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_SFViewObject, &IID_IShellFolderView, (void**)&punk);
            ok(hr == E_NOINTERFACE, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_Storage */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_Storage, &IID_IStream, (void**)&punk);
            ok(hr == E_NOINTERFACE, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_Storage, &IID_IUnknown, (void**)&punk);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_Stream */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_Stream, &IID_IStream, (void**)&punk);
            ok(hr == E_NOINTERFACE, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_Stream, &IID_IUnknown, (void**)&punk);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_StorageEnum */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_StorageEnum, &IID_IEnumShellItems, (void**)&punk);
            ok(hr == S_OK, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_Transfer
               ITransferSource and ITransferDestination are accessible starting from Vista, IUnknown is
               supported starting from Win8. */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_Transfer, &IID_ITransferSource, (void**)&punk);
            ok(hr == S_OK || broken(FAILED(hr)) /* pre-Vista */, "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr))
            {
                IUnknown_Release(punk);

                hr = IShellItem_BindToHandler(psi, NULL, &BHID_Transfer, &IID_ITransferDestination, (void**)&punk);
                ok(hr == S_OK, "Got 0x%08lx\n", hr);
                if(SUCCEEDED(hr)) IUnknown_Release(punk);

                hr = IShellItem_BindToHandler(psi, NULL, &BHID_Transfer, &IID_IUnknown, (void**)&punk);
                ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* pre-Win8 */, "Got 0x%08lx\n", hr);
                if(SUCCEEDED(hr)) IUnknown_Release(punk);
            }

            /* BHID_EnumItems */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_EnumItems, &IID_IEnumShellItems, (void**)&punk);
            ok(hr == S_OK || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_Filter */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_Filter, &IID_IUnknown, (void**)&punk);
            ok(hr == S_OK || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_LinkTargetItem */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_LinkTargetItem, &IID_IShellItem, (void**)&punk);
            ok(hr == E_NOINTERFACE || broken(hr == E_INVALIDARG /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_LinkTargetItem, &IID_IUnknown, (void**)&punk);
            ok(hr == E_NOINTERFACE || broken(hr == E_INVALIDARG /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_PropertyStore */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_PropertyStore, &IID_IPropertyStore, (void**)&punk);
            ok(hr == E_NOINTERFACE || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_PropertyStore, &IID_IPropertyStoreFactory, (void**)&punk);
            ok(hr == E_NOINTERFACE || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_ThumbnailHandler */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_ThumbnailHandler, &IID_IUnknown, (void**)&punk);
            ok(hr == E_INVALIDARG || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_AssociationArray */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_AssociationArray, &IID_IQueryAssociations, (void**)&punk);
            ok(hr == S_OK || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);

            /* BHID_EnumAssocHandlers */
            hr = IShellItem_BindToHandler(psi, NULL, &BHID_EnumAssocHandlers, &IID_IUnknown, (void**)&punk);
            ok(hr == E_NOINTERFACE || broken(hr == MK_E_NOOBJECT /* XP */), "Got 0x%08lx\n", hr);
            if(SUCCEEDED(hr)) IUnknown_Release(punk);
        }

        IShellItem_Release(psi);
    }
    else
        skip("Failed to create ShellItem.\n");

    ILFree(pidl_desktop);
}

static void test_ShellItemGetAttributes(void)
{
    IShellItem *psi, *psi_folder1, *psi_file1;
    IShellFolder *pdesktopsf;
    LPITEMIDLIST pidl_desktop, pidl;
    SFGAOF sfgao;
    HRESULT hr;
    WCHAR curdirW[MAX_PATH];
    WCHAR buf[MAX_PATH];

    if(!pSHCreateShellItem)
    {
        skip("SHCreateShellItem missing.\n");
        return;
    }

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl_desktop);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = pSHCreateShellItem(NULL, NULL, pidl_desktop, &psi);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        ILFree(pidl_desktop);
    }
    if(FAILED(hr))
    {
        skip("Skipping tests.\n");
        return;
    }

    if(0)
    {
        /* Crashes on native (Win 7) */
        IShellItem_GetAttributes(psi, 0, NULL);
    }

    /* Test GetAttributes on the desktop folder. */
    sfgao = 0xdeadbeef;
    hr = IShellItem_GetAttributes(psi, SFGAO_FOLDER, &sfgao);
    ok(hr == S_OK || broken(hr == E_FAIL) /* <Vista */, "Got 0x%08lx\n", hr);
    ok(sfgao == SFGAO_FOLDER || broken(sfgao == 0) /* <Vista */, "Got 0x%08lx\n", sfgao);

    IShellItem_Release(psi);

    CreateFilesFolders();

    SHGetDesktopFolder(&pdesktopsf);

    GetCurrentDirectoryW(MAX_PATH, curdirW);
    myPathAddBackslashW(curdirW);

    lstrcpyW(buf, curdirW);
    lstrcatW(buf, L"testdir");
    hr = IShellFolder_ParseDisplayName(pdesktopsf, NULL, NULL, buf, NULL, &pidl, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = pSHCreateShellItem(NULL, NULL, pidl, &psi_folder1);
    ok(hr == S_OK, "Got 0x%08lx\n", sfgao);
    ILFree(pidl);

    lstrcpyW(buf, curdirW);
    lstrcatW(buf, L"testdir\\test1.txt");
    hr = IShellFolder_ParseDisplayName(pdesktopsf, NULL, NULL, buf, NULL, &pidl, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = pSHCreateShellItem(NULL, NULL, pidl, &psi_file1);
    ok(hr == S_OK, "Got 0x%08lx\n", sfgao);
    ILFree(pidl);

    IShellFolder_Release(pdesktopsf);

    sfgao = 0xdeadbeef;
    hr = IShellItem_GetAttributes(psi_folder1, 0, &sfgao);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(sfgao == 0, "Got 0x%08lx\n", sfgao);

    sfgao = 0xdeadbeef;
    hr = IShellItem_GetAttributes(psi_folder1, SFGAO_FOLDER, &sfgao);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(sfgao == SFGAO_FOLDER, "Got 0x%08lx\n", sfgao);

    sfgao = 0xdeadbeef;
    hr = IShellItem_GetAttributes(psi_file1, SFGAO_FOLDER, &sfgao);
    ok(hr == S_FALSE, "Got 0x%08lx\n", hr);
    ok(sfgao == 0, "Got 0x%08lx\n", sfgao);

    IShellItem_Release(psi_folder1);
    IShellItem_Release(psi_file1);

    Cleanup();
}

static void test_ShellItemArrayGetAttributes(void)
{
    IShellItemArray *psia_files, *psia_folders1, *psia_folders2, *psia_all;
    IShellFolder *pdesktopsf;
    LPCITEMIDLIST pidl_array[5];
    SFGAOF attr;
    HRESULT hr;
    WCHAR curdirW[MAX_PATH];
    WCHAR buf[MAX_PATH];
    UINT i;
    static const WCHAR *testfilesW[] =
        { L"testdir", L"testdir\\testdir2", L"testdir\\testdir3", L"testdir\\test1.txt", L"testdir\\test2.txt" };

    if(!pSHCreateShellItemArrayFromShellItem)
    {
        win_skip("No SHCreateShellItemArrayFromShellItem, skipping test...\n");
        return;
    }

    CreateFilesFolders();
    CreateDirectoryA(".\\testdir\\testdir3", NULL);

    SHGetDesktopFolder(&pdesktopsf);

    GetCurrentDirectoryW(MAX_PATH, curdirW);
    myPathAddBackslashW(curdirW);

    for(i = 0; i < 5; i++)
    {
        lstrcpyW(buf, curdirW);
        lstrcatW(buf, testfilesW[i]);
        hr = IShellFolder_ParseDisplayName(pdesktopsf, NULL, NULL, buf, NULL, (LPITEMIDLIST*)&pidl_array[i], NULL);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
    }
    IShellFolder_Release(pdesktopsf);

    hr = pSHCreateShellItemArrayFromIDLists(2, pidl_array, &psia_folders1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = pSHCreateShellItemArrayFromIDLists(2, &pidl_array[1], &psia_folders2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = pSHCreateShellItemArrayFromIDLists(2, &pidl_array[3], &psia_files);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = pSHCreateShellItemArrayFromIDLists(4, &pidl_array[1], &psia_all); /* All except the first */
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    for(i = 0; i < 5; i++)
        ILFree((LPITEMIDLIST)pidl_array[i]);

    /* [testfolder/, testfolder/testfolder2] seems to break in Vista */
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_folders1, SIATTRIBFLAGS_AND, SFGAO_FOLDER, &attr);
    ok(hr == S_OK || broken(hr == E_UNEXPECTED)  /* Vista */, "Got 0x%08lx\n", hr);
    ok(attr == SFGAO_FOLDER || broken(attr == 0) /* Vista */, "Got 0x%08lx\n", attr);
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_folders1, SIATTRIBFLAGS_OR, SFGAO_FOLDER, &attr);
    ok(hr == S_OK || broken(hr == E_UNEXPECTED)  /* Vista */, "Got 0x%08lx\n", hr);
    ok(attr == SFGAO_FOLDER || broken(attr == 0) /* Vista */, "Got 0x%08lx\n", attr);

    /* [testfolder/testfolder2, testfolder/testfolder3] works */
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_folders2, SIATTRIBFLAGS_AND, SFGAO_FOLDER, &attr);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(attr == SFGAO_FOLDER, "Got 0x%08lx\n", attr);
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_files, SIATTRIBFLAGS_AND, SFGAO_FOLDER, &attr);
    ok(hr == S_FALSE || broken(hr == S_OK) /* Vista */, "Got 0x%08lx\n", hr);
    ok(attr == 0, "Got 0x%08lx\n", attr);
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_all, SIATTRIBFLAGS_AND, SFGAO_FOLDER, &attr);
    ok(hr == S_FALSE || broken(hr == S_OK) /* Vista */, "Got 0x%08lx\n", hr);
    ok(attr == 0, "Got 0x%08lx\n", attr);
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_folders2, SIATTRIBFLAGS_OR, SFGAO_FOLDER, &attr);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(attr == SFGAO_FOLDER, "Got 0x%08lx\n", attr);
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_files, SIATTRIBFLAGS_OR, SFGAO_FOLDER, &attr);
    ok(hr == S_FALSE || broken(hr == S_OK) /* Vista */, "Got 0x%08lx\n", hr);
    ok(attr == 0, "Got 0x%08lx\n", attr);
    attr = 0xdeadbeef;
    hr = IShellItemArray_GetAttributes(psia_all, SIATTRIBFLAGS_OR, SFGAO_FOLDER, &attr);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    ok(attr == SFGAO_FOLDER, "Got 0x%08lx\n", attr);

    IShellItemArray_Release(psia_folders1);
    IShellItemArray_Release(psia_folders2);
    IShellItemArray_Release(psia_files);
    IShellItemArray_Release(psia_all);

    RemoveDirectoryA(".\\testdir\\testdir3");
    Cleanup();
}

static void test_SHParseDisplayName(void)
{
    LPITEMIDLIST pidl1, pidl2;
    IShellFolder *desktop;
    WCHAR dirW[MAX_PATH];
    WCHAR nameW[10];
    HRESULT hr;
    BOOL ret, is_wow64;

if (0)
{
    /* crashes on native */
    SHParseDisplayName(NULL, NULL, NULL, 0, NULL);
    nameW[0] = 0;
    SHParseDisplayName(nameW, NULL, NULL, 0, NULL);
}

    pidl1 = (LPITEMIDLIST)0xdeadbeef;
    hr = SHParseDisplayName(NULL, NULL, &pidl1, 0, NULL);
    ok(broken(hr == E_OUTOFMEMORY) /* < Vista */ ||
       hr == E_INVALIDARG, "failed %08lx\n", hr);
    ok(pidl1 == 0, "expected null ptr, got %p\n", pidl1);

    /* dummy name */
    nameW[0] = 0;
    hr = SHParseDisplayName(nameW, NULL, &pidl1, 0, NULL);
    ok(hr == S_OK, "failed %08lx\n", hr);
    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "failed %08lx\n", hr);
    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, nameW, NULL, &pidl2, NULL);
    ok(hr == S_OK, "failed %08lx\n", hr);
    ret = ILIsEqual(pidl1, pidl2);
    ok(ret == TRUE, "expected equal idls\n");
    ILFree(pidl1);
    ILFree(pidl2);

    /* with path */
    GetWindowsDirectoryW( dirW, MAX_PATH );

    hr = SHParseDisplayName(dirW, NULL, &pidl1, 0, NULL);
    ok(hr == S_OK, "failed %08lx\n", hr);
    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, dirW, NULL, &pidl2, NULL);
    ok(hr == S_OK, "failed %08lx\n", hr);

    ret = ILIsEqual(pidl1, pidl2);
    ok(ret == TRUE, "expected equal idls\n");
    ILFree(pidl1);
    ILFree(pidl2);

    /* system32 is not redirected to syswow64 on WOW64 */
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;
    if (is_wow64)
    {
        UINT len;
        *dirW = 0;
        len = GetSystemDirectoryW(dirW, MAX_PATH);
        ok(len > 0, "GetSystemDirectoryW failed: %lu\n", GetLastError());
        hr = SHParseDisplayName(dirW, NULL, &pidl1, 0, NULL);
        ok(hr == S_OK, "failed %08lx\n", hr);
        *dirW = 0;
        len = GetSystemWow64DirectoryW(dirW, MAX_PATH);
        ok(len > 0, "GetSystemWow64DirectoryW failed: %lu\n", GetLastError());
        hr = SHParseDisplayName(dirW, NULL, &pidl2, 0, NULL);
        ok(hr == S_OK, "failed %08lx\n", hr);
        ret = ILIsEqual(pidl1, pidl2);
        ok(ret == FALSE, "expected different idls\n");
        ILFree(pidl1);
        ILFree(pidl2);
    }

    IShellFolder_Release(desktop);
}

static void test_desktop_IPersist(void)
{
    IShellFolder *desktop;
    IPersist *persist;
    IPersistFolder2 *ppf2;
    CLSID clsid;
    HRESULT hr;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "failed %08lx\n", hr);

    hr = IShellFolder_QueryInterface(desktop, &IID_IPersist, (void**)&persist);
    ok(hr == S_OK, "failed %08lx\n", hr);

    if (hr == S_OK)
    {
    if (0)
    {
        /* crashes on native */
        IPersist_GetClassID(persist, NULL);
    }
        memset(&clsid, 0, sizeof(clsid));
        hr = IPersist_GetClassID(persist, &clsid);
        ok(hr == S_OK, "failed %08lx\n", hr);
        ok(IsEqualIID(&CLSID_ShellDesktop, &clsid), "Expected CLSID_ShellDesktop\n");
        IPersist_Release(persist);
    }

    hr = IShellFolder_QueryInterface(desktop, &IID_IPersistFolder2, (void**)&ppf2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* pre-Vista */, "failed %08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        IPersistFolder *ppf;
        LPITEMIDLIST pidl;
        hr = IShellFolder_QueryInterface(desktop, &IID_IPersistFolder, (void**)&ppf);
        ok(hr == S_OK, "IID_IPersistFolder2 without IID_IPersistFolder.\n");
        if(SUCCEEDED(hr))
            IPersistFolder_Release(ppf);

        todo_wine {
            hr = IPersistFolder2_Initialize(ppf2, NULL);
            ok(hr == S_OK, "got %08lx\n", hr);
        }

        pidl = NULL;
        hr = IPersistFolder2_GetCurFolder(ppf2, &pidl);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(pidl != NULL, "pidl was NULL.\n");
        if(SUCCEEDED(hr)) ILFree(pidl);

        IPersistFolder2_Release(ppf2);
    }

    IShellFolder_Release(desktop);
}

static void test_contextmenu_qi(IContextMenu *menu, BOOL todo)
{
    IUnknown *unk;
    HRESULT hr;

    hr = IContextMenu_QueryInterface(menu, &IID_IShellExtInit, (void **)&unk);
    todo_wine_if(todo)
    ok(hr == S_OK, "Failed to get IShellExtInit, hr %#lx.\n", hr);
    if (hr == S_OK)
        IUnknown_Release(unk);

    hr = IContextMenu_QueryInterface(menu, &IID_IObjectWithSite, (void **)&unk);
    todo_wine_if(todo)
    ok(hr == S_OK, "Failed to get IShellExtInit, hr %#lx.\n", hr);
    if (hr == S_OK)
        IUnknown_Release(unk);
}

static void test_contextmenu(IContextMenu *menu, BOOL background)
{
    HMENU hmenu = CreatePopupMenu();
    UINT count, i, max_id;
    HRESULT hr;

    test_contextmenu_qi(menu, FALSE);

    hr = IContextMenu_QueryContextMenu(menu, hmenu, 0, 64, 32767, CMF_NORMAL);
    ok(SUCCEEDED(hr), "Failed to query the menu, hr %#lx.\n", hr);

    max_id = HRESULT_CODE(hr) - 1; /* returns max_id + 1 */
    ok(max_id <= 32767, "Got %d\n", max_id);
    count = GetMenuItemCount(hmenu);
    ok(count, "Got %d\n", count);

    for (i = 0; i < count; i++)
    {
        MENUITEMINFOA mii;
        INT res;
        char buf[255], buf2[255];
        ZeroMemory(&mii, sizeof(MENUITEMINFOA));
        mii.cbSize = sizeof(MENUITEMINFOA);
        mii.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
        mii.dwTypeData = buf2;
        mii.cch = sizeof(buf2);

        res = GetMenuItemInfoA(hmenu, i, TRUE, &mii);
        ok(res, "Failed to get menu item info, error %ld.\n", GetLastError());

        if (!(mii.fType & MFT_SEPARATOR))
        {
            ok(mii.wID >= 64 && mii.wID <= 64 + max_id,
                    "Expected between 64 and %d, got %d.\n", 64 + max_id, mii.wID);
            hr = IContextMenu_GetCommandString(menu, mii.wID - 64, GCS_VERBA, 0, buf, sizeof(buf));
            ok(hr == S_OK || hr == E_NOTIMPL || hr == E_INVALIDARG,
                    "Got unexpected hr %#lx for ID %d, string %s.\n", hr, mii.wID, debugstr_a(mii.dwTypeData));
            if (hr == S_OK)
            {
                trace("Got ID %d, verb %s, string %s.\n", mii.wID, debugstr_a(buf), debugstr_a(mii.dwTypeData));
                if (!strcmp(buf, "copy"))
                {
                    CMINVOKECOMMANDINFO cmi;
                    ok(mii.wID == 64 - 0x7000 + FCIDM_SHVIEW_COPY, "wrong menu wID %d\n", mii.wID);
                    memset(&cmi, 0, sizeof(CMINVOKECOMMANDINFO));
                    cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
                    cmi.lpVerb = "copy";
                    hr = IContextMenu_InvokeCommand(menu, &cmi);
                    ok(hr == S_OK, "got 0x%08lx\n", hr);
                    ok(IsClipboardFormatAvailable(RegisterClipboardFormatA(CFSTR_SHELLIDLISTA)), "CFSTR_SHELLIDLISTA not available\n");
                    ok(IsClipboardFormatAvailable(CF_HDROP), "CF_HDROP not available\n");
                }
                else if (!strcmp(buf, "paste"))
                    ok(mii.wID == 64 - 0x7000 + FCIDM_SHVIEW_INSERT, "wrong menu wID %d\n", mii.wID);
                else if (!strcmp(buf, "properties"))
                    ok(mii.wID == 64 - 0x7000 + FCIDM_SHVIEW_PROPERTIES, "wrong menu wID %d\n", mii.wID);
            }
            else
                trace("Got ID %d, hr %#lx, string %s.\n", mii.wID, hr, debugstr_a(mii.dwTypeData));
        }
    }

    if (count)
    {
        CMINVOKECOMMANDINFO cmi;

        memset(&cmi, 0, sizeof(CMINVOKECOMMANDINFO));
        cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);

        /* Attempt to execute a nonexistent command */
        cmi.lpVerb = MAKEINTRESOURCEA(9999);
        hr = IContextMenu_InvokeCommand(menu, &cmi);
        todo_wine_if(background)
        ok(hr == E_INVALIDARG, "Got 0x%08lx\n", hr);

        cmi.lpVerb = "foobar_wine_test";
        hr = IContextMenu_InvokeCommand(menu, &cmi);
        todo_wine_if(background)
        ok((hr == E_INVALIDARG) || (hr == E_FAIL /* Win7 */) ||
           (hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /* Vista */),
            "Unexpected hr %#lx.\n", hr);
    }

    DestroyMenu(hmenu);
}

static void test_IShellItemImageFactory(void)
{
    HRESULT ret;
    IShellItem *shellitem;
    IShellItemImageFactory *siif;
    LPITEMIDLIST pidl_desktop = NULL;

    ret = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl_desktop);
    ok(ret == S_OK, "SHGetSpecialFolderLocation returned 0x%08lx\n", ret);

    ret = pSHCreateShellItem(NULL, NULL, pidl_desktop, &shellitem);
    ILFree(pidl_desktop);
    ok(SUCCEEDED(ret), "SHCreateShellItem returned 0x%08lx\n", ret);

    ret = IShellItem_QueryInterface(shellitem, &IID_IShellItemImageFactory, (void **)&siif);
    IShellItem_Release(shellitem);
    ok(ret == S_OK, "QueryInterface returned 0x%08lx\n", ret);
    if (SUCCEEDED(ret))
    {
        HBITMAP hbm = NULL;
        SIZE size = {32, 32};

        ret = IShellItemImageFactory_GetImage(siif, size, SIIGBF_BIGGERSIZEOK, &hbm);
        IShellItemImageFactory_Release(siif);
        ok(ret == S_OK, "GetImage returned %lx\n", ret);
        ok(FAILED(ret) == !hbm, "result = %lx but bitmap = %p\n", ret, hbm);

        if (SUCCEEDED(ret) && hbm)
        {
            DIBSECTION dib;
            int infosz;

            infosz = GetObjectW(hbm, sizeof(dib), &dib);
            ok(infosz == sizeof(dib), "Expected GetObjectW to return sizeof(DIBSECTION), got %d\n", infosz);

            /* Bitmap must have 32-bit pixels regardless of original image format */
            ok(dib.dsBm.bmPlanes == 1, "Expected bmPlanes to be %d, got %d\n", 1, dib.dsBm.bmPlanes);
            ok(dib.dsBm.bmBitsPixel == 32, "Expected bmBitsPixel to be %d, got %d\n", 32, dib.dsBm.bmBitsPixel);

            /* DIB must be truecolor (32bppRGB/32bppARGB) regardless of original image format */
            ok(dib.dsBmih.biPlanes == 1, "Expected biPlanes to be %d, got %d\n", 1, dib.dsBmih.biPlanes);
            ok(dib.dsBmih.biBitCount == 32, "Expected biBitCount to be %d, got %d\n", 32, dib.dsBmih.biBitCount);
            ok(dib.dsBmih.biCompression == BI_RGB, "Expected biCompression to be BI_RGB, got %lu\n", dib.dsBmih.biCompression);

            DeleteObject(hbm);
        }
    }
}

static void test_GetUIObject(void)
{
    IShellFolder *psf_desktop;
    IContextMenu *pcm;
    LPITEMIDLIST pidl;
    HRESULT hr;
    WCHAR path[MAX_PATH];
    LPCITEMIDLIST pidl_child;
    IShellFolder *psf;

    GetCurrentDirectoryW(MAX_PATH, path);
    if (!path[0])
    {
        skip("GetCurrentDirectoryW returned an empty string.\n");
        return;
    }
    lstrcatW(path, L"\\testdir\\test1.txt");
    SHGetDesktopFolder(&psf_desktop);

    CreateFilesFolders();

    hr = IShellFolder_ParseDisplayName(psf_desktop, NULL, NULL, path, NULL, &pidl, 0);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);

    hr = SHBindToParent(pidl, &IID_IShellFolder, (void **)&psf, &pidl_child);
    ok(hr == S_OK, "Failed to bind to folder, hr %#lx.\n", hr);

    /* Item menu */
    hr = IShellFolder_GetUIObjectOf(psf, NULL, 1, &pidl_child, &IID_IContextMenu, NULL, (void **)&pcm);
    ok(hr == S_OK, "GetUIObjectOf() failed, hr %#lx.\n", hr);
    test_contextmenu(pcm, FALSE);
    IContextMenu_Release(pcm);

    /* Background menu */
    hr = IShellFolder_GetUIObjectOf(psf_desktop, NULL, 0, NULL, &IID_IContextMenu, NULL, (void **)&pcm);
    ok(hr == S_OK, "GetUIObjectOf() failed, hr %#lx.\n", hr);
    test_contextmenu(pcm, TRUE);
    IContextMenu_Release(pcm);

    IShellFolder_Release(psf);
    ILFree(pidl);

    IShellFolder_Release(psf_desktop);
    Cleanup();
}

static void test_CreateViewObject_contextmenu(void)
{
    IShellFolder *desktop;
    IShellFolder *folder;
    IContextMenu *cmenu;
    WCHAR path[MAX_PATH];
    LPITEMIDLIST pidl;
    HRESULT hr;
    DWORD ret;
    int i;

    static const CLSID *folders[] =
    {
        &CLSID_MyComputer,
        &CLSID_MyDocuments,
        &CLSID_ControlPanel,
        &CLSID_NetworkPlaces,
        &CLSID_Printers,
        &CLSID_RecycleBin
    };

    for (i = 0; i < ARRAY_SIZE(folders); i++)
    {
        hr = CoCreateInstance(folders[i], NULL, CLSCTX_INPROC_SERVER, &IID_IShellFolder, (void**)&folder);
        if (hr != S_OK)
        {
            win_skip("Failed to create folder %s, hr %#lx.\n", wine_dbgstr_guid(folders[i]), hr);
            continue;
        }

        hr = IShellFolder_CreateViewObject(folder, NULL, &IID_IContextMenu, (void**)&cmenu);
        if (IsEqualIID(folders[i], &CLSID_MyDocuments))
            ok(hr == S_OK, "got 0x%08lx\n", hr);
        else if (IsEqualIID(folders[i], &CLSID_MyComputer))
            ok(hr == S_OK || broken(hr == E_NOINTERFACE /* win10 */), "got 0x%08lx\n", hr);
        else
            ok(hr == E_NOINTERFACE || broken(FAILED(hr)), "got 0x%08lx for %s\n", hr, wine_dbgstr_guid(folders[i]));
        if (SUCCEEDED(hr))
            IContextMenu_Release(cmenu);
        IShellFolder_Release(folder);
    }

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellFolder_CreateViewObject(desktop, NULL, &IID_IContextMenu, (void**)&cmenu);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (SUCCEEDED(hr))
        IContextMenu_Release(cmenu);
    ret = GetCurrentDirectoryW(MAX_PATH, path);
    ok(ret, "got %ld\n", GetLastError());
    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, path, NULL, &pidl, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellFolder_BindToObject(desktop, pidl, NULL, &IID_IShellFolder, (void**)&folder);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IShellFolder_CreateViewObject(folder, NULL, &IID_IContextMenu, (void**)&cmenu);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (SUCCEEDED(hr))
        IContextMenu_Release(cmenu);
    IShellFolder_Release(folder);
    ILFree(pidl);
    IShellFolder_Release(desktop);
}

#define verify_pidl(i,p) r_verify_pidl(__LINE__, i, p)
static void r_verify_pidl(unsigned l, LPCITEMIDLIST pidl, const WCHAR *path)
{
    LPCITEMIDLIST child;
    IShellFolder *parent;
    STRRET filename;
    HRESULT hr;

    if(path){
        if(!pidl){
            ok_(__FILE__,l)(0, "didn't get expected path (%s), instead: NULL\n", wine_dbgstr_w(path));
            return;
        }

        hr = SHBindToParent(pidl, &IID_IShellFolder, (void **)&parent, &child);
        ok_(__FILE__,l)(hr == S_OK, "SHBindToParent failed: 0x%08lx\n", hr);
        if(FAILED(hr))
            return;

        hr = IShellFolder_GetDisplayNameOf(parent, child, SHGDN_FORPARSING, &filename);
        ok_(__FILE__,l)(hr == S_OK, "GetDisplayNameOf failed: 0x%08lx\n", hr);
        if(FAILED(hr)){
            IShellFolder_Release(parent);
            return;
        }

        ok_(__FILE__,l)(filename.uType == STRRET_WSTR || filename.uType == STRRET_CSTR,
                "Got unexpected string type: %d\n", filename.uType);
        if(filename.uType == STRRET_WSTR){
            ok_(__FILE__,l)(lstrcmpW(path, filename.pOleStr) == 0,
                    "didn't get expected path (%s), instead: %s\n",
                     wine_dbgstr_w(path), wine_dbgstr_w(filename.pOleStr));
            SHFree(filename.pOleStr);
        }
        else if(filename.uType == STRRET_CSTR)
        {
            WCHAR *strW = make_wstr(filename.cStr);
            ok_(__FILE__,l)(!lstrcmpW(path, strW), "didn't get expected path (%s), instead: %s\n",
                     wine_dbgstr_w(path), filename.cStr);
            free(strW);
        }

        IShellFolder_Release(parent);
    }else
        ok_(__FILE__,l)(pidl == NULL, "Expected PIDL to be NULL\n");
}

static void test_SHSimpleIDListFromPath(void)
{
    const WCHAR adirW[] = L"C:\\sidlfpdir";
    const CHAR adirA[] = "C:\\sidlfpdir";
    BOOL br, is_unicode = !(GetVersion() & 0x80000000);

    LPITEMIDLIST pidl = NULL;

    br = CreateDirectoryA(adirA, NULL);
    ok(br == TRUE, "CreateDirectory failed: %ld\n", GetLastError());

    if(is_unicode)
        pidl = SHSimpleIDListFromPath(adirW);
    else
        pidl = SHSimpleIDListFromPath((const WCHAR *)adirA);
    verify_pidl(pidl, adirW);
    ILFree(pidl);

    br = RemoveDirectoryA(adirA);
    ok(br == TRUE, "RemoveDirectory failed: %ld\n", GetLastError());

    if(is_unicode)
        pidl = SHSimpleIDListFromPath(adirW);
    else
        pidl = SHSimpleIDListFromPath((const WCHAR *)adirA);
    verify_pidl(pidl, adirW);
    ILFree(pidl);
}

/* IFileSystemBindData impl */
static HRESULT WINAPI fsbd_QueryInterface(IFileSystemBindData *fsbd,
        REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IFileSystemBindData) ||
            IsEqualIID(riid, &IID_IUnknown)){
        *ppv = fsbd;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI fsbd_AddRef(IFileSystemBindData *fsbd)
{
    return 2;
}

static ULONG WINAPI fsbd_Release(IFileSystemBindData *fsbd)
{
    return 1;
}

static HRESULT WINAPI fsbd_SetFindData(IFileSystemBindData *fsbd,
        const WIN32_FIND_DATAW *pfd)
{
    ok(0, "SetFindData called\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fsbd_GetFindData_nul(IFileSystemBindData *fsbd,
        WIN32_FIND_DATAW *pfd)
{
    memset(pfd, 0, sizeof(WIN32_FIND_DATAW));
    return S_OK;
}

static HRESULT WINAPI fsbd_GetFindData_junk(IFileSystemBindData *fsbd,
        WIN32_FIND_DATAW *pfd)
{
    memset(pfd, 0xef, sizeof(WIN32_FIND_DATAW));
    return S_OK;
}

static HRESULT WINAPI fsbd_GetFindData_invalid(IFileSystemBindData *fsbd,
        WIN32_FIND_DATAW *pfd)
{
    memset(pfd, 0, sizeof(WIN32_FIND_DATAW));
    *pfd->cFileName = 'a';
    *pfd->cAlternateFileName = 'a';
    return S_OK;
}

static HRESULT WINAPI fsbd_GetFindData_valid(IFileSystemBindData *fsbd,
        WIN32_FIND_DATAW *pfd)
{
    HANDLE handle = FindFirstFileW(L"C:\\fsbddir", pfd);
    FindClose(handle);
    return S_OK;
}

static HRESULT WINAPI fsbd_GetFindData_fail(IFileSystemBindData *fsbd,
        WIN32_FIND_DATAW *pfd)
{
    return E_FAIL;
}

static IFileSystemBindDataVtbl fsbdVtbl = {
    fsbd_QueryInterface,
    fsbd_AddRef,
    fsbd_Release,
    fsbd_SetFindData,
    NULL
};

static IFileSystemBindData fsbd = { &fsbdVtbl };

static void test_ParseDisplayNamePBC(void)
{
    WCHAR wFileSystemBindData[] = L"File System Bind Data";
    WCHAR adirW[] = L"C:\\fsbddir";
    WCHAR afileW[] = L"C:\\fsbddir\\file.txt";
    WCHAR afile2W[] = L"C:\\fsbddir\\s\\file.txt";
    const HRESULT exp_err = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    IShellFolder *psf;
    IBindCtx *pbc;
    HRESULT hres;
    ITEMIDLIST *pidl;

    /* Check if we support WCHAR functions */
    SetLastError(0xdeadbeef);
    lstrcmpiW(adirW, adirW);
    if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED){
        win_skip("Most W-calls are not implemented\n");
        return;
    }

    hres = SHGetDesktopFolder(&psf);
    ok(hres == S_OK, "SHGetDesktopFolder failed: 0x%08lx\n", hres);
    if(FAILED(hres)){
        win_skip("Failed to get IShellFolder, can't run tests\n");
        return;
    }

    /* fails on unknown dir with no IBindCtx */
    hres = IShellFolder_ParseDisplayName(psf, NULL, NULL, adirW, NULL, &pidl, NULL);
    ok(hres == exp_err, "ParseDisplayName failed with wrong error: 0x%08lx\n", hres);
    hres = IShellFolder_ParseDisplayName(psf, NULL, NULL, afileW, NULL, &pidl, NULL);
    ok(hres == exp_err, "ParseDisplayName failed with wrong error: 0x%08lx\n", hres);
    hres = IShellFolder_ParseDisplayName(psf, NULL, NULL, afile2W, NULL, &pidl, NULL);
    ok(hres == exp_err, "ParseDisplayName failed with wrong error: 0x%08lx\n", hres);

    /* fails on unknown dir with IBindCtx with no IFileSystemBindData */
    hres = CreateBindCtx(0, &pbc);
    ok(hres == S_OK, "CreateBindCtx failed: 0x%08lx\n", hres);

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, adirW, NULL, &pidl, NULL);
    ok(hres == exp_err, "ParseDisplayName failed with wrong error: 0x%08lx\n", hres);
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afileW, NULL, &pidl, NULL);
    ok(hres == exp_err, "ParseDisplayName failed with wrong error: 0x%08lx\n", hres);
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afile2W, NULL, &pidl, NULL);
    ok(hres == exp_err, "ParseDisplayName failed with wrong error: 0x%08lx\n", hres);

    /* unknown dir with IBindCtx with IFileSystemBindData */
    hres = IBindCtx_RegisterObjectParam(pbc, wFileSystemBindData, (IUnknown*)&fsbd);
    ok(hres == S_OK, "RegisterObjectParam failed: 0x%08lx\n", hres);

    /* return E_FAIL from GetFindData */
    pidl = (ITEMIDLIST*)0xdeadbeef;
    fsbdVtbl.GetFindData = fsbd_GetFindData_fail;
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, adirW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, adirW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afileW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afileW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afile2W, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afile2W);
        ILFree(pidl);
    }

    /* set FIND_DATA struct to NULLs */
    pidl = (ITEMIDLIST*)0xdeadbeef;
    fsbdVtbl.GetFindData = fsbd_GetFindData_nul;
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, adirW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, adirW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afileW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afileW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afile2W, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afile2W);
        ILFree(pidl);
    }

    /* set FIND_DATA struct to junk */
    pidl = (ITEMIDLIST*)0xdeadbeef;
    fsbdVtbl.GetFindData = fsbd_GetFindData_junk;
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, adirW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, adirW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afileW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afileW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afile2W, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afile2W);
        ILFree(pidl);
    }

    /* set FIND_DATA struct to invalid data */
    pidl = (ITEMIDLIST*)0xdeadbeef;
    fsbdVtbl.GetFindData = fsbd_GetFindData_invalid;
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, adirW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, adirW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afileW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afileW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afile2W, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afile2W);
        ILFree(pidl);
    }

    /* set FIND_DATA struct to valid data */
    pidl = (ITEMIDLIST*)0xdeadbeef;
    fsbdVtbl.GetFindData = fsbd_GetFindData_valid;
    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, adirW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, adirW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afileW, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afileW);
        ILFree(pidl);
    }

    hres = IShellFolder_ParseDisplayName(psf, NULL, pbc, afile2W, NULL, &pidl, NULL);
    ok(hres == S_OK, "ParseDisplayName failed: 0x%08lx\n", hres);
    if(SUCCEEDED(hres)){
        verify_pidl(pidl, afile2W);
        ILFree(pidl);
    }

    IBindCtx_Release(pbc);
    IShellFolder_Release(psf);
}

static const CHAR testwindow_class[] = "testwindow";
#define WM_USER_NOTIFY (WM_APP+1)

struct ChNotifyTest {
    const char id[256];
    const UINT notify_count;
    UINT missing_events;
    UINT signal;
    const char path_1[256];
    const char path_2[256];
} chnotify_tests[] = {
    {"MKDIR", 1, 0, SHCNE_MKDIR, "C:\\shell32_cn_test\\test", ""},
    {"CREATE", 1, 0, SHCNE_CREATE, "C:\\shell32_cn_test\\test\\file.txt", ""},
    {"RMDIR", 1, 0, SHCNE_RMDIR, "C:\\shell32_cn_test\\test", ""},
};

struct ChNotifyTest *exp_data;
BOOL test_new_delivery_flag;

static LRESULT CALLBACK testwindow_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LONG signal = (LONG)lparam;

    switch(msg){
    case WM_USER_NOTIFY:
        if(exp_data->missing_events > 0) {
            WCHAR *path1, *path2;
            LPITEMIDLIST *pidls = (LPITEMIDLIST*)wparam;
            HANDLE hLock = NULL;

            if(test_new_delivery_flag) {
                hLock = SHChangeNotification_Lock((HANDLE)wparam, lparam, &pidls, &signal);
                ok(hLock != NULL, "SHChangeNotification_Lock returned NULL\n");
            }

            ok(exp_data->signal == signal,
                    "%s: expected notification type %x, got: %lx\n",
                    exp_data->id, exp_data->signal, signal);

            trace("verifying pidls for: %s\n", exp_data->id);
            path1 = make_wstr(exp_data->path_1);
            path2 = make_wstr(exp_data->path_2);
            verify_pidl(pidls[0], path1);
            verify_pidl(pidls[1], path2);
            free(path1);
            free(path2);

            exp_data->missing_events--;

            if(test_new_delivery_flag)
                SHChangeNotification_Unlock(hLock);
        }else
            ok(0, "Didn't expect a WM_USER_NOTIFY message (event: %lx)\n", signal);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void register_testwindow_class(void)
{
    WNDCLASSEXA cls;
    ATOM ret;

    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.style = 0;
    cls.lpfnWndProc = testwindow_wndproc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = testwindow_class;

    SetLastError(0);
    ret = RegisterClassExA(&cls);
    ok(ret != 0, "RegisterClassExA failed: %ld\n", GetLastError());
}

/* SHCNF_FLUSH doesn't seem to work as advertised for SHCNF_PATHA, so we
 * have to poll repeatedly for the message to appear */
static void do_events(void)
{
    int c = 0;
    while (exp_data->missing_events && (c++ < 10)){
        MSG msg;
        while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if(exp_data->missing_events)
            Sleep(500);
    }
    trace("%s: took %d tries\n", exp_data->id, c);
}

static void test_SHChangeNotify(BOOL test_new_delivery)
{
    HWND wnd;
    ULONG notifyID, i;
    HRESULT hr;
    BOOL br, has_unicode;
    SHChangeNotifyEntry entries[1];

    trace("SHChangeNotify tests (%x)\n", test_new_delivery);

    CreateDirectoryW(NULL, NULL);
    has_unicode = !(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED);

    test_new_delivery_flag = test_new_delivery;
    if(!test_new_delivery)
        register_testwindow_class();

    wnd = CreateWindowExA(0, testwindow_class, testwindow_class, 0,
            CW_USEDEFAULT, CW_USEDEFAULT, 130, 105,
            NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(wnd != NULL, "Failed to make a window\n");

    br = CreateDirectoryA("C:\\shell32_cn_test", NULL);
    ok(br == TRUE, "CreateDirectory failed: %ld\n", GetLastError());

    entries[0].pidl = NULL;
    if(has_unicode)
        hr = SHILCreateFromPath(L"C:\\shell32_cn_test", (ITEMIDLIST**)&entries[0].pidl, 0);
    else
        hr = SHILCreateFromPath((const void*)"C:\\shell32_cn_test", (ITEMIDLIST**)&entries[0].pidl, 0);
    ok(hr == S_OK, "SHILCreateFromPath failed: 0x%08lx\n", hr);
    entries[0].fRecursive = TRUE;

    notifyID = SHChangeNotifyRegister(wnd, !test_new_delivery ? SHCNRF_ShellLevel : SHCNRF_ShellLevel|SHCNRF_NewDelivery,
            SHCNE_MKDIR | SHCNE_CREATE | SHCNE_RMDIR, WM_USER_NOTIFY, 1, entries);
    ok(notifyID != 0, "Failed to register a window for change notifications\n");

    for (i = 0; i < ARRAY_SIZE(chnotify_tests); ++i)
    {
        exp_data = chnotify_tests + i;

        exp_data->missing_events = exp_data->notify_count;
        SHChangeNotify(exp_data->signal, SHCNF_PATHA | SHCNF_FLUSH,
                exp_data->path_1[0] ? exp_data->path_1 : NULL,
                exp_data->path_2[0] ? exp_data->path_2 : NULL);
        do_events();
        ok(exp_data->missing_events == 0, "%s: Expected wndproc to be called\n", exp_data->id);

        if(has_unicode){
            WCHAR *path1, *path2;

            path1 = make_wstr(exp_data->path_1);
            path2 = make_wstr(exp_data->path_2);

            exp_data->missing_events = exp_data->notify_count;
            SHChangeNotify(exp_data->signal, SHCNF_PATHW | SHCNF_FLUSH, path1, path2);
            do_events();
            ok(exp_data->missing_events == 0, "%s: Expected wndproc to be called\n", exp_data->id);

            free(path1);
            free(path2);
        }
    }

    SHChangeNotifyDeregister(notifyID);
    DestroyWindow(wnd);

    ILFree((LPITEMIDLIST)entries[0].pidl);
    br = RemoveDirectoryA("C:\\shell32_cn_test");
    ok(br == TRUE, "RemoveDirectory failed: %ld\n", GetLastError());
}

static void test_SHCreateDefaultContextMenu(void)
{
    HKEY keys[16];
    WCHAR path[MAX_PATH];
    IShellFolder *desktop,*folder;
    IPersistFolder2 *persist;
    IContextMenu *cmenu;
    LONG status;
    LPITEMIDLIST pidlFolder, pidl_child, pidl;
    DEFCONTEXTMENU cminfo;
    HRESULT hr;
    UINT i;
    if(!pSHCreateDefaultContextMenu)
    {
        win_skip("SHCreateDefaultContextMenu missing.\n");
        return;
    }

    GetCurrentDirectoryW(MAX_PATH, path);
    if (!path[0])
    {
        skip("GetCurrentDirectoryW returned an empty string.\n");
        return;
    }
    lstrcatW(path, L"\\testdir\\test1.txt");
    SHGetDesktopFolder(&desktop);

    CreateFilesFolders();

    hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, path, NULL, &pidl, 0);
    ok(hr == S_OK, "Got 0x%08lx\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = SHBindToParent(pidl, &IID_IShellFolder, (void **)&folder, (const ITEMIDLIST **)&pidl_child);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);

        IShellFolder_QueryInterface(folder,&IID_IPersistFolder2,(void**)&persist);
        IPersistFolder2_GetCurFolder(persist,&pidlFolder);
        IPersistFolder2_Release(persist);
        if(SUCCEEDED(hr))
        {
            cminfo.hwnd=NULL;
            cminfo.pcmcb=NULL;
            cminfo.psf=folder;
            cminfo.pidlFolder=NULL;
            cminfo.apidl=(LPCITEMIDLIST*)&pidl_child;
            cminfo.cidl=1;
            cminfo.aKeys=NULL;
            cminfo.cKeys=0;
            cminfo.punkAssociationInfo=NULL;

            hr = pSHCreateDefaultContextMenu(&cminfo,&IID_IContextMenu,(void**)&cmenu);
            ok(hr==S_OK,"Got 0x%08lx\n", hr);
            test_contextmenu_qi(cmenu, TRUE);
            IContextMenu_Release(cmenu);

            cminfo.pidlFolder=pidlFolder;
            hr = pSHCreateDefaultContextMenu(&cminfo,&IID_IContextMenu,(void**)&cmenu);
            ok(hr==S_OK,"Got 0x%08lx\n", hr);
            test_contextmenu_qi(cmenu, TRUE);
            IContextMenu_Release(cmenu);

            status = RegOpenKeyExA(HKEY_CLASSES_ROOT,"*",0,KEY_READ,keys);
            if(status==ERROR_SUCCESS){
                for(i=1;i<16;i++)
                    keys[i]=keys[0];
                cminfo.aKeys=keys;
                cminfo.cKeys=16;
                hr = pSHCreateDefaultContextMenu(&cminfo,&IID_IContextMenu,(void**)&cmenu);
                RegCloseKey(keys[0]);
                ok(hr==S_OK,"Got 0x%08lx\n", hr);
                IContextMenu_Release(cmenu);
            }
        }
        ILFree(pidlFolder);
        IShellFolder_Release(folder);
    }
    IShellFolder_Release(desktop);
    ILFree(pidl);
    Cleanup();
}

static void test_enum_format(IDataObject *data_obj)
{
    IEnumFORMATETC *enum1, *enum2;
    FORMATETC formats[4];
    STGMEDIUM medium;
    ULONG count, ref;
    HRESULT hr;

    hr = IDataObject_EnumFormatEtc(data_obj, DATADIR_GET, &enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFORMATETC_Next(enum1, 1, formats, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, NULL);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    hr = IEnumFORMATETC_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IEnumFORMATETC_Next(enum1, 1, formats, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 1, "Got count %lu.\n", count);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, &count);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    todo_wine ok(!count, "Got count %lu.\n", count);

    hr = IEnumFORMATETC_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFORMATETC_Next(enum1, 2, formats, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum1, 2, formats, &count);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 1, "Got count %lu.\n", count);

    hr = IEnumFORMATETC_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFORMATETC_Next(enum1, 4, formats, &count);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 3, "Got count %lu.\n", count);

    hr = IEnumFORMATETC_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFORMATETC_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Skip(enum1, 4);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Skip(enum1, 3);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Skip(enum1, 1);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum1, 1, formats, NULL);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum2, 1, formats, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Adding another format does not affect existing enum objects. */
    medium.tymed = TYMED_HGLOBAL;
    medium.pUnkForRelease = NULL;
    medium.hGlobal = GlobalAlloc(GMEM_MOVEABLE, 1);
    formats[0].cfFormat = RegisterClipboardFormatW(L"bogus_format2");
    formats[0].ptd = NULL;
    formats[0].dwAspect = DVASPECT_CONTENT;
    formats[0].lindex = -1;
    formats[0].tymed = TYMED_HGLOBAL;
    hr = IDataObject_SetData(data_obj, &formats[0], &medium, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumFORMATETC_Reset(enum1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IEnumFORMATETC_Next(enum1, 4, formats, &count);
    todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    todo_wine ok(count == 3, "Got count %lu.\n", count);

    ref = IEnumFORMATETC_Release(enum1);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IEnumFORMATETC_Release(enum2);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_DataObject(void)
{
    IEnumFORMATETC *enum_format;
    IShellFolder *desktop;
    IDataObject *data_obj;
    WCHAR format_name[50];
    HRESULT hr;
    IEnumIDList *peidl;
    LPITEMIDLIST apidl;
    FORMATETC fmt;
    DWORD cf_shellidlist;
    STGMEDIUM medium;
    HGLOBAL global;
    int *value;
    int ret;

    static const DWORD enum_directions[] = {DATADIR_GET, DATADIR_SET};

    SHGetDesktopFolder(&desktop);

    hr = IShellFolder_EnumObjects(desktop, NULL,
            SHCONTF_NONFOLDERS|SHCONTF_FOLDERS|SHCONTF_INCLUDEHIDDEN, &peidl);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if(IEnumIDList_Next(peidl, 1, &apidl, NULL) != S_OK) {
        skip("no files on desktop - skipping GetDataObject tests\n");
        IEnumIDList_Release(peidl);
        IShellFolder_Release(desktop);
        return;
    }
    IEnumIDList_Release(peidl);

    hr = IShellFolder_GetUIObjectOf(desktop, NULL, 1, (LPCITEMIDLIST*)&apidl,
            &IID_IDataObject, NULL, (void**)&data_obj);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ILFree(apidl);
    IShellFolder_Release(desktop);

    cf_shellidlist = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    fmt.cfFormat = cf_shellidlist;
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;
    hr = IDataObject_QueryGetData(data_obj, &fmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    fmt.tymed = TYMED_HGLOBAL | TYMED_ISTREAM;
    hr = IDataObject_QueryGetData(data_obj, &fmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    fmt.tymed = TYMED_ISTREAM;
    hr = IDataObject_QueryGetData(data_obj, &fmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    ok(hr == DV_E_FORMATETC, "Got hr %#lx.\n", hr);

    fmt.tymed = TYMED_HGLOBAL | TYMED_ISTREAM;
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "medium.tymed = %lx\n", medium.tymed);
    ReleaseStgMedium(&medium);

    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW);
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(medium.tymed == TYMED_HGLOBAL, "medium.tymed = %lx\n", medium.tymed);
        value = GlobalLock(medium.hGlobal);
        ok(*value == DROPEFFECT_LINK, "Got value %#x.\n", *value);
        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }

    fmt.cfFormat = CF_HDROP;
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    todo_wine ok(hr == DV_E_FORMATETC, "Got hr %#lx.\n", hr);
    fmt.cfFormat = RegisterClipboardFormatA(CFSTR_FILENAMEA);
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    todo_wine ok(hr == DV_E_FORMATETC, "Got hr %#lx.\n", hr);
    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_FILENAMEW);
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    todo_wine ok(hr == DV_E_FORMATETC, "Got hr %#lx.\n", hr);

    fmt.cfFormat = RegisterClipboardFormatW(L"bogus_format");

    hr = IDataObject_QueryGetData(data_obj, &fmt);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    ok(hr == DV_E_FORMATETC, "Got hr %#lx.\n", hr);

    global = GlobalAlloc(GMEM_MOVEABLE, sizeof(*value));
    value = GlobalLock(global);
    *value = 0xabacab;
    GlobalUnlock(global);
    medium.tymed = TYMED_HGLOBAL;
    medium.pUnkForRelease = NULL;
    medium.hGlobal = global;
    fmt.tymed = TYMED_HGLOBAL;
    hr = IDataObject_SetData(data_obj, &fmt, &medium, FALSE);
    ok(hr == E_INVALIDARG || hr == E_NOTIMPL /* win 8+ */, "Got hr %#lx.\n", hr);
    hr = IDataObject_SetData(data_obj, &fmt, &medium, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDataObject_QueryGetData(data_obj, &fmt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    memset(&medium, 0xcc, sizeof(medium));
    hr = IDataObject_GetData(data_obj, &fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.hGlobal && medium.hGlobal != global, "Got global %p.\n", medium.hGlobal);
    value = GlobalLock(medium.hGlobal);
    ok(*value == 0xabacab, "Got value %#x.\n", *value);
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

    for (unsigned int i = 0; i < ARRAY_SIZE(enum_directions); ++i)
    {
        hr = IDataObject_EnumFormatEtc(data_obj, enum_directions[i], &enum_format);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        memset(&fmt, 0xcc, sizeof(fmt));
        hr = IEnumFORMATETC_Next(enum_format, 1, &fmt, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ret = GetClipboardFormatNameW(fmt.cfFormat, format_name, ARRAY_SIZE(format_name));
        ok(ret > 0, "Got %d.\n",ret);
        ok(!wcscmp(format_name, CFSTR_SHELLIDLISTW), "Got clipboard format %s.\n", debugstr_w(format_name));
        ok(!fmt.ptd, "Got target device %p.\n", fmt.ptd);
        ok(fmt.dwAspect == DVASPECT_CONTENT, "Got aspect %#lx.\n", fmt.dwAspect);
        ok(fmt.lindex == -1, "Got index %ld.\n", fmt.lindex);
        ok(fmt.tymed == TYMED_HGLOBAL, "Got tymed %#lx.\n", fmt.tymed);

        hr = IEnumFORMATETC_Next(enum_format, 1, &fmt, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ret = GetClipboardFormatNameW(fmt.cfFormat, format_name, ARRAY_SIZE(format_name));
        ok(ret > 0, "Got %d.\n",ret);
        if (ret > 0)
            todo_wine ok(!wcscmp(format_name, CFSTR_PREFERREDDROPEFFECTW), "Got clipboard format %s.\n", debugstr_w(format_name));
        ok(!fmt.ptd, "Got target device %p.\n", fmt.ptd);
        ok(fmt.dwAspect == DVASPECT_CONTENT, "Got aspect %#lx.\n", fmt.dwAspect);
        ok(fmt.lindex == -1, "Got index %ld.\n", fmt.lindex);
        ok(fmt.tymed == TYMED_HGLOBAL, "Got tymed %#lx.\n", fmt.tymed);

        hr = IEnumFORMATETC_Next(enum_format, 1, &fmt, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ret = GetClipboardFormatNameW(fmt.cfFormat, format_name, ARRAY_SIZE(format_name));
        ok(ret > 0, "Got %d.\n",ret);
        todo_wine ok(!wcscmp(format_name, L"bogus_format"), "Got clipboard format %s.\n", debugstr_w(format_name));
        ok(!fmt.ptd, "Got target device %p.\n", fmt.ptd);
        ok(fmt.dwAspect == DVASPECT_CONTENT, "Got aspect %#lx.\n", fmt.dwAspect);
        ok(fmt.lindex == -1, "Got index %ld.\n", fmt.lindex);
        ok(fmt.tymed == TYMED_HGLOBAL, "Got tymed %#lx.\n", fmt.tymed);

        hr = IEnumFORMATETC_Next(enum_format, 1, &fmt, NULL);
        todo_wine ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

        IEnumFORMATETC_Release(enum_format);
    }

    test_enum_format(data_obj);

    IDataObject_Release(data_obj);
}

static void test_GetDefaultColumn(void)
{
    static const CLSID *folders[] =
    {
        &CLSID_MyComputer,
        &CLSID_MyDocuments,
        &CLSID_ControlPanel,
        &CLSID_NetworkPlaces,
        &CLSID_Printers,
        &CLSID_RecycleBin,
        &CLSID_ShellDesktop,
    };
    HRESULT hr;
    int i;

    CoInitialize(NULL);

    for (i = 0; i < ARRAY_SIZE(folders); i++)
    {
        IShellFolder2 *folder;
        ULONG sort, display;

        hr = CoCreateInstance(folders[i], NULL, CLSCTX_INPROC_SERVER, &IID_IShellFolder2, (void **)&folder);
        if (hr != S_OK)
        {
            win_skip("Failed to create folder %s, hr %#lx.\n", wine_dbgstr_guid(folders[i]), hr);
            continue;
        }

        hr = IShellFolder2_GetDefaultColumn(folder, 0, NULL, NULL);
        ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

        sort = display = 123;
        hr = IShellFolder2_GetDefaultColumn(folder, 0, &sort, &display);
        ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        ok(sort == 123 && display == 123, "Unexpected default column.\n");

        display = 123;
        hr = IShellFolder2_GetDefaultColumn(folder, 0, NULL, &display);
        ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        ok(display == 123, "Unexpected default column.\n");

        sort = 123;
        hr = IShellFolder2_GetDefaultColumn(folder, 0, &sort, NULL);
        ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        ok(sort == 123, "Unexpected default column.\n");

        IShellFolder2_Release(folder);
    }

    CoUninitialize();
}

static void test_GetDefaultSearchGUID(void)
{
    static const CLSID *folders[] =
    {
        &CLSID_MyComputer,
        &CLSID_MyDocuments,
        &CLSID_ControlPanel,
        &CLSID_NetworkPlaces,
        &CLSID_Printers,
        &CLSID_RecycleBin,
        &CLSID_ShellDesktop,
    };
    HRESULT hr;
    int i;

    CoInitialize(NULL);

    for (i = 0; i < ARRAY_SIZE(folders); i++)
    {
        IShellFolder2 *folder;
        GUID guid;

        hr = CoCreateInstance(folders[i], NULL, CLSCTX_INPROC_SERVER, &IID_IShellFolder2, (void **)&folder);
        if (hr != S_OK)
        {
            win_skip("Failed to create folder %s, hr %#lx.\n", wine_dbgstr_guid(folders[i]), hr);
            continue;
        }

        if (0)
        {
            /* crashes on XP */
            hr = IShellFolder2_GetDefaultSearchGUID(folder, NULL);
            ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        }

        memcpy(&guid, &CLSID_MyComputer, sizeof(guid));
        hr = IShellFolder2_GetDefaultSearchGUID(folder, &guid);
        ok(hr == E_NOTIMPL || broken(hr == S_OK) /* Method was last supported on XP */, "Unexpected hr %#lx.\n", hr);
        if (hr == E_NOTIMPL)
            ok(IsEqualGUID(&guid, &CLSID_MyComputer), "Unexpected guid %s.\n", wine_dbgstr_guid(&guid));

        IShellFolder2_Release(folder);
    }

    CoUninitialize();
}

static void test_SHLimitInputEdit(void)
{
    IShellFolder *desktop;
    HRESULT hr;
    HWND hwnd;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "Failed to get desktop folder, hr %#lx.\n", hr);

    hr = SHLimitInputEdit(NULL, desktop);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hwnd = CreateWindowA("EDIT", NULL, WS_VISIBLE, 0, 0, 100, 30, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "Failed to create Edit control.\n");

    hr = SHLimitInputEdit(hwnd, desktop);
    todo_wine
    ok(hr == S_OK, "Failed to set input limits, hr %#lx.\n", hr);

    hr = SHLimitInputEdit(hwnd, desktop);
    todo_wine
    ok(hr == S_OK, "Failed to set input limits, hr %#lx.\n", hr);

    DestroyWindow(hwnd);
    IShellFolder_Release(desktop);
}

static void test_SHGetSetFolderCustomSettings(void)
{
    HRESULT hr;
    SHFOLDERCUSTOMSETTINGS fcs;
    WCHAR pathW[MAX_PATH];
    WCHAR bufferW[MAX_PATH];
    WCHAR iconpathW[MAX_PATH];

    if (!pSHGetSetFolderCustomSettings)
    {
        win_skip("SHGetSetFolderCustomSetting is not available.\n");
        return;
    }

    GetTempPathW(MAX_PATH, pathW);
    lstrcatW(pathW, L"some_dir");
    CreateDirectoryW(pathW, NULL);

    lstrcpyW(iconpathW, pathW);
    lstrcatW(iconpathW, L"\\some_icon.ico");

    memset(&fcs, 0, sizeof(fcs));
    fcs.dwSize = sizeof(fcs);
    fcs.dwMask = FCSM_ICONFILE;
    fcs.pszIconFile = iconpathW;

    hr = pSHGetSetFolderCustomSettings(&fcs, pathW, FCS_FORCEWRITE); /*creates and writes to a Desktop.ini*/
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    memset(&fcs, 0, sizeof(fcs));
    fcs.dwSize = sizeof(fcs);
    fcs.dwMask = FCSM_ICONFILE;
    fcs.cchIconFile = MAX_PATH;
    fcs.pszIconFile = bufferW;
    bufferW[0] = 0;

    hr = pSHGetSetFolderCustomSettings(&fcs, pathW, FCS_READ);
    todo_wine ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    todo_wine ok(!lstrcmpiW(iconpathW, fcs.pszIconFile), "Expected %s, got %s\n", wine_dbgstr_w(iconpathW), wine_dbgstr_w(fcs.pszIconFile));

    hr = pSHGetSetFolderCustomSettings(&fcs, NULL, FCS_READ);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    lstrcpyW(bufferW, pathW);
    lstrcatW(bufferW, L"\\Desktop.ini");
    DeleteFileW(bufferW);
    RemoveDirectoryW(pathW);
}

static void test_SHOpenFolderAndSelectItems(void)
{
    PIDLIST_ABSOLUTE folder, items[2];
    HRESULT hr;

    /* NULL folder */
    hr = SHOpenFolderAndSelectItems(NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    /* Open and select folder without child items */
    folder = ILCreateFromPathW(L"C:\\Windows\\System32");
    hr = SHOpenFolderAndSelectItems(folder, 0, NULL, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(check_window_exists("Windows"), "Failed to create window.\n");
    ILFree(folder);

    /* Open folder and select one child item */
    folder = ILCreateFromPathW(L"C:\\Windows");
    items[0] = ILCreateFromPathW(L"C:\\Windows\\System32");
    hr = SHOpenFolderAndSelectItems(folder, 1, (PCUITEMID_CHILD_ARRAY)items, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(check_window_exists("Windows"), "Failed to create window.\n");
    ILFree(items[0]);
    ILFree(folder);

    /* Open folder and select two child items */
    folder = ILCreateFromPathW(L"C:\\Windows");
    items[0] = ILCreateFromPathW(L"C:\\Windows\\System32");
    items[1] = ILCreateFromPathW(L"C:\\Windows\\Resources");
    hr = SHOpenFolderAndSelectItems(folder, 2, (PCUITEMID_CHILD_ARRAY)items, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(check_window_exists("Windows"), "Failed to create window.\n");
    ILFree(items[1]);
    ILFree(items[0]);
    ILFree(folder);

    /* Open folder and select one child item with OFASI_EDIT */
    folder = ILCreateFromPathW(L"C:\\Windows");
    items[0] = ILCreateFromPathW(L"C:\\Windows\\System32");
    hr = SHOpenFolderAndSelectItems(folder, 1, (PCUITEMID_CHILD_ARRAY)items, OFASI_EDIT);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(check_window_exists("Windows"), "Failed to create window.\n");
    ILFree(items[0]);
    ILFree(folder);

    /* Open folder and select two child items and OFASI_EDIT */
    folder = ILCreateFromPathW(L"C:\\Windows");
    items[0] = ILCreateFromPathW(L"C:\\Windows\\System32");
    items[1] = ILCreateFromPathW(L"C:\\Windows\\Resources");
    hr = SHOpenFolderAndSelectItems(folder, 2, (PCUITEMID_CHILD_ARRAY)items, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(check_window_exists("Windows"), "Failed to create window.\n");
    ILFree(items[1]);
    ILFree(items[0]);
    ILFree(folder);
}

static void test_SHBindToFolderIDListParent(void)
{
    IShellFolder *psf_desktop;
    LPITEMIDLIST pidl;
    HRESULT hr;
    WCHAR path[MAX_PATH];
    SHITEMID empty_item = { 0, { 0 } };
    LPITEMIDLIST pidl_empty = (LPITEMIDLIST)&empty_item;
    LPCITEMIDLIST pidl_last;
    IShellFolder *psf;

    if (!pSHBindToFolderIDListParent)
    {
        win_skip("SHBindToFolderIDListParent not available\n");
        return;
    }

    GetTempPathW(ARRAY_SIZE(path), path);
    SHGetDesktopFolder(&psf_desktop);

    hr = IShellFolder_ParseDisplayName(psf_desktop, NULL, NULL, path, NULL, &pidl, 0);
    ok(hr == S_OK, "got %#lx\n", hr);

    pidl_last = NULL;
    hr = pSHBindToFolderIDListParent(psf_desktop, pidl, &IID_IShellFolder, (void **)&psf, &pidl_last);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(pidl_last != NULL, "got %p\n", pidl_last);
    IShellFolder_Release(psf);

    hr = pSHBindToFolderIDListParent(NULL, pidl_empty, &IID_IShellFolder, (void **)&psf, &pidl_last);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(pidl_last == pidl_empty, "got %p\n", pidl_last);
    IShellFolder_Release(psf);

    hr = pSHBindToFolderIDListParent(NULL, pidl, &IID_IShellFolder, (void **)&psf, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    IShellFolder_Release(psf);

    if (0) /* crashes under Windows */
        hr = pSHBindToFolderIDListParent(NULL, pidl, &IID_IShellFolder, NULL, NULL);

    ILFree(pidl);
    IShellFolder_Release(psf_desktop);

    pidl_last = (LPITEMIDLIST)0xdeadbeef;
    hr = pSHBindToFolderIDListParent(NULL, NULL, &IID_IShellFolder, (void **)&psf, &pidl_last);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);
    ok(pidl_last == NULL, "got %p\n", pidl_last);
}

static void test_copy_paste(void)
{
    CMINVOKECOMMANDINFO invoke_info = {.cbSize = sizeof(invoke_info)};
    WCHAR cwd[MAX_PATH], temp_path[MAX_PATH], path[MAX_PATH];
    ITEMIDLIST *pidl, *src_pidl, *dst_pidl, *pidls[2];
    IShellFolder *tmp_folder, *dst_folder;
    IContextMenu *src_menu, *dst_menu;
    const DROPFILES *dropfiles;
    const WCHAR *filenameW;
    IDataObject *data_obj;
    const CIDA *cida;
    STGMEDIUM medium;
    FORMATETC format;
    DWORD *effect;
    HRESULT hr;
    BOOL ret;

    format.dwAspect = DVASPECT_CONTENT;
    format.ptd = NULL;
    format.tymed = TYMED_HGLOBAL;
    format.lindex = -1;

    GetCurrentDirectoryW(ARRAY_SIZE(cwd), cwd);
    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    SetCurrentDirectoryW(temp_path);

    ret = CreateDirectoryW(L"testcopy_src", NULL);
    ok(ret, "Got error %lu.\n", GetLastError());

    ret = CreateDirectoryW(L"testcopy_dst", NULL);
    ok(ret, "Got error %lu.\n", GetLastError());

    hr = SHParseDisplayName(temp_path, NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = SHBindToObject(NULL, pidl, NULL, &IID_IShellFolder, (void **)&tmp_folder);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ILFree(pidl);

    hr = IShellFolder_ParseDisplayName(tmp_folder, NULL, NULL, (WCHAR *)L"testcopy_src", NULL, &src_pidl, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IShellFolder_ParseDisplayName(tmp_folder, NULL, NULL, (WCHAR *)L"testcopy_dst", NULL, &dst_pidl, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IShellFolder_BindToObject(tmp_folder, dst_pidl, NULL, &IID_IShellFolder, (void **)&dst_folder);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IShellFolder_GetUIObjectOf(tmp_folder, NULL, 1, (const ITEMIDLIST **)&src_pidl,
            &IID_IContextMenu, NULL, (void **)&src_menu);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Cut. */

    invoke_info.lpVerb = "cut";
    hr = IContextMenu_InvokeCommand(src_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    trace("%p ole %p shell %p\n", data_obj->lpVtbl->SetData, GetModuleHandleW(L"ole32"), GetModuleHandleW(L"shell32"));

    format.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ReleaseStgMedium(&medium);
    format.cfFormat = CF_HDROP;
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ReleaseStgMedium(&medium);
    format.cfFormat = RegisterClipboardFormatA(CFSTR_FILENAMEA);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ReleaseStgMedium(&medium);
    format.cfFormat = RegisterClipboardFormatW(CFSTR_FILENAMEW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ReleaseStgMedium(&medium);

    format.cfFormat = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IDataObject_Release(data_obj);

    ret = GetFileAttributesW(L"testcopy_src");
    ok(ret != INVALID_FILE_ATTRIBUTES, "Got %#x.\n", ret);

    hr = IShellFolder_GetUIObjectOf(tmp_folder, NULL, 1, (const ITEMIDLIST **)&dst_pidl,
            &IID_IContextMenu, NULL, (void **)&dst_menu);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    invoke_info.lpVerb = "paste";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = MoveFileExW(L"testcopy_dst/testcopy_src", L"testcopy_src", 0);
    ok(ret, "Got error %lu.\n", GetLastError());

    /* Copy. */

    invoke_info.lpVerb = "copy";
    hr = IContextMenu_InvokeCommand(src_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    format.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ReleaseStgMedium(&medium);

    format.cfFormat = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IDataObject_Release(data_obj);

    ret = GetFileAttributesW(L"testcopy_src");
    ok(ret != INVALID_FILE_ATTRIBUTES, "Got %#x.\n", ret);

    hr = IShellFolder_GetUIObjectOf(tmp_folder, NULL, 1, (const ITEMIDLIST **)&dst_pidl,
            &IID_IContextMenu, NULL, (void **)&dst_menu);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    invoke_info.lpVerb = "paste";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = GetFileAttributesW(L"testcopy_src");
    ok(ret != INVALID_FILE_ATTRIBUTES, "Got %#x.\n", ret);

    ret = RemoveDirectoryW(L"testcopy_dst/testcopy_src");
    ok(ret, "Got error %lu.\n", GetLastError());

    /* Manually change the drop effect back to "cut". */

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    format.cfFormat = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW);
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
    effect = GlobalLock(medium.hGlobal);
    *effect = DROPEFFECT_MOVE;
    GlobalUnlock(medium.hGlobal);
    hr = IDataObject_SetData(data_obj, &format, &medium, TRUE);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IDataObject_Release(data_obj);

    invoke_info.lpVerb = "paste";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = MoveFileExW(L"testcopy_dst/testcopy_src", L"testcopy_src", 0);
    todo_wine ok(ret, "Got error %lu.\n", GetLastError());
    if (!ret && GetLastError() == ERROR_ALREADY_EXISTS)
        RemoveDirectoryW(L"testcopy_dst/testcopy_src");

    /* Paste into a background menu. */

    IContextMenu_Release(dst_menu);

    invoke_info.lpVerb = "copy";
    hr = IContextMenu_InvokeCommand(src_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IShellFolder_CreateViewObject(dst_folder, NULL, &IID_IContextMenu, (void **)&dst_menu);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    invoke_info.lpVerb = "paste";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = GetFileAttributesW(L"testcopy_src");
    ok(ret != INVALID_FILE_ATTRIBUTES, "Got %#x.\n", ret);

    ret = RemoveDirectoryW(L"testcopy_dst/testcopy_src");
    ok(ret, "Got error %lu.\n", GetLastError());

    /* Paste into a selection comprising multiple directories. In this case the
     * first directory is used, and the second is just ignored.
     * This same behaviour can of course be observed when using the UI. */

    IContextMenu_Release(dst_menu);

    ret = CreateDirectoryW(L"testcopy_dst2", NULL);
    ok(ret, "Got error %lu.\n", GetLastError());

    hr = IShellFolder_ParseDisplayName(tmp_folder, NULL, NULL, (WCHAR *)L"testcopy_dst2", NULL, &pidls[0], NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    pidls[1] = dst_pidl;

    hr = IShellFolder_GetUIObjectOf(tmp_folder, NULL, 2, (const ITEMIDLIST **)pidls,
            &IID_IContextMenu, NULL, (void **)&dst_menu);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    invoke_info.lpVerb = "paste";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ret = RemoveDirectoryW(L"testcopy_dst2/testcopy_src");
    ok(ret, "Got error %lu.\n", GetLastError());
    ret = GetFileAttributesW(L"testcopy_dst/testcopy_src");
    ok(ret == INVALID_FILE_ATTRIBUTES, "Got %#x.\n", ret);

    /* Cut multiple files, and test the clipboard contents. */

    invoke_info.lpVerb = "cut";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    format.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    cida = GlobalLock(medium.hGlobal);
    ok(cida->cidl == 2, "Got count %u.\n", cida->cidl);
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

    format.cfFormat = CF_HDROP;
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    dropfiles = GlobalLock(medium.hGlobal);
    ok(dropfiles->pFiles == sizeof(DROPFILES), "Got offset %lu.\n", dropfiles->pFiles);
    ok(dropfiles->fWide == TRUE, "Got wide %u.\n", dropfiles->fWide);
    filenameW = (const WCHAR *)((const char *)dropfiles + dropfiles->pFiles);
    swprintf(path, ARRAY_SIZE(path), L"%stestcopy_dst2", temp_path);
    ok(!wcscmp(filenameW, path), "Got path %s.\n", debugstr_w(filenameW));
    filenameW += wcslen(filenameW) + 1;
    swprintf(path, ARRAY_SIZE(path), L"%stestcopy_dst", temp_path);
    ok(!wcscmp(filenameW, path), "Got path %s.\n", debugstr_w(filenameW));
    filenameW += wcslen(filenameW) + 1;
    ok(!filenameW[0], "Got path %s.\n", debugstr_w(filenameW));
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

    format.cfFormat = RegisterClipboardFormatA(CFSTR_FILENAMEA);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ReleaseStgMedium(&medium);

    format.cfFormat = RegisterClipboardFormatW(CFSTR_FILENAMEW);
    hr = IDataObject_GetData(data_obj, &format, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    filenameW = GlobalLock(medium.hGlobal);
    swprintf(path, ARRAY_SIZE(path), L"%stestcopy_dst2", temp_path);
    ok(!wcscmp(filenameW, path), "Got path %s.\n", debugstr_w(filenameW));
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

    IDataObject_Release(data_obj);

    ILFree(pidls[0]);

    /* Paste with nothing in the clipboard. */

    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    invoke_info.lpVerb = "paste";
    hr = IContextMenu_InvokeCommand(dst_menu, &invoke_info);
    ok(hr == S_OK || hr == S_FALSE /* win10 < 1809 */, "Got hr %#lx.\n", hr);

    ret = RemoveDirectoryW(L"testcopy_src");
    ok(ret, "Got error %lu.\n", GetLastError());
    ret = RemoveDirectoryW(L"testcopy_dst");
    ok(ret, "Got error %lu.\n", GetLastError());
    ret = RemoveDirectoryW(L"testcopy_dst2");
    ok(ret, "Got error %lu.\n", GetLastError());

    IContextMenu_Release(src_menu);
    IContextMenu_Release(dst_menu);
    ILFree(src_pidl);
    ILFree(dst_pidl);
    IShellFolder_Release(dst_folder);
    IShellFolder_Release(tmp_folder);
    SetCurrentDirectoryW(cwd);
}

START_TEST(shlfolder)
{
    init_function_pointers();
    /* if OleInitialize doesn't get called, ParseDisplayName returns
       CO_E_NOTINITIALIZED for malformed directory names */
    OleInitialize(NULL);

    test_SHBindToFolderIDListParent();
    test_ParseDisplayName();
    test_SHParseDisplayName();
    test_BindToObject();
    test_EnumObjects_and_CompareIDs();
    test_GetDisplayName();
    test_GetAttributesOf();
    test_SHGetPathFromIDList();
    test_CallForAttributes();
    test_FolderShortcut();
    test_ITEMIDLIST_format();
    test_SHGetFolderPathA();
    test_SHGetFolderPathAndSubDirA();
    test_LocalizedNames();
    test_SHCreateShellItem();
    test_SHCreateShellItemArray();
    test_ShellItemArrayEnumItems();
    test_desktop_IPersist();
    test_IShellItemImageFactory();
    test_GetUIObject();
    test_CreateViewObject_contextmenu();
    test_SHSimpleIDListFromPath();
    test_ParseDisplayNamePBC();
    test_SHGetNameFromIDList();
    test_SHGetItemFromDataObject();
    test_SHGetIDListFromObject();
    test_SHGetItemFromObject();
    test_ShellItemCompare();
    test_SHChangeNotify(FALSE);
    test_SHChangeNotify(TRUE);
    test_ShellItemBindToHandler();
    test_ShellItemGetAttributes();
    test_ShellItemArrayGetAttributes();
    test_SHCreateDefaultContextMenu();
    test_DataObject();
    test_GetDefaultColumn();
    test_GetDefaultSearchGUID();
    test_SHLimitInputEdit();
    test_SHGetSetFolderCustomSettings();
    test_SHOpenFolderAndSelectItems();
    test_copy_paste();

    OleUninitialize();
}
