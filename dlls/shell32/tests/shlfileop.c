/*
 * Unit test of the SHFileOperation function.
 *
 * Copyright 2002 Andriy Palamarchuk
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
#include <windows.h>
#include "shellapi.h"
#include "shlwapi.h"
#include "shlobj.h"
#include "sherrors.h"
#include "commoncontrols.h"

#include "wine/test.h"

/* Error codes could be pre-Win32 */
#define DE_SAMEFILE      0x71
#define DE_MANYSRC1DEST  0x72
#define DE_DIFFDIR       0x73
#define DE_OPCANCELLED   0x75
#define DE_DESTSUBTREE   0x76
#define DE_INVALIDFILES  0x7C
#define DE_DESTSAMETREE  0x7D
#define DE_FLDDESTISFILE 0x7E
#define DE_FILEDESTISFLD 0x80
#define expect_retval(ret, ret_prewin32)\
    ok(retval == ret ||\
       broken(retval == ret_prewin32),\
       "Expected %d, got %ld\n", ret, retval)

static BOOL old_shell32 = FALSE;

static CHAR CURR_DIR[MAX_PATH];
static const WCHAR UNICODE_PATH[] = {'c',':','\\',0x00ae,'\0','\0'};
    /* "c:\®" can be used in all codepages */
    /* Double-null termination needed for pFrom field of SHFILEOPSTRUCT */

/* creates a file with the specified name for tests */
static void createTestFile(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

static void createTestFileW(const WCHAR *name)
{
    HANDLE file;

    file = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file\n");
    CloseHandle(file);
}

static BOOL file_exists(const CHAR *name)
{
    return GetFileAttributesA(name) != INVALID_FILE_ATTRIBUTES;
}

static BOOL dir_exists(const CHAR *name)
{
    DWORD attr;
    BOOL dir;

    attr = GetFileAttributesA(name);
    dir = ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

    return ((attr != INVALID_FILE_ATTRIBUTES) && dir);
}

static BOOL file_existsW(LPCWSTR name)
{
  return GetFileAttributesW(name) != INVALID_FILE_ATTRIBUTES;
}

static BOOL file_has_content(const CHAR *name, const CHAR *content)
{
    CHAR buf[MAX_PATH];
    HANDLE file;
    DWORD read;

    file = CreateFileA(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    ReadFile(file, buf, MAX_PATH - 1, &read, NULL);
    buf[read] = 0;
    CloseHandle(file);
    return strcmp(buf, content)==0;
}

static void remove_directory(const WCHAR *name)
{
    SHFILEOPSTRUCTW shfo;
    WCHAR path[MAX_PATH];

    memset(&shfo, 0, sizeof(shfo));
    shfo.wFunc = FO_DELETE;
    wcscpy(path, name);
    path[wcslen(name) + 1] = 0;
    shfo.pFrom = path;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
    SHFileOperationW(&shfo);
}

/* initializes the tests */
static void init_shfo_tests(void)
{
    int len;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len-1] == '\\'))
        CURR_DIR[len-1] = 0;

    createTestFile("test1.txt");
    createTestFile("test2.txt");
    createTestFile("test3.txt");
    createTestFile("test_5.txt");
    CreateDirectoryA("test4.txt", NULL);
    CreateDirectoryA("testdir2", NULL);
    CreateDirectoryA("testdir2\\nested", NULL);
    createTestFile("testdir2\\one.txt");
    createTestFile("testdir2\\nested\\two.txt");
    CreateDirectoryA("testdir4", NULL);
    CreateDirectoryA("testdir4\\nested", NULL);
    CreateDirectoryA("testdir4\\nested\\subnested", NULL);
    createTestFile("testdir4\\nested\\2.txt");
    createTestFile("testdir4\\nested\\subnested\\3.txt");
    CreateDirectoryA("testdir6", NULL);
    CreateDirectoryA("testdir6\\nested", NULL);
    CreateDirectoryA("testdir8", NULL);
    CreateDirectoryA("testdir8\\nested", NULL);
}

/* cleans after tests */
static void clean_after_shfo_tests(void)
{
    DeleteFileA("test1.txt");
    DeleteFileA("test2.txt");
    DeleteFileA("test3.txt");
    DeleteFileA("test_5.txt");
    DeleteFileA("one.txt");
    DeleteFileA("test4.txt\\test1.txt");
    DeleteFileA("test4.txt\\test2.txt");
    DeleteFileA("test4.txt\\test3.txt");
    DeleteFileA("test4.txt\\one.txt");
    DeleteFileA("test4.txt\\nested\\two.txt");
    RemoveDirectoryA("test4.txt\\nested");
    RemoveDirectoryA("test4.txt");
    DeleteFileA("testdir2\\one.txt");
    DeleteFileA("testdir2\\test1.txt");
    DeleteFileA("testdir2\\test2.txt");
    DeleteFileA("testdir2\\test3.txt");
    DeleteFileA("testdir2\\test4.txt\\test1.txt");
    DeleteFileA("testdir2\\nested\\two.txt");
    RemoveDirectoryA("testdir2\\test4.txt");
    RemoveDirectoryA("testdir2\\nested");
    RemoveDirectoryA("testdir2");
    DeleteFileA("testdir4\\nested\\subnested\\3.txt");
    DeleteFileA("testdir4\\nested\\two.txt");
    DeleteFileA("testdir4\\nested\\2.txt");
    RemoveDirectoryA("testdir4\\nested\\subnested");
    RemoveDirectoryA("testdir4\\nested");
    RemoveDirectoryA("testdir4");
    DeleteFileA("testdir6\\nested\\subnested\\3.txt");
    DeleteFileA("testdir6\\nested\\two.txt");
    DeleteFileA("testdir6\\nested\\2.txt");
    DeleteFileA("testdir6\\one.txt");
    DeleteFileA("testdir6\\two.txt");
    RemoveDirectoryA("testdir6\\nested\\subnested");
    RemoveDirectoryA("testdir6\\subnested");
    RemoveDirectoryA("testdir6\\nested");
    RemoveDirectoryA("testdir6");
    DeleteFileA("testdir8\\nested\\subnested\\3.txt");
    DeleteFileA("testdir8\\subnested\\3.txt");
    DeleteFileA("testdir8\\nested\\2.txt");
    DeleteFileA("testdir8\\2.txt");
    RemoveDirectoryA("testdir8\\nested\\subnested");
    RemoveDirectoryA("testdir8\\subnested");
    RemoveDirectoryA("testdir8\\nested");
    RemoveDirectoryA("testdir8");
    RemoveDirectoryA("c:\\testdir3");
    DeleteFileA("nonexistent\\notreal\\test2.txt");
    RemoveDirectoryA("nonexistent\\notreal");
    RemoveDirectoryA("nonexistent");
}


static void test_get_file_info(void)
{
    DWORD rc, rc2;
    SHFILEINFOA shfi, shfi2;
    SHFILEINFOW shfiw;
    char notepad[MAX_PATH];
    HANDLE unset_icon;

    /* Test whether fields of SHFILEINFOA are always cleared */
    memset(&shfi, 0xcf, sizeof(shfi));
    rc=SHGetFileInfoA("", 0, &shfi, sizeof(shfi), 0);
    ok(rc == 1, "SHGetFileInfoA('' | 0) should return 1, got 0x%lx\n", rc);
    todo_wine ok(shfi.hIcon == 0, "SHGetFileInfoA('' | 0) did not clear hIcon\n");
    todo_wine ok(shfi.szDisplayName[0] == 0, "SHGetFileInfoA('' | 0) did not clear szDisplayName[0]\n");
    todo_wine ok(shfi.szTypeName[0] == 0, "SHGetFileInfoA('' | 0) did not clear szTypeName[0]\n");
    ok(shfi.iIcon == 0xcfcfcfcf ||
       broken(shfi.iIcon != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA('' | 0) should not clear iIcon\n");
    ok(shfi.dwAttributes == 0xcfcfcfcf ||
       broken(shfi.dwAttributes != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA('' | 0) should not clear dwAttributes\n");

    memset(&shfiw, 0xcf, sizeof(shfiw));
    memset(&unset_icon, 0xcf, sizeof(unset_icon));
    rc = SHGetFileInfoW(NULL, 0, &shfiw, sizeof(shfiw), 0);
    ok(!rc, "SHGetFileInfoW(NULL | 0) should fail\n");
    ok(shfiw.hIcon == unset_icon, "SHGetFileInfoW(NULL | 0) should not clear hIcon\n");
    ok(shfiw.szDisplayName[0] == 0xcfcf, "SHGetFileInfoW(NULL | 0) should not clear szDisplayName[0]\n");
    ok(shfiw.szTypeName[0] == 0xcfcf, "SHGetFileInfoW(NULL | 0) should not clear szTypeName[0]\n");
    ok(shfiw.iIcon == 0xcfcfcfcf, "SHGetFileInfoW(NULL | 0) should not clear iIcon\n");
    ok(shfiw.dwAttributes == 0xcfcfcfcf, "SHGetFileInfoW(NULL | 0) should not clear dwAttributes\n");

    /* Test some flag combinations that MSDN claims are not allowed,
     * but which work anyway
     */
    memset(&shfi, 0xcf, sizeof(shfi));
    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_ATTRIBUTES | SHGFI_USEFILEATTRIBUTES);
    ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) should return 1, got 0x%lx\n", rc);
    if (rc)
        ok(shfi.dwAttributes != 0xcfcfcfcf, "dwFileAttributes is not set\n");
    todo_wine ok(shfi.hIcon == 0, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) did not clear hIcon\n");
    todo_wine ok(shfi.szDisplayName[0] == 0, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) did not clear szDisplayName[0]\n");
    todo_wine ok(shfi.szTypeName[0] == 0, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) did not clear szTypeName[0]\n");
    ok(shfi.iIcon == 0xcfcfcfcf ||
       broken(shfi.iIcon != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) should not clear iIcon\n");

    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_EXETYPE | SHGFI_USEFILEATTRIBUTES);
    todo_wine ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent | SHGFI_EXETYPE) should return 1, got 0x%lx\n", rc);

    /* Test SHGFI_USEFILEATTRIBUTES support */
    strcpy(shfi.szDisplayName, "dummy");
    shfi.iIcon=0xdeadbeef;
    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
    ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent) should return 1, got 0x%lx\n", rc);
    if (rc)
    {
        ok(strcmp(shfi.szDisplayName, "dummy"), "SHGetFileInfoA(c:\\nonexistent) displayname is not set\n");
        ok(shfi.iIcon != 0xdeadbeef, "SHGetFileInfoA(c:\\nonexistent) iIcon is not set\n");
    }

    /* Wine does not have a default icon for text files, and Windows 98 fails
     * if we give it an empty executable. So use notepad.exe as the test
     */
    if (SearchPathA(NULL, "notepad.exe", NULL, sizeof(notepad), notepad, NULL))
    {
        strcpy(shfi.szDisplayName, "dummy");
        shfi.iIcon=0xdeadbeef;
        rc=SHGetFileInfoA(notepad, GetFileAttributesA(notepad),
                          &shfi, sizeof(shfi),
                          SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
        ok(rc == 1, "SHGetFileInfoA(%s, SHGFI_USEFILEATTRIBUTES) should return 1, got 0x%lx\n", notepad, rc);
        strcpy(shfi2.szDisplayName, "dummy");
        shfi2.iIcon=0xdeadbeef;
        rc2=SHGetFileInfoA(notepad, 0,
                           &shfi2, sizeof(shfi2),
                           SHGFI_ICONLOCATION);
        ok(rc2 == 1, "SHGetFileInfoA(%s) failed %lx\n", notepad, rc2);
        if (rc && rc2)
        {
            ok(lstrcmpiA(shfi2.szDisplayName, shfi.szDisplayName) == 0, "wrong display name %s != %s\n", shfi.szDisplayName, shfi2.szDisplayName);
            ok(shfi2.iIcon == shfi.iIcon, "wrong icon index %d != %d\n", shfi.iIcon, shfi2.iIcon);
        }
    }

    /* with a directory now */
    strcpy(shfi.szDisplayName, "dummy");
    shfi.iIcon=0xdeadbeef;
    rc=SHGetFileInfoA("test4.txt", GetFileAttributesA("test4.txt"),
                      &shfi, sizeof(shfi),
                      SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
    ok(rc == 1, "SHGetFileInfoA(test4.txt/, SHGFI_USEFILEATTRIBUTES) should return 1, got 0x%lx\n", rc);
    strcpy(shfi2.szDisplayName, "dummy");
    shfi2.iIcon=0xdeadbeef;
    rc2=SHGetFileInfoA("test4.txt", 0,
                      &shfi2, sizeof(shfi2),
                      SHGFI_ICONLOCATION);
    ok(rc2 == 1, "SHGetFileInfoA(test4.txt/) should return 1, got 0x%lx\n", rc2);
    if (rc && rc2)
    {
        ok(lstrcmpiA(shfi2.szDisplayName, shfi.szDisplayName) == 0, "wrong display name %s != %s\n", shfi.szDisplayName, shfi2.szDisplayName);
        ok(shfi2.iIcon == shfi.iIcon, "wrong icon index %d != %d\n", shfi.iIcon, shfi2.iIcon);
    }
    /* with drive root directory */
    strcpy(shfi.szDisplayName, "dummy");
    strcpy(shfi.szTypeName, "dummy");
    shfi.hIcon=(HICON) 0xdeadbeef;
    shfi.iIcon=0xdeadbeef;
    shfi.dwAttributes=0xdeadbeef;
    rc=SHGetFileInfoA("c:\\", 0, &shfi, sizeof(shfi),
                      SHGFI_TYPENAME | SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON);
    ok(rc == 1, "SHGetFileInfoA(c:\\) should return 1, got 0x%lx\n", rc);
    ok(strcmp(shfi.szDisplayName, "dummy") != 0, "display name was expected to change\n");
    ok(strcmp(shfi.szTypeName, "dummy") != 0, "type name was expected to change\n");
    ok(shfi.hIcon != (HICON) 0xdeadbeef, "hIcon was expected to change\n");
    ok(shfi.iIcon != 0xdeadbeef, "iIcon was expected to change\n");
}

static void check_icon_size( HICON icon, DWORD flags )
{
    ICONINFO info;
    BITMAP bm;
    SIZE metrics_size;
    int list_cx, list_cy;
    IImageList *list;

    GetIconInfo( icon, &info );
    GetObjectW( info.hbmColor, sizeof(bm), &bm );

    SHGetImageList( (flags & SHGFI_SMALLICON) ? SHIL_SMALL : SHIL_LARGE,
                    &IID_IImageList, (void **)&list );
    IImageList_GetIconSize( list, &list_cx, &list_cy );
    IImageList_Release( list );

    metrics_size.cx = GetSystemMetrics( (flags & SHGFI_SMALLICON) ? SM_CXSMICON : SM_CXICON );
    metrics_size.cy = GetSystemMetrics( (flags & SHGFI_SMALLICON) ? SM_CYSMICON : SM_CYICON );


    if (flags & SHGFI_SHELLICONSIZE)
    {
        ok( bm.bmWidth == list_cx, "got %d expected %d\n", bm.bmWidth, list_cx );
        ok( bm.bmHeight == list_cy, "got %d expected %d\n", bm.bmHeight, list_cy );
    }
    else
    {
        ok( bm.bmWidth == metrics_size.cx, "got %d expected %ld\n", bm.bmWidth, metrics_size.cx );
        ok( bm.bmHeight == metrics_size.cy, "got %d expected %ld\n", bm.bmHeight, metrics_size.cy );
    }
}

static void test_get_file_info_iconlist(void)
{
    /* Test retrieving a handle to the system image list, and
     * what that returns for hIcon
     */
    HRESULT hr;
    HIMAGELIST hSysImageList;
    LPITEMIDLIST pidList;
    SHFILEINFOA shInfoa;
    SHFILEINFOW shInfow;
    IImageList *small_list, *large_list;
    ULONG start_refs, refs;

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidList);
    if (FAILED(hr)) {
         skip("can't get desktop pidl\n");
         return;
    }

    SHGetImageList( SHIL_LARGE, &IID_IImageList, (void **)&large_list );
    SHGetImageList( SHIL_SMALL, &IID_IImageList, (void **)&small_list );

    start_refs = IImageList_AddRef( small_list );
    IImageList_Release( small_list );

    memset(&shInfoa, 0xcf, sizeof(shInfoa));
    hSysImageList = (HIMAGELIST) SHGetFileInfoA((const char *)pidList, 0,
            &shInfoa, sizeof(shInfoa),
	    SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_PIDL);
    ok(hSysImageList == (HIMAGELIST)small_list, "got %p expect %p\n", hSysImageList, small_list);
    refs = IImageList_AddRef( small_list );
    IImageList_Release( small_list );
    ok( refs == start_refs + 1 ||
        broken( refs == start_refs ), /* XP and 2003 */
        "got %ld, start_refs %ld\n", refs, start_refs );
    todo_wine ok(shInfoa.hIcon == 0, "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear hIcon\n");
    todo_wine ok(shInfoa.szTypeName[0] == 0, "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear szTypeName[0]\n");
    ok(shInfoa.iIcon != 0xcfcfcfcf, "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) should set iIcon\n");
    ok(shInfoa.dwAttributes == 0xcfcfcfcf ||
       shInfoa.dwAttributes ==  0 || /* Vista */
       broken(shInfoa.dwAttributes != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL), unexpected dwAttributes\n");
    /* Don't release hSysImageList here (and in similar places below) because of the broken reference behaviour of XP and 2003. */

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST) SHGetFileInfoW((const WCHAR *)pidList, 0,
        &shInfow, sizeof(shInfow), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_PIDL);
    ok(hSysImageList == (HIMAGELIST)small_list, "got %p expect %p\n", hSysImageList, small_list);
    todo_wine ok(shInfow.hIcon == 0, "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear hIcon\n");
    ok(shInfow.szTypeName[0] == 0, "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear szTypeName[0]\n");
    ok(shInfow.iIcon != 0xcfcfcfcf, "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) should set iIcon\n");
    ok(shInfow.dwAttributes == 0xcfcfcfcf ||
       shInfoa.dwAttributes ==  0, /* Vista */
       "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) unexpected dwAttributes\n");

    /* Various suposidly invalid flag testing */
    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hSysImageList == (HIMAGELIST)small_list, "got %p expect %p\n", hSysImageList, small_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, " SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    check_icon_size( shInfow.hIcon, SHGFI_SMALLICON );
    DestroyIcon(shInfow.hIcon);
    todo_wine ok(shInfow.dwAttributes==0,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON);
    ok(hr != 0, "SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    check_icon_size( shInfow.hIcon, SHGFI_LARGEICON );
    DestroyIcon( shInfow.hIcon );
    todo_wine ok(shInfow.dwAttributes==0,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON);
    ok(hSysImageList == (HIMAGELIST)large_list, "got %p expect %p\n", hSysImageList, small_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_OPENICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_OPENICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|
        SHGFI_ATTRIBUTES);
    ok(hSysImageList == (HIMAGELIST)small_list, "got %p expect %p\n", hSysImageList, small_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|
        SHGFI_EXETYPE);
    todo_wine ok(hSysImageList == (HIMAGELIST)small_list, "got %p expect %p\n", hSysImageList, small_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_EXETYPE);
    todo_wine ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_EXETYPE Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_ATTRIBUTES);
    ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_ATTRIBUTES Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|
        SHGFI_ATTRIBUTES);
    ok(hSysImageList == (HIMAGELIST)large_list, "got %p expect %p\n", hSysImageList, large_list);
    ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_ATTRIBUTES Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE);
    todo_wine ok(hSysImageList == (HIMAGELIST)large_list, "got %p expect %p\n", hSysImageList, large_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE);
    todo_wine ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_ATTRIBUTES);
    ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_ATTRIBUTES Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_SHELLICONSIZE|SHGFI_ICON);
    ok(hSysImageList == (HIMAGELIST)small_list, "got %p expect %p\n", hSysImageList, small_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    check_icon_size( shInfow.hIcon, SHGFI_SMALLICON | SHGFI_SHELLICONSIZE );
    DestroyIcon( shInfow.hIcon );

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST)SHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_PIDL|SHGFI_SHELLICONSIZE|SHGFI_ICON);
    ok(hSysImageList == (HIMAGELIST)large_list, "got %p expect %p\n", hSysImageList, small_list);
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    check_icon_size( shInfow.hIcon, SHGFI_LARGEICON | SHGFI_SHELLICONSIZE );
    DestroyIcon( shInfow.hIcon );

    ILFree(pidList);
    IImageList_Release( small_list );
    IImageList_Release( large_list );
}


