/*
 * Unit tests for shelllinks
 *
 * Copyright 2004 Mike McCormack
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
 *
 */

#define COBJMACROS

#include "initguid.h"
#include "windows.h"
#include "shlguid.h"
#include "shobjidl.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "shellapi.h"
#include "commoncontrols.h"

#include "wine/test.h"

#include "shell32_test.h"

static HRESULT (WINAPI *pSHILCreateFromPath)(LPCWSTR, LPITEMIDLIST *,DWORD*);
static HRESULT (WINAPI *pSHGetFolderLocation)(HWND,INT,HANDLE,DWORD,PIDLIST_ABSOLUTE*);
static HRESULT (WINAPI *pSHGetStockIconInfo)(SHSTOCKICONID, UINT, SHSTOCKICONINFO *);
static UINT (WINAPI *pSHExtractIconsW)(LPCWSTR, int, int, int, HICON *, UINT *, UINT, UINT);
static BOOL (WINAPI *pIsProcessDPIAware)(void);

/* For some reason SHILCreateFromPath does not work on Win98 and
 * SHSimpleIDListFromPathA does not work on NT4. But if we call both we
 * get what we want on all platforms.
 */
static LPITEMIDLIST (WINAPI *pSHSimpleIDListFromPathAW)(LPCVOID);

static LPITEMIDLIST path_to_pidl(const char* path)
{
    LPITEMIDLIST pidl;

    if (!pSHSimpleIDListFromPathAW)
    {
        HMODULE hdll=GetModuleHandleA("shell32.dll");
        pSHSimpleIDListFromPathAW=(void*)GetProcAddress(hdll, (char*)162);
        if (!pSHSimpleIDListFromPathAW)
            win_skip("SHSimpleIDListFromPathAW not found in shell32.dll\n");
    }

    pidl=NULL;
    /* pSHSimpleIDListFromPathAW maps to A on non NT platforms */
    if (pSHSimpleIDListFromPathAW && (GetVersion() & 0x80000000))
        pidl=pSHSimpleIDListFromPathAW(path);

    if (!pidl)
    {
        WCHAR* pathW;
        HRESULT r;
        int len;

        len=MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        pathW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);

        r=pSHILCreateFromPath(pathW, &pidl, NULL);
        ok(r == S_OK, "SHILCreateFromPath failed (0x%08lx)\n", r);
        free(pathW);
    }
    return pidl;
}


/*
 * Test manipulation of an IShellLink's properties.
 */

