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

extern WCHAR wine_compose( const WCHAR *str ) DECLSPEC_HIDDEN;
extern const unsigned short combining_class_table[] DECLSPEC_HIDDEN;
extern const unsigned short nfd_table[] DECLSPEC_HIDDEN;
extern const unsigned short nfkd_table[] DECLSPEC_HIDDEN;

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


static DWORD mbtowc_size( const CPTABLEINFO *info, LPCSTR str, UINT len )
{
    DWORD res;

    if (!info->DBCSCodePage) return len;

    for (res = 0; len; len--, str++, res++)
    {
        if (info->DBCSOffsets[(unsigned char)*str] && len > 1)
        {
            str++;
            len--;
        }
    }
    return res;
}


static DWORD wctomb_size( const CPTABLEINFO *info, LPCWSTR str, UINT len )
{
    if (info->DBCSCodePage)
    {
        WCHAR *uni2cp = info->WideCharTable;
        DWORD res;

        for (res = 0; len; len--, str++, res++)
            if (uni2cp[*str] & 0xff00) res++;
        return res;
    }
    else return len;
}


static WCHAR casemap( USHORT *table, WCHAR ch )
{
    return ch + table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}


static WCHAR casemap_ascii( WCHAR ch )
{
    if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
    return ch;
}


static const WCHAR *get_decomposition( const unsigned short *table, WCHAR ch, unsigned int *len )
{
    unsigned short offset = table[table[ch >> 8] + ((ch >> 4) & 0xf)] + (ch & 0xf);
    unsigned short start = table[offset];
    unsigned short end = table[offset + 1];

    if ((*len = end - start)) return table + start;
    *len = 1;
    return NULL;
}


static BYTE get_combining_class( WCHAR c )
{
    return combining_class_table[combining_class_table[combining_class_table[c >> 8] + ((c >> 4) & 0xf)] + (c & 0xf)];
}


static BOOL is_starter( WCHAR c )
{
    return !get_combining_class( c );
}


static BOOL reorderable_pair( WCHAR c1, WCHAR c2 )
{
    BYTE ccc1, ccc2;

    /* reorderable if ccc1 > ccc2 > 0 */
    ccc1 = get_combining_class( c1 );
    if (ccc1 < 2) return FALSE;
    ccc2 = get_combining_class( c2 );
    return ccc2 && (ccc1 > ccc2);
}


static void canonical_order_substring( WCHAR *str, unsigned int len )
{
    unsigned int i;
    BOOL swapped;

    do
    {
        swapped = FALSE;
        for (i = 0; i < len - 1; i++)
        {
            if (reorderable_pair( str[i], str[i + 1] ))
            {
                WCHAR tmp = str[i];
                str[i] = str[i + 1];
                str[i + 1] = tmp;
                swapped = TRUE;
            }
        }
    } while (swapped);
}


/****************************************************************************
 *             canonical_order_string
 *
 * Reorder the string into canonical order - D108/D109.
 *
 * Starters (chars with combining class == 0) don't move, so look for continuous
 * substrings of non-starters and only reorder those.
 */
static void canonical_order_string( WCHAR *str, unsigned int len )
{
    unsigned int i, next = 0;

    for (i = 1; i <= len; i++)
    {
        if (i == len || is_starter( str[i] ))
        {
            if (i > next + 1) /* at least two successive non-starters */
                canonical_order_substring( str + next, i - next );
            next = i + 1;
        }
    }
}


static NTSTATUS decompose_string( int compat, const WCHAR *src, int src_len, WCHAR *dst, int *dst_len )
{
    const unsigned short *table = compat ? nfkd_table : nfd_table;
    int src_pos, dst_pos = 0;
    unsigned int decomp_len;
    const WCHAR *decomp;

    for (src_pos = 0; src_pos < src_len; src_pos++)
    {
        if (dst_pos == *dst_len) break;
        if ((decomp = get_decomposition( table, src[src_pos], &decomp_len )))
        {
            if (dst_pos + decomp_len > *dst_len) break;
            memcpy( dst + dst_pos, decomp, decomp_len * sizeof(WCHAR) );
        }
        else dst[dst_pos] = src[src_pos];
        dst_pos += decomp_len;
    }
    if (src_pos < src_len)
    {
        *dst_len += (src_len - src_pos) * (compat ? 18 : 3);
        return STATUS_BUFFER_TOO_SMALL;
    }
    canonical_order_string( dst, dst_pos );
    *dst_len = dst_pos;
    return STATUS_SUCCESS;
}


