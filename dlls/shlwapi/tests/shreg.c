/* Unit test suite for SHReg* functions
 *
 * Copyright 2002 Juergen Schmied
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"

/* Keys used for testing */
#define REG_TEST_KEY        "Software\\Wine\\Test"
#define REG_CURRENT_VERSION "Software\\Microsoft\\Windows NT\\CurrentVersion"

static HMODULE hshlwapi;
typedef DWORD (WINAPI *SHCopyKeyA_func)(HKEY,LPCSTR,HKEY,DWORD);
static SHCopyKeyA_func pSHCopyKeyA;
typedef DWORD (WINAPI *SHRegGetPathA_func)(HKEY,LPCSTR,LPCSTR,LPSTR,DWORD);
static SHRegGetPathA_func pSHRegGetPathA;

static const char * sTestpath1 = "%LONGSYSTEMVAR%\\subdir1";
static const char * sTestpath2 = "%FOO%\\subdir1";

static char sExpTestpath1[MAX_PATH];
static char sExpTestpath2[MAX_PATH];
static unsigned sExpLen1;
static unsigned sExpLen2;

static const char * sEmptyBuffer ="0123456789";

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey )
{
    WCHAR name[MAX_PATH];
    DWORD ret;

    while (!(ret = RegEnumKeyW(hkey, 0, name, sizeof(name))))
    {
        HKEY tmp;
        if (!(ret = RegOpenKeyExW( hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp )))
        {
            ret = delete_key( tmp );
            RegCloseKey( tmp );
        }
        if (ret) break;
    }
    if (ret != ERROR_NO_MORE_ITEMS) return ret;
    RegDeleteKeyA( hkey, NULL );
    return 0;
}

static HKEY create_test_entries(void)
{
	HKEY hKey;

        SetEnvironmentVariableA("LONGSYSTEMVAR", "bar");
        SetEnvironmentVariableA("FOO", "ImARatherLongButIndeedNeededString");

	ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEY, &hKey), "RegCreateKeyA failed\n");

	if (hKey)
	{
           ok(!RegSetValueExA(hKey,"Test1",0,REG_EXPAND_SZ, sTestpath1, strlen(sTestpath1)+1), "RegSetValueExA failed\n");
           ok(!RegSetValueExA(hKey,"Test2",0,REG_SZ, sTestpath1, strlen(sTestpath1)+1), "RegSetValueExA failed\n");
           ok(!RegSetValueExA(hKey,"Test3",0,REG_EXPAND_SZ, sTestpath2, strlen(sTestpath2)+1), "RegSetValueExA failed\n");
	}

	sExpLen1 = ExpandEnvironmentStringsA(sTestpath1, sExpTestpath1, sizeof(sExpTestpath1));
	sExpLen2 = ExpandEnvironmentStringsA(sTestpath2, sExpTestpath2, sizeof(sExpTestpath2));

        ok(sExpLen1 > 0, "Couldn't expand %s\n", sTestpath1);
        ok(sExpLen2 > 0, "Couldn't expand %s\n", sTestpath2);
        return hKey;
}

