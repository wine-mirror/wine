/*
 * Unit tests for profile functions
 *
 * Copyright (c) 2003 Stefan Leichter
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "windows.h"

#define KEY      "ProfileInt"
#define SECTION  "Test"
#define TESTFILE ".\\testwine.ini"
#define TESTFILE2 ".\\testwine2.ini"

struct _profileInt { 
    LPCSTR section;
    LPCSTR key;
    LPCSTR value;
    LPCSTR iniFile;
    INT defaultVal;
    UINT result;
    UINT result9x;
};

static void test_profile_int(void)
{
    struct _profileInt profileInt[]={
         { NULL,    NULL, NULL,          NULL,     70, 0          , 0}, /*  0 */
         { NULL,    NULL, NULL,          TESTFILE, -1, 4294967295U, 0},
         { NULL,    NULL, NULL,          TESTFILE,  1, 1          , 0},
         { SECTION, NULL, NULL,          TESTFILE, -1, 4294967295U, 0},
         { SECTION, NULL, NULL,          TESTFILE,  1, 1          , 0},
         { NULL,    KEY,  NULL,          TESTFILE, -1, 4294967295U, 0}, /*  5 */
         { NULL,    KEY,  NULL,          TESTFILE,  1, 1          , 0},
         { SECTION, KEY,  NULL,          TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  NULL,          TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "-1",          TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  "-1",          TESTFILE,  1, 4294967295U, 4294967295U}, /* 10 */
         { SECTION, KEY,  "1",           TESTFILE, -1, 1          , 1},
         { SECTION, KEY,  "1",           TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "+1",          TESTFILE, -1, 1          , 0},
         { SECTION, KEY,  "+1",          TESTFILE,  1, 1          , 0},
         { SECTION, KEY,  "4294967296",  TESTFILE, -1, 0          , 0}, /* 15 */
         { SECTION, KEY,  "4294967296",  TESTFILE,  1, 0          , 0},
         { SECTION, KEY,  "4294967297",  TESTFILE, -1, 1          , 1},
         { SECTION, KEY,  "4294967297",  TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "-4294967297", TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  "-4294967297", TESTFILE,  1, 4294967295U, 4294967295U}, /* 20 */
         { SECTION, KEY,  "42A94967297", TESTFILE, -1, 42         , 42},
         { SECTION, KEY,  "42A94967297", TESTFILE,  1, 42         , 42},
         { SECTION, KEY,  "B4294967297", TESTFILE, -1, 0          , 0},
         { SECTION, KEY,  "B4294967297", TESTFILE,  1, 0          , 0},
    };
    int i, num_test = (sizeof(profileInt)/sizeof(struct _profileInt));
    UINT res;

    DeleteFileA( TESTFILE);

    for (i=0; i < num_test; i++) {
        if (profileInt[i].value)
            WritePrivateProfileStringA(SECTION, KEY, profileInt[i].value, 
                                      profileInt[i].iniFile);

       res = GetPrivateProfileIntA(profileInt[i].section, profileInt[i].key, 
                 profileInt[i].defaultVal, profileInt[i].iniFile); 
       ok((res == profileInt[i].result) || (res == profileInt[i].result9x),
	    "test<%02d>: ret<%010u> exp<%010u><%010u>\n",
            i, res, profileInt[i].result, profileInt[i].result9x);
    }

    DeleteFileA( TESTFILE);
}

