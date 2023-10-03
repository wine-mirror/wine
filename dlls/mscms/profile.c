/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005, 2006, 2008 Hans Leidekker
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "icm.h"
#include "wine/debug.h"

#include "mscms_priv.h"

static void basename( const WCHAR *path, WCHAR *name )
{
    int i = lstrlenW( path );
    while (i > 0 && path[i - 1] != '\\' && path[i - 1] != '/') i--;
    lstrcpyW( name, &path[i] );
}

static inline WCHAR *strdupW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

/******************************************************************************
 * AssociateColorProfileWithDeviceA               [MSCMS.@]
 */
BOOL WINAPI AssociateColorProfileWithDeviceA( PCSTR machine, PCSTR profile, PCSTR device )
{
    int len;
    BOOL ret = FALSE;
    WCHAR *profileW, *deviceW;

    TRACE( "( %s, %s, %s )\n", debugstr_a(machine), debugstr_a(profile), debugstr_a(device) );

    if (!profile || !device)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (machine)
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    len = MultiByteToWideChar( CP_ACP, 0, profile, -1, NULL, 0 );
    if (!(profileW = malloc( len * sizeof(WCHAR) ))) return FALSE;

    MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );

    len = MultiByteToWideChar( CP_ACP, 0, device, -1, NULL, 0 );
    if ((deviceW = malloc( len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, device, -1, deviceW, len );
        ret = AssociateColorProfileWithDeviceW( NULL, profileW, deviceW );
    }

    free( profileW );
    free( deviceW );
    return ret;
}

static BOOL set_profile_device_key( PCWSTR file, const BYTE *value, DWORD size )
{
    PROFILEHEADER header;
    PROFILE profile;
    HPROFILE handle;
    HKEY icm_key, class_key;
    WCHAR basenameW[MAX_PATH], classW[5];

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = (PVOID)file;
    profile.cbDataSize = (lstrlenW( file ) + 1) * sizeof(WCHAR);

    /* FIXME is the profile installed? */
    if (!(handle = OpenColorProfileW( &profile, PROFILE_READ, 0, OPEN_EXISTING )))
    {
        SetLastError( ERROR_INVALID_PROFILE );
        return FALSE;
    }
    if (!GetColorProfileHeader( handle, &header ))
    {
        CloseColorProfile( handle );
        SetLastError( ERROR_INVALID_PROFILE );
        return FALSE;
    }
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM",
                     0, NULL, 0, KEY_ALL_ACCESS, NULL, &icm_key, NULL );

    basename( file, basenameW );
    swprintf( classW, ARRAY_SIZE(classW), L"%c%c%c%c",
              (header.phClass >> 24) & 0xff, (header.phClass >> 16) & 0xff,
              (header.phClass >> 8) & 0xff,  header.phClass & 0xff );

    RegCreateKeyExW( icm_key, classW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &class_key, NULL );
    if (value) RegSetValueExW( class_key, basenameW, 0, REG_BINARY, value, size );
    else RegDeleteValueW( class_key, basenameW );

    RegCloseKey( class_key );
    RegCloseKey( icm_key );
    CloseColorProfile( handle );
    return TRUE;
}

/******************************************************************************
 * AssociateColorProfileWithDeviceW               [MSCMS.@]
 */
BOOL WINAPI AssociateColorProfileWithDeviceW( PCWSTR machine, PCWSTR profile, PCWSTR device )
{
    static const BYTE dummy_value[12];

    TRACE( "( %s, %s, %s )\n", debugstr_w(machine), debugstr_w(profile), debugstr_w(device) );

    if (!profile || !device)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (machine)
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    return set_profile_device_key( profile, dummy_value, sizeof(dummy_value) );
}

/******************************************************************************
 * DisassociateColorProfileFromDeviceA            [MSCMS.@]
 */
BOOL WINAPI DisassociateColorProfileFromDeviceA( PCSTR machine, PCSTR profile, PCSTR device )
{
    int len;
    BOOL ret = FALSE;
    WCHAR *profileW, *deviceW;

    TRACE( "( %s, %s, %s )\n", debugstr_a(machine), debugstr_a(profile), debugstr_a(device) );

    if (!profile || !device)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (machine)
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    len = MultiByteToWideChar( CP_ACP, 0, profile, -1, NULL, 0 );
    if (!(profileW = malloc( len * sizeof(WCHAR) ))) return FALSE;

    MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );

    len = MultiByteToWideChar( CP_ACP, 0, device, -1, NULL, 0 );
    if ((deviceW = malloc( len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, device, -1, deviceW, len );
        ret = DisassociateColorProfileFromDeviceW( NULL, profileW, deviceW );
    }

    free( profileW );
    free( deviceW );
    return ret;
}

/******************************************************************************
 * DisassociateColorProfileFromDeviceW            [MSCMS.@]
 */
BOOL WINAPI DisassociateColorProfileFromDeviceW( PCWSTR machine, PCWSTR profile, PCWSTR device )
{
    TRACE( "( %s, %s, %s )\n", debugstr_w(machine), debugstr_w(profile), debugstr_w(device) );

    if (!profile || !device)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (machine)
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    return set_profile_device_key( profile, NULL, 0 );
}

/******************************************************************************
 * GetColorDirectoryA               [MSCMS.@]
 *
 * See GetColorDirectoryW.
 */
