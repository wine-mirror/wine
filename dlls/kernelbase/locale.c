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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winuser.h"
#include "winternl.h"
#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);


static const WCHAR codepages_key[] =
{ 'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
  '\\','C','o','n','t','r','o','l','\\','N','l','s','\\','C','o','d','e','P','a','g','e',0};
static const WCHAR language_groups_key[] =
{ 'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
  '\\','C','o','n','t','r','o','l','\\','N','l','s',
  '\\','L','a','n','g','u','a','g','e',' ','G','r','o','u','p','s',0 };
static const WCHAR locales_key[] =
{ 'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
  '\\','C','o','n','t','r','o','l','\\','N','l','s','\\','L','o','c','a','l','e',0};
static const WCHAR altsort_key[] = {'A','l','t','e','r','n','a','t','e',' ','S','o','r','t','s',0};


/**************************************************************************
 *	Internal_EnumDateFormats   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumDateFormats( DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags,
                                                        BOOL unicode, BOOL ex, BOOL exex, LPARAM lparam )
{
    WCHAR buffer[256];
    LCTYPE lctype;
    CALID cal_id;
    INT ret;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!GetLocaleInfoW( lcid, LOCALE_ICALENDARTYPE|LOCALE_RETURN_NUMBER,
                         (LPWSTR)&cal_id, sizeof(cal_id)/sizeof(WCHAR) ))
        return FALSE;

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        lctype = LOCALE_SSHORTDATE;
        break;
    case DATE_LONGDATE:
        lctype = LOCALE_SLONGDATE;
        break;
    case DATE_YEARMONTH:
        lctype = LOCALE_SYEARMONTH;
        break;
    default:
        FIXME( "unknown date format 0x%08x\n", flags );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lctype |= flags & LOCALE_USE_CP_ACP;
    if (unicode)
        ret = GetLocaleInfoW( lcid, lctype, buffer, ARRAY_SIZE(buffer) );
    else
        ret = GetLocaleInfoA( lcid, lctype, (char *)buffer, sizeof(buffer) );

    if (ret)
    {
        if (exex) ((DATEFMT_ENUMPROCEXEX)proc)( buffer, cal_id, lparam );
        else if (ex) ((DATEFMT_ENUMPROCEXW)proc)( buffer, cal_id );
        else proc( buffer );
    }
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumLanguageGroupLocales   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumLanguageGroupLocales( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                                DWORD flags, LONG_PTR param, BOOL unicode )
{
    WCHAR name[10], value[10];
    DWORD name_len, value_len, type, index = 0, alt = 0;
    HKEY key, altkey;
    LCID lcid;

    if (!proc || id < LGRPID_WESTERN_EUROPE || id > LGRPID_ARMENIAN)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, locales_key, 0, KEY_READ, &key )) return FALSE;
    if (RegOpenKeyExW( key, altsort_key, 0, KEY_READ, &altkey )) altkey = 0;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        if (RegEnumValueW( alt ? altkey : key, index++, name, &name_len, NULL,
                           &type, (BYTE *)value, &value_len ))
        {
            if (alt++) break;
            index = 0;
            continue;
        }
        if (type != REG_SZ) continue;
        if (id != wcstoul( value, NULL, 16 )) continue;
        lcid = wcstoul( name, NULL, 16 );
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((LANGGROUPLOCALE_ENUMPROCA)proc)( id, lcid, nameA, param )) break;
        }
        else if (!proc( id, lcid, name, param )) break;
    }
    RegCloseKey( altkey );
    RegCloseKey( key );
    return TRUE;
}


/***********************************************************************
 *	Internal_EnumSystemCodePages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumSystemCodePages( CODEPAGE_ENUMPROCW proc, DWORD flags,
                                                            BOOL unicode )
{
    WCHAR name[10];
    DWORD name_len, type, index = 0;
    HKEY key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, codepages_key, 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, NULL, NULL )) break;
        if (type != REG_SZ) continue;
        if (!wcstoul( name, NULL, 10 )) continue;
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((CODEPAGE_ENUMPROCA)proc)( nameA )) break;
        }
        else if (!proc( name )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumSystemLanguageGroups   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumSystemLanguageGroups( LANGUAGEGROUP_ENUMPROCW proc,
                                                                DWORD flags, LONG_PTR param, BOOL unicode )
{
    WCHAR name[10], value[10], descr[80];
    DWORD name_len, value_len, type, index = 0;
    HKEY key;
    LGRPID id;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (flags)
    {
    case 0:
        flags = LGRPID_INSTALLED;
        break;
    case LGRPID_INSTALLED:
    case LGRPID_SUPPORTED:
        break;
    default:
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, language_groups_key, 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, (BYTE *)value, &value_len )) break;
        if (type != REG_SZ) continue;

        id = wcstoul( name, NULL, 16 );

        if (!(flags & LGRPID_SUPPORTED) && !wcstoul( value, NULL, 10 )) continue;
        if (!LoadStringW( kernel32_handle, 0x2000 + id, descr, ARRAY_SIZE(descr) )) descr[0] = 0;
        TRACE( "%p: %u %s %s %x %lx\n", proc, id, debugstr_w(name), debugstr_w(descr), flags, param );
        if (!unicode)
        {
            char nameA[10], descrA[80];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, descr, -1, descrA, sizeof(descrA), NULL, NULL );
            if (!((LANGUAGEGROUP_ENUMPROCA)proc)( id, nameA, descrA, flags, param )) break;
        }
        else if (!proc( id, name, descr, flags, param )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/**************************************************************************
 *	Internal_EnumTimeFormats   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumTimeFormats( TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags,
                                                        BOOL unicode, BOOL ex, LPARAM lparam )
{
    WCHAR buffer[256];
    LCTYPE lctype;
    INT ret;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
        lctype = LOCALE_STIMEFORMAT;
        break;
    case TIME_NOSECONDS:
        lctype = LOCALE_SSHORTTIME;
        break;
    default:
        FIXME( "Unknown time format %x\n", flags );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lctype |= flags & LOCALE_USE_CP_ACP;
    if (unicode)
        ret = GetLocaleInfoW( lcid, lctype, buffer, ARRAY_SIZE(buffer) );
    else
        ret = GetLocaleInfoA( lcid, lctype, (char *)buffer, sizeof(buffer) );

    if (ret)
    {
        if (ex) ((TIMEFMT_ENUMPROCEX)proc)( buffer, lparam );
        else proc( buffer );
    }
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumUILanguages( UILANGUAGE_ENUMPROCW proc, DWORD flags,
                                                        LONG_PTR param, BOOL unicode )
{
    WCHAR name[10];
    DWORD name_len, type, index = 0;
    HKEY key;

    if (!proc)
    {
	SetLastError( ERROR_INVALID_PARAMETER );
	return FALSE;
    }
    if (flags & ~MUI_LANGUAGE_ID)
    {
	SetLastError( ERROR_INVALID_FLAGS );
	return FALSE;
    }

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, locales_key, 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, NULL, NULL )) break;
        if (type != REG_SZ) continue;
        if (!wcstoul( name, NULL, 16 )) continue;
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((UILANGUAGE_ENUMPROCA)proc)( nameA, param )) break;
        }
        else if (!proc( name, param )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


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


/**************************************************************************
 *	EnumDateFormatsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsW( DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumDateFormats( proc, lcid, flags, TRUE, FALSE, FALSE, 0 );
}


/**************************************************************************
 *	EnumDateFormatsExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsExW( DATEFMT_ENUMPROCEXW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, lcid, flags, TRUE, TRUE, FALSE, 0 );
}


/**************************************************************************
 *	EnumDateFormatsExEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsExEx( DATEFMT_ENUMPROCEXEX proc, const WCHAR *locale,
                                                   DWORD flags, LPARAM lparam )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, lcid, flags, TRUE, TRUE, TRUE, lparam );
}


/******************************************************************************
 *	EnumLanguageGroupLocalesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumLanguageGroupLocalesW( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                         DWORD flags, LONG_PTR param )
{
    return Internal_EnumLanguageGroupLocales( proc, id, flags, param, TRUE );
}


/******************************************************************************
 *	EnumUILanguagesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumUILanguagesW( UILANGUAGE_ENUMPROCW proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumUILanguages( proc, flags, param, TRUE );
}


/***********************************************************************
 *	EnumSystemCodePagesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemCodePagesW( CODEPAGE_ENUMPROCW proc, DWORD flags )
{
    return Internal_EnumSystemCodePages( proc, flags, TRUE );
}


/******************************************************************************
 *	EnumSystemLanguageGroupsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLanguageGroupsW( LANGUAGEGROUP_ENUMPROCW proc,
                                                         DWORD flags, LONG_PTR param )
{
    return Internal_EnumSystemLanguageGroups( proc, flags, param, TRUE );
}


/******************************************************************************
 *	EnumSystemLocalesA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesA( LOCALE_ENUMPROCA proc, DWORD flags )
{
    char name[10];
    DWORD name_len, type, index = 0;
    HKEY key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, locales_key, 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueA( key, index++, name, &name_len, NULL, &type, NULL, NULL )) break;
        if (type != REG_SZ) continue;
        if (!strtoul( name, NULL, 16 )) continue;
        if (!proc( name )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLocalesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesW( LOCALE_ENUMPROCW proc, DWORD flags )
{
    WCHAR name[10];
    DWORD name_len, type, index = 0;
    HKEY key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, locales_key, 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, NULL, NULL )) break;
        if (type != REG_SZ) continue;
        if (!wcstoul( name, NULL, 16 )) continue;
        if (!proc( name )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLocalesEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesEx( LOCALE_ENUMPROCEX proc, DWORD wanted_flags,
                                                   LPARAM param, void *reserved )
{
    WCHAR buffer[256], name[10];
    DWORD name_len, type, neutral, flags, index = 0, alt = 0;
    HKEY key, altkey;
    LCID lcid;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, locales_key, 0, KEY_READ, &key )) return FALSE;
    if (RegOpenKeyExW( key, altsort_key, 0, KEY_READ, &altkey )) altkey = 0;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueW( alt ? altkey : key, index++, name, &name_len, NULL, &type, NULL, NULL ))
        {
            if (alt++) break;
            index = 0;
            continue;
        }
        if (type != REG_SZ) continue;
        if (!(lcid = wcstoul( name, NULL, 16 ))) continue;

        GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE, buffer, ARRAY_SIZE( buffer ));
        if (!GetLocaleInfoW( lcid, LOCALE_INEUTRAL | LOCALE_NOUSEROVERRIDE | LOCALE_RETURN_NUMBER,
                             (LPWSTR)&neutral, sizeof(neutral) / sizeof(WCHAR) ))
            neutral = 0;

        if (alt)
            flags = LOCALE_ALTERNATE_SORTS;
        else
            flags = LOCALE_WINDOWS | (neutral ? LOCALE_NEUTRALDATA : LOCALE_SPECIFICDATA);

        if (wanted_flags && !(flags & wanted_flags)) continue;
        if (!proc( buffer, flags, param )) break;
    }
    RegCloseKey( altkey );
    RegCloseKey( key );
    return TRUE;
}


/**************************************************************************
 *	EnumTimeFormatsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumTimeFormatsW( TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumTimeFormats( proc, lcid, flags, TRUE, FALSE, 0 );
}


/**************************************************************************
 *	EnumTimeFormatsEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumTimeFormatsEx( TIMEFMT_ENUMPROCEX proc, const WCHAR *locale,
                                                 DWORD flags, LPARAM lparam )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return Internal_EnumTimeFormats( (TIMEFMT_ENUMPROCW)proc, lcid, flags, TRUE, TRUE, lparam );
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


/******************************************************************************
 *	IsValidLanguageGroup   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLanguageGroup( LGRPID id, DWORD flags )
{
    static const WCHAR format[] = { '%','x',0 };
    WCHAR name[10], value[10];
    DWORD type, value_len = sizeof(value);
    BOOL ret = FALSE;
    HKEY key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, language_groups_key, 0, KEY_READ, &key )) return FALSE;

    swprintf( name, ARRAY_SIZE(name), format, id );
    if (!RegQueryValueExW( key, name, NULL, &type, (BYTE *)value, &value_len ) && type == REG_SZ)
        ret = (flags & LGRPID_SUPPORTED) || wcstoul( value, NULL, 10 );

    RegCloseKey( key );
    return ret;
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
