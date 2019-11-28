/*
 * Locale functions
 *
 * Copyright 2004, 2019 Alexandre Julliard
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

#define NONAMELESSUNION
#include "config.h"
#include "wine/port.h"

#include <locale.h>
#include <langinfo.h>
#include <string.h>
#include <stdlib.h>

#ifdef __APPLE__
# include <CoreFoundation/CFLocale.h>
# include <CoreFoundation/CFString.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntdll_misc.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

/* NLS file format:
 *
 * header:
 *   WORD      offset to cp2uni table in words
 *   WORD      CodePage
 *   WORD      MaximumCharacterSize
 *   BYTE[2]   DefaultChar
 *   WORD      UniDefaultChar
 *   WORD      TransDefaultChar
 *   WORD      TransUniDefaultChar
 *   BYTE[12]  LeadByte
 * cp2uni table:
 *   WORD      offset to uni2cp table in words
 *   WORD[256] cp2uni table
 *   WORD      glyph table size
 *   WORD[glyph_table_size] glyph table
 *   WORD      number of lead byte ranges
 *   WORD[256] lead byte offsets in words
 *   WORD[leadbytes][256] cp2uni table for lead bytes
 * uni2cp table:
 *   WORD      0 / 4
 *   BYTE[65536] / WORD[65536]  uni2cp table
 */

enum nls_section_type
{
    NLS_SECTION_CASEMAP = 10,
    NLS_SECTION_CODEPAGE = 11,
    NLS_SECTION_NORMALIZE = 12
};

UINT NlsAnsiCodePage = 0;
BYTE NlsMbCodePageTag = 0;
BYTE NlsMbOemCodePageTag = 0;

LCID user_lcid = 0, system_lcid = 0;

static LANGID user_ui_language, system_ui_language;
static NLSTABLEINFO nls_info;
static HMODULE kernel32_handle;
static const union cptable *unix_table; /* NULL if UTF8 */

static NTSTATUS load_string( ULONG id, LANGID lang, WCHAR *buffer, ULONG len )
{
    const IMAGE_RESOURCE_DATA_ENTRY *data;
    LDR_RESOURCE_INFO info;
    NTSTATUS status;
    WCHAR *p;
    int i;

    info.Type = 6; /* RT_STRING */
    info.Name = (id >> 4) + 1;
    info.Language = lang;
    if ((status = LdrFindResource_U( kernel32_handle, &info, 3, &data ))) return status;
    p = (WCHAR *)((char *)kernel32_handle + data->OffsetToData);
    for (i = 0; i < (id & 0x0f); i++) p += *p + 1;
    if (*p >= len) return STATUS_BUFFER_TOO_SMALL;
    memcpy( buffer, p + 1, *p * sizeof(WCHAR) );
    buffer[*p] = 0;
    return STATUS_SUCCESS;
}


