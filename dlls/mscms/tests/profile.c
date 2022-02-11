/*
 * Tests for color profile functions
 *
 * Copyright 2004, 2005, 2006 Hans Leidekker
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "wine/test.h"

static HMODULE hmscms;
static HMODULE huser32;

static BOOL     (WINAPI *pAssociateColorProfileWithDeviceA)(PCSTR,PCSTR,PCSTR);
static BOOL     (WINAPI *pCloseColorProfile)(HPROFILE);
static HTRANSFORM (WINAPI *pCreateMultiProfileTransform)(PHPROFILE,DWORD,PDWORD,DWORD,DWORD,DWORD);
static BOOL     (WINAPI *pDeleteColorTransform)(HTRANSFORM);
static BOOL     (WINAPI *pDisassociateColorProfileFromDeviceA)(PCSTR,PCSTR,PCSTR);
static BOOL     (WINAPI *pGetColorDirectoryA)(PCHAR,PCHAR,PDWORD);
static BOOL     (WINAPI *pGetColorDirectoryW)(PWCHAR,PWCHAR,PDWORD);
static BOOL     (WINAPI *pGetColorProfileElement)(HPROFILE,TAGTYPE,DWORD,PDWORD,PVOID,PBOOL);
static BOOL     (WINAPI *pGetColorProfileElementTag)(HPROFILE,DWORD,PTAGTYPE);
static BOOL     (WINAPI *pGetColorProfileFromHandle)(HPROFILE,PBYTE,PDWORD);
static BOOL     (WINAPI *pGetColorProfileHeader)(HPROFILE,PPROFILEHEADER);
static BOOL     (WINAPI *pGetCountColorProfileElements)(HPROFILE,PDWORD);
static BOOL     (WINAPI *pGetStandardColorSpaceProfileA)(PCSTR,DWORD,PSTR,PDWORD);
static BOOL     (WINAPI *pGetStandardColorSpaceProfileW)(PCWSTR,DWORD,PWSTR,PDWORD);
static BOOL     (WINAPI *pEnumColorProfilesA)(PCSTR,PENUMTYPEA,PBYTE,PDWORD,PDWORD);
static BOOL     (WINAPI *pEnumColorProfilesW)(PCWSTR,PENUMTYPEW,PBYTE,PDWORD,PDWORD);
static BOOL     (WINAPI *pInstallColorProfileA)(PCSTR,PCSTR);
static BOOL     (WINAPI *pInstallColorProfileW)(PCWSTR,PCWSTR);
static BOOL     (WINAPI *pIsColorProfileTagPresent)(HPROFILE,TAGTYPE,PBOOL);
static HPROFILE (WINAPI *pOpenColorProfileA)(PPROFILE,DWORD,DWORD,DWORD);
static HPROFILE (WINAPI *pOpenColorProfileW)(PPROFILE,DWORD,DWORD,DWORD);
static BOOL     (WINAPI *pSetColorProfileElement)(HPROFILE,TAGTYPE,DWORD,PDWORD,PVOID);
static BOOL     (WINAPI *pSetColorProfileHeader)(HPROFILE,PPROFILEHEADER);
static BOOL     (WINAPI *pSetStandardColorSpaceProfileA)(PCSTR,DWORD,PSTR);
static BOOL     (WINAPI *pSetStandardColorSpaceProfileW)(PCWSTR,DWORD,PWSTR);
static BOOL     (WINAPI *pUninstallColorProfileA)(PCSTR,PCSTR,BOOL);
static BOOL     (WINAPI *pUninstallColorProfileW)(PCWSTR,PCWSTR,BOOL);

static BOOL     (WINAPI *pEnumDisplayDevicesA)(LPCSTR,DWORD,PDISPLAY_DEVICEA,DWORD);

#define GETFUNCPTR(func) p##func = (void *)GetProcAddress( hmscms, #func ); \
    if (!p##func) return FALSE;

static BOOL init_function_ptrs( void )
{
    GETFUNCPTR( AssociateColorProfileWithDeviceA )
    GETFUNCPTR( CloseColorProfile )
    GETFUNCPTR( CreateMultiProfileTransform )
    GETFUNCPTR( DeleteColorTransform )
    GETFUNCPTR( DisassociateColorProfileFromDeviceA )
    GETFUNCPTR( GetColorDirectoryA )
    GETFUNCPTR( GetColorDirectoryW )
    GETFUNCPTR( GetColorProfileElement )
    GETFUNCPTR( GetColorProfileElementTag )
    GETFUNCPTR( GetColorProfileFromHandle )
    GETFUNCPTR( GetColorProfileHeader )
    GETFUNCPTR( GetCountColorProfileElements )
    GETFUNCPTR( GetStandardColorSpaceProfileA )
    GETFUNCPTR( GetStandardColorSpaceProfileW )
    GETFUNCPTR( EnumColorProfilesA )
    GETFUNCPTR( EnumColorProfilesW )
    GETFUNCPTR( InstallColorProfileA )
    GETFUNCPTR( InstallColorProfileW )
    GETFUNCPTR( IsColorProfileTagPresent )
    GETFUNCPTR( OpenColorProfileA )
    GETFUNCPTR( OpenColorProfileW )
    GETFUNCPTR( SetColorProfileElement )
    GETFUNCPTR( SetColorProfileHeader )
    GETFUNCPTR( SetStandardColorSpaceProfileA )
    GETFUNCPTR( SetStandardColorSpaceProfileW )
    GETFUNCPTR( UninstallColorProfileA )
    GETFUNCPTR( UninstallColorProfileW )

    pEnumDisplayDevicesA = (void *)GetProcAddress( huser32, "EnumDisplayDevicesA" );

    return TRUE;
}

static const char machine[] = "dummy";
static const WCHAR machineW[] = L"dummy";

/*  To do any real functionality testing with this suite you need a copy of
 *  the freely distributable standard RGB color space profile. It comes
 *  standard with Windows, but on Wine you probably need to install it yourself
 *  in one of the locations mentioned below.
 */

/* Two common places to find the standard color space profile, relative
 * to the system directory.
 */
static const char profile1[] =
"\\color\\srgb color space profile.icm";
static const char profile2[] =
"\\spool\\drivers\\color\\srgb color space profile.icm";

static const WCHAR profile1W[] =
L"\\color\\srgb color space profile.icm";
static const WCHAR profile2W[] =
L"\\spool\\drivers\\color\\srgb color space profile.icm";

