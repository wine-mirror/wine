/*
 * Copyright (C) 2004 Stefan Leichter
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winver.h"

#define MY_LAST_ERROR -1L
#define EXPECT_BAD_PATH__NOT_FOUND \
    ok( (ERROR_PATH_NOT_FOUND == GetLastError()) || \
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) || \
	(ERROR_FILE_NOT_FOUND == GetLastError()) || \
	(ERROR_BAD_PATHNAME == GetLastError()), \
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_BAD_PATHNAME (98)/" \
	"ERROR_PATH_NOT_FOUND (NT4)/ERROR_FILE_NOT_FOUND (2k3)" \
	"expected, got 0x%08lx\n", GetLastError());
#define EXPECT_INVALID__NOT_FOUND \
    ok( (ERROR_PATH_NOT_FOUND == GetLastError()) || \
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) || \
	(ERROR_FILE_NOT_FOUND == GetLastError()) || \
	(ERROR_INVALID_PARAMETER == GetLastError()), \
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_INVALID_PARAMETER (98)/" \
	"ERROR_PATH_NOT_FOUND (NT4)/ERROR_FILE_NOT_FOUND (2k3)" \
	"expected, got 0x%08lx\n", GetLastError());

static void test_info_size(void)
{   DWORD hdl, retval;
    char mypath[MAX_PATH] = "";

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( NULL, NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    EXPECT_INVALID__NOT_FOUND;

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( NULL, &hdl);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    EXPECT_INVALID__NOT_FOUND;
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "", NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    EXPECT_BAD_PATH__NOT_FOUND;

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "", &hdl);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    EXPECT_BAD_PATH__NOT_FOUND;
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", NULL);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
	retval);
    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	"Last error wrong! NO_ERROR/0x%08lx (NT4)  expected, got 0x%08lx\n",
	MY_LAST_ERROR, GetLastError());

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", &hdl);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
	retval);
    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	"Last error wrong! NO_ERROR/0x%08lx (NT4)  expected, got 0x%08lx\n",
	MY_LAST_ERROR, GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "notexist.dll", NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    ok( (ERROR_FILE_NOT_FOUND == GetLastError()) ||
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) ||
	(MY_LAST_ERROR == GetLastError()),
	"Last error wrong! ERROR_FILE_NOT_FOUND/ERROR_RESOURCE_DATA_NOT_FOUND "
	"(XP)/0x%08lx (NT4) expected, got 0x%08lx\n", MY_LAST_ERROR, GetLastError());

    /* test a currently loaded executable */
    if(GetModuleFileNameA(NULL, mypath, MAX_PATH)) {
	hdl = 0x55555555;
	SetLastError(MY_LAST_ERROR);
	retval = GetFileVersionInfoSizeA( mypath, &hdl);
	ok( retval,
	    "GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
	    retval);
	ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	    "Last error wrong! NO_ERROR/0x%08lx (NT4)  expected, got 0x%08lx\n",
	    MY_LAST_ERROR, GetLastError());
	ok( hdl == 0L,
	    "Handle wrong! 0L expected, got 0x%08lx\n", hdl);
    }
    else
	trace("skipping GetModuleFileNameA(NULL,..) failed\n");

    /* test a not loaded executable */
    if(GetSystemDirectoryA(mypath, MAX_PATH)) {
	lstrcatA(mypath, "\\regsvr32.exe");

	if(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(mypath))
	    trace("GetFileAttributesA(%s) failed\n", mypath);
	else {
	    hdl = 0x55555555;
	    SetLastError(MY_LAST_ERROR);
	    retval = GetFileVersionInfoSizeA( mypath, &hdl);
	    ok( retval,
		"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
		retval);
	    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
		"Last error wrong! NO_ERROR/0x%08lx (NT4)  expected, got 0x%08lx\n",
		MY_LAST_ERROR, GetLastError());
	    ok( hdl == 0L,
		"Handle wrong! 0L expected, got 0x%08lx\n", hdl);
	}
    }
    else
	trace("skipping GetModuleFileNameA(NULL,..) failed\n");
}

