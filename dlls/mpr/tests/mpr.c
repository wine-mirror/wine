/*
 * Copyright 2012 Andrew Eikum for CodeWeavers
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

#define COBJMACROS

#include <stdio.h>

#include "windows.h"
#include "winnetwk.h"
#include "wine/test.h"

static void test_WNetGetUniversalName(void)
{
    DWORD ret;
    char buffer[1024];
    DWORD drive_type, info_size;
    char driveA[] = "A:\\";
    WCHAR driveW[] = {'A',':','\\',0};

    for(; *driveA <= 'Z'; ++*driveA, ++*driveW){
        drive_type = GetDriveTypeW(driveW);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);

        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            /* WN_NO_NET_OR_BAD_PATH (DRIVE_FIXED) returned from the virtual drive (usual Q:)
               created by the microsoft application virtualization client */
            ok((ret == WN_NOT_CONNECTED) || (ret == WN_NO_NET_OR_BAD_PATH),
                "WNetGetUniversalNameA(%s, ...) returned %u (drive_type: %u)\n",
                driveA, ret, drive_type);

        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameW(driveW, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);

        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameW failed: %08x\n", ret);
        else
            ok((ret == WN_NOT_CONNECTED) || (ret == WN_NO_NET_OR_BAD_PATH),
                "WNetGetUniversalNameW(%s, ...) returned %u (drive_type: %u)\n",
                wine_dbgstr_w(driveW), ret, drive_type);

        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);
    }
}

START_TEST(mpr)
{
    test_WNetGetUniversalName();
}
