/*
 * Copyright (C) 2003 Stefan Leichter
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

static void test_printer_directory(ivoid)
{   LPBYTE buffer = NULL;
    DWORD  cbBuf, pcbNeeded;
    BOOL   res;

    (void) GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, 0, &cbBuf);

    buffer = HeapAlloc( GetProcessHeap(), 0, cbBuf*2);

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf, &pcbNeeded);
    ok( res, "expected result != 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf*2, &pcbNeeded);
    ok( res, "expected result != 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);
 
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf-1, &pcbNeeded);
    ok( !res , "expected result == 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
        "last error set to %ld instead of ERROR_INSUFFICIENT_BUFFER\n",
        GetLastError());
 
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, &pcbNeeded);
    ok( (!res && ERROR_INVALID_USER_BUFFER == GetLastError()) || 
        ( res && ERROR_INVALID_PARAMETER == GetLastError()) ,
         "expected either result == 0 and "
         "last error == ERROR_INVALID_USER_BUFFER "
         "or result != 0 and last error == ERROR_INVALID_PARAMETER "
         "got result %d and last error == %ld\n", res, GetLastError());

    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf, NULL);
    ok( (!res && RPC_X_NULL_REF_POINTER == GetLastError()) || res,
         "expected either result == 0 and "
         "last error == RPC_X_NULL_REF_POINTER or result != 0 "
         "got result %d and last error == %ld\n", res, GetLastError());

    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, NULL);
    ok( (!res && RPC_X_NULL_REF_POINTER == GetLastError()) || 
        ( res && ERROR_INVALID_PARAMETER == GetLastError()) ,
         "expected either result == 0 and "
         "last error == RPC_X_NULL_REF_POINTER "
         "or result != 0 and last error == ERROR_INVALID_PARAMETER "
         "got result %d and last error == %ld\n", res, GetLastError());

    HeapFree( GetProcessHeap(), 0, buffer);
}

START_TEST(info)
{
    test_printer_directory();
}