static void test_get_set(void)
{
    HRESULT r;
    IShellLinkA *sl;
    IShellLinkW *slW = NULL;
    char mypath[MAX_PATH];
    char buffer[INFOTIPSIZE];
    WIN32_FIND_DATAA finddata;
    LPITEMIDLIST pidl, tmp_pidl;
    const char * str;
    int i;
    WORD w;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (LPVOID*)&sl);
    ok(r == S_OK, "no IID_IShellLinkA (0x%08lx)\n", r);
    if (r != S_OK)
        return;

    /* Test Getting / Setting the description */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetDescription failed (0x%08lx)\n", r);
    ok(*buffer=='\0', "GetDescription returned '%s'\n", buffer);

    str="Some description";
    r = IShellLinkA_SetDescription(sl, str);
    ok(r == S_OK, "SetDescription failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetDescription failed (0x%08lx)\n", r);
    ok(strcmp(buffer,str)==0, "GetDescription returned '%s'\n", buffer);

    r = IShellLinkA_SetDescription(sl, NULL);
    ok(r == S_OK, "SetDescription failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetDescription failed (0x%08lx)\n", r);
    ok(!*buffer, "GetDescription returned '%s'\n", buffer);

    /* Test Getting / Setting the work directory */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetWorkingDirectory(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetWorkingDirectory failed (0x%08lx)\n", r);
    ok(*buffer=='\0', "GetWorkingDirectory returned '%s'\n", buffer);

    str="c:\\nonexistent\\directory";
    r = IShellLinkA_SetWorkingDirectory(sl, str);
    ok(r == S_OK, "SetWorkingDirectory failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetWorkingDirectory(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetWorkingDirectory failed (0x%08lx)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetWorkingDirectory returned '%s'\n", buffer);

    /* Test Getting / Setting the path */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    ok(r == S_FALSE, "GetPath failed (0x%08lx)\n", r);
    ok(!*buffer, "GetPath returned '%s'\n", buffer);

    strcpy(buffer,"garbage");
    memset(&finddata, 0xaa, sizeof(finddata));
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), &finddata, SLGP_RAWPATH);
    ok(r == S_FALSE, "GetPath failed (0x%08lx)\n", r);
    ok(!*buffer, "GetPath returned '%s'\n", buffer);
    ok(finddata.dwFileAttributes == 0, "unexpected attributes %lx\n", finddata.dwFileAttributes);
    ok(finddata.cFileName[0] == 0, "unexpected filename '%s'\n", finddata.cFileName);

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&slW);
    ok(r == S_OK, "CoCreateInstance failed (0x%08lx)\n", r);
    IShellLinkW_Release(slW);

    r = IShellLinkA_SetPath(sl, NULL);
    ok(r == E_INVALIDARG, "Unexpected hr %#lx.\n", r);

    r = IShellLinkA_SetPath(sl, "");
    ok(r==S_OK, "SetPath failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    ok(r == S_FALSE, "GetPath failed (0x%08lx)\n", r);
    ok(*buffer=='\0', "GetPath returned '%s'\n", buffer);

    /* Win98 returns S_FALSE, but WinXP returns S_OK */
    str="c:\\nonexistent\\file";
    r = IShellLinkA_SetPath(sl, str);
    ok(r==S_FALSE || r==S_OK, "SetPath failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    ok(r == S_OK, "GetPath failed (0x%08lx)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetPath returned '%s'\n", buffer);

    strcpy(buffer,"garbage");
    memset(&finddata, 0xaa, sizeof(finddata));
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), &finddata, SLGP_RAWPATH);
    ok(r == S_OK, "GetPath failed (0x%08lx)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetPath returned '%s'\n", buffer);
    ok(finddata.dwFileAttributes == 0, "unexpected attributes %lx\n", finddata.dwFileAttributes);
    ok(lstrcmpiA(finddata.cFileName, "file") == 0, "unexpected filename '%s'\n", finddata.cFileName);

    /* Get some real path to play with */
    GetWindowsDirectoryA( mypath, sizeof(mypath)-12 );
    strcat(mypath, "\\regedit.exe");

    /* Test the interaction of SetPath and SetIDList */
    tmp_pidl=NULL;
    r = IShellLinkA_GetIDList(sl, &tmp_pidl);
    ok(r == S_OK, "GetIDList failed (0x%08lx)\n", r);
    if (r == S_OK)
    {
        BOOL ret;

        strcpy(buffer,"garbage");
        ret = SHGetPathFromIDListA(tmp_pidl, buffer);
        ok(ret, "SHGetPathFromIDListA failed\n");
        if (ret)
            ok(lstrcmpiA(buffer,str)==0, "GetIDList returned '%s'\n", buffer);
        ILFree(tmp_pidl);
    }

    pidl=path_to_pidl(mypath);
    ok(pidl!=NULL, "path_to_pidl returned a NULL pidl\n");

    if (pidl)
    {
        LPITEMIDLIST second_pidl;

        r = IShellLinkA_SetIDList(sl, pidl);
        ok(r == S_OK, "SetIDList failed (0x%08lx)\n", r);

        tmp_pidl=NULL;
        r = IShellLinkA_GetIDList(sl, &tmp_pidl);
        ok(r == S_OK, "GetIDList failed (0x%08lx)\n", r);
        ok(tmp_pidl && ILIsEqual(pidl, tmp_pidl),
           "GetIDList returned an incorrect pidl\n");

        r = IShellLinkA_GetIDList(sl, &second_pidl);
        ok(r == S_OK, "GetIDList failed (0x%08lx)\n", r);
        ok(second_pidl && ILIsEqual(pidl, second_pidl),
           "GetIDList returned an incorrect pidl\n");
        ok(second_pidl != tmp_pidl, "pidls are the same\n");

        ILFree(second_pidl);
        ILFree(tmp_pidl);
        ILFree(pidl);

        strcpy(buffer,"garbage");
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
        ok(r == S_OK, "GetPath failed (0x%08lx)\n", r);
        ok(lstrcmpiA(buffer, mypath)==0, "GetPath returned '%s'\n", buffer);

        strcpy(buffer,"garbage");
        memset(&finddata, 0xaa, sizeof(finddata));
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), &finddata, SLGP_RAWPATH);
        ok(r == S_OK, "GetPath failed (0x%08lx)\n", r);
        ok(lstrcmpiA(buffer, mypath)==0, "GetPath returned '%s'\n", buffer);
        ok(finddata.dwFileAttributes != 0, "unexpected attributes %lx\n", finddata.dwFileAttributes);
        ok(lstrcmpiA(finddata.cFileName, "regedit.exe") == 0, "unexpected filename '%s'\n", finddata.cFileName);
    }

    if (pSHGetFolderLocation)
    {
        LPITEMIDLIST pidl_controls;

        r = pSHGetFolderLocation(NULL, CSIDL_CONTROLS, NULL, 0, &pidl_controls);
        ok(r == S_OK, "SHGetFolderLocation failed (0x%08lx)\n", r);

        r = IShellLinkA_SetIDList(sl, pidl_controls);
        ok(r == S_OK, "SetIDList failed (0x%08lx)\n", r);

        strcpy(buffer,"garbage");
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
        ok(r == S_FALSE, "GetPath failed (0x%08lx)\n", r);
        ok(buffer[0] == 0, "GetPath returned '%s'\n", buffer);

        strcpy(buffer,"garbage");
        memset(&finddata, 0xaa, sizeof(finddata));
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), &finddata, SLGP_RAWPATH);
        ok(r == S_FALSE, "GetPath failed (0x%08lx)\n", r);
        ok(buffer[0] == 0, "GetPath returned '%s'\n", buffer);
        ok(finddata.dwFileAttributes == 0, "unexpected attributes %lx\n", finddata.dwFileAttributes);
        ok(finddata.cFileName[0] == 0, "unexpected filename '%s'\n", finddata.cFileName);

        ILFree(pidl_controls);
    }

    /* test path with quotes (IShellLinkA_SetPath returns S_FALSE on W2K and below and S_OK on XP and above */
    r = IShellLinkA_SetPath(sl, "\"c:\\nonexistent\\file\"");
    ok(r==S_FALSE || r == S_OK, "SetPath failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    ok(r==S_OK, "GetPath failed (0x%08lx)\n", r);
    todo_wine ok(!strcmp(buffer, "C:\\nonexistent\\file"),
       "case doesn't match\n");

    r = IShellLinkA_SetPath(sl, "\"c:\\foo");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08lx)\n", r);

    r = IShellLinkA_SetPath(sl, "\"\"c:\\foo");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08lx)\n", r);

    r = IShellLinkA_SetPath(sl, "c:\\foo\"");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08lx)\n", r);

    r = IShellLinkA_SetPath(sl, "\"\"c:\\foo\"");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08lx)\n", r);

    r = IShellLinkA_SetPath(sl, "\"\"c:\\foo\"\"");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08lx)\n", r);

    /* Test Getting / Setting the arguments */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08lx)\n", r);
    ok(*buffer=='\0', "GetArguments returned '%s'\n", buffer);

    str="param1 \"spaced param2\"";
    r = IShellLinkA_SetArguments(sl, str);
    ok(r == S_OK, "SetArguments failed (0x%08lx)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08lx)\n", r);
    ok(strcmp(buffer,str)==0, "GetArguments returned '%s'\n", buffer);

    strcpy(buffer,"garbage");
    r = IShellLinkA_SetArguments(sl, NULL);
    ok(r == S_OK, "SetArguments failed (0x%08lx)\n", r);
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08lx)\n", r);
    ok(!buffer[0] || strcmp(buffer,str)==0, "GetArguments returned '%s'\n", buffer);

    strcpy(buffer,"garbage");
    r = IShellLinkA_SetArguments(sl, "");
    ok(r == S_OK, "SetArguments failed (0x%08lx)\n", r);
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08lx)\n", r);
    ok(!buffer[0], "GetArguments returned '%s'\n", buffer);

    /* Test Getting / Setting showcmd */
    i=0xdeadbeef;
    r = IShellLinkA_GetShowCmd(sl, &i);
    ok(r == S_OK, "GetShowCmd failed (0x%08lx)\n", r);
    ok(i==SW_SHOWNORMAL, "GetShowCmd returned %d\n", i);

    r = IShellLinkA_SetShowCmd(sl, SW_SHOWMAXIMIZED);
    ok(r == S_OK, "SetShowCmd failed (0x%08lx)\n", r);

    i=0xdeadbeef;
    r = IShellLinkA_GetShowCmd(sl, &i);
    ok(r == S_OK, "GetShowCmd failed (0x%08lx)\n", r);
    ok(i==SW_SHOWMAXIMIZED, "GetShowCmd returned %d'\n", i);

    /* Test Getting / Setting the icon */
    i=0xdeadbeef;
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(*buffer=='\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i==0, "GetIconLocation returned %d\n", i);

    str="c:\\nonexistent\\file";
    r = IShellLinkA_SetIconLocation(sl, str, 0xbabecafe);
    ok(r == S_OK, "SetIconLocation failed (0x%08lx)\n", r);

    i=0xdeadbeef;
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetIconLocation returned '%s'\n", buffer);
    ok(i==0xbabecafe, "GetIconLocation returned %d'\n", i);

    /* Test Getting / Setting the hot key */
    w=0xbeef;
    r = IShellLinkA_GetHotkey(sl, &w);
    ok(r == S_OK, "GetHotkey failed (0x%08lx)\n", r);
    ok(w==0, "GetHotkey returned %d\n", w);

    r = IShellLinkA_SetHotkey(sl, 0x5678);
    ok(r == S_OK, "SetHotkey failed (0x%08lx)\n", r);

    w=0xbeef;
    r = IShellLinkA_GetHotkey(sl, &w);
    ok(r == S_OK, "GetHotkey failed (0x%08lx)\n", r);
    ok(w==0x5678, "GetHotkey returned %d'\n", w);

    IShellLinkA_Release(sl);
}


