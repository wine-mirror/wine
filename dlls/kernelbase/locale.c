/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2002 Alexandre Julliard for CodeWeavers
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);


/******************************************************************************
 *	CompareStringOrdinal   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringOrdinal( const WCHAR *str1, INT len1,
                                                   const WCHAR *str2, INT len2, BOOL ignore_case )
{
    int ret;

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len1 < 0) len1 = lstrlenW( str1 );
    if (len2 < 0) len2 = lstrlenW( str2 );

    ret = RtlCompareUnicodeStrings( str1, len1, str2, len2, ignore_case );
    if (ret < 0) return CSTR_LESS_THAN;
    if (ret > 0) return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}


/******************************************************************************
 *	FindStringOrdinal   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindStringOrdinal( DWORD flag, const WCHAR *src, INT src_size,
                                                const WCHAR *val, INT val_size, BOOL ignore_case )
{
    INT offset, inc, count;

    TRACE( "%#x %s %d %s %d %d\n", flag, wine_dbgstr_w(src), src_size,
           wine_dbgstr_w(val), val_size, ignore_case );

    if (!src || !val)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (flag != FIND_FROMSTART && flag != FIND_FROMEND && flag != FIND_STARTSWITH && flag != FIND_ENDSWITH)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return -1;
    }

    if (src_size == -1) src_size = lstrlenW( src );
    if (val_size == -1) val_size = lstrlenW( val );

    SetLastError( ERROR_SUCCESS );
    src_size -= val_size;
    if (src_size < 0) return -1;

    count = flag & (FIND_FROMSTART | FIND_FROMEND) ? src_size + 1 : 1;
    offset = flag & (FIND_FROMSTART | FIND_STARTSWITH) ? 0 : src_size;
    inc = flag & (FIND_FROMSTART | FIND_STARTSWITH) ? 1 : -1;
    while (count--)
    {
        if (CompareStringOrdinal( src + offset, val_size, val, val_size, ignore_case ) == CSTR_EQUAL)
            return offset;
        offset += inc;
    }
    return -1;
}


/***********************************************************************
 *	LCIDToLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCIDToLocaleName( LCID lcid, LPWSTR name, INT count, DWORD flags )
{
    static int once;
    if (flags && !once++) FIXME( "unsupported flags %x\n", flags );

    return GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE, name, count );
}


/***********************************************************************
 *	GetSystemDefaultLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLCID(void)
{
    LCID lcid;
    NtQueryDefaultLocale( FALSE, &lcid );
    return lcid;
}


/***********************************************************************
 *	GetSystemDefaultLangID   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLangID(void)
{
    return LANGIDFROMLCID( GetSystemDefaultLCID() );
}


/***********************************************************************
 *	GetSystemDefaultLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLocaleName( LPWSTR name, INT len )
{
    return LCIDToLocaleName( GetSystemDefaultLCID(), name, len, 0 );
}


/***********************************************************************
 *	GetSystemDefaultUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryInstallUILanguage( &lang );
    return lang;
}


/***********************************************************************
 *	GetUserDefaultLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH GetUserDefaultLCID(void)
{
    LCID lcid;
    NtQueryDefaultLocale( TRUE, &lcid );
    return lcid;
}


/***********************************************************************
 *	GetUserDefaultLangID   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetUserDefaultLangID(void)
{
    return LANGIDFROMLCID( GetUserDefaultLCID() );
}


/***********************************************************************
 *	GetUserDefaultLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetUserDefaultLocaleName( LPWSTR name, INT len )
{
    return LCIDToLocaleName( GetUserDefaultLCID(), name, len, 0 );
}


/***********************************************************************
 *	GetUserDefaultUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetUserDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryDefaultUILanguage( &lang );
    return lang;
}


/******************************************************************************
 *	ResolveLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH ResolveLocaleName( LPCWSTR name, LPWSTR buffer, INT len )
{
    FIXME( "stub: %s, %p, %d\n", wine_dbgstr_w(name), buffer, len );

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}


/***********************************************************************
 *	VerLanguageNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH VerLanguageNameA( DWORD lang, LPSTR buffer, DWORD size )
{
    return GetLocaleInfoA( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SENGLANGUAGE, buffer, size );
}


/***********************************************************************
 *	VerLanguageNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH VerLanguageNameW( DWORD lang, LPWSTR buffer, DWORD size )
{
    return GetLocaleInfoW( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SENGLANGUAGE, buffer, size );
}