static BOOL is_blocked( WCHAR *starter, WCHAR *ptr )
{
    if (ptr == starter + 1) return FALSE;
    /* Because the string is already canonically ordered, the chars are blocked
       only if the previous char's combining class is equal to the test char. */
    if (get_combining_class( *(ptr - 1) ) == get_combining_class( *ptr )) return TRUE;
    return FALSE;
}


static unsigned int compose_string( WCHAR *str, unsigned int len )
{
    unsigned int i, last_starter = len;
    WCHAR pair[2], comp;

    for (i = 0; i < len; i++)
    {
        pair[1] = str[i];
        if (last_starter == len || is_blocked( str + last_starter, str + i ) || !(comp = wine_compose( pair )))
        {
            if (is_starter( str[i] ))
            {
                last_starter = i;
                pair[0] = str[i];
            }
            continue;
        }
        str[last_starter] = pair[0] = comp;
        len--;
        memmove( str + i, str + i + 1, (len - i) * sizeof(WCHAR) );
        i = last_starter;
    }
    return len;
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

    if (!name || !*name)  /* otherwise some hardcoded defaults */
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
            memcpy( ptr, src->sbcs.cp2uni_glyphs, 256 * sizeof(USHORT) );
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
    DWORD reslen;
    NTSTATUS status;

    if (unix_table) return wine_cp_mbstowcs( unix_table, flags, src, srclen, dst, dstlen );

    if (!dstlen) dst = NULL;
    status = RtlUTF8ToUnicodeN( dst, dstlen * sizeof(WCHAR), &reslen, src, srclen );
    if (status && status != STATUS_SOME_NOT_MAPPED) return -1;
    reslen /= sizeof(WCHAR);
#ifdef __APPLE__  /* work around broken Mac OS X filesystem that enforces decomposed Unicode */
    if (reslen && dst) reslen = compose_string( dst, reslen );
#endif
    return reslen;
}


/******************************************************************
 *      ntdll_wcstoumbs
 */