/*
 * Test saving and loading .lnk files
 */

#define lok                   ok_(__FILE__, line)
#define check_lnk(a,b,c)        check_lnk_(__LINE__, (a), (b), (c))

void create_lnk_(int line, const WCHAR* path, lnk_desc_t* desc)
{
    HRESULT r, init_dirty;
    IShellLinkA *sl;
    IPersistFile *pf;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (LPVOID*)&sl);
    lok(r == S_OK, "no IID_IShellLinkA (0x%08lx)\n", r);
    if (r != S_OK)
        return;

    if (desc->description)
    {
        r = IShellLinkA_SetDescription(sl, desc->description);
        lok(r == S_OK, "SetDescription failed (0x%08lx)\n", r);
    }
    if (desc->workdir)
    {
        r = IShellLinkA_SetWorkingDirectory(sl, desc->workdir);
        lok(r == S_OK, "SetWorkingDirectory failed (0x%08lx)\n", r);
    }
    if (desc->path)
    {
        r = IShellLinkA_SetPath(sl, desc->path);
        lok(SUCCEEDED(r), "SetPath failed (0x%08lx)\n", r);
    }
    if (desc->pidl)
    {
        r = IShellLinkA_SetIDList(sl, desc->pidl);
        lok(r == S_OK, "SetIDList failed (0x%08lx)\n", r);
    }
    if (desc->arguments)
    {
        r = IShellLinkA_SetArguments(sl, desc->arguments);
        lok(r == S_OK, "SetArguments failed (0x%08lx)\n", r);
    }
    if (desc->showcmd)
    {
        r = IShellLinkA_SetShowCmd(sl, desc->showcmd);
        lok(r == S_OK, "SetShowCmd failed (0x%08lx)\n", r);
    }
    if (desc->icon)
    {
        r = IShellLinkA_SetIconLocation(sl, desc->icon, desc->icon_id);
        lok(r == S_OK, "SetIconLocation failed (0x%08lx)\n", r);
    }
    if (desc->hotkey)
    {
        r = IShellLinkA_SetHotkey(sl, desc->hotkey);
        lok(r == S_OK, "SetHotkey failed (0x%08lx)\n", r);
    }

    r = IShellLinkA_QueryInterface(sl, &IID_IPersistFile, (void**)&pf);
    lok(r == S_OK, "no IID_IPersistFile (0x%08lx)\n", r);
    if (r == S_OK)
    {
        LPOLESTR str;

    if (0)
    {
        /* crashes on XP */
        IPersistFile_GetCurFile(pf, NULL);
    }

        init_dirty = IPersistFile_IsDirty(pf); /* empty links start off as clean */
        r = IPersistFile_Save(pf, NULL, FALSE);
        lok(r == S_OK || r == E_INVALIDARG /* before Windows 7 */,
            "save(NULL, FALSE) failed (0x%08lx)\n", r);

        r = IPersistFile_IsDirty(pf);
        lok(r == init_dirty, "dirty(NULL, FALSE) (0x%08lx)\n", r);

        r = IPersistFile_Save(pf, NULL, TRUE);
        lok(r == S_OK || r == E_INVALIDARG /* before Windows 7 */,
            "save(NULL, TRUE) failed (0x%08lx)\n", r);

        r = IPersistFile_IsDirty(pf);
        lok(r == init_dirty, "dirty(NULL, TRUE) (0x%08lx)\n", r);

        /* test GetCurFile before ::Save */
        str = (LPWSTR)0xdeadbeef;
        r = IPersistFile_GetCurFile(pf, &str);
        lok(r == S_FALSE, "GetCurFile:before got 0x%08lx\n", r);
        lok(str == NULL, "GetCurFile:before got %p\n", str);

        r = IPersistFile_Save(pf, path, TRUE);
        lok(r == S_OK, "save(path, TRUE) failed (0x%08lx)\n", r);
        r = IPersistFile_IsDirty(pf);
        lok(r == S_FALSE, "dirty(path, TRUE) (0x%08lx)\n", r);

        /* test GetCurFile after ::Save */
        r = IPersistFile_GetCurFile(pf, &str);
        lok(r == S_OK, "GetCurFile(path, TRUE) got 0x%08lx\n", r);
        lok(str != NULL, "GetCurFile(path, TRUE) Didn't expect NULL\n");
        lok(!wcscmp(path, str), "GetCurFile(path, TRUE) Expected %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(str));
        CoTaskMemFree(str);

        r = IPersistFile_Save(pf, NULL, TRUE);
        lok(r == S_OK, "save(NULL, TRUE) failed (0x%08lx)\n", r);

        /* test GetCurFile after ::Save */
        r = IPersistFile_GetCurFile(pf, &str);
        lok(r == S_OK, "GetCurFile(NULL, TRUE) got 0x%08lx\n", r);
        lok(str != NULL, "GetCurFile(NULL, TRUE) Didn't expect NULL\n");
        lok(!wcscmp(path, str), "GetCurFile(NULL, TRUE) Expected %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(str));
        CoTaskMemFree(str);

        IPersistFile_Release(pf);
    }

    IShellLinkA_Release(sl);
}

static void check_lnk_(int line, const WCHAR* path, lnk_desc_t* desc, int todo)
{
    HRESULT r;
    IShellLinkA *sl;
    IPersistFile *pf;
    char buffer[INFOTIPSIZE];
    LPOLESTR str;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (LPVOID*)&sl);
    lok(r == S_OK, "no IID_IShellLinkA (0x%08lx)\n", r);
    if (r != S_OK)
        return;

    r = IShellLinkA_QueryInterface(sl, &IID_IPersistFile, (LPVOID*)&pf);
    lok(r == S_OK, "no IID_IPersistFile (0x%08lx)\n", r);
    if (r != S_OK)
    {
        IShellLinkA_Release(sl);
        return;
    }

    /* test GetCurFile before ::Load */
    str = (LPWSTR)0xdeadbeef;
    r = IPersistFile_GetCurFile(pf, &str);
    lok(r == S_FALSE, "got 0x%08lx\n", r);
    lok(str == NULL, "got %p\n", str);

    r = IPersistFile_Load(pf, path, STGM_READ);
    lok(r == S_OK, "load failed (0x%08lx)\n", r);

    /* test GetCurFile after ::Save */
    r = IPersistFile_GetCurFile(pf, &str);
    lok(r == S_OK, "got 0x%08lx\n", r);
    lok(str != NULL, "Didn't expect NULL\n");
    lok(!wcscmp(path, str), "Expected %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(str));
    CoTaskMemFree(str);

    IPersistFile_Release(pf);

    if (desc->description)
    {
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
        lok(r == S_OK, "GetDescription failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x1) != 0)
            lok(strcmp(buffer, desc->description)==0, "GetDescription returned '%s' instead of '%s'\n",
                buffer, desc->description);
    }
    if (desc->workdir)
    {
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetWorkingDirectory(sl, buffer, sizeof(buffer));
        lok(r == S_OK, "GetWorkingDirectory failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x2) != 0)
            lok(lstrcmpiA(buffer, desc->workdir)==0, "GetWorkingDirectory returned '%s' instead of '%s'\n",
                buffer, desc->workdir);
    }
    if (desc->path)
    {
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
        lok(SUCCEEDED(r), "GetPath failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x4) != 0)
            lok(lstrcmpiA(buffer, desc->path)==0, "GetPath returned '%s' instead of '%s'\n",
                buffer, desc->path);
    }
    if (desc->pidl)
    {
        LPITEMIDLIST pidl=NULL;
        r = IShellLinkA_GetIDList(sl, &pidl);
        lok(r == S_OK, "GetIDList failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x8) != 0)
            lok(ILIsEqual(pidl, desc->pidl), "GetIDList returned an incorrect pidl\n");
    }
    if (desc->showcmd)
    {
        int i=0xdeadbeef;
        r = IShellLinkA_GetShowCmd(sl, &i);
        lok(r == S_OK, "GetShowCmd failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x10) != 0)
            lok(i==desc->showcmd, "GetShowCmd returned 0x%0x instead of 0x%0x\n",
                i, desc->showcmd);
    }
    if (desc->icon)
    {
        int i=0xdeadbeef;
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
        lok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x20) != 0) {
            lok(lstrcmpiA(buffer, desc->icon)==0, "GetIconLocation returned '%s' instead of '%s'\n",
                buffer, desc->icon);
            lok(i==desc->icon_id, "GetIconLocation returned 0x%0x instead of 0x%0x\n",
                i, desc->icon_id);
        }
    }
    if (desc->hotkey)
    {
        WORD i=0xbeef;
        r = IShellLinkA_GetHotkey(sl, &i);
        lok(r == S_OK, "GetHotkey failed (0x%08lx)\n", r);
        todo_wine_if ((todo & 0x40) != 0)
            lok(i==desc->hotkey, "GetHotkey returned 0x%04x instead of 0x%04x\n",
                i, desc->hotkey);
    }

    IShellLinkA_Release(sl);
}

static void test_load_save(void)
{
    WCHAR lnkfile[MAX_PATH];
    char lnkfileA[MAX_PATH];
    static const char lnkfileA_name[] = "\\test.lnk";

    lnk_desc_t desc;
    char mypath[MAX_PATH];
    char mydir[MAX_PATH];
    char realpath[MAX_PATH];
    IPersistFile *pf;
    IShellLinkA *sl;
    IStream *stm;
    char* p;
    HANDLE hf;
    DWORD r;

    /* Don't used a fixed path for the test.lnk file */
    GetTempPathA(MAX_PATH, lnkfileA);
    lstrcatA(lnkfileA, lnkfileA_name);
    MultiByteToWideChar(CP_ACP, 0, lnkfileA, -1, lnkfile, MAX_PATH);

    /* Save an empty .lnk file */
    memset(&desc, 0, sizeof(desc));
    create_lnk(lnkfile, &desc);

    /* It should come back as a bunch of empty strings */
    desc.description="";
    desc.workdir="";
    desc.path="";
    desc.arguments="";
    desc.icon="";
    check_lnk(lnkfile, &desc, 0x0);

    /* Point a .lnk file to nonexistent files */
    desc.description="";
    desc.workdir="c:\\Nonexitent\\work\\directory";
    desc.path="c:\\nonexistent\\path";
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=0;
    desc.icon="c:\\nonexistent\\icon\\file";
    desc.icon_id=1234;
    desc.hotkey=0;
    create_lnk(lnkfile, &desc);
    check_lnk(lnkfile, &desc, 0x0);

    r=GetModuleFileNameA(NULL, mypath, sizeof(mypath));
    ok(r<sizeof(mypath), "GetModuleFileName failed (%ld)\n", r);
    strcpy(mydir, mypath);
    p=strrchr(mydir, '\\');
    if (p)
        *p='\0';

    /* IShellLink returns path in long form */
    if (!GetLongPathNameA(mypath, realpath, MAX_PATH))
        strcpy( realpath, mypath );

    /* Overwrite the existing lnk file and point it to existing files */
    desc.description="test 2";
    desc.workdir=mydir;
    desc.path=realpath;
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    check_lnk(lnkfile, &desc, 0x0);

    /* Test omitting .exe from an absolute path */
    p=strrchr(realpath, '.');
    if (p)
        *p='\0';

    desc.description="absolute path without .exe";
    desc.workdir=mydir;
    desc.path=realpath;
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    strcat(realpath, ".exe");
    check_lnk(lnkfile, &desc, 0x4);

    /* Overwrite the existing lnk file and test link to a command on the path */
    desc.description="command on path";
    desc.workdir=mypath;
    desc.path="rundll32.exe";
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    /* Check that link is created to proper location */
    SearchPathA( NULL, desc.path, NULL, MAX_PATH, realpath, NULL);
    desc.path=realpath;
    check_lnk(lnkfile, &desc, 0x0);

    /* Test omitting .exe from a command on the path */
    desc.description="command on path without .exe";
    desc.workdir=mypath;
    desc.path="rundll32";
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    /* Check that link is created to proper location */
    SearchPathA( NULL, "rundll32", NULL, MAX_PATH, realpath, NULL);
    desc.path=realpath;
    check_lnk(lnkfile, &desc, 0x4);

    /* Create a temporary non-executable file */
    r=GetTempPathA(sizeof(mypath), mypath);
    ok(r<sizeof(mypath), "GetTempPath failed (%ld), err %ld\n", r, GetLastError());
    r = GetLongPathNameA(mypath, mydir, sizeof(mydir));
    ok(r<sizeof(mydir), "GetLongPathName failed (%ld), err %ld\n", r, GetLastError());
    p=strrchr(mydir, '\\');
    if (p)
        *p='\0';

    strcpy(mypath, mydir);
    strcat(mypath, "\\test.txt");
    hf = CreateFileA(mypath, GENERIC_WRITE, 0, NULL,
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(hf);

    /* Overwrite the existing lnk file and test link to an existing non-executable file */
    desc.description="non-executable file";
    desc.workdir=mydir;
    desc.path=mypath;
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    check_lnk(lnkfile, &desc, 0x0);

    r = GetShortPathNameA(mydir, mypath, sizeof(mypath));
    ok(r<sizeof(mypath), "GetShortPathName failed (%ld), err %ld\n", r, GetLastError());

    strcpy(realpath, mypath);
    strcat(realpath, "\\test.txt");
    strcat(mypath, "\\\\test.txt");

    /* Overwrite the existing lnk file and test link to a short path with double backslashes */
    desc.description="non-executable file";
    desc.workdir=mydir;
    desc.path=mypath;
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    desc.path=realpath;
    check_lnk(lnkfile, &desc, 0x0);

    r = DeleteFileA(mypath);
    ok(r, "failed to delete file %s (%ld)\n", mypath, GetLastError());

    /* Create a temporary .bat file */
    strcpy(mypath, mydir);
    strcat(mypath, "\\test.bat");
    hf = CreateFileA(mypath, GENERIC_WRITE, 0, NULL,
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(hf);

    strcpy(realpath, mypath);

    p=strrchr(mypath, '.');
    if (p)
        *p='\0';

    /* Try linking to the .bat file without the extension */
    desc.description="batch file";
    desc.workdir=mydir;
    desc.path=mypath;
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc);
    desc.path = realpath;
    check_lnk(lnkfile, &desc, 0x4);

    r = DeleteFileA(realpath);
    ok(r, "failed to delete file %s (%ld)\n", realpath, GetLastError());

    /* test sharing modes */
    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkA, (LPVOID*)&sl);
    ok( r == S_OK, "no IID_IShellLinkA (0x%08lx)\n", r );
    r = IShellLinkA_QueryInterface(sl, &IID_IPersistFile, (void**)&pf);
    ok( r == S_OK, "no IID_IPersistFile (0x%08lx)\n", r );

    r = SHCreateStreamOnFileW(lnkfile, STGM_READ, &stm);
    ok( !r, "SHCreateStreamOnFileW failed %lx\n", r );
    r = IPersistFile_Save(pf, lnkfile, FALSE);
    ok(r == S_OK, "IPersistFile_Save failed (0x%08lx)\n", r);
    r = IPersistFile_Load(pf, lnkfile, 0);
    ok(r == S_OK, "IPersistFile_Load failed (0x%08lx)\n", r);
    IStream_Release( stm );

    r = SHCreateStreamOnFileW(lnkfile, STGM_READ | STGM_SHARE_DENY_WRITE, &stm);
    ok( r == S_OK, "SHCreateStreamOnFileW failed %lx\n", r );
    r = IPersistFile_Save(pf, lnkfile, FALSE);
    ok( r == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "IPersistFile_Save failed (0x%08lx)\n", r );
    r = IPersistFile_Load(pf, lnkfile, 0);
    ok(r == S_OK, "IPersistFile_Load failed (0x%08lx)\n", r);
    IStream_Release( stm );

    r = SHCreateStreamOnFileW(lnkfile, STGM_READWRITE | STGM_SHARE_DENY_WRITE, &stm);
    ok( r == S_OK, "SHCreateStreamOnFileW failed %lx\n", r );
    r = IPersistFile_Save(pf, lnkfile, FALSE);
    ok( r == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "IPersistFile_Save failed (0x%08lx)\n", r );
    r = IPersistFile_Load(pf, lnkfile, 0);
    ok(r == S_OK, "IPersistFile_Load failed (0x%08lx)\n", r);
    IStream_Release( stm );

    r = SHCreateStreamOnFileW(lnkfile, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &stm);
    ok( r == S_OK, "SHCreateStreamOnFileW failed %lx\n", r );
    r = IPersistFile_Save(pf, lnkfile, FALSE);
    ok( r == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "IPersistFile_Save failed (0x%08lx)\n", r );
    r = IPersistFile_Load(pf, lnkfile, 0);
    ok( r == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "IPersistFile_Load failed (0x%08lx)\n", r );
    IStream_Release( stm );

    IShellLinkA_Release( sl );
    IPersistFile_Release( pf );

    /* FIXME: Also test saving a .lnk pointing to a pidl that cannot be
     * represented as a path.
     */

    DeleteFileW(lnkfile);
}

static void test_datalink(void)
{
    static const WCHAR lnk[] =
      L"::{9db1186e-40df-11d1-aa8c-00c04fb67863}:26,!!gxsf(Ng]qF`H{LsACCESSFiles>plT]jI{jf(=1&L[-81-]::";
    static const WCHAR comp[] = L"26,!!gxsf(Ng]qF`H{LsACCESSFiles>plT]jI{jf(=1&L[-81-]";
    IShellLinkDataList *dl = NULL;
    IShellLinkW *sl = NULL;
    HRESULT r;
    DWORD flags = 0;
    EXP_DARWIN_LINK *dar;

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IShellLinkW, (LPVOID*)&sl );
    ok( r == S_OK, "Failed to create shelllink object, hr %#lx.\n", r);

    r = IShellLinkW_QueryInterface( sl, &IID_IShellLinkDataList, (void **)&dl );
    ok( r == S_OK, "Failed to get interface, hr %#lx.\n", r);

    flags = 0;
    r = IShellLinkDataList_GetFlags( dl, &flags );
    ok( r == S_OK, "GetFlags failed\n");
    ok( flags == 0, "GetFlags returned wrong flags\n");

    dar = (void*)-1;
    r = IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG, (LPVOID*) &dar );
    ok( r == E_FAIL, "CopyDataBlock failed\n");
    ok( dar == NULL, "should be null\n");

    r = IShellLinkW_SetPath(sl, NULL);
    ok(r == E_INVALIDARG, "Unexpected hr %#lx.\n", r);

    r = IShellLinkW_SetPath(sl, lnk);
    ok(r == S_OK, "SetPath failed\n");

if (0)
{
    /* the following crashes */
    IShellLinkDataList_GetFlags( dl, NULL );
}

    flags = 0;
    r = IShellLinkDataList_GetFlags( dl, &flags );
    ok( r == S_OK, "GetFlags failed\n");
    /* SLDF_HAS_LOGO3ID is no longer supported on Vista+, filter it out */
    ok( (flags & (~ SLDF_HAS_LOGO3ID)) == SLDF_HAS_DARWINID,
        "GetFlags returned wrong flags\n");

    dar = NULL;
    r = IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG, (LPVOID*) &dar );
    ok( r == S_OK, "CopyDataBlock failed\n");

    ok( dar && ((DATABLOCK_HEADER*)dar)->dwSignature == EXP_DARWIN_ID_SIG, "signature wrong\n");
    ok( dar && 0==lstrcmpW(dar->szwDarwinID, comp ), "signature wrong\n");

    LocalFree( dar );

    IShellLinkDataList_Release( dl );
    IShellLinkW_Release( sl );
}

static void test_shdefextracticon(void)
{
    HICON hiconlarge=NULL, hiconsmall=NULL;
    HRESULT res;

    res = SHDefExtractIconA("shell32.dll", 0, 0, &hiconlarge, &hiconsmall, MAKELONG(16,24));
    ok(SUCCEEDED(res), "SHDefExtractIconA failed, res=%lx\n", res);
    ok(hiconlarge != NULL, "got null hiconlarge\n");
    ok(hiconsmall != NULL, "got null hiconsmall\n");
    DestroyIcon(hiconlarge);
    DestroyIcon(hiconsmall);

    hiconsmall = NULL;
    res = SHDefExtractIconA("shell32.dll", 0, 0, NULL, &hiconsmall, MAKELONG(16,24));
    ok(SUCCEEDED(res), "SHDefExtractIconA failed, res=%lx\n", res);
    ok(hiconsmall != NULL, "got null hiconsmall\n");
    DestroyIcon(hiconsmall);

    res = SHDefExtractIconA("shell32.dll", 0, 0, NULL, NULL, MAKELONG(16,24));
    ok(SUCCEEDED(res), "SHDefExtractIconA failed, res=%lx\n", res);
}

static void test_GetIconLocation(void)
{
    IShellLinkW *slW;
    IShellLinkA *sl;
    const char *str;
    char buffer[INFOTIPSIZE], mypath[MAX_PATH];
    int i;
    HRESULT r;
    LPITEMIDLIST pidl;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IShellLinkA, (LPVOID*)&sl);
    ok(r == S_OK, "no IID_IShellLinkA (0x%08lx)\n", r);
    if(r != S_OK)
        return;

    i = 0xdeadbeef;
    strcpy(buffer, "garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(*buffer == '\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0, "GetIconLocation returned %d\n", i);

    str = "c:\\some\\path";
    r = IShellLinkA_SetPath(sl, str);
    ok(r == S_FALSE || r == S_OK, "SetPath failed (0x%08lx)\n", r);

    i = 0xdeadbeef;
    strcpy(buffer, "garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(*buffer == '\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0, "GetIconLocation returned %d\n", i);

    GetWindowsDirectoryA(mypath, sizeof(mypath) - 12);
    strcat(mypath, "\\regedit.exe");
    pidl = path_to_pidl(mypath);
    r = IShellLinkA_SetIDList(sl, pidl);
    ok(r == S_OK, "SetPath failed (0x%08lx)\n", r);
    ILFree(pidl);

    i = 0xdeadbeef;
    strcpy(buffer, "garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(*buffer == '\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0, "GetIconLocation returned %d\n", i);

    str = "c:\\nonexistent\\file";
    r = IShellLinkA_SetIconLocation(sl, str, 0xbabecafe);
    ok(r == S_OK, "SetIconLocation failed (0x%08lx)\n", r);

    i = 0xdeadbeef;
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(lstrcmpiA(buffer,str) == 0, "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0xbabecafe, "GetIconLocation returned %#x.\n", i);

    r = IShellLinkA_SetIconLocation(sl, NULL, 0xcafefe);
    ok(r == S_OK, "SetIconLocation failed (0x%08lx)\n", r);

    i = 0xdeadbeef;
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(!*buffer, "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0xcafefe, "GetIconLocation returned %#x.\n", i);

    r = IShellLinkA_QueryInterface(sl, &IID_IShellLinkW, (void **)&slW);
    ok(SUCCEEDED(r), "Failed to get IShellLinkW, hr %#lx.\n", r);

    str = "c:\\nonexistent\\file";
    r = IShellLinkA_SetIconLocation(sl, str, 0xbabecafe);
    ok(r == S_OK, "SetIconLocation failed (0x%08lx)\n", r);

    r = IShellLinkA_SetIconLocation(sl, NULL, 0xcafefe);
    ok(r == S_OK, "SetIconLocation failed (0x%08lx)\n", r);

    i = 0xdeadbeef;
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08lx)\n", r);
    ok(!*buffer, "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0xcafefe, "GetIconLocation returned %#x.\n", i);

    IShellLinkW_Release(slW);
    IShellLinkA_Release(sl);
}

static void test_SHGetStockIconInfo(void)
{
    BYTE buffer[sizeof(SHSTOCKICONINFO) + 16];
    SHSTOCKICONINFO *sii = (SHSTOCKICONINFO *) buffer;
    HRESULT hr;
    INT i;

    /* not present before vista */
    if (!pSHGetStockIconInfo)
    {
        win_skip("SHGetStockIconInfo not available\n");
        return;
    }

    /* negative values are handled */
    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO);
    hr = pSHGetStockIconInfo(SIID_INVALID, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "-1: got 0x%lx (expected E_INVALIDARG)\n", hr);

    /* max. id for vista is 140 (no definition exists for this value) */
    for (i = SIID_DOCNOASSOC; i <= SIID_CLUSTEREDDRIVE; i++)
    {
        memset(buffer, '#', sizeof(buffer));
        sii->cbSize = sizeof(SHSTOCKICONINFO);
        hr = pSHGetStockIconInfo(i, SHGSI_ICONLOCATION, sii);

        ok(hr == S_OK,
            "%3d: got 0x%lx, iSysImageIndex: 0x%x, iIcon: 0x%x (expected S_OK)\n",
            i, hr, sii->iSysImageIndex, sii->iIcon);

        if ((hr == S_OK) && (winetest_debug > 1))
            trace("%3d: got iSysImageIndex %3d, iIcon %3d and %s\n", i, sii->iSysImageIndex,
                  sii->iIcon, wine_dbgstr_w(sii->szPath));
    }

    /* test invalid icons indices that are invalid for all platforms */
    for (i = SIID_MAX_ICONS; i < (SIID_MAX_ICONS + 25) ; i++)
    {
        memset(buffer, '#', sizeof(buffer));
        sii->cbSize = sizeof(SHSTOCKICONINFO);
        hr = pSHGetStockIconInfo(i, SHGSI_ICONLOCATION, sii);
        ok(hr == E_INVALIDARG, "%3d: got 0x%lx (expected E_INVALIDARG)\n", i, hr);
    todo_wine {
        ok(sii->iSysImageIndex == -1, "%d: got iSysImageIndex %d\n", i, sii->iSysImageIndex);
        ok(sii->iIcon == -1, "%d: got iIcon %d\n", i, sii->iIcon);
    }
    }

    /* test more returned SHSTOCKICONINFO elements without extra flags */
    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO);
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
    ok(!sii->hIcon, "got %p (expected NULL)\n", sii->hIcon);
    ok(sii->iSysImageIndex == -1, "got %d (expected -1)\n", sii->iSysImageIndex);

    /* the exact size is required of the struct */
    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) + 2;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "+2: got 0x%lx, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) + 1;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "+1: got 0x%lx, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) - 1;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "-1: got 0x%lx, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) - 2;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "-2: got 0x%lx, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    /* there is a NULL check for the struct  */
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, NULL);
    ok(hr == E_INVALIDARG, "NULL: got 0x%lx\n", hr);
}

