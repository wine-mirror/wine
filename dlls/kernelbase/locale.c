/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2002, 2019 Alexandre Julliard
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

#define CALINFO_MAX_YEAR 2029

static HANDLE kernel32_handle;

static const struct registry_value
{
    DWORD           lctype;
    const WCHAR    *name;
} registry_values[] =
{
    { LOCALE_ICALENDARTYPE, L"iCalendarType" },
    { LOCALE_ICURRDIGITS, L"iCurrDigits" },
    { LOCALE_ICURRENCY, L"iCurrency" },
    { LOCALE_IDIGITS, L"iDigits" },
    { LOCALE_IFIRSTDAYOFWEEK, L"iFirstDayOfWeek" },
    { LOCALE_IFIRSTWEEKOFYEAR, L"iFirstWeekOfYear" },
    { LOCALE_ILZERO, L"iLZero" },
    { LOCALE_IMEASURE, L"iMeasure" },
    { LOCALE_INEGCURR, L"iNegCurr" },
    { LOCALE_INEGNUMBER, L"iNegNumber" },
    { LOCALE_IPAPERSIZE, L"iPaperSize" },
    { LOCALE_ITIME, L"iTime" },
    { LOCALE_S1159, L"s1159" },
    { LOCALE_S2359, L"s2359" },
    { LOCALE_SCURRENCY, L"sCurrency" },
    { LOCALE_SDATE, L"sDate" },
    { LOCALE_SDECIMAL, L"sDecimal" },
    { LOCALE_SGROUPING, L"sGrouping" },
    { LOCALE_SLIST, L"sList" },
    { LOCALE_SLONGDATE, L"sLongDate" },
    { LOCALE_SMONDECIMALSEP, L"sMonDecimalSep" },
    { LOCALE_SMONGROUPING, L"sMonGrouping" },
    { LOCALE_SMONTHOUSANDSEP, L"sMonThousandSep" },
    { LOCALE_SNEGATIVESIGN, L"sNegativeSign" },
    { LOCALE_SPOSITIVESIGN, L"sPositiveSign" },
    { LOCALE_SSHORTDATE, L"sShortDate" },
    { LOCALE_STHOUSAND, L"sThousand" },
    { LOCALE_STIME, L"sTime" },
    { LOCALE_STIMEFORMAT, L"sTimeFormat" },
    { LOCALE_SYEARMONTH, L"sYearMonth" },
    /* The following are not listed under MSDN as supported,
     * but seem to be used and also stored in the registry.
     */
    { LOCALE_SNAME, L"LocaleName" },
    { LOCALE_ICOUNTRY, L"iCountry" },
    { LOCALE_IDATE, L"iDate" },
    { LOCALE_ILDATE, L"iLDate" },
    { LOCALE_ITLZERO, L"iTLZero" },
    { LOCALE_SCOUNTRY, L"sCountry" },
    { LOCALE_SABBREVLANGNAME, L"sLanguage" },
    { LOCALE_IDIGITSUBSTITUTION, L"Numshape" },
    { LOCALE_SNATIVEDIGITS, L"sNativeDigits" },
    { LOCALE_ITIMEMARKPOSN, L"iTimePrefix" },
};

static NLSTABLEINFO nls_info;
static UINT mac_cp = 10000;
static HKEY intl_key;
static HKEY nls_key;


/***********************************************************************
 *		init_locale
 */