BOOL WINAPI GetColorDirectoryA( PCSTR machine, PSTR buffer, PDWORD size )
{
    INT len;
    LPWSTR bufferW;
    BOOL ret = FALSE;
    DWORD sizeW;

    TRACE( "( %p, %p )\n", buffer, size );

    if (machine || !size) return FALSE;

    if (!buffer)
    {
        ret = GetColorDirectoryW( NULL, NULL, &sizeW );
        *size = sizeW / sizeof(WCHAR);
        return ret;
    }

    sizeW = *size * sizeof(WCHAR);

    if ((bufferW = malloc( sizeW )))
    {
        if ((ret = GetColorDirectoryW( NULL, bufferW, &sizeW )))
        {
            *size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
            len = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }
        else *size = sizeW / sizeof(WCHAR);
        free( bufferW );
    }
    return ret;
}

/******************************************************************************
 * GetColorDirectoryW               [MSCMS.@]
 *
 * Get the directory where color profiles are stored.
 *
 * PARAMS
 *  machine  [I]   Name of the machine for which to get the color directory.
 *                 Must be NULL, which indicates the local machine.
 *  buffer   [I]   Buffer to receive the path name.
 *  size     [I/O] Size of the buffer in bytes. On return the variable holds
 *                 the number of bytes actually needed.
 */
BOOL WINAPI GetColorDirectoryW( PCWSTR machine, PWSTR buffer, PDWORD size )
{
    WCHAR colordir[MAX_PATH];
    DWORD len;

    TRACE( "( %p, %p )\n", buffer, size );

    if (machine || !size) return FALSE;

    GetSystemDirectoryW( colordir, ARRAY_SIZE( colordir ));
    lstrcatW( colordir, L"\\spool\\drivers\\color" );

    len = lstrlenW( colordir ) * sizeof(WCHAR);

    if (buffer && len <= *size)
    {
        lstrcpyW( buffer, colordir );
        *size = len;
        return TRUE;
    }

    SetLastError( ERROR_MORE_DATA );
    *size = len;
    return FALSE;
}

/******************************************************************************
 * GetColorProfileElement               [MSCMS.@]
 *
 * Retrieve data for a specified tag type.
 *
 * PARAMS
 *  profile  [I]   Handle to a color profile.
 *  type     [I]   ICC tag type. 
 *  offset   [I]   Offset in bytes to start copying from. 
 *  size     [I/O] Size of the buffer in bytes. On return the variable holds
 *                 the number of bytes actually needed.
 *  buffer   [O]   Buffer to receive the tag data.
 *  ref      [O]   Pointer to a BOOL that specifies whether more than one tag
 *                 references the data.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI GetColorProfileElement( HPROFILE handle, TAGTYPE type, DWORD offset, PDWORD size,
                                    PVOID buffer, PBOOL ref )
{
    BOOL ret;
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p, %#lx, %lu, %p, %p, %p )\n", handle, type, offset, size, buffer, ref );

    if (!profile) return FALSE;

    if (!size || !ref)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    ret = get_tag_data( profile, type, offset, buffer, size, ref );
    release_object( &profile->hdr );
    return ret;
}

/******************************************************************************
 * GetColorProfileElementTag               [MSCMS.@]
 *
 * Get the tag type from a color profile by index. 
 *
 * PARAMS
 *  profile  [I]   Handle to a color profile.
 *  index    [I]   Index into the tag table of the color profile.
 *  type     [O]   Pointer to a variable that holds the ICC tag type on return.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  The tag table index starts at 1.
 *  Use GetCountColorProfileElements to retrieve a count of tagged elements.
 */
BOOL WINAPI GetColorProfileElementTag( HPROFILE handle, DWORD index, PTAGTYPE type )
{
    BOOL ret;
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );
    struct tag_entry tag;

    TRACE( "( %p, %lu, %p )\n", handle, index, type );

    if (!profile) return FALSE;

    if (!type)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    if ((ret = get_tag_entry( profile, index, &tag ))) *type = tag.sig;
    release_object( &profile->hdr );
    return ret;
}

/******************************************************************************
 * GetColorProfileFromHandle               [MSCMS.@]
 *
 * Retrieve an ICC color profile by handle.
 *
 * PARAMS
 *  profile  [I]   Handle to a color profile.
 *  buffer   [O]   Buffer to receive the ICC profile.
 *  size     [I/O] Size of the buffer in bytes. On return the variable holds the
 *                 number of bytes actually needed.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  The profile returned will be in big-endian format.
 */
BOOL WINAPI GetColorProfileFromHandle( HPROFILE handle, PBYTE buffer, PDWORD size )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );
    PROFILEHEADER header;

    TRACE( "( %p, %p, %p )\n", handle, buffer, size );

    if (!profile) return FALSE;

    if (!size)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    get_profile_header( profile, &header );

    if (!buffer || header.phSize > *size)
    {
        *size = header.phSize;
        release_object( &profile->hdr );
        return FALSE;
    }

    /* No endian conversion needed */
    memcpy( buffer, profile->data, profile->size );
    *size = profile->size;

    release_object( &profile->hdr );
    return TRUE;
}

/******************************************************************************
 * GetColorProfileHeader               [MSCMS.@]
 *
 * Retrieve a color profile header by handle.
 *
 * PARAMS
 *  profile  [I]   Handle to a color profile.
 *  header   [O]   Buffer to receive the ICC profile header.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  The profile header returned will be adjusted for endianness.
 */