static void test_SHExtractIcons(void)
{
    UINT ret, ret2;
    HICON icons[256];
    UINT ids[256], i;

    if (!pSHExtractIconsW)
    {
        win_skip("SHExtractIconsW not available\n");
        return;
    }

    ret = pSHExtractIconsW(L"", 0, 16, 16, icons, ids, 1, 0);
    ok(ret == ~0u, "got %u\n", ret);

    ret = pSHExtractIconsW(L"notepad.exe", 0, 16, 16, NULL, NULL, 1, 0);
    ok(ret == 1 || ret == 4 /* win11 */, "got %u\n", ret);

    icons[0] = (HICON)0xdeadbeef;
    ret = pSHExtractIconsW(L"notepad.exe", 0, 16, 16, icons, NULL, 1, 0);
    ok(ret == 1, "got %u\n", ret);
    ok(icons[0] != (HICON)0xdeadbeef, "icon not set\n");
    DestroyIcon(icons[0]);

    icons[0] = (HICON)0xdeadbeef;
    ids[0] = 0xdeadbeef;
    ret = pSHExtractIconsW(L"notepad.exe", 0, 16, 16, icons, ids, 1, 0);
    ok(ret == 1, "got %u\n", ret);
    ok(icons[0] != (HICON)0xdeadbeef, "icon not set\n");
    ok(ids[0] != 0xdeadbeef, "id not set\n");
    DestroyIcon(icons[0]);

    ret = pSHExtractIconsW(L"shell32.dll", 0, 16, 16, NULL, NULL, 0, 0);
    ret2 = pSHExtractIconsW(L"shell32.dll", 4, MAKELONG(32,16), MAKELONG(32,16), NULL, NULL, 256, 0);
    ok(ret && ret == ret2,
       "icon count should be independent of requested icon sizes and base icon index\n");

    ret = pSHExtractIconsW(L"shell32.dll", 0, 16, 16, icons, ids, 0, 0);
    ok(ret == ~0u || !ret /* < vista */, "got %u\n", ret);

    ret = pSHExtractIconsW(L"shell32.dll", 0, 16, 16, icons, ids, 3, 0);
    ok(ret == 3, "got %u\n", ret);
    for (i = 0; i < ret; i++) DestroyIcon(icons[i]);

    /* count must be a multiple of two when getting two sizes */
    ret = pSHExtractIconsW(L"shell32.dll", 0, MAKELONG(16,32), MAKELONG(16,32), icons, ids, 3, 0);
    ok(!ret /* vista */ || ret == 4, "got %u\n", ret);
    for (i = 0; i < ret; i++) DestroyIcon(icons[i]);

    ret = pSHExtractIconsW(L"shell32.dll", 0, MAKELONG(16,32), MAKELONG(16,32), icons, ids, 4, 0);
    ok(ret == 4, "got %u\n", ret);
    for (i = 0; i < ret; i++) DestroyIcon(icons[i]);
}

