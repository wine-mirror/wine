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

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winspool.h"

static void test_printer_directory(ivoid)
{   LPBYTE buffer = NULL;
    DWORD  cbBuf, pcbNeeded;
    BOOL   res;
    OSVERSIONINFOA ver;

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if(!GetVersionExA( &ver)) {
        ok( 0, "GetVersionExA failed!");
        return ;
    }

    (void) GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, 0, &cbBuf);

    buffer = HeapAlloc( GetProcessHeap(), 0, cbBuf*2);

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf, &pcbNeeded);
    ok( res, "expected result != 0, got %d", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld",
                            pcbNeeded, cbBuf);

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf*2, &pcbNeeded);
    ok( res, "expected result != 0, got %d", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld",
                            pcbNeeded, cbBuf);
 
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf-1, &pcbNeeded);
    ok( !res , "expected result == 0, got %d", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld",
                            pcbNeeded, cbBuf);
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
        "last error set to %ld instead of ERROR_INSUFFICIENT_BUFFER",
        GetLastError());
 
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, &pcbNeeded);
    if(ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ok( !res , "expected result == 0, got %d", res);
        ok( ERROR_INVALID_USER_BUFFER == GetLastError(),
            "last error set to %ld instead of ERROR_INVALID_USER_BUFFER",
             GetLastError());
    } else {
        ok( res , "expected result != 0, got %d", res);
        ok( ERROR_INVALID_PARAMETER == GetLastError(),
            "last error set to %ld instead of ERROR_INVALID_PARAMETER",
             GetLastError());
    }

    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf, NULL);
    if(ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ok( !res , "expected result == 0, got %d", res);
        ok( RPC_X_NULL_REF_POINTER == GetLastError(),
            "last error set to %ld instead of RPC_X_NULL_REF_POINTER",
            GetLastError());
    } else {
        ok( res , "expected result != 0, got %d", res);
    }

    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, NULL);
    if(ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ok( !res , "expected result == 0, got %d", res);
        ok( RPC_X_NULL_REF_POINTER == GetLastError(),
            "last error set to %ld instead of RPC_X_NULL_REF_POINTER",
            GetLastError());
    } else {
        ok( res , "expected result != 0, got %d", res);
        ok( ERROR_INVALID_PARAMETER == GetLastError(),
            "last error set to %ld instead of ERROR_INVALID_PARAMETER",
             GetLastError());
    }

    HeapFree( GetProcessHeap(), 0, buffer);
}

START_TEST(info)
{
    test_printer_directory();
}