static NTSTATUS open_nls_data_file( ULONG type, ULONG id, HANDLE *file )
{
    static const WCHAR pathfmtW[] = {'\\','?','?','\\','%','s','%','s',0};
    static const WCHAR keyfmtW[] =
    {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','N','l','s','\\','%','s',0};
    static const WCHAR cpW[] = {'C','o','d','e','p','a','g','e',0};
    static const WCHAR normW[] = {'N','o','r','m','a','l','i','z','a','t','i','o','n',0};
    static const WCHAR langW[] = {'L','a','n','g','u','a','g','e',0};
    static const WCHAR cpfmtW[] = {'%','u',0};
    static const WCHAR normfmtW[] = {'%','x',0};
    static const WCHAR langfmtW[] = {'%','0','4','x',0};
    static const WCHAR winedatadirW[] = {'W','I','N','E','D','A','T','A','D','I','R',0};
    static const WCHAR winebuilddirW[] = {'W','I','N','E','B','U','I','L','D','D','I','R',0};
    static const WCHAR dataprefixW[] = {'\\',0};
    static const WCHAR buildprefixW[] = {'\\','l','o','a','d','e','r','\\',0};
    static const WCHAR cpdefaultW[] = {'c','_','%','0','3','d','.','n','l','s',0};
    static const WCHAR intlW[] = {'l','_','i','n','t','l','.','n','l','s',0};
    static const WCHAR normnfcW[] = {'n','o','r','m','n','f','c','.','n','l','s',0};
    static const WCHAR normnfdW[] = {'n','o','r','m','n','f','d','.','n','l','s',0};
    static const WCHAR normnfkcW[] = {'n','o','r','m','n','f','k','c','.','n','l','s',0};
    static const WCHAR normnfkdW[] = {'n','o','r','m','n','f','k','d','.','n','l','s',0};

    DWORD size;
    HANDLE handle;
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    WCHAR buffer[MAX_PATH], value[10];
    const WCHAR *name = NULL, *prefix = buildprefixW;
    KEY_VALUE_PARTIAL_INFORMATION *info;

    /* get filename from registry */

    switch (type)
    {
    case NLS_SECTION_CASEMAP:
        if (id) return STATUS_UNSUCCESSFUL;
        sprintfW( buffer, keyfmtW, langW );
        sprintfW( value, langfmtW, LANGIDFROMLCID(system_lcid) );
        break;
    case NLS_SECTION_CODEPAGE:
        sprintfW( buffer, keyfmtW, cpW );
        sprintfW( value, cpfmtW, id );
        break;
    case NLS_SECTION_NORMALIZE:
        sprintfW( buffer, keyfmtW, normW );
        sprintfW( value, normfmtW, id );
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    RtlInitUnicodeString( &nameW, buffer );
    RtlInitUnicodeString( &valueW, value );
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    if (!(status = NtOpenKey( &handle, KEY_READ, &attr )))
    {
        info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
        size = sizeof(buffer) - sizeof(WCHAR);
        if (!(status = NtQueryValueKey( handle, &valueW, KeyValuePartialInformation, info, size, &size )))
        {
            ((WCHAR *)info->Data)[info->DataLength / sizeof(WCHAR)] = 0;
            name = (WCHAR *)info->Data;
        }
        NtClose( handle );
    }

    if (!name)  /* otherwise some hardcoded defaults */
    {
        switch (type)
        {
        case NLS_SECTION_CASEMAP:
            name = intlW;
            break;
        case NLS_SECTION_CODEPAGE:
            sprintfW( buffer, cpdefaultW, id );
            name = buffer;
            break;
        case NLS_SECTION_NORMALIZE:
            switch (id)
            {
            case NormalizationC: name = normnfcW; break;
            case NormalizationD: name = normnfdW; break;
            case NormalizationKC: name = normnfkcW; break;
            case NormalizationKD: name = normnfkdW; break;
            }
            break;
        }
        if (!name) return status;
    }

    /* try to open file in system dir */

    valueW.MaximumLength = (strlenW(name) + strlenW(system_dir) + 5) * sizeof(WCHAR);
    if (!(valueW.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, valueW.MaximumLength )))
        return STATUS_NO_MEMORY;
    valueW.Length = sprintfW( valueW.Buffer, pathfmtW, system_dir, name ) * sizeof(WCHAR);
    InitializeObjectAttributes( &attr, &valueW, 0, 0, NULL );
    status = NtOpenFile( file, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
    if (!status) TRACE( "found %s\n", debugstr_w( valueW.Buffer ));
    RtlFreeUnicodeString( &valueW );
    if (status != STATUS_OBJECT_NAME_NOT_FOUND) return status;

    /* not found, try in build or data dir */

    RtlInitUnicodeString( &nameW, winebuilddirW );
    valueW.MaximumLength = 0;
    if (RtlQueryEnvironmentVariable_U( NULL, &nameW, &valueW ) != STATUS_BUFFER_TOO_SMALL)
    {
        RtlInitUnicodeString( &nameW, winedatadirW );
        prefix = dataprefixW;
        if (RtlQueryEnvironmentVariable_U( NULL, &nameW, &valueW ) != STATUS_BUFFER_TOO_SMALL)
            return status;
    }
    valueW.MaximumLength = valueW.Length + sizeof(buildprefixW) + strlenW(name) * sizeof(WCHAR);
    if (!(valueW.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, valueW.MaximumLength )))
        return STATUS_NO_MEMORY;
    if (!RtlQueryEnvironmentVariable_U( NULL, &nameW, &valueW ))
    {
        strcatW( valueW.Buffer, prefix );
        strcatW( valueW.Buffer, name );
        valueW.Length = strlenW(valueW.Buffer) * sizeof(WCHAR);
        InitializeObjectAttributes( &attr, &valueW, 0, 0, NULL );
        status = NtOpenFile( file, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
        if (!status) TRACE( "found %s\n", debugstr_w( valueW.Buffer ));
    }
    RtlFreeUnicodeString( &valueW );
    return status;
}


static USHORT *build_cptable( const union cptable *src, SIZE_T *size )
{
    unsigned int i, leadbytes = 0;
    USHORT *data, *ptr;

    *size = 13 + 1 + 256 + 1 + 1 + 1;
    if (src->info.char_size == 2)
    {
        for (i = leadbytes = 0; i < 256; i++) if (src->dbcs.cp2uni_leadbytes[i]) leadbytes++;
        *size += 256 + 256 * leadbytes;
        *size += 65536;
        *size *= sizeof(USHORT);
    }
    else
    {
        if (src->sbcs.cp2uni_glyphs != src->sbcs.cp2uni) *size += 256;
        *size *= sizeof(USHORT);
        *size += 65536;
    }
    if (!(data = RtlAllocateHeap( GetProcessHeap(), 0, *size ))) return NULL;
    ptr = data;
    ptr[0] = 0x0d;
    ptr[1] = src->info.codepage;
    ptr[2] = src->info.char_size;
    ptr[3] = (src->info.def_char & 0xff00 ?
              RtlUshortByteSwap( src->info.def_char ) : src->info.def_char);
    ptr[4] = src->info.def_unicode_char;

    if (src->info.char_size == 2)
    {
        USHORT off = src->dbcs.cp2uni_leadbytes[src->info.def_char >> 8] * 256;
        ptr[5] = src->dbcs.cp2uni[off + (src->info.def_char & 0xff)];

        ptr[6] = src->dbcs.uni2cp_low[src->dbcs.uni2cp_high[src->info.def_unicode_char >> 8]
                                      + (src->info.def_unicode_char & 0xff)];

        ptr += 7;
        memcpy( ptr, src->dbcs.lead_bytes, 12 );
        ptr += 6;
        *ptr++ = 256 + 3 + (leadbytes + 1) * 256;
        for (i = 0; i < 256; i++) *ptr++ = (src->dbcs.cp2uni_leadbytes[i] ? 0 : src->dbcs.cp2uni[i]);
        *ptr++ = 0;
        for (i = 0; i < 12; i++) if (!src->dbcs.lead_bytes[i]) break;
        *ptr++ = i / 2;
        for (i = 0; i < 256; i++) *ptr++ = 256 * src->dbcs.cp2uni_leadbytes[i];
        for (i = 0; i < leadbytes; i++, ptr += 256)
            memcpy( ptr, src->dbcs.cp2uni + 256 * (i + 1), 256 * sizeof(USHORT) );
        *ptr++ = 4;
        for (i = 0; i < 65536; i++)
            ptr[i] = src->dbcs.uni2cp_low[src->dbcs.uni2cp_high[i >> 8] + (i & 0xff)];
    }
    else
    {
        char *uni2cp;

        ptr[5] = src->sbcs.cp2uni[src->info.def_char];
        ptr[6] = src->sbcs.uni2cp_low[src->sbcs.uni2cp_high[src->info.def_unicode_char >> 8]
                                      + (src->info.def_unicode_char & 0xff)];

        ptr += 7;
        memset( ptr, 0, 12 );
        ptr += 6;
        *ptr++ = 256 + 3 + (src->sbcs.cp2uni_glyphs != src->sbcs.cp2uni ? 256 : 0);
        memcpy( ptr, src->sbcs.cp2uni, 256 * sizeof(USHORT) );
        ptr += 256;
        if (src->sbcs.cp2uni_glyphs != src->sbcs.cp2uni)
        {
            *ptr++ = 256;
            memcpy( ptr + 1, src->sbcs.cp2uni_glyphs, 256 );
            ptr += 256;
        }
        else *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        uni2cp = (char *)ptr;
        for (i = 0; i < 65536; i++)
            uni2cp[i] = src->sbcs.uni2cp_low[src->sbcs.uni2cp_high[i >> 8] + (i & 0xff)];
    }
    return data;
}


#if !defined(__APPLE__) && !defined(__ANDROID__)  /* these platforms always use UTF-8 */

/* charset to codepage map, sorted by name */
static const struct { const char *name; UINT cp; } charset_names[] =
{
    { "ANSIX341968", 20127 },
    { "BIG5", 950 },
    { "BIG5HKSCS", 950 },
    { "CP1250", 1250 },
    { "CP1251", 1251 },
    { "CP1252", 1252 },
    { "CP1253", 1253 },
    { "CP1254", 1254 },
    { "CP1255", 1255 },
    { "CP1256", 1256 },
    { "CP1257", 1257 },
    { "CP1258", 1258 },
    { "CP932", 932 },
    { "CP936", 936 },
    { "CP949", 949 },
    { "CP950", 950 },
    { "EUCJP", 20932 },
    { "EUCKR", 949 },
    { "GB18030", 936  /* 54936 */ },
    { "GB2312", 936 },
    { "GBK", 936 },
    { "IBM037", 37 },
    { "IBM1026", 1026 },
    { "IBM424", 424 },
    { "IBM437", 437 },
    { "IBM500", 500 },
    { "IBM850", 850 },
    { "IBM852", 852 },
    { "IBM855", 855 },
    { "IBM857", 857 },
    { "IBM860", 860 },
    { "IBM861", 861 },
    { "IBM862", 862 },
    { "IBM863", 863 },
    { "IBM864", 864 },
    { "IBM865", 865 },
    { "IBM866", 866 },
    { "IBM869", 869 },
    { "IBM874", 874 },
    { "IBM875", 875 },
    { "ISO88591", 28591 },
    { "ISO885910", 28600 },
    { "ISO885911", 28601 },
    { "ISO885913", 28603 },
    { "ISO885914", 28604 },
    { "ISO885915", 28605 },
    { "ISO885916", 28606 },
    { "ISO88592", 28592 },
    { "ISO88593", 28593 },
    { "ISO88594", 28594 },
    { "ISO88595", 28595 },
    { "ISO88596", 28596 },
    { "ISO88597", 28597 },
    { "ISO88598", 28598 },
    { "ISO88599", 28599 },
    { "KOI8R", 20866 },
    { "KOI8U", 21866 },
    { "TIS620", 28601 },
    { "UTF8", CP_UTF8 }
};

void init_unix_codepage(void)
{
    char charset_name[16];
    const char *name;
    size_t i, j;
    int min = 0, max = ARRAY_SIZE(charset_names) - 1;

    setlocale( LC_CTYPE, "" );
    if (!(name = nl_langinfo( CODESET ))) return;

    /* remove punctuation characters from charset name */
    for (i = j = 0; name[i] && j < sizeof(charset_name)-1; i++)
        if (isalnum((unsigned char)name[i])) charset_name[j++] = name[i];
    charset_name[j] = 0;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        int res = _strnicmp( charset_names[pos].name, charset_name, -1 );
        if (!res)
        {
            if (charset_names[pos].cp == CP_UTF8) return;
            unix_table = wine_cp_get_table( charset_names[pos].cp );
            return;
        }
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    ERR( "unrecognized charset '%s'\n", name );
}

#else  /* __APPLE__ || __ANDROID__ */

void init_unix_codepage(void) { }

#endif  /* __APPLE__ || __ANDROID__ */

/* Unix format is: lang[_country][.charset][@modifier]
 * Windows format is: lang[-script][-country][_modifier] */
static LCID unix_locale_to_lcid( const char *unix_name )
{
    static const WCHAR sepW[] = {'_','.','@',0};
    static const WCHAR posixW[] = {'P','O','S','I','X',0};
    static const WCHAR cW[] = {'C',0};
    static const WCHAR euroW[] = {'e','u','r','o',0};
    static const WCHAR latinW[] = {'l','a','t','i','n',0};
    static const WCHAR latnW[] = {'-','L','a','t','n',0};
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH], win_name[LOCALE_NAME_MAX_LENGTH];
    WCHAR *p, *country = NULL, *modifier = NULL;
    LCID lcid;

    if (!unix_name || !unix_name[0] || !strcmp( unix_name, "C" ))
    {
        unix_name = getenv( "LC_ALL" );
        if (!unix_name || !unix_name[0]) return 0;
    }

    if (ntdll_umbstowcs( 0, unix_name, strlen(unix_name) + 1, buffer, ARRAY_SIZE(buffer) ) < 0) return 0;

    if (!(p = strpbrkW( buffer, sepW )))
    {
        if (!strcmpW( buffer, posixW ) || !strcmpW( buffer, cW ))
            return MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT );
        strcpyW( win_name, buffer );
    }
    else
    {
        if (*p == '_')
        {
            *p++ = 0;
            country = p;
            p = strpbrkW( p, sepW + 1 );
        }
        if (p && *p == '.')
        {
            *p++ = 0;
            /* charset, ignore */
            p = strchrW( p, '@' );
        }
        if (p)
        {
            *p++ = 0;
            modifier = p;
        }
    }

    /* rebuild a Windows name */

    strcpyW( win_name, buffer );
    if (modifier)
    {
        if (!strcmpW( modifier, latinW )) strcatW( win_name, latnW );
        else if (!strcmpW( modifier, euroW )) {} /* ignore */
        else return 0;
    }
    if (country)
    {
        p = win_name + strlenW(win_name);
        *p++ = '-';
        strcpyW( p, country );
    }

    if (!RtlLocaleNameToLcid( win_name, &lcid, 0 )) return lcid;

    /* try neutral name */
    if (country)
    {
        p[-1] = 0;
        if (!RtlLocaleNameToLcid( win_name, &lcid, 2 ))
        {
            if (SUBLANGID(lcid) == SUBLANG_NEUTRAL)
                lcid = MAKELANGID( PRIMARYLANGID(lcid), SUBLANG_DEFAULT );
            return lcid;
        }
    }
    return 0;
}


