/*
 * Tests for color profile functions
 *
 * Copyright 2004 Hans Leidekker
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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "wine/test.h"

static const char machine[] = "dummy";
static const WCHAR machineW[] = { 'd','u','m','m','y',0 };

/* Two common places to find the standard color space profile */
static const char profile1[] =
"c:\\windows\\system\\color\\srgb color space profile.icm";
static const char profile2[] =
"c:\\windows\\system32\\spool\\drivers\\color\\srgb color space profile.icm";

static const WCHAR profile1W[] =
{ 'c',':','\\','w','i','n','d','o','w','s','\\', 's','y','s','t','e','m',
  '\\','c','o','l','o','r','\\','s','r','g','b',' ','c','o','l','o','r',' ',
  's','p','a','c','e',' ','p','r','o','f','i','l','e','.','i','c','m',0 };
static const WCHAR profile2W[] =
{ 'c',':','\\','w','i','n','d','o','w','s','\\', 's','y','s','t','e','m','3','2',
  '\\','s','p','o','o','l','\\','d','r','i','v','e','r','s',
  '\\','c','o','l','o','r','\\','s','r','g','b',' ','c','o','l','o','r',' ',
  's','p','a','c','e',' ','p','r','o','f','i','l','e','.','i','c','m',0 };

static LPSTR standardprofile;
static LPWSTR standardprofileW;

static LPSTR testprofile;
static LPWSTR testprofileW;

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