void init_locale(void)
{
    UINT ansi_cp = 0, oem_cp = 0;
    USHORT *ansi_ptr, *oem_ptr, *casemap_ptr;
    LCID lcid = GetUserDefaultLCID();
    WCHAR bufferW[80];
    DWORD count, i;
    SIZE_T size;
    HKEY hkey;

    kernel32_handle = GetModuleHandleW( L"kernel32.dll" );

    GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&ansi_cp, sizeof(ansi_cp)/sizeof(WCHAR) );
    GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&mac_cp, sizeof(mac_cp)/sizeof(WCHAR) );
    GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&oem_cp, sizeof(oem_cp)/sizeof(WCHAR) );

    NtGetNlsSectionPtr( 10, 0, 0, (void **)&casemap_ptr, &size );
    if (!ansi_cp || NtGetNlsSectionPtr( 11, ansi_cp, NULL, (void **)&ansi_ptr, &size ))
        NtGetNlsSectionPtr( 11, 1252, NULL, (void **)&ansi_ptr, &size );
    if (!oem_cp || NtGetNlsSectionPtr( 11, oem_cp, 0, (void **)&oem_ptr, &size ))
        NtGetNlsSectionPtr( 11, 437, NULL, (void **)&oem_ptr, &size );
    NtCurrentTeb()->Peb->AnsiCodePageData = ansi_ptr;
    NtCurrentTeb()->Peb->OemCodePageData = oem_ptr;
    NtCurrentTeb()->Peb->UnicodeCaseTableData = casemap_ptr;
    RtlInitNlsTables( ansi_ptr, oem_ptr, casemap_ptr, &nls_info );
    RtlResetRtlTranslations( &nls_info );

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Nls",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nls_key, NULL );
    RegCreateKeyExW( HKEY_CURRENT_USER, L"Control Panel\\International",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &intl_key, NULL );

    /* Update registry contents if the user locale has changed.
     * This simulates the action of the Windows control panel. */

    count = sizeof(bufferW);
    if (!RegQueryValueExW( intl_key, L"Locale", NULL, NULL, (BYTE *)bufferW, &count ))
    {
        if (wcstoul( bufferW, NULL, 16 ) == lcid) return;  /* already set correctly */
        TRACE( "updating registry, locale changed %s -> %08x\n", debugstr_w(bufferW), lcid );
    }
    else TRACE( "updating registry, locale changed none -> %08x\n", lcid );
    swprintf( bufferW, ARRAY_SIZE(bufferW), L"%08x", lcid );
    RegSetValueExW( intl_key, L"Locale", 0, REG_SZ,
                    (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );

    for (i = 0; i < ARRAY_SIZE(registry_values); i++)
    {
        GetLocaleInfoW( LOCALE_USER_DEFAULT, registry_values[i].lctype | LOCALE_NOUSEROVERRIDE,
                        bufferW, ARRAY_SIZE( bufferW ));
        RegSetValueExW( intl_key, registry_values[i].name, 0, REG_SZ,
                        (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );
    }

    if (!RegCreateKeyExW( nls_key, L"Codepage",
                          0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", ansi_cp );
        RegSetValueExW( hkey, L"ACP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", oem_cp );
        RegSetValueExW( hkey, L"OEMCP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", mac_cp );
        RegSetValueExW( hkey, L"MACCP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }
}


static UINT get_lcid_codepage( LCID lcid, ULONG flags )
{
    UINT ret = GetACP();

    if (!(flags & LOCALE_USE_CP_ACP) && lcid != GetSystemDefaultLCID())
        GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                        (WCHAR *)&ret, sizeof(ret)/sizeof(WCHAR) );
    return ret;
}


/* Note: the Internal_ functions are not documented. The number of parameters
 * should be correct, but their exact meaning may not.
 */

/******************************************************************************
 *	Internal_EnumCalendarInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumCalendarInfo( CALINFO_ENUMPROCW proc, LCID lcid, CALID id,
                                                         CALTYPE type, BOOL unicode, BOOL ex,
                                                         BOOL exex, LPARAM lparam )
{
    WCHAR buffer[256];
    DWORD optional = 0;
    INT ret;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (id == ENUM_ALL_CALENDARS)
    {
        if (!GetLocaleInfoW( lcid, LOCALE_ICALENDARTYPE | LOCALE_RETURN_NUMBER,
                             (WCHAR *)&id, sizeof(id) / sizeof(WCHAR) )) return FALSE;
        if (!GetLocaleInfoW( lcid, LOCALE_IOPTIONALCALENDAR | LOCALE_RETURN_NUMBER,
                             (WCHAR *)&optional, sizeof(optional) / sizeof(WCHAR) )) optional = 0;
    }

    for (;;)
    {
        if (type & CAL_RETURN_NUMBER)
            ret = GetCalendarInfoW( lcid, id, type, NULL, 0, (LPDWORD)buffer );
        else if (unicode)
            ret = GetCalendarInfoW( lcid, id, type, buffer, ARRAY_SIZE(buffer), NULL );
        else
        {
            WCHAR bufW[256];
            ret = GetCalendarInfoW( lcid, id, type, bufW, ARRAY_SIZE(bufW), NULL );
            if (ret) WideCharToMultiByte( CP_ACP, 0, bufW, -1, (char *)buffer, sizeof(buffer), NULL, NULL );
        }

        if (ret)
        {
            if (exex) ret = ((CALINFO_ENUMPROCEXEX)proc)( buffer, id, NULL, lparam );
            else if (ex) ret = ((CALINFO_ENUMPROCEXW)proc)( buffer, id );
            else ret = proc( buffer );
        }
        if (!ret) break;
        if (!optional) break;
        id = optional;
    }
    return TRUE;
}


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

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;
    if (RegOpenKeyExW( key, L"Alternate Sorts", 0, KEY_READ, &altkey )) altkey = 0;

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

    if (RegOpenKeyExW( nls_key, L"Codepage", 0, KEY_READ, &key )) return FALSE;

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

    if (RegOpenKeyExW( nls_key, L"Language Groups", 0, KEY_READ, &key )) return FALSE;

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

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;

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


/******************************************************************************
 *	ConvertDefaultLocale   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH ConvertDefaultLocale( LCID lcid )
{
    switch (lcid)
    {
    case LOCALE_INVARIANT:
        return lcid; /* keep as-is */
    case LOCALE_SYSTEM_DEFAULT:
        return GetSystemDefaultLCID();
    case LOCALE_USER_DEFAULT:
    case LOCALE_NEUTRAL:
        return GetUserDefaultLCID();
    case MAKELANGID( LANG_CHINESE, SUBLANG_NEUTRAL ):
    case MAKELANGID( LANG_CHINESE, 0x1e ):
        return MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
    case MAKELANGID( LANG_CHINESE, 0x1f ):
        return MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG );
    case MAKELANGID( LANG_SPANISH, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_SPANISH, SUBLANG_SPANISH_MODERN );
    case MAKELANGID( LANG_IRISH, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_IRISH, SUBLANG_IRISH_IRELAND );
    case MAKELANGID( LANG_BENGALI, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_BENGALI, SUBLANG_BENGALI_BANGLADESH );
    case MAKELANGID( LANG_SINDHI, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_SINDHI, SUBLANG_SINDHI_AFGHANISTAN );
    case MAKELANGID( LANG_INUKTITUT, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_INUKTITUT, SUBLANG_INUKTITUT_CANADA_LATIN );
    case MAKELANGID( LANG_TAMAZIGHT, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_TAMAZIGHT, SUBLANG_TAMAZIGHT_ALGERIA_LATIN );
    case MAKELANGID( LANG_FULAH, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_FULAH, SUBLANG_FULAH_SENEGAL );
    case MAKELANGID( LANG_TIGRINYA, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_TIGRINYA, SUBLANG_TIGRINYA_ERITREA );
    default:
        /* Replace SUBLANG_NEUTRAL with SUBLANG_DEFAULT */
        if (SUBLANGID(lcid) == SUBLANG_NEUTRAL && SORTIDFROMLCID(lcid) == SORT_DEFAULT)
            lcid = MAKELANGID( PRIMARYLANGID(lcid), SUBLANG_DEFAULT );
        break;
    }
    return lcid;
}


