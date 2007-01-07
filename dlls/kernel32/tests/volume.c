/*
 * Unit test suite
 *
 * Copyright 2006 Stefan Leichter
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

#include "wine/test.h"
#include "winbase.h"

#define CDROM   "CDROM"
#define FLOPPY  "FLOPPY"
#define HARDISK "HARDDISK"
#define LANMAN  "LANMANREDIRECTOR"
#define RAMDISK "RAMDISK"

static void test_query_dos_deviceA(void)
{
    char drivestr[] = "a:";
    char *p, buffer[2000];
    DWORD ret;
    for (;drivestr[0] <= 'z'; drivestr[0]++) {
        ret = QueryDosDeviceA( drivestr, buffer, sizeof(buffer));
        if(ret) {
            for (p = buffer; *p; p++) *p = toupper(*p);
            todo_wine
            ok( strstr( buffer, CDROM)   || strstr( buffer, FLOPPY) ||
                strstr( buffer, HARDISK) || strstr( buffer, LANMAN) ||
                strstr( buffer, RAMDISK), "expect the string %s contains %s,%s,%s,%s or %s\n",
                buffer, CDROM, FLOPPY, HARDISK, LANMAN, RAMDISK);
        }
    }
}

static void test_GetVolumeNameForVolumeMountPointA(void)
{
    BOOL ret;
    char volume[MAX_PATH], path[] = "c:\\";
    DWORD len = sizeof(volume);

    ret = GetVolumeNameForVolumeMountPointA(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    if (0) { /* these crash on XP */
    ret = GetVolumeNameForVolumeMountPointA(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    ret = GetVolumeNameForVolumeMountPointA(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    }

    ret = GetVolumeNameForVolumeMountPointA(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
}

static void test_GetVolumeNameForVolumeMountPointW(void)
{
    BOOL ret;
    WCHAR volume[MAX_PATH], path[] = {'c',':','\\',0};
    DWORD len = sizeof(volume) / sizeof(WCHAR);

    ret = GetVolumeNameForVolumeMountPointW(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    if (0) { /* these crash on XP */
    ret = GetVolumeNameForVolumeMountPointW(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");

    ret = GetVolumeNameForVolumeMountPointW(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");
    }

    ret = GetVolumeNameForVolumeMountPointW(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointW failed\n");
}

START_TEST(volume)
{
    test_query_dos_deviceA();
    test_GetVolumeNameForVolumeMountPointA();
    test_GetVolumeNameForVolumeMountPointW();
}
