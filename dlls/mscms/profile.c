/*
 * MSCMS - Color Management System for Wine
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

#include "config.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#define LCMS_API_FUNCTION(f) extern typeof(f) * p##f;
#include "lcms_api.h"
#undef LCMS_API_FUNCTION

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

static void MSCMS_basename( LPCWSTR path, LPWSTR name )
{
    INT i = lstrlenW( path );

    while (i > 0 && !IS_SEPARATOR(path[i - 1])) i--;
    lstrcpyW( name, &path[i] );
}

#ifdef HAVE_LCMS_H
static BOOL MSCMS_cpu_is_little_endian()
{
    long l = 1;
    void *p = &l;
    char b = *(char *)p;

    return b ? TRUE : FALSE;
}

static void MSCMS_endian_swap16( BYTE *byte )
{
    BYTE tmp;

    tmp = byte[0];
    byte[0] = byte[1];
    byte[1] = tmp;
}

static void MSCMS_endian_swap32( BYTE *byte )
{
    BYTE tmp1, tmp2;

    tmp1 = *byte++;
    tmp2 = *byte++;

    *(byte - 1) = *byte;
    *byte++ = tmp2;
    *(byte - 3) = *byte;
    *byte = tmp1;
}

static void MSCMS_adjust_endianess32( BYTE *byte )
{
    if (MSCMS_cpu_is_little_endian())
        MSCMS_endian_swap32( byte );
}

static void MSCMS_get_profile_header( icProfile *iccprofile, PROFILEHEADER *header )
{
    memcpy( header, iccprofile, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */ 

    if (MSCMS_cpu_is_little_endian())
    {
        MSCMS_endian_swap32( (BYTE *)&header->phSize );
        MSCMS_endian_swap32( (BYTE *)&header->phCMMType );
        MSCMS_endian_swap32( (BYTE *)&header->phVersion );
        MSCMS_endian_swap32( (BYTE *)&header->phClass );
        MSCMS_endian_swap32( (BYTE *)&header->phDataColorSpace );
        MSCMS_endian_swap32( (BYTE *)&header->phConnectionSpace );

        MSCMS_endian_swap32( (BYTE *)&header->phDateTime[0] );
        MSCMS_endian_swap32( (BYTE *)&header->phDateTime[1] );
        MSCMS_endian_swap32( (BYTE *)&header->phDateTime[2] );

        MSCMS_endian_swap32( (BYTE *)&header->phSignature );
        MSCMS_endian_swap32( (BYTE *)&header->phPlatform );
        MSCMS_endian_swap32( (BYTE *)&header->phProfileFlags );
        MSCMS_endian_swap32( (BYTE *)&header->phManufacturer );
        MSCMS_endian_swap32( (BYTE *)&header->phModel );

        MSCMS_endian_swap32( (BYTE *)&header->phAttributes[0] );
        MSCMS_endian_swap32( (BYTE *)&header->phAttributes[1] );

        MSCMS_endian_swap32( (BYTE *)&header->phRenderingIntent );
        MSCMS_endian_swap32( (BYTE *)&header->phIlluminant );
        MSCMS_endian_swap32( (BYTE *)&header->phCreator );
    }
}

