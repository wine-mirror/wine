/*
 * Unit test suite for version functions
 *
 * Copyright 2006 Robert Shearman
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

START_TEST(version)
{
    OSVERSIONINFOEX info = { sizeof(info) };
    BOOL ret;

    ret = VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION,
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    ret = VerifyVersionInfo(&info, VER_BUILDNUMBER | VER_MAJORVERSION |
        VER_MINORVERSION/* | VER_PLATFORMID | VER_SERVICEPACKMAJOR |
        VER_SERVICEPACKMINOR | VER_SUITENAME | VER_PRODUCT_TYPE */,
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());

    /* tests special handling of VER_SUITENAME */

    ret = VerifyVersionInfo(&info, VER_SUITENAME,
        VerSetConditionMask(0, VER_SUITENAME, VER_AND));
    ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    ret = VerifyVersionInfo(&info, VER_SUITENAME,
        VerSetConditionMask(0, VER_SUITENAME, VER_OR));
    ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    /* test handling of version numbers */
    
    /* v3.10 is always less than v4.x even
     * if the minor version is tested */
    info.dwMajorVersion = 3;
    info.dwMinorVersion = 10;
    ret = VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    info.dwMinorVersion = 0;
    info.wServicePackMajor = 10;
    ret = VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    info.wServicePackMajor = 0;
    info.wServicePackMinor = 10;
    ret = VerifyVersionInfo(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    GetVersionEx((OSVERSIONINFO *)&info);
    info.wServicePackMinor++;
    ret = VerifyVersionInfo(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());

    /* test the failure hierarchy for the four version fields */

    GetVersionEx((OSVERSIONINFO *)&info);
    info.wServicePackMajor++;
    ret = VerifyVersionInfo(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());

    GetVersionEx((OSVERSIONINFO *)&info);
    info.dwMinorVersion++;
    ret = VerifyVersionInfo(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());

    GetVersionEx((OSVERSIONINFO *)&info);
    info.dwMajorVersion++;
    ret = VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());

    ret = VerifyVersionInfo(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());


    /* shows that build number fits into the hierarchy after major version, but before minor version */
    GetVersionEx((OSVERSIONINFO *)&info);
    info.dwBuildNumber++;
    ret = VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());

    ret = VerifyVersionInfo(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(ret, "VerifyVersionInfo failed with error %ld\n", GetLastError());

    /* test bad dwOSVersionInfoSize */
    GetVersionEx((OSVERSIONINFO *)&info);
    info.dwOSVersionInfoSize = 0;
    ret = VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    todo_wine ok(!ret && (GetLastError() == ERROR_OLD_WIN_VERSION),
        "VerifyVersionInfo should have failed with ERROR_OLD_WIN_VERSION instead of %ld\n", GetLastError());
}