BOOL WINAPI GetColorProfileHeader( HPROFILE handle, PPROFILEHEADER header )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p, %p )\n", handle, header );

    if (!profile) return FALSE;

    if (!header)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    get_profile_header( profile, header );
    release_object( &profile->hdr );
    return TRUE;
}

/******************************************************************************
 * GetCountColorProfileElements               [MSCMS.@]
 *
 * Retrieve the number of elements in a color profile.
 *
 * PARAMS
 *  profile  [I] Handle to a color profile.
 *  count    [O] Pointer to a variable which is set to the number of elements
 *               in the color profile.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI GetCountColorProfileElements( HPROFILE handle, PDWORD count )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p, %p )\n", handle, count );

    if (!profile) return FALSE;

    if (!count)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    *count = get_tag_count( profile );
    release_object( &profile->hdr );
    return TRUE;
}

/******************************************************************************
 * GetStandardColorSpaceProfileA               [MSCMS.@]
 *
 * See GetStandardColorSpaceProfileW.
 */
BOOL WINAPI GetStandardColorSpaceProfileA( PCSTR machine, DWORD id, PSTR profile, PDWORD size )
{
    INT len;
    LPWSTR profileW;
    BOOL ret = FALSE;
    DWORD sizeW;

    TRACE( "( %#lx, %p, %p )\n", id, profile, size );

    if (machine) 
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    if (!size) 
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    sizeW = *size * sizeof(WCHAR);

    if (!profile)
    {
        ret = GetStandardColorSpaceProfileW( NULL, id, NULL, &sizeW );
        *size = sizeW / sizeof(WCHAR);
        return ret;
    }

    if ((profileW = malloc( sizeW )))
    {
        if ((ret = GetStandardColorSpaceProfileW( NULL, id, profileW, &sizeW )))
        {
            *size = WideCharToMultiByte( CP_ACP, 0, profileW, -1, NULL, 0, NULL, NULL );
            len = WideCharToMultiByte( CP_ACP, 0, profileW, -1, profile, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }
        else *size = sizeW / sizeof(WCHAR);
        free( profileW );
    }
    return ret;
}

/******************************************************************************
 * GetStandardColorSpaceProfileW               [MSCMS.@]
 *
 * Retrieve the profile filename for a given standard color space id.
 *
 * PARAMS
 *  machine  [I]   Name of the machine for which to get the standard color space.
 *                 Must be NULL, which indicates the local machine.
 *  id       [I]   Id of a standard color space.
 *  profile  [O]   Buffer to receive the profile filename.
 *  size     [I/O] Size of the filename buffer in bytes.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI GetStandardColorSpaceProfileW( PCWSTR machine, DWORD id, PWSTR profile, PDWORD size )
{
    WCHAR rgbprofile[MAX_PATH];
    DWORD len = sizeof(rgbprofile);

    TRACE( "( %#lx, %p, %p )\n", id, profile, size );

    if (machine) 
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    if (!size) 
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!profile)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    GetColorDirectoryW( machine, rgbprofile, &len );

    switch (id)
    {
        case LCS_sRGB:
        case LCS_WINDOWS_COLOR_SPACE: /* FIXME */
            lstrcatW( rgbprofile, L"\\srgb color space profile.icm" );
            len = lstrlenW( rgbprofile ) * sizeof(WCHAR);

            if (*size < len)
            {
                *size = len;
                SetLastError( ERROR_MORE_DATA );
                return FALSE;
            }

            lstrcpyW( profile, rgbprofile );
            break;

        default:
            SetLastError( ERROR_FILE_NOT_FOUND );
            return FALSE;
    }
    return TRUE;
}