static void MSCMS_set_profile_header( icProfile *iccprofile, PROFILEHEADER *header )
{
    icHeader *iccheader = (icHeader *)iccprofile;

    memcpy( iccprofile, header, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */

    if (MSCMS_cpu_is_little_endian())
    {
        MSCMS_endian_swap32( (BYTE *)&iccheader->size );
        MSCMS_endian_swap32( (BYTE *)&iccheader->cmmId );
        MSCMS_endian_swap32( (BYTE *)&iccheader->version );
        MSCMS_endian_swap32( (BYTE *)&iccheader->deviceClass );
        MSCMS_endian_swap32( (BYTE *)&iccheader->colorSpace );
        MSCMS_endian_swap32( (BYTE *)&iccheader->pcs );

        MSCMS_endian_swap16( (BYTE *)&iccheader->date.year );
        MSCMS_endian_swap16( (BYTE *)&iccheader->date.month );
        MSCMS_endian_swap16( (BYTE *)&iccheader->date.day );
        MSCMS_endian_swap16( (BYTE *)&iccheader->date.hours );
        MSCMS_endian_swap16( (BYTE *)&iccheader->date.minutes );
        MSCMS_endian_swap16( (BYTE *)&iccheader->date.seconds );

        MSCMS_endian_swap32( (BYTE *)&iccheader->magic );
        MSCMS_endian_swap32( (BYTE *)&iccheader->platform );
        MSCMS_endian_swap32( (BYTE *)&iccheader->flags );
        MSCMS_endian_swap32( (BYTE *)&iccheader->manufacturer );
        MSCMS_endian_swap32( (BYTE *)&iccheader->model );

        MSCMS_endian_swap32( (BYTE *)&iccheader->attributes[0] );
        MSCMS_endian_swap32( (BYTE *)&iccheader->attributes[1] );

        MSCMS_endian_swap32( (BYTE *)&iccheader->renderingIntent );
        MSCMS_endian_swap32( (BYTE *)&iccheader->illuminant );
        MSCMS_endian_swap32( (BYTE *)&iccheader->creator );
    }
}

static DWORD MSCMS_get_tag_count( icProfile *iccprofile )
{
    DWORD count = iccprofile->count;

    MSCMS_adjust_endianess32( (BYTE *)&count );
    return count;
}

static void MSCMS_get_tag_by_index( icProfile *iccprofile, DWORD index, icTag *tag )
{
    icTag *tmp = (icTag *)&iccprofile->data + (index * sizeof(icTag));

    tag->sig = tmp->sig;
    tag->offset = tmp->offset;
    tag->size = tmp->size;

    MSCMS_adjust_endianess32( (BYTE *)&tag->sig );
    MSCMS_adjust_endianess32( (BYTE *)&tag->offset );
    MSCMS_adjust_endianess32( (BYTE *)&tag->size );
}

static void MSCMS_get_tag_data( icProfile *iccprofile, icTag *tag, DWORD offset, void *buffer )
{
    memcpy( buffer, (char *)iccprofile + tag->offset + offset, tag->size - offset );
}
#endif /* HAVE_LCMS_H */

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
    DWORD sizeW = *size * sizeof(WCHAR);

    TRACE( "( %p, %ld )\n", buffer, *size );

    if (machine || !buffer) return FALSE;

    bufferW = HeapAlloc( GetProcessHeap(), 0, sizeW );

    if (bufferW)
    {
        ret = GetColorDirectoryW( NULL, bufferW, &sizeW );
        *size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );

        if (ret)
        {
            len = WideCharToMultiByte( CP_ACP, 0, bufferW, *size, buffer, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }

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
 *  buffer   [I]   Buffer to recieve the path name in.
 *  size     [I/O] Size of the buffer in bytes. On return the variable holds
 *                 the number of bytes actually needed.
 */
BOOL WINAPI GetColorDirectoryW( PCWSTR machine, PWSTR buffer, PDWORD size )
{
    /* FIXME: Get this directory from the registry? */
    static const WCHAR colordir[] =
    { 'c',':','\\','w','i','n','d','o','w','s','\\', 's','y','s','t','e','m','3','2',
      '\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\','c','o','l','o','r',0 };

    DWORD len;

    TRACE( "( %p, %ld )\n", buffer, *size );

    if (machine || !buffer) return FALSE;

    len = lstrlenW( colordir ) * sizeof(WCHAR);

    if (len <= *size)
    {
        lstrcpyW( buffer, colordir );
        return TRUE;
    }

    *size = len;
    return FALSE;
}

BOOL WINAPI GetColorProfileElement( HPROFILE profile, TAGTYPE type, DWORD offset, PDWORD size,
                                    PVOID buffer, PBOOL ref )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD i, count;
    icTag tag;

    TRACE( "( %p, 0x%08lx, %ld, %p, %p, %p )\n", profile, type, offset, size, buffer, ref );

    if (!iccprofile || !ref) return FALSE;
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

#endif /* HAVE_LCMS_H */
    return ret;
}

/******************************************************************************
 * GetColorProfileElementTag               [MSCMS.@]
 *
 * Get a tag name from a color profile by index. 
 *
 * PARAMS
 *  profile  [I]   Handle to a color profile.
 *  index    [I]   Index into the tag table of the color profile.
 *  tag      [O]   Pointer to a TAGTYPE variable.
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
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    DWORD count;
    icTag tag;

    TRACE( "( %p, %ld, %p )\n", profile, index, type );

    if (!iccprofile) return FALSE;

    count = MSCMS_get_tag_count( iccprofile );
    if (index > count) return FALSE;

    MSCMS_get_tag_by_index( iccprofile, index - 1, &tag );
    *type = tag.sig;

    ret = TRUE;

#endif /* HAVE_LCMS_H */
    return ret;
}

BOOL WINAPI GetColorProfileFromHandle( HPROFILE profile, PBYTE buffer, PDWORD size )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );
    PROFILEHEADER header;

    TRACE( "( %p, %p, %p )\n", profile, buffer, size );

    if (!iccprofile) return FALSE;
    MSCMS_get_profile_header( profile, &header );

    if (!buffer || header.phSize > *size)
    {
        *size = header.phSize;
        return FALSE;
    }

    /* FIXME: no endian conversion */
    memcpy( buffer, iccprofile, header.phSize );
    ret = TRUE;