static void VersionDwordLong2String(DWORDLONG Version, LPSTR lpszVerString)
{
    WORD a, b, c, d;

    a = (WORD)(Version >> 48);
    b = (WORD)((Version >> 32) & 0xffff);
    c = (WORD)((Version >> 16) & 0xffff);
    d = (WORD)(Version & 0xffff);

    sprintf(lpszVerString, "%d.%d.%d.%d", a, b, c, d);

    return;
}

static void test_info(void)
{
    DWORD hdl, retval;
    PVOID pVersionInfo = NULL;
    BOOL boolret;
    VS_FIXEDFILEINFO *pFixedVersionInfo;
    UINT uiLength;
    char VersionString[MAX_PATH];
    DWORDLONG dwlVersion;

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", &hdl);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
	retval);
    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	"Last error wrong! NO_ERROR/0x%08lx (NT4)  expected, got 0x%08lx\n",
	MY_LAST_ERROR, GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    if ( retval == 0 || hdl != 0)
        return;

    pVersionInfo = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, retval );
    ok(pVersionInfo != 0, "HeapAlloc failed\n" );
    if (pVersionInfo == 0)
        return;

#if 0
    /* this test crashes on WinNT4
     */
    boolret = GetFileVersionInfoA( "kernel32.dll", 0, retval, 0);
    ok (!boolret, "GetFileVersionInfoA should have failed: GetLastError = 0x%08lx\n", GetLastError());
    ok ((GetLastError() == ERROR_INVALID_DATA) || (GetLastError() == ERROR_BAD_PATHNAME) ||
	(GetLastError() == NO_ERROR),
        "Last error wrong! ERROR_INVALID_DATA/ERROR_BAD_PATHNAME (ME)/"
	"NO_ERROR (95) expected, got 0x%08lx\n",
        GetLastError());
#endif

    boolret = GetFileVersionInfoA( "kernel32.dll", 0, retval, pVersionInfo );
    ok (boolret, "GetFileVersionInfoA failed: GetLastError = 0x%08lx\n", GetLastError());
    if (!boolret)
        return;

    boolret = VerQueryValueA( pVersionInfo, "\\", (LPVOID *)&pFixedVersionInfo, &uiLength );
    ok (boolret, "VerQueryValueA failed: GetLastError = 0x%08lx\n", GetLastError());
    if (!boolret)
        return;

    dwlVersion = (((DWORDLONG)pFixedVersionInfo->dwFileVersionMS) << 32) +
        pFixedVersionInfo->dwFileVersionLS;

    VersionDwordLong2String(dwlVersion, VersionString);

    trace("kernel32.dll version: %s\n", VersionString);

#if 0
    /* this test crashes on WinNT4
     */
    boolret = VerQueryValueA( pVersionInfo, "\\", (LPVOID *)&pFixedVersionInfo, 0);
    ok (boolret, "VerQueryValue failed: GetLastError = 0x%08lx\n", GetLastError());
#endif
}