static void test_profile_string(void)
{
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    char *p;
    /* test that lines without an '=' will not be enumerated */
    /* in the case below, name2 is a key while name3 is not. */
    char content[]="[s]\r\nname1=val1\r\nname2=\r\nname3\r\nname4=val4\r\n";
    DeleteFileA( TESTFILE2);
    h = CreateFileA( TESTFILE2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", TESTFILE2);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* enumerate the keys */
    ret=GetPrivateProfileStringA( "s", NULL, "", buf, sizeof(buf),
        TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    /* and test */
    ok( ret == 18 && !strcmp( buf, "name1,name2,name4"), "wrong keys returned(%d): %s\n", ret,
            buf);

    /* add a new key to test that the file is quite usable */
    WritePrivateProfileStringA( "s", "name5", "val5", TESTFILE2); 
    ret=GetPrivateProfileStringA( "s", NULL, "", buf, sizeof(buf),
        TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    ok( ret == 24 && !strcmp( buf, "name1,name2,name4,name5"), "wrong keys returned(%d): %s\n",
            ret, buf);

    DeleteFileA( TESTFILE2);
}

static void test_profile_sections(void)
{
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    char *p;
    static const char content[]="[section1]\r\nname1=val1\r\nname2=\r\nname3\r\nname4=val4\r\n[section2]\r\n";
    static const char testfile4[]=".\\testwine4.ini";
    BOOL on_win98 = FALSE;

    DeleteFileA( testfile4 );
    h = CreateFileA( testfile4, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", testfile4);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* Some parameter checking */
    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( NULL, NULL, 0, NULL );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == 0xdeadbeef /* Win98 */,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    if (GetLastError() == 0xdeadbeef) on_win98 = TRUE;

    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( NULL, NULL, 0, testfile4 );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == 0xdeadbeef /* Win98 */,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    if (!on_win98)
    {
        SetLastError(0xdeadbeef);
        ret = GetPrivateProfileSectionA( "section1", NULL, 0, testfile4 );
        ok( ret == 0, "expected return size 0, got %d\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( NULL, buf, sizeof(buf), testfile4 );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( "section1", buf, sizeof(buf), NULL );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    todo_wine
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* And a real one */
    ret=GetPrivateProfileSectionA("section1", buf, sizeof(buf), testfile4);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1)
        p[-1] = ',';
    ok( ret == 35 && !strcmp( buf, "name1=val1,name2=,name3,name4=val4"), "wrong section returned(%d): %s\n",
            ret, buf);
    ok( buf[ret-1] == 0 && buf[ret] == 0, "returned buffer not terminated with double-null\n" );
    ok( GetLastError() == S_OK, "expected S_OK, got %d\n", GetLastError());

    DeleteFileA( testfile4 );
}

static void test_profile_sections_names(void)
{
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    WCHAR bufW[100];
    static const char content[]="[section1]\r\n[section2]\r\n[section3]\r\n";
    static const char testfile3[]=".\\testwine3.ini";
    static const WCHAR testfile3W[]={ '.','\\','t','e','s','t','w','i','n','e','3','.','i','n','i',0 };
    static const WCHAR not_here[] = {'.','\\','n','o','t','_','h','e','r','e','.','i','n','i',0};
    DeleteFileA( testfile3 );
    h = CreateFileA( testfile3, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", testfile3);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* Test with sufficiently large buffer */
    ret = GetPrivateProfileSectionNamesA( buf, 29, testfile3 );
    ok( ret == 27, "expected return size 27, got %d\n", ret );
    ok( buf[ret-1] == 0 && buf[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    /* Test with exactly fitting buffer */
    ret = GetPrivateProfileSectionNamesA( buf, 28, testfile3 );
    ok( ret == 26, "expected return size 26, got %d\n", ret );
    ok( buf[ret+1] == 0 && buf[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    /* Test with a buffer too small */
    ret = GetPrivateProfileSectionNamesA( buf, 27, testfile3 );
    ok( ret == 25, "expected return size 25, got %d\n", ret );
    ok( buf[ret+1] == 0 && buf[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    /* Tests on nonexistent file */
    memset(buf, 0xcc, sizeof(buf));
    ret = GetPrivateProfileSectionNamesA( buf, 10, ".\\not_here.ini" );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( buf[0] == 0, "returned buffer not terminated with null\n" );
    ok( buf[1] != 0, "returned buffer terminated with double-null\n" );
    
    /* Test with sufficiently large buffer */
    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionNamesW( bufW, 29, testfile3W );
    if (ret == 0 && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("GetPrivateProfileSectionNamesW is not implemented\n");
        DeleteFileA( testfile3 );
        return;
    }
    ok( ret == 27, "expected return size 27, got %d\n", ret );
    ok( bufW[ret-1] == 0 && bufW[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    /* Test with exactly fitting buffer */
    ret = GetPrivateProfileSectionNamesW( bufW, 28, testfile3W );
    ok( ret == 26, "expected return size 26, got %d\n", ret );
    ok( bufW[ret+1] == 0 && bufW[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    /* Test with a buffer too small */
    ret = GetPrivateProfileSectionNamesW( bufW, 27, testfile3W );
    ok( ret == 25, "expected return size 25, got %d\n", ret );
    ok( bufW[ret+1] == 0 && bufW[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    DeleteFileA( testfile3 );

    /* Tests on nonexistent file */
    memset(bufW, 0xcc, sizeof(bufW));
    ret = GetPrivateProfileSectionNamesW( bufW, 10, not_here );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( bufW[0] == 0, "returned buffer not terminated with null\n" );
    ok( bufW[1] != 0, "returned buffer terminated with double-null\n" );
}

/* If the ini-file has already been opened with CreateFile, WritePrivateProfileString failed in wine with an error ERROR_SHARING_VIOLATION,  some testing here */
static void test_profile_existing(void)
{
    static const char *testfile1 = ".\\winesharing1.ini";
    static const char *testfile2 = ".\\winesharing2.ini";

    static const struct {
        DWORD dwDesiredAccess;
        DWORD dwShareMode;
        DWORD write_error;
        BOOL read_error;
    } pe[] = {
        {GENERIC_READ,  FILE_SHARE_READ,  ERROR_SHARING_VIOLATION, FALSE },
        {GENERIC_READ,  FILE_SHARE_WRITE, ERROR_SHARING_VIOLATION, TRUE },
        {GENERIC_WRITE, FILE_SHARE_READ,  ERROR_SHARING_VIOLATION, FALSE },
        {GENERIC_WRITE, FILE_SHARE_WRITE, ERROR_SHARING_VIOLATION, TRUE },
        {GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,  ERROR_SHARING_VIOLATION, FALSE },
        {GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE, ERROR_SHARING_VIOLATION, TRUE },
        {GENERIC_READ,  FILE_SHARE_READ|FILE_SHARE_WRITE, 0, FALSE },
        {GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, FALSE },
        /*Thief demo (bug 5024) opens .ini file like this*/
        {GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, FALSE }
    };

    int i;
    BOOL ret;
    DWORD size;
    HANDLE h = 0;
    char buffer[MAX_PATH];

    for (i=0; i < sizeof(pe)/sizeof(pe[0]); i++)
    {
        h = CreateFile(testfile1, pe[i].dwDesiredAccess, pe[i].dwShareMode, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(INVALID_HANDLE_VALUE != h, "%d: CreateFile failed\n",i);
        SetLastError(0xdeadbeef);

        ret = WritePrivateProfileString(SECTION, KEY, "12345", testfile1);
        if (!pe[i].write_error)
        {
            ok( ret, "%d: WritePrivateProfileString failed with error %u\n", i, GetLastError() );
            CloseHandle(h);
            size = GetPrivateProfileString(SECTION, KEY, 0, buffer, MAX_PATH, testfile1);
            ok( size == 5, "%d: test failed, number of characters copied: %d instead of 5\n", i, size );
        }
        else
        {
            DWORD err = GetLastError();
            ok( !ret, "%d: WritePrivateProfileString succeeded\n", i );
            if (!ret)
                ok( err == pe[i].write_error, "%d: WritePrivateProfileString failed with error %u/%u\n",
                    i, err, pe[i].write_error );
            CloseHandle(h);
            size = GetPrivateProfileString(SECTION, KEY, 0, buffer, MAX_PATH, testfile1);
            ok( !size, "%d: test failed, number of characters copied: %d instead of 0\n", i, size );
        }

        ok( DeleteFile(testfile1), "delete failed\n" );
    }

    h = CreateFile(testfile2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    sprintf( buffer, "[%s]\r\n%s=123\r\n", SECTION, KEY );
    ok( WriteFile( h, buffer, strlen(buffer), &size, NULL ), "failed to write\n" );
    CloseHandle( h );

    for (i=0; i < sizeof(pe)/sizeof(pe[0]); i++)
    {
        h = CreateFile(testfile2, pe[i].dwDesiredAccess, pe[i].dwShareMode, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(INVALID_HANDLE_VALUE != h, "%d: CreateFile failed\n",i);
        SetLastError(0xdeadbeef);
        ret = GetPrivateProfileStringA(SECTION, KEY, NULL, buffer, MAX_PATH, testfile2);
        if (!pe[i].read_error)
            ok( ret, "%d: GetPrivateProfileString failed with error %u\n", i, GetLastError() );
        else
            ok( !ret, "%d: GetPrivateProfileString succeeded\n", i );
        CloseHandle(h);
    }
    ok( DeleteFile(testfile2), "delete failed\n" );
}

static void create_test_file(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE hfile;
    DWORD count;

    hfile = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "cannot create %s\n", name);
    WriteFile(hfile, data, size, &count, NULL);
    CloseHandle(hfile);
}

static void test_GetPrivateProfileString(void)
{
    DWORD ret;
    CHAR buf[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR windir[MAX_PATH];
    LPSTR tempfile;

    static const char filename[] = ".\\winetest.ini";
    static const char content[]=
        "[section1]\r\n"
        "name1=val1\r\n"
        "name2=\"val2\"\r\n"
        "name3\r\n"
        "name4=a\r\n"
        "[section2]\r\n";

    create_test_file(filename, content, sizeof(content));

    /* lpAppName is NULL */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(NULL, "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 18, "Expected 18, got %d\n", ret);
    ok(!memcmp(buf, "section1\0section2\0", ret + 1),
       "Expected \"section1\\0section2\\0\", got \"%s\"\n", buf);

    /* lpAppName is empty */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("", "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpAppName is missing */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("notasection", "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpAppName is empty, lpDefault is NULL */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("", "name1", NULL,
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);

    /* lpAppName is empty, lpDefault is empty */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("", "name1", "",
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);

    /* lpAppName is empty, lpDefault has trailing blank characters */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("", "name1", "default  ",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpAppName is empty, many blank characters in lpDefault */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("", "name1", "one two  ",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "one two"), "Expected \"one two\", got \"%s\"\n", buf);

    /* lpAppName is empty, blank character but not trailing in lpDefault */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("", "name1", "one two",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "one two"), "Expected \"one two\", got \"%s\"\n", buf);

    /* lpKeyName is NULL */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", NULL, "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 18, "Expected 18, got %d\n", ret);
    ok(!memcmp(buf, "name1\0name2\0name4\0", ret + 1),
       "Expected \"name1\\0name2\\0name4\\0\", got \"%s\"\n", buf);

    /* lpKeyName is empty */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "", "default",
                                   buf, MAX_PATH, filename);
    todo_wine
    {
        ok(ret == 7, "Expected 7, got %d\n", ret);
        ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);
    }

    /* lpKeyName is missing */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "notakey", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpKeyName is empty, lpDefault is NULL */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "", NULL,
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    todo_wine
    {
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    }

    /* lpKeyName is empty, lpDefault is empty */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "", "",
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    todo_wine
    {
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    }

    /* lpKeyName is empty, lpDefault has trailing blank characters */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "", "default  ",
                                   buf, MAX_PATH, filename);
    todo_wine
    {
        ok(ret == 7, "Expected 7, got %d\n", ret);
        ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);
    }

    if (0) /* crashes */
    {
        /* lpReturnedString is NULL */
        ret = GetPrivateProfileStringA("section1", "name1", "default",
                                       NULL, MAX_PATH, filename);
    }

    /* lpFileName is NULL */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, NULL);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpFileName is empty */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, "");
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpFileName is nonexistent */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, "nonexistent");
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* nSize is 0 */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, 0, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, "kumquat"), "Expected buf to be unchanged, got \"%s\"\n", buf);

    /* nSize is exact size of output */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, 4, filename);
    ok(ret == 3, "Expected 3, got %d\n", ret);
    ok(!lstrcmpA(buf, "val"), "Expected \"val\", got \"%s\"\n", buf);

    /* nSize has room for NULL terminator */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, 5, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    /* output is 1 character */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name4", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 1, "Expected 1, got %d\n", ret);
    ok(!lstrcmpA(buf, "a"), "Expected \"a\", got \"%s\"\n", buf);

    /* output is 1 character, no room for NULL terminator */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name4", "default",
                                   buf, 1, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);

    /* lpAppName is NULL, not enough room for final section name */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(NULL, "name1", "default",
                                   buf, 16, filename);
    ok(ret == 14, "Expected 14, got %d\n", ret);
    ok(!memcmp(buf, "section1\0secti\0", ret + 1),
       "Expected \"section1\\0secti\\0\", got \"%s\"\n", buf);

    /* lpKeyName is NULL, not enough room for final key name */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", NULL, "default",
                                   buf, 16, filename);
    ok(ret == 14, "Expected 14, got %d\n", ret);
    ok(!memcmp(buf, "name1\0name2\0na\0", ret + 1),
       "Expected \"name1\\0name2\\0na\\0\", got \"%s\"\n", buf);

    /* key value has quotation marks which are stripped */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name2", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val2"), "Expected \"val2\", got \"%s\"\n", buf);

    /* case does not match */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "NaMe1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    /* only filename is used */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "NaMe1", "default",
                                   buf, MAX_PATH, "winetest.ini");
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    GetWindowsDirectoryA(windir, MAX_PATH);
    GetTempFileNameA(windir, "pre", 0, path);
    tempfile = strrchr(path, '\\') + 1;
    create_test_file(path, content, sizeof(content));

    /* only filename is used, file exists in windows directory */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "NaMe1", "default",
                                   buf, MAX_PATH, tempfile);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    /* successful case */
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    DeleteFileA(path);
    DeleteFileA(filename);
}

START_TEST(profile)
{
    test_profile_int();
    test_profile_string();
    test_profile_sections();
    test_profile_sections_names();
    test_profile_existing();
    test_GetPrivateProfileString();
}