int ntdll_wcstoumbs( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                     const char *defchar, int *used )
{
    DWORD reslen;
    NTSTATUS status;

    if (unix_table) return wine_cp_wcstombs( unix_table, flags, src, srclen, dst, dstlen, defchar, used );

    if (used) *used = 0;  /* all chars are valid for UTF-8 */
    if (!dstlen) dst = NULL;
    status = RtlUnicodeToUTF8N( dst, dstlen, &reslen, src, srclen * sizeof(WCHAR) );
    if (status && status != STATUS_SOME_NOT_MAPPED) return -1;
    return reslen;
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


/**************************************************************************
 *      RtlAnsiCharToUnicodeChar   (NTDLL.@)
 */
WCHAR WINAPI RtlAnsiCharToUnicodeChar( char **ansi )
{
    if (nls_info.AnsiTableInfo.DBCSOffsets)
    {
        USHORT off = nls_info.AnsiTableInfo.DBCSOffsets[(unsigned char)**ansi];
        if (off)
        {
            (*ansi)++;
            return nls_info.AnsiTableInfo.DBCSOffsets[off + (unsigned char)*(*ansi)++];
        }
    }
    return nls_info.AnsiTableInfo.MultiByteTable[(unsigned char)*(*ansi)++];
}


/******************************************************************************
 *	RtlCompareUnicodeStrings   (NTDLL.@)
 */
LONG WINAPI RtlCompareUnicodeStrings( const WCHAR *s1, SIZE_T len1, const WCHAR *s2, SIZE_T len2,
                                      BOOLEAN case_insensitive )
{
    LONG ret = 0;
    SIZE_T len = min( len1, len2 );

    if (case_insensitive)
    {
        if (nls_info.UpperCaseTable)
        {
            while (!ret && len--) ret = casemap( nls_info.UpperCaseTable, *s1++ ) -
                                        casemap( nls_info.UpperCaseTable, *s2++ );
        }
        else  /* locale not setup yet */
        {
            while (!ret && len--) ret = casemap_ascii( *s1++ ) - casemap_ascii( *s2++ );
        }
    }
    else
    {
        while (!ret && len--) ret = *s1++ - *s2++;
    }
    if (!ret) ret = len1 - len2;
    return ret;
}


/**************************************************************************
 *	RtlPrefixUnicodeString   (NTDLL.@)
 */
BOOLEAN WINAPI RtlPrefixUnicodeString( const UNICODE_STRING *s1, const UNICODE_STRING *s2,
                                       BOOLEAN ignore_case )
{
    unsigned int i;

    if (s1->Length > s2->Length) return FALSE;
    if (ignore_case)
    {
        for (i = 0; i < s1->Length / sizeof(WCHAR); i++)
            if (casemap( nls_info.UpperCaseTable, s1->Buffer[i] ) !=
                casemap( nls_info.UpperCaseTable, s2->Buffer[i] )) return FALSE;
    }
    else
    {
        for (i = 0; i < s1->Length / sizeof(WCHAR); i++)
            if (s1->Buffer[i] != s2->Buffer[i]) return FALSE;
    }
    return TRUE;
}


/**************************************************************************
 *	RtlCustomCPToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlCustomCPToUnicodeN( CPTABLEINFO *info, WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                       const char *src, DWORD srclen )
{
    DWORD i, ret;

    dstlen /= sizeof(WCHAR);
    if (info->DBCSOffsets)
    {
        for (i = dstlen; srclen && i; i--, srclen--, src++, dst++)
        {
            USHORT off = info->DBCSOffsets[(unsigned char)*src];
            if (off && srclen > 1)
            {
                src++;
                srclen--;
                *dst = info->DBCSOffsets[off + (unsigned char)*src];
            }
            else *dst = info->MultiByteTable[(unsigned char)*src];
        }
        ret = dstlen - i;
    }
    else
    {
        ret = min( srclen, dstlen );
        for (i = 0; i < ret; i++) dst[i] = info->MultiByteTable[(unsigned char)src[i]];
    }
    if (reslen) *reslen = ret * sizeof(WCHAR);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUnicodeToCustomCPN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToCustomCPN( CPTABLEINFO *info, char *dst, DWORD dstlen, DWORD *reslen,
                                       const WCHAR *src, DWORD srclen )
{
    DWORD i, ret;

    srclen /= sizeof(WCHAR);
    if (info->DBCSCodePage)
    {
        WCHAR *uni2cp = info->WideCharTable;

        for (i = dstlen; srclen && i; i--, srclen--, src++)
        {
            if (uni2cp[*src] & 0xff00)
            {
                if (i == 1) break;  /* do not output a partial char */
                i--;
                *dst++ = uni2cp[*src] >> 8;
            }
            *dst++ = (char)uni2cp[*src];
        }
        ret = dstlen - i;
    }
    else
    {
        char *uni2cp = info->WideCharTable;
        ret = min( srclen, dstlen );
        for (i = 0; i < ret; i++) dst[i] = uni2cp[src[i]];
    }
    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlMultiByteToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlMultiByteToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                        const char *src, DWORD srclen )
{
    if (nls_info.AnsiTableInfo.WideCharTable)
        return RtlCustomCPToUnicodeN( &nls_info.AnsiTableInfo, dst, dstlen, reslen, src, srclen );

    /* locale not setup yet */
    dstlen = min( srclen, dstlen / sizeof(WCHAR) );
    if (reslen) *reslen = dstlen * sizeof(WCHAR);
    while (dstlen--) *dst++ = *src++ & 0x7f;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlMultiByteToUnicodeSize   (NTDLL.@)
 */
NTSTATUS WINAPI RtlMultiByteToUnicodeSize( DWORD *size, const char *str, DWORD len )
{
    *size = mbtowc_size( &nls_info.AnsiTableInfo, str, len ) * sizeof(WCHAR);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlOemToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlOemToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                  const char *src, DWORD srclen )
{
    return RtlCustomCPToUnicodeN( &nls_info.OemTableInfo, dst, dstlen, reslen, src, srclen );
}


/**************************************************************************
 *      RtlOemStringToUnicodeSize   (NTDLL.@)
 *      RtlxOemStringToUnicodeSize  (NTDLL.@)
 */
DWORD WINAPI RtlOemStringToUnicodeSize( const STRING *str )
{
    return (mbtowc_size( &nls_info.OemTableInfo, str->Buffer, str->Length ) + 1) * sizeof(WCHAR);
}


/**************************************************************************
 *      RtlUnicodeStringToOemSize   (NTDLL.@)
 *      RtlxUnicodeStringToOemSize  (NTDLL.@)
 */
DWORD WINAPI RtlUnicodeStringToOemSize( const UNICODE_STRING *str )
{
    return wctomb_size( &nls_info.OemTableInfo, str->Buffer, str->Length / sizeof(WCHAR) ) + 1;
}


/**************************************************************************
 *	RtlUnicodeToMultiByteN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteN( char *dst, DWORD dstlen, DWORD *reslen,
                                        const WCHAR *src, DWORD srclen )
{
    if (nls_info.AnsiTableInfo.WideCharTable)
        return RtlUnicodeToCustomCPN( &nls_info.AnsiTableInfo, dst, dstlen, reslen, src, srclen );

    /* locale not setup yet */
    dstlen = min( srclen / sizeof(WCHAR), dstlen );
    if (reslen) *reslen = dstlen;
    while (dstlen--)
    {
        WCHAR ch = *src++;
        if (ch > 0x7f) ch = '?';
        *dst++ = ch;
    }
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlUnicodeToMultiByteSize   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteSize( DWORD *size, const WCHAR *str, DWORD len )
{
    *size = wctomb_size( &nls_info.AnsiTableInfo, str, len / sizeof(WCHAR) );
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUnicodeToOemN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToOemN( char *dst, DWORD dstlen, DWORD *reslen,
                                  const WCHAR *src, DWORD srclen )
{
    return RtlUnicodeToCustomCPN( &nls_info.OemTableInfo, dst, dstlen, reslen, src, srclen );
}


/**************************************************************************
 *	RtlDowncaseUnicodeChar   (NTDLL.@)
 */
WCHAR WINAPI RtlDowncaseUnicodeChar( WCHAR wch )
{
    return casemap( nls_info.LowerCaseTable, wch );
}


/**************************************************************************
 *	RtlDowncaseUnicodeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlDowncaseUnicodeString( UNICODE_STRING *dest, const UNICODE_STRING *src,
                                          BOOLEAN alloc )
{
    DWORD i, len = src->Length;

    if (alloc)
    {
        dest->MaximumLength = len;
        if (!(dest->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len ))) return STATUS_NO_MEMORY;
    }
    else if (len > dest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < len / sizeof(WCHAR); i++)
        dest->Buffer[i] = casemap( nls_info.LowerCaseTable, src->Buffer[i] );
    dest->Length = len;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUpcaseUnicodeChar   (NTDLL.@)
 */
WCHAR WINAPI RtlUpcaseUnicodeChar( WCHAR wch )
{
    return casemap( nls_info.UpperCaseTable, wch );
}


/**************************************************************************
 *	RtlUpcaseUnicodeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeString( UNICODE_STRING *dest, const UNICODE_STRING *src,
                                        BOOLEAN alloc )
{
    DWORD i, len = src->Length;

    if (alloc)
    {
        dest->MaximumLength = len;
        if (!(dest->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len ))) return STATUS_NO_MEMORY;
    }
    else if (len > dest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < len / sizeof(WCHAR); i++)
        dest->Buffer[i] = casemap( nls_info.UpperCaseTable, src->Buffer[i] );
    dest->Length = len;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUpcaseUnicodeToCustomCPN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToCustomCPN( CPTABLEINFO *info, char *dst, DWORD dstlen, DWORD *reslen,
                                             const WCHAR *src, DWORD srclen )
{
    DWORD i, ret;

    srclen /= sizeof(WCHAR);
    if (info->DBCSCodePage)
    {
        WCHAR *uni2cp = info->WideCharTable;

        for (i = dstlen; srclen && i; i--, srclen--, src++)
        {
            WCHAR ch = casemap( nls_info.UpperCaseTable, *src );
            if (uni2cp[ch] & 0xff00)
            {
                if (i == 1) break;  /* do not output a partial char */
                i--;
                *dst++ = uni2cp[ch] >> 8;
            }
            *dst++ = (char)uni2cp[ch];
        }
        ret = dstlen - i;
    }
    else
    {
        char *uni2cp = info->WideCharTable;
        ret = min( srclen, dstlen );
        for (i = 0; i < ret; i++) dst[i] = uni2cp[casemap( nls_info.UpperCaseTable, src[i] )];
    }
    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUpcaseUnicodeToMultiByteN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToMultiByteN( char *dst, DWORD dstlen, DWORD *reslen,
                                              const WCHAR *src, DWORD srclen )
{
    return RtlUpcaseUnicodeToCustomCPN( &nls_info.AnsiTableInfo, dst, dstlen, reslen, src, srclen );
}