/******************************************************************************
 *	EnumCalendarInfoW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoW( CALINFO_ENUMPROCW proc, LCID lcid,
                                                 CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( proc, lcid, id, type, TRUE, FALSE, FALSE, 0 );
}


/******************************************************************************
 *	EnumCalendarInfoExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoExW( CALINFO_ENUMPROCEXW proc, LCID lcid,
                                                   CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, lcid, id, type, TRUE, TRUE, FALSE, 0 );
}

/******************************************************************************
 *	EnumCalendarInfoExEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoExEx( CALINFO_ENUMPROCEXEX proc, LPCWSTR locale, CALID id,
                                                    LPCWSTR reserved, CALTYPE type, LPARAM lparam )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, lcid, id, type, TRUE, TRUE, TRUE, lparam );
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

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;

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

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;

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

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;
    if (RegOpenKeyExW( key, L"Alternate Sorts", 0, KEY_READ, &altkey )) altkey = 0;

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
 *	GetACP   (kernelbase.@)
 */
UINT WINAPI GetACP(void)
{
    return nls_info.AnsiTableInfo.CodePage;
}


/***********************************************************************
 *	GetCalendarInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetCalendarInfoW( LCID lcid, CALID calendar, CALTYPE type,
                                               WCHAR *data, INT count, DWORD *value )
{
    static const LCTYPE lctype_map[] =
    {
        0, /* not used */
        0, /* CAL_ICALINTVALUE */
        0, /* CAL_SCALNAME */
        0, /* CAL_IYEAROFFSETRANGE */
        0, /* CAL_SERASTRING */
        LOCALE_SSHORTDATE,
        LOCALE_SLONGDATE,
        LOCALE_SDAYNAME1,
        LOCALE_SDAYNAME2,
        LOCALE_SDAYNAME3,
        LOCALE_SDAYNAME4,
        LOCALE_SDAYNAME5,
        LOCALE_SDAYNAME6,
        LOCALE_SDAYNAME7,
        LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2,
        LOCALE_SABBREVDAYNAME3,
        LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5,
        LOCALE_SABBREVDAYNAME6,
        LOCALE_SABBREVDAYNAME7,
        LOCALE_SMONTHNAME1,
        LOCALE_SMONTHNAME2,
        LOCALE_SMONTHNAME3,
        LOCALE_SMONTHNAME4,
        LOCALE_SMONTHNAME5,
        LOCALE_SMONTHNAME6,
        LOCALE_SMONTHNAME7,
        LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME9,
        LOCALE_SMONTHNAME10,
        LOCALE_SMONTHNAME11,
        LOCALE_SMONTHNAME12,
        LOCALE_SMONTHNAME13,
        LOCALE_SABBREVMONTHNAME1,
        LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3,
        LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5,
        LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7,
        LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9,
        LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11,
        LOCALE_SABBREVMONTHNAME12,
        LOCALE_SABBREVMONTHNAME13,
        LOCALE_SYEARMONTH,
        0, /* CAL_ITWODIGITYEARMAX */
        LOCALE_SSHORTESTDAYNAME1,
        LOCALE_SSHORTESTDAYNAME2,
        LOCALE_SSHORTESTDAYNAME3,
        LOCALE_SSHORTESTDAYNAME4,
        LOCALE_SSHORTESTDAYNAME5,
        LOCALE_SSHORTESTDAYNAME6,
        LOCALE_SSHORTESTDAYNAME7,
        LOCALE_SMONTHDAY,
        0, /* CAL_SABBREVERASTRING */
    };
    DWORD flags = 0;
    CALTYPE calinfo = type & 0xffff;

    if (type & CAL_NOUSEROVERRIDE) FIXME("flag CAL_NOUSEROVERRIDE used, not fully implemented\n");
    if (type & CAL_USE_CP_ACP) FIXME("flag CAL_USE_CP_ACP used, not fully implemented\n");

    if ((type & CAL_RETURN_NUMBER) && !value)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (type & CAL_RETURN_GENITIVE_NAMES) flags |= LOCALE_RETURN_GENITIVE_NAMES;

    switch (calinfo)
    {
    case CAL_ICALINTVALUE:
        if (type & CAL_RETURN_NUMBER)
            return GetLocaleInfoW( lcid, LOCALE_RETURN_NUMBER | LOCALE_ICALENDARTYPE,
                                   (WCHAR *)value, sizeof(*value) / sizeof(WCHAR) );
        return GetLocaleInfoW( lcid, LOCALE_ICALENDARTYPE, data, count );

    case CAL_SCALNAME:
        FIXME( "Unimplemented caltype %d\n", calinfo );
        if (data) *data = 0;
        return 1;

    case CAL_IYEAROFFSETRANGE:
    case CAL_SERASTRING:
    case CAL_SABBREVERASTRING:
        FIXME( "Unimplemented caltype %d\n", calinfo );
        return 0;

    case CAL_SSHORTDATE:
    case CAL_SLONGDATE:
    case CAL_SDAYNAME1:
    case CAL_SDAYNAME2:
    case CAL_SDAYNAME3:
    case CAL_SDAYNAME4:
    case CAL_SDAYNAME5:
    case CAL_SDAYNAME6:
    case CAL_SDAYNAME7:
    case CAL_SABBREVDAYNAME1:
    case CAL_SABBREVDAYNAME2:
    case CAL_SABBREVDAYNAME3:
    case CAL_SABBREVDAYNAME4:
    case CAL_SABBREVDAYNAME5:
    case CAL_SABBREVDAYNAME6:
    case CAL_SABBREVDAYNAME7:
    case CAL_SMONTHNAME1:
    case CAL_SMONTHNAME2:
    case CAL_SMONTHNAME3:
    case CAL_SMONTHNAME4:
    case CAL_SMONTHNAME5:
    case CAL_SMONTHNAME6:
    case CAL_SMONTHNAME7:
    case CAL_SMONTHNAME8:
    case CAL_SMONTHNAME9:
    case CAL_SMONTHNAME10:
    case CAL_SMONTHNAME11:
    case CAL_SMONTHNAME12:
    case CAL_SMONTHNAME13:
    case CAL_SABBREVMONTHNAME1:
    case CAL_SABBREVMONTHNAME2:
    case CAL_SABBREVMONTHNAME3:
    case CAL_SABBREVMONTHNAME4:
    case CAL_SABBREVMONTHNAME5:
    case CAL_SABBREVMONTHNAME6:
    case CAL_SABBREVMONTHNAME7:
    case CAL_SABBREVMONTHNAME8:
    case CAL_SABBREVMONTHNAME9:
    case CAL_SABBREVMONTHNAME10:
    case CAL_SABBREVMONTHNAME11:
    case CAL_SABBREVMONTHNAME12:
    case CAL_SABBREVMONTHNAME13:
    case CAL_SMONTHDAY:
    case CAL_SYEARMONTH:
    case CAL_SSHORTESTDAYNAME1:
    case CAL_SSHORTESTDAYNAME2:
    case CAL_SSHORTESTDAYNAME3:
    case CAL_SSHORTESTDAYNAME4:
    case CAL_SSHORTESTDAYNAME5:
    case CAL_SSHORTESTDAYNAME6:
    case CAL_SSHORTESTDAYNAME7:
        return GetLocaleInfoW( lcid, lctype_map[calinfo] | flags, data, count );

    case CAL_ITWODIGITYEARMAX:
        if (type & CAL_RETURN_NUMBER)
        {
            *value = CALINFO_MAX_YEAR;
            return sizeof(DWORD) / sizeof(WCHAR);
        }
        else
        {
            WCHAR buffer[10];
            int ret = swprintf( buffer, ARRAY_SIZE(buffer), L"%u", CALINFO_MAX_YEAR ) + 1;
            if (!data) return ret;
            if (ret <= count)
            {
                lstrcpyW( data, buffer );
                return ret;
            }
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }
        break;
    default:
        FIXME( "Unknown caltype %d\n", calinfo );
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    return 0;
}