static void MSCMS_basenameA( LPCSTR path, LPSTR name )
{
    INT i = strlen( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    strcpy( name, &path[i] );
}

static void MSCMS_basenameW( LPCWSTR path, LPWSTR name )
{
    INT i = lstrlenW( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    lstrcpyW( name, &path[i] );
}

static void test_GetColorDirectoryA()
{
    BOOL ret;
    DWORD size;
    char buffer[MAX_PATH];

    /* Parameter checks */

    size = 0;

    ret = GetColorDirectoryA( NULL, NULL, &size );
    ok( !ret, "GetColorDirectoryA() succeeded (%ld)\n", GetLastError() );

    size = 0;

    ret = GetColorDirectoryA( NULL, buffer, &size );
    ok( !ret, "GetColorDirectoryA() succeeded (%ld)\n", GetLastError() );

    size = 1;

    ret = GetColorDirectoryA( NULL, buffer, &size );
    ok( !ret, "GetColorDirectoryA() succeeded (%ld)\n", GetLastError() );

    size = sizeof(buffer);

    ret = GetColorDirectoryA( machine, buffer, &size );
    ok( !ret, "GetColorDirectoryA() succeeded (%ld)\n", GetLastError() );

    /* Functional checks */

    size = sizeof(buffer);

    ret = GetColorDirectoryA( NULL, buffer, &size );
    ok( ret, "GetColorDirectoryA() failed (%ld)\n", GetLastError() );
}

static void test_GetColorDirectoryW()
{
    BOOL ret;
    DWORD size;
    WCHAR buffer[MAX_PATH];

    /* Parameter checks */

    size = 0;

    ret = GetColorDirectoryW( NULL, NULL, &size );
    ok( !ret, "GetColorDirectoryW() succeeded (%ld)\n", GetLastError() );

    size = 0;

    ret = GetColorDirectoryW( NULL, buffer, &size );
    ok( !ret, "GetColorDirectoryW() succeeded (%ld)\n", GetLastError() );

    size = 1;

    ret = GetColorDirectoryW( NULL, buffer, &size );
    ok( !ret, "GetColorDirectoryW() succeeded (%ld)\n", GetLastError() );

    size = sizeof(buffer);

    ret = GetColorDirectoryW( machineW, buffer, &size );
    ok( !ret, "GetColorDirectoryW() succeeded (%ld)\n", GetLastError() );

    /* Functional checks */

    size = sizeof(buffer);

    ret = GetColorDirectoryW( NULL, buffer, &size );
    ok( ret, "GetColorDirectoryW() failed (%ld)\n", GetLastError() );
}

static void test_InstallColorProfileA()
{
    BOOL ret;

    /* Parameter checks */

    ret = InstallColorProfileA( NULL, NULL );
    ok( !ret, "InstallColorProfileA() succeeded (%ld)\n", GetLastError() );

    ret = InstallColorProfileA( machine, NULL );
    ok( !ret, "InstallColorProfileA() succeeded (%ld)\n", GetLastError() );

    ret = InstallColorProfileA( NULL, machine );
    ok( !ret, "InstallColorProfileA() succeeded (%ld)\n", GetLastError() );

    if (standardprofile)
    {
        ret = InstallColorProfileA( NULL, standardprofile );
        ok( ret, "InstallColorProfileA() failed (%ld)\n", GetLastError() );
    }

    /* Functional checks */

    if (testprofile)
    {
        CHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        CHAR slash[] = "\\";
        HANDLE handle;

        ret = InstallColorProfileA( NULL, testprofile );
        ok( ret, "InstallColorProfileA() failed (%ld)\n", GetLastError() );

        ret = GetColorDirectoryA( NULL, dest, &size );
        ok( ret, "GetColorDirectoryA() failed (%ld)\n", GetLastError() );

        MSCMS_basenameA( testprofile, base );

        strcat( dest, slash );
        strcat( dest, base );

        /* Check if the profile is really there */ 
        handle = CreateFileA( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle != INVALID_HANDLE_VALUE, "Couldn't find the profile (%ld)\n", GetLastError() );
        CloseHandle( handle );
        
        ret = UninstallColorProfileA( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileA() failed (%ld)\n", GetLastError() );
    }
}

static void test_InstallColorProfileW()
{
    BOOL ret;

    /* Parameter checks */

    ret = InstallColorProfileW( NULL, NULL );
    ok( !ret, "InstallColorProfileW() succeeded (%ld)\n", GetLastError() );

    ret = InstallColorProfileW( machineW, NULL );
    ok( !ret, "InstallColorProfileW() succeeded (%ld)\n", GetLastError() );

    ret = InstallColorProfileW( NULL, machineW );
    ok( !ret, "InstallColorProfileW() failed (%ld)\n", GetLastError() );

    if (standardprofileW)
    {
        ret = InstallColorProfileW( NULL, standardprofileW );
        ok( ret, "InstallColorProfileW() failed (%ld)\n", GetLastError() );
    }

    /* Functional checks */

    if (testprofileW)
    {
        WCHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        WCHAR slash[] = { '\\', 0 };
        HANDLE handle;

        ret = InstallColorProfileW( NULL, testprofileW );
        ok( ret, "InstallColorProfileW() failed (%ld)\n", GetLastError() );

        ret = GetColorDirectoryW( NULL, dest, &size );
        ok( ret, "GetColorDirectoryW() failed (%ld)\n", GetLastError() );

        MSCMS_basenameW( testprofileW, base );

        lstrcatW( dest, slash );
        lstrcatW( dest, base );

        /* Check if the profile is really there */
        handle = CreateFileW( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle != INVALID_HANDLE_VALUE, "Couldn't find the profile (%ld)\n", GetLastError() );
        CloseHandle( handle );

        ret = UninstallColorProfileW( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileW() failed (%ld)\n", GetLastError() );
    }
}

static void test_OpenColorProfileA()
{
    PROFILE profile;
    HPROFILE handle;

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = NULL;
    profile.cbDataSize = 0;

    /* Parameter checks */

    handle = OpenColorProfileA( NULL, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

    handle = OpenColorProfileA( &profile, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

    handle = OpenColorProfileA( &profile, PROFILE_READ, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

    handle = OpenColorProfileA( &profile, PROFILE_READWRITE, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

    ok ( !CloseColorProfile( NULL ), "CloseColorProfile() succeeded" );

    if (standardprofile)
    {
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = OpenColorProfileA( &profile, 0, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

        handle = OpenColorProfileA( &profile, PROFILE_READ, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

        handle = OpenColorProfileA( &profile, PROFILE_READ|PROFILE_READWRITE, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

        /* Functional checks */

        handle = OpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%ld)\n", GetLastError() );

        ok( CloseColorProfile( handle ), "CloseColorProfile() failed (%ld)\n", GetLastError() );
    }
}

static void test_OpenColorProfileW()
{
    PROFILE profile;
    HPROFILE handle;

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = NULL;
    profile.cbDataSize = 0;

    /* Parameter checks */

    handle = OpenColorProfileW( NULL, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

    handle = OpenColorProfileW( &profile, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

    handle = OpenColorProfileW( &profile, PROFILE_READ, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

    handle = OpenColorProfileW( &profile, PROFILE_READWRITE, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

    ok ( !CloseColorProfile( NULL ), "CloseColorProfile() succeeded" );

    if (standardprofileW)
    {
        profile.pProfileData = standardprofileW;
        profile.cbDataSize = lstrlenW(standardprofileW) * sizeof(WCHAR);

        handle = OpenColorProfileW( &profile, 0, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

        handle = OpenColorProfileW( &profile, PROFILE_READ, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

        handle = OpenColorProfileW( &profile, PROFILE_READ|PROFILE_READWRITE, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

        /* Functional checks */

        handle = OpenColorProfileW( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileW() failed (%ld)\n", GetLastError() );

        ok( CloseColorProfile( handle ), "CloseColorProfile() failed (%ld)\n", GetLastError() );
    }
}

static void test_UninstallColorProfileA()
{
    BOOL ret;

    /* Parameter checks */

    ret = UninstallColorProfileA( NULL, NULL, FALSE );
    ok( !ret, "UninstallColorProfileA() succeeded (%ld)\n", GetLastError() );

    ret = UninstallColorProfileA( machine, NULL, FALSE );
    ok( !ret, "UninstallColorProfileA() succeeded (%ld)\n", GetLastError() );

    /* Functional checks */

    if (testprofile)
    {
        CHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        CHAR slash[] = "\\";
        HANDLE handle;

        ret = InstallColorProfileA( NULL, testprofile );
        ok( ret, "InstallColorProfileA() failed (%ld)\n", GetLastError() );

        ret = GetColorDirectoryA( NULL, dest, &size );
        ok( ret, "GetColorDirectoryA() failed (%ld)\n", GetLastError() );

        MSCMS_basenameA( testprofile, base );

        strcat( dest, slash );
        strcat( dest, base );

        ret = UninstallColorProfileA( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileA() failed (%ld)\n", GetLastError() );

        /* Check if the profile is really gone */
        handle = CreateFileA( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle == INVALID_HANDLE_VALUE, "Found the profile (%ld)\n", GetLastError() );
        CloseHandle( handle );
    }
}

static void test_UninstallColorProfileW()
{
    BOOL ret;

    /* Parameter checks */

    ret = UninstallColorProfileW( NULL, NULL, FALSE );
    ok( !ret, "UninstallColorProfileW() succeeded (%ld)\n", GetLastError() );

    ret = UninstallColorProfileW( machineW, NULL, FALSE );
    ok( !ret, "UninstallColorProfileW() succeeded (%ld)\n", GetLastError() );

    /* Functional checks */

    if (testprofileW)
    {
        WCHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        WCHAR slash[] = { '\\', 0 };
        HANDLE handle;

        ret = InstallColorProfileW( NULL, testprofileW );
        ok( ret, "InstallColorProfileW() failed (%ld)\n", GetLastError() );

        ret = GetColorDirectoryW( NULL, dest, &size );
        ok( ret, "GetColorDirectoryW() failed (%ld)\n", GetLastError() );

        MSCMS_basenameW( testprofileW, base );

        lstrcatW( dest, slash );
        lstrcatW( dest, base );

        ret = UninstallColorProfileW( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileW() failed (%ld)\n", GetLastError() );

        /* Check if the profile is really gone */
        handle = CreateFileW( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle == INVALID_HANDLE_VALUE, "Found the profile (%ld)\n", GetLastError() );
        CloseHandle( handle );
    }
}

START_TEST(profile)
{
    UINT len;
    HANDLE handle;
    char path[MAX_PATH], file[MAX_PATH];
    WCHAR fileW[MAX_PATH];

    /* See if we can find the standard color profile */
    handle = CreateFileA( (LPCSTR)&profile1, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );

    if (handle != INVALID_HANDLE_VALUE)
    {
        standardprofile = (LPSTR)&profile1;
        standardprofileW = (LPWSTR)&profile1W;
        CloseHandle( handle );
    }

    handle = CreateFileA( (LPCSTR)&profile2, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );

    if (handle != INVALID_HANDLE_VALUE)
    {
        standardprofile = (LPSTR)&profile2;
        standardprofileW = (LPWSTR)&profile2W;
        CloseHandle( handle );
    }

    /* If found, create a temporary copy for testing purposes */
    if (standardprofile && GetTempPath( sizeof(path), path ))
    {
        if (GetTempFileName( path, "rgb", 0, file ))
        {
            if (CopyFileA( standardprofile, file, FALSE ))
            {

                testprofile = (LPSTR)&file;

                len = MultiByteToWideChar( CP_ACP, 0, testprofile, -1, NULL, 0 );
                MultiByteToWideChar( CP_ACP, 0, testprofile, -1, fileW, len );

                testprofileW = (LPWSTR)&fileW;
            }
        }
    }

    test_GetColorDirectoryA();
    test_GetColorDirectoryW();

    test_InstallColorProfileA();
    test_InstallColorProfileW();

    test_OpenColorProfileA();
    test_OpenColorProfileW();

    test_UninstallColorProfileA();
    test_UninstallColorProfileW();

    /* Clean up */
    if (testprofile)
        DeleteFileA( testprofile );
}