static void test_propertystore(void)
{
    IShellLinkA *linkA;
    IShellLinkW *linkW;
    IPropertyStore *ps;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (void**)&linkA);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellLinkA_QueryInterface(linkA, &IID_IShellLinkW, (void**)&linkW);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IShellLinkA_QueryInterface(linkA, &IID_IPropertyStore, (void**)&ps);
    if (hr == S_OK) {
        IPropertyStoreCache *pscache;

        IPropertyStore_Release(ps);

        hr = IShellLinkW_QueryInterface(linkW, &IID_IPropertyStore, (void**)&ps);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IPropertyStore_QueryInterface(ps, &IID_IPropertyStoreCache, (void**)&pscache);
        ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

        IPropertyStore_Release(ps);
    }
    else
        win_skip("IShellLink doesn't support IPropertyStore.\n");

    IShellLinkA_Release(linkA);
    IShellLinkW_Release(linkW);
}

static void test_ExtractIcon(void)
{
    WCHAR pathW[MAX_PATH];
    HICON hicon, hicon2;
    char path[MAX_PATH];
    HANDLE file;
    int r;
    ICONINFO info;
    BITMAP bm;

    /* specified instance handle */
    hicon = ExtractIconA(GetModuleHandleA("shell32.dll"), NULL, 0);
    ok(hicon == NULL, "Got icon %p\n", hicon);
    hicon2 = ExtractIconA(GetModuleHandleA("shell32.dll"), "shell32.dll", -1);
    ok(hicon2 != NULL, "Got icon %p\n", hicon2);

    /* existing index */
    hicon = ExtractIconA(NULL, "shell32.dll", 0);
    ok(hicon != NULL && HandleToLong(hicon) != -1, "Got icon %p\n", hicon);
    DestroyIcon(hicon);

    /* returns number of resources */
    hicon = ExtractIconA(NULL, "shell32.dll", -1);
    ok(HandleToLong(hicon) > 1 && hicon == hicon2, "Got icon %p\n", hicon);

    /* invalid index, valid dll name */
    hicon = ExtractIconA(NULL, "shell32.dll", 3000);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    /* Create a temporary non-executable file */
    GetTempPathA(sizeof(path), path);
    strcat(path, "\\extracticon_test.txt");
    file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create a test file\n");
    CloseHandle(file);

    hicon = ExtractIconA(NULL, path, 0);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    hicon = ExtractIconA(NULL, path, -1);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    hicon = ExtractIconA(NULL, path, 1);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    r = DeleteFileA(path);
    ok(r, "failed to delete file %s (%ld)\n", path, GetLastError());

    /* Empty file path */
    hicon = ExtractIconA(NULL, "", -1);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    hicon = ExtractIconA(NULL, "", 0);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    /* same for W variant */
if (0)
{
    /* specified instance handle, crashes on XP, 2k3 */
    hicon = ExtractIconW(GetModuleHandleA("shell32.dll"), NULL, 0);
    ok(hicon == NULL, "Got icon %p\n", hicon);
}
    hicon2 = ExtractIconW(GetModuleHandleA("shell32.dll"), L"shell32.dll", -1);
    ok(hicon2 != NULL, "Got icon %p\n", hicon2);

    /* existing index */
    hicon = ExtractIconW(NULL, L"shell32.dll", 0);
    ok(hicon != NULL && HandleToLong(hicon) != -1, "Got icon %p\n", hicon);
    GetIconInfo(hicon, &info);
    GetObjectW(info.hbmColor, sizeof(bm), &bm);
    ok(bm.bmWidth == GetSystemMetrics(SM_CXICON), "got %d\n", bm.bmWidth);
    ok(bm.bmHeight == GetSystemMetrics(SM_CYICON), "got %d\n", bm.bmHeight);
    DestroyIcon(hicon);

    /* returns number of resources */
    hicon = ExtractIconW(NULL, L"shell32.dll", -1);
    ok(HandleToLong(hicon) > 1 && hicon == hicon2, "Got icon %p\n", hicon);

    /* invalid index, valid dll name */
    hicon = ExtractIconW(NULL, L"shell32.dll", 3000);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    /* Create a temporary non-executable file */
    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, L"\\extracticon_test.txt");
    file = CreateFileW(pathW, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create a test file\n");
    CloseHandle(file);

    hicon = ExtractIconW(NULL, pathW, 0);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    hicon = ExtractIconW(NULL, pathW, -1);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    hicon = ExtractIconW(NULL, pathW, 1);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    r = DeleteFileW(pathW);
    ok(r, "failed to delete file %s (%ld)\n", path, GetLastError());

    /* Empty file path */
    hicon = ExtractIconW(NULL, L"", -1);
    ok(hicon == NULL, "Got icon %p\n", hicon);

    hicon = ExtractIconW(NULL, L"", 0);
    ok(hicon == NULL, "Got icon %p\n", hicon);
}