/*
 puts into the specified buffer file names with current directory.
 files - string with file names, separated by null characters. Ends on a double
 null characters
*/
static void set_curr_dir_path(CHAR *buf, const CHAR* files)
{
    buf[0] = 0;
    while (files[0])
    {
        strcpy(buf, CURR_DIR);
        buf += strlen(buf);
        buf[0] = '\\';
        buf++;
        strcpy(buf, files);
        buf += strlen(buf) + 1;
        files += strlen(files) + 1;
    }
    buf[0] = 0;
}

#define check_file_operation(func, flags, from, to, expect_ret, expect_aborted, todo_ret, todo_aborted) \
        check_file_operation_(__LINE__, func, flags, from, to, expect_ret, expect_aborted, todo_ret, todo_aborted)
void check_file_operation_(unsigned int line, UINT func, FILEOP_FLAGS flags, const char *from, const char *to,
        DWORD expect_ret, BOOL expect_aborted, BOOL todo_ret, BOOL todo_aborted)
{
    SHFILEOPSTRUCTA op;
    DWORD ret;

    memset(&op, 0, sizeof(op));
    op.wFunc = func;
    op.fFlags = flags;
    op.pFrom = from;
    op.pTo = to;
    op.fAnyOperationsAborted = 0xdeadbeef;

    ret = SHFileOperationA(&op);
    todo_wine_if(todo_ret)
    ok_(__FILE__, line)(ret == expect_ret, "SHFileOperationA returned %ld, expected %ld.\n", ret, expect_ret);
    todo_wine_if(todo_aborted)
    ok_(__FILE__, line)(op.fAnyOperationsAborted == expect_aborted,
            "Unexpected fAnyOperationsAborted %d, expected %d.\n",
            op.fAnyOperationsAborted, expect_aborted);
}

/* tests the FO_DELETE action */
static void test_delete(void)
{
    SHFILEOPSTRUCTA shfo;
    DWORD ret;
    CHAR buf[sizeof(CURR_DIR)+sizeof("/test?.txt")+1];

    sprintf(buf, "%s\\%s", CURR_DIR, "test?.txt");
    buf[strlen(buf) + 1] = '\0';

    shfo.hwnd = NULL;
    shfo.wFunc = FO_DELETE;
    shfo.pFrom = buf;
    shfo.pTo = NULL;
    shfo.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    ok(!SHFileOperationA(&shfo), "Deletion was not successful\n");
    ok(dir_exists("test4.txt"), "Directory should not have been removed\n");
    ok(!file_exists("test1.txt"), "File should have been removed\n");
    ok(!file_exists("test2.txt"), "File should have been removed\n");
    ok(!file_exists("test3.txt"), "File should have been removed\n");

    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_SUCCESS, "Directory exists, but is not removed, ret=%ld\n", ret);
    ok(dir_exists("test4.txt"), "Directory should not have been removed\n");

    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

    ok(!SHFileOperationA(&shfo), "Directory is not removed\n");
    ok(!dir_exists("test4.txt"), "Directory should have been removed\n");

    ret = SHFileOperationA(&shfo);
    ok(!ret, "The requested file does not exist, ret=%ld\n", ret);

    init_shfo_tests();
    sprintf(buf, "%s\\%s", CURR_DIR, "test4.txt");
    buf[strlen(buf) + 1] = '\0';
    ok(MoveFileA("test1.txt", "test4.txt\\test1.txt"), "Filling the subdirectory failed\n");
    ok(!SHFileOperationA(&shfo), "Directory is not removed\n");
    ok(!dir_exists("test4.txt"), "Directory is not removed\n");

    init_shfo_tests();
    shfo.pFrom = "test1.txt\0test4.txt\0";
    ok(!SHFileOperationA(&shfo), "Directory and a file are not removed\n");
    ok(!file_exists("test1.txt"), "The file should have been removed\n");
    ok(!dir_exists("test4.txt"), "Directory should have been removed\n");
    ok(file_exists("test2.txt"), "This file should not have been removed\n");

    /* FOF_FILESONLY does not delete a dir matching a wildcard */
    init_shfo_tests();
    shfo.fFlags |= FOF_FILESONLY;
    shfo.pFrom = "*.txt\0";
    ok(!SHFileOperationA(&shfo), "Failed to delete files\n");
    ok(!file_exists("test1.txt"), "test1.txt should have been removed\n");
    ok(!file_exists("test_5.txt"), "test_5.txt should have been removed\n");
    ok(dir_exists("test4.txt"), "test4.txt should not have been removed\n");

    /* FOF_FILESONLY only deletes a dir if explicitly specified */
    init_shfo_tests();
    shfo.pFrom = "test_?.txt\0test4.txt\0";
    ok(!SHFileOperationA(&shfo), "Failed to delete files and directory\n");
    ok(!dir_exists("test4.txt") ||
       broken(dir_exists("test4.txt")), /* NT4 */
      "test4.txt should have been removed\n");
    ok(!file_exists("test_5.txt"), "test_5.txt should have been removed\n");
    ok(file_exists("test1.txt"), "test1.txt should not have been removed\n");

    /* try to delete an invalid filename */
    if (0) {
        /* this crashes on win9x */
        init_shfo_tests();
        shfo.pFrom = "\0";
        shfo.fFlags &= ~FOF_FILESONLY;
        shfo.fAnyOperationsAborted = FALSE;
        ret = SHFileOperationA(&shfo);
        ok(ret == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %ld\n", ret);
        ok(!shfo.fAnyOperationsAborted, "Expected no aborted operations\n");
        ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    }

    /* try an invalid function */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.wFunc = 0;
    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_INVALID_PARAMETER ||
       broken(ret == ERROR_SUCCESS), /* Win9x, NT4 */
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* try an invalid list, only one null terminator */
    if (0) {
        /* this crashes on win9x */
        init_shfo_tests();
        shfo.pFrom = "";
        shfo.wFunc = FO_DELETE;
        ret = SHFileOperationA(&shfo);
        ok(ret == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %ld\n", ret);
        ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    }

    /* delete a nonexistent file */
    shfo.pFrom = "nonexistent.txt\0";
    shfo.wFunc = FO_DELETE;
    ret = SHFileOperationA(&shfo);
    ok(ret == 1026 ||
       ret == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(ret == ERROR_SUCCESS), /* NT4 */
       "Expected 1026 or ERROR_FILE_NOT_FOUND, got %ld\n", ret);

    /* delete a dir, and then a file inside the dir, same as
    * deleting a nonexistent file
    */
    if (ret != ERROR_FILE_NOT_FOUND)
    {
        /* Vista would throw up a dialog box that we can't suppress */
        init_shfo_tests();
        shfo.pFrom = "testdir2\0testdir2\\one.txt\0";
        ret = SHFileOperationA(&shfo);
        ok(ret == ERROR_PATH_NOT_FOUND ||
           broken(ret == ERROR_SUCCESS), /* NT4 */
           "Expected ERROR_PATH_NOT_FOUND, got %ld\n", ret);
        ok(!dir_exists("testdir2"), "Expected testdir2 to not exist\n");
        ok(!file_exists("testdir2\\one.txt"), "Expected testdir2\\one.txt to not exist\n");
    }
    else
        skip("Test would show a dialog box\n");

    /* delete an existent file and a nonexistent file */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0nonexistent.txt\0test2.txt\0";
    shfo.wFunc = FO_DELETE;
    ret = SHFileOperationA(&shfo);
    ok(ret == 1026 ||
       ret == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(ret == ERROR_SUCCESS), /* NT4 */
       "Expected 1026 or ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    todo_wine
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");

    /* delete a nonexistent file in an existent dir or a nonexistent dir */
    init_shfo_tests();
    shfo.pFrom = "testdir2\\nonexistent.txt\0";
    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(ret == 0x402) || /* XP */
       broken(ret == ERROR_SUCCESS), /* NT4 */
       "Expected 0x402 or ERROR_FILE_NOT_FOUND, got %lx\n", ret);
    shfo.pFrom = "nonexistent\\one.txt\0";
    ret = SHFileOperationA(&shfo);
    ok(ret == DE_INVALIDFILES || /* Vista or later */
       broken(ret == 0x402), /* XP */
       "Expected 0x402 or DE_INVALIDFILES, got %lx\n", ret);

    /* try the FOF_NORECURSION flag, continues deleting subdirs */
    init_shfo_tests();
    shfo.pFrom = "testdir2\0";
    shfo.fFlags |= FOF_NORECURSION;
    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!file_exists("testdir2\\one.txt"), "Expected testdir2\\one.txt to not exist\n");
    ok(!dir_exists("testdir2\\nested"), "Expected testdir2\\nested to not exist\n");
}