/**************************************************************************
 *	RtlUpcaseUnicodeToOemN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToOemN( char *dst, DWORD dstlen, DWORD *reslen,
                                        const WCHAR *src, DWORD srclen )
{
    if (nls_info.OemTableInfo.WideCharTable)
        return RtlUpcaseUnicodeToCustomCPN( &nls_info.OemTableInfo, dst, dstlen, reslen, src, srclen );

    /* locale not setup yet */
    dstlen = min( srclen / sizeof(WCHAR), dstlen );
    if (reslen) *reslen = dstlen;
    while (dstlen--)
    {
        WCHAR ch = *src++;
        if (ch > 0x7f) ch = '?';
        else ch = casemap_ascii( ch );
        *dst++ = ch;
    }
    return STATUS_SUCCESS;
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


/* helper for the various utf8 mbstowcs functions */
static unsigned int decode_utf8_char( unsigned char ch, const char **str, const char *strend )
{
    /* number of following bytes in sequence based on first byte value (for bytes above 0x7f) */
    static const char utf8_length[128] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xa0-0xaf */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xb0-0xbf */
        0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xc0-0xcf */
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xd0-0xdf */
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xe0-0xef */
        3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0  /* 0xf0-0xff */
    };

    /* first byte mask depending on UTF-8 sequence length */
    static const unsigned char utf8_mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };

    unsigned int len = utf8_length[ch - 0x80];
    unsigned int res = ch & utf8_mask[len];
    const char *end = *str + len;

    if (end > strend)
    {
        *str = end;
        return ~0;
    }
    switch (len)
    {
    case 3:
        if ((ch = end[-3] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        (*str)++;
        if (res < 0x10) break;
    case 2:
        if ((ch = end[-2] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        if (res >= 0x110000 >> 6) break;
        (*str)++;
        if (res < 0x20) break;
        if (res >= 0xd800 >> 6 && res <= 0xdfff >> 6) break;
    case 1:
        if ((ch = end[-1] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        (*str)++;
        if (res < 0x80) break;
        return res;
    }
    return ~0;
}


/**************************************************************************
 *	RtlUTF8ToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUTF8ToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen, const char *src, DWORD srclen )
{
    unsigned int res, len;
    NTSTATUS status = STATUS_SUCCESS;
    const char *srcend = src + srclen;
    WCHAR *dstend;

    if (!src) return STATUS_INVALID_PARAMETER_4;
    if (!reslen) return STATUS_INVALID_PARAMETER;

    dstlen /= sizeof(WCHAR);
    dstend = dst + dstlen;
    if (!dst)
    {
        for (len = 0; src < srcend; len++)
        {
            unsigned char ch = *src++;
            if (ch < 0x80) continue;
            if ((res = decode_utf8_char( ch, &src, srcend )) > 0x10ffff)
                status = STATUS_SOME_NOT_MAPPED;
            else
                if (res > 0xffff) len++;
        }
        *reslen = len * sizeof(WCHAR);
        return status;
    }

    while ((dst < dstend) && (src < srcend))
    {
        unsigned char ch = *src++;
        if (ch < 0x80)  /* special fast case for 7-bit ASCII */
        {
            *dst++ = ch;
            continue;
        }
        if ((res = decode_utf8_char( ch, &src, srcend )) <= 0xffff)
        {
            *dst++ = res;
        }
        else if (res <= 0x10ffff)  /* we need surrogates */
        {
            res -= 0x10000;
            *dst++ = 0xd800 | (res >> 10);
            if (dst == dstend) break;
            *dst++ = 0xdc00 | (res & 0x3ff);
        }
        else
        {
            *dst++ = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
    }
    if (src < srcend) status = STATUS_BUFFER_TOO_SMALL;  /* overflow */
    *reslen = (dstlen - (dstend - dst)) * sizeof(WCHAR);
    return status;
}


/* get the next char value taking surrogates into account */
static inline unsigned int get_surrogate_value( const WCHAR *src, unsigned int srclen )
{
    if (src[0] >= 0xd800 && src[0] <= 0xdfff)  /* surrogate pair */
    {
        if (src[0] > 0xdbff || /* invalid high surrogate */
            srclen <= 1 ||     /* missing low surrogate */
            src[1] < 0xdc00 || src[1] > 0xdfff) /* invalid low surrogate */
            return 0;
        return 0x10000 + ((src[0] & 0x3ff) << 10) + (src[1] & 0x3ff);
    }
    return src[0];
}


/**************************************************************************
 *	RtlUnicodeToUTF8N   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToUTF8N( char *dst, DWORD dstlen, DWORD *reslen, const WCHAR *src, DWORD srclen )
{
    char *end;
    unsigned int val, len;
    NTSTATUS status = STATUS_SUCCESS;

    if (!src) return STATUS_INVALID_PARAMETER_4;
    if (!reslen) return STATUS_INVALID_PARAMETER;
    if (dst && (srclen & 1)) return STATUS_INVALID_PARAMETER_5;

    srclen /= sizeof(WCHAR);

    if (!dst)
    {
        for (len = 0; srclen; srclen--, src++)
        {
            if (*src < 0x80) len++;  /* 0x00-0x7f: 1 byte */
            else if (*src < 0x800) len += 2;  /* 0x80-0x7ff: 2 bytes */
            else
            {
                if (!(val = get_surrogate_value( src, srclen )))
                {
                    val = 0xfffd;
                    status = STATUS_SOME_NOT_MAPPED;
                }
                if (val < 0x10000) len += 3; /* 0x800-0xffff: 3 bytes */
                else   /* 0x10000-0x10ffff: 4 bytes */
                {
                    len += 4;
                    src++;
                    srclen--;
                }
            }
        }
        *reslen = len;
        return status;
    }

    for (end = dst + dstlen; srclen; srclen--, src++)
    {
        WCHAR ch = *src;

        if (ch < 0x80)  /* 0x00-0x7f: 1 byte */
        {
            if (dst > end - 1) break;
            *dst++ = ch;
            continue;
        }
        if (ch < 0x800)  /* 0x80-0x7ff: 2 bytes */
        {
            if (dst > end - 2) break;
            dst[1] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[0] = 0xc0 | ch;
            dst += 2;
            continue;
        }
        if (!(val = get_surrogate_value( src, srclen )))
        {
            val = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
        if (val < 0x10000)  /* 0x800-0xffff: 3 bytes */
        {
            if (dst > end - 3) break;
            dst[2] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[1] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[0] = 0xe0 | val;
            dst += 3;
        }
        else   /* 0x10000-0x10ffff: 4 bytes */
        {
            if (dst > end - 4) break;
            dst[3] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[2] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[1] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[0] = 0xf0 | val;
            dst += 4;
            src++;
            srclen--;
        }
    }
    if (srclen) status = STATUS_BUFFER_TOO_SMALL;
    *reslen = dstlen - (end - dst);
    return status;
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
    int compose, compat, buf_len;
    WCHAR *buf = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%x %s %d %p %d\n", form, debugstr_wn(src, src_len), src_len, dst, *dst_len );

    switch (form)
    {
    case NormalizationC:  compose = 1; compat = 0; break;
    case NormalizationD:  compose = 0; compat = 0; break;
    case NormalizationKC: compose = 1; compat = 1; break;
    case NormalizationKD: compose = 0; compat = 1; break;
    case 0: return STATUS_INVALID_PARAMETER;
    default: return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (src_len == -1) src_len = strlenW(src) + 1;

    if (!*dst_len)
    {
        *dst_len = compat ? src_len * 18 : src_len * 3;
        if (*dst_len > 64) *dst_len = max( 64, src_len + src_len / 8 );
        return STATUS_SUCCESS;
    }
    if (!src_len)
    {
        *dst_len = 0;
        return STATUS_SUCCESS;
    }

    if (!compose) return decompose_string( compat, src, src_len, dst, dst_len );

    buf_len = src_len * 4;
    for (;;)
    {
        buf = RtlAllocateHeap( GetProcessHeap(), 0, buf_len * sizeof(WCHAR) );
        if (!buf) return STATUS_NO_MEMORY;
        status = decompose_string( compat, src, src_len, buf, &buf_len );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        RtlFreeHeap( GetProcessHeap(), 0, buf );
    }
    if (!status)
    {
        buf_len = compose_string( buf, buf_len );
        if (*dst_len >= buf_len) memcpy( dst, buf, buf_len * sizeof(WCHAR) );
        else status = STATUS_BUFFER_TOO_SMALL;
    }
    RtlFreeHeap( GetProcessHeap(), 0, buf );
    *dst_len = buf_len;
    return status;
}