static void test_ExtractAssociatedIcon(void)
{
    char pathA[MAX_PATH];
    HICON hicon;
    WORD index;

    /* empty path */
    index = 0;
    *pathA = 0;
    hicon = ExtractAssociatedIconA(NULL, pathA, &index);
todo_wine {
    ok(hicon != NULL, "Got icon %p\n", hicon);
    ok(!*pathA, "Unexpected path %s\n", pathA);
    ok(index == 0, "Unexpected index %u\n", index);
}
    DestroyIcon(hicon);

    /* by index */
    index = 0;
    strcpy(pathA, "shell32.dll");
    hicon = ExtractAssociatedIconA(NULL, pathA, &index);
    ok(hicon != NULL, "Got icon %p\n", hicon);
    ok(!strcmp(pathA, "shell32.dll"), "Unexpected path %s\n", pathA);
    ok(index == 0, "Unexpected index %u\n", index);
    DestroyIcon(hicon);

    /* valid dll name, invalid index */
    index = 5000;
    strcpy(pathA, "user32.dll");
    hicon = ExtractAssociatedIconA(NULL, pathA, &index);
    CharLowerBuffA(pathA, strlen(pathA));
todo_wine {
    ok(hicon != NULL, "Got icon %p\n", hicon);
    ok(!!strstr(pathA, "shell32.dll"), "Unexpected path %s\n", pathA);
}
    ok(index != 5000, "Unexpected index %u\n", index);
    DestroyIcon(hicon);

    /* associated icon */
    index = 0xcaca;
    strcpy(pathA, "dummy.exe");
    hicon = ExtractAssociatedIconA(NULL, pathA, &index);
    CharLowerBuffA(pathA, strlen(pathA));
todo_wine {
    ok(hicon != NULL, "Got icon %p\n", hicon);
    ok(!!strstr(pathA, "shell32.dll"), "Unexpected path %s\n", pathA);
}
    ok(index != 0xcaca, "Unexpected index %u\n", index);
    DestroyIcon(hicon);
}

