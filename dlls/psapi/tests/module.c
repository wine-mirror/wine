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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "psapi.h"

/* Function ptrs */
static HMODULE dll;
static DWORD (WINAPI *pGetModuleBaseNameA)(HANDLE, HANDLE, LPSTR, DWORD);

static void test_module_base_name(void)
{   DWORD retval;
    char buffer[MAX_PATH];
    HMODULE self, modself;
    DWORD exact;

    if (!pGetModuleBaseNameA) return;

    self = OpenProcess( PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,
			FALSE, GetCurrentProcessId());
    if (!self) {
	ok(0, "OpenProcess() failed\n");
	return;
    }
    modself = GetModuleHandle(NULL);
    if (!modself) {
	ok(0, "GetModuleHandle() failed\n");
	return;
    }
    exact = pGetModuleBaseNameA( self, modself, buffer, MAX_PATH);
    if (!exact) {
	ok(0, "GetModuleBaseNameA failed unexpected with error 0x%08lx\n",
		GetLastError());
	return;
    }

    SetLastError(ERROR_SUCCESS);
    retval = pGetModuleBaseNameA( NULL, NULL, NULL, 0);
    ok(!retval, "function result wrong, got %ld expected 0\n", retval);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_INVALID_PARAMETER\n",
	GetLastError());

    SetLastError(ERROR_SUCCESS);
    retval = pGetModuleBaseNameA( NULL, NULL, NULL, MAX_PATH);
    ok(!retval, "function result wrong, got %ld expected 0\n", retval);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_INVALID_PARAMETER\n",
	GetLastError());

    SetLastError(ERROR_SUCCESS);
    retval = pGetModuleBaseNameA( NULL, NULL, buffer, 0);
    ok(!retval, "function result wrong, got %ld expected 0\n", retval);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_INVALID_PARAMETER\n",
	GetLastError());

    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetModuleBaseNameA( NULL, NULL, buffer, 1);
    ok(!retval, "function result wrong, got %ld expected 0\n", retval);
    ok(ERROR_INVALID_HANDLE == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_INVALID_HANDLE\n",
	GetLastError());

    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    /* GetModuleFileNameEx may need to be fixed first ? */
    retval = pGetModuleBaseNameA( self, NULL, buffer, 1);
    todo_wine ok(retval == 1, "function result wrong, got %ld expected 1\n", retval);
    todo_wine ok(ERROR_SUCCESS == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_SUCCESS\n",
	GetLastError());
    todo_wine ok(1 == strlen(buffer),
	"buffer content length wrong, got %d(%s) expected 1\n",
	strlen(buffer), buffer);

    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetModuleBaseNameA( self, modself, buffer, 1);
    ok(retval == 1, "function result wrong, got %ld expected 1\n", retval);
    ok(ERROR_SUCCESS == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_SUCCESS\n",
	GetLastError());
    ok(1 == strlen(buffer),
	"buffer content length wrong, got %d(%s) expected 1\n",
	strlen(buffer), buffer);

    SetLastError(ERROR_SUCCESS);
    memset(buffer, 0, sizeof(buffer));
    retval = pGetModuleBaseNameA( self, NULL, buffer, exact);
    /* GetModuleFileNameEx may need to be fixed first ? */
    todo_wine ok(retval == exact, 
	"function result wrong, got %ld expected %ld\n", retval, exact);
    todo_wine ok(ERROR_SUCCESS == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_SUCCESS\n",
	GetLastError());
    todo_wine ok(exact == strlen(buffer),
	"buffer content length wrong, got %d(%s) expected %ld\n",
	strlen(buffer), buffer, exact);

    SetLastError(ERROR_SUCCESS);
    memset(buffer, 0, sizeof(buffer));
    retval = pGetModuleBaseNameA( self, modself, buffer, exact);
    ok(retval == exact, 
	"function result wrong, got %ld expected %ld\n", retval, exact);
    ok(ERROR_SUCCESS == GetLastError(),
	"last error wrong, got 0x%08lx expected ERROR_SUCCESS\n",
	GetLastError());
    ok(exact == strlen(buffer),
	"buffer content length wrong, got %d(%s) expected %ld\n",
	strlen(buffer), buffer, exact);
}

START_TEST(module)
{
    dll = LoadLibrary("psapi.dll");
    ok(dll != 0, "LoadLibraryA failed\n");
    if (!dll) return;

    pGetModuleBaseNameA = (void*) GetProcAddress(dll, "GetModuleBaseNameA");

    test_module_base_name();

    FreeLibrary(dll);
}
