/*
 * Copyright (c) 2005 Robert Reif
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

#define DIRECTINPUT_VERSION 0x0700

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dinput.h"
#include "dxerr8.h"
#include "dinput_test.h"

const char * get_file_version(const char * file_name)
{
    static char version[32];
    DWORD size;
    DWORD handle;

    size = GetFileVersionInfoSizeA(file_name, &handle);
    if (size) {
        char * data = HeapAlloc(GetProcessHeap(), 0, size);
        if (data) {
            if (GetFileVersionInfoA(file_name, handle, size, data)) {
                VS_FIXEDFILEINFO *pFixedVersionInfo;
                UINT len;
                if (VerQueryValueA(data, "\\", (LPVOID *)&pFixedVersionInfo, &len)) {
                    sprintf(version, "%ld.%ld.%ld.%ld",
                            pFixedVersionInfo->dwFileVersionMS >> 16,
                            pFixedVersionInfo->dwFileVersionMS & 0xffff,
                            pFixedVersionInfo->dwFileVersionLS >> 16,
                            pFixedVersionInfo->dwFileVersionLS & 0xffff);
                } else
                    sprintf(version, "not available");
            } else
                sprintf(version, "failed");

            HeapFree(GetProcessHeap(), 0, data);
        } else
            sprintf(version, "failed");
    } else
        sprintf(version, "not available");

    return version;
}

static void keyboard_tests(void)
{
}

START_TEST(keyboard)
{
    CoInitialize(NULL);

    trace("DLL Version: %s\n", get_file_version("dinput.dll"));

    keyboard_tests();

    CoUninitialize();
}
