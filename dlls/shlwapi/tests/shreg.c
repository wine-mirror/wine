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
#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "shlwapi.h"

static char * sTestpath1 = "%SYSTEMROOT%\\subdir1";
static char * sTestpath2 = "%USERPROFILE%\\subdir1";

static char sExpTestpath1[MAX_PATH];
static char sExpTestpath2[MAX_PATH];

static char * sEmptyBuffer ="0123456789";

static void create_test_entrys()
{
	HKEY hKey;

	ok(!RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hKey), "");

	if (hKey)
	{
           ok(!RegSetValueExA(hKey,"Test1",0,REG_EXPAND_SZ, sTestpath1, strlen(sTestpath1)), "");
           ok(!RegSetValueExA(hKey,"Test2",0,REG_SZ, sTestpath1, strlen(sTestpath1)), "");
           ok(!RegSetValueExA(hKey,"Test3",0,REG_EXPAND_SZ, sTestpath2, strlen(sTestpath2)), "");
	   RegCloseKey(hKey);
	}

	ExpandEnvironmentStringsA(sTestpath1, sExpTestpath1, sizeof(sExpTestpath1));
	ExpandEnvironmentStringsA(sTestpath2, sExpTestpath2, sizeof(sExpTestpath2));
	ok(strlen(sExpTestpath2) > 25, "%USERPROFILE% is set to a short value on this machine. we cant perform all tests.");
}

static void test_SHGetValue(void)
{
	DWORD dwSize;
	DWORD dwType;
	char buf[MAX_PATH];

	strcpy(buf, sEmptyBuffer);
	dwSize = MAX_PATH;
	dwType = -1;
	ok(! SHGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "Test1", &dwType, buf, &dwSize), "");
	ok( 0 == strcmp(sExpTestpath1, buf), "(%s,%s)", buf, sExpTestpath1);
	ok( REG_SZ == dwType, "(%lx)", dwType);

	strcpy(buf, sEmptyBuffer);
	dwSize = MAX_PATH;
	dwType = -1;
	ok(! SHGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "Test2", &dwType, buf, &dwSize), "");
	ok( 0 == strcmp(sTestpath1, buf) , "(%s)", buf);
	ok( REG_SZ == dwType , "(%lx)", dwType);
}

static void test_SHGetTegPath(void)
{
	char buf[MAX_PATH];

	strcpy(buf, sEmptyBuffer);
	ok(! SHRegGetPathA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "Test1", buf, 0), "");
	ok( 0 == strcmp(sExpTestpath1, buf) , "(%s)", buf);
}

static void test_SHQUeryValueEx(void)
{
	HKEY hKey;
	DWORD dwSize;
	DWORD dwType;
	char buf[MAX_PATH];
	DWORD dwRet;
	char * sTestedFunction = "";
	int nUsedBuffer1;
	int nUsedBuffer2;

	ok(! RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Wine\\Test", 0,  KEY_QUERY_VALUE, &hKey), "test4 RegOpenKey");

	/****** SHQueryValueExA ******/

	sTestedFunction = "SHQueryValueExA";
	nUsedBuffer1 = max(strlen(sExpTestpath1)+1, strlen(sTestpath1));
	nUsedBuffer2 = max(strlen(sExpTestpath2)+1, strlen(sTestpath2));
	/*
	 * Case 1.1 All arguments are NULL
	 */
	ok(! SHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, NULL), "");

	/*
	 * Case 1.2 dwType is set
	 */
	dwType = -1;
	ok(! SHQueryValueExA( hKey, "Test1", NULL, &dwType, NULL, NULL), "");
	ok( dwType == REG_SZ, "(%lu)", dwType);

	/*
	 * dwSize is set
         * dwExpanded < dwUnExpanded
	 */
	dwSize = 6;
	ok(! SHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, &dwSize), "");
	ok( dwSize == nUsedBuffer1, "(%lu,%lu)", dwSize, nUsedBuffer1);

	/*
         * dwExpanded > dwUnExpanded
	 */
	dwSize = 6;
	ok(! SHQueryValueExA( hKey, "Test3", NULL, NULL, NULL, &dwSize), "");
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)", dwSize, nUsedBuffer2);


	/*
	 * Case 1 string shrinks during expanding
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test1", NULL, &dwType, buf, &dwSize);
	ok( dwRet == ERROR_MORE_DATA, "(%lu)", dwRet);
	ok( 0 == strcmp(sEmptyBuffer, buf), "(%s)", buf);
	ok( dwType == REG_SZ, "(%lu)" , dwType);
	ok( dwSize == nUsedBuffer1, "(%lu,%lu)" , dwSize, nUsedBuffer1);

	/*
	 * string grows during expanding
	 */	
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize);
	ok( ERROR_MORE_DATA == dwRet, "");
	ok( 0 == strcmp(sEmptyBuffer, buf), "(%s)", buf);
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)" , dwSize, nUsedBuffer2);
	ok( dwType == REG_SZ, "(%lu)" , dwType);

	/*
	 * if the unexpanded string fits into the buffer it can get cut when expanded
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 24;
	dwType = -1;
	ok( ERROR_MORE_DATA == SHQueryValueExA( hKey, "Test3", NULL, &dwType, buf, &dwSize), "");
	ok( 0 == strncmp(sExpTestpath2, buf, 24-1), "(%s)", buf);
	ok( 24-1 == strlen(buf), "(%s)", buf);
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)" , dwSize, nUsedBuffer2);
	ok( dwType == REG_SZ, "(%lu)" , dwType);

	/*
	 * The buffer is NULL but the size is set
	 */
	strcpy(buf, sEmptyBuffer);
	dwSize = 6;
	dwType = -1;
	dwRet = SHQueryValueExA( hKey, "Test3", NULL, &dwType, NULL, &dwSize);
	ok( ERROR_SUCCESS == dwRet, "(%lu)", dwRet);
	ok( dwSize == nUsedBuffer2, "(%lu,%lu)" , dwSize, nUsedBuffer2);
	ok( dwType == REG_SZ, "(%lu)" , dwType);


	RegCloseKey(hKey);
}

START_TEST(shreg)
{
	create_test_entrys();
	test_SHGetValue();
	test_SHQUeryValueEx();
	test_SHGetTegPath();
}