static int get_shell_icon_size(void)
{
    char buf[10];
    DWORD value = 32, size = sizeof(buf), type;
    HKEY key;

    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Control Panel\\Desktop\\WindowMetrics", &key ))
    {
        if (!RegQueryValueExA( key, "Shell Icon Size", NULL, &type, (BYTE *)buf, &size ) && type == REG_SZ)
            value = atoi( buf );
        RegCloseKey( key );
    }
    return value;
}

static void test_SHGetImageList(void)
{
    HRESULT hr;
    IImageList *list, *list2;
    BOOL ret;
    HIMAGELIST lg, sm;
    ULONG start_refs, refs;
    int i, width, height, expect;
    BOOL dpi_aware = pIsProcessDPIAware && pIsProcessDPIAware();

    hr = SHGetImageList( SHIL_LARGE, &IID_IImageList, (void **)&list );
    ok( hr == S_OK, "got %08lx\n", hr );
    start_refs = IImageList_AddRef( list );
    IImageList_Release( list );

    hr = SHGetImageList( SHIL_LARGE, &IID_IImageList, (void **)&list2 );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( list == list2, "lists differ\n" );
    refs = IImageList_AddRef( list );
    IImageList_Release( list );
    ok( refs == start_refs + 1, "got %ld, start_refs %ld\n", refs, start_refs );
    IImageList_Release( list2 );

    hr = SHGetImageList( SHIL_SMALL, &IID_IImageList, (void **)&list2 );
    ok( hr == S_OK, "got %08lx\n", hr );

    ret = Shell_GetImageLists( &lg, &sm );
    ok( ret, "got %d\n", ret );
    ok( lg == (HIMAGELIST)list, "mismatch\n" );
    ok( sm == (HIMAGELIST)list2, "mismatch\n" );

    /* Shell_GetImageLists doesn't take a reference */
    refs = IImageList_AddRef( list );
    IImageList_Release( list );
    ok( refs == start_refs, "got %ld, start_refs %ld\n", refs, start_refs );

    IImageList_Release( list2 );
    IImageList_Release( list );

    /* Test the icon sizes */
    for (i = 0; i <= SHIL_LAST; i++)
    {
        hr = SHGetImageList( i, &IID_IImageList, (void **)&list );
        ok( hr == S_OK || broken( i == SHIL_JUMBO && hr == E_INVALIDARG ), /* XP and 2003 */
                "%d: got %08lx\n", i, hr );
        if (FAILED(hr)) continue;
        IImageList_GetIconSize( list, &width, &height );
        switch (i)
        {
        case SHIL_LARGE:
            if (dpi_aware) expect = GetSystemMetrics( SM_CXICON );
            else expect = get_shell_icon_size();
            break;
        case SHIL_SMALL:
            if (dpi_aware) expect = GetSystemMetrics( SM_CXICON ) / 2;
            else expect = GetSystemMetrics( SM_CXSMICON );
            break;
        case SHIL_EXTRALARGE:
            expect = (GetSystemMetrics( SM_CXICON ) * 3) / 2;
            break;
        case SHIL_SYSSMALL:
            expect = GetSystemMetrics( SM_CXSMICON );
            break;
        case SHIL_JUMBO:
            expect = 256;
            break;
        }
        todo_wine_if(i == SHIL_SYSSMALL && dpi_aware && expect != GetSystemMetrics( SM_CXICON ) / 2)
        {
            ok( width == expect, "%d: got %d expect %d\n", i, width, expect );
            ok( height == expect, "%d: got %d expect %d\n", i, height, expect );
        }
        IImageList_Release( list );
    }
}

START_TEST(shelllink)
{
    HRESULT r;
    HMODULE hmod = GetModuleHandleA("shell32.dll");
    HMODULE huser32 = GetModuleHandleA("user32.dll");

    pSHILCreateFromPath = (void *)GetProcAddress(hmod, (LPSTR)28);
    pSHGetFolderLocation = (void *)GetProcAddress(hmod, "SHGetFolderLocation");
    pSHGetStockIconInfo = (void *)GetProcAddress(hmod, "SHGetStockIconInfo");
    pSHExtractIconsW = (void *)GetProcAddress(hmod, "SHExtractIconsW");
    pIsProcessDPIAware = (void *)GetProcAddress(huser32, "IsProcessDPIAware");

    r = CoInitialize(NULL);
    ok(r == S_OK, "CoInitialize failed (0x%08lx)\n", r);
    if (r != S_OK)
        return;

    test_get_set();
    test_load_save();
    test_datalink();
    test_shdefextracticon();
    test_GetIconLocation();
    test_SHGetStockIconInfo();
    test_SHExtractIcons();
    test_propertystore();
    test_ExtractIcon();
    test_ExtractAssociatedIcon();
    test_SHGetImageList();

    CoUninitialize();
}