static BOOL header_from_file( LPCWSTR file, PPROFILEHEADER header )
{
    BOOL ret;
    PROFILE profile;
    WCHAR path[MAX_PATH];
    DWORD size = sizeof(path);
    HANDLE handle;

    ret = GetColorDirectoryW( NULL, path, &size );
    if (!ret)
    {
        WARN( "Can't retrieve color directory\n" );
        return FALSE;
    }
    if (size + sizeof(L"\\") + sizeof(WCHAR) * lstrlenW( file ) > sizeof(path))
    {
        WARN( "Filename too long\n" );
        return FALSE;
    }

    lstrcatW( path, L"\\" );
    lstrcatW( path, file );

    profile.dwType = PROFILE_FILENAME;
    profile.pProfileData = path;
    profile.cbDataSize = lstrlenW( path ) + 1;

    handle = OpenColorProfileW( &profile, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
    if (!handle)
    {
        WARN( "Can't open color profile\n" );
        return FALSE;
    }

    ret = GetColorProfileHeader( handle, header );
    if (!ret)
        WARN( "Can't retrieve color profile header\n" );

    CloseColorProfile( handle );
    return ret;
}

static BOOL match_profile( PENUMTYPEW rec, PPROFILEHEADER hdr )
{
    if (rec->dwFields & ET_DEVICENAME)
    {
        FIXME( "ET_DEVICENAME: %s\n", debugstr_w(rec->pDeviceName) );
    }
    if (rec->dwFields & ET_MEDIATYPE)
    {
        FIXME( "ET_MEDIATYPE: %#lx\n", rec->dwMediaType );
    }
    if (rec->dwFields & ET_DITHERMODE)
    {
        FIXME( "ET_DITHERMODE: %#lx\n", rec->dwDitheringMode );
    }
    if (rec->dwFields & ET_RESOLUTION)
    {
        FIXME( "ET_RESOLUTION: %#lx, %#lx\n",
               rec->dwResolution[0], rec->dwResolution[1] );
    }
    if (rec->dwFields & ET_DEVICECLASS)
    {
        FIXME( "ET_DEVICECLASS: %s\n", debugstr_fourcc(rec->dwMediaType) );
    }
    if (rec->dwFields & ET_CMMTYPE)
    {
        TRACE( "ET_CMMTYPE: %s\n", debugstr_fourcc(rec->dwCMMType) );
        if (rec->dwCMMType != hdr->phCMMType) return FALSE;
    }
    if (rec->dwFields & ET_CLASS)
    {
        TRACE( "ET_CLASS: %s\n", debugstr_fourcc(rec->dwClass) );
        if (rec->dwClass != hdr->phClass) return FALSE;
    }
    if (rec->dwFields & ET_DATACOLORSPACE)
    {
        TRACE( "ET_DATACOLORSPACE: %s\n", debugstr_fourcc(rec->dwDataColorSpace) );
        if (rec->dwDataColorSpace != hdr->phDataColorSpace) return FALSE;
    }
    if (rec->dwFields & ET_CONNECTIONSPACE)
    {
        TRACE( "ET_CONNECTIONSPACE: %s\n", debugstr_fourcc(rec->dwConnectionSpace) );
        if (rec->dwConnectionSpace != hdr->phConnectionSpace) return FALSE;
    }
    if (rec->dwFields & ET_SIGNATURE)
    {
        TRACE( "ET_SIGNATURE: %s\n", debugstr_fourcc(rec->dwSignature) );
        if (rec->dwSignature != hdr->phSignature) return FALSE;
    }
    if (rec->dwFields & ET_PLATFORM)
    {
        TRACE( "ET_PLATFORM: %s\n", debugstr_fourcc(rec->dwPlatform) );
        if (rec->dwPlatform != hdr->phPlatform) return FALSE;
    }
    if (rec->dwFields & ET_PROFILEFLAGS)
    {
        TRACE( "ET_PROFILEFLAGS: %#lx\n", rec->dwProfileFlags );
        if (rec->dwProfileFlags != hdr->phProfileFlags) return FALSE;
    }
    if (rec->dwFields & ET_MANUFACTURER)
    {
        TRACE( "ET_MANUFACTURER: %s\n", debugstr_fourcc(rec->dwManufacturer) );
        if (rec->dwManufacturer != hdr->phManufacturer) return FALSE;
    }
    if (rec->dwFields & ET_MODEL)
    {
        TRACE( "ET_MODEL: %s\n", debugstr_fourcc(rec->dwModel) );
        if (rec->dwModel != hdr->phModel) return FALSE;
    }
    if (rec->dwFields & ET_ATTRIBUTES)
    {
        TRACE( "ET_ATTRIBUTES: %#lx, %#lx\n",
               rec->dwAttributes[0], rec->dwAttributes[1] );
        if (rec->dwAttributes[0] != hdr->phAttributes[0] || 
            rec->dwAttributes[1] != hdr->phAttributes[1]) return FALSE;
    }
    if (rec->dwFields & ET_RENDERINGINTENT)
    {
        TRACE( "ET_RENDERINGINTENT: %#lx\n", rec->dwRenderingIntent );
        if (rec->dwRenderingIntent != hdr->phRenderingIntent) return FALSE;
    }
    if (rec->dwFields & ET_CREATOR)
    {
        TRACE( "ET_CREATOR: %s\n", debugstr_fourcc(rec->dwCreator) );
        if (rec->dwCreator != hdr->phCreator) return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * EnumColorProfilesA               [MSCMS.@]
 *
 * See EnumColorProfilesW.
 */
BOOL WINAPI EnumColorProfilesA( PCSTR machine, PENUMTYPEA record, PBYTE buffer,
                                PDWORD size, PDWORD number )
{
    BOOL match, ret = FALSE;
    char spec[] = "\\*.icm";
    char colordir[MAX_PATH], glob[MAX_PATH], **profiles = NULL;
    DWORD i, len = sizeof(colordir), count = 0, totalsize = 0;
    PROFILEHEADER header;
    WIN32_FIND_DATAA data;
    ENUMTYPEW recordW;
    WCHAR *fileW = NULL, *deviceW = NULL;
    HANDLE find;

    TRACE( "( %p, %p, %p, %p, %p )\n", machine, record, buffer, size, number );

    if (machine || !record || !size ||
        record->dwSize != sizeof(ENUMTYPEA) ||
        record->dwVersion != ENUM_TYPE_VERSION) return FALSE;

    ret = GetColorDirectoryA( machine, colordir, &len );
    if (!ret || len + sizeof(spec) > MAX_PATH)
    {
        WARN( "can't retrieve color directory\n" );
        return FALSE;
    }

    lstrcpyA( glob, colordir );
    lstrcatA( glob, spec );

    find = FindFirstFileA( glob, &data );
    if (find == INVALID_HANDLE_VALUE) return FALSE;

    if (!(profiles = calloc( 1, sizeof(char *) + 1 ))) goto exit;

    memcpy( &recordW, record, sizeof(ENUMTYPEA) );
    if (record->pDeviceName)
    {
        deviceW = strdupW( record->pDeviceName );
        if (!(recordW.pDeviceName = deviceW)) goto exit;
    }

    if (!(fileW = strdupW( data.cFileName ))) goto exit;

    if ((ret = header_from_file( fileW, &header )))
    {
        if ((match = match_profile( &recordW, &header )))
        {
            len = sizeof(char) * (lstrlenA( data.cFileName ) + 1);
            if (!(profiles[count] = malloc( len ))) goto exit;
            else
            {
                TRACE( "matching profile: %s\n", debugstr_a(data.cFileName) );
                lstrcpyA( profiles[count], data.cFileName );
                totalsize += len;
                count++;
            }
        }
    }
    free( fileW );
    fileW = NULL;

    while (FindNextFileA( find, &data ))
    {
        if (!(fileW = strdupW( data.cFileName ))) goto exit;
        if (!(ret = header_from_file( fileW, &header )))
        {
            free( fileW );
            fileW = NULL;
            continue;
        }

        if ((match = match_profile( &recordW, &header )))
        {
            char **tmp = realloc( profiles, sizeof(char *) * (count + 1) );
            if (!tmp) goto exit;
            else profiles = tmp;

            len = sizeof(char) * (lstrlenA( data.cFileName ) + 1);
            if (!(profiles[count] = malloc( len ))) goto exit;
            else
            {
                TRACE( "matching profile: %s\n", debugstr_a(data.cFileName) );
                lstrcpyA( profiles[count], data.cFileName );
                totalsize += len;
                count++;
            }
        }
        free( fileW );
        fileW = NULL;
    }

    totalsize++;
    if (buffer && *size >= totalsize)
    {
        char *p = (char *)buffer;

        for (i = 0; i < count; i++)
        {
            lstrcpyA( p, profiles[i] );
            p += lstrlenA( profiles[i] ) + 1;
        }
        *p = 0;
        ret = TRUE;
    }
    else
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = FALSE;
    }

    *size = totalsize;
    if (number) *number = count;

exit:
    for (i = 0; i < count; i++) free( profiles[i] );
    free( profiles );
    free( deviceW );
    free( fileW );
    FindClose( find );

    return ret;
}

/******************************************************************************
 * EnumColorProfilesW               [MSCMS.@]
 *
 * Enumerate profiles that match given criteria.
 *
 * PARAMS
 *  machine  [I]   Name of the machine for which to enumerate profiles.
 *                 Must be NULL, which indicates the local machine.
 *  record   [I]   Record of criteria that a profile must match.
 *  buffer   [O]   Buffer to receive a string array of profile filenames.
 *  size     [I/O] Size of the filename buffer in bytes.
 *  number   [O]   Number of filenames copied into buffer.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI EnumColorProfilesW( PCWSTR machine, PENUMTYPEW record, PBYTE buffer,
                                PDWORD size, PDWORD number )
{
    BOOL match, ret = FALSE;
    WCHAR colordir[MAX_PATH], glob[MAX_PATH], **profiles = NULL;
    DWORD i, len = sizeof(colordir), count = 0, totalsize = 0;
    PROFILEHEADER header;
    WIN32_FIND_DATAW data;
    HANDLE find;

    TRACE( "( %p, %p, %p, %p, %p )\n", machine, record, buffer, size, number );

    if (machine || !record || !size ||
        record->dwSize != sizeof(ENUMTYPEW) ||
        record->dwVersion != ENUM_TYPE_VERSION) return FALSE;

    ret = GetColorDirectoryW( machine, colordir, &len );
    if (!ret || len + ARRAY_SIZE(L"\\*icm") > MAX_PATH)
    {
        WARN( "Can't retrieve color directory\n" );
        return FALSE;
    }

    lstrcpyW( glob, colordir );
    lstrcatW( glob, L"\\*icm" );

    find = FindFirstFileW( glob, &data );
    if (find == INVALID_HANDLE_VALUE) return FALSE;

    if (!(profiles = calloc( 1, sizeof(WCHAR *) + 1 ))) goto exit;

    if ((ret = header_from_file( data.cFileName, &header )))
    {
        if ((match = match_profile( record, &header )))
        {
            len = sizeof(WCHAR) * (lstrlenW( data.cFileName ) + 1);
            if (!(profiles[count] = malloc( len ))) goto exit;
            else
            {
                TRACE( "matching profile: %s\n", debugstr_w(data.cFileName) );
                lstrcpyW( profiles[count], data.cFileName );
                totalsize += len;
                count++;
            }
        }
    }

    while (FindNextFileW( find, &data ))
    {
        if (!(ret = header_from_file( data.cFileName, &header ))) continue;

        if ((match = match_profile( record, &header )))
        {
            WCHAR **tmp = realloc( profiles, sizeof(WCHAR *) * (count + 1) );
            if (!tmp) goto exit;
            else profiles = tmp;

            len = sizeof(WCHAR) * (lstrlenW( data.cFileName ) + 1);
            if (!(profiles[count] = malloc( len ))) goto exit;
            else
            {
                TRACE( "matching profile: %s\n", debugstr_w(data.cFileName) );
                lstrcpyW( profiles[count], data.cFileName );
                totalsize += len;
                count++;
            }
        }
    }

    totalsize++;
    if (buffer && *size >= totalsize)
    {
        WCHAR *p = (WCHAR *)buffer;

        for (i = 0; i < count; i++)
        {
            lstrcpyW( p, profiles[i] );
            p += lstrlenW( profiles[i] ) + 1;
        }
        *p = 0;
        ret = TRUE;
    }
    else
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = FALSE;
    }

    *size = totalsize;
    if (number) *number = count;

exit:
    for (i = 0; i < count; i++) free( profiles[i] );
    free( profiles );
    FindClose( find );

    return ret;
}

/******************************************************************************
 * InstallColorProfileA               [MSCMS.@]
 *
 * See InstallColorProfileW.
 */
BOOL WINAPI InstallColorProfileA( PCSTR machine, PCSTR profile )
{
    UINT len;
    LPWSTR profileW;
    BOOL ret = FALSE;

    TRACE( "( %s )\n", debugstr_a(profile) );

    if (machine || !profile) return FALSE;

    len = MultiByteToWideChar( CP_ACP, 0, profile, -1, NULL, 0 );
    if ((profileW = malloc( len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );
        ret = InstallColorProfileW( NULL, profileW );
        free( profileW );
    }
    return ret;
}

/******************************************************************************
 * InstallColorProfileW               [MSCMS.@]
 *
 * Install a color profile.
 *
 * PARAMS
 *  machine  [I] Name of the machine to install the profile on. Must be NULL,
 *               which indicates the local machine.
 *  profile  [I] Full path name of the profile to install.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI InstallColorProfileW( PCWSTR machine, PCWSTR profile )
{
    WCHAR dest[MAX_PATH], base[MAX_PATH];
    DWORD size = sizeof(dest);

    TRACE( "( %s )\n", debugstr_w(profile) );

    if (machine || !profile) return FALSE;

    if (!GetColorDirectoryW( machine, dest, &size )) return FALSE;

    basename( profile, base );
    lstrcatW( dest, L"\\" );
    lstrcatW( dest, base );

    /* Is source equal to destination? */
    if (!wcscmp( profile, dest )) return TRUE;

    return CopyFileW( profile, dest, TRUE );
}

/******************************************************************************
 * IsColorProfileTagPresent               [MSCMS.@]
 *
 * Determine if a given ICC tag type is present in a color profile.
 *
 * PARAMS
 *  profile  [I] Color profile handle.
 *  tag      [I] ICC tag type.
 *  present  [O] Pointer to a BOOL variable. Set to TRUE if tag type is present,
 *               FALSE otherwise.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI IsColorProfileTagPresent( HPROFILE handle, TAGTYPE type, PBOOL present )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );
    struct tag_entry tag;

    TRACE( "( %p, %#lx, %p )\n", handle, type, present );

    if (!profile) return FALSE;

    if (!present)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    *present = get_adjusted_tag( profile, type, &tag );
    release_object( &profile->hdr );
    return TRUE;
}

/******************************************************************************
 * IsColorProfileValid               [MSCMS.@]
 *
 * Determine if a given color profile is valid.
 *
 * PARAMS
 *  profile  [I] Color profile handle.
 *  valid    [O] Pointer to a BOOL variable. Set to TRUE if profile is valid,
 *               FALSE otherwise.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE 
 */
BOOL WINAPI IsColorProfileValid( HPROFILE handle, PBOOL valid )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p, %p )\n", handle, valid );

    if (!profile) return FALSE;

    if (!valid)
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    *valid = !!profile->data;
    release_object( &profile->hdr );
    return *valid;
}

