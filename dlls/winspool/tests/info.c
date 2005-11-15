/*
 * Copyright (C) 2003, 2004 Stefan Leichter
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
#include "winspool.h"


static char env_x86[] = "Windows NT x86";
static char env_win9x_case[] = "windowS 4.0";


static void test_default_printer(void)
{
#define DEFAULT_PRINTER_SIZE 1000
    BOOL    retval;
    DWORD   exact = DEFAULT_PRINTER_SIZE;
    DWORD   size;
    FARPROC func = NULL;
    HMODULE lib = NULL;
    char    buffer[DEFAULT_PRINTER_SIZE];

    lib = GetModuleHandleA("winspool.drv");
    if (!lib) {
	ok( 0, "GetModuleHandleA(\"winspool.drv\") failed\n");
	return;
    }

    func = GetProcAddress( lib, "GetDefaultPrinterA");
    if (!func)
	/* only supported on NT like OSes starting with win2k */
	return;

    SetLastError(ERROR_SUCCESS);
    retval = func( buffer, &exact);
    if (!retval || !exact || !strlen(buffer) ||
	(ERROR_SUCCESS != GetLastError())) {
	if ((ERROR_FILE_NOT_FOUND == GetLastError()) ||
	    (ERROR_INVALID_NAME == GetLastError()))
	    trace("this test requires a default printer to be set\n");
	else {
		ok( 0, "function call GetDefaultPrinterA failed unexpected!\n"
		"function returned %s\n"
		"last error 0x%08lx\n"
		"returned buffer size 0x%08lx\n"
		"returned buffer content %s\n",
		retval ? "true" : "false", GetLastError(), exact, buffer);
	}
	return;
    }
    SetLastError(ERROR_SUCCESS);
    retval = func( NULL, NULL); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INVALID_PARAMETER == GetLastError(),
	"Last error wrong! ERROR_INVALID_PARAMETER expected, got 0x%08lx\n",
	GetLastError());

    SetLastError(ERROR_SUCCESS);
    retval = func( buffer, NULL); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INVALID_PARAMETER == GetLastError(),
	"Last error wrong! ERROR_INVALID_PARAMETER expected, got 0x%08lx\n",
	GetLastError());

    SetLastError(ERROR_SUCCESS);
    size = 0;
    retval = func( NULL, &size); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
	"Last error wrong! ERROR_INSUFFICIENT_BUFFER expected, got 0x%08lx\n",
	GetLastError());
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);

    SetLastError(ERROR_SUCCESS);
    size = DEFAULT_PRINTER_SIZE;
    retval = func( NULL, &size); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
	"Last error wrong! ERROR_INSUFFICIENT_BUFFER expected, got 0x%08lx\n",
	GetLastError());
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);

    size = 0;
    retval = func( buffer, &size); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
	"Last error wrong! ERROR_INSUFFICIENT_BUFFER expected, got 0x%08lx\n",
	GetLastError());
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);

    size = exact;
    retval = func( buffer, &size); 
    ok( retval, "function result wrong! True expected\n");
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);
}

static void test_printer_directory(void)
{   LPBYTE buffer = NULL;
    DWORD  cbBuf = 0, pcbNeeded = 0;
    BOOL   res;

    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, 0, &cbBuf);
    trace("GetPrinterDriverDirectoryA: first call returned 0x%04x, "
	  "buffer size 0x%08lx\n", res, cbBuf);

    if((res == 0) && (GetLastError() == RPC_S_SERVER_UNAVAILABLE))
    {
        trace("The Service 'Spooler' is required for this test\n");
        return;
    }
    ok((res == 0) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "returned %d with lasterror=%ld (expected '0' with " \
        "ERROR_INSUFFICIENT_BUFFER)\n", res, GetLastError());

    if (!cbBuf) {
        trace("no valid buffer size returned, skipping tests\n");
        return;
    }

    buffer = HeapAlloc( GetProcessHeap(), 0, cbBuf*2);
    if (buffer == NULL)  return ;

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf, &pcbNeeded);
    ok( res, "expected result != 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf*2, &pcbNeeded);
    ok( res, "expected result != 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);
 
    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf-1, &pcbNeeded);
    ok( !res , "expected result == 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);
    todo_wine {
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
        "last error set to %ld instead of ERROR_INSUFFICIENT_BUFFER\n",
        GetLastError());
    }
 
    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, &pcbNeeded);
    ok( (!res && ERROR_INVALID_USER_BUFFER == GetLastError()) || 
        ( res && ERROR_INVALID_PARAMETER == GetLastError()) ,
         "expected either result == 0 and "
         "last error == ERROR_INVALID_USER_BUFFER "
         "or result != 0 and last error == ERROR_INVALID_PARAMETER "
         "got result %d and last error == %ld\n", res, GetLastError());

    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf, NULL);
    ok( (!res && RPC_X_NULL_REF_POINTER == GetLastError()) || res,
         "expected either result == 0 and "
         "last error == RPC_X_NULL_REF_POINTER or result != 0 "
         "got result %d and last error == %ld\n", res, GetLastError());

    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, NULL);
    ok( (!res && RPC_X_NULL_REF_POINTER == GetLastError()) || 
        ( res && ERROR_INVALID_PARAMETER == GetLastError()) ,
         "expected either result == 0 and "
         "last error == RPC_X_NULL_REF_POINTER "
         "or result != 0 and last error == ERROR_INVALID_PARAMETER "
         "got result %d and last error == %ld\n", res, GetLastError());

    /* with a valid buffer, but level is to large */
    buffer[0] = '\0';
    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA(NULL, NULL, 2, buffer, cbBuf, &pcbNeeded);

    /* Level not checked in win9x and wine:*/
    if((res != FALSE) && buffer[0])
    {
        trace("invalid Level '2' not checked (valid Level is '1') => '%s'\n", 
                buffer);
    }
    else
    {
        ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "returned %d with lasterror=%ld (expected '0' with " \
        "ERROR_INVALID_LEVEL)\n", res, GetLastError());
    }

    /* printing environments are case insensitive */
    /* "Windows 4.0" is valid for win9x and NT */
    buffer[0] = '\0';
    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA(NULL, env_win9x_case, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        cbBuf = pcbNeeded;
        buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, cbBuf*2);
        if (buffer == NULL)  return ;

        SetLastError(0x00dead00);
        res = GetPrinterDriverDirectoryA(NULL, env_win9x_case, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);
    }
    
    todo_wine{
    ok(res && buffer[0], "returned %d with " \
        "lasterror=%ld and len=%d (expected '0' with 'len > 0')\n", 
        res, GetLastError(), lstrlenA((char *)buffer));
    }

    buffer[0] = '\0';
    SetLastError(0x00dead00);
    res = GetPrinterDriverDirectoryA(NULL, env_x86, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        cbBuf = pcbNeeded;
        buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, cbBuf*2);
        if (buffer == NULL)  return ;

        buffer[0] = '\0';
        SetLastError(0x00dead00);
        res = GetPrinterDriverDirectoryA(NULL, env_x86, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);
    }

    /* "Windows NT x86" is invalid for win9x */
    ok( (res && buffer[0]) ||
        (!res && (GetLastError() == ERROR_INVALID_ENVIRONMENT)), 
        "returned %d with lasterror=%ld and len=%d (expected '!= 0' with " \
        "'len > 0' or '0' with ERROR_INVALID_ENVIRONMENT)\n",
        res, GetLastError(), lstrlenA((char *)buffer));

    HeapFree( GetProcessHeap(), 0, buffer);
}

START_TEST(info)
{
    test_default_printer();
    test_printer_directory();
}