static BOOL have_color_profile;

static const unsigned char rgbheader[] =
{ 0x48, 0x0c, 0x00, 0x00, 0x6f, 0x6e, 0x69, 0x4c, 0x00, 0x00, 0x10, 0x02,
  0x72, 0x74, 0x6e, 0x6d, 0x20, 0x42, 0x47, 0x52, 0x20, 0x5a, 0x59, 0x58,
  0x02, 0x00, 0xce, 0x07, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x31, 0x00,
  0x70, 0x73, 0x63, 0x61, 0x54, 0x46, 0x53, 0x4d, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x43, 0x45, 0x49, 0x42, 0x47, 0x52, 0x73, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd6, 0xf6, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x2d, 0xd3, 0x00, 0x00, 0x20, 0x20, 0x50, 0x48 };

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

static void test_GetColorDirectoryA(void)
{
    BOOL ret;
    DWORD size;
    char buffer[MAX_PATH];

    /* Parameter checks */

    ret = pGetColorDirectoryA( NULL, NULL, NULL );
    ok( !ret, "GetColorDirectoryA() succeeded (%lu)\n", GetLastError() );

    size = 0;
    ret = pGetColorDirectoryA( NULL, NULL, &size );
    ok( !ret && size > 0, "GetColorDirectoryA() succeeded (%lu)\n", GetLastError() );

    size = 0;
    ret = pGetColorDirectoryA( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryA() succeeded (%lu)\n", GetLastError() );

    size = 1;
    ret = pGetColorDirectoryA( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryA() succeeded (%lu)\n", GetLastError() );

    /* Functional checks */

    size = sizeof(buffer);
    ret = pGetColorDirectoryA( NULL, buffer, &size );
    ok( ret && size > 0, "GetColorDirectoryA() failed (%lu)\n", GetLastError() );
}

static void test_GetColorDirectoryW(void)
{
    BOOL ret;
    DWORD size;
    WCHAR buffer[MAX_PATH];

    /* Parameter checks */

    /* This one crashes win2k
    ret = pGetColorDirectoryW( NULL, NULL, NULL );
    ok( !ret, "GetColorDirectoryW() succeeded (%lu)\n", GetLastError() );
     */

    size = 0;
    ret = pGetColorDirectoryW( NULL, NULL, &size );
    ok( !ret && size > 0, "GetColorDirectoryW() succeeded (%lu)\n", GetLastError() );

    size = 0;
    ret = pGetColorDirectoryW( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryW() succeeded (%lu)\n", GetLastError() );

    size = 1;
    ret = pGetColorDirectoryW( NULL, buffer, &size );
    ok( !ret && size > 0, "GetColorDirectoryW() succeeded (%lu)\n", GetLastError() );

    /* Functional checks */

    size = sizeof(buffer);
    ret = pGetColorDirectoryW( NULL, buffer, &size );
    ok( ret && size > 0, "GetColorDirectoryW() failed (%lu)\n", GetLastError() );
}

static void test_GetColorProfileElement( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret, ref;
        DWORD size;
        TAGTYPE tag = 0x63707274;  /* 'cprt' */
        char buffer[256];
        static const char expect[] = "text\0\0\0\0Copyright";

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetColorProfileElement( handle, tag, 0, NULL, NULL, &ref );
        ok( !ret, "GetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        ret = pGetColorProfileElement( handle, tag, 0, &size, NULL, NULL );
        ok( !ret, "GetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        size = 0;
        ret = pGetColorProfileElement( handle, tag, 0, &size, NULL, &ref );
        ok( !ret, "GetColorProfileElement() succeeded\n" );
        ok( size > 0, "wrong size\n" );

        /* Functional checks */

        size = sizeof(buffer);
        ret = pGetColorProfileElement( handle, tag, 0, &size, buffer, &ref );
        ok( ret, "GetColorProfileElement() failed %lu\n", GetLastError() );
        ok( size > 0, "wrong size\n" );
        ok( !memcmp( buffer, expect, sizeof(expect)-1 ), "Unexpected tag data\n" );

        pCloseColorProfile( handle );
    }
}

static void test_GetColorProfileElementTag( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        DWORD index = 1;
        TAGTYPE tag, expect = 0x63707274;  /* 'cprt' */

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetColorProfileElementTag( NULL, index, &tag );
        ok( !ret, "GetColorProfileElementTag() succeeded (%lu)\n", GetLastError() );

        ret = pGetColorProfileElementTag( handle, 0, &tag );
        ok( !ret, "GetColorProfileElementTag() succeeded (%lu)\n", GetLastError() );

        ret = pGetColorProfileElementTag( handle, index, NULL );
        ok( !ret, "GetColorProfileElementTag() succeeded (%lu)\n", GetLastError() );

        ret = pGetColorProfileElementTag( handle, 18, NULL );
        ok( !ret, "GetColorProfileElementTag() succeeded (%lu)\n", GetLastError() );

        /* Functional checks */

        while ((ret = pGetColorProfileElementTag( handle, index, &tag )) && tag != expect) index++;
        ok( ret && tag == expect, "GetColorProfileElementTag() failed (%lu)\n", GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_GetColorProfileFromHandle( char *testprofile )
{
    if (testprofile)
    {
        PROFILEHEADER *header;
        PROFILE profile;
        HPROFILE handle;
        DWORD size;
        BOOL ret;
        unsigned char *buffer;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Parameter checks */

        size = 0;

        ret = pGetColorProfileFromHandle( handle, NULL, &size );
        ok( !ret && size > 0, "GetColorProfileFromHandle() failed (%lu)\n", GetLastError() );

        buffer = HeapAlloc( GetProcessHeap(), 0, size );

        if (buffer)
        {
            ret = pGetColorProfileFromHandle( NULL, buffer, &size );
            ok( !ret, "GetColorProfileFromHandle() succeeded (%lu)\n", GetLastError() );

            ret = pGetColorProfileFromHandle( handle, buffer, NULL );
            ok( !ret, "GetColorProfileFromHandle() succeeded (%lu)\n", GetLastError() );

            /* Functional checks */

            ret = pGetColorProfileFromHandle( handle, buffer, &size );
            ok( ret && size > 0, "GetColorProfileFromHandle() failed (%lu)\n", GetLastError() );

            header = (PROFILEHEADER *)buffer;
            ok( header->phClass == 0x72746e6d, "wrong phClass %lx\n", header->phClass );
            ok( header->phDataColorSpace == 0x20424752, "wrong phDataColorSpace %lx\n", header->phDataColorSpace );
            ok( header->phConnectionSpace  == 0x205a5958, "wrong phConnectionSpace %lx\n", header->phConnectionSpace );
            ok( header->phSignature == 0x70736361, "wrong phSignature %lx\n", header->phSignature );
            ok( header->phProfileFlags == 0x00000000, "wrong phProfileFlags %lx\n", header->phProfileFlags );
            ok( header->phRenderingIntent == 0x00000000, "wrong phRenderingIntent %lx\n", header->phRenderingIntent );
            ok( header->phIlluminant.ciexyzX == 0xd6f60000, "wrong phIlluminant.ciexyzX %lx\n", header->phIlluminant.ciexyzX );
            ok( header->phIlluminant.ciexyzY == 0x00000100, "wrong phIlluminant.ciexyzY %lx\n", header->phIlluminant.ciexyzY );
            ok( header->phIlluminant.ciexyzZ == 0x2dd30000, "wrong phIlluminant.ciexyzZ %lx\n", header->phIlluminant.ciexyzZ );

            HeapFree( GetProcessHeap(), 0, buffer );
        }

        pCloseColorProfile( handle );
    }
}

static void test_GetColorProfileHeader( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        PROFILEHEADER header;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetColorProfileHeader( NULL, NULL );
        ok( !ret, "GetColorProfileHeader() succeeded (%lu)\n", GetLastError() );

        ret = pGetColorProfileHeader( NULL, &header );
        ok( !ret, "GetColorProfileHeader() succeeded (%lu)\n", GetLastError() );

        if (0) /* Crashes on Vista */
        {
            ret = pGetColorProfileHeader( handle, NULL );
            ok( !ret, "GetColorProfileHeader() succeeded (%lu)\n", GetLastError() );
        }

        /* Functional checks */

        ret = pGetColorProfileHeader( handle, &header );
        ok( ret, "GetColorProfileHeader() failed (%lu)\n", GetLastError() );

        ok( header.phClass == 0x6d6e7472, "wrong phClass %#lx\n", header.phClass );
        ok( header.phDataColorSpace == 0x52474220, "wrong phDataColorSpace %#lx\n", header.phDataColorSpace );
        ok( header.phConnectionSpace  == 0x58595a20, "wrong phConnectionSpace %#lx\n", header.phConnectionSpace );
        ok( header.phSignature == 0x61637370, "wrong phSignature %#lx\n", header.phSignature );
        ok( header.phProfileFlags == 0x00000000, "wrong phProfileFlags %#lx\n", header.phProfileFlags );
        ok( header.phRenderingIntent == 0x00000000, "wrong phRenderingIntent %#lx\n", header.phRenderingIntent );
        ok( header.phIlluminant.ciexyzX == 0x0000f6d6, "wrong phIlluminant.ciexyzX %#lx\n", header.phIlluminant.ciexyzX );
        ok( header.phIlluminant.ciexyzY == 0x00010000, "wrong phIlluminant.ciexyzY %#lx\n", header.phIlluminant.ciexyzY );
        ok( header.phIlluminant.ciexyzZ == 0x0000d32d, "wrong phIlluminant.ciexyzZ %#lx\n", header.phIlluminant.ciexyzZ );

        pCloseColorProfile( handle );
    }
}

static void test_GetCountColorProfileElements( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        DWORD count;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Parameter checks */

        ret = pGetCountColorProfileElements( NULL, &count );
        ok( !ret, "GetCountColorProfileElements() succeeded (%lu)\n", GetLastError() );

        ret = pGetCountColorProfileElements( handle, NULL );
        ok( !ret, "GetCountColorProfileElements() succeeded (%lu)\n", GetLastError() );

        /* Functional checks */

        ret = pGetCountColorProfileElements( handle, &count );
        ok( ret && count > 15 && count < 20,
            "GetCountColorProfileElements() failed (%lu) %lu\n", GetLastError(), count );

        pCloseColorProfile( handle );
    }
}

static void test_GetStandardColorSpaceProfileA( char *standardprofile )
{
    BOOL ret;
    DWORD size;
    CHAR oldprofile[MAX_PATH];
    CHAR newprofile[MAX_PATH];

    /* Parameter checks */

    /* Single invalid parameter checks: */

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 1st param, */
    ret = pGetStandardColorSpaceProfileA(machine, LCS_sRGB, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_NOT_SUPPORTED, "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 2nd param, */
    ret = pGetStandardColorSpaceProfileA(NULL, (DWORD)-1, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, LCS_sRGB, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 3rd param, */
    ret = pGetStandardColorSpaceProfileA(NULL, LCS_sRGB, NULL, &size);
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, LCS_sRGB, newprofile, &size);
    ok( !ret && (GetLastError() == ERROR_MORE_DATA || GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n", ret, GetLastError() );

    /* Several invalid parameter checks: */

    size = 0;
    SetLastError(0xfaceabee); /* 1st, maybe 2nd and then dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileA(machine, 0, newprofile, &size);
    ok( !ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED),
        "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n", ret, GetLastError() );

    SetLastError(0xfaceabee); /* maybe 2nd and then 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, 0, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* maybe 2nd, then 3rd and dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileA(NULL, 0, NULL, &size);
    ok( !ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_FILE_NOT_FOUND),
        "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* maybe 2nd param. */
    ret = pGetStandardColorSpaceProfileA(NULL, 0, newprofile, &size);
    if (!ret) ok( GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n",
        ret, GetLastError() );
    else ok( !lstrcmpiA( newprofile, "" ) && GetLastError() == 0xfaceabee,
             "GetStandardColorSpaceProfileA() returns %d (GLE=%lu)\n", ret, GetLastError() );

    /* Functional checks */

    size = sizeof(oldprofile);
    ret = pGetStandardColorSpaceProfileA( NULL, LCS_sRGB, oldprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileA() failed (%lu)\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pSetStandardColorSpaceProfileA( NULL, LCS_sRGB, standardprofile );
    if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights for SetStandardColorSpaceProfileA\n");
        return;
    }
    ok( ret, "SetStandardColorSpaceProfileA() failed (%lu)\n", GetLastError() );

    size = sizeof(newprofile);
    ret = pGetStandardColorSpaceProfileA( NULL, LCS_sRGB, newprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileA() failed (%lu)\n", GetLastError() );

    ret = pSetStandardColorSpaceProfileA( NULL, LCS_sRGB, oldprofile );
    ok( ret, "SetStandardColorSpaceProfileA() failed (%lu)\n", GetLastError() );
}

static void test_GetStandardColorSpaceProfileW( WCHAR *standardprofileW )
{
    BOOL ret;
    DWORD size;
    WCHAR oldprofile[MAX_PATH];
    WCHAR newprofile[MAX_PATH];
    CHAR newprofileA[MAX_PATH];

    /* Parameter checks */

    /* Single invalid parameter checks: */

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 1st param, */
    ret = pGetStandardColorSpaceProfileW(machineW, LCS_sRGB, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_NOT_SUPPORTED, "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 2nd param, */
    ret = pGetStandardColorSpaceProfileW(NULL, (DWORD)-1, newprofile, &size);
    ok( !ret && GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 2nd param, */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, newprofile, &size);
    ok( (!ret && GetLastError() == ERROR_FILE_NOT_FOUND) ||
        broken(ret), /* Win98 and WinME */
        "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n", ret, GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 3rd param, */
    ret = pGetStandardColorSpaceProfileW(NULL, LCS_sRGB, NULL, &size);
    ok( !ret || broken(ret) /* win98 */, "GetStandardColorSpaceProfileW succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        broken(GetLastError() == 0xfaceabee) /* win98 */,
        "GetStandardColorSpaceProfileW() returns GLE=%lu\n", GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* 4th param, */
    ret = pGetStandardColorSpaceProfileW(NULL, LCS_sRGB, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* dereferenced 4th param. */
    ret = pGetStandardColorSpaceProfileW(NULL, LCS_sRGB, newprofile, &size);
    ok( !ret || broken(ret) /* win98 */, "GetStandardColorSpaceProfileW succeeded\n" );
    ok( GetLastError() == ERROR_MORE_DATA ||
        GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        broken(GetLastError() == 0xfaceabee) /* win98 */,
        "GetStandardColorSpaceProfileW() returns GLE=%lu\n", GetLastError() );

    /* Several invalid parameter checks: */

    size = 0;
    SetLastError(0xfaceabee); /* 1st, maybe 2nd and then dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileW(machineW, 0, newprofile, &size);
    ok( !ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED),
        "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n", ret, GetLastError() );

    SetLastError(0xfaceabee); /* maybe 2nd and then 4th param, */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, newprofile, NULL);
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER, "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n",
        ret, GetLastError() );

    size = 0;
    SetLastError(0xfaceabee); /* maybe 2nd, then 3rd and dereferenced 4th param, */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, NULL, &size);
    ok( !ret || broken(ret) /* win98 */, "GetStandardColorSpaceProfileW succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == ERROR_FILE_NOT_FOUND ||
        broken(GetLastError() == 0xfaceabee) /* win98 */,
        "GetStandardColorSpaceProfileW() returns GLE=%lu\n", GetLastError() );

    size = sizeof(newprofile);
    SetLastError(0xfaceabee); /* maybe 2nd param. */
    ret = pGetStandardColorSpaceProfileW(NULL, 0, newprofile, &size);
    if (!ret) ok( GetLastError() == ERROR_FILE_NOT_FOUND, "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n",
                  ret, GetLastError() );
    else
    {
        WideCharToMultiByte(CP_ACP, 0, newprofile, -1, newprofileA, sizeof(newprofileA), NULL, NULL);
        ok( !lstrcmpiA( newprofileA, "" ) && GetLastError() == 0xfaceabee,
             "GetStandardColorSpaceProfileW() returns %d (GLE=%lu)\n", ret, GetLastError() );
    }

    /* Functional checks */

    size = sizeof(oldprofile);
    ret = pGetStandardColorSpaceProfileW( NULL, LCS_sRGB, oldprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileW() failed (%lu)\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pSetStandardColorSpaceProfileW( NULL, LCS_sRGB, standardprofileW );
    if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights for SetStandardColorSpaceProfileW\n");
        return;
    }
    ok( ret, "SetStandardColorSpaceProfileW() failed (%lu)\n", GetLastError() );

    size = sizeof(newprofile);
    ret = pGetStandardColorSpaceProfileW( NULL, LCS_sRGB, newprofile, &size );
    ok( ret, "GetStandardColorSpaceProfileW() failed (%lu)\n", GetLastError() );

    ret = pSetStandardColorSpaceProfileW( NULL, LCS_sRGB, oldprofile );
    ok( ret, "SetStandardColorSpaceProfileW() failed (%lu)\n", GetLastError() );
}

static void test_EnumColorProfilesA( char *standardprofile )
{
    BOOL ret;
    DWORD total, size, number;
    ENUMTYPEA record;
    BYTE *buffer;

    /* Parameter checks */

    memset( &record, 0, sizeof(ENUMTYPEA) );

    record.dwSize = sizeof(ENUMTYPEA);
    record.dwVersion = ENUM_TYPE_VERSION;
    record.dwFields |= ET_DATACOLORSPACE;
    record.dwDataColorSpace = SPACE_RGB;

    total = 0;
    SetLastError( 0xdeadbeef );
    ret = pEnumColorProfilesA( NULL, &record, NULL, &total, &number );
    ok( !ret, "EnumColorProfilesA succeeded\n" );
    if (have_color_profile) ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", GetLastError() );
    buffer = HeapAlloc( GetProcessHeap(), 0, total );

    size = total;
    ret = pEnumColorProfilesA( machine, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesA succeeded\n" );

    ret = pEnumColorProfilesA( NULL, NULL, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesA succeeded\n" );

    ret = pEnumColorProfilesA( NULL, &record, buffer, NULL, &number );
    ok( !ret, "EnumColorProfilesA succeeded\n" );

    ret = pEnumColorProfilesA( NULL, &record, buffer, &size, &number );
    todo_wine_if (!have_color_profile)
        ok( ret, "EnumColorProfilesA failed %lu\n", GetLastError() );

    size = 0;
    ret = pEnumColorProfilesA( NULL, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesA succeeded\n" );

    /* Functional checks */

    size = total;
    ret = pEnumColorProfilesA( NULL, &record, buffer, &size, &number );
    todo_wine_if (!have_color_profile)
        ok( ret, "EnumColorProfilesA failed %lu\n", GetLastError() );

    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_EnumColorProfilesW( WCHAR *standardprofileW )
{
    BOOL ret;
    DWORD total, size, number;
    ENUMTYPEW record;
    BYTE *buffer;

    /* Parameter checks */

    memset( &record, 0, sizeof(ENUMTYPEW) );

    record.dwSize = sizeof(ENUMTYPEW);
    record.dwVersion = ENUM_TYPE_VERSION;
    record.dwFields |= ET_DATACOLORSPACE;
    record.dwDataColorSpace = SPACE_RGB;

    total = 0;
    SetLastError( 0xdeadbeef );
    ret = pEnumColorProfilesW( NULL, &record, NULL, &total, &number );
    ok( !ret, "EnumColorProfilesW succeeded\n" );
    if (have_color_profile) ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", GetLastError() );
    buffer = HeapAlloc( GetProcessHeap(), 0, total * sizeof(WCHAR) );

    size = total;
    ret = pEnumColorProfilesW( machineW, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesW succeeded\n" );

    ret = pEnumColorProfilesW( NULL, NULL, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesW succeeded\n" );

    ret = pEnumColorProfilesW( NULL, &record, buffer, NULL, &number );
    ok( !ret, "EnumColorProfilesW succeeded\n" );

    ret = pEnumColorProfilesW( NULL, &record, buffer, &size, &number );
    todo_wine_if (!have_color_profile)
        ok( ret, "EnumColorProfilesW failed %lu\n", GetLastError() );

    size = 0;
    ret = pEnumColorProfilesW( NULL, &record, buffer, &size, &number );
    ok( !ret, "EnumColorProfilesW succeeded\n" );

    /* Functional checks */

    size = total;
    ret = pEnumColorProfilesW( NULL, &record, buffer, &size, &number );
    todo_wine_if (!have_color_profile)
        ok( ret, "EnumColorProfilesW failed %lu\n", GetLastError() );

    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_InstallColorProfileA( char *standardprofile, char *testprofile )
{
    BOOL ret;

    /* Parameter checks */

    ret = pInstallColorProfileA( NULL, NULL );
    ok( !ret, "InstallColorProfileA() succeeded (%lu)\n", GetLastError() );

    ret = pInstallColorProfileA( machine, NULL );
    ok( !ret, "InstallColorProfileA() succeeded (%lu)\n", GetLastError() );

    ret = pInstallColorProfileA( NULL, machine );
    ok( !ret, "InstallColorProfileA() succeeded (%lu)\n", GetLastError() );

    if (standardprofile)
    {
        ret = pInstallColorProfileA( NULL, standardprofile );
        ok( ret, "InstallColorProfileA() failed (%lu)\n", GetLastError() );
    }

    /* Functional checks */

    if (testprofile)
    {
        CHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        HANDLE handle;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileA( NULL, testprofile );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileA\n");
            return;
        }
        ok( ret, "InstallColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pGetColorDirectoryA( NULL, dest, &size );
        ok( ret, "GetColorDirectoryA() failed (%lu)\n", GetLastError() );

        MSCMS_basenameA( testprofile, base );

        lstrcatA( dest, "\\" );
        lstrcatA( dest, base );

        /* Check if the profile is really there */ 
        handle = CreateFileA( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle != INVALID_HANDLE_VALUE, "Couldn't find the profile (%lu)\n", GetLastError() );
        CloseHandle( handle );
        
        ret = pUninstallColorProfileA( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileA() failed (%lu)\n", GetLastError() );
    }
}

static void test_InstallColorProfileW( WCHAR *standardprofileW, WCHAR *testprofileW )
{
    BOOL ret;

    /* Parameter checks */

    ret = pInstallColorProfileW( NULL, NULL );
    ok( !ret, "InstallColorProfileW() succeeded (%lu)\n", GetLastError() );

    ret = pInstallColorProfileW( machineW, NULL );
    ok( !ret, "InstallColorProfileW() succeeded (%lu)\n", GetLastError() );

    ret = pInstallColorProfileW( NULL, machineW );
    ok( !ret, "InstallColorProfileW() failed (%lu)\n", GetLastError() );

    if (standardprofileW)
    {
        ret = pInstallColorProfileW( NULL, standardprofileW );
        ok( ret, "InstallColorProfileW() failed (%lu)\n", GetLastError() );
    }

    /* Functional checks */

    if (testprofileW)
    {
        WCHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        HANDLE handle;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileW( NULL, testprofileW );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileW\n");
            return;
        }
        ok( ret, "InstallColorProfileW() failed (%lu)\n", GetLastError() );

        ret = pGetColorDirectoryW( NULL, dest, &size );
        ok( ret, "GetColorDirectoryW() failed (%lu)\n", GetLastError() );

        MSCMS_basenameW( testprofileW, base );

        lstrcatW( dest, L"\\" );
        lstrcatW( dest, base );

        /* Check if the profile is really there */
        handle = CreateFileW( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle != INVALID_HANDLE_VALUE, "Couldn't find the profile (%lu)\n", GetLastError() );
        CloseHandle( handle );

        ret = pUninstallColorProfileW( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileW() failed (%lu)\n", GetLastError() );
    }
}

static void test_IsColorProfileTagPresent( char *standardprofile )
{
    if (standardprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret, present;
        TAGTYPE tag;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Parameter checks */

        tag = 0;

        ret = pIsColorProfileTagPresent( handle, tag, &present );
        ok( !(ret && present), "IsColorProfileTagPresent() succeeded (%lu)\n", GetLastError() );

        tag = 0x63707274;  /* 'cprt' */

        ret = pIsColorProfileTagPresent( NULL, tag, &present );
        ok( !ret, "IsColorProfileTagPresent() succeeded (%lu)\n", GetLastError() );

        ret = pIsColorProfileTagPresent( handle, tag, NULL );
        ok( !ret, "IsColorProfileTagPresent() succeeded (%lu)\n", GetLastError() );

        /* Functional checks */

        ret = pIsColorProfileTagPresent( handle, tag, &present );
        ok( ret && present, "IsColorProfileTagPresent() failed (%lu)\n", GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_OpenColorProfileA( char *standardprofile )
{
    PROFILE profile;
    HPROFILE handle;
    BOOL ret;

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = NULL;
    profile.cbDataSize = 0;

    /* Parameter checks */

    handle = pOpenColorProfileA( NULL, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

    handle = pOpenColorProfileA( &profile, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

    handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

    handle = pOpenColorProfileA( &profile, PROFILE_READWRITE, 0, 0 );
    ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

    ok ( !pCloseColorProfile( NULL ), "CloseColorProfile() succeeded\n" );

    if (standardprofile)
    {
        profile.pProfileData = standardprofile;
        profile.cbDataSize = strlen(standardprofile);

        handle = pOpenColorProfileA( &profile, 0, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        handle = pOpenColorProfileA( &profile, PROFILE_READ|PROFILE_READWRITE, 0, 0 );
        ok( handle == NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        /* Functional checks */

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pCloseColorProfile( handle );
        ok( ret, "CloseColorProfile() failed (%lu)\n", GetLastError() );

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = (void *)"sRGB Color Space Profile.icm";
        profile.cbDataSize = sizeof("sRGB Color Space Profile.icm");

        handle = pOpenColorProfileA( &profile, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pCloseColorProfile( handle );
        ok( ret, "CloseColorProfile() failed (%lu)\n", GetLastError() );
    }
}

static void test_OpenColorProfileW( WCHAR *standardprofileW )
{
    PROFILE profile;
    HPROFILE handle;
    BOOL ret;

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = NULL;
    profile.cbDataSize = 0;

    /* Parameter checks */

    handle = pOpenColorProfileW( NULL, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

    handle = pOpenColorProfileW( &profile, 0, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

    handle = pOpenColorProfileW( &profile, PROFILE_READ, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

    handle = pOpenColorProfileW( &profile, PROFILE_READWRITE, 0, 0 );
    ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

    ok ( !pCloseColorProfile( NULL ), "CloseColorProfile() succeeded\n" );

    if (standardprofileW)
    {
        profile.pProfileData = standardprofileW;
        profile.cbDataSize = lstrlenW(standardprofileW) * sizeof(WCHAR);

        handle = pOpenColorProfileW( &profile, 0, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

        handle = pOpenColorProfileW( &profile, PROFILE_READ, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

        handle = pOpenColorProfileW( &profile, PROFILE_READ|PROFILE_READWRITE, 0, 0 );
        ok( handle == NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

        /* Functional checks */

        handle = pOpenColorProfileW( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileW() failed (%lu)\n", GetLastError() );

        ret = pCloseColorProfile( handle );
        ok( ret, "CloseColorProfile() failed (%lu)\n", GetLastError() );
    }
}

static void test_SetColorProfileElement( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        DWORD size;
        BOOL ret, ref;

        TAGTYPE tag = 0x63707274;  /* 'cprt' */
        static char data[] = "(c) The Wine Project";
        char buffer[256];

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        /* Parameter checks */

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, tag, 0, &size, data );
        ok( !ret, "SetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        pCloseColorProfile( handle );

        handle = pOpenColorProfileA( &profile, PROFILE_READWRITE, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pSetColorProfileElement( NULL, 0, 0, NULL, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, 0, 0, NULL, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, tag, 0, NULL, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        ret = pSetColorProfileElement( handle, tag, 0, &size, NULL );
        ok( !ret, "SetColorProfileElement() succeeded (%lu)\n", GetLastError() );

        /* Functional checks */

        size = sizeof(data);
        ret = pSetColorProfileElement( handle, tag, 0, &size, data );
        ok( ret, "SetColorProfileElement() failed %lu\n", GetLastError() );

        size = sizeof(buffer);
        ret = pGetColorProfileElement( handle, tag, 0, &size, buffer, &ref );
        ok( ret, "GetColorProfileElement() failed %lu\n", GetLastError() );
        ok( size > 0, "wrong size\n" );

        ok( !memcmp( data, buffer, sizeof(data) ),
            "Unexpected tag data, expected %s, got %s (%lu)\n", data, buffer, GetLastError() );

        pCloseColorProfile( handle );
    }
}

static void test_SetColorProfileHeader( char *testprofile )
{
    if (testprofile)
    {
        PROFILE profile;
        HPROFILE handle;
        BOOL ret;
        PROFILEHEADER header;

        profile.dwType = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize = strlen(testprofile);

        header.phSize = 0x00000c48;
        header.phCMMType = 0x4c696e6f;
        header.phVersion = 0x02100000;
        header.phClass = 0x6d6e7472;
        header.phDataColorSpace = 0x52474220;
        header.phConnectionSpace  = 0x58595a20;
        header.phDateTime[0] = 0x07ce0002;
        header.phDateTime[1] = 0x00090006;
        header.phDateTime[2] = 0x00310000;
        header.phSignature = 0x61637370;
        header.phPlatform = 0x4d534654;
        header.phProfileFlags = 0x00000000;
        header.phManufacturer = 0x49454320;
        header.phModel = 0x73524742;
        header.phAttributes[0] = 0x00000000;
        header.phAttributes[1] = 0x00000000;
        header.phRenderingIntent = 0x00000000;
        header.phIlluminant.ciexyzX = 0x0000f6d6;
        header.phIlluminant.ciexyzY = 0x00010000;
        header.phIlluminant.ciexyzZ = 0x0000d32d;
        header.phCreator = 0x48502020;

        /* Parameter checks */

        handle = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pSetColorProfileHeader( handle, &header );
        ok( !ret, "SetColorProfileHeader() succeeded (%lu)\n", GetLastError() );

        pCloseColorProfile( handle );

        handle = pOpenColorProfileA( &profile, PROFILE_READWRITE, 0, OPEN_EXISTING );
        ok( handle != NULL, "OpenColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pSetColorProfileHeader( NULL, NULL );
        ok( !ret, "SetColorProfileHeader() succeeded (%lu)\n", GetLastError() );

        ret = pSetColorProfileHeader( handle, NULL );
        ok( !ret, "SetColorProfileHeader() succeeded (%lu)\n", GetLastError() );

        ret = pSetColorProfileHeader( NULL, &header );
        ok( !ret, "SetColorProfileHeader() succeeded (%lu)\n", GetLastError() );

        /* Functional checks */

        ret = pSetColorProfileHeader( handle, &header );
        ok( ret, "SetColorProfileHeader() failed (%lu)\n", GetLastError() );

        ret = pGetColorProfileHeader( handle, &header );
        ok( ret, "GetColorProfileHeader() failed (%lu)\n", GetLastError() );

        ok( !memcmp( &header, rgbheader, sizeof(rgbheader) ), "Unexpected header data\n" );

        pCloseColorProfile( handle );
    }
}

static void test_UninstallColorProfileA( char *testprofile )
{
    BOOL ret;

    /* Parameter checks */

    ret = pUninstallColorProfileA( NULL, NULL, FALSE );
    ok( !ret, "UninstallColorProfileA() succeeded (%lu)\n", GetLastError() );

    ret = pUninstallColorProfileA( machine, NULL, FALSE );
    ok( !ret, "UninstallColorProfileA() succeeded (%lu)\n", GetLastError() );

    /* Functional checks */

    if (testprofile)
    {
        CHAR dest[MAX_PATH], base[MAX_PATH];
        DWORD size = sizeof(dest);
        HANDLE handle;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileA( NULL, testprofile );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileA\n");
            return;
        }
        ok( ret, "InstallColorProfileA() failed (%lu)\n", GetLastError() );

        ret = pGetColorDirectoryA( NULL, dest, &size );
        ok( ret, "GetColorDirectoryA() failed (%lu)\n", GetLastError() );

        MSCMS_basenameA( testprofile, base );

        lstrcatA( dest, "\\" );
        lstrcatA( dest, base );

        ret = pUninstallColorProfileA( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileA() failed (%lu)\n", GetLastError() );

        /* Check if the profile is really gone */
        handle = CreateFileA( dest, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle == INVALID_HANDLE_VALUE, "Found the profile (%lu)\n", GetLastError() );
        CloseHandle( handle );
    }
}

static void test_UninstallColorProfileW( WCHAR *testprofileW )
{
    BOOL ret;

    /* Parameter checks */

    ret = pUninstallColorProfileW( NULL, NULL, FALSE );
    ok( !ret, "UninstallColorProfileW() succeeded (%lu)\n", GetLastError() );

    ret = pUninstallColorProfileW( machineW, NULL, FALSE );
    ok( !ret, "UninstallColorProfileW() succeeded (%lu)\n", GetLastError() );

    /* Functional checks */

    if (testprofileW)
    {
        WCHAR dest[MAX_PATH], base[MAX_PATH];
        char destA[MAX_PATH];
        DWORD size = sizeof(dest);
        HANDLE handle;
        int bytes_copied;

        SetLastError(0xdeadbeef);
        ret = pInstallColorProfileW( NULL, testprofileW );
        if (!ret && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights for InstallColorProfileW\n");
            return;
        }
        ok( ret, "InstallColorProfileW() failed (%lu)\n", GetLastError() );

        ret = pGetColorDirectoryW( NULL, dest, &size );
        ok( ret, "GetColorDirectoryW() failed (%lu)\n", GetLastError() );

        MSCMS_basenameW( testprofileW, base );

        lstrcatW( dest, L"\\" );
        lstrcatW( dest, base );

        ret = pUninstallColorProfileW( NULL, dest, TRUE );
        ok( ret, "UninstallColorProfileW() failed (%lu)\n", GetLastError() );

        bytes_copied = WideCharToMultiByte(CP_ACP, 0, dest, -1, destA, MAX_PATH, NULL, NULL);
        ok( bytes_copied > 0 , "WideCharToMultiByte() returns %d\n", bytes_copied);
        /* Check if the profile is really gone */
        handle = CreateFileA( destA, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( handle == INVALID_HANDLE_VALUE, "Found the profile (%lu)\n", GetLastError() );
        CloseHandle( handle );
    }
}

static void test_AssociateColorProfileWithDeviceA( char *testprofile )
{
    BOOL ret;
    char profile[MAX_PATH], basename[MAX_PATH];
    DWORD error, size = sizeof(profile);
    DISPLAY_DEVICEA display, monitor;
    BOOL res;

    if (testprofile && pEnumDisplayDevicesA)
    {
        display.cb = sizeof( DISPLAY_DEVICEA );
        res = pEnumDisplayDevicesA( NULL, 0, &display, 0 );
        ok( res, "Can't get display info\n" );

        monitor.cb = sizeof( DISPLAY_DEVICEA );
        res = pEnumDisplayDevicesA( display.DeviceName, 0, &monitor, 0 );
        if (res)
        {
            SetLastError(0xdeadbeef);
            ret = pAssociateColorProfileWithDeviceA( "machine", testprofile, NULL );
            error = GetLastError();
            ok( !ret, "AssociateColorProfileWithDevice() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error );

            SetLastError(0xdeadbeef);
            ret = pAssociateColorProfileWithDeviceA( "machine", NULL, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "AssociateColorProfileWithDevice() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error );

            SetLastError(0xdeadbeef);
            ret = pAssociateColorProfileWithDeviceA( "machine", testprofile, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "AssociateColorProfileWithDevice() succeeded\n" );
            ok( error == ERROR_NOT_SUPPORTED, "expected ERROR_NOT_SUPPORTED, got %lu\n", error );

            ret = pInstallColorProfileA( NULL, testprofile );
            ok( ret, "InstallColorProfileA() failed (%lu)\n", GetLastError() );

            ret = pGetColorDirectoryA( NULL, profile, &size );
            ok( ret, "GetColorDirectoryA() failed (%lu)\n", GetLastError() );

            MSCMS_basenameA( testprofile, basename );
            lstrcatA( profile, "\\" );
            lstrcatA( profile, basename );

            ret = pAssociateColorProfileWithDeviceA( NULL, profile, monitor.DeviceID );
            ok( ret, "AssociateColorProfileWithDevice() failed (%lu)\n", GetLastError() );

            SetLastError(0xdeadbeef);
            ret = pDisassociateColorProfileFromDeviceA( "machine", profile, NULL );
            error = GetLastError();
            ok( !ret, "DisassociateColorProfileFromDeviceA() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error );

            SetLastError(0xdeadbeef);
            ret = pDisassociateColorProfileFromDeviceA( "machine", NULL, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "DisassociateColorProfileFromDeviceA() succeeded\n" );
            ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error );

            SetLastError(0xdeadbeef);
            ret = pDisassociateColorProfileFromDeviceA( "machine", profile, monitor.DeviceID );
            error = GetLastError();
            ok( !ret, "DisassociateColorProfileFromDeviceA() succeeded\n" );
            ok( error == ERROR_NOT_SUPPORTED, "expected ERROR_NOT_SUPPORTED, got %lu\n", error );

            ret = pDisassociateColorProfileFromDeviceA( NULL, profile, monitor.DeviceID );
            ok( ret, "DisassociateColorProfileFromDeviceA() failed (%lu)\n", GetLastError() );

            ret = pUninstallColorProfileA( NULL, profile, TRUE );
            ok( ret, "UninstallColorProfileA() failed (%lu)\n", GetLastError() );
        }
        else
            skip("Unable to obtain monitor name\n");
    }
}

static BOOL have_profile(void)
{
    char glob[MAX_PATH + sizeof("\\*.icm")];
    DWORD size = MAX_PATH;
    HANDLE handle;
    WIN32_FIND_DATAA data;

    if (!pGetColorDirectoryA( NULL, glob, &size )) return FALSE;
    lstrcatA( glob, "\\*.icm" );
    handle = FindFirstFileA( glob, &data );
    if (handle == INVALID_HANDLE_VALUE) return FALSE;
    FindClose( handle );
    return TRUE;
}

static void test_CreateMultiProfileTransform( char *standardprofile, char *testprofile )
{
    PROFILE profile;
    HPROFILE handle[2];
    HTRANSFORM transform;
    DWORD intents[2] = { INTENT_PERCEPTUAL, INTENT_PERCEPTUAL };

    if (testprofile)
    {
        profile.dwType       = PROFILE_FILENAME;
        profile.pProfileData = standardprofile;
        profile.cbDataSize   = strlen(standardprofile);

        handle[0] = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle[0] != NULL, "got %lu\n", GetLastError() );

        profile.dwType       = PROFILE_FILENAME;
        profile.pProfileData = testprofile;
        profile.cbDataSize   = strlen(testprofile);

        handle[1] = pOpenColorProfileA( &profile, PROFILE_READ, 0, OPEN_EXISTING );
        ok( handle[1] != NULL, "got %lu\n", GetLastError() );

        transform = pCreateMultiProfileTransform( handle, 2, intents, 2, 0, 0 );
        ok( transform != NULL, "got %lu\n", GetLastError() );

        pDeleteColorTransform( transform );
        pCloseColorProfile( handle[0] );
        pCloseColorProfile( handle[1] );
    }
}

START_TEST(profile)
{
    UINT len;
    HANDLE handle;
    char path[MAX_PATH], file[MAX_PATH], profilefile1[MAX_PATH], profilefile2[MAX_PATH];
    WCHAR profilefile1W[MAX_PATH], profilefile2W[MAX_PATH], fileW[MAX_PATH];
    char *standardprofile = NULL, *testprofile = NULL;
    WCHAR *standardprofileW = NULL, *testprofileW = NULL;
    UINT ret;

    hmscms = LoadLibraryA( "mscms.dll" );
    if (!hmscms) return;

    huser32 = LoadLibraryA( "user32.dll" );
    if (!huser32)
    {
        FreeLibrary( hmscms );
        return;
    }

    if (!init_function_ptrs())
    {
        FreeLibrary( huser32 );
        FreeLibrary( hmscms );
        return;
    }

    /* See if we can find the standard color profile */
    ret = GetSystemDirectoryA( profilefile1, sizeof(profilefile1) );
    ok( ret > 0, "GetSystemDirectoryA() returns %d, LastError = %lu\n", ret, GetLastError());
    ok(profilefile1[0] && lstrlenA(profilefile1) < MAX_PATH,
        "Expected length between 0 and MAX_PATH, got %d\n", lstrlenA(profilefile1));
    MultiByteToWideChar(CP_ACP, 0, profilefile1, -1, profilefile1W, MAX_PATH);
    ok(profilefile1W[0] && lstrlenW(profilefile1W) < MAX_PATH,
        "Expected length between 0 and MAX_PATH, got %d\n", lstrlenW(profilefile1W));
    lstrcpyA(profilefile2, profilefile1);
    lstrcpyW(profilefile2W, profilefile1W);

    lstrcatA( profilefile1, profile1 );
    lstrcatW( profilefile1W, profile1W );
    handle = CreateFileA( profilefile1, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );

    if (handle != INVALID_HANDLE_VALUE)
    {
        standardprofile = profilefile1;
        standardprofileW = profilefile1W;
        CloseHandle( handle );
    }

    lstrcatA( profilefile2, profile2 );
    lstrcatW( profilefile2W, profile2W );
    handle = CreateFileA( profilefile2, 0 , 0, NULL, OPEN_EXISTING, 0, NULL );

    if (handle != INVALID_HANDLE_VALUE)
    {
        standardprofile = profilefile2;
        standardprofileW = profilefile2W;
        CloseHandle( handle );
    }

    /* If found, create a temporary copy for testing purposes */
    if (standardprofile && GetTempPathA( sizeof(path), path ))
    {
        if (GetTempFileNameA( path, "rgb", 0, file ))
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

    have_color_profile = have_profile();

    test_GetColorDirectoryA();
    test_GetColorDirectoryW();

    test_GetColorProfileElement( standardprofile );
    test_GetColorProfileElementTag( standardprofile );

    test_GetColorProfileFromHandle( testprofile );
    test_GetColorProfileHeader( testprofile );

    test_GetCountColorProfileElements( standardprofile );

    test_GetStandardColorSpaceProfileA( standardprofile );
    test_GetStandardColorSpaceProfileW( standardprofileW );

    test_EnumColorProfilesA( standardprofile );
    test_EnumColorProfilesW( standardprofileW );

    test_InstallColorProfileA( standardprofile, testprofile );
    test_InstallColorProfileW( standardprofileW, testprofileW );

    test_IsColorProfileTagPresent( standardprofile );

    test_OpenColorProfileA( standardprofile );
    test_OpenColorProfileW( standardprofileW );

    test_SetColorProfileElement( testprofile );
    test_SetColorProfileHeader( testprofile );

    test_UninstallColorProfileA( testprofile );
    test_UninstallColorProfileW( testprofileW );

    test_AssociateColorProfileWithDeviceA( testprofile );
    test_CreateMultiProfileTransform( standardprofile, testprofile );

    if (testprofile) DeleteFileA( testprofile );
    FreeLibrary( huser32 );
    FreeLibrary( hmscms );
}