/***********************************************************************
 *	GetCalendarInfoEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetCalendarInfoEx( const WCHAR *locale, CALID calendar, const WCHAR *reserved,
                                                CALTYPE type, WCHAR *data, INT count, DWORD *value )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return GetCalendarInfoW( lcid, calendar, type, data, count, value );
}


/******************************************************************************
 *	GetLocaleInfoA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoA( LCID lcid, LCTYPE lctype, char *buffer, INT len )
{
    WCHAR *bufferW;
    INT lenW, ret;

    TRACE( "lcid=0x%x lctype=0x%x %p %d\n", lcid, lctype, buffer, len );

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (LOWORD(lctype) == LOCALE_SSHORTTIME || (lctype & LOCALE_RETURN_GENITIVE_NAMES))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (LOWORD(lctype) == LOCALE_FONTSIGNATURE || (lctype & LOCALE_RETURN_NUMBER))
        return GetLocaleInfoW( lcid, lctype, (WCHAR *)buffer, len / sizeof(WCHAR) ) * sizeof(WCHAR);

    if (!(lenW = GetLocaleInfoW( lcid, lctype, NULL, 0 ))) return 0;

    if (!(bufferW = RtlAllocateHeap( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    ret = GetLocaleInfoW( lcid, lctype, bufferW, lenW );
    if (ret) ret = WideCharToMultiByte( get_lcid_codepage( lcid, lctype ), 0,
                                        bufferW, ret, buffer, len, NULL, NULL );
    RtlFreeHeap( GetProcessHeap(), 0, bufferW );
    return ret;
}


/******************************************************************************
 *	GetLocaleInfoEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoEx( const WCHAR *locale, LCTYPE info, WCHAR *buffer, INT len )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );

    TRACE( "%s lcid=0x%x 0x%x\n", debugstr_w(locale), lcid, info );

    if (!lcid) return 0;

    /* special handling for neutral locale names */
    if (locale && lstrlenW( locale ) == 2)
    {
        switch (LOWORD( info ))
        {
        case LOCALE_SNAME:
            if (len && len < 3)
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }
            if (len) lstrcpyW( buffer, locale );
            return 3;
        case LOCALE_SPARENT:
            if (len) buffer[0] = 0;
            return 1;
        }
    }
    return GetLocaleInfoW( lcid, info, buffer, len );
}