#endif /* HAVE_LCMS_H */
    return ret;
}

BOOL WINAPI GetColorProfileHeader( HPROFILE profile, PPROFILEHEADER header )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, header );

    if (!iccprofile || !header) return FALSE;

    MSCMS_get_profile_header( iccprofile, header );
    return TRUE;

#endif /* HAVE_LCMS_H */
    return ret;
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
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, count );

    if (!count) return FALSE;
    *count = MSCMS_get_tag_count( iccprofile );
    ret = TRUE;

#endif /* HAVE_LCMS_H */
    return ret;
}

BOOL WINAPI GetStandardColorSpaceProfileA( PCSTR machine, DWORD id, PSTR profile, PDWORD size )
{
    INT len;
    LPWSTR profileW;
    BOOL ret = FALSE;
    DWORD sizeW = *size * sizeof(WCHAR);

    TRACE( "( 0x%08lx, %p, %ld )\n", id, profile, *size );

    if (machine || !profile) return FALSE;

    profileW = HeapAlloc( GetProcessHeap(), 0, sizeW );

    if (profileW)
    {
        ret = GetStandardColorSpaceProfileW( NULL, id, profileW, &sizeW );
        *size = WideCharToMultiByte( CP_ACP, 0, profileW, -1, NULL, 0, NULL, NULL );

        if (ret)
        {
            len = WideCharToMultiByte( CP_ACP, 0, profileW, *size, profile, *size, NULL, NULL );
            if (!len) ret = FALSE;
        }

        HeapFree( GetProcessHeap(), 0, profileW );
    }
    return ret;
}

BOOL WINAPI GetStandardColorSpaceProfileW( PCWSTR machine, DWORD id, PWSTR profile, PDWORD size )
{
    FIXME( "( %lx, %p, %ld ) stub\n", id, profile, *size );

    return FALSE;
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
 * Determine if a given ICC tag is present in a color profile.
 *
 * PARAMS
 *  profile  [I] Color profile handle.
 *  tag      [I] ICC tag.
 *  present  [O] Pointer to a BOOL variable. Set to TRUE if tag is present,
 *               FALSE otherwise.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI IsColorProfileTagPresent( HPROFILE profile, TAGTYPE tag, PBOOL present )
{
    BOOL ret = FALSE;

    TRACE( "( %p, 0x%08lx, %p )\n", profile, tag, present );

    if (!present) return FALSE;

#ifdef HAVE_LCMS_H
    ret = cmsIsTag( MSCMS_hprofile2cmsprofile( profile ), tag );

#endif /* HAVE_LCMS_H */
    return *present = ret;
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
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, valid );

    if (!valid) return FALSE;
    if (iccprofile) return *valid = TRUE;

#endif /* HAVE_LCMS_H */
    return ret;
}

BOOL WINAPI SetColorProfileHeader( HPROFILE profile, PPROFILEHEADER header )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS_H
    icProfile *iccprofile = MSCMS_hprofile2iccprofile( profile );

    TRACE( "( %p, %p )\n", profile, header );

    if (!iccprofile || !header) return FALSE;

    MSCMS_set_profile_header( iccprofile, header );
    return TRUE;

#endif /* HAVE_LCMS_H */
    return ret;
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

    TRACE( "( %p, 0x%08lx, 0x%08lx, 0x%08lx )\n", profile, access, sharing, creation );

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
#ifdef HAVE_LCMS_H
    cmsHPROFILE cmsprofile = NULL;
    icProfile *iccprofile = NULL;
    HANDLE handle = NULL;
    DWORD size;

    TRACE( "( %p, 0x%08lx, 0x%08lx, 0x%08lx )\n", profile, access, sharing, creation );

    if (!profile || !profile->pProfileData) return NULL;

    if (profile->dwType & PROFILE_MEMBUFFER)
    {
        FIXME( "access flags not implemented for memory based profiles\n" );

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

        iccprofile = (icProfile *)HeapAlloc( GetProcessHeap(), 0, size );
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
            HeapFree( GetProcessHeap, 0, iccprofile );
            return NULL;
        }

        cmsprofile = cmsOpenProfileFromMem( iccprofile, size );
    }

    if (cmsprofile)
        return MSCMS_create_hprofile_handle( handle, iccprofile, cmsprofile );

#endif /* HAVE_LCMS_H */
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

    TRACE( "( %p )\n", profile );

#ifdef HAVE_LCMS_H
    ret = cmsCloseProfile( MSCMS_hprofile2cmsprofile( profile ) );

    HeapFree( GetProcessHeap(), 0, MSCMS_hprofile2iccprofile( profile ) );
    CloseHandle( MSCMS_hprofile2handle( profile ) );

    MSCMS_destroy_hprofile_handle( profile );

#endif /* HAVE_LCMS_H */
    return ret;
}
