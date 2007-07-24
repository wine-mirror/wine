/*
 * MSCMS - Color Management System for Wine
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

#include "config.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

static void MSCMS_basename( LPCWSTR path, LPWSTR name )
{
    INT i = lstrlenW( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    lstrcpyW( name, &path[i] );
}

static inline LPWSTR MSCMS_strdupW( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static const char *MSCMS_dbgstr_tag( DWORD tag )
{
    return wine_dbg_sprintf( "'%c%c%c%c'",
        (char)(tag >> 24), (char)(tag >> 16), (char)(tag >> 8), (char)(tag) );
}

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

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
        return FALSE;
    }

    sizeW = *size * sizeof(WCHAR);

    bufferW = HeapAlloc( GetProcessHeap(), 0, sizeW );

    if (bufferW)
    {
        if ((ret = GetColorDirectoryW( NULL, bufferW, &sizeW )))
        {
            *size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
            len = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }
        else *size = sizeW / sizeof(WCHAR);

        HeapFree( GetProcessHeap(), 0, bufferW );
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
    static const WCHAR colorsubdir[] = { '\\','c','o','l','o','r',0 };
    DWORD len;

    TRACE( "( %p, %p )\n", buffer, size );

    if (machine || !size) return FALSE;

    GetSystemDirectoryW( colordir, sizeof(colordir) / sizeof(WCHAR) );
    lstrcatW( colordir, colorsubdir );

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
BOOL WINAPI GetColorProfileElement( HPROFILE profile, TAGTYPE type, DWORD offset, PDWORD size,
                                    PVOID buffer, PBOOL ref )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD i, count;
    icTag tag;

    TRACE( "( %p, 0x%08x, %d, %p, %p, %p )\n", profile, type, offset, size, buffer, ref );

    if (!iccprofile || !size || !ref) return FALSE;
    count = MSCMS_get_tag_count( iccprofile );

    for (i = 0; i < count; i++)
    {
        MSCMS_get_tag_by_index( iccprofile, i, &tag );

        if (tag.sig == type)
        {
            if ((tag.size - offset) > *size || !buffer)
            {
                *size = (tag.size - offset);
                return FALSE;
            }

            MSCMS_get_tag_data( iccprofile, &tag, offset, buffer );

            *ref = FALSE; /* FIXME: calculate properly */
            return TRUE;
        }
    }

#endif /* HAVE_LCMS */
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
BOOL WINAPI GetColorProfileElementTag( HPROFILE profile, DWORD index, PTAGTYPE type )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD count;
    icTag tag;

    TRACE( "( %p, %d, %p )\n", profile, index, type );

    if (!iccprofile || !type) return FALSE;

    count = MSCMS_get_tag_count( iccprofile );
    if (index > count || index < 1) return FALSE;

    MSCMS_get_tag_by_index( iccprofile, index - 1, &tag );
    *type = tag.sig;

    ret = TRUE;

#endif /* HAVE_LCMS */
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
BOOL WINAPI GetColorProfileFromHandle( HPROFILE profile, PBYTE buffer, PDWORD size )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    PROFILEHEADER header;

    TRACE( "( %p, %p, %p )\n", profile, buffer, size );

    if (!iccprofile || !size) return FALSE;
    MSCMS_get_profile_header( iccprofile, &header );

    if (!buffer || header.phSize > *size)
    {
        *size = header.phSize;
        return FALSE;
    }

    /* No endian conversion needed */
    memcpy( buffer, iccprofile, header.phSize );

    *size = header.phSize;
    ret = TRUE;

#endif /* HAVE_LCMS */
    return ret;
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
 *  The profile header returned will be adjusted for endianess.
 */
BOOL WINAPI GetColorProfileHeader( HPROFILE profile, PPROFILEHEADER header )
{
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, header );

    if (!iccprofile || !header) return FALSE;

    MSCMS_get_profile_header( iccprofile, header );
    return TRUE;

#else
    return FALSE;
#endif /* HAVE_LCMS */
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
BOOL WINAPI GetCountColorProfileElements( HPROFILE profile, PDWORD count )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, count );

    if (!iccprofile || !count) return FALSE;
    *count = MSCMS_get_tag_count( iccprofile );
    ret = TRUE;

