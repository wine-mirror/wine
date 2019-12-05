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

extern UINT CDECL __wine_get_unix_codepage(void);
extern unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen ) DECLSPEC_HIDDEN;
extern WCHAR wine_compose( const WCHAR *str ) DECLSPEC_HIDDEN;

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

static const struct { UINT cp; const WCHAR *name; } codepage_names[] =
{
    { 37,    L"IBM EBCDIC US Canada" },
    { 424,   L"IBM EBCDIC Hebrew" },
    { 437,   L"OEM United States" },
    { 500,   L"IBM EBCDIC International" },
    { 737,   L"OEM Greek 437G" },
    { 775,   L"OEM Baltic" },
    { 850,   L"OEM Multilingual Latin 1" },
    { 852,   L"OEM Slovak Latin 2" },
    { 855,   L"OEM Cyrillic" },
    { 856,   L"Hebrew PC" },
    { 857,   L"OEM Turkish" },
    { 860,   L"OEM Portuguese" },
    { 861,   L"OEM Icelandic" },
    { 862,   L"OEM Hebrew" },
    { 863,   L"OEM Canadian French" },
    { 864,   L"OEM Arabic" },
    { 865,   L"OEM Nordic" },
    { 866,   L"OEM Russian" },
    { 869,   L"OEM Greek" },
    { 874,   L"ANSI/OEM Thai" },
    { 875,   L"IBM EBCDIC Greek" },
    { 878,   L"Russian KOI8" },
    { 932,   L"ANSI/OEM Japanese Shift-JIS" },
    { 936,   L"ANSI/OEM Simplified Chinese GBK" },
    { 949,   L"ANSI/OEM Korean Unified Hangul" },
    { 950,   L"ANSI/OEM Traditional Chinese Big5" },
    { 1006,  L"IBM Arabic" },
    { 1026,  L"IBM EBCDIC Latin 5 Turkish" },
    { 1250,  L"ANSI Eastern Europe" },
    { 1251,  L"ANSI Cyrillic" },
    { 1252,  L"ANSI Latin 1" },
    { 1253,  L"ANSI Greek" },
    { 1254,  L"ANSI Turkish" },
    { 1255,  L"ANSI Hebrew" },
    { 1256,  L"ANSI Arabic" },
    { 1257,  L"ANSI Baltic" },
    { 1258,  L"ANSI/OEM Viet Nam" },
    { 1361,  L"Korean Johab" },
    { 10000, L"Mac Roman" },
    { 10001, L"Mac Japanese" },
    { 10002, L"Mac Traditional Chinese" },
    { 10003, L"Mac Korean" },
    { 10004, L"Mac Arabic" },
    { 10005, L"Mac Hebrew" },
    { 10006, L"Mac Greek" },
    { 10007, L"Mac Cyrillic" },
    { 10008, L"Mac Simplified Chinese" },
    { 10010, L"Mac Romanian" },
    { 10017, L"Mac Ukrainian" },
    { 10021, L"Mac Thai" },
    { 10029, L"Mac Latin 2" },
    { 10079, L"Mac Icelandic" },
    { 10081, L"Mac Turkish" },
    { 10082, L"Mac Croatian" },
    { 20127, L"US-ASCII (7bit)" },
    { 20866, L"Russian KOI8" },
    { 20932, L"EUC-JP" },
    { 21866, L"Ukrainian KOI8" },
    { 28591, L"ISO 8859-1 Latin 1" },
    { 28592, L"ISO 8859-2 Latin 2 (East European)" },
    { 28593, L"ISO 8859-3 Latin 3 (South European)" },
    { 28594, L"ISO 8859-4 Latin 4 (Baltic old)" },
    { 28595, L"ISO 8859-5 Cyrillic" },
    { 28596, L"ISO 8859-6 Arabic" },
    { 28597, L"ISO 8859-7 Greek" },
    { 28598, L"ISO 8859-8 Hebrew" },
    { 28599, L"ISO 8859-9 Latin 5 (Turkish)" },
    { 28600, L"ISO 8859-10 Latin 6 (Nordic)" },
    { 28601, L"ISO 8859-11 Latin (Thai)" },
    { 28603, L"ISO 8859-13 Latin 7 (Baltic)" },
    { 28604, L"ISO 8859-14 Latin 8 (Celtic)" },
    { 28605, L"ISO 8859-15 Latin 9 (Euro)" },
    { 28606, L"ISO 8859-16 Latin 10 (Balkan)" },
    { 65000, L"Unicode (UTF-7)" },
    { 65001, L"Unicode (UTF-8)" }
};