static void test_32bit_win(void)
{
    DWORD hdlA, retvalA;
    DWORD hdlW, retvalW;
    BOOL retA,retW;
    PVOID pVersionInfoA = NULL;
    PVOID pVersionInfoW = NULL;
    char *pBufA;
    WCHAR *pBufW;
    UINT uiLength;
    char mypathA[MAX_PATH];
    WCHAR mypathW[MAX_PATH];
    char varfileinfoA[] = "\\\\VarFileInfo\\\\Translation";
    WCHAR varfileinfoW[]    = { '\\','\\','V','a','r','F','i','l','e','I','n','f','o',
                                '\\','\\','T','r','a','n','s','l','a','t','i','o','n', 0 };
    char FileDescriptionA[] = "\\\\StringFileInfo\\\\040904E4\\\\FileDescription";
    WCHAR FileDescriptionW[] = { '\\','\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
                                '\\','\\','0','4','0','9','0','4','e','4',
                                '\\','\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n', 0 };
    char WineFileDescriptionA[] = "Wine version test";
    WCHAR WineFileDescriptionW[] = { 'W','i','n','e',' ','v','e','r','s','i','o','n',' ','t','e','s','t', 0 };

    /* If we call GetFileVersionInfoA on a system that supports Unicode (NT/W2K/XP/W2K3 by default)
     * then the versioninfo will contain Unicode strings.
     * Wine however always converts a VersionInfo32 to VersionInfo16 when called through GetFileVersionInfoA
     * regardless of Windows version
     * The test is to call both the A and W versions, which should have the same Version Information,
     * on systems that support both calls.
     */

    /* First get the versioninfo via the W versions */
    GetModuleFileNameW(NULL, mypathW, MAX_PATH);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("GetModuleFileNameW not existing on this platform, skipping rest of test\n");
        return;
    }

    retvalW = GetFileVersionInfoSizeW( mypathW, &hdlW);
    pVersionInfoW = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, retvalW );
    retW = GetFileVersionInfoW( mypathW, 0, retvalW, pVersionInfoW );

    /* And now via the A versions */
    GetModuleFileNameA(NULL, mypathA, MAX_PATH);
    retvalA = GetFileVersionInfoSizeA( mypathA, &hdlA);
    pVersionInfoA = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, retvalA );
    retA = GetFileVersionInfoA( mypathA, 0, retvalA, pVersionInfoA );

    ok( retvalA == retvalW, "The size of the struct should be the same for both A/W calls, it is (%ld) vs. (%ld)\n",
                            retvalA, retvalW);

    ok( !memcmp(pVersionInfoA, pVersionInfoW, retvalA), "Both structs should be the same, they aren't\n");

    /* The structs are the same but that will mysteriously change with the next calls on Windows (not on Wine).
     * The structure on windows is way bigger then needed, so there must be something to it. As we do not
     * seem to need this bigger structure, we can leave that as is.
     * The change is in this not needed part.
     *
     * Although the structures contain Unicode strings, VerQueryValueA will return normal strings,
     * VerQueryValueW will return Unicode ones.
     */

    retA = VerQueryValueA( pVersionInfoA, varfileinfoA, (LPVOID *)&pBufA, &uiLength );
    ok (retA, "VerQueryValueA failed: GetLastError = 0x%08lx\n", GetLastError());

    retA = VerQueryValueA( pVersionInfoA, FileDescriptionA, (LPVOID *)&pBufA, &uiLength );
    ok (retA, "VerQueryValueA failed: GetLastError = 0x%08lx\n", GetLastError());
    ok( !lstrcmpA(WineFileDescriptionA, pBufA), "FileDescription should have been 'Wine version test'\n");

    /* And the W-way */

    retW = VerQueryValueW( pVersionInfoW, varfileinfoW, (LPVOID *)&pBufW, &uiLength );
    ok (retW, "VerQueryValueW failed: GetLastError = 0x%08lx\n", GetLastError());

    retW = VerQueryValueW( pVersionInfoW, FileDescriptionW, (LPVOID *)&pBufW, &uiLength );
    ok (retW, "VerQueryValueW failed: GetLastError = 0x%08lx\n", GetLastError());
    ok( !lstrcmpW(WineFileDescriptionW, pBufW), "FileDescription should have been 'Wine version test' (unicode)\n");

    HeapFree( GetProcessHeap(), 0, pVersionInfoA);
    HeapFree( GetProcessHeap(), 0, pVersionInfoW);
}

START_TEST(info)
{
    test_info_size();
    test_info();
   
    /* Test GetFileVersionInfoSize[AW] and GetFileVersionInfo[AW] on a 32 bit windows executable */
    trace("Testing 32 bit windows application\n");
    test_32bit_win();
}
