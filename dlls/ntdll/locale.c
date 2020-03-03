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
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

/* NLS codepage file format:
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

/* NLS normalization file */
struct norm_table
{
    WCHAR   name[13];      /* 00 file name */
    USHORT  checksum[3];   /* 1a checksum? */
    USHORT  version[4];    /* 20 Unicode version */
    USHORT  form;          /* 28 normalization form */
    USHORT  len_factor;    /* 2a factor for length estimates */
    USHORT  unknown1;      /* 2c */
    USHORT  decomp_size;   /* 2e decomposition hash size */
    USHORT  comp_size;     /* 30 composition hash size */
    USHORT  unknown2;      /* 32 */
    USHORT  classes;       /* 34 combining classes table offset */
    USHORT  props_level1;  /* 36 char properties table level 1 offset */
    USHORT  props_level2;  /* 38 char properties table level 2 offset */
    USHORT  decomp_hash;   /* 3a decomposition hash table offset */
    USHORT  decomp_map;    /* 3c decomposition character map table offset */
    USHORT  decomp_seq;    /* 3e decomposition character sequences offset */
    USHORT  comp_hash;     /* 40 composition hash table offset */
    USHORT  comp_seq;      /* 42 composition character sequences offset */
    /* BYTE[]       combining class values */
    /* BYTE[0x2200] char properties index level 1 */
    /* BYTE[]       char properties index level 2 */
    /* WORD[]       decomposition hash table */
    /* WORD[]       decomposition character map */
    /* WORD[]       decomposition character sequences */
    /* WORD[]       composition hash table */
    /* WORD[]       composition character sequences */
};

LCID user_lcid = 0, system_lcid = 0;

static LANGID user_ui_language, system_ui_language;
static NLSTABLEINFO nls_info;
static HMODULE kernel32_handle;
static CPTABLEINFO unix_table;
static struct norm_table *norm_tables[16];


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


static int get_utf16( const WCHAR *src, unsigned int srclen, unsigned int *ch )
{
    if (IS_HIGH_SURROGATE( src[0] ))
    {
        if (srclen <= 1) return 0;
        if (!IS_LOW_SURROGATE( src[1] )) return 0;
        *ch = 0x10000 + ((src[0] & 0x3ff) << 10) + (src[1] & 0x3ff);
        return 2;
    }
    if (IS_LOW_SURROGATE( src[0] )) return 0;
    *ch = src[0];
    return 1;
}

static void put_utf16( WCHAR *dst, unsigned int ch )
{
    if (ch >= 0x10000)
    {
        ch -= 0x10000;
        dst[0] = 0xd800 | (ch >> 10);
        dst[1] = 0xdc00 | (ch & 0x3ff);
    }
    else dst[0] = ch;
}


static NTSTATUS load_norm_table( ULONG form, const struct norm_table **info )
{
    unsigned int i;
    USHORT *data, *tables;
    SIZE_T size;
    NTSTATUS status;

    if (!form) return STATUS_INVALID_PARAMETER;
    if (form >= ARRAY_SIZE(norm_tables)) return STATUS_OBJECT_NAME_NOT_FOUND;

    if (!norm_tables[form])
    {
        if ((status = NtGetNlsSectionPtr( NLS_SECTION_NORMALIZE, form, NULL, (void **)&data, &size )))
            return status;

        /* sanity checks */

        if (size <= 0x44) goto invalid;
        if (data[0x14] != form) goto invalid;
        tables = data + 0x1a;
        for (i = 0; i < 8; i++)
        {
            if (tables[i] > size / sizeof(USHORT)) goto invalid;
            if (i && tables[i] < tables[i-1]) goto invalid;
        }

        if (interlocked_cmpxchg_ptr( (void **)&norm_tables[form], data, NULL ))
            RtlFreeHeap( GetProcessHeap(), 0, data );
    }
    *info = norm_tables[form];
    return STATUS_SUCCESS;

invalid:
    RtlFreeHeap( GetProcessHeap(), 0, data );
    return STATUS_INVALID_PARAMETER;
}


static BYTE rol( BYTE val, BYTE count )
{
    return (val << count) | (val >> (8 - count));
}