static void test_SHGetValue(void)
{
	DWORD dwSize;
	DWORD dwType;
	char buf[MAX_PATH];

	strcpy(buf, sEmptyBuffer);
	dwSize = MAX_PATH;
	dwType = -1;
	ok(! SHGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", &dwType, buf, &dwSize), "SHGetValueA failed\n");
	ok( 0 == strcmp(sExpTestpath1, buf), "(%s,%s)\n", buf, sExpTestpath1);
	ok( REG_SZ == dwType, "(%lx)\n", dwType);

	strcpy(buf, sEmptyBuffer);
	dwSize = MAX_PATH;
	dwType = -1;
	ok(! SHGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test2", &dwType, buf, &dwSize), "SHGetValueA failed\n");
	ok( 0 == strcmp(sTestpath1, buf) , "(%s)\n", buf);
	ok( REG_SZ == dwType , "(%lx)\n", dwType);
}

static void test_SHGetRegPath(void)
{
	char buf[MAX_PATH];

	if (!pSHRegGetPathA)
		return;

	strcpy(buf, sEmptyBuffer);
	ok(! (*pSHRegGetPathA)(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", buf, 0), "SHRegGetPathA failed\n");
	ok( 0 == strcmp(sExpTestpath1, buf) , "(%s)\n", buf);
}

static void test_SHQUeryValueEx(void)
{
	HKEY hKey;
	DWORD dwSize;
	DWORD dwType;
	char buf[MAX_PATH];
	DWORD dwRet;
	const char * sTestedFunction = "";
	DWORD nUsedBuffer1,nUsedBuffer2;

	ok(! RegOpenKeyExA(HKEY_CURRENT_USER, REG_TEST_KEY, 0,  KEY_QUERY_VALUE, &hKey), "test4 RegOpenKey\n");

	/****** SHQueryValueExA ******/

	sTestedFunction = "SHQueryValueExA";
	nUsedBuffer1 = max(strlen(sExpTestpath1)+1, strlen(sTestpath1)+1);
	nUsedBuffer2 = max(strlen(sExpTestpath2)+1, strlen(sTestpath2)+1);
	/*
	 * Case 1.1 All arguments are NULL
	 */
	ok(! SHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, NULL), "SHQueryValueExA failed\n");

	/*
	 * Case 1.2 dwType is set
	 */
	dwType = -1;
	ok(! SHQueryValueExA( hKey, "Test1", NULL, &dwType, NULL, NULL), "SHQueryValueExA failed\n");
	ok( dwType == REG_SZ, "(%lu)\n", dwType);

	/*
	 * dwSize is set
         * dwExpanded < dwUnExpanded
	 */
	dwSize = 6;
	ok(! SHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, &dwSize), "SHQueryValueExA failed\n");
	ok( dwSize == nUsedBuffer1, "(%lu,%lu)\n", dwSize, nUsedBuffer1);

	/*
         * dwExpanded > dwUnExpanded
	 */
	dwSize = 6;
	ok(! SHQueryValueExA( hKey, "Test3", NULL, NULL, NULL, &dwSize), "SHQueryValueExA failed\n");
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)\n", dwSize, nUsedBuffer2);


	/*
	 * Case 1 string shrinks during expanding
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test1", NULL, &dwType, buf, &dwSize);
	ok( dwRet == ERROR_MORE_DATA, "(%lu)\n", dwRet);
	ok( 0 == strcmp(sEmptyBuffer, buf), "(%s)\n", buf);
	ok( dwType == REG_SZ, "(%lu)\n" , dwType);
	ok( dwSize == nUsedBuffer1, "(%lu,%lu)\n" , dwSize, nUsedBuffer1);

	/*
	 * string grows during expanding
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize);
	ok( ERROR_MORE_DATA == dwRet, "ERROR_MORE_DATA\n");
	ok( 0 == strcmp(sEmptyBuffer, buf), "(%s)\n", buf);
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)\n" , dwSize, nUsedBuffer2);
	ok( dwType == REG_SZ, "(%lu)\n" , dwType);

	/*
	 * if the unexpanded string fits into the buffer it can get cut when expanded
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = sExpLen2 - 4;
	dwType = -1;
	ok( ERROR_MORE_DATA == SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize), "Expected ERROR_MORE_DATA\n");
	ok( 0 == strncmp(sExpTestpath2, buf, sExpLen2 - 4 - 1), "(%s)\n", buf);
	ok( sExpLen2 - 4 - 1 == strlen(buf), "(%s)\n", buf);
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)\n" , dwSize, nUsedBuffer2);
	ok( dwType == REG_SZ, "(%lu)\n" , dwType);

	/*
	 * The buffer is NULL but the size is set
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, NULL, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "(%lu)\n", dwRet);
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)\n" , dwSize, nUsedBuffer2);
	ok( dwType == REG_SZ, "(%lu)\n" , dwType);


	RegCloseKey(hKey);
}

static void test_SHCopyKey(void)
{
	HKEY hKeySrc, hKeyDst;

	/* Delete existing destination sub keys */
	hKeyDst = NULL;
	if (!RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination", &hKeyDst) && hKeyDst)
	{
		SHDeleteKeyA(hKeyDst, NULL);
		RegCloseKey(hKeyDst);
	}

	hKeyDst = NULL;
	if (RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination", &hKeyDst) || !hKeyDst)
	{
		ok(0, "didn't open dest\n");
		return;
	}

	hKeySrc = NULL;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, REG_CURRENT_VERSION, &hKeySrc) || !hKeySrc)
	{
		ok(0, "didn't open source\n");
		return;
	}


	if (pSHCopyKeyA)
		ok (!(*pSHCopyKeyA)(hKeyDst, NULL, hKeySrc, 0), "failed copy\n");

	RegCloseKey(hKeySrc);
	RegCloseKey(hKeyDst);

	/* Check we copied the sub keys, i.e. AeDebug from the default wine registry */
	hKeyDst = NULL;
	if (RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination\\AeDebug", &hKeyDst) || !hKeyDst)
	{
		ok(0, "didn't open copy\n");
		return;
	}

	/* And the we copied the values too */
	ok(!SHQueryValueExA(hKeyDst, "Debugger", NULL, NULL, NULL, NULL), "SHQueryValueExA failed\n");

	RegCloseKey(hKeyDst);
}


START_TEST(shreg)
{
	HKEY hkey = create_test_entries();
	hshlwapi = GetModuleHandleA("shlwapi.dll");
	if (hshlwapi)
	{
		pSHCopyKeyA=(SHCopyKeyA_func)GetProcAddress(hshlwapi,"SHCopyKeyA");
		pSHRegGetPathA=(SHRegGetPathA_func)GetProcAddress(hshlwapi,"SHRegGetPathA");
	}
	test_SHGetValue();
	test_SHQUeryValueEx();
	test_SHGetRegPath();
	test_SHCopyKey();
        delete_key( hkey );
}