/* Unicode expanded ligatures */
static const WCHAR ligatures[][5] =
{
    { 0x00c6,  'A','E',0 },
    { 0x00de,  'T','H',0 },
    { 0x00df,  's','s',0 },
    { 0x00e6,  'a','e',0 },
    { 0x00fe,  't','h',0 },
    { 0x0132,  'I','J',0 },
    { 0x0133,  'i','j',0 },
    { 0x0152,  'O','E',0 },
    { 0x0153,  'o','e',0 },
    { 0x01c4,  'D',0x017d,0 },
    { 0x01c5,  'D',0x017e,0 },
    { 0x01c6,  'd',0x017e,0 },
    { 0x01c7,  'L','J',0 },
    { 0x01c8,  'L','j',0 },
    { 0x01c9,  'l','j',0 },
    { 0x01ca,  'N','J',0 },
    { 0x01cb,  'N','j',0 },
    { 0x01cc,  'n','j',0 },
    { 0x01e2,  0x0100,0x0112,0 },
    { 0x01e3,  0x0101,0x0113,0 },
    { 0x01f1,  'D','Z',0 },
    { 0x01f2,  'D','z',0 },
    { 0x01f3,  'd','z',0 },
    { 0x01fc,  0x00c1,0x00c9,0 },
    { 0x01fd,  0x00e1,0x00e9,0 },
    { 0x05f0,  0x05d5,0x05d5,0 },
    { 0x05f1,  0x05d5,0x05d9,0 },
    { 0x05f2,  0x05d9,0x05d9,0 },
    { 0xfb00,  'f','f',0 },
    { 0xfb01,  'f','i',0 },
    { 0xfb02,  'f','l',0 },
    { 0xfb03,  'f','f','i',0 },
    { 0xfb04,  'f','f','l',0 },
    { 0xfb05,  0x017f,'t',0 },
    { 0xfb06,  's','t',0 },
};

static NLSTABLEINFO nls_info;
static UINT mac_cp = 10000;
static HKEY intl_key;
static HKEY nls_key;

static CPTABLEINFO codepages[128];
static unsigned int nb_codepages;

static CRITICAL_SECTION locale_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &locale_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": locale_section") }
};
static CRITICAL_SECTION locale_section = { &critsect_debug, -1, 0, 0, 0, 0 };

/***********************************************************************
 *		init_locale
 */
void init_locale(void)
{
    UINT ansi_cp = 0, oem_cp = 0;
    USHORT *ansi_ptr, *oem_ptr, *casemap_ptr;
    LCID lcid = GetUserDefaultLCID();
    WCHAR bufferW[80];
    GEOID geoid = GEOID_NOT_AVAILABLE;
    DWORD count, dispos, i;
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

    if (!RegCreateKeyExW( intl_key, L"Geo", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, &dispos ))
    {
        if (dispos == REG_CREATED_NEW_KEY)
        {
            GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IGEOID | LOCALE_RETURN_NUMBER,
                            (WCHAR *)&geoid, sizeof(geoid) / sizeof(WCHAR) );
            SetUserGeoID( geoid );
        }
        RegCloseKey( hkey );
    }

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

    if (geoid == GEOID_NOT_AVAILABLE)
    {
        GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IGEOID | LOCALE_RETURN_NUMBER,
                        (WCHAR *)&geoid, sizeof(geoid) / sizeof(WCHAR) );
        SetUserGeoID( geoid );
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


