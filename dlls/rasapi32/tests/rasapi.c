/*
* Unit test suite for rasapi32 functions
*
* Copyright 2008 Austin English
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
#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include "ras.h"

static HMODULE hmodule;
static DWORD (WINAPI *pRasEnumDevicesA)(LPRASDEVINFOA, LPDWORD, LPDWORD);

#define RASAPI32_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hmodule, #func); \
    if(!p ## func) \
        trace("GetProcAddress(%s) failed\n", #func);

static void InitFunctionPtrs(void)
{
    hmodule = LoadLibraryA("rasapi32.dll");

    RASAPI32_GET_PROC(RasEnumDevicesA)
}

static void test_rasenum(void)
{
    DWORD result;
    DWORD cDevices = 0;
    DWORD cb = 0;
    RASDEVINFOA rasDevInfo;
    rasDevInfo.dwSize = sizeof(rasDevInfo);

    if(!pRasEnumDevicesA) {
        win_skip("Skipping RasEnumDevicesA tests, function not present\n");
        return;
    }

    result = pRasEnumDevicesA(NULL, &cb, &cDevices);
    trace("RasEnumDevicesA: buffersize %d\n", cb);
    ok(result == ERROR_BUFFER_TOO_SMALL,
    "Expected ERROR_BUFFER_TOO_SMALL, got %08d\n", result);

    result = pRasEnumDevicesA(&rasDevInfo, NULL, &cDevices);
    ok(result == ERROR_INVALID_PARAMETER,
    "Expected ERROR_INVALID_PARAMETER, got %08d\n", result);
}

START_TEST(rasapi)
{
    InitFunctionPtrs();

    test_rasenum();
}