/******************************************************************************
 *	GetOEMCP   (kernelbase.@)
 */
UINT WINAPI GetOEMCP(void)
{
    return nls_info.OemTableInfo.CodePage;
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
 *	IsNormalizedString   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsNormalizedString( NORM_FORM form, const WCHAR *str, INT len )
{
    BOOLEAN res;
    if (!set_ntstatus( RtlIsNormalizedString( form, str, len, &res ))) res = FALSE;
    return res;
}


/******************************************************************************
 *	IsValidLanguageGroup   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLanguageGroup( LGRPID id, DWORD flags )
{
    WCHAR name[10], value[10];
    DWORD type, value_len = sizeof(value);
    BOOL ret = FALSE;
    HKEY key;

    if (RegOpenKeyExW( nls_key, L"Language Groups", 0, KEY_READ, &key )) return FALSE;

    swprintf( name, ARRAY_SIZE(name), L"%x", id );
    if (!RegQueryValueExW( key, name, NULL, &type, (BYTE *)value, &value_len ) && type == REG_SZ)
        ret = (flags & LGRPID_SUPPORTED) || wcstoul( value, NULL, 10 );

    RegCloseKey( key );
    return ret;
}


/******************************************************************************
 *	IsValidLocale   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLocale( LCID lcid, DWORD flags )
{
    /* check if language is registered in the kernel32 resources */
    return FindResourceExW( kernel32_handle, (LPWSTR)RT_STRING,
                            ULongToPtr( (LOCALE_ILANGUAGE >> 4) + 1 ), LANGIDFROMLCID(lcid)) != 0;
}


