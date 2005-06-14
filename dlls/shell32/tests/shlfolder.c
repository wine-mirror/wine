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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"


#include "shlguid.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "shlwapi.h"


#include "wine/unicode.h"
#include "wine/test.h"


static IMalloc *ppM;

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
    ITEMIDLIST *newPIDL, *(idlArr [5]);
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

    /* Just test SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR for now */
    static const ULONG attrs[5] =
    {
        SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR,
        SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR,
        SFGAO_FILESYSTEM,
        SFGAO_FILESYSTEM,
        SFGAO_FILESYSTEM,
    };

    hr = IShellFolder_EnumObjects(iFolder, NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &iEnumList);
    ok(hr == S_OK, "EnumObjects failed %08lx\n", hr);

    while (IEnumIDList_Next(iEnumList, 1, &newPIDL, &NumPIDLs) == S_OK)
    {
        idlArr[i++] = newPIDL;
    }

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


    for (i = 0; i < 5; i++)
    {
        SFGAOF flags;
        flags = SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR;
        hr = IShellFolder_GetAttributesOf(iFolder, 1, (LPCITEMIDLIST*)(idlArr + i), &flags);
        flags &= SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR;
        ok(hr == S_OK, "GetAttributesOf returns %08lx\n", hr);
        ok(flags == attrs[i], "GetAttributesOf gets attrs %08lx, expects %08lx\n", flags, attrs[i]);
    }

    for (i=0;i<5;i++)
        IMalloc_Free(ppM, idlArr[i]);
}