static const CPTABLEINFO *get_codepage_table( UINT codepage )
{
    unsigned int i;
    USHORT *ptr;
    SIZE_T size;

    switch (codepage)
    {
    case CP_ACP:
        return &nls_info.AnsiTableInfo;
    case CP_OEMCP:
        return &nls_info.OemTableInfo;
    case CP_MACCP:
        codepage = mac_cp;
        break;
    case CP_THREAD_ACP:
        if (NtCurrentTeb()->CurrentLocale == GetUserDefaultLCID()) return &nls_info.AnsiTableInfo;
        codepage = get_lcid_codepage( NtCurrentTeb()->CurrentLocale, 0 );
        if (!codepage) return &nls_info.AnsiTableInfo;
        break;
    default:
        if (codepage == nls_info.AnsiTableInfo.CodePage) return &nls_info.AnsiTableInfo;
        if (codepage == nls_info.OemTableInfo.CodePage) return &nls_info.OemTableInfo;
        break;
    }

    RtlEnterCriticalSection( &locale_section );

    for (i = 0; i < nb_codepages; i++) if (codepages[i].CodePage == codepage) goto done;

    if (i == ARRAY_SIZE( codepages ))
    {
        RtlLeaveCriticalSection( &locale_section );
        ERR( "too many codepages\n" );
        return NULL;
    }
    if (NtGetNlsSectionPtr( 11, codepage, NULL, (void **)&ptr, &size ))
    {
        RtlLeaveCriticalSection( &locale_section );
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    RtlInitCodePageTable( ptr, &codepages[i] );
    nb_codepages++;
done:
    RtlLeaveCriticalSection( &locale_section );
    return &codepages[i];
}


static const WCHAR *get_ligature( WCHAR wc )
{
    int low = 0, high = ARRAY_SIZE( ligatures ) -1;
    while (low <= high)
    {
        int pos = (low + high) / 2;
        if (ligatures[pos][0] < wc) low = pos + 1;
        else if (ligatures[pos][0] > wc) high = pos - 1;
        else return ligatures[pos] + 1;
    }
    return NULL;
}


static int expand_ligatures( const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    int i, len, pos = 0;
    const WCHAR *expand;

    for (i = 0; i < srclen; i++)
    {
        if (!(expand = get_ligature( src[i] )))
        {
            expand = src + i;
            len = 1;
        }
        else len = lstrlenW( expand );

        if (dstlen)
        {
            if (pos + len > dstlen) break;
            memcpy( dst + pos, expand, len * sizeof(WCHAR) );
        }
        pos += len;
    }
    return pos;
}


static int fold_digits( const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    extern const WCHAR wine_digitmap[] DECLSPEC_HIDDEN;
    int i;

    if (!dstlen) return srclen;
    if (srclen > dstlen) return 0;
    for (i = 0; i < srclen; i++)
        dst[i] = src[i] + wine_digitmap[wine_digitmap[src[i] >> 8] + (src[i] & 0xff)];
    return srclen;
}


static int mbstowcs_cpsymbol( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    int len, i;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) return srclen;
    len = min( srclen, dstlen );
    for (i = 0; i < len; i++)
    {
        unsigned char c = src[i];
        dst[i] = (c < 0x20) ? c : c + 0xf000;
    }
    if (len < srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return len;
}


static int mbstowcs_utf7( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    static const signed char base64_decoding_table[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20-0x2F */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40-0x4F */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50-0x5F */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60-0x6F */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70-0x7F */
    };

    const char *source_end = src + srclen;
    int offset = 0, pos = 0;
    DWORD byte_pair = 0;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
#define OUTPUT(ch) \
    do { \
        if (dstlen > 0) \
        { \
            if (pos >= dstlen) goto overflow; \
            dst[pos] = (ch); \
        } \
        pos++; \
    } while(0)

    while (src < source_end)
    {
        if (*src == '+')
        {
            src++;
            if (src >= source_end) break;
            if (*src == '-')
            {
                /* just a plus sign escaped as +- */
                OUTPUT( '+' );
                src++;
                continue;
            }

            do
            {
                signed char sextet = *src;
                if (sextet == '-')
                {
                    /* skip over the dash and end base64 decoding
                     * the current, unfinished byte pair is discarded */
                    src++;
                    offset = 0;
                    break;
                }
                if (sextet < 0)
                {
                    /* the next character of src is < 0 and therefore not part of a base64 sequence
                     * the current, unfinished byte pair is NOT discarded in this case
                     * this is probably a bug in Windows */
                    break;
                }
                sextet = base64_decoding_table[sextet];
                if (sextet == -1)
                {
                    /* -1 means that the next character of src is not part of a base64 sequence
                     * in other words, all sextets in this base64 sequence have been processed
                     * the current, unfinished byte pair is discarded */
                    offset = 0;
                    break;
                }

                byte_pair = (byte_pair << 6) | sextet;
                offset += 6;
                if (offset >= 16)
                {
                    /* this byte pair is done */
                    OUTPUT( byte_pair >> (offset - 16) );
                    offset -= 16;
                }
                src++;
            }
            while (src < source_end);
        }
        else
        {
            OUTPUT( (unsigned char)*src );
            src++;
        }
    }
    return pos;

overflow:
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return 0;
#undef OUTPUT
}


static int mbstowcs_utf8( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    DWORD reslen;
    NTSTATUS status;

    if (flags & ~(MB_PRECOMPOSED | MB_COMPOSITE | MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) dst = NULL;
    status = RtlUTF8ToUnicodeN( dst, dstlen * sizeof(WCHAR), &reslen, src, srclen );
    if (status == STATUS_SOME_NOT_MAPPED)
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    else if (!set_ntstatus( status )) reslen = 0;

    return reslen / sizeof(WCHAR);
}


static inline int is_private_use_area_char( WCHAR code )
{
    return (code >= 0xe000 && code <= 0xf8ff);
}


static int check_invalid_chars( const CPTABLEINFO *info, const unsigned char *src, int srclen )
{
    if (info->DBCSOffsets)
    {
        for ( ; srclen; src++, srclen-- )
        {
            USHORT off = info->DBCSOffsets[*src];
            if (off)
            {
                if (srclen == 1) break;  /* partial char, error */
                if (info->DBCSOffsets[off + src[1]] == info->UniDefaultChar &&
                    ((src[0] << 8) | src[1]) != info->TransDefaultChar) break;
                src++;
                srclen--;
                continue;
            }
            if (info->MultiByteTable[*src] == info->UniDefaultChar && *src != info->TransDefaultChar)
                break;
            if (is_private_use_area_char( info->MultiByteTable[*src] )) break;
        }
    }
    else
    {
        for ( ; srclen; src++, srclen-- )
        {
            if (info->MultiByteTable[*src] == info->UniDefaultChar && *src != info->TransDefaultChar)
                break;
            if (is_private_use_area_char( info->MultiByteTable[*src] )) break;
        }
    }
    return !!srclen;

}


static int mbstowcs_decompose( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                               WCHAR *dst, int dstlen )
{
    WCHAR ch, dummy[4]; /* no decomposition is larger than 4 chars */
    USHORT off;
    int len, res;

    if (info->DBCSOffsets)
    {
        if (!dstlen)  /* compute length */
        {
            for (len = 0; srclen; srclen--, src++)
            {
                if ((off = info->DBCSOffsets[*src]))
                {
                    if (srclen > 1 && src[1])
                    {
                        src++;
                        srclen--;
                        ch = info->DBCSOffsets[off + *src];
                    }
                    else ch = info->UniDefaultChar;
                }
                else ch = info->MultiByteTable[*src];
                len += wine_decompose( 0, ch, dummy, 4 );
            }
            return len;
        }

        for (len = dstlen; srclen && len; srclen--, src++)
        {
            if ((off = info->DBCSOffsets[*src]))
            {
                if (srclen > 1 && src[1])
                {
                    src++;
                    srclen--;
                    ch = info->DBCSOffsets[off + *src];
                }
                else ch = info->UniDefaultChar;
            }
            else ch = info->MultiByteTable[*src];
            if (!(res = wine_decompose( 0, ch, dst, len ))) break;
            dst += res;
            len -= res;
        }
    }
    else
    {
        if (!dstlen)  /* compute length */
        {
            for (len = 0; srclen; srclen--, src++)
                len += wine_decompose( 0, info->MultiByteTable[*src], dummy, 4 );
            return len;
        }

        for (len = dstlen; srclen && len; srclen--, src++)
        {
            if (!(res = wine_decompose( 0, info->MultiByteTable[*src], dst, len ))) break;
            len -= res;
            dst += res;
        }
    }

    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - len;
}


static int mbstowcs_sbcs( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                          WCHAR *dst, int dstlen )
{
    const USHORT *table = info->MultiByteTable;
    int ret = srclen;

    if (!dstlen) return srclen;

    if (dstlen < srclen)  /* buffer too small: fill it up to dstlen and return error */
    {
        srclen = dstlen;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }

    while (srclen >= 16)
    {
        dst[0]  = table[src[0]];
        dst[1]  = table[src[1]];
        dst[2]  = table[src[2]];
        dst[3]  = table[src[3]];
        dst[4]  = table[src[4]];
        dst[5]  = table[src[5]];
        dst[6]  = table[src[6]];
        dst[7]  = table[src[7]];
        dst[8]  = table[src[8]];
        dst[9]  = table[src[9]];
        dst[10] = table[src[10]];
        dst[11] = table[src[11]];
        dst[12] = table[src[12]];
        dst[13] = table[src[13]];
        dst[14] = table[src[14]];
        dst[15] = table[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle the remaining characters */
    src += srclen;
    dst += srclen;
    switch (srclen)
    {
    case 15: dst[-15] = table[src[-15]];
    case 14: dst[-14] = table[src[-14]];
    case 13: dst[-13] = table[src[-13]];
    case 12: dst[-12] = table[src[-12]];
    case 11: dst[-11] = table[src[-11]];
    case 10: dst[-10] = table[src[-10]];
    case 9:  dst[-9]  = table[src[-9]];
    case 8:  dst[-8]  = table[src[-8]];
    case 7:  dst[-7]  = table[src[-7]];
    case 6:  dst[-6]  = table[src[-6]];
    case 5:  dst[-5]  = table[src[-5]];
    case 4:  dst[-4]  = table[src[-4]];
    case 3:  dst[-3]  = table[src[-3]];
    case 2:  dst[-2]  = table[src[-2]];
    case 1:  dst[-1]  = table[src[-1]];
    case 0: break;
    }
    return ret;
}


static int mbstowcs_dbcs( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                          WCHAR *dst, int dstlen )
{
    USHORT off;
    int i;

    if (!dstlen)
    {
        for (i = 0; srclen; i++, src++, srclen--)
            if (info->DBCSOffsets[*src] && srclen > 1 && src[1]) { src++; srclen--; }
        return i;
    }

    for (i = dstlen; srclen && i; i--, srclen--, src++, dst++)
    {
        if ((off = info->DBCSOffsets[*src]))
        {
            if (srclen > 1 && src[1])
            {
                src++;
                srclen--;
                *dst = info->DBCSOffsets[off + *src];
            }
            else *dst = info->UniDefaultChar;
        }
        else *dst = info->MultiByteTable[*src];
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int mbstowcs_codepage( UINT codepage, DWORD flags, const char *src, int srclen,
                              WCHAR *dst, int dstlen )
{
    CPTABLEINFO local_info;
    const CPTABLEINFO *info = get_codepage_table( codepage );
    const unsigned char *str = (const unsigned char *)src;

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags & ~(MB_PRECOMPOSED | MB_COMPOSITE | MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if ((flags & MB_USEGLYPHCHARS) && info->MultiByteTable[256] == 256)
    {
        local_info = *info;
        local_info.MultiByteTable += 257;
        info = &local_info;
    }
    if ((flags & MB_ERR_INVALID_CHARS) && check_invalid_chars( info, str, srclen ))
    {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        return 0;
    }

    if (flags & MB_COMPOSITE) return mbstowcs_decompose( info, str, srclen, dst, dstlen );

    if (info->DBCSOffsets)
        return mbstowcs_dbcs( info, str, srclen, dst, dstlen );
    else
        return mbstowcs_sbcs( info, str, srclen, dst, dstlen );
}


static int wcstombs_cpsymbol( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                              const char *defchar, BOOL *used )
{
    int len, i;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!dstlen) return srclen;
    len = min( srclen, dstlen );
    for (i = 0; i < len; i++)
    {
        if (src[i] < 0x20) dst[i] = src[i];
        else if (src[i] >= 0xf020 && src[i] < 0xf100) dst[i] = src[i] - 0xf000;
        else
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    if (srclen > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return len;
}


static int wcstombs_utf7( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                          const char *defchar, BOOL *used )
{
    static const char directly_encodable[] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1f */
        1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3f */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5f */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                 /* 0x70 - 0x7a */
    };
#define ENCODABLE(ch) ((ch) <= 0x7a && directly_encodable[(ch)])

    static const char base64_encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const WCHAR *source_end = src + srclen;
    int pos = 0;

    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

#define OUTPUT(ch) \
    do { \
        if (dstlen > 0) \
        { \
            if (pos >= dstlen) goto overflow; \
            dst[pos] = (ch); \
        } \
        pos++; \
    } while (0)

    while (src < source_end)
    {
        if (*src == '+')
        {
            OUTPUT( '+' );
            OUTPUT( '-' );
            src++;
        }
        else if (ENCODABLE(*src))
        {
            OUTPUT( *src );
            src++;
        }
        else
        {
            unsigned int offset = 0, byte_pair = 0;

            OUTPUT( '+' );
            while (src < source_end && !ENCODABLE(*src))
            {
                byte_pair = (byte_pair << 16) | *src;
                offset += 16;
                while (offset >= 6)
                {
                    offset -= 6;
                    OUTPUT( base64_encoding_table[(byte_pair >> offset) & 0x3f] );
                }
                src++;
            }
            if (offset)
            {
                /* Windows won't create a padded base64 character if there's no room for the - sign
                 * as well ; this is probably a bug in Windows */
                if (dstlen > 0 && pos + 1 >= dstlen) goto overflow;
                byte_pair <<= (6 - offset);
                OUTPUT( base64_encoding_table[byte_pair & 0x3f] );
            }
            /* Windows always explicitly terminates the base64 sequence
               even though RFC 2152 (page 3, rule 2) does not require this */
            OUTPUT( '-' );
        }
    }
    return pos;

overflow:
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return 0;
#undef OUTPUT
#undef ENCODABLE
}


static int wcstombs_utf8( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                          const char *defchar, BOOL *used )
{
    DWORD reslen;
    NTSTATUS status;

    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags & ~(WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR | WC_ERR_INVALID_CHARS |
                  WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) dst = NULL;
    status = RtlUnicodeToUTF8N( dst, dstlen, &reslen, src, srclen * sizeof(WCHAR) );
    if (status == STATUS_SOME_NOT_MAPPED)
    {
        if (flags & WC_ERR_INVALID_CHARS)
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    else if (!set_ntstatus( status )) reslen = 0;
    return reslen;
}


static int wcstombs_sbcs( const CPTABLEINFO *info, const WCHAR *src, unsigned int srclen,
                          char *dst, unsigned int dstlen )
{
    const char *table = info->WideCharTable;
    int ret = srclen;

    if (!dstlen) return srclen;

    if (dstlen < srclen)
    {
        /* buffer too small: fill it up to dstlen and return error */
        srclen = dstlen;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }

    while (srclen >= 16)
    {
        dst[0]  = table[src[0]];
        dst[1]  = table[src[1]];
        dst[2]  = table[src[2]];
        dst[3]  = table[src[3]];
        dst[4]  = table[src[4]];
        dst[5]  = table[src[5]];
        dst[6]  = table[src[6]];
        dst[7]  = table[src[7]];
        dst[8]  = table[src[8]];
        dst[9]  = table[src[9]];
        dst[10] = table[src[10]];
        dst[11] = table[src[11]];
        dst[12] = table[src[12]];
        dst[13] = table[src[13]];
        dst[14] = table[src[14]];
        dst[15] = table[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle remaining characters */
    src += srclen;
    dst += srclen;
    switch(srclen)
    {
    case 15: dst[-15] = table[src[-15]];
    case 14: dst[-14] = table[src[-14]];
    case 13: dst[-13] = table[src[-13]];
    case 12: dst[-12] = table[src[-12]];
    case 11: dst[-11] = table[src[-11]];
    case 10: dst[-10] = table[src[-10]];
    case 9:  dst[-9]  = table[src[-9]];
    case 8:  dst[-8]  = table[src[-8]];
    case 7:  dst[-7]  = table[src[-7]];
    case 6:  dst[-6]  = table[src[-6]];
    case 5:  dst[-5]  = table[src[-5]];
    case 4:  dst[-4]  = table[src[-4]];
    case 3:  dst[-3]  = table[src[-3]];
    case 2:  dst[-2]  = table[src[-2]];
    case 1:  dst[-1]  = table[src[-1]];
    case 0: break;
    }
    return ret;
}


static int wcstombs_dbcs( const CPTABLEINFO *info, const WCHAR *src, unsigned int srclen,
                          char *dst, unsigned int dstlen )
{
    const USHORT *table = info->WideCharTable;
    int i;

    if (!dstlen)
    {
        for (i = 0; srclen; src++, srclen--, i++) if (table[*src] & 0xff00) i++;
        return i;
    }

    for (i = dstlen; srclen && i; i--, srclen--, src++)
    {
        if (table[*src] & 0xff00)
        {
            if (i == 1) break;  /* do not output a partial char */
            i--;
            *dst++ = table[*src] >> 8;
        }
        *dst++ = (char)table[*src];
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static inline int is_valid_sbcs_mapping( const CPTABLEINFO *info, DWORD flags,
                                         WCHAR wch, unsigned char ch )
{
    if ((flags & WC_NO_BEST_FIT_CHARS) || ch == info->DefaultChar)
        return (info->MultiByteTable[ch] == wch);
    return 1;
}


static inline int is_valid_dbcs_mapping( const CPTABLEINFO *info, DWORD flags,
                                         WCHAR wch, unsigned short ch )
{
    if ((flags & WC_NO_BEST_FIT_CHARS) || ch == info->DefaultChar)
        return info->DBCSOffsets[info->DBCSOffsets[ch >> 8] + (ch & 0xff)] == wch;
    return 1;
}


static int wcstombs_sbcs_slow( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen, const char *defchar, BOOL *used )
{
    const char *table = info->WideCharTable;
    const char def = defchar ? *defchar : (char)info->DefaultChar;
    int i;
    BOOL tmp;
    WCHAR wch, composed;

    if (!used) used = &tmp;  /* avoid checking on every char */
    *used = FALSE;

    if (!dstlen)
    {
        for (i = 0; srclen; i++, src++, srclen--)
        {
            wch = *src;
            if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = wine_compose( src )))
            {
                /* now check if we can use the composed char */
                if (is_valid_sbcs_mapping( info, flags, composed, table[composed] ))
                {
                    /* we have a good mapping, use it */
                    src++;
                    srclen--;
                    continue;
                }
                /* no mapping for the composed char, check the other flags */
                if (flags & WC_DEFAULTCHAR) /* use the default char instead */
                {
                    *used = TRUE;
                    src++;  /* skip the non-spacing char */
                    srclen--;
                    continue;
                }
                if (flags & WC_DISCARDNS) /* skip the second char of the composition */
                {
                    src++;
                    srclen--;
                }
                /* WC_SEPCHARS is the default */
            }
            if (!*used) *used = !is_valid_sbcs_mapping( info, flags, wch, table[wch] );
        }
        return i;
    }

    for (i = dstlen; srclen && i; dst++, i--, src++, srclen--)
    {
        wch = *src;
        if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = wine_compose( src )))
        {
            /* now check if we can use the composed char */
            *dst = table[composed];
            if (is_valid_sbcs_mapping( info, flags, composed, table[composed] ))
            {
                /* we have a good mapping, use it */
                src++;
                srclen--;
                continue;
            }
            /* no mapping for the composed char, check the other flags */
            if (flags & WC_DEFAULTCHAR) /* use the default char instead */
            {
                *dst = def;
                *used = TRUE;
                src++;  /* skip the non-spacing char */
                srclen--;
                continue;
            }
            if (flags & WC_DISCARDNS) /* skip the second char of the composition */
            {
                src++;
                srclen--;
            }
            /* WC_SEPCHARS is the default */
        }

        *dst = table[wch];
        if (!is_valid_sbcs_mapping( info, flags, wch, table[wch] ))
        {
            *dst = def;
            *used = TRUE;
        }
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int wcstombs_dbcs_slow( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen, const char *defchar, BOOL *used )
{
    const USHORT *table = info->WideCharTable;
    WCHAR wch, composed, defchar_value;
    unsigned short res;
    BOOL tmp;
    int i;

    if (!defchar[1]) defchar_value = (unsigned char)defchar[0];
    else defchar_value = ((unsigned char)defchar[0] << 8) | (unsigned char)defchar[1];

    if (!used) used = &tmp;  /* avoid checking on every char */
    *used = FALSE;

    if (!dstlen)
    {
        if (!defchar && !used && !(flags & WC_COMPOSITECHECK))
        {
            for (i = 0; srclen; srclen--, src++, i++) if (table[*src] & 0xff00) i++;
            return i;
        }
        for (i = 0; srclen; srclen--, src++, i++)
        {
            wch = *src;
            if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = wine_compose( src )))
            {
                /* now check if we can use the composed char */
                res = table[composed];
                if (is_valid_dbcs_mapping( info, flags, composed, res ))
                {
                    /* we have a good mapping for the composed char, use it */
                    if (res & 0xff00) i++;
                    src++;
                    srclen--;
                    continue;
                }
                /* no mapping for the composed char, check the other flags */
                if (flags & WC_DEFAULTCHAR) /* use the default char instead */
                {
                    if (defchar_value & 0xff00) i++;
                    *used = TRUE;
                    src++;  /* skip the non-spacing char */
                    srclen--;
                    continue;
                }
                if (flags & WC_DISCARDNS) /* skip the second char of the composition */
                {
                    src++;
                    srclen--;
                }
                /* WC_SEPCHARS is the default */
            }

            res = table[wch];
            if (!is_valid_dbcs_mapping( info, flags, wch, res ))
            {
                res = defchar_value;
                *used = TRUE;
            }
            if (res & 0xff00) i++;
        }
        return i;
    }


    for (i = dstlen; srclen && i; i--, srclen--, src++)
    {
        wch = *src;
        if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = wine_compose( src )))
        {
            /* now check if we can use the composed char */
            res = table[composed];

            if (is_valid_dbcs_mapping( info, flags, composed, res ))
            {
                /* we have a good mapping for the composed char, use it */
                src++;
                srclen--;
                goto output_char;
            }
            /* no mapping for the composed char, check the other flags */
            if (flags & WC_DEFAULTCHAR) /* use the default char instead */
            {
                res = defchar_value;
                *used = TRUE;
                src++;  /* skip the non-spacing char */
                srclen--;
                goto output_char;
            }
            if (flags & WC_DISCARDNS) /* skip the second char of the composition */
            {
                src++;
                srclen--;
            }
            /* WC_SEPCHARS is the default */
        }

        res = table[wch];
        if (!is_valid_dbcs_mapping( info, flags, wch, res ))
        {
            res = defchar_value;
            *used = TRUE;
        }

    output_char:
        if (res & 0xff00)
        {
            if (i == 1) break;  /* do not output a partial char */
            i--;
            *dst++ = res >> 8;
        }
        *dst++ = (char)res;
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int wcstombs_codepage( UINT codepage, DWORD flags, const WCHAR *src, int srclen,
                              char *dst, int dstlen, const char *defchar, BOOL *used )
{
    const CPTABLEINFO *info = get_codepage_table( codepage );

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags & ~(WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR | WC_ERR_INVALID_CHARS |
                  WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (flags || defchar || used)
    {
        if (!defchar) defchar = (const char *)&info->DefaultChar;
        if (info->DBCSOffsets)
            return wcstombs_dbcs_slow( info, flags, src, srclen, dst, dstlen, defchar, used );
        else
            return wcstombs_sbcs_slow( info, flags, src, srclen, dst, dstlen, defchar, used );
    }
    if (info->DBCSOffsets)
        return wcstombs_dbcs( info, src, srclen, dst, dstlen );
    else
        return wcstombs_sbcs( info, src, srclen, dst, dstlen );
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
 *	FoldStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FoldStringW( DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen )
{
    WCHAR *tmp;
    int ret;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (srclen == -1) srclen = lstrlenW(src) + 1;

    switch (flags)
    {
    case MAP_PRECOMPOSED:
        return NormalizeString( NormalizationC, src, srclen, dst, dstlen );
    case MAP_FOLDCZONE:
    case MAP_PRECOMPOSED | MAP_FOLDCZONE:
        return NormalizeString( NormalizationKC, src, srclen, dst, dstlen );
    case MAP_COMPOSITE:
        return NormalizeString( NormalizationD, src, srclen, dst, dstlen );
    case MAP_COMPOSITE | MAP_FOLDCZONE:
        return NormalizeString( NormalizationKD, src, srclen, dst, dstlen );
    case MAP_FOLDDIGITS:
        return fold_digits( src, srclen, dst, dstlen );
    case MAP_EXPAND_LIGATURES:
    case MAP_EXPAND_LIGATURES | MAP_FOLDCZONE:
        return expand_ligatures( src, srclen, dst, dstlen );
    case MAP_FOLDDIGITS | MAP_PRECOMPOSED:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) ))) break;
        if (!(ret = fold_digits( src, srclen, tmp, srclen ))) break;
        ret = NormalizeString( NormalizationC, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_FOLDCZONE:
    case MAP_FOLDDIGITS | MAP_PRECOMPOSED | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) ))) break;
        if (!(ret = fold_digits( src, srclen, tmp, srclen ))) break;
        ret = NormalizeString( NormalizationKC, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_COMPOSITE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) ))) break;
        if (!(ret = fold_digits( src, srclen, tmp, srclen ))) break;
        ret = NormalizeString( NormalizationD, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_COMPOSITE | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) ))) break;
        if (!(ret = fold_digits( src, srclen, tmp, srclen ))) break;
        ret = NormalizeString( NormalizationKD, tmp, srclen, dst, dstlen );
        break;
    case MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS:
    case MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) ))) break;
        if (!(ret = fold_digits( src, srclen, tmp, srclen ))) break;
        ret = expand_ligatures( tmp, srclen, dst, dstlen );
        break;
    default:
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!tmp)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    RtlFreeHeap( GetProcessHeap(), 0, tmp );
    return ret;
}