/******************************************************************
 *		init_locale
 */
void init_locale( HMODULE module )
{
    LCID system_lcid, user_lcid;

    kernel32_handle = module;

    setlocale( LC_ALL, "" );

    system_lcid = unix_locale_to_lcid( setlocale( LC_CTYPE, NULL ));
    user_lcid = unix_locale_to_lcid( setlocale( LC_MESSAGES, NULL ));

#ifdef __APPLE__
    if (!system_lcid)
    {
        char buffer[LOCALE_NAME_MAX_LENGTH];

        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef lang = CFLocaleGetValue( locale, kCFLocaleLanguageCode );
        CFStringRef country = CFLocaleGetValue( locale, kCFLocaleCountryCode );
        CFStringRef locale_string;

        if (country)
            locale_string = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@_%@"), lang, country);
        else
            locale_string = CFStringCreateCopy(NULL, lang);

        CFStringGetCString(locale_string, buffer, sizeof(buffer), kCFStringEncodingUTF8);
        system_lcid = unix_locale_to_lcid( buffer );
        CFRelease(locale);
        CFRelease(locale_string);
    }
    if (!user_lcid)
    {
        /* Retrieve the preferred language as chosen in System Preferences. */
        char buffer[LOCALE_NAME_MAX_LENGTH];
        CFArrayRef preferred_langs = CFLocaleCopyPreferredLanguages();
        if (preferred_langs && CFArrayGetCount( preferred_langs ))
        {
            CFStringRef preferred_lang = CFArrayGetValueAtIndex( preferred_langs, 0 );
            CFDictionaryRef components = CFLocaleCreateComponentsFromLocaleIdentifier( NULL, preferred_lang );
            if (components)
            {
                CFStringRef lang = CFDictionaryGetValue( components, kCFLocaleLanguageCode );
                CFStringRef country = CFDictionaryGetValue( components, kCFLocaleCountryCode );
                CFLocaleRef locale = NULL;
                CFStringRef locale_string;

                if (!country)
                {
                    locale = CFLocaleCopyCurrent();
                    country = CFLocaleGetValue( locale, kCFLocaleCountryCode );
                }
                if (country)
                    locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@_%@"), lang, country );
                else
                    locale_string = CFStringCreateCopy( NULL, lang );
                CFStringGetCString( locale_string, buffer, sizeof(buffer), kCFStringEncodingUTF8 );
                CFRelease( locale_string );
                if (locale) CFRelease( locale );
                CFRelease( components );
                user_lcid = unix_locale_to_lcid( buffer );
            }
        }
        if (preferred_langs) CFRelease( preferred_langs );
    }
#endif

    if (!system_lcid) system_lcid = MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT );
    if (!user_lcid) user_lcid = system_lcid;

    NtSetDefaultUILanguage( LANGIDFROMLCID(user_lcid) );
    NtSetDefaultLocale( TRUE, user_lcid );
    NtSetDefaultLocale( FALSE, system_lcid );
    TRACE( "system=%04x user=%04x\n", system_lcid, user_lcid );

    setlocale( LC_NUMERIC, "C" );  /* FIXME: oleaut32 depends on this */
}