/******************************************************************************
 * SetColorProfileElement               [MSCMS.@]
 *
 * Set data for a specified tag type.
 *
 * PARAMS
 *  profile  [I]   Handle to a color profile.
 *  type     [I]   ICC tag type.
 *  offset   [I]   Offset in bytes to start copying to.
 *  size     [I/O] Size of the buffer in bytes. On return the variable holds the
 *                 number of bytes actually needed.
 *  buffer   [O]   Buffer holding the tag data.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetColorProfileElement( HPROFILE handle, TAGTYPE type, DWORD offset, PDWORD size,
                                    PVOID buffer )
{
    BOOL ret;
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p, %#lx, %lu, %p, %p )\n", handle, type, offset, size, buffer );

    if (!profile) return FALSE;

    if (!size || !buffer || !(profile->access & PROFILE_READWRITE))
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    ret = set_tag_data( profile, type, offset, buffer, size );
    release_object( &profile->hdr );
    return ret;
}

/******************************************************************************
 * SetColorProfileHeader               [MSCMS.@]
 *
 * Set header data for a given profile.
 *
 * PARAMS
 *  profile  [I] Handle to a color profile.
 *  header   [I] Buffer holding the header data.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetColorProfileHeader( HPROFILE handle, PPROFILEHEADER header )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p, %p )\n", handle, header );

    if (!profile) return FALSE;

    if (!header || !(profile->access & PROFILE_READWRITE))
    {
        release_object( &profile->hdr );
        return FALSE;
    }
    set_profile_header( profile, header );
    release_object( &profile->hdr );
    return TRUE;
}

/******************************************************************************
 * UninstallColorProfileA               [MSCMS.@]
 *
 * See UninstallColorProfileW.
 */