static BYTE get_char_props( const struct norm_table *info, unsigned int ch )
{
    const BYTE *level1 = (const BYTE *)((const USHORT *)info + info->props_level1);
    const BYTE *level2 = (const BYTE *)((const USHORT *)info + info->props_level2);
    BYTE off = level1[ch / 128];

    if (!off || off >= 0xfb) return rol( off, 5 );
    return level2[(off - 1) * 128 + ch % 128];
}


#define HANGUL_SBASE  0xac00
#define HANGUL_LBASE  0x1100
#define HANGUL_VBASE  0x1161
#define HANGUL_TBASE  0x11a7
#define HANGUL_LCOUNT 19
#define HANGUL_VCOUNT 21
#define HANGUL_TCOUNT 28
#define HANGUL_NCOUNT (HANGUL_VCOUNT * HANGUL_TCOUNT)
#define HANGUL_SCOUNT (HANGUL_LCOUNT * HANGUL_NCOUNT)

static const WCHAR *get_decomposition( const struct norm_table *info, unsigned int ch,
                                       BYTE props, WCHAR *buffer, unsigned int *ret_len )
{
    const struct pair { WCHAR src; USHORT dst; } *pairs;
    const USHORT *hash_table = (const USHORT *)info + info->decomp_hash;
    const WCHAR *ret;
    unsigned int i, pos, end, len, hash;

    /* default to no decomposition */
    put_utf16( buffer, ch );
    *ret_len = 1 + (ch >= 0x10000);
    if (!props || props == 0x7f) return buffer;

    if (props == 0xff)  /* Hangul or invalid char */
    {
        if (ch >= HANGUL_SBASE && ch < HANGUL_SBASE + HANGUL_SCOUNT)
        {
            unsigned short sindex = ch - HANGUL_SBASE;
            unsigned short tindex = sindex % HANGUL_TCOUNT;
            buffer[0] = HANGUL_LBASE + sindex / HANGUL_NCOUNT;
            buffer[1] = HANGUL_VBASE + (sindex % HANGUL_NCOUNT) / HANGUL_TCOUNT;
            if (tindex) buffer[2] = HANGUL_TBASE + tindex;
            *ret_len = 2 + !!tindex;
            return buffer;
        }
        /* ignore other chars in Hangul range */
        if (ch >= HANGUL_LBASE && ch < HANGUL_LBASE + 0x100) return buffer;
        if (ch >= HANGUL_SBASE && ch < HANGUL_SBASE + 0x2c00) return buffer;
        return NULL;
    }

    hash = ch % info->decomp_size;
    pos = hash_table[hash];
    if (pos >> 13)
    {
        if (props != 0xbf) return buffer;
        ret = (const USHORT *)info + info->decomp_seq + (pos & 0x1fff);
        len = pos >> 13;
    }
    else
    {
        pairs = (const struct pair *)((const USHORT *)info + info->decomp_map);

        /* find the end of the hash bucket */
        for (i = hash + 1; i < info->decomp_size; i++) if (!(hash_table[i] >> 13)) break;
        if (i < info->decomp_size) end = hash_table[i];
        else for (end = pos; pairs[end].src; end++) ;

        for ( ; pos < end; pos++)
        {
            if (pairs[pos].src != (WCHAR)ch) continue;
            ret = (const USHORT *)info + info->decomp_seq + (pairs[pos].dst & 0x1fff);
            len = pairs[pos].dst >> 13;
            break;
        }
        if (pos >= end) return buffer;
    }

    if (len == 7) while (ret[len]) len++;
    if (!ret[0]) len = 0;  /* ignored char */
    *ret_len = len;
    return ret;
}


static BYTE get_combining_class( const struct norm_table *info, unsigned int c )
{
    const BYTE *classes = (const BYTE *)((const USHORT *)info + info->classes);
    BYTE class = get_char_props( info, c ) & 0x3f;

    if (class == 0x3f) return 0;
    return classes[class];
}


static BOOL is_starter( const struct norm_table *info, unsigned int c )
{
    return !get_combining_class( info, c );
}


static BOOL reorderable_pair( const struct norm_table *info, unsigned int c1, unsigned int c2 )
{
    BYTE ccc1, ccc2;

    /* reorderable if ccc1 > ccc2 > 0 */
    ccc1 = get_combining_class( info, c1 );
    if (ccc1 < 2) return FALSE;
    ccc2 = get_combining_class( info, c2 );
    return ccc2 && (ccc1 > ccc2);
}