/******************************************************************
 *      ntdll_umbstowcs
 */
int ntdll_umbstowcs( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
#ifdef __APPLE__
    /* work around broken Mac OS X filesystem that enforces decomposed Unicode */
    flags |= MB_COMPOSITE;
#endif
    return unix_table ?
        wine_cp_mbstowcs( unix_table, flags, src, srclen, dst, dstlen ) :
        wine_utf8_mbstowcs( flags, src, srclen, dst, dstlen );
}


/******************************************************************
 *      ntdll_wcstoumbs
 */
int ntdll_wcstoumbs( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                     const char *defchar, int *used )
{
    if (unix_table)
        return wine_cp_wcstombs( unix_table, flags, src, srclen, dst, dstlen, defchar, used );
    if (used) *used = 0;  /* all chars are valid for UTF-8 */
    return wine_utf8_wcstombs( flags, src, srclen, dst, dstlen );
}


/******************************************************************
 *      __wine_get_unix_codepage   (NTDLL.@)
 */
UINT CDECL __wine_get_unix_codepage(void)
{
    if (!unix_table) return CP_UTF8;
    return unix_table->info.codepage;
}


/**********************************************************************
 *      NtQueryDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultLocale( BOOLEAN user, LCID *lcid )
{
    *lcid = user ? user_lcid : system_lcid;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtSetDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultLocale( BOOLEAN user, LCID lcid )
{
    if (user) user_lcid = lcid;
    else
    {
        system_lcid = lcid;
        system_ui_language = LANGIDFROMLCID(lcid); /* there is no separate call to set it */
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtQueryDefaultUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultUILanguage( LANGID *lang )
{
    *lang = user_ui_language;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtSetDefaultUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultUILanguage( LANGID lang )
{
    user_ui_language = lang;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtQueryInstallUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInstallUILanguage( LANGID *lang )
{
    *lang = system_ui_language;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      NtGetNlsSectionPtr   (NTDLL.@)
 */
NTSTATUS WINAPI NtGetNlsSectionPtr( ULONG type, ULONG id, void *unknown, void **ptr, SIZE_T *size )
{
    FILE_END_OF_FILE_INFORMATION info;
    IO_STATUS_BLOCK io;
    HANDLE file;
    NTSTATUS status;

    if ((status = open_nls_data_file( type, id, &file )))
    {
        /* FIXME: special case for codepage table, generate it from the libwine data */
        if (type == NLS_SECTION_CODEPAGE)
        {
            const union cptable *table = wine_cp_get_table( id );
            if (table && (*ptr = build_cptable( table, size ))) return STATUS_SUCCESS;
        }
        return status;
    }

    if ((status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileEndOfFileInformation )))
        goto done;
    /* FIXME: return a heap block instead of a file mapping for now */
    if (!(*ptr = RtlAllocateHeap( GetProcessHeap(), 0, info.EndOfFile.QuadPart )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }
    status = NtReadFile( file, 0, NULL, NULL, &io, *ptr, info.EndOfFile.QuadPart, NULL, NULL );
    if (!status && io.Information != info.EndOfFile.QuadPart) status = STATUS_INVALID_FILE_FOR_SECTION;
    if (!status) *size = io.Information;
    else RtlFreeHeap( GetProcessHeap(), 0, *ptr );
done:
    NtClose( file );
    return status;
}


/******************************************************************
 *      RtlInitCodePageTable   (NTDLL.@)
 */
void WINAPI RtlInitCodePageTable( USHORT *ptr, CPTABLEINFO *info )
{
    USHORT hdr_size = ptr[0];

    info->CodePage             = ptr[1];
    info->MaximumCharacterSize = ptr[2];
    info->DefaultChar          = ptr[3];
    info->UniDefaultChar       = ptr[4];
    info->TransDefaultChar     = ptr[5];
    info->TransUniDefaultChar  = ptr[6];
    memcpy( info->LeadByte, ptr + 7, sizeof(info->LeadByte) );
    ptr += hdr_size;

    info->WideCharTable = ptr + ptr[0] + 1;
    info->MultiByteTable = ++ptr;
    ptr += 256;
    if (*ptr++) ptr += 256;  /* glyph table */
    info->DBCSRanges = ptr;
    if (*ptr)  /* dbcs ranges */
    {
        info->DBCSCodePage = 1;
        info->DBCSOffsets  = ptr + 1;
    }
    else
    {
        info->DBCSCodePage = 0;
        info->DBCSOffsets  = NULL;
    }
}


/**************************************************************************
 *      RtlInitNlsTables   (NTDLL.@)
 */
void WINAPI RtlInitNlsTables( USHORT *ansi, USHORT *oem, USHORT *casetable, NLSTABLEINFO *info )
{
    RtlInitCodePageTable( ansi, &info->AnsiTableInfo );
    RtlInitCodePageTable( oem, &info->OemTableInfo );
    info->UpperCaseTable = casetable + 2;
    info->LowerCaseTable = casetable + casetable[1] + 2;
}


/**************************************************************************
 *      RtlResetRtlTranslations   (NTDLL.@)
 */
void WINAPI RtlResetRtlTranslations( const NLSTABLEINFO *info )
{
    NlsAnsiCodePage     = info->AnsiTableInfo.CodePage;
    NlsMbCodePageTag    = info->AnsiTableInfo.DBCSCodePage;
    NlsMbOemCodePageTag = info->OemTableInfo.DBCSCodePage;
    nls_info = *info;
}


/******************************************************************
 *      RtlLocaleNameToLcid   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLocaleNameToLcid( const WCHAR *name, LCID *lcid, ULONG flags )
{
    /* locale name format is: lang[-script][-country][_modifier] */

    static const WCHAR sepW[] = {'-','_',0};

    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    LDR_RESOURCE_INFO info;
    WCHAR buf[LOCALE_NAME_MAX_LENGTH];
    WCHAR lang[LOCALE_NAME_MAX_LENGTH]; /* language ("en") (note: buffer contains the other strings too) */
    WCHAR *country = NULL; /* country ("US") */
    WCHAR *script = NULL; /* script ("Latn") */
    WCHAR *p;
    int i;

    if (!name) return STATUS_INVALID_PARAMETER_1;

    if (!name[0])
    {
        *lcid = LANG_INVARIANT;
        goto found;
    }
    if (strlenW( name ) >= LOCALE_NAME_MAX_LENGTH) return STATUS_INVALID_PARAMETER_1;
    strcpyW( lang, name );

    if ((p = strpbrkW( lang, sepW )) && *p == '-')
    {
        *p++ = 0;
        country = p;
        if ((p = strpbrkW( p, sepW )) && *p == '-')
        {
            *p++ = 0;
            script = country;
            country = p;
            p = strpbrkW( p, sepW );
        }
        if (p) *p = 0;  /* FIXME: modifier is ignored */
        /* second value can be script or country, check length to resolve the ambiguity */
        if (!script && strlenW( country ) == 4)
        {
            script = country;
            country = NULL;
        }
    }

    info.Type = 6; /* RT_STRING */
    info.Name = (LOCALE_SNAME >> 4) + 1;
    if (LdrFindResourceDirectory_U( kernel32_handle, &info, 2, &resdir ))
        return STATUS_INVALID_PARAMETER_1;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
    {
        LANGID id = et[i].u.Id;

        if (PRIMARYLANGID(id) == LANG_NEUTRAL) continue;

        if (!load_string( LOCALE_SNAME, id, buf, ARRAY_SIZE(buf) ) && !strcmpiW( name, buf ))
        {
            *lcid = MAKELCID( id, SORT_DEFAULT );  /* FIXME: handle sort order */
            goto found;
        }

        if (load_string( LOCALE_SISO639LANGNAME, id, buf, ARRAY_SIZE(buf) ) || strcmpiW( lang, buf ))
            continue;

        if (script)
        {
            unsigned int len = strlenW( script );
            if (load_string( LOCALE_SSCRIPTS, id, buf, ARRAY_SIZE(buf) )) continue;
            p = buf;
            while (*p)
            {
                if (!strncmpiW( p, script, len ) && (!p[len] || p[len] == ';')) break;
                if (!(p = strchrW( p, ';'))) break;
                p++;
            }
            if (!p || !*p) continue;
        }

        if (!country && (flags & 2))
        {
            if (!script) id = MAKELANGID( PRIMARYLANGID(id), LANG_NEUTRAL );
            switch (id)
            {
            case MAKELANGID( LANG_CHINESE, SUBLANG_NEUTRAL ):
            case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE ):
                *lcid = MAKELCID( 0x7804, SORT_DEFAULT );
                break;
            case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL ):
            case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_MACAU ):
            case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG ):
                *lcid = MAKELCID( 0x7c04, SORT_DEFAULT );
                break;
            default:
                *lcid = MAKELANGID( PRIMARYLANGID(id), SUBLANG_NEUTRAL );
                break;
            }
            goto found;
        }
    }
    return STATUS_INVALID_PARAMETER_1;