static void test_BindToObject(void)
{
    HRESULT hr;
    UINT cChars;
    IShellFolder *psfDesktop, *psfChild, *psfMyComputer, *psfSystemDir;
    SHITEMID emptyitem = { 0, { 0 } };
    LPITEMIDLIST pidlMyComputer, pidlSystemDir, pidlEmpty = (LPITEMIDLIST)&emptyitem;
    WCHAR wszSystemDir[MAX_PATH];
    WCHAR wszMyComputer[] = { 
        ':',':','{','2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-',
        'A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D','}',0 };

    /* The following tests shows that BindToObject should fail with E_INVALIDARG if called
     * with an empty pidl. This is tested for Desktop, MyComputer and the FS ShellFolder
     */
    hr = SHGetDesktopFolder(&psfDesktop);
    ok (SUCCEEDED(hr), "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (FAILED(hr)) return;
    
    hr = IShellFolder_BindToObject(psfDesktop, pidlEmpty, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, "Desktop's BindToObject should fail, when called with empty pidl! hr = %08lx\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszMyComputer, NULL, &pidlMyComputer, NULL);
    ok (SUCCEEDED(hr), "Desktop's ParseDisplayName failed to parse MyComputer's CLSID! hr = %08lx\n", hr);
    if (FAILED(hr)) {
        IShellFolder_Release(psfDesktop);
        return;
    }
    
    hr = IShellFolder_BindToObject(psfDesktop, pidlMyComputer, NULL, &IID_IShellFolder, (LPVOID*)&psfMyComputer);
    ok (SUCCEEDED(hr), "Desktop failed to bind to MyComputer object! hr = %08lx\n", hr);
    IShellFolder_Release(psfDesktop);
    ILFree(pidlMyComputer);
    if (FAILED(hr)) return;

    hr = IShellFolder_BindToObject(psfMyComputer, pidlEmpty, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, "MyComputers's BindToObject should fail, when called with empty pidl! hr = %08lx\n", hr);

    cChars = GetSystemDirectoryW(wszSystemDir, MAX_PATH);
    ok (cChars > 0 && cChars < MAX_PATH, "GetSystemDirectoryW failed! LastError: %08lx\n", GetLastError());
    if (cChars == 0 || cChars >= MAX_PATH) {
        IShellFolder_Release(psfMyComputer);
        return;
    }
    
    hr = IShellFolder_ParseDisplayName(psfMyComputer, NULL, NULL, wszSystemDir, NULL, &pidlSystemDir, NULL);
    ok (SUCCEEDED(hr), "MyComputers's ParseDisplayName failed to parse the SystemDirectory! hr = %08lx\n", hr);
    if (FAILED(hr)) {
        IShellFolder_Release(psfMyComputer);
        return;
    }

    hr = IShellFolder_BindToObject(psfMyComputer, pidlSystemDir, NULL, &IID_IShellFolder, (LPVOID*)&psfSystemDir);
    ok (SUCCEEDED(hr), "MyComputer failed to bind to a FileSystem ShellFolder! hr = %08lx\n", hr);
    IShellFolder_Release(psfMyComputer);
    ILFree(pidlSystemDir);
    if (FAILED(hr)) return;

    hr = IShellFolder_BindToObject(psfSystemDir, pidlEmpty, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
    ok (hr == E_INVALIDARG, 
        "FileSystem ShellFolder's BindToObject should fail, when called with empty pidl! hr = %08lx\n", hr);
    
    IShellFolder_Release(psfSystemDir);
}
  
static void test_GetDisplayName(void)
{
    BOOL result;
    HRESULT hr;
    HANDLE hTestFile;
    WCHAR wszTestFile[MAX_PATH], wszTestFile2[MAX_PATH], wszTestDir[MAX_PATH];
    STRRET strret;
    LPSHELLFOLDER psfDesktop, psfPersonal;
    IUnknown *psfFile;
    LPITEMIDLIST pidlTestFile;
    LPCITEMIDLIST pidlLast;
    static const WCHAR wszFileName[] = { 'w','i','n','e','t','e','s','t','.','f','o','o',0 };
    static const WCHAR wszDirName[] = { 'w','i','n','e','t','e','s','t',0 };

    /* I'm trying to figure if there is a functional difference between calling
     * SHGetPathFromIDList and calling GetDisplayNameOf(SHGDN_FORPARSING) after
     * binding to the shellfolder. One thing I thought of was that perhaps 
     * SHGetPathFromIDList would be able to get the path to a file, which does
     * not exist anymore, while the other method would'nt. It turns out there's
     * no functional difference in this respect.
     */

    /* First creating a directory in MyDocuments and a file in this directory. */
    result = SHGetSpecialFolderPathW(NULL, wszTestDir, CSIDL_PERSONAL, FALSE);
    ok(result, "SHGetSpecialFolderPathW failed! Last error: %08lx\n", GetLastError());
    if (!result) return;

    PathAddBackslashW(wszTestDir);
    lstrcatW(wszTestDir, wszDirName);
    result = CreateDirectoryW(wszTestDir, NULL);
    ok(result, "CreateDirectoryW failed! Last error: %08lx\n", GetLastError());
    if (!result) return;

    lstrcpyW(wszTestFile, wszTestDir);
    PathAddBackslashW(wszTestFile);
    lstrcatW(wszTestFile, wszFileName);

    hTestFile = CreateFileW(wszTestFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    ok(hTestFile != INVALID_HANDLE_VALUE, "CreateFileW failed! Last error: %08lx\n", GetLastError());
    if (hTestFile == INVALID_HANDLE_VALUE) return;
    CloseHandle(hTestFile);

    /* Getting a itemidlist for the file. */
    hr = SHGetDesktopFolder(&psfDesktop);
    ok(SUCCEEDED(hr), "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (FAILED(hr)) return;

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszTestFile, NULL, &pidlTestFile, NULL);
    ok(SUCCEEDED(hr), "Desktop->ParseDisplayName failed! hr = %08lx\n", hr);
    if (FAILED(hr)) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    /* It seems as if we can not bind to regular files on windows, but only directories. 
     * XP sp2 returns 0x80070002, which is not defined in the PSDK 
     */
    hr = IShellFolder_BindToObject(psfDesktop, pidlTestFile, NULL, &IID_IUnknown, (VOID**)&psfFile);
    todo_wine { ok (hr == 0x80070002, "hr = %08lx\n", hr); }
    if (SUCCEEDED(hr)) {
        IShellFolder_Release(psfFile);
    }
    
    /* Deleting the file and the directory */
    DeleteFileW(wszTestFile);
    RemoveDirectoryW(wszTestDir);

    /* SHGetPathFromIDListW still works, although the file is not present anymore. */
    result = SHGetPathFromIDListW(pidlTestFile, wszTestFile2);
    ok (result, "SHGetPathFromIDListW failed! Last error: %08lx\n", GetLastError());
    ok (!lstrcmpiW(wszTestFile, wszTestFile2), "SHGetPathFromIDListW returns incorrect path!\n");

    /* Binding to the folder and querying the display name of the file also works. */
    hr = SHBindToParent(pidlTestFile, &IID_IShellFolder, (VOID**)&psfPersonal, &pidlLast); 
    ok (SUCCEEDED(hr), "SHBindToParent failed! hr = %08lx\n", hr);
    if (FAILED(hr)) {
        IShellFolder_Release(psfDesktop);
        return;
    }

    hr = IShellFolder_GetDisplayNameOf(psfPersonal, pidlLast, SHGDN_FORPARSING, &strret);
    ok (SUCCEEDED(hr), "Personal->GetDisplayNameOf failed! hr = %08lx\n", hr);
    if (FAILED(hr)) {
        IShellFolder_Release(psfDesktop);
        IShellFolder_Release(psfPersonal);
        return;
    }
    
    hr = StrRetToBufW(&strret, pidlLast, wszTestFile2, MAX_PATH);
    ok (SUCCEEDED(hr), "StrRetToBufW failed! hr = %08lx\n", hr);
    ok (!lstrcmpiW(wszTestFile, wszTestFile2), "GetDisplayNameOf returns incorrect path!\n");
    
    IShellFolder_Release(psfDesktop);
    IShellFolder_Release(psfPersonal);
}

static void test_GetAttributesOf(void) 
{
    HRESULT hr;
    LPSHELLFOLDER psfDesktop;
    SHITEMID emptyitem = { 0, { 0 } };
    LPCITEMIDLIST pidlEmpty = (LPCITEMIDLIST)&emptyitem;
    DWORD dwFlags;
    const static DWORD dwDesktopFlags = /* As observed on WinXP SP2 */
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGE_ANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
    
    hr = SHGetDesktopFolder(&psfDesktop);
    ok (SUCCEEDED(hr), "SHGetDesktopFolder failed! hr = %08lx\n", hr);
    if (FAILED(hr)) return;

    /* The Desktop attributes can be queried with a single empty itemidlist, .. */
    dwFlags = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 1, &pidlEmpty, &dwFlags);
    ok (SUCCEEDED(hr), "Desktop->GetAttributesOf failed! hr = %08lx\n", hr);
    ok (dwFlags == dwDesktopFlags, "Wrong Desktop attributes: %08lx, expected: %08lx\n", 
        dwFlags, dwDesktopFlags);

    /* .. or with no itemidlist at all. */
    dwFlags = 0xffffffff;
    hr = IShellFolder_GetAttributesOf(psfDesktop, 0, NULL, &dwFlags);
    ok (SUCCEEDED(hr), "Desktop->GetAttributesOf failed! hr = %08lx\n", hr);
    ok (dwFlags == dwDesktopFlags, "Wrong Desktop attributes: %08lx, expected: %08lx\n", 
        dwFlags, dwDesktopFlags);
    
    IShellFolder_Release(psfDesktop);
}    

START_TEST(shlfolder)
{
    ITEMIDLIST *newPIDL;
    IShellFolder *IDesktopFolder, *testIShellFolder;
    char  cCurrDirA [MAX_PATH] = {0};
    WCHAR cCurrDirW [MAX_PATH];
    static const WCHAR cTestDirW[] = {'\\','t','e','s','t','d','i','r',0};
    HRESULT hr;
    
    GetCurrentDirectoryA(MAX_PATH, cCurrDirA);
    MultiByteToWideChar(CP_ACP, 0, cCurrDirA, -1, cCurrDirW, MAX_PATH);
    strcatW(cCurrDirW, cTestDirW);

    OleInitialize(NULL);

    hr = SHGetMalloc(&ppM);
    ok(hr == S_OK, "SHGetMalloc failed %08lx\n", hr);

    CreateFilesFolders();
    
    hr = SHGetDesktopFolder(&IDesktopFolder);
    ok(hr == S_OK, "SHGetDesktopfolder failed %08lx\n", hr);

    hr = IShellFolder_ParseDisplayName(IDesktopFolder, NULL, NULL, cCurrDirW, NULL, &newPIDL, 0);
    ok(hr == S_OK, "ParseDisplayName failed %08lx\n", hr);

    hr = IShellFolder_BindToObject(IDesktopFolder, newPIDL, NULL, (REFIID)&IID_IShellFolder, (LPVOID *)&testIShellFolder);
    ok(hr == S_OK, "BindToObject failed %08lx\n", hr);
        
    test_EnumObjects(testIShellFolder);
    test_BindToObject();
    test_GetDisplayName();
    test_GetAttributesOf();

    hr = IShellFolder_Release(testIShellFolder);
    ok(hr == S_OK, "IShellFolder_Release failed %08lx\n", hr);

    IMalloc_Free(ppM, newPIDL);

    Cleanup();
}