BOOL WINAPI UninstallColorProfileA( PCSTR machine, PCSTR profile, BOOL delete )
{
    UINT len;
    LPWSTR profileW;
    BOOL ret = FALSE;

    TRACE( "( %s, %x )\n", debugstr_a(profile), delete );

    if (machine || !profile) return FALSE;

    len = MultiByteToWideChar( CP_ACP, 0, profile, -1, NULL, 0 );
    if ((profileW = malloc( len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );
        ret = UninstallColorProfileW( NULL, profileW , delete );
        free( profileW );
    }
    return ret;
}

/******************************************************************************
 * UninstallColorProfileW               [MSCMS.@]
 *
 * Uninstall a color profile.
 *
 * PARAMS
 *  machine  [I] Name of the machine to uninstall the profile on. Must be NULL,
 *               which indicates the local machine.
 *  profile  [I] Full path name of the profile to uninstall.
 *  delete   [I] Bool that specifies whether the profile file should be deleted.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI UninstallColorProfileW( PCWSTR machine, PCWSTR profile, BOOL delete )
{
    TRACE( "( %s, %x )\n", debugstr_w(profile), delete );

    if (machine || !profile) return FALSE;

    if (delete) return DeleteFileW( profile );
    return TRUE;
}

static BOOL profile_AtoW( const PROFILE *in, PROFILE *out )
{
    int len;
    if (!in->pProfileData) return FALSE;
    len = MultiByteToWideChar( CP_ACP, 0, in->pProfileData, -1, NULL, 0 );
    if (!(out->pProfileData = malloc( len * sizeof(WCHAR) ))) return FALSE;
    out->cbDataSize = len * sizeof(WCHAR);
    MultiByteToWideChar( CP_ACP, 0, in->pProfileData, -1, out->pProfileData, len );
    out->dwType = in->dwType;
    return TRUE;
}

/******************************************************************************
 * OpenColorProfileA               [MSCMS.@]
 *
 * See OpenColorProfileW.
 */
HPROFILE WINAPI OpenColorProfileA( PPROFILE profile, DWORD access, DWORD sharing, DWORD creation )
{
    HPROFILE handle = NULL;
    PROFILE profileW;

    TRACE( "( %p, %#lx, %#lx, %#lx )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    /* No AW conversion needed for memory based profiles */
    if (profile->dwType & PROFILE_MEMBUFFER)
        return OpenColorProfileW( profile, access, sharing, creation );

    if (!profile_AtoW( profile, &profileW )) return FALSE;
    handle = OpenColorProfileW( &profileW, access, sharing, creation );
    free( profileW.pProfileData );
    return handle;
}

void close_profile( struct object *obj )
{
    struct profile *profile = (struct profile *)obj;

    if (profile->file != INVALID_HANDLE_VALUE)
    {
        if (profile->access & PROFILE_READWRITE)
        {
            DWORD count;
            if (SetFilePointer( profile->file, 0, NULL, FILE_BEGIN ) ||
                !WriteFile( profile->file, profile->data, profile->size, &count, NULL ) || count != profile->size)
            {
                ERR( "Unable to write color profile\n" );
            }
        }
        CloseHandle( profile->file );
    }

    if (profile->cmsprofile) cmsCloseProfile( profile->cmsprofile );
    free( profile->data );
}

/******************************************************************************
 * OpenColorProfileW               [MSCMS.@]
 *
 * Open a color profile.
 *
 * PARAMS
 *  profile   [I] Pointer to a color profile structure.
 *  access    [I] Desired access.
 *  sharing   [I] Sharing mode.
 *  creation  [I] Creation mode.
 *
 * RETURNS
 *  Success: Handle to the opened profile.
 *  Failure: NULL
 *
 * NOTES
 *  Values for access:   PROFILE_READ or PROFILE_READWRITE.
 *  Values for sharing:  0 (no sharing), FILE_SHARE_READ and/or FILE_SHARE_WRITE.
 *  Values for creation: one of CREATE_NEW, CREATE_ALWAYS, OPEN_EXISTING,
 *                       OPEN_ALWAYS, TRUNCATE_EXISTING.
 *  Sharing and creation flags are ignored for memory based profiles.
 */
HPROFILE WINAPI OpenColorProfileW( PPROFILE profile, DWORD access, DWORD sharing, DWORD creation )
{
    struct profile *prof;
    HPROFILE ret;
    cmsHPROFILE cmsprofile;
    char *data = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;
    DWORD size;

    TRACE( "( %p, %#lx, %#lx, %#lx )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    if (profile->dwType == PROFILE_MEMBUFFER)
    {
        /* FIXME: access flags not implemented for memory based profiles */

        if (!(data = malloc( profile->cbDataSize ))) return NULL;
        memcpy( data, profile->pProfileData, profile->cbDataSize );

        if (!(cmsprofile = cmsOpenProfileFromMem( data, profile->cbDataSize )))
        {
            free( data );
            return FALSE;
        }
        size = profile->cbDataSize;
    }
    else if (profile->dwType == PROFILE_FILENAME)
    {
        DWORD read, flags = 0;

        TRACE( "profile file: %s\n", debugstr_w( profile->pProfileData ) );

        if (access & PROFILE_READ) flags = GENERIC_READ;
        if (access & PROFILE_READWRITE) flags = GENERIC_READ|GENERIC_WRITE;

        if (!flags) return NULL;
        if (!sharing) sharing = FILE_SHARE_READ;

        if (!PathIsRelativeW( profile->pProfileData ))
            handle = CreateFileW( profile->pProfileData, flags, sharing, NULL, creation, 0, NULL );
        else
        {
            WCHAR *path;

            if (!GetColorDirectoryW( NULL, NULL, &size ) && GetLastError() == ERROR_MORE_DATA)
            {
                size += (lstrlenW( profile->pProfileData ) + 2) * sizeof(WCHAR);
                if (!(path = malloc( size ))) return NULL;
                GetColorDirectoryW( NULL, path, &size );
                PathAddBackslashW( path );
                lstrcatW( path, profile->pProfileData );
            }
            else return NULL;
            handle = CreateFileW( path, flags, sharing, NULL, creation, 0, NULL );
            free( path );
        }
        if (handle == INVALID_HANDLE_VALUE)
        {
            WARN( "Unable to open color profile %lu\n", GetLastError() );
            return NULL;
        }
        if ((size = GetFileSize( handle, NULL )) == INVALID_FILE_SIZE)
        {
            ERR( "Unable to retrieve size of color profile\n" );
            CloseHandle( handle );
            return NULL;
        }
        if (!(data = malloc( size )))
        {
            ERR( "Unable to allocate memory for color profile\n" );
            CloseHandle( handle );
            return NULL;
        }
        if (!ReadFile( handle, data, size, &read, NULL ) || read != size)
        {
            ERR( "Unable to read color profile\n" );
            CloseHandle( handle );
            free( data );
            return NULL;
        }
        if (!(cmsprofile = cmsOpenProfileFromMem( data, size )))
        {
            CloseHandle( handle );
            free( data );
            return NULL;
        }
    }
    else
    {
        ERR( "Invalid profile type %lu\n", profile->dwType );
        return NULL;
    }

    if ((prof = calloc( 1, sizeof(*prof) )))
    {
        prof->hdr.type  = OBJECT_TYPE_PROFILE;
        prof->hdr.close = close_profile;
        prof->file       = handle;
        prof->access     = access;
        prof->data       = data;
        prof->size       = size;
        prof->cmsprofile = cmsprofile;
        if ((ret = alloc_handle( &prof->hdr ))) return ret;
        free( prof );
    }

    cmsCloseProfile( cmsprofile );
    free( data );
    CloseHandle( handle );
    return NULL;
}

/******************************************************************************
 * CloseColorProfile               [MSCMS.@]
 *
 * Close a color profile.
 *
 * PARAMS
 *  profile  [I] Handle to the profile.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CloseColorProfile( HPROFILE handle )
{
    struct profile *profile = (struct profile *)grab_object( handle, OBJECT_TYPE_PROFILE );

    TRACE( "( %p )\n", handle );

    if (!profile) return FALSE;
    free_handle( handle );
    release_object( &profile->hdr );
    return TRUE;
}

/******************************************************************************
 * WcsGetUsePerUserProfiles               [MSCMS.@]
 */
BOOL WINAPI WcsGetUsePerUserProfiles( const WCHAR* name, DWORD class, BOOL* use_per_user_profile )
{
    FIXME( "%s %s %p\n", debugstr_w(name), debugstr_fourcc(class), use_per_user_profile );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/******************************************************************************
 * WcsEnumColorProfilesSize               [MSCMS.@]
 */
BOOL WINAPI WcsEnumColorProfilesSize( WCS_PROFILE_MANAGEMENT_SCOPE scope, ENUMTYPEW *record, DWORD *size )
{
    FIXME( "%d %p %p\n", scope, record, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/******************************************************************************
 * WcsGetDefaultColorProfileSize     [MSCMS.@]
 */
BOOL WINAPI WcsGetDefaultColorProfileSize( WCS_PROFILE_MANAGEMENT_SCOPE scope, PCWSTR device_name,
                                           COLORPROFILETYPE type, COLORPROFILESUBTYPE subtype,
                                           DWORD profile_id, PDWORD profile_size)
{
    FIXME( "%d, %s, %d, %d, %lu, %p\n", scope, debugstr_w(device_name), type, subtype, profile_id, profile_size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/******************************************************************************
 * WcsGetDefaultRednderingIntent      [MSCMS.@]
 */
BOOL WINAPI WcsGetDefaultRenderingIntent( WCS_PROFILE_MANAGEMENT_SCOPE scope, PDWORD intent)
{
    FIXME( "%d %p\n", scope, intent );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/******************************************************************************
 * WcsOpenColorProfileA               [MSCMS.@]
 */
HPROFILE WINAPI WcsOpenColorProfileA( PROFILE *cdm, PROFILE *camp, PROFILE *gmmp, DWORD access, DWORD sharing,
                                      DWORD creation, DWORD flags )
{
    PROFILE cdmW, campW = {0}, gmmpW = {0};
    HPROFILE ret = NULL;

    TRACE( "%p, %p, %p, %#lx, %#lx, %#lx, %#lx\n", cdm, camp, gmmp, access, sharing, creation, flags );

    if (!cdm || !profile_AtoW( cdm, &cdmW )) return NULL;
    if (camp && !profile_AtoW( camp, &campW )) goto done;
    if (gmmp && !profile_AtoW( gmmp, &gmmpW )) goto done;

    ret = WcsOpenColorProfileW( &cdmW, &campW, &gmmpW, access, sharing, creation, flags );

done:
    free( cdmW.pProfileData );
    free( campW.pProfileData );
    free( gmmpW.pProfileData );
    return ret;
}

/******************************************************************************
 * WcsOpenColorProfileW               [MSCMS.@]
 */
HPROFILE WINAPI WcsOpenColorProfileW( PROFILE *cdm, PROFILE *camp, PROFILE *gmmp, DWORD access, DWORD sharing,
                                      DWORD creation, DWORD flags )
{
    TRACE( "%p, %p, %p, %#lx, %#lx, %#lx, %#lx\n", cdm, camp, gmmp, access, sharing, creation, flags );
    FIXME("no support for WCS profiles\n" );

    return OpenColorProfileW( cdm, access, sharing, creation );
}