/******************************************************************************
 *	IsValidLocaleName   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLocaleName( const WCHAR *locale )
{
    LCID lcid;

    return !RtlLocaleNameToLcid( locale, &lcid, 2 );
}


/***********************************************************************
 *	LCIDToLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCIDToLocaleName( LCID lcid, WCHAR *name, INT count, DWORD flags )
{
    static int once;
    if (flags && !once++) FIXME( "unsupported flags %x\n", flags );

    return GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE, name, count );
}


/***********************************************************************
 *	LocaleNameToLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH LocaleNameToLCID( const WCHAR *name, DWORD flags )
{
    LCID lcid;

    if (!name) return GetUserDefaultLCID();
    if (!set_ntstatus( RtlLocaleNameToLcid( name, &lcid, 2 ))) return 0;
    if (!(flags & LOCALE_ALLOW_NEUTRAL_NAMES)) lcid = ConvertDefaultLocale( lcid );
    return lcid;
}


/******************************************************************************
 *	NormalizeString   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH NormalizeString(NORM_FORM form, const WCHAR *src, INT src_len,
                                             WCHAR *dst, INT dst_len)
{
    set_ntstatus( RtlNormalizeString( form, src, src_len, dst, &dst_len ));
    return dst_len;
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
 *	SetCalendarInfoW   (kernelbase.@)
 */
INT WINAPI /* DECLSPEC_HOTPATCH */ SetCalendarInfoW( LCID lcid, CALID calendar, CALTYPE type, const WCHAR *data )
{
    FIXME( "(%08x,%08x,%08x,%s): stub\n", lcid, calendar, type, debugstr_w(data) );
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