#endif /* HAVE_LCMS */
    return ret;
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

    TRACE( "( 0x%08x, %p, %p )\n", id, profile, size );

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
        return FALSE;
    }

    profileW = HeapAlloc( GetProcessHeap(), 0, sizeW );

    if (profileW)
    {
        if ((ret = GetStandardColorSpaceProfileW( NULL, id, profileW, &sizeW )))
        {
            *size = WideCharToMultiByte( CP_ACP, 0, profileW, -1, NULL, 0, NULL, NULL );
            len = WideCharToMultiByte( CP_ACP, 0, profileW, -1, profile, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }
        else *size = sizeW / sizeof(WCHAR);

        HeapFree( GetProcessHeap(), 0, profileW );
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
    static const WCHAR rgbprofilefile[] =
        { '\\','s','r','g','b',' ','c','o','l','o','r',' ',
          's','p','a','c','e',' ','p','r','o','f','i','l','e','.','i','c','m',0 };
    WCHAR rgbprofile[MAX_PATH];
    DWORD len = sizeof(rgbprofile);

    TRACE( "( 0x%08x, %p, %p )\n", id, profile, size );

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
        case SPACE_RGB: /* 'RGB ' */
            lstrcatW( rgbprofile, rgbprofilefile );
            len = lstrlenW( rgbprofile ) * sizeof(WCHAR);

            if (*size < len || !profile)
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

static BOOL MSCMS_header_from_file( LPCWSTR file, PPROFILEHEADER header )
{
    BOOL ret;
    PROFILE profile;
    WCHAR path[MAX_PATH], slash[] = {'\\',0};
    DWORD size = sizeof(path);
    HANDLE handle;

    ret = GetColorDirectoryW( NULL, path, &size );
    if (!ret)
    {
        WARN( "Can't retrieve color directory\n" );
        return FALSE;
    }
    if (size + sizeof(slash) + sizeof(WCHAR) * lstrlenW( file ) > sizeof(path))
    {
        WARN( "Filename too long\n" );
        return FALSE;
    }

    lstrcatW( path, slash );
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

static BOOL MSCMS_match_profile( PENUMTYPEW rec, PPROFILEHEADER hdr )
{
    if (rec->dwFields & ET_DEVICENAME)
    {
        FIXME( "ET_DEVICENAME: %s\n", debugstr_w(rec->pDeviceName) );
    }
    if (rec->dwFields & ET_MEDIATYPE)
    {
        FIXME( "ET_MEDIATYPE: 0x%08x\n", rec->dwMediaType );
    }
    if (rec->dwFields & ET_DITHERMODE)
    {
        FIXME( "ET_DITHERMODE: 0x%08x\n", rec->dwDitheringMode );
    }
    if (rec->dwFields & ET_RESOLUTION)
    {
        FIXME( "ET_RESOLUTION: 0x%08x, 0x%08x\n",
               rec->dwResolution[0], rec->dwResolution[1] );
    }
    if (rec->dwFields & ET_DEVICECLASS)
    {
        FIXME( "ET_DEVICECLASS: %s\n", MSCMS_dbgstr_tag(rec->dwMediaType) );
    }
    if (rec->dwFields & ET_CMMTYPE)
    {
        TRACE( "ET_CMMTYPE: %s\n", MSCMS_dbgstr_tag(rec->dwCMMType) );
        if (rec->dwCMMType != hdr->phCMMType) return FALSE;
    }
    if (rec->dwFields & ET_CLASS)
    {
        TRACE( "ET_CLASS: %s\n", MSCMS_dbgstr_tag(rec->dwClass) );
        if (rec->dwClass != hdr->phClass) return FALSE;
    }
    if (rec->dwFields & ET_DATACOLORSPACE)
    {
        TRACE( "ET_DATACOLORSPACE: %s\n", MSCMS_dbgstr_tag(rec->dwDataColorSpace) );
        if (rec->dwDataColorSpace != hdr->phDataColorSpace) return FALSE;
    }
    if (rec->dwFields & ET_CONNECTIONSPACE)
    {
        TRACE( "ET_CONNECTIONSPACE: %s\n", MSCMS_dbgstr_tag(rec->dwConnectionSpace) );
        if (rec->dwConnectionSpace != hdr->phConnectionSpace) return FALSE;
    }
    if (rec->dwFields & ET_SIGNATURE)
    {
        TRACE( "ET_SIGNATURE: %s\n", MSCMS_dbgstr_tag(rec->dwSignature) );
        if (rec->dwSignature != hdr->phSignature) return FALSE;
    }
    if (rec->dwFields & ET_PLATFORM)
    {
        TRACE( "ET_PLATFORM: %s\n", MSCMS_dbgstr_tag(rec->dwPlatform) );
        if (rec->dwPlatform != hdr->phPlatform) return FALSE;
    }
    if (rec->dwFields & ET_PROFILEFLAGS)
    {
        TRACE( "ET_PROFILEFLAGS: 0x%08x\n", rec->dwProfileFlags );
        if (rec->dwProfileFlags != hdr->phProfileFlags) return FALSE;
    }
    if (rec->dwFields & ET_MANUFACTURER)
    {
        TRACE( "ET_MANUFACTURER: %s\n", MSCMS_dbgstr_tag(rec->dwManufacturer) );
        if (rec->dwManufacturer != hdr->phManufacturer) return FALSE;
    }
    if (rec->dwFields & ET_MODEL)
    {
        TRACE( "ET_MODEL: %s\n", MSCMS_dbgstr_tag(rec->dwModel) );
        if (rec->dwModel != hdr->phModel) return FALSE;
    }
    if (rec->dwFields & ET_ATTRIBUTES)
    {
        TRACE( "ET_ATTRIBUTES: 0x%08x, 0x%08x\n",
               rec->dwAttributes[0], rec->dwAttributes[1] );
        if (rec->dwAttributes[0] != hdr->phAttributes[0] || 
            rec->dwAttributes[1] != hdr->phAttributes[1]) return FALSE;
    }
    if (rec->dwFields & ET_RENDERINGINTENT)
    {
        TRACE( "ET_RENDERINGINTENT: 0x%08x\n", rec->dwRenderingIntent );
        if (rec->dwRenderingIntent != hdr->phRenderingIntent) return FALSE;
    }
    if (rec->dwFields & ET_CREATOR)
    {
        TRACE( "ET_CREATOR: %s\n", MSCMS_dbgstr_tag(rec->dwCreator) );
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
    char spec[] = "\\*";
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

    profiles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(char *) + 1 );
    if (!profiles) goto exit;

    memcpy( &recordW, record, sizeof(ENUMTYPEA) );
    if (record->pDeviceName)
    {
        deviceW = MSCMS_strdupW( record->pDeviceName );
        if (!(recordW.pDeviceName = deviceW)) goto exit;
    }

    fileW = MSCMS_strdupW( data.cFileName );
    if (!fileW) goto exit;

    ret = MSCMS_header_from_file( fileW, &header );
    if (ret)
    {
        match = MSCMS_match_profile( &recordW, &header );
        if (match)
        {
            len = sizeof(char) * (lstrlenA( data.cFileName ) + 1);
            profiles[count] = HeapAlloc( GetProcessHeap(), 0, len );

            if (!profiles[count]) goto exit;
            else
            {
                TRACE( "matching profile: %s\n", debugstr_a(data.cFileName) );
                lstrcpyA( profiles[count], data.cFileName );
                totalsize += len;
                count++;
            }
        }
    }
    HeapFree( GetProcessHeap(), 0, fileW );
    fileW = NULL;

    while (FindNextFileA( find, &data ))
    {
        fileW = MSCMS_strdupW( data.cFileName );
        if (!fileW) goto exit;

        ret = MSCMS_header_from_file( fileW, &header );
        if (!ret)
        {
            HeapFree( GetProcessHeap(), 0, fileW );
            continue;
        }

        match = MSCMS_match_profile( &recordW, &header );
        if (match)
        {
            char **tmp = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      profiles, sizeof(char *) * (count + 1) );
            if (!tmp) goto exit;
            else profiles = tmp;

            len = sizeof(char) * (lstrlenA( data.cFileName ) + 1);
            profiles[count] = HeapAlloc( GetProcessHeap(), 0, len );

            if (!profiles[count]) goto exit;
            else
            {
                TRACE( "matching profile: %s\n", debugstr_a(data.cFileName) );
                lstrcpyA( profiles[count], data.cFileName );
                totalsize += len;
                count++;
            }
        }
        HeapFree( GetProcessHeap(), 0, fileW );
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
    else ret = FALSE;

    *size = totalsize;
    if (number) *number = count;

exit:
    for (i = 0; i < count; i++)
        HeapFree( GetProcessHeap(), 0, profiles[i] );
    HeapFree( GetProcessHeap(), 0, profiles );
    HeapFree( GetProcessHeap(), 0, deviceW );
    HeapFree( GetProcessHeap(), 0, fileW );
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
    WCHAR spec[] = {'\\','*',0};
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
    if (!ret || len + sizeof(spec) > MAX_PATH)
    {
        WARN( "Can't retrieve color directory\n" );
        return FALSE;
    }

    lstrcpyW( glob, colordir );
    lstrcatW( glob, spec );

    find = FindFirstFileW( glob, &data );
    if (find == INVALID_HANDLE_VALUE) return FALSE;

    profiles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR *) + 1 );
    if (!profiles) goto exit;

    ret = MSCMS_header_from_file( data.cFileName, &header );
    if (ret)
    {
        match = MSCMS_match_profile( record, &header );
        if (match)
        {
            len = sizeof(WCHAR) * (lstrlenW( data.cFileName ) + 1);
            profiles[count] = HeapAlloc( GetProcessHeap(), 0, len );

            if (!profiles[count]) goto exit;
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
        ret = MSCMS_header_from_file( data.cFileName, &header );
        if (!ret) continue;

        match = MSCMS_match_profile( record, &header );
        if (match)
        {
            WCHAR **tmp = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       profiles, sizeof(WCHAR *) * (count + 1) );
            if (!tmp) goto exit;
            else profiles = tmp;

            len = sizeof(WCHAR) * (lstrlenW( data.cFileName ) + 1);
            profiles[count] = HeapAlloc( GetProcessHeap(), 0, len );

            if (!profiles[count]) goto exit;
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
    else ret = FALSE;

    *size = totalsize;
    if (number) *number = count;

exit:
    for (i = 0; i < count; i++)
        HeapFree( GetProcessHeap(), 0, profiles[i] );
    HeapFree( GetProcessHeap(), 0, profiles );
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
    profileW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (profileW)
    {
        MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );

        ret = InstallColorProfileW( NULL, profileW );
        HeapFree( GetProcessHeap(), 0, profileW );
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
    static const WCHAR slash[] = { '\\', 0 };

    TRACE( "( %s )\n", debugstr_w(profile) );

    if (machine || !profile) return FALSE;

    if (!GetColorDirectoryW( machine, dest, &size )) return FALSE;

    MSCMS_basename( profile, base );

    lstrcatW( dest, slash );
    lstrcatW( dest, base );

    /* Is source equal to destination? */
    if (!lstrcmpW( profile, dest )) return TRUE;

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
BOOL WINAPI IsColorProfileTagPresent( HPROFILE profile, TAGTYPE type, PBOOL present )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD i, count;
    icTag tag;

    TRACE( "( %p, 0x%08x, %p )\n", profile, type, present );

    if (!iccprofile || !present) return FALSE;

    count = MSCMS_get_tag_count( iccprofile );

    for (i = 0; i < count; i++)
    {
        MSCMS_get_tag_by_index( iccprofile, i, &tag );

        if (tag.sig == type)
        {
            *present = ret = TRUE;
            break;
        }
    }

#endif /* HAVE_LCMS */
    return ret;
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
BOOL WINAPI IsColorProfileValid( HPROFILE profile, PBOOL valid )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, valid );

    if (!valid) return FALSE;
    if (iccprofile) return *valid = TRUE;

#endif /* HAVE_LCMS */
    return ret;
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
BOOL WINAPI SetColorProfileElement( HPROFILE profile, TAGTYPE type, DWORD offset, PDWORD size,
                                    PVOID buffer )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD i, count, access = MSCMS_hprofile2access( profile );
    icTag tag;

    TRACE( "( %p, 0x%08x, %d, %p, %p )\n", profile, type, offset, size, buffer );

    if (!iccprofile || !size || !buffer) return FALSE;
    if (!(access & PROFILE_READWRITE)) return FALSE;

    count = MSCMS_get_tag_count( iccprofile );

    for (i = 0; i < count; i++)
    {
        MSCMS_get_tag_by_index( iccprofile, i, &tag );

        if (tag.sig == type)
        {
            if (offset > tag.size) return FALSE;

            MSCMS_set_tag_data( iccprofile, &tag, offset, buffer );
            return TRUE;
        }
    }

#endif /* HAVE_LCMS */
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
BOOL WINAPI SetColorProfileHeader( HPROFILE profile, PPROFILEHEADER header )
{
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD access = MSCMS_hprofile2access( profile );

    TRACE( "( %p, %p )\n", profile, header );

    if (!iccprofile || !header) return FALSE;
    if (!(access & PROFILE_READWRITE)) return FALSE;

    MSCMS_set_profile_header( iccprofile, header );
    return TRUE;

#else
    return FALSE;
#endif /* HAVE_LCMS */
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
    profileW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (profileW)
    {
        MultiByteToWideChar( CP_ACP, 0, profile, -1, profileW, len );

        ret = UninstallColorProfileW( NULL, profileW , delete );

        HeapFree( GetProcessHeap(), 0, profileW );
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

/******************************************************************************
 * OpenColorProfileA               [MSCMS.@]
 *
 * See OpenColorProfileW.
 */
HPROFILE WINAPI OpenColorProfileA( PPROFILE profile, DWORD access, DWORD sharing, DWORD creation )
{
    HPROFILE handle = NULL;

    TRACE( "( %p, 0x%08x, 0x%08x, 0x%08x )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    /* No AW conversion needed for memory based profiles */
    if (profile->dwType & PROFILE_MEMBUFFER)
        return OpenColorProfileW( profile, access, sharing, creation );

    if (profile->dwType & PROFILE_FILENAME)
    {
        UINT len;
        PROFILE profileW;

        profileW.dwType = profile->dwType;
 
        len = MultiByteToWideChar( CP_ACP, 0, profile->pProfileData, -1, NULL, 0 );
        profileW.pProfileData = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

        if (profileW.pProfileData)
        {
            profileW.cbDataSize = len * sizeof(WCHAR);
            MultiByteToWideChar( CP_ACP, 0, profile->pProfileData, -1, profileW.pProfileData, len );

            handle = OpenColorProfileW( &profileW, access, sharing, creation );
            HeapFree( GetProcessHeap(), 0, profileW.pProfileData );
        }
    }
    return handle;
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
#ifdef HAVE_LCMS
    cmsHPROFILE cmsprofile = NULL;
    icProfile *iccprofile = NULL;
    HANDLE handle = NULL;
    DWORD size;

    TRACE( "( %p, 0x%08x, 0x%08x, 0x%08x )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    if (profile->dwType & PROFILE_MEMBUFFER)
    {
        /* FIXME: access flags not implemented for memory based profiles */

        iccprofile = profile->pProfileData;
        size = profile->cbDataSize;
    
        cmsprofile = cmsOpenProfileFromMem( iccprofile, size );
    }

    if (profile->dwType & PROFILE_FILENAME)
    {
        DWORD read, flags = 0;

        TRACE( "profile file: %s\n", debugstr_w( (WCHAR *)profile->pProfileData ) );

        if (access & PROFILE_READ) flags = GENERIC_READ;
        if (access & PROFILE_READWRITE) flags = GENERIC_READ|GENERIC_WRITE;

        if (!flags) return NULL;

        handle = CreateFileW( profile->pProfileData, flags, sharing, NULL, creation, 0, NULL );
        if (handle == INVALID_HANDLE_VALUE)
        {
            WARN( "Unable to open color profile\n" );
            return NULL;
        }

        if ((size = GetFileSize( handle, NULL )) == INVALID_FILE_SIZE)
        {
            ERR( "Unable to retrieve size of color profile\n" );
            CloseHandle( handle );
            return NULL;
        }

        iccprofile = HeapAlloc( GetProcessHeap(), 0, size );
        if (!iccprofile)
        {
            ERR( "Unable to allocate memory for color profile\n" );
            CloseHandle( handle );
            return NULL;
        }

        if (!ReadFile( handle, iccprofile, size, &read, NULL ) || read != size)
        {
            ERR( "Unable to read color profile\n" );

            CloseHandle( handle );
            HeapFree( GetProcessHeap(), 0, iccprofile );
            return NULL;
        }

        cmsprofile = cmsOpenProfileFromMem( iccprofile, size );
    }

    if (cmsprofile)
        return MSCMS_create_hprofile_handle( handle, iccprofile, cmsprofile, access );

#endif /* HAVE_LCMS */
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
BOOL WINAPI CloseColorProfile( HPROFILE profile )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    HANDLE file = MSCMS_hprofile2handle( profile );
    DWORD access = MSCMS_hprofile2access( profile );

    TRACE( "( %p )\n", profile );

    if (file && (access & PROFILE_READWRITE))
    {
        DWORD written, size = MSCMS_get_profile_size( iccprofile );

        if (SetFilePointer( file, 0, NULL, FILE_BEGIN ) ||
            !WriteFile( file, iccprofile, size, &written, NULL ) || written != size)
            ERR( "Unable to write color profile\n" );
    }

    ret = cmsCloseProfile( MSCMS_hprofile2cmsprofile( profile ) );
    HeapFree( GetProcessHeap(), 0, MSCMS_hprofile2iccprofile( profile ) );

    CloseHandle( MSCMS_hprofile2handle( profile ) );
    MSCMS_destroy_hprofile_handle( profile );

#endif /* HAVE_LCMS */
    return ret;
}