found:
    TRACE( "%s -> %04x\n", debugstr_w(name), *lcid );
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlIsNormalizedString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlIsNormalizedString( ULONG form, const WCHAR *str, INT len, BOOLEAN *res )
{
    FIXME( "%x %p %d\n", form, str, len );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *      RtlNormalizeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlNormalizeString( ULONG form, const WCHAR *src, INT src_len, WCHAR *dst, INT *dst_len )
{
    int flags = 0, compose = 0;
    unsigned int res, buf_len;
    WCHAR *buf = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%x %s %d %p %d\n", form, debugstr_wn(src, src_len), src_len, dst, *dst_len );

    if (src_len == -1) src_len = strlenW(src) + 1;

    if (form == NormalizationKC || form == NormalizationKD) flags |= WINE_DECOMPOSE_COMPAT;
    if (form == NormalizationC || form == NormalizationKC) compose = 1;
    if (compose || *dst_len) flags |= WINE_DECOMPOSE_REORDER;

    if (!compose && *dst_len)
    {
        res = wine_decompose_string( flags, src, src_len, dst, *dst_len );
        if (!res)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto done;
        }
        buf = dst;
    }
    else
    {
        buf_len = src_len * 4;
        do
        {
            WCHAR *old_buf = buf;

            if (!buf) buf = RtlAllocateHeap( GetProcessHeap(), 0, buf_len );
            else buf = RtlReAllocateHeap( GetProcessHeap(), 0, buf, buf_len );
            if (!buf)
            {
                RtlFreeHeap( GetProcessHeap(), 0, old_buf );
                return STATUS_NO_MEMORY;
            }
            res = wine_decompose_string( flags, src, src_len, buf, buf_len );
            buf_len *= 2;
        } while (!res);
    }

    if (compose)
    {
        res = wine_compose_string( buf, res );
        if (*dst_len >= res) memcpy( dst, buf, res * sizeof(WCHAR) );
    }

done:
    if (buf != dst) RtlFreeHeap( GetProcessHeap(), 0, buf );
    *dst_len = res;
    return status;
}