/* tests the FO_RENAME action */
static void test_rename(void)
{
    char from[5 * MAX_PATH], to[5 * MAX_PATH];

    /* Rename a file to a dir. */
    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            DE_FILEDESTISFLD, FALSE, TRUE, FALSE);
    ok(file_exists("test1.txt"), "The file is renamed\n");

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            DE_DIFFDIR, FALSE, TRUE, FALSE);
    todo_wine
    ok(!file_exists("test4.txt\\test1.txt"), "The file is renamed\n");

    /* Multiple sources and targets, no FOF_MULTIDESTFILES. */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            DE_MANYSRC1DEST, FALSE, TRUE, FALSE);
    ok(file_exists("test1.txt"), "The file is renamed - many files are specified\n");

    /* Multiple sources and targets, with FOF_MULTIDESTFILES. */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI | FOF_MULTIDESTFILES, from, to,
            DE_MANYSRC1DEST, FALSE, TRUE, FALSE);
    ok(file_exists("test1.txt"), "The file is not renamed - many files are specified\n");

    /* Rename a file. */
    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(!file_exists("test1.txt"), "The file is not renamed\n");
    ok(file_exists("test6.txt"), "The file is not renamed\n");

    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);

    /* Rename a dir. */
    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(!dir_exists("test4.txt"), "The dir is not renamed\n");
    ok(dir_exists("test6.txt"), "The dir is not renamed\n");

    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    check_file_operation(FO_RENAME, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(dir_exists("test4.txt"), "The dir is not renamed\n");

    /* Rename multiple files to a single file. */
    check_file_operation(FO_RENAME, FOF_NO_UI,
            "test1.txt\0test2.txt\0", "a.txt\0",
            DE_MANYSRC1DEST, FALSE, TRUE, FALSE);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");
    ok(!file_exists("a.txt"), "Expected a.txt to not exist\n");

    /* Rename a nonexistent file. */
    check_file_operation(FO_RENAME, FOF_NO_UI,
            "idontexist\0", "newfile\0",
            ERROR_FILE_NOT_FOUND, FALSE, TRUE, FALSE);
    ok(!file_exists("newfile"), "Expected newfile to not exist\n");

    /* Target already exists. */
    check_file_operation(FO_RENAME, FOF_NO_UI,
            "test1.txt\0", "test2.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, FALSE);

    /* Empty target. */
    createTestFile("test1.txt");
    check_file_operation(FO_RENAME, FOF_NO_UI,
            "test1.txt\0", "\0",
            DE_DIFFDIR, FALSE, TRUE, TRUE);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* Empty source. */
    check_file_operation(FO_RENAME, FOF_NO_UI,
            "\0", "\0",
            DE_MANYSRC1DEST, FALSE, TRUE, TRUE);

    /* pFrom is NULL. */
    check_file_operation(FO_RENAME, FOF_NO_UI,
            NULL, "\0",
            ERROR_INVALID_PARAMETER, 0xdeadbeef, FALSE, FALSE);
}

/* tests the FO_COPY action */
static void test_copy(void)
{
    char from[5 * MAX_PATH], to[5 * MAX_PATH];
    SHFILEOPSTRUCTA shfo;
    DWORD retval;
    LPSTR ptr;
    BOOL ret;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_COPY;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    /* Sources and targets have the same number, no FOF_MULTIDESTFILES. */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    check_file_operation(FO_COPY, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    ok(DeleteFileA("test6.txt\\test1.txt"), "test6.txt\\test1.txt is not copied.\n");
    todo_wine
    ok(DeleteFileA("test6.txt\\test2.txt"), "test6.txt\\test2.txt is not copied.\n");
    RemoveDirectoryA("test6.txt\\test4.txt");
    RemoveDirectoryA("test6.txt");

    /* Sources and targets have the same number, with FOF_MULTIDESTFILES. */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("test6.txt"), "The file is not copied - many files are "
       "specified as a target\n");
    DeleteFileA("test6.txt");
    DeleteFileA("test7.txt");
    RemoveDirectoryA("test8.txt");

    /* Sources outnumber targets, with FOF_MULTIDESTFILES. */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES, from, to,
            DE_DESTSAMETREE, FALSE, TRUE, TRUE);
    todo_wine
    ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not copied.\n");
    RemoveDirectoryA("test6.txt");
    todo_wine
    ok(DeleteFileA("test7.txt\\test2.txt"), "The file is not copied.\n");
    RemoveDirectoryA("test7.txt");

    /* Wildcard source. */
    createTestFile("test4.txt\\test1.txt");
    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    check_file_operation(FO_COPY, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(file_exists("testdir2\\test4.txt"), "The directory is copied\n");
    ok(file_exists("testdir2\\test4.txt\\test1.txt"), "The file in subdirectory is copied\n");
    clean_after_shfo_tests();

    /* Wildcard source, with FOF_FILESONLY. */
    init_shfo_tests();
    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_FILESONLY, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(!file_exists("testdir2\\test4.txt"), "The directory is copied\n");
    clean_after_shfo_tests();

    /* Multiple sources, with FOF_FILESONLY. */
    init_shfo_tests();
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_FILESONLY, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(file_exists("testdir2\\test2.txt"), "The file is copied\n");
    clean_after_shfo_tests();

    /* Multiple sources, one not existing. */
    init_shfo_tests();
    set_curr_dir_path(from, "test1.txt\0test10.txt\0test2.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    check_file_operation(FO_COPY, FOF_NO_UI, from, to,
            ERROR_FILE_NOT_FOUND, FALSE, TRUE, FALSE);
    ok(!file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(!file_exists("testdir2\\test2.txt"), "The file is copied\n");
    clean_after_shfo_tests();

    /* Nonexistent target directory. */
    init_shfo_tests();
    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "nonexistent\\notreal\\test2.txt\0");
    check_file_operation(FO_COPY, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("nonexistent"), "nonexistent not created\n");
    ok(file_exists("nonexistent\\notreal"), "nonexistent\\notreal not created\n");
    ok(file_exists("nonexistent\\notreal\\test2.txt"), "Directory not created\n");
    ok(!file_exists("nonexistent\\notreal\\test1.txt"), "test1.txt should not exist\n");
    clean_after_shfo_tests();

    /* Relative path. */
    init_shfo_tests();
    check_file_operation(FO_COPY, FOF_NO_UI,"test1.txt\0test2.txt\0test3.txt\0", "testdir2\0",
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1 to exist\n");
    clean_after_shfo_tests();

    /* Overwrite an existing write protected file. */
    init_shfo_tests();
    ret = SetFileAttributesA("test2.txt", FILE_ATTRIBUTE_READONLY);
    ok(ret, "Failure to set file attributes (error %lx)\n", GetLastError());
    retval = CopyFileA("test1.txt", "test2.txt", FALSE);
    ok(!retval && GetLastError() == ERROR_ACCESS_DENIED, "CopyFileA should have fail with ERROR_ACCESS_DENIED\n");
    check_file_operation(FO_COPY, FOF_NO_UI,"test1.txt\0", "test2.txt\0",
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    /* Set back normal attributes to make the file deletion succeed */
    ret = SetFileAttributesA("test2.txt", FILE_ATTRIBUTE_NORMAL);
    ok(ret, "Failure to set file attributes (error %lx)\n", GetLastError());
    clean_after_shfo_tests();

    /* Copy files to a file */
    init_shfo_tests();
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "test3.txt\0");
    check_file_operation(FO_COPY, FOF_NO_UI, from, to,
            DE_INVALIDFILES, FALSE, TRUE, TRUE);
    ok(!file_exists("test3.txt\\test2.txt"), "Expected test3.txt\\test2.txt to not exist\n");

    /* Copy many files to a nonexistent directory. */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "test3.txt\0");
    DeleteFileA(to);
    check_file_operation(FO_COPY, FOF_NO_UI, from, to,
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(DeleteFileA("test3.txt\\test1.txt"), "Expected test3.txt\\test1.txt to exist\n");
    ok(DeleteFileA("test3.txt\\test2.txt"), "Expected test3.txt\\test1.txt to exist\n");
    ok(RemoveDirectoryA(to), "Expected test3.txt to exist\n");

    /* Targets outnumber sources, with FOF_MULTIDESTFILES. */
    init_shfo_tests();
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test1.txt\0test2.txt\0test3.txt\0",
            "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0testdir2\\d.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("testdir2\\a.txt\\test1.txt"), "Expected testdir2\\a.txt\\test1.txt to exist\n");
    RemoveDirectoryA("testdir2\\a.txt");
    ok(DeleteFileA("testdir2\\b.txt\\test2.txt"), "Expected testdir2\\b.txt\\test2.txt to exist\n");
    RemoveDirectoryA("testdir2\\b.txt");
    ok(DeleteFileA("testdir2\\c.txt\\test3.txt"), "Expected testdir2\\c.txt\\test3.txt to exist\n");
    }
    RemoveDirectoryA("testdir2\\c.txt");
    ok(!file_exists("testdir2\\d.txt"), "Expected testdir2\\d.txt to not exist\n");

    /* Sources outnumber targets, with FOF_MULTIDESTFILES. */
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test1.txt\0test2.txt\0test3.txt\0",
            "e.txt\0f.txt\0",
            DE_SAMEFILE, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("e.txt\\test1.txt"), "Expected e.txt\\test1.txt to exist\n");
    RemoveDirectoryA("e.txt");
    ok(DeleteFileA("f.txt\\test2.txt"), "Expected f.txt\\test2.txt to exist\n");
    RemoveDirectoryA("f.txt");
    }

    /* Sources and targets have the same number, with FOF_MULTIDESTFILES. */
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test1.txt\0test2.txt\0test4.txt\0",
            "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0",
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(DeleteFileA("testdir2\\a.txt"), "Expected testdir2\\a.txt to exist\n");
    ok(DeleteFileA("testdir2\\b.txt"), "Expected testdir2\\b.txt to exist\n");
    ok(RemoveDirectoryA("testdir2\\c.txt"), "Expected testdir2\\c.txt to exist\n");

    /* Sources and targets have the same number, no FOF_MULTIDESTFILES. */
    check_file_operation(FO_COPY, FOF_NO_UI,
            "test1.txt\0test2.txt\0test3.txt\0",
            "a.txt\0b.txt\0c.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("a.txt\\test1.txt"), "Expected a.txt\\test1.txt to exist\n");
    ok(DeleteFileA("a.txt\\test2.txt"), "Expected a.txt\\test2.txt to exist\n");
    ok(DeleteFileA("a.txt\\test3.txt"), "Expected a.txt\\test3.txt to exist\n");
    }
    ok(!dir_exists("b.txt"), "Expected b.txt directory to not exist.\n");
    ok(!dir_exists("c.txt"), "Expected c.txt directory to not exist.\n");
    RemoveDirectoryA("a.txt");

    /* Sources outnumber targets, no FOF_MULTIDESTFILES. */
    check_file_operation(FO_COPY, FOF_NO_UI,
            "test1.txt\0test2.txt\0test3.txt\0",
            "a.txt\0b.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("a.txt\\test1.txt"), "Expected a.txt\\test1.txt to exist\n");
    ok(DeleteFileA("a.txt\\test2.txt"), "Expected a.txt\\test2.txt to exist\n");
    ok(DeleteFileA("a.txt\\test3.txt"), "Expected a.txt\\test3.txt to exist\n");
    }
    ok(!dir_exists("b.txt"), "Expected b.txt directory to not exist.\n");
    RemoveDirectoryA("a.txt");

    /* Wildcard source. */
    check_file_operation(FO_COPY, FOF_NO_UI, "test?.txt\0", "testdir2\0",
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");
    ok(dir_exists("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to exist\n");

    /* Wildcard source, with FOF_FILESONLY. */
    clean_after_shfo_tests();
    init_shfo_tests();
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_FILESONLY, "test?.txt\0", "testdir2\0",
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");
    ok(!dir_exists("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to not exist\n");

    /* Wildcard source, and same number of targets, with FOF_MULTIDESTFILES. */
    clean_after_shfo_tests();
    init_shfo_tests();
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test?.txt\0", "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0testdir2\\d.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("testdir2\\a.txt\\test1.txt"), "Expected testdir2\\a.txt\\test1.txt to exist\n");
    ok(DeleteFileA("testdir2\\a.txt\\test2.txt"), "Expected testdir2\\a.txt\\test2.txt to exist\n");
    ok(DeleteFileA("testdir2\\a.txt\\test3.txt"), "Expected testdir2\\a.txt\\test3.txt to exist\n");
    ok(RemoveDirectoryA("testdir2\\a.txt\\test4.txt"), "Expected testdir2\\a.txt\\test4.txt to exist\n");
    }
    RemoveDirectoryA("testdir2\\a.txt");
    ok(!RemoveDirectoryA("b.txt"), "b.txt should not exist\n");

    /* Copy one file to two others, second is ignored. */
    clean_after_shfo_tests();
    init_shfo_tests();
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test1.txt\0", "b.txt\0c.txt\0",
            ERROR_SUCCESS, FALSE, FALSE, FALSE);
    ok(DeleteFileA("b.txt"), "Expected b.txt to exist\n");
    ok(!DeleteFileA("c.txt"), "Expected c.txt to not exist\n");

    /* Copy two file to three others. */
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test1.txt\0test2.txt\0", "b.txt\0c.txt\0d.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("b.txt\\test1.txt"), "Expected b.txt\\test1.txt to exist\n");
    RemoveDirectoryA("b.txt");
    ok(DeleteFileA("c.txt\\test2.txt"), "Expected c.txt\\test2.txt to exist\n");
    }
    RemoveDirectoryA("c.txt");

    /* Copy one file and one directory to three others. */
    check_file_operation(FO_COPY, FOF_NO_UI | FOF_MULTIDESTFILES,
            "test1.txt\0test4.txt\0", "b.txt\0c.txt\0d.txt\0",
            ERROR_SUCCESS, FALSE, TRUE, TRUE);
    todo_wine
    {
    ok(DeleteFileA("b.txt\\test1.txt"), "Expected b.txt\\test1.txt to exist\n");
    RemoveDirectoryA("b.txt");
    ok(RemoveDirectoryA("c.txt\\test4.txt"), "Expected c.txt\\test4.txt to exist\n");
    RemoveDirectoryA("c.txt");
    }

    /* copy a directory with a file beneath it, plus some files */
    createTestFile("test4.txt\\a.txt");
    shfo.pFrom = "test4.txt\0test1.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");
    ok(DeleteFileA("testdir2\\test4.txt\\a.txt"), "Expected a.txt to exist\n");
    ok(RemoveDirectoryA("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to exist\n");

    /* copy one directory and a file in that dir to another dir */
    shfo.pFrom = "test4.txt\0test4.txt\\a.txt\0";
    shfo.pTo = "testdir2\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("testdir2\\test4.txt\\a.txt"), "Expected a.txt to exist\n");
    ok(DeleteFileA("testdir2\\a.txt"), "Expected testdir2\\a.txt to exist\n");

    /* copy a file in a directory first, and then the directory to a nonexistent dir */
    shfo.pFrom = "test4.txt\\a.txt\0test4.txt\0";
    shfo.pTo = "nonexistent\0";
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    todo_wine
    ok(DeleteFileA("nonexistent\\test4.txt\\a.txt"), "Expected nonexistent\\test4.txt\\a.txt to exist\n");
    RemoveDirectoryA("nonexistent\\test4.txt");
    todo_wine
    ok(DeleteFileA("nonexistent\\a.txt"), "Expected nonexistent\\a.txt to exist\n");
    RemoveDirectoryA("nonexistent");
    DeleteFileA("test4.txt\\a.txt");

    /* destination is same as source file */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "b.txt\0test2.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags = FOF_NOERRORUI | FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    ok(retval == DE_SAMEFILE, "Expected DE_SAMEFILE, got %ld\n", retval);
    ok(DeleteFileA("b.txt"), "Expected b.txt to exist\n");
    ok(!shfo.fAnyOperationsAborted, "Expected no operations to be aborted\n");
    ok(!file_exists("c.txt"), "Expected c.txt to not exist\n");

    /* destination is same as source directory */
    shfo.pFrom = "test1.txt\0test4.txt\0test3.txt\0";
    shfo.pTo = "b.txt\0test4.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_DESTSAMETREE, "Expected DE_DESTSAMETREE, got %ld\n", retval);
    ok(DeleteFileA("b.txt"), "Expected b.txt to exist\n");
    ok(!file_exists("c.txt"), "Expected c.txt to not exist\n");

    /* copy a directory into itself, error displayed in UI */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\\newdir\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_DESTSUBTREE, "Expected DE_DESTSUBTREE, got %ld\n", retval);
    ok(!RemoveDirectoryA("test4.txt\\newdir"), "Expected test4.txt\\newdir to not exist\n");

    /* copy a directory to itself, error displayed in UI */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_DESTSUBTREE, "Expected DE_DESTSUBTREE, got %ld\n", retval);

    /* copy a file into a directory, and the directory into itself */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "test4.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags |= FOF_NOCONFIRMATION;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_DESTSUBTREE, "Expected DE_DESTSUBTREE, got %ld\n", retval);
    ok(DeleteFileA("test4.txt\\test1.txt"), "Expected test4.txt\\test1.txt to exist\n");

    /* copy a file to a file, and the directory into itself */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "test4.txt\\a.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_DESTSUBTREE, "Expected DE_DESTSUBTREE, got %ld\n", retval);
    todo_wine
    ok(DeleteFileA("test4.txt\\a.txt\\test1.txt"), "Expected test4.txt\\a.txt\\test1.txt to exist\n");
    RemoveDirectoryA("test4.txt\\a.txt");

    /* copy a nonexistent file to a nonexistent directory */
    shfo.pFrom = "e.txt\0";
    shfo.pTo = "nonexistent\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", retval);
    ok(!file_exists("nonexistent\\e.txt"), "Expected nonexistent\\e.txt to not exist\n");
    ok(!file_exists("nonexistent"), "Expected nonexistent to not exist\n");

    /* Overwrite tests */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.fFlags = FOF_NOCONFIRMATION;
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "test2.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    /* without FOF_NOCONFIRMATION the confirmation is Yes/No */
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
    ok(file_has_content("test2.txt", "test1.txt\n"), "The file was not copied\n");

    shfo.pFrom = "test3.txt\0test1.txt\0";
    shfo.pTo = "test2.txt\0one.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_MULTIDESTFILES;
    /* without FOF_NOCONFIRMATION the confirmation is Yes/Yes to All/No/Cancel */
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(file_has_content("test2.txt", "test3.txt\n"), "The file was not copied\n");

    shfo.pFrom = "one.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags = FOF_NOCONFIRMATION;
    /* without FOF_NOCONFIRMATION the confirmation is Yes/No */
    retval = SHFileOperationA(&shfo);
    ok(retval == 0, "Expected 0, got %ld\n", retval);
    ok(file_has_content("testdir2\\one.txt", "test1.txt\n"), "The file was not copied\n");

    createTestFile("test4.txt\\test1.txt");
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "testdir2\0";
    /* WinMe needs FOF_NOERRORUI */
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    shfo.fFlags = FOF_NOCONFIRMATION;
    if (ERROR_SUCCESS)
    {
        createTestFile("test4.txt\\.\\test1.txt"); /* modify the content of the file */
        /* without FOF_NOCONFIRMATION the confirmation is "This folder already contains a folder named ..." */
        retval = SHFileOperationA(&shfo);
        ok(retval == 0, "Expected 0, got %ld\n", retval);
        ok(file_has_content("testdir2\\test4.txt\\test1.txt", "test4.txt\\.\\test1.txt\n"), "The file was not copied\n");
    }

    createTestFile("one.txt");

    /* pFrom contains bogus 2nd name longer than MAX_PATH */
    memset(from, 'a', MAX_PATH*2);
    memset(from+MAX_PATH*2, 0, 2);
    lstrcpyA(from, "one.txt");
    shfo.pFrom = from;
    shfo.pTo = "two.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_INVALIDFILES, "Unexpected return value, got %ld\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    todo_wine
    ok(RemoveDirectoryA("two.txt"), "Expected two.txt to exist\n");

    createTestFile("one.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH */
    memset(to, 'a', MAX_PATH*2);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "two.txt");
    shfo.pFrom = "one.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");

    createTestFile("one.txt");

    /* no FOF_MULTIDESTFILES, two files in pTo */
    shfo.pFrom = "one.txt\0";
    shfo.pTo = "two.txt\0three.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");

    createTestFile("one.txt");

    /* both pFrom and pTo contain bogus 2nd names longer than MAX_PATH */
    memset(from, 'a', MAX_PATH*2);
    memset(from+MAX_PATH*2, 0, 2);
    memset(to, 'a', MAX_PATH*2);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(from, "one.txt");
    lstrcpyA(to, "two.txt");
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_INVALIDFILES, "Unexpected return value, got %ld\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    todo_wine
    ok(RemoveDirectoryA("two.txt"), "Expected two.txt to exist\n");

    createTestFile("one.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH, FOF_MULTIDESTFILES */
    memset(to, 'a', MAX_PATH*2);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "two.txt");
    shfo.pFrom = "one.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files,
     * dest directory does not exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    {
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld.\n", retval);
    ok(DeleteFileA("threedir\\one.txt"), "Expected threedir\\one.txt to exist.\n");
    ok(DeleteFileA("threedir\\two.txt"), "Expected threedir\\one.txt to exist.\n");
    ok(RemoveDirectoryA("threedir"), "Expected threedir to exist.\n");
    }
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files,
     * dest directory does exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("threedir\\two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files, FOF_MULTIDESTFILES
     * dest dir does not exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                      FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %ld\n", retval);
    ok(!DeleteFileA("threedir\\one.txt"), "Expected file to not exist\n");
    ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(!RemoveDirectoryA("threedir"), "Expected dir to not exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files, FOF_MULTIDESTFILES
     * dest dir does exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    ptr = to + lstrlenA(to) + 1;
    lstrcpyA(ptr, "fourdir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    todo_wine
    ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
    ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
    todo_wine
    ok(DeleteFileA("fourdir\\two.txt"), "Expected file to exist\n");
    RemoveDirectoryA("fourdir");
    todo_wine
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* multiple source files, FOF_MULTIDESTFILES
     * multiple dest files, but first dest dir exists
     * num files in lists is equal
     */
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = "threedir\0fourdir\0";
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == DE_FILEDESTISFLD, "Expected DE_FILEDESTISFLD. got %ld\n", retval);
    ok(!DeleteFileA("threedir\\one.txt"), "Expected file to not exist\n");
    ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    todo_wine
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");
    todo_wine
    ok(!DeleteFileA("fourdir"), "Expected file to not exist\n");
    ok(!RemoveDirectoryA("fourdir"), "Expected dir to not exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* multiple source files, FOF_MULTIDESTFILES
     * multiple dest files, but first dest dir exists
     * num files in lists is not equal
     */
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = "threedir\0fourdir\0five\0";
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    todo_wine
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    todo_wine
    ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
    ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
    todo_wine
    ok(DeleteFileA("fourdir\\two.txt"), "Expected file to exist\n");
    RemoveDirectoryA("fourdir");
    todo_wine
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");
    ok(!DeleteFileA("five"), "Expected file to not exist\n");
    ok(!RemoveDirectoryA("five"), "Expected dir to not exist\n");

    createTestFile("aa.txt");
    createTestFile("ab.txt");
    createTestFile("bb.txt");
    CreateDirectoryA("one", NULL);
    CreateDirectoryA("two", NULL);

    /* Test with FOF_MULTIDESTFILES with a wildcard in pFrom and single output directory. */
    shfo.pFrom = "a*.txt\0";
    shfo.pTo = "one\0";
    shfo.fFlags |= FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("one\\aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one\\ab.txt"), "Expected file to exist\n");

    /* pFrom has a glob, pTo has more than one dest */
    shfo.pFrom = "a*.txt\0";
    shfo.pTo = "one\0two\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("one\\aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one\\ab.txt"), "Expected file to exist\n");
    ok(!DeleteFileA("two\\aa.txt"), "Expected file to not exist\n");
    ok(!DeleteFileA("two\\ab.txt"), "Expected file to not exist\n");

    /* pFrom has more than one glob, pTo has more than one dest, without FOF_MULTIDESTFILES. */
    shfo.pFrom = "a*.txt\0*b.txt\0";
    shfo.pTo = "one\0two\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("one\\aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one\\ab.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one\\bb.txt"), "Expected file to exist\n");
    ok(!DeleteFileA("two\\ab.txt"), "Expected file to exist\n");
    ok(!DeleteFileA("two\\bb.txt"), "Expected file to exist\n");

    /* pFrom has more than one glob, pTo has more than one dest, with FOF_MULTIDESTFILES. */
    shfo.pFrom = "a*.txt\0*b.txt\0";
    shfo.pTo = "one\0two\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    todo_wine
    {
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("one\\aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one\\ab.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two\\ab.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two\\bb.txt"), "Expected file to exist\n");
    }

    ok(DeleteFileA("aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("ab.txt"), "Expected file to exist\n");
    ok(DeleteFileA("bb.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("one"), "Expected dir to exist\n");
    ok(RemoveDirectoryA("two"), "Expected dir to exist\n");

    /* pTo is an empty string  */
    CreateDirectoryA("dir", NULL);
    createTestFile("dir\\abcdefgh.abc");
    shfo.pFrom = "dir\\abcdefgh.abc\0";
    shfo.pTo = "\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("abcdefgh.abc"), "Expected file to exist\n");
    ok(DeleteFileA("dir\\abcdefgh.abc"), "Expected file to exist\n");
    ok(RemoveDirectoryA("dir"), "Expected dir to exist\n");

    /* Check last error after a successful file operation. */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    SetLastError(0xdeadbeef);
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "File copy failed with %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());

    /* Check last error after a failed file operation. */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "nonexistent\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    SetLastError(0xdeadbeef);
    retval = SHFileOperationA(&shfo);
    ok(retval != ERROR_SUCCESS, "Unexpected ERROR_SUCCESS\n");
    ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());

    /* test with / */
    CreateDirectoryA("dir", NULL);
    CreateDirectoryA("dir\\subdir", NULL);
    createTestFile("dir\\subdir\\aa.txt");
    shfo.pFrom = "dir/subdir/aa.txt\0";
    shfo.pTo = "dir\\destdir/aa.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", retval);
    ok(DeleteFileA("dir\\destdir\\aa.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("dir\\destdir"), "Expected dir to exist\n");
    ok(DeleteFileA("dir\\subdir\\aa.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("dir\\subdir"), "Expected dir to exist\n");
    ok(RemoveDirectoryA("dir"), "Expected dir to exist\n");
}

/* tests the FO_MOVE action */
static void test_move(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    DWORD retval;

    clean_after_shfo_tests();
    init_shfo_tests();

    shfo.hwnd = NULL;
    shfo.wFunc = FO_MOVE;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;
    shfo.fAnyOperationsAborted = FALSE;

    set_curr_dir_path(from, "testdir2\\*.*\0");
    set_curr_dir_path(to, "test4.txt\\*.*\0");
    retval = SHFileOperationA(&shfo);
    ok(retval != 0, "SHFileOperation should fail\n");
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(file_exists("testdir2"), "dir should not be moved\n");
    ok(file_exists("testdir2\\one.txt"), "file should not be moved\n");
    ok(file_exists("testdir2\\nested"), "dir should not be moved\n");
    ok(file_exists("testdir2\\nested\\two.txt"), "file should not be moved\n");

    set_curr_dir_path(from, "testdir2\\*.*\0");
    set_curr_dir_path(to, "test4.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "SHFileOperation error %#lx\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(file_exists("testdir2"), "dir should not be moved\n");
    ok(!file_exists("testdir2\\one.txt"), "file should be moved\n");
    ok(!file_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(file_exists("test4.txt"), "dir should exist\n");
    ok(file_exists("test4.txt\\one.txt"), "file should exist\n");
    ok(file_exists("test4.txt\\nested"), "dir should exist\n");
    ok(file_exists("test4.txt\\nested\\two.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* same tests above, but with / */
    set_curr_dir_path(from, "testdir2/*.*\0");
    set_curr_dir_path(to, "test4.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(dir_exists("testdir2"), "dir should not be moved\n");
    ok(!file_exists("testdir2\\one.txt"), "file should be moved\n");
    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(file_exists("test4.txt\\one.txt"), "file should exist\n");
    ok(dir_exists("test4.txt\\nested"), "dir should exist\n");
    ok(file_exists("test4.txt\\nested\\two.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    shfo.hwnd = NULL;
    shfo.wFunc = FO_MOVE;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Prepare test to check how directories are moved recursively\n");
    ok(!file_exists("test1.txt"), "test1.txt should not exist\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is not moved\n");

    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    ok(!file_exists("testdir2\\test2.txt"), "The file is not moved yet\n");
    ok(!file_exists("testdir2\\test4.txt"), "The directory is not moved yet\n");
    ok(!SHFileOperationA(&shfo), "Files and directories are moved to directory\n");
    ok(file_exists("testdir2\\test2.txt"), "The file is moved\n");
    ok(file_exists("testdir2\\test4.txt"), "The directory is moved\n");
    ok(file_exists("testdir2\\test4.txt\\test1.txt"), "The file in subdirectory is moved\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* test moving dir to destination containing dir of the same name */
    set_curr_dir_path(from, "testdir2\\nested\0");
    set_curr_dir_path(to, "testdir4\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(dir_exists("testdir4\\nested"), "dir should exist\n");
    ok(file_exists("testdir4\\nested\\two.txt"), "file should exist\n");
    ok(file_exists("testdir4\\nested\\2.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* test moving empty dir to destination containing dir of the same name */
    DeleteFileA("testdir2\\nested\\two.txt");
    set_curr_dir_path(from, "testdir2\\nested\0");
    set_curr_dir_path(to, "testdir4\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");

    ok(dir_exists("testdir4\\nested"), "dir should exist\n");
    ok(file_exists("testdir4\\nested\\2.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* test moving multiple dirs to destination containing dir of the same name */
    set_curr_dir_path(from, "testdir2\\nested\0testdir4\\nested\0");
    set_curr_dir_path(to, "testdir6\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(!dir_exists("testdir4\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir4\\nested\\2.txt"), "file should be moved\n");
    ok(!dir_exists("testdir4\\nested\\subnested"), "dir should be moved\n");
    ok(!file_exists("testdir4\\nested\\subnested\\3.txt"), "file should be moved\n");

    ok(dir_exists("testdir6\\nested"), "dir should exist\n");
    ok(file_exists("testdir6\\nested\\two.txt"), "file should exist\n");
    ok(file_exists("testdir6\\nested\\2.txt"), "file should exist\n");
    ok(dir_exists("testdir6\\nested\\subnested"), "dir should exist\n");
    ok(file_exists("testdir6\\nested\\subnested\\3.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    /* test moving dir to destination containing dir of the same name with FOF_MULTIDESTFILES set */
    set_curr_dir_path(from, "testdir2\\nested\0");
    set_curr_dir_path(to, "testdir6\0");
    retval = SHFileOperationA(&shfo2);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(!file_exists("testdir6\\nested\\two.txt"), "file should not exist\n");
    ok(dir_exists("testdir6\\nested"), "dir should exist\n");
    ok(file_exists("testdir6\\two.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* same as above, without 'nested' in from path */
    set_curr_dir_path(from, "testdir2\0");
    set_curr_dir_path(to, "testdir6\0");
    retval = SHFileOperationA(&shfo2);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(!file_exists("testdir6\\two.txt"), "file should not exist\n");
    ok(dir_exists("testdir6\\nested"), "dir should exist\n");
    ok(file_exists("testdir6\\nested\\two.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* test moving multiple dirs to multiple destinations containing dir of the same name */
    set_curr_dir_path(from, "testdir2\\nested\0testdir4\\nested\0");
    set_curr_dir_path(to, "testdir6\0testdir8\0");
    retval = SHFileOperationA(&shfo2);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(!dir_exists("testdir4\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir4\\nested\\2.txt"), "file should be moved\n");

    ok(!file_exists("testdir6\\nested\\two.txt"), "file should not exist\n");
    ok(!file_exists("testdir6\\nested\\2.txt"), "file should not exist\n");
    ok(dir_exists("testdir6\\nested"), "dir should exist\n");
    ok(file_exists("testdir6\\two.txt"), "file should exist\n");

    ok(!dir_exists("testdir8\\nested\\subnested"), "dir should not exist\n");
    ok(!file_exists("testdir8\\nested\\subnested\\3.txt"), "file should not exist\n");
    ok(!file_exists("testdir8\\nested\\two.txt"), "file should not exist\n");
    ok(!file_exists("testdir8\\nested\\2.txt"), "file should not exist\n");
    ok(dir_exists("testdir8\\nested"), "dir should exist\n");
    ok(dir_exists("testdir8\\subnested"), "dir should exist\n");
    ok(file_exists("testdir8\\subnested\\3.txt"), "file should exist\n");
    ok(file_exists("testdir8\\2.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    /* same as above, but include subdir in destinations */
    set_curr_dir_path(from, "testdir2\\nested\0testdir4\\nested\0");
    set_curr_dir_path(to, "testdir6\\nested\0testdir8\\nested\0");
    retval = SHFileOperationA(&shfo2);
    ok(!retval, "got %ld\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(!dir_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(!dir_exists("testdir4\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir4\\nested\\2.txt"), "file should be moved\n");

    ok(dir_exists("testdir6\\nested"), "dir should exist\n");
    ok(file_exists("testdir6\\nested\\two.txt"), "file should exist\n");
    ok(!file_exists("testdir6\\nested\\2.txt"), "file should not exist\n");
    ok(!file_exists("testdir6\\two.txt"), "file should not exist\n");

    ok(!dir_exists("testdir8\\subnested"), "dir should not exist\n");
    ok(!file_exists("testdir8\\2.txt"), "file should not exist\n");
    ok(!file_exists("testdir8\\nested\\two.txt"), "file should not exist\n");
    ok(dir_exists("testdir8\\nested"), "dir should exist\n");
    ok(file_exists("testdir8\\nested\\2.txt"), "file should exist\n");
    ok(dir_exists("testdir8\\nested\\subnested"), "dir should exist\n");
    ok(file_exists("testdir8\\nested\\subnested\\3.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(!SHFileOperationA(&shfo2), "Move many files\n");
    ok(DeleteFileA("test6.txt"), "The file is not moved - many files are "
       "specified as a target\n");
    ok(DeleteFileA("test7.txt"), "The file is not moved\n");
    ok(RemoveDirectoryA("test8.txt"), "The directory is not moved\n");

    init_shfo_tests();

    /* number of sources does not correspond to number of targets,
       include directories */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == DE_DESTSAMETREE, "got %ld\n", retval);
    ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not moved\n");
    RemoveDirectoryA("test6.txt");
    ok(DeleteFileA("test7.txt\\test2.txt"), "The file is not moved\n");
    RemoveDirectoryA("test7.txt");

    init_shfo_tests();
    /* number of sources does not correspond to number of targets,
       files only,
       from exceeds to */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test3.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == DE_SAMEFILE, "got %ld\n", retval);
    ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not moved\n");
    RemoveDirectoryA("test6.txt");
    ok(DeleteFileA("test7.txt\\test2.txt"), "The file is not moved\n");
    RemoveDirectoryA("test7.txt");
    ok(file_exists("test3.txt"), "File should not be moved\n");

    init_shfo_tests();
    /* number of sources does not correspond to number of targets,
       files only,
       too exceeds from */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo2);
    ok(!retval, "got %ld\n", retval);
    ok(DeleteFileA("test6.txt\\test1.txt"),"The file is not moved\n");
    ok(DeleteFileA("test7.txt\\test2.txt"),"The file is not moved\n");
    ok(!dir_exists("test8.txt") && !file_exists("test8.txt"), "Directory should not be created\n");
    RemoveDirectoryA("test6.txt");
    RemoveDirectoryA("test7.txt");

    init_shfo_tests();
    /* number of sources does not correspond to number of targets,
       target directories */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test3.txt\0");
    set_curr_dir_path(to, "test4.txt\0test5.txt\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == DE_SAMEFILE, "got %ld\n", retval);
    ok(DeleteFileA("test4.txt\\test1.txt"),"The file is not moved\n");
    ok(DeleteFileA("test5.txt\\test2.txt"),"The file is not moved\n");
    ok(file_exists("test3.txt"), "The file is not moved\n");
    RemoveDirectoryA("test4.txt");
    RemoveDirectoryA("test5.txt");


    init_shfo_tests();
    /*  0 incoming files */
    set_curr_dir_path(from, "\0\0");
    set_curr_dir_path(to, "test6.txt\0\0");
    retval = SHFileOperationA(&shfo2);
    todo_wine ok(!retval, "got %ld\n", retval);
    ok(!file_exists("test6.txt"), "The file should not exist\n");

    init_shfo_tests();
    /*  0 outgoing files */
    set_curr_dir_path(from, "test1\0\0");
    set_curr_dir_path(to, "\0\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == ERROR_FILE_NOT_FOUND, "got %ld\n", retval);
    ok(!file_exists("test6.txt"), "The file should not exist\n");

    init_shfo_tests();

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Can't move file to other directory\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is not moved\n");

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo);
    todo_wine ok(!retval, "got %ld\n", retval);
    todo_wine ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not moved. Many files are specified\n");
    todo_wine ok(DeleteFileA("test6.txt\\test2.txt"), "The file is not moved. Many files are specified\n");
    todo_wine ok(DeleteFileA("test6.txt\\test4.txt\\test1.txt"), "The file is not moved. Many files are specified\n");
    todo_wine ok(RemoveDirectoryA("test6.txt\\test4.txt"), "The directory is not moved. Many files are specified\n");
    RemoveDirectoryA("test6.txt");
    init_shfo_tests();

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file failed\n");
    ok(!file_exists("test1.txt"), "The file is not moved\n");
    ok(file_exists("test6.txt"), "The file is not moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file back failed\n");

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir failed\n");
    ok(!dir_exists("test4.txt"), "The dir is not moved\n");
    ok(dir_exists("test6.txt"), "The dir is moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir back failed\n");

    /* move one file to two others */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "a.txt\0b.txt\0";
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(DeleteFileA("a.txt"), "Expected a.txt to exist\n");
    ok(!file_exists("test1.txt"), "Expected test1.txt to not exist\n");
    ok(!file_exists("b.txt"), "Expected b.txt to not exist\n");

    /* move two files to one other */
    shfo.pFrom = "test2.txt\0test3.txt\0";
    shfo.pTo = "test1.txt\0";
    retval = SHFileOperationA(&shfo);
    todo_wine ok(!retval, "got %ld\n", retval);
    todo_wine ok(DeleteFileA("test1.txt\\test2.txt"), "Expected test1.txt\\test2.txt to exist\n");
    todo_wine ok(DeleteFileA("test1.txt\\test3.txt"), "Expected test1.txt\\test3.txt to exist\n");
    RemoveDirectoryA("test1.txt");
    createTestFile("test2.txt");
    createTestFile("test3.txt");

    /* move a directory into itself */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\\b.txt\0";
    retval = SHFileOperationA(&shfo);
    todo_wine ok(retval == DE_DESTSUBTREE, "got %ld\n", retval);
    ok(!RemoveDirectoryA("test4.txt\\b.txt"), "Expected test4.txt\\b.txt to not exist\n");
    ok(dir_exists("test4.txt"), "Expected test4.txt to exist\n");

    /* move many files without FOF_MULTIDESTFILES */
    shfo.pFrom = "test2.txt\0test3.txt\0";
    shfo.pTo = "d.txt\0e.txt\0";
    retval = SHFileOperationA(&shfo);
    todo_wine ok(!retval, "got %ld\n", retval);
    todo_wine ok(DeleteFileA("d.txt\\test2.txt"), "Expected d.txt\\test2.txt to exist\n");
    todo_wine ok(DeleteFileA("d.txt\\test3.txt"), "Expected d.txt\\test3.txt to exist\n");
    RemoveDirectoryA("d.txt");
    createTestFile("test2.txt");
    createTestFile("test3.txt");

    /* number of sources != number of targets */
    shfo.pTo = "d.txt\0";
    shfo.fFlags |= FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    ok(retval == DE_SAMEFILE, "got %ld\n", retval);
    ok(DeleteFileA("d.txt\\test2.txt"), "Expected d.txt\\test2.txt to exist\n");
    ok(!file_exists("d.txt\\test3.txt"), "Expected d.txt\\test3.txt to not exist\n");
    RemoveDirectoryA("d.txt");
    createTestFile("test2.txt");

    /* FO_MOVE does not create dest directories */
    shfo.pFrom = "test2.txt\0";
    shfo.pTo = "dir1\\dir2\\test2.txt\0";
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(DeleteFileA("dir1\\dir2\\test2.txt"), "Expected dir1\\dir2\\test2.txt to exist\n");
    RemoveDirectoryA("dir1\\dir2");
    RemoveDirectoryA("dir1");
    createTestFile("test2.txt");

    /* try to overwrite an existing file */
    shfo.pTo = "test3.txt\0";
    retval = SHFileOperationA(&shfo);
    ok(!retval, "got %ld\n", retval);
    ok(!file_exists("test2.txt"), "Expected test2.txt to not exist\n");
    ok(file_exists("test3.txt"), "Expected test3.txt to exist\n");
}

static void test_sh_create_dir(void)
{
    CHAR path[MAX_PATH];
    int ret;

    set_curr_dir_path(path, "testdir2\\test4.txt\0");
    ret = SHCreateDirectoryExA(NULL, path, NULL);
    ok(ERROR_SUCCESS == ret, "SHCreateDirectoryEx failed to create directory recursively, ret = %d\n", ret);
    ok(file_exists("testdir2"), "The first directory is not created\n");
    ok(file_exists("testdir2\\test4.txt"), "The second directory is not created\n");

    ret = SHCreateDirectoryExA(NULL, path, NULL);
    ok(ERROR_ALREADY_EXISTS == ret, "SHCreateDirectoryEx should fail to create existing directory, ret = %d\n", ret);

    ret = SHCreateDirectoryExA(NULL, "c:\\testdir3", NULL);
    ok(ERROR_SUCCESS == ret, "SHCreateDirectoryEx failed to create directory, ret = %d\n", ret);
    ok(file_exists("c:\\testdir3"), "The directory is not created\n");
}

static void test_sh_path_prepare(void)
{
    HRESULT res;
    CHAR path[MAX_PATH];
    CHAR UNICODE_PATH_A[MAX_PATH];
    BOOL UsedDefaultChar;

    /* directory exists, SHPPFW_NONE */
    set_curr_dir_path(path, "testdir2\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);

    /* directory exists, SHPPFW_IGNOREFILENAME */
    set_curr_dir_path(path, "testdir2\\test4.txt\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);

    /* directory exists, SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "testdir2\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);

    /* directory exists, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "testdir2\\test4.txt\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);
    ok(!file_exists("nonexistent\\"), "nonexistent\\ exists but shouldn't\n");

    /* file exists, SHPPFW_NONE */
    set_curr_dir_path(path, "test1.txt\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_DIRECTORY) ||
       res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* WinMe */
       res == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), /* Vista */
       "Unexpected result : 0x%08lx\n", res);

    /* file exists, SHPPFW_DIRCREATE */
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_DIRCREATE);
    ok(res == HRESULT_FROM_WIN32(ERROR_DIRECTORY) ||
       res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* WinMe */
       res == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), /* Vista */
       "Unexpected result : 0x%08lx\n", res);

    /* file exists, SHPPFW_NONE, trailing \ */
    set_curr_dir_path(path, "test1.txt\\\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_DIRECTORY) ||
       res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* WinMe */
       res == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), /* Vista */
       "Unexpected result : 0x%08lx\n", res);

    /* relative path exists, SHPPFW_DIRCREATE */
    res = SHPathPrepareForWriteA(0, 0, ".\\testdir2", SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);

    /* relative path doesn't exist, SHPPFW_DIRCREATE -- Windows does not create the directory in this case */
    res = SHPathPrepareForWriteA(0, 0, ".\\testdir2\\test4.txt", SHPPFW_DIRCREATE);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == 0x%08lx, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);
    ok(!file_exists(".\\testdir2\\test4.txt\\"), ".\\testdir2\\test4.txt\\ exists but shouldn't\n");

    /* directory doesn't exist, SHPPFW_NONE */
    set_curr_dir_path(path, "nonexistent\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == 0x%08lx, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);

    /* directory doesn't exist, SHPPFW_IGNOREFILENAME */
    set_curr_dir_path(path, "nonexistent\\notreal\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == 0x%08lx, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);
    ok(!file_exists("nonexistent\\notreal"), "nonexistent\\notreal exists but shouldn't\n");
    ok(!file_exists("nonexistent\\"), "nonexistent\\ exists but shouldn't\n");

    /* directory doesn't exist, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "testdir2\\test4.txt\\\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);
    ok(file_exists("testdir2\\test4.txt\\"), "testdir2\\test4.txt doesn't exist but should\n");

    /* nested directory doesn't exist, SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "nonexistent\\notreal\0");
    res = SHPathPrepareForWriteA(0, 0, path, SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08lx, expected S_OK\n", res);
    ok(file_exists("nonexistent\\notreal"), "nonexistent\\notreal doesn't exist but should\n");

    /* SHPPFW_ASKDIRCREATE, SHPPFW_NOWRITECHECK, and SHPPFW_MEDIACHECKONLY are untested */

    SetLastError(0xdeadbeef);
    UsedDefaultChar = FALSE;
    if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, UNICODE_PATH, -1, UNICODE_PATH_A, sizeof(UNICODE_PATH_A), NULL, &UsedDefaultChar) == 0)
    {
        win_skip("Could not convert Unicode path name to multibyte (%ld)\n", GetLastError());
        return;
    }
    if (UsedDefaultChar)
    {
        win_skip("Could not find unique multibyte representation for directory name using default codepage\n");
        return;
    }

    /* unicode directory doesn't exist, SHPPFW_NONE */
    RemoveDirectoryA(UNICODE_PATH_A);
    res = SHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == %08lx, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);
    ok(!file_exists(UNICODE_PATH_A), "unicode path was created but shouldn't be\n");
    RemoveDirectoryA(UNICODE_PATH_A);

    /* unicode directory doesn't exist, SHPPFW_DIRCREATE */
    res = SHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == %08lx, expected S_OK\n", res);
    ok(file_exists(UNICODE_PATH_A), "unicode path should've been created\n");

    /* unicode directory exists, SHPPFW_NONE */
    res = SHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_NONE);
    ok(res == S_OK, "ret == %08lx, expected S_OK\n", res);

    /* unicode directory exists, SHPPFW_DIRCREATE */
    res = SHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_DIRCREATE);
    ok(res == S_OK, "ret == %08lx, expected S_OK\n", res);
    RemoveDirectoryA(UNICODE_PATH_A);
}

static void test_sh_new_link_info(void)
{
    BOOL ret, mustcopy=TRUE;
    CHAR linkto[MAX_PATH];
    CHAR destdir[MAX_PATH];
    CHAR result[MAX_PATH];
    CHAR result2[MAX_PATH];

    /* source file does not exist */
    set_curr_dir_path(linkto, "nosuchfile.txt\0");
    set_curr_dir_path(destdir, "testdir2\0");
    ret = SHGetNewLinkInfoA(linkto, destdir, result, &mustcopy, 0);
    ok(ret == FALSE ||
       broken(ret == lstrlenA(result) + 1), /* NT4 */
       "SHGetNewLinkInfoA succeeded\n");
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");

    /* dest dir does not exist */
    set_curr_dir_path(linkto, "test1.txt\0");
    set_curr_dir_path(destdir, "nosuchdir\0");
    ret = SHGetNewLinkInfoA(linkto, destdir, result, &mustcopy, 0);
    ok(ret == TRUE ||
       broken(ret == lstrlenA(result) + 1), /* NT4 */
       "SHGetNewLinkInfoA failed, err=%li\n", GetLastError());
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");

    /* source file exists */
    set_curr_dir_path(linkto, "test1.txt\0");
    set_curr_dir_path(destdir, "testdir2\0");
    ret = SHGetNewLinkInfoA(linkto, destdir, result, &mustcopy, 0);
    ok(ret == TRUE ||
       broken(ret == lstrlenA(result) + 1), /* NT4 */
       "SHGetNewLinkInfoA failed, err=%li\n", GetLastError());
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");
    ok(CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, destdir,
                      lstrlenA(destdir), result, lstrlenA(destdir)) == CSTR_EQUAL,
       "%s does not start with %s\n", result, destdir);
    ok(lstrlenA(result) > 4 && lstrcmpiA(result+lstrlenA(result)-4, ".lnk") == 0,
       "%s does not end with .lnk\n", result);

    /* preferred target name already exists */
    createTestFile(result);
    ret = SHGetNewLinkInfoA(linkto, destdir, result2, &mustcopy, 0);
    ok(ret == TRUE ||
       broken(ret == lstrlenA(result2) + 1), /* NT4 */
       "SHGetNewLinkInfoA failed, err=%li\n", GetLastError());
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");
    ok(CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, destdir,
                      lstrlenA(destdir), result2, lstrlenA(destdir)) == CSTR_EQUAL,
       "%s does not start with %s\n", result2, destdir);
    ok(lstrlenA(result2) > 4 && lstrcmpiA(result2+lstrlenA(result2)-4, ".lnk") == 0,
       "%s does not end with .lnk\n", result2);
    ok(lstrcmpiA(result, result2) != 0, "%s and %s are the same\n", result, result2);
    DeleteFileA(result);
}

static void test_unicode(void)
{
    SHFILEOPSTRUCTW shfoW;
    int ret;
    HANDLE file;
    static const WCHAR UNICODE_PATH_TO[] = {'c',':','\\',0x00ae,0x00ae,'\0'};
    HWND hwnd;

    shfoW.hwnd = NULL;
    shfoW.wFunc = FO_DELETE;
    shfoW.pFrom = UNICODE_PATH;
    shfoW.pTo = NULL;
    shfoW.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfoW.hNameMappings = NULL;
    shfoW.lpszProgressTitle = NULL;

    /* Clean up before start test */
    DeleteFileW(UNICODE_PATH);
    RemoveDirectoryW(UNICODE_PATH);

    /* Make sure we are on a system that supports unicode */
    SetLastError(0xdeadbeef);
    file = CreateFileW(UNICODE_PATH, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("Unicode tests skipped on non-unicode system\n");
        return;
    }
    if (GetLastError()==ERROR_ACCESS_DENIED)
    {
        skip("test needs admin rights\n");
        return;
    }
    CloseHandle(file);

    /* Try to delete a file with unicode filename */
    ok(file_existsW(UNICODE_PATH), "The file does not exist\n");
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "File is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The file should have been removed\n");

    /* Try to trash a file with unicode filename */
    createTestFileW(UNICODE_PATH);
    shfoW.fFlags |= FOF_ALLOWUNDO;
    ok(file_existsW(UNICODE_PATH), "The file does not exist\n");
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "File is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The file should have been removed\n");

    /* Try to delete a directory with unicode filename */
    ret = SHCreateDirectoryExW(NULL, UNICODE_PATH, NULL);
    ok(!ret, "SHCreateDirectoryExW returned %d\n", ret);
    ok(file_existsW(UNICODE_PATH), "The directory is not created\n");
    shfoW.fFlags &= ~FOF_ALLOWUNDO;
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "Directory is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The directory should have been removed\n");

    /* Try to trash a directory with unicode filename */
    ret = SHCreateDirectoryExW(NULL, UNICODE_PATH, NULL);
    ok(!ret, "SHCreateDirectoryExW returned %d\n", ret);
    ok(file_existsW(UNICODE_PATH), "The directory was not created\n");
    shfoW.fFlags |= FOF_ALLOWUNDO;
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "Directory is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The directory should have been removed\n");

    shfoW.hwnd = NULL;
    shfoW.wFunc = FO_COPY;
    shfoW.pFrom = UNICODE_PATH;
    shfoW.pTo = UNICODE_PATH_TO;
    shfoW.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfoW.hNameMappings = NULL;
    shfoW.lpszProgressTitle = NULL;

    /* Check last error after a successful file operation. */
    createTestFileW(UNICODE_PATH);
    ok(file_existsW(UNICODE_PATH), "The file does not exist\n");
    SetLastError(0xdeadbeef);
    ret = SHFileOperationW(&shfoW);
    ok(ret == ERROR_SUCCESS, "File copy failed with %d\n", ret);
    ok(!shfoW.fAnyOperationsAborted, "Didn't expect aborted operations\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == ERROR_INVALID_HANDLE), /* WinXp, win2k3 */
       "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    DeleteFileW(UNICODE_PATH_TO);

    /* Check last error after a failed file operation. */
    DeleteFileW(UNICODE_PATH);
    ok(!file_existsW(UNICODE_PATH), "The file should have been removed\n");
    SetLastError(0xdeadbeef);
    ret = SHFileOperationW(&shfoW);
    ok(ret != ERROR_SUCCESS, "Unexpected ERROR_SUCCESS\n");
    ok(!shfoW.fAnyOperationsAborted, "Didn't expect aborted operations\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == ERROR_INVALID_HANDLE), /* WinXp, win2k3 */
       "Expected ERROR_SUCCESS, got %ld\n", GetLastError());

    /* Check SHCreateDirectoryExW with a Hwnd
     * returns ERROR_ALREADY_EXISTS where a directory already exists */
    /* Get any window handle */
    hwnd = FindWindowA(NULL, NULL);
    ok(hwnd != 0, "FindWindowA failed to produce a hwnd\n");
    ret = SHCreateDirectoryExW(hwnd, UNICODE_PATH, NULL);
    ok(!ret, "SHCreateDirectoryExW returned %d\n", ret);
    /* Create already-existing directory */
    ok(file_existsW(UNICODE_PATH), "The directory was not created\n");
    ret = SHCreateDirectoryExW(hwnd, UNICODE_PATH, NULL);
    ok(ret == ERROR_ALREADY_EXISTS,
       "Expected ERROR_ALREADY_EXISTS, got %d\n", ret);
    RemoveDirectoryW(UNICODE_PATH);
}

static void
test_shlmenu(void) {
	HRESULT hres;
	HMENU src_menu, dst_menu;
	int count;
	MENUITEMINFOA item_info;
	BOOL bres;

	hres = Shell_MergeMenus (0, 0, 0x42, 0x4242, 0x424242, 0);
	ok (hres == 0x4242, "expected 0x4242 but got %lx\n", hres);
	hres = Shell_MergeMenus ((HMENU)42, 0, 0x42, 0x4242, 0x424242, 0);
	ok (hres == 0x4242, "expected 0x4242 but got %lx\n", hres);

	src_menu = CreatePopupMenu ();
	ok (src_menu != NULL, "CreatePopupMenu() failed, error %ld\n", GetLastError ());

	dst_menu = CreatePopupMenu ();
	ok (dst_menu != NULL, "CreatePopupMenu() failed, error %ld\n", GetLastError ());
	bres = InsertMenuA (src_menu, -1, MF_BYPOSITION | MF_STRING, 10, "item1");
        ok (bres, "InsertMenuA failed, error %ld\n", GetLastError());
	bres = InsertMenuA (src_menu, -1, MF_BYPOSITION | MF_STRING, 11, "item2");
        ok (bres, "InsertMenuA failed, error %ld\n", GetLastError());
	hres = Shell_MergeMenus (dst_menu, src_menu, 0, 123, 133, MM_SUBMENUSHAVEIDS);
	ok (hres == 134, "got %ld\n", hres);
	count = GetMenuItemCount (dst_menu);
	ok (count == 1, "got %d\n", count);
	memset (&item_info, 0, sizeof(item_info));
	item_info.cbSize = sizeof(item_info);
	item_info.fMask = MIIM_ID;
	bres = GetMenuItemInfoA (dst_menu, 0, TRUE, &item_info);
	ok (bres, "GetMenuItemInfoA failed, error %ld\n", GetLastError ());
	ok (item_info.wID == 133, "got %d\n", item_info.wID);
	DestroyMenu (dst_menu);

	/* integer overflow: Shell_MergeMenus() return value is wrong, but items are still added */
	dst_menu = CreatePopupMenu ();
	ok (dst_menu != NULL, "CreatePopupMenu() failed, error %ld\n", GetLastError ());
	hres = Shell_MergeMenus (dst_menu, src_menu, 0, -1, 133, MM_SUBMENUSHAVEIDS);
	ok (hres == -1, "got %ld\n", hres);
	count = GetMenuItemCount (dst_menu);
	ok (count == 2, "got %d\n", count);
	DestroyMenu (dst_menu);

	DestroyMenu (src_menu);
}

/* Check for old shell32 (4.0.x) */
static BOOL is_old_shell32(void)
{
    SHFILEOPSTRUCTA shfo;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    DWORD retval;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_COPY;
    shfo.pFrom = from;
    shfo.pTo = to;
    /* FOF_NOCONFIRMMKDIR is needed for old shell32 */
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test3.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo);

    /* Delete extra files on old shell32 and Vista+*/
    DeleteFileA("test6.txt\\test1.txt");
    /* Delete extra files on old shell32 */
    DeleteFileA("test6.txt\\test2.txt");
    DeleteFileA("test6.txt\\test3.txt");
    /* Delete extra directory on old shell32 and Vista+ */
    RemoveDirectoryA("test6.txt");
    /* Delete extra files/directories on Vista+*/
    DeleteFileA("test7.txt\\test2.txt");
    RemoveDirectoryA("test7.txt");

    if (retval == ERROR_SUCCESS)
        return TRUE;

    return FALSE;
}

struct progress_sink
{
    IFileOperationProgressSink IFileOperationProgressSink_iface;
    unsigned int instance_id;
    LONG ref;
    struct progress_expected_notifications *expected;
};

struct progress_expected_notification
{
    const char *text;
    DWORD tsf, tsf_broken;
    HRESULT hres, hres_broken;
};
struct progress_expected_notifications
{
    unsigned int index, count, line;
    const WCHAR *dir_prefix;
    const struct progress_expected_notification *expected;
};

static inline struct progress_sink *impl_from_IFileOperationProgressSink(IFileOperationProgressSink *iface)
{
    return CONTAINING_RECORD(iface, struct progress_sink, IFileOperationProgressSink_iface);
}

#define progress_init_check_notifications(a, b, c, d, e) progress_init_check_notifications_(__LINE__, a, b, c, d, e)
static void progress_init_check_notifications_(unsigned int line, IFileOperationProgressSink *iface,
        unsigned int count, const struct progress_expected_notification *expected, const WCHAR *dir_prefix,
        struct progress_expected_notifications *n)
{
    struct progress_sink *progress = impl_from_IFileOperationProgressSink(iface);
    n->line = line;
    n->index = 0;
    n->count = count;
    n->expected = expected;
    n->dir_prefix = dir_prefix;
    progress->expected = n;
}

static void progress_check_notification(struct progress_sink *progress, const char *text, DWORD tsf, HRESULT hres)
{
    struct progress_expected_notifications *e = progress->expected;
    char str[4096];

    ok(!!e, "expected notifications are not set up.\n");
    sprintf(str, "[%u] %s", progress->instance_id, text);
    ok_(__FILE__, e->line)(e->index < e->count, "extra notification %s.\n", debugstr_a(str));
    if (e->index < e->count)
    {
        ok_(__FILE__, e->line)(!strcmp(str, e->expected[e->index].text), "got notification %s, expected %s, index %u.\n",
                debugstr_a(str), debugstr_a(e->expected[e->index].text), e->index);
        ok_(__FILE__, e->line)(tsf == e->expected[e->index].tsf || broken(tsf == e->expected[e->index].tsf_broken),
                "got tsf %#lx, expected %#lx, index %u (%s).\n", tsf, e->expected[e->index].tsf, e->index, debugstr_a(text));
        ok_(__FILE__, e->line)(hres == e->expected[e->index].hres || broken(hres == e->expected[e->index].hres_broken),
                "got hres %#lx, expected %#lx, index %u (%s).\n", hres, e->expected[e->index].hres, e->index, debugstr_a(text));
    }
    ++e->index;
}

static void progress_end_check_notifications(IFileOperationProgressSink *iface)
{
    struct progress_sink *progress = impl_from_IFileOperationProgressSink(iface);
    struct progress_expected_notifications *e = progress->expected;

    ok(!!e, "expected notifications are not set up.\n");
    ok_(__FILE__, e->line)(e->index == e->count, "got notification count %u, expected %u.\n", e->index, e->count);
    progress->expected = NULL;
}

static const char *shellitem_str(struct progress_sink *progress, IShellItem *item)
{
    char str[MAX_PATH];
    unsigned int len;
    const char *ret;
    WCHAR *path;
    HRESULT hr;

    if (!item)
        return "<null>";

    hr = IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path);
    if (FAILED(hr))
        return "<invalid>";

    len = wcslen(progress->expected->dir_prefix);
    if (progress->expected->dir_prefix[len - 1] == '\\')
        --len;
    if (wcsncmp(path, progress->expected->dir_prefix, len))
    {
        ret = debugstr_w(path);
    }
    else
    {
        sprintf(str, "<prefix>%s", debugstr_w(path + len) + 1);
        ret = __wine_dbg_strdup(str);
    }
    CoTaskMemFree(path);
    return ret;
}

static HRESULT WINAPI progress_QueryInterface(IFileOperationProgressSink *iface, REFIID riid, void **out)
{
    struct progress_sink *operation = impl_from_IFileOperationProgressSink(iface);

    if (IsEqualIID(&IID_IFileOperationProgressSink, riid) ||  IsEqualIID(&IID_IUnknown, riid))
        *out = &operation->IFileOperationProgressSink_iface;
    else
    {
        trace("not implemented for %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI progress_AddRef(IFileOperationProgressSink *iface)
{
    struct progress_sink *progress = impl_from_IFileOperationProgressSink(iface);

    return InterlockedIncrement(&progress->ref);
}

static ULONG WINAPI progress_Release(IFileOperationProgressSink *iface)
{
    struct progress_sink *progress = impl_from_IFileOperationProgressSink(iface);
    LONG ref = InterlockedDecrement(&progress->ref);

    if (!ref)
        free(progress);

    return ref;
}

static HRESULT WINAPI progress_StartOperations(IFileOperationProgressSink *iface)
{
    progress_check_notification(impl_from_IFileOperationProgressSink(iface), "StartOperations", 0, 0);
    return S_OK;
}

static HRESULT WINAPI progress_FinishOperations(IFileOperationProgressSink *iface, HRESULT result)
{
    progress_check_notification(impl_from_IFileOperationProgressSink(iface), "FinishOperations", 0, result);
    return S_OK;
}

static HRESULT WINAPI progress_PreRenameItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        const WCHAR *new_name)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PostRenameItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        const WCHAR *new_name, HRESULT hrRename, IShellItem *newly_created)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PreMoveItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        IShellItem *dest_folder, const WCHAR *new_name)
{
    struct progress_sink *progress = impl_from_IFileOperationProgressSink(iface);
    char str[1024];

    sprintf(str, "PreMoveItem %s, %s, %s", shellitem_str(progress, item),
            shellitem_str(progress, dest_folder), debugstr_w(new_name) + 1);
    progress_check_notification(progress, str, flags, 0);
    return S_OK;
}

static HRESULT WINAPI progress_PostMoveItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        IShellItem *dest_folder, const WCHAR *new_name, HRESULT result, IShellItem *newly_created)
{
    struct progress_sink *progress = impl_from_IFileOperationProgressSink(iface);
    char str[1024];

    sprintf(str, "PostMoveItem %s, %s, %s -> %s", shellitem_str(progress, item),
            shellitem_str(progress, dest_folder), debugstr_w(new_name) + 1, shellitem_str(progress, newly_created));
    progress_check_notification(progress, str, flags, result);
    return S_OK;
}

static HRESULT WINAPI progress_PreCopyItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        IShellItem *dest_folder,LPCWSTR pszNewName)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PostCopyItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        IShellItem *dest_folder, const WCHAR *new_name, HRESULT result, IShellItem *newly_created)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PreDeleteItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PostDeleteItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *item,
        HRESULT result, IShellItem *newly_created)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PreNewItem(IFileOperationProgressSink *iface, DWORD flags,IShellItem *dest_folder,
        const WCHAR *new_name)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_PostNewItem(IFileOperationProgressSink *iface, DWORD flags, IShellItem *dest_folder,
        const WCHAR *new_name, const WCHAR *template_name, DWORD file_attrs, HRESULT result, IShellItem *newly_created)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_UpdateProgress(IFileOperationProgressSink *iface, UINT total, UINT sofar)
{
    return S_OK;
}

static HRESULT WINAPI progress_ResetTimer(IFileOperationProgressSink *iface)
{
    progress_check_notification(impl_from_IFileOperationProgressSink(iface), "ResetTimer", 0, 0);
    return S_OK;
}

static HRESULT WINAPI progress_PauseTimer(IFileOperationProgressSink *iface)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI progress_ResumeTimer(IFileOperationProgressSink *iface)
{
    ok(0, ".\n");

    return E_NOTIMPL;
}

static const IFileOperationProgressSinkVtbl progress_vtbl =
{
    progress_QueryInterface,
    progress_AddRef,
    progress_Release,
    progress_StartOperations,
    progress_FinishOperations,
    progress_PreRenameItem,
    progress_PostRenameItem,
    progress_PreMoveItem,
    progress_PostMoveItem,
    progress_PreCopyItem,
    progress_PostCopyItem,
    progress_PreDeleteItem,
    progress_PostDeleteItem,
    progress_PreNewItem,
    progress_PostNewItem,
    progress_UpdateProgress,
    progress_ResetTimer,
    progress_PauseTimer,
    progress_ResumeTimer,
};

static IFileOperationProgressSink *create_progress_sink(unsigned int instance_id)
{
    struct progress_sink *obj;

    obj = calloc(1, sizeof(*obj));
    obj->IFileOperationProgressSink_iface.lpVtbl = &progress_vtbl;
    obj->instance_id = instance_id;
    obj->ref = 1;
    return &obj->IFileOperationProgressSink_iface;
}

static void set_shell_item_path(IShellItem *item, const WCHAR *path)
{
    IPersistIDList *idlist;
    ITEMIDLIST *pidl;
    HRESULT hr;

    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    if (FAILED(hr))
        return;
    hr = IShellItem_QueryInterface(item, &IID_IPersistIDList, (void **)&idlist);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IPersistIDList_SetIDList(idlist, pidl);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IPersistIDList_Release(idlist);
    ILFree(pidl);
}

static void test_file_operation(void)
{
#define DEFAULT_TSF_FLAGS (TSF_COPY_LOCALIZED_NAME | TSF_COPY_WRITE_TIME | TSF_COPY_CREATION_TIME | TSF_OVERWRITE_EXIST)
#define MEGRE_TSF_FLAGS (TSF_COPY_WRITE_TIME | TSF_COPY_CREATION_TIME | TSF_OVERWRITE_EXIST)
#define UNKNOWN_POST_MERGE_TSF_FLAG 0x1000
    static const struct progress_expected_notification notifications1[] =
    {
        {"[0] StartOperations"},
        {"[0] ResetTimer"},
        {"[0] PreMoveItem <prefix>\"\\\\testfile1\", <prefix>\"\", \"test\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[1] PreMoveItem <prefix>\"\\\\testfile1\", <prefix>\"\", \"test\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[1] PostMoveItem <prefix>\"\\\\testfile1\", <prefix>\"\", \"test\" -> <prefix>\"\\\\test\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PostMoveItem <prefix>\"\\\\testfile1\", <prefix>\"\", \"test\" -> <prefix>\"\\\\test\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] FinishOperations"},
    };
    static const struct progress_expected_notification notifications2[] =
    {
        {"[0] StartOperations"},
        {"[0] ResetTimer"},
        {"[0] PreMoveItem <prefix>\"\\\\testfile1\", <prefix>\"\", \"test2\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[0] PostMoveItem <prefix>\"\\\\testfile1\", <prefix>\"\", \"test2\" -> <null>",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                0x80070002, COPYENGINE_S_USER_IGNORED},
        {"[0] FinishOperations"},
    };
    static const struct progress_expected_notification notifications3[] =
    {
        {"[0] StartOperations"},
        {"[0] ResetTimer"},
        {"[0] PreMoveItem <prefix>\"\\\\test\", <prefix>\"\", \"test2\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[0] PostMoveItem <prefix>\"\\\\test\", <prefix>\"\", \"test2\" -> <prefix>\"\\\\test2\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] FinishOperations"},
    };
    static const struct progress_expected_notification notifications4[] =
    {
        {"[0] StartOperations"},
        {"[0] ResetTimer"},
        {"[0] PreMoveItem <prefix>\"\\\\test_dir1\", <prefix>\"\\\\test_dir2\", \"test2\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[0] PostMoveItem <prefix>\"\\\\test_dir1\", <prefix>\"\\\\test_dir2\", \"test2\" -> <null>",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_E_FLD_IS_FILE_DEST, COPYENGINE_S_USER_IGNORED},
        {"[0] FinishOperations"},
    };
    static const struct progress_expected_notification notifications5[] =
    {
        {"[0] StartOperations"},
        {"[0] ResetTimer"},
        {"[0] PreMoveItem <prefix>\"\\\\test_dir1\", <prefix>\"\\\\test_dir2\", \"test2\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[0] PostMoveItem <prefix>\"\\\\test_dir1\", <prefix>\"\\\\test_dir2\", \"test2\" -> <prefix>\"\\\\test_dir2\\\\test2\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] FinishOperations"},
    };
    static const struct progress_expected_notification notifications6[] =
    {
        {"[0] StartOperations"},
        {"[0] ResetTimer"},
        {"[0] PreMoveItem <prefix>\"\\\\test_dir3\", <prefix>\"\\\\test_dir2\", \"test2\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[1] PreMoveItem <prefix>\"\\\\test_dir3\", <prefix>\"\\\\test_dir2\", \"test2\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[1] PostMoveItem <prefix>\"\\\\test_dir3\", <prefix>\"\\\\test_dir2\", \"test2\" -> <prefix>\"\\\\test_dir2\\\\test2\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_NOT_HANDLED, COPYENGINE_S_MERGE},
        {"[0] PostMoveItem <prefix>\"\\\\test_dir3\", <prefix>\"\\\\test_dir2\", \"test2\" -> <prefix>\"\\\\test_dir2\\\\test2\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_NOT_HANDLED, COPYENGINE_S_MERGE},
        {"[0] PreMoveItem <prefix>\"\\\\test_dir3\\\\testfile5\", <prefix>\"\\\\test_dir2\\\\test2\", \"\"", MEGRE_TSF_FLAGS, MEGRE_TSF_FLAGS},
        {"[1] PreMoveItem <prefix>\"\\\\test_dir3\\\\testfile5\", <prefix>\"\\\\test_dir2\\\\test2\", \"\"", MEGRE_TSF_FLAGS, MEGRE_TSF_FLAGS},
        {"[1] PostMoveItem <prefix>\"\\\\test_dir3\\\\testfile5\", <prefix>\"\\\\test_dir2\\\\test2\", \"testfile5\" -> <prefix>\"\\\\test_dir2\\\\test2\\\\testfile5\"",
                MEGRE_TSF_FLAGS | UNKNOWN_POST_MERGE_TSF_FLAG, MEGRE_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PostMoveItem <prefix>\"\\\\test_dir3\\\\testfile5\", <prefix>\"\\\\test_dir2\\\\test2\", \"testfile5\" -> <prefix>\"\\\\test_dir2\\\\test2\\\\testfile5\"",
                MEGRE_TSF_FLAGS | UNKNOWN_POST_MERGE_TSF_FLAG, MEGRE_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PreMoveItem <prefix>\"\\\\test_dir3\\\\testfile6\", <prefix>\"\\\\test_dir2\\\\test2\", \"\"", MEGRE_TSF_FLAGS, MEGRE_TSF_FLAGS},
        {"[1] PreMoveItem <prefix>\"\\\\test_dir3\\\\testfile6\", <prefix>\"\\\\test_dir2\\\\test2\", \"\"", MEGRE_TSF_FLAGS, MEGRE_TSF_FLAGS},
        {"[1] PostMoveItem <prefix>\"\\\\test_dir3\\\\testfile6\", <prefix>\"\\\\test_dir2\\\\test2\", \"testfile6\" -> <prefix>\"\\\\test_dir2\\\\test2\\\\testfile6\"",
                MEGRE_TSF_FLAGS | UNKNOWN_POST_MERGE_TSF_FLAG, MEGRE_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PostMoveItem <prefix>\"\\\\test_dir3\\\\testfile6\", <prefix>\"\\\\test_dir2\\\\test2\", \"testfile6\" -> <prefix>\"\\\\test_dir2\\\\test2\\\\testfile6\"",
                MEGRE_TSF_FLAGS | UNKNOWN_POST_MERGE_TSF_FLAG, MEGRE_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PreMoveItem <prefix>\"\\\\test_dir3\\\\inner_dir\", <prefix>\"\\\\test_dir2\\\\test2\", \"\"", MEGRE_TSF_FLAGS, MEGRE_TSF_FLAGS},
        {"[1] PreMoveItem <prefix>\"\\\\test_dir3\\\\inner_dir\", <prefix>\"\\\\test_dir2\\\\test2\", \"\"", MEGRE_TSF_FLAGS, MEGRE_TSF_FLAGS},
        {"[1] PostMoveItem <prefix>\"\\\\test_dir3\\\\inner_dir\", <prefix>\"\\\\test_dir2\\\\test2\", \"inner_dir\" -> <prefix>\"\\\\test_dir2\\\\test2\\\\inner_dir\"",
                MEGRE_TSF_FLAGS | UNKNOWN_POST_MERGE_TSF_FLAG, MEGRE_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PostMoveItem <prefix>\"\\\\test_dir3\\\\inner_dir\", <prefix>\"\\\\test_dir2\\\\test2\", \"inner_dir\" -> <prefix>\"\\\\test_dir2\\\\test2\\\\inner_dir\"",
                MEGRE_TSF_FLAGS | UNKNOWN_POST_MERGE_TSF_FLAG, MEGRE_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] PreMoveItem <prefix>\"\\\\testfile8\", <prefix>\"\\\\test_dir2\", \"\"", DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS},
        {"[0] PostMoveItem <prefix>\"\\\\testfile8\", <prefix>\"\\\\test_dir2\", \"testfile8\" -> <prefix>\"\\\\test_dir2\\\\testfile8\"",
                DEFAULT_TSF_FLAGS, DEFAULT_TSF_FLAGS,
                COPYENGINE_S_DONT_PROCESS_CHILDREN, COPYENGINE_S_DONT_PROCESS_CHILDREN},
        {"[0] FinishOperations"},
    };

    WCHAR dirpath[MAX_PATH], tmpfile[MAX_PATH], path[MAX_PATH];
    struct progress_expected_notifications expected_notif;
    IFileOperationProgressSink *progress, *progress2;
    IFileOperation *operation;
    IShellItem *item, *item2;
    IShellItem *folder;
    LONG refcount;
    IUnknown *unk;
    DWORD cookie;
    BOOL aborted;
    HRESULT hr;
    BOOL bret;
    DWORD ret;

    hr = CoCreateInstance(&CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFileOperation, (void **)&operation);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* before vista */,
        "Got hr %#lx.\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("IFileOperation isn't supported.\n");
        return;
    }

    hr = IFileOperation_QueryInterface(operation, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IFileOperation_Advise(operation, NULL, &cookie);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    progress = create_progress_sink(0);
    hr = IFileOperation_Advise(operation, progress, &cookie);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IFileOperation_PerformOperations(operation);
    ok(hr == E_UNEXPECTED, "got %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_ShellItem, NULL, CLSCTX_INPROC_SERVER, &IID_IShellItem, (void **)&item);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = CoCreateInstance(&CLSID_ShellItem, NULL, CLSCTX_INPROC_SERVER, &IID_IShellItem, (void **)&folder);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IFileOperation_MoveItem(operation, item, folder, L"test", NULL);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    GetTempPathW(ARRAY_SIZE(dirpath), dirpath);
    PathCombineW(tmpfile, dirpath, L"testfile1");
    createTestFileW(tmpfile);

    set_shell_item_path(folder, dirpath);
    set_shell_item_path(item, tmpfile);

    hr = IFileOperation_SetOperationFlags(operation, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    progress2 = create_progress_sink(1);
    progress_init_check_notifications(progress, ARRAY_SIZE(notifications1), notifications1, dirpath, &expected_notif);
    progress_init_check_notifications(progress2, ARRAY_SIZE(notifications1), notifications1, dirpath, &expected_notif);
    hr = IFileOperation_MoveItem(operation, item, folder, L"test", progress2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_SetOperationFlags(operation, FOF_NO_UI);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_PerformOperations(operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    aborted = 0xdeadbeef;
    hr = IFileOperation_GetAnyOperationsAborted(operation, &aborted);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(!aborted, "got %d.\n", aborted);
    progress_end_check_notifications(progress);

    hr = IFileOperation_PerformOperations(operation);
    ok(hr == E_UNEXPECTED, "got %#lx.\n", hr);

    /* Input file does not exist: PerformOperations succeeds, 'aborted' is set. */
    hr = IFileOperation_MoveItem(operation, item, folder, L"test2", NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_init_check_notifications(progress, ARRAY_SIZE(notifications2), notifications2, dirpath, &expected_notif);
    hr = IFileOperation_PerformOperations(operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_end_check_notifications(progress);
    aborted = 0;
    hr = IFileOperation_GetAnyOperationsAborted(operation, &aborted);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(aborted == TRUE, "got %d.\n", aborted);
    aborted = 0;
    hr = IFileOperation_GetAnyOperationsAborted(operation, &aborted);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(aborted == TRUE, "got %d.\n", aborted);

    /* Input file exists: PerformOperations succeeds, the item data at the moment of MoveItem is used. */
    PathCombineW(path, dirpath, L"test");
    set_shell_item_path(item, path);
    hr = IFileOperation_MoveItem(operation, item, folder, L"test2", NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    PathCombineW(tmpfile, dirpath, L"testfile2");
    /* Actual paths are fetched at _MoveItem and not at _Perform operation: changing item after doesn't matter. */
    createTestFileW(tmpfile);
    set_shell_item_path(item, tmpfile);
    bret = DeleteFileW(tmpfile);
    ok(bret, "got error %ld.\n", GetLastError());
    progress_init_check_notifications(progress, ARRAY_SIZE(notifications3), notifications3, dirpath, &expected_notif);
    hr = IFileOperation_PerformOperations(operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_end_check_notifications(progress);
    aborted = 0;
    hr = IFileOperation_GetAnyOperationsAborted(operation, &aborted);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(aborted == TRUE, "got %d.\n", aborted);
    ret = GetFileAttributesW(tmpfile);
    ok(ret == INVALID_FILE_ATTRIBUTES, "got %#lx.\n", ret);
    PathCombineW(path, dirpath, L"test");
    ret = GetFileAttributesW(path);
    ok(ret == INVALID_FILE_ATTRIBUTES, "got %#lx.\n", ret);
    PathCombineW(path, dirpath, L"test2");
    ret = GetFileAttributesW(path);
    ok(ret != INVALID_FILE_ATTRIBUTES, "got %#lx.\n", ret);
    bret = DeleteFileW(path);
    ok(bret, "got error %ld.\n", GetLastError());

    refcount = IShellItem_Release(item);
    ok(!refcount, "got %ld.\n", refcount);
    IShellItem_AddRef(folder);
    refcount = IShellItem_Release(folder);
    todo_wine ok(refcount > 1, "got %ld.\n", refcount);

    hr = IFileOperation_Unadvise(operation, 0xdeadbeef);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_Unadvise(operation, cookie);
    ok(hr == S_OK, "got %#lx.\n", hr);

    IFileOperation_Release(operation);
    refcount = IShellItem_Release(folder);
    ok(!refcount, "got %ld.\n", refcount);

    /* Move directory to directory. */
    hr = CoCreateInstance(&CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOperation, (void **)&operation);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IFileOperation_SetOperationFlags(operation, FOF_NO_UI);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IFileOperation_Advise(operation, progress, &cookie);
    ok(hr == S_OK, "got %#lx.\n", hr);

    PathCombineW(path, dirpath, L"test_dir1");
    bret = CreateDirectoryW(path, NULL);
    PathCombineW(tmpfile, path, L"testfile3");
    createTestFileW(tmpfile);
    PathCombineW(tmpfile, path, L"testfile4");
    createTestFileW(tmpfile);
    hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void**)&item);
    ok(hr == S_OK, "got %#lx.\n", hr);
    PathCombineW(path, dirpath, L"test_dir2");
    bret = CreateDirectoryW(path, NULL);
    ok(bret, "got error %ld.\n", GetLastError());

    hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void**)&folder);
    ok(hr == S_OK, "got %#lx.\n", hr);

    PathCombineW(path, path, L"test2");
    createTestFileW(path);

    /* Source is directory, destination test2 is file. */
    hr = IFileOperation_MoveItem(operation, item, folder, L"test2", NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_init_check_notifications(progress, ARRAY_SIZE(notifications4), notifications4, dirpath, &expected_notif);
    hr = IFileOperation_PerformOperations(operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_end_check_notifications(progress);
    aborted = 0;
    hr = IFileOperation_GetAnyOperationsAborted(operation, &aborted);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(aborted, "got %d.\n", aborted);

    bret = DeleteFileW(path);
    ok(bret, "got error %ld.\n", GetLastError());
    IFileOperation_Release(operation);

    /* Source is directory, destination is absent (simple move). */
    hr = CoCreateInstance(&CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOperation, (void **)&operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_Advise(operation, progress, &cookie);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_SetOperationFlags(operation, FOF_NO_UI);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IFileOperation_MoveItem(operation, item, folder, L"test2", NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_init_check_notifications(progress, ARRAY_SIZE(notifications5), notifications5, dirpath, &expected_notif);
    hr = IFileOperation_PerformOperations(operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_end_check_notifications(progress);
    IFileOperation_Release(operation);

    /* Source and dest are directories, merge is performed. */
    hr = CoCreateInstance(&CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOperation, (void **)&operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_Advise(operation, progress, &cookie);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_SetOperationFlags(operation, FOF_NO_UI);
    ok(hr == S_OK, "got %#lx.\n", hr);

    PathCombineW(path, dirpath, L"test_dir3");
    bret = CreateDirectoryW(path, NULL);
    set_shell_item_path(item, path);
    ok(bret, "got error %ld.\n", GetLastError());
    PathCombineW(tmpfile, path, L"testfile5");
    createTestFileW(tmpfile);
    PathCombineW(tmpfile, path, L"testfile6");
    createTestFileW(tmpfile);
    PathCombineW(path, path, L"inner_dir");
    bret = CreateDirectoryW(path, NULL);
    ok(bret, "got error %ld.\n", GetLastError());
    PathCombineW(tmpfile, path, L"testfile7");
    createTestFileW(tmpfile);
    PathCombineW(tmpfile, dirpath, L"testfile8");
    createTestFileW(tmpfile);

    hr = SHCreateItemFromParsingName(tmpfile, NULL, &IID_IShellItem, (void**)&item2);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IFileOperation_MoveItem(operation, item, folder, L"test2", progress2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IFileOperation_MoveItem(operation, item2, folder, NULL, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    refcount = IShellItem_Release(item2);
    ok(!refcount, "got %ld.\n", refcount);
    progress_init_check_notifications(progress, ARRAY_SIZE(notifications6), notifications6, dirpath, &expected_notif);
    hr = IFileOperation_PerformOperations(operation);
    ok(hr == S_OK, "got %#lx.\n", hr);
    progress_end_check_notifications(progress);

    PathCombineW(path, dirpath, L"test_dir2");
    PathCombineW(tmpfile, dirpath, L"test_dir2\\test2\\testfile6");
    ret = GetFileAttributesW(tmpfile);
    ok(ret != INVALID_FILE_ATTRIBUTES, "got %#lx.\n", ret);
    remove_directory(path);
    PathCombineW(path, dirpath, L"test_dir3");
    ret = GetFileAttributesW(path);
    ok(ret == INVALID_FILE_ATTRIBUTES, "got %#lx.\n", ret);
    remove_directory(path);

    IFileOperation_Release(operation);

    refcount = IShellItem_Release(item);
    ok(!refcount, "got %ld.\n", refcount);
    refcount = IShellItem_Release(folder);
    ok(!refcount, "got %ld.\n", refcount);

    refcount = IFileOperationProgressSink_Release(progress);
    ok(!refcount, "got %ld.\n", refcount);
    refcount = IFileOperationProgressSink_Release(progress2);
    ok(!refcount, "got %ld.\n", refcount);
}

START_TEST(shlfileop)
{
    clean_after_shfo_tests();

    init_shfo_tests();
    old_shell32 = is_old_shell32();
    if (old_shell32)
        win_skip("Need to cater for old shell32 (4.0.x) on Win95\n");

    clean_after_shfo_tests();

    init_shfo_tests();
    test_get_file_info();
    test_get_file_info_iconlist();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_delete();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_rename();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_copy();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_move();
    clean_after_shfo_tests();

    test_sh_create_dir();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_sh_path_prepare();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_sh_new_link_info();
    clean_after_shfo_tests();

    test_unicode();

    test_shlmenu();

    CoInitialize(NULL);

    test_file_operation();

    CoUninitialize();
}