static void canonical_order_substring( const struct norm_table *info, WCHAR *str, unsigned int len )
{
    unsigned int i, ch1, ch2, len1, len2;
    BOOL swapped;

    do
    {
        swapped = FALSE;
        for (i = 0; i < len - 1; i += len1)
        {
            if (!(len1 = get_utf16( str + i, len - i, &ch1 ))) break;
            if (i + len1 >= len) break;
            if (!(len2 = get_utf16( str + i + len1, len - i - len1, &ch2 ))) break;
            if (reorderable_pair( info, ch1, ch2 ))
            {
                WCHAR tmp[2];
                memcpy( tmp, str + i, len1 * sizeof(WCHAR) );
                memcpy( str + i, str + i + len1, len2 * sizeof(WCHAR) );
                memcpy( str + i + len2, tmp, len1 * sizeof(WCHAR) );
                swapped = TRUE;
                i += len2 - len1;
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
static void canonical_order_string( const struct norm_table *info, WCHAR *str, unsigned int len )
{
    unsigned int ch, i, r, next = 0;

    for (i = 0; i < len; i += r)
    {
        if (!(r = get_utf16( str + i, len - i, &ch ))) return;
        if (i && is_starter( info, ch ))
        {
            if (i > next + 1) /* at least two successive non-starters */
                canonical_order_substring( info, str + next, i - next );
            next = i + r;
        }
    }
    if (i > next + 1) canonical_order_substring( info, str + next, i - next );
}


static NTSTATUS decompose_string( const struct norm_table *info, const WCHAR *src, int src_len,
                                  WCHAR *dst, int *dst_len )
{
    BYTE props;
    int src_pos, dst_pos;
    unsigned int ch, len, decomp_len;
    WCHAR buffer[3];
    const WCHAR *decomp;

    for (src_pos = dst_pos = 0; src_pos < src_len; src_pos += len)
    {
        if (!(len = get_utf16( src + src_pos, src_len - src_pos, &ch )))
        {
            *dst_len = src_pos + IS_HIGH_SURROGATE( src[src_pos] );
            return STATUS_NO_UNICODE_TRANSLATION;
        }
        props = get_char_props( info, ch );
        if (!(decomp = get_decomposition( info, ch, props, buffer, &decomp_len )))
        {
            /* allow final null */
            if (!ch && src_pos == src_len - 1 && dst_pos < *dst_len)
            {
                dst[dst_pos++] = 0;
                break;
            }
            *dst_len = src_pos;
            return STATUS_NO_UNICODE_TRANSLATION;
        }
        if (dst_pos + decomp_len > *dst_len)
        {
            *dst_len += (src_len - src_pos) * info->len_factor;
            return STATUS_BUFFER_TOO_SMALL;
        }
        memcpy( dst + dst_pos, decomp, decomp_len * sizeof(WCHAR) );
        dst_pos += decomp_len;
    }

    canonical_order_string( info, dst, dst_pos );
    *dst_len = dst_pos;
    return STATUS_SUCCESS;
}


static unsigned int compose_hangul( unsigned int ch1, unsigned int ch2 )
{
    if (ch1 >= HANGUL_LBASE && ch1 < HANGUL_LBASE + HANGUL_LCOUNT)
    {
        int lindex = ch1 - HANGUL_LBASE;
        int vindex = ch2 - HANGUL_VBASE;
        if (vindex >= 0 && vindex < HANGUL_VCOUNT)
            return HANGUL_SBASE + (lindex * HANGUL_VCOUNT + vindex) * HANGUL_TCOUNT;
    }
    if (ch1 >= HANGUL_SBASE && ch1 < HANGUL_SBASE + HANGUL_SCOUNT)
    {
        int sindex = ch1 - HANGUL_SBASE;
        if (!(sindex % HANGUL_TCOUNT))
        {
            int tindex = ch2 - HANGUL_TBASE;
            if (tindex > 0 && tindex < HANGUL_TCOUNT) return ch1 + tindex;
        }
    }
    return 0;
}


static unsigned int compose_chars( const struct norm_table *info, unsigned int ch1, unsigned int ch2 )
{
    const USHORT *table = (const USHORT *)info + info->comp_hash;
    const WCHAR *chars = (const USHORT *)info + info->comp_seq;
    unsigned int hash, start, end, i, len, ch[3];

    hash = (ch1 + 95 * ch2) % info->comp_size;
    start = table[hash];
    end = table[hash + 1];
    while (start < end)
    {
        for (i = 0; i < 3; i++, start += len) len = get_utf16( chars + start, end - start, ch + i );
        if (ch[0] == ch1 && ch[1] == ch2) return ch[2];
    }
    return 0;
}

static unsigned int compose_string( const struct norm_table *info, WCHAR *str, unsigned int srclen )
{
    unsigned int i, ch, comp, len, start_ch = 0, last_starter = srclen;
    BYTE class, prev_class = 0;

    for (i = 0; i < srclen; i += len)
    {
        if (!(len = get_utf16( str + i, srclen - i, &ch ))) return 0;
        class = get_combining_class( info, ch );
        if (last_starter == srclen || (prev_class && prev_class >= class) ||
            (!(comp = compose_hangul( start_ch, ch )) &&
             !(comp = compose_chars( info, start_ch, ch ))))
        {
            if (!class)
            {
                last_starter = i;
                start_ch = ch;
            }
            prev_class = class;
        }
        else
        {
            int comp_len = 1 + (comp >= 0x10000);
            int start_len = 1 + (start_ch >= 0x10000);

            if (comp_len != start_len)
                memmove( str + last_starter + comp_len, str + last_starter + start_len,
                         (i - (last_starter + start_len)) * sizeof(WCHAR) );
            memmove( str + i + comp_len - start_len, str + i + len, (srclen - i - len) * sizeof(WCHAR) );
            srclen += comp_len - start_len - len;
            start_ch = comp;
            i = last_starter;
            len = comp_len;
            prev_class = 0;
            put_utf16( str + i, comp );
        }
    }
    return srclen;
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
    static const WCHAR dataprefixW[] = {'\\','n','l','s','\\',0};
    static const WCHAR cpdefaultW[] = {'c','_','%','0','3','d','.','n','l','s',0};
    static const WCHAR intlW[] = {'l','_','i','n','t','l','.','n','l','s',0};
    static const WCHAR normnfcW[] = {'n','o','r','m','n','f','c','.','n','l','s',0};
    static const WCHAR normnfdW[] = {'n','o','r','m','n','f','d','.','n','l','s',0};
    static const WCHAR normnfkcW[] = {'n','o','r','m','n','f','k','c','.','n','l','s',0};
    static const WCHAR normnfkdW[] = {'n','o','r','m','n','f','k','d','.','n','l','s',0};
    static const WCHAR normidnaW[] = {'n','o','r','m','i','d','n','a','.','n','l','s',0};

    DWORD size;
    HANDLE handle;
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    WCHAR buffer[MAX_PATH], value[10];
    const WCHAR *name = NULL;
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
            case 13: name = normidnaW; break;
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
        if (RtlQueryEnvironmentVariable_U( NULL, &nameW, &valueW ) != STATUS_BUFFER_TOO_SMALL)
            return status;
    }
    valueW.MaximumLength = valueW.Length + sizeof(dataprefixW) + strlenW(name) * sizeof(WCHAR);
    if (!(valueW.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, valueW.MaximumLength )))
        return STATUS_NO_MEMORY;
    if (!RtlQueryEnvironmentVariable_U( NULL, &nameW, &valueW ))
    {
        strcatW( valueW.Buffer, dataprefixW );
        strcatW( valueW.Buffer, name );
        valueW.Length = strlenW(valueW.Buffer) * sizeof(WCHAR);
        InitializeObjectAttributes( &attr, &valueW, 0, 0, NULL );
        status = NtOpenFile( file, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
        if (!status) TRACE( "found %s\n", debugstr_w( valueW.Buffer ));
    }
    RtlFreeUnicodeString( &valueW );
    return status;
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
    { "IBM424", 20424 },
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
    { "ISO885913", 28603 },
    { "ISO885915", 28605 },
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

static void load_unix_cptable( unsigned int cp )
{
    const char *build_dir = wine_get_build_dir();
    const char *data_dir = wine_get_data_dir();
    const char *dir = build_dir ? build_dir : data_dir;
    struct stat st;
    char *name;
    USHORT *data;
    int fd;

    if (!(name = RtlAllocateHeap( GetProcessHeap(), 0, strlen(dir) + 22 ))) return;
    sprintf( name, "%s/nls/c_%03u.nls", dir, cp );
    if ((fd = open( name, O_RDONLY )) != -1)
    {
        fstat( fd, &st );
        if ((data = RtlAllocateHeap( GetProcessHeap(), 0, st.st_size )) &&
            st.st_size > 0x10000 &&
            read( fd, data, st.st_size ) == st.st_size)
        {
            RtlInitCodePageTable( data, &unix_table );
        }
        else
        {
            RtlFreeHeap( GetProcessHeap(), 0, data );
        }
        close( fd );
    }
    else ERR( "failed to load %s\n", debugstr_a(name) );
    RtlFreeHeap( GetProcessHeap(), 0, name );
}

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
            if (charset_names[pos].cp != CP_UTF8) load_unix_cptable( charset_names[pos].cp );
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
    DWORD len;
    LCID lcid;

    if (!unix_name || !unix_name[0] || !strcmp( unix_name, "C" ))
    {
        unix_name = getenv( "LC_ALL" );
        if (!unix_name || !unix_name[0]) return 0;
    }

    len = ntdll_umbstowcs( unix_name, strlen(unix_name), buffer, ARRAY_SIZE(buffer) );
    if (len == ARRAY_SIZE(buffer)) return 0;
    buffer[len] = 0;

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
    {
        const struct norm_table *info;
        load_norm_table( NormalizationC, &info );
    }
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
DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen )
{
    DWORD reslen;

    if (unix_table.CodePage)
        RtlCustomCPToUnicodeN( &unix_table, dst, dstlen * sizeof(WCHAR), &reslen, src, srclen );
    else
        RtlUTF8ToUnicodeN( dst, dstlen * sizeof(WCHAR), &reslen, src, srclen );

    reslen /= sizeof(WCHAR);
#ifdef __APPLE__  /* work around broken Mac OS X filesystem that enforces decomposed Unicode */
    if (reslen && dst && norm_tables[NormalizationC])
        reslen = compose_string( norm_tables[NormalizationC], dst, reslen );
#endif
    return reslen;
}


/******************************************************************
 *      ntdll_wcstoumbs
 */
int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict )
{
    DWORD i, reslen;

    if (!unix_table.CodePage)
        RtlUnicodeToUTF8N( dst, dstlen, &reslen, src, srclen * sizeof(WCHAR) );
    else if (!strict)
        RtlUnicodeToCustomCPN( &unix_table, dst, dstlen, &reslen, src, srclen * sizeof(WCHAR) );
    else  /* do it by hand to make sure every character roundtrips correctly */
    {
        if (unix_table.DBCSOffsets)
        {
            const unsigned short *uni2cp = unix_table.WideCharTable;
            for (i = dstlen; srclen && i; i--, srclen--, src++)
            {
                unsigned short ch = uni2cp[*src];
                if (ch >> 8)
                {
                    if (unix_table.DBCSOffsets[unix_table.DBCSOffsets[ch >> 8] + (ch & 0xff)] != *src)
                        return -1;
                    if (i == 1) break;  /* do not output a partial char */
                    i--;
                    *dst++ = ch >> 8;
                }
                else
                {
                    if (unix_table.MultiByteTable[ch] != *src) return -1;
                    *dst++ = (char)ch;
                }
            }
            reslen = dstlen - i;
        }
        else
        {
            const unsigned char *uni2cp = unix_table.WideCharTable;
            reslen = min( srclen, dstlen );
            for (i = 0; i < reslen; i++)
            {
                unsigned char ch = uni2cp[src[i]];
                if (unix_table.MultiByteTable[ch] != src[i]) return -1;
                dst[i] = ch;
            }
        }
    }
    return reslen;
}


/******************************************************************
 *      __wine_get_unix_codepage   (NTDLL.@)
 */
UINT CDECL __wine_get_unix_codepage(void)
{
    if (!unix_table.CodePage) return CP_UTF8;
    return unix_table.CodePage;
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

    if ((status = open_nls_data_file( type, id, &file ))) return status;

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
                if (!get_utf16( src, srclen, &val ))
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
        if (!get_utf16( src, srclen, &val ))
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
    const struct norm_table *info;
    NTSTATUS status;
    BYTE props, class, last_class = 0;
    unsigned int ch;
    int i, r, result = 1;

    if ((status = load_norm_table( form, &info ))) return status;

    if (len == -1) len = strlenW( str );

    for (i = 0; i < len && result; i += r)
    {
        if (!(r = get_utf16( str + i, len - i, &ch ))) return STATUS_NO_UNICODE_TRANSLATION;
        if (info->comp_size)
        {
            if ((ch >= HANGUL_VBASE && ch < HANGUL_VBASE + HANGUL_VCOUNT) ||
                (ch >= HANGUL_TBASE && ch < HANGUL_TBASE + HANGUL_TCOUNT))
            {
                result = -1;  /* QC=Maybe */
                continue;
            }
        }
        else if (ch >= HANGUL_SBASE && ch < HANGUL_SBASE + HANGUL_SCOUNT)
        {
            result = 0;  /* QC=No */
            break;
        }
        props = get_char_props( info, ch );
        class = props & 0x3f;
        if (class == 0x3f)
        {
            last_class = 0;
            if (props == 0xbf) result = 0;  /* QC=No */
            else if (props == 0xff)
            {
                /* ignore other chars in Hangul range */
                if (ch >= HANGUL_LBASE && ch < HANGUL_LBASE + 0x100) continue;
                if (ch >= HANGUL_SBASE && ch < HANGUL_SBASE + 0x2c00) continue;
                /* allow final null */
                if (!ch && i == len - 1) continue;
                return STATUS_NO_UNICODE_TRANSLATION;
            }
        }
        else if (props & 0x80)
        {
            if ((props & 0xc0) == 0xc0) result = -1;  /* QC=Maybe */
            if (class && class < last_class) result = 0;  /* QC=No */
            last_class = class;
        }
        else last_class = 0;
    }

    if (result == -1)
    {
        int dstlen = len * 4;
        NTSTATUS status;
        WCHAR *buffer = RtlAllocateHeap( GetProcessHeap(), 0, dstlen * sizeof(WCHAR) );
        if (!buffer) return STATUS_NO_MEMORY;
        status = RtlNormalizeString( form, str, len, buffer, &dstlen );
        result = !status && (dstlen == len) && !strncmpW( buffer, str, len );
        RtlFreeHeap( GetProcessHeap(), 0, buffer );
    }
    *res = result;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlNormalizeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlNormalizeString( ULONG form, const WCHAR *src, INT src_len, WCHAR *dst, INT *dst_len )
{
    int buf_len;
    WCHAR *buf = NULL;
    const struct norm_table *info;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%x %s %d %p %d\n", form, debugstr_wn(src, src_len), src_len, dst, *dst_len );

    if ((status = load_norm_table( form, &info ))) return status;

    if (src_len == -1) src_len = strlenW(src) + 1;

    if (!*dst_len)
    {
        *dst_len = src_len * info->len_factor;
        if (*dst_len > 64) *dst_len = max( 64, src_len + src_len / 8 );
        return STATUS_SUCCESS;
    }
    if (!src_len)
    {
        *dst_len = 0;
        return STATUS_SUCCESS;
    }

    if (!info->comp_size) return decompose_string( info, src, src_len, dst, dst_len );

    buf_len = src_len * 4;
    for (;;)
    {
        buf = RtlAllocateHeap( GetProcessHeap(), 0, buf_len * sizeof(WCHAR) );
        if (!buf) return STATUS_NO_MEMORY;
        status = decompose_string( info, src, src_len, buf, &buf_len );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        RtlFreeHeap( GetProcessHeap(), 0, buf );
    }
    if (!status)
    {
        buf_len = compose_string( info, buf, buf_len );
        if (*dst_len >= buf_len) memcpy( dst, buf, buf_len * sizeof(WCHAR) );
        else status = STATUS_BUFFER_TOO_SMALL;
    }
    RtlFreeHeap( GetProcessHeap(), 0, buf );
    *dst_len = buf_len;
    return status;
}