/******************************************************************************
 *	GetACP   (kernelbase.@)
 */
UINT WINAPI GetACP(void)
{
    return nls_info.AnsiTableInfo.CodePage;
}


/***********************************************************************
 *	GetCPInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCPInfo( UINT codepage, CPINFO *cpinfo )
{
    const CPTABLEINFO *table;

    if (!cpinfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (codepage)
    {
    case CP_UTF7:
    case CP_UTF8:
        cpinfo->DefaultChar[0] = 0x3f;
        cpinfo->DefaultChar[1] = 0;
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;
        cpinfo->MaxCharSize = (codepage == CP_UTF7) ? 5 : 4;
        break;
    default:
        if (!(table = get_codepage_table( codepage ))) return FALSE;
        cpinfo->MaxCharSize = table->MaximumCharacterSize;
        memcpy( cpinfo->DefaultChar, &table->DefaultChar, sizeof(cpinfo->DefaultChar) );
        memcpy( cpinfo->LeadByte, table->LeadByte, sizeof(cpinfo->LeadByte) );
        break;
    }
    return TRUE;
}


/***********************************************************************
 *	GetCPInfoExW   (kernelbase.@)
 */
BOOL WINAPI GetCPInfoExW( UINT codepage, DWORD flags, CPINFOEXW *cpinfo )
{
    const CPTABLEINFO *table;
    int min, max, pos;

    if (!cpinfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (codepage)
    {
    case CP_UTF7:
        cpinfo->DefaultChar[0] = 0x3f;
        cpinfo->DefaultChar[1] = 0;
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;
        cpinfo->MaxCharSize = 5;
        cpinfo->CodePage = CP_UTF7;
        cpinfo->UnicodeDefaultChar = 0x3f;
        break;
    case CP_UTF8:
        cpinfo->DefaultChar[0] = 0x3f;
        cpinfo->DefaultChar[1] = 0;
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;
        cpinfo->MaxCharSize = 4;
        cpinfo->CodePage = CP_UTF8;
        cpinfo->UnicodeDefaultChar = 0x3f;
        break;
    default:
        if (!(table = get_codepage_table( codepage ))) return FALSE;
        cpinfo->MaxCharSize = table->MaximumCharacterSize;
        memcpy( cpinfo->DefaultChar, &table->DefaultChar, sizeof(cpinfo->DefaultChar) );
        memcpy( cpinfo->LeadByte, table->LeadByte, sizeof(cpinfo->LeadByte) );
        cpinfo->CodePage = table->CodePage;
        cpinfo->UnicodeDefaultChar = table->UniDefaultChar;
        break;
    }

    min = 0;
    max = ARRAY_SIZE(codepage_names) - 1;
    cpinfo->CodePageName[0] = 0;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (codepage_names[pos].cp < cpinfo->CodePage) min = pos + 1;
        else if (codepage_names[pos].cp > cpinfo->CodePage) max = pos - 1;
        else
        {
            wcscpy( cpinfo->CodePageName, codepage_names[pos].name );
            break;
        }
    }
    return TRUE;
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
 *	IsDBCSLeadByte   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsDBCSLeadByte( BYTE testchar )
{
    return nls_info.AnsiTableInfo.DBCSCodePage && nls_info.AnsiTableInfo.DBCSOffsets[testchar];
}


/******************************************************************************
 *	IsDBCSLeadByteEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const CPTABLEINFO *table = get_codepage_table( codepage );
    return table && table->DBCSCodePage && table->DBCSOffsets[testchar];
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
 *	IsValidCodePage   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidCodePage( UINT codepage )
{
    switch (codepage)
    {
    case CP_ACP:
    case CP_OEMCP:
    case CP_MACCP:
    case CP_THREAD_ACP:
        return FALSE;
    case CP_UTF7:
    case CP_UTF8:
        return TRUE;
    default:
        return get_codepage_table( codepage ) != NULL;
    }
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
 *	MultiByteToWideChar   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH MultiByteToWideChar( UINT codepage, DWORD flags, const char *src, INT srclen,
                                                  WCHAR *dst, INT dstlen )
{
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (srclen < 0) srclen = strlen(src) + 1;

    switch (codepage)
    {
    case CP_SYMBOL:
        ret = mbstowcs_cpsymbol( flags, src, srclen, dst, dstlen );
        break;
    case CP_UTF7:
        ret = mbstowcs_utf7( flags, src, srclen, dst, dstlen );
        break;
    case CP_UTF8:
        ret = mbstowcs_utf8( flags, src, srclen, dst, dstlen );
        break;
    case CP_UNIXCP:
        codepage = __wine_get_unix_codepage();
        if (codepage == CP_UTF8)
        {
            ret = mbstowcs_utf8( flags, src, srclen, dst, dstlen );
#ifdef __APPLE__  /* work around broken Mac OS X filesystem that enforces decomposed Unicode */
            if (ret && dstlen) RtlNormalizeString( NormalizationC, dst, ret, dst, &ret );
#endif
            break;
        }
        /* fall through */
    default:
        ret = mbstowcs_codepage( codepage, flags, src, srclen, dst, dstlen );
        break;
    }
    TRACE( "cp %d %s -> %s, ret = %d\n", codepage, debugstr_an(src, srclen), debugstr_wn(dst, ret), ret );
    return ret;
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


/***********************************************************************
 *	WideCharToMultiByte   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH WideCharToMultiByte( UINT codepage, DWORD flags, LPCWSTR src, INT srclen,
                                                  LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
{
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = lstrlenW(src) + 1;

    switch (codepage)
    {
    case CP_SYMBOL:
        ret = wcstombs_cpsymbol( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UTF7:
        ret = wcstombs_utf7( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UTF8:
        ret = wcstombs_utf8( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UNIXCP:
        codepage = __wine_get_unix_codepage();
        if (codepage == CP_UTF8)
        {
            if (used) *used = FALSE;
            ret = wcstombs_utf8( flags, src, srclen, dst, dstlen, NULL, NULL );
            break;
        }
        /* fall through */
    default:
        ret = wcstombs_codepage( codepage, flags, src, srclen, dst, dstlen, defchar, used );
        break;
    }
    TRACE( "cp %d %s -> %s, ret = %d\n", codepage, debugstr_wn(src, srclen), debugstr_an(dst, ret), ret );
    return ret;
}
