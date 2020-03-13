/*
 * Dump a NLS file
 *
 * Copyright 2020 Alexandre Julliard
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
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winedump.h"

static const void *read_data( unsigned int *pos, unsigned int size )
{
    const void *ret = PRD( *pos, size );
    *pos += size;
    return ret;
}

static unsigned short mapchar( const unsigned short *table, unsigned int len, unsigned short ch )
{
    unsigned int off = table[ch >> 8] + ((ch >> 4) & 0x0f);
    if (off >= len) return 0;
    off = table[off] + (ch & 0x0f);
    if (off >= len) return 0;
    return ch + table[off];
}

static void dump_offset_table( const unsigned short *table, unsigned int len )
{
    int i, ch;

    for (i = 0; i < 0x10000; i++)
    {
        if (!(i % 16)) printf( "\n%04x:", i );
        ch = mapchar( table, len, i );
        if (ch == i) printf( " ...." );
        else printf( " %04x", ch );
    }
}

struct ctype
{
    WORD c1, c2, c3;
};

static const char *get_ctype( const struct ctype *ctype )
{
    static char buffer[100];
    static const char *c1[] = { "up ", "lo ", "dg ", "sp ", "pt ", "cl ", "bl ", "xd ", "al " };
    static const char *c2[] = { "  ", "L ", "R ", "EN", "ES", "ET",
                                "AN", "CS", "B ", "S ", "WS", "ON" };
    static const char *c3[] = { "ns ", "di ", "vo ", "sy ", "ka ", "hi ", "hw ", "fw ",
                                "id ", "ks ", "lx ", "hi ", "lo ", "   ", "   ", "al " };
    int i;
    strcpy( buffer, "| " );
    for (i = 0; i < ARRAY_SIZE(c1); i++)
        strcat( buffer, (ctype->c1 & (1 << i)) ? c1[i] : "__ " );
    strcat( buffer, "|  " );
    strcat( buffer, ctype->c2 < ARRAY_SIZE(c2) ? c2[ctype->c2] : "??" );
    strcat( buffer, "  | " );
    for (i = 0; i < ARRAY_SIZE(c3); i++)
        strcat( buffer, (ctype->c3 & (1 << i)) ? c3[i] : "__ " );
    strcat( buffer, "|" );
    return buffer;
}

static void dump_ctype_table( const USHORT *ptr )
{
    const struct ctype *ctypes = (const struct ctype *)(ptr + 2);
    const BYTE *types = (const BYTE *)ptr + ptr[1] + 2;
    int i, len = (ptr[1] - 2) / sizeof(*ctypes);

    printf( "                  CTYPE1            CTYPE2                     CTYPE3\n" );
    for (i = 0; i < 0x10000; i++)
    {
        const BYTE *b = types + ((const WORD *)types)[i >> 8];
        b = types + ((const WORD *)b)[(i >> 4) & 0x0f] + (i & 0x0f);
        if (*b < len) printf( "%04x  %s\n", i, get_ctype( ctypes + *b ));
        else  printf( "%04x  ??? %02x\n", i, *b );
    }
    printf( "\n" );
}

static void dump_casemap(void)
{
    unsigned int pos = 0, upper_len, lower_len;
    const unsigned short *header, *upper, *lower;

    if (!(header = read_data( &pos, 2 * sizeof(*header) ))) return;
    upper_len = header[1];
    if (!(upper = read_data( &pos, upper_len * sizeof(*upper) )))
    {
        printf( "Invalid len %04x\n", header[1] );
        return;
    }
    lower_len = dump_total_len / sizeof(*lower) - 2 - upper_len;
    if (!(lower = read_data( &pos, lower_len * sizeof(*lower) ))) return;

    printf( "Magic: %04x\n", header[0] );
    printf( "Upper-case table:\n" );
    dump_offset_table( upper, upper_len );
    printf( "\n\nLower-case table:\n" );
    dump_offset_table( lower, lower_len );
    printf( "\n\n" );
}

static void dump_codepage(void)
{
    unsigned int i, j, uni2cp_offset, pos = 0;
    const unsigned short *header, *ptr;

    if (!(header = read_data( &pos, 13 * sizeof(*header) ))) return;
    printf( "Codepage:       %03u\n", header[1] );
    printf( "Char size:      %u\n", header[2] );
    printf( "Default char A: %04x / %04x\n", header[3], header[5] );
    printf( "Default char W: %04x / %04x\n", header[4], header[6] );
    if (header[2] == 2)
    {
        printf( "Lead bytes:    " );
        for (i = 0; i < 12; i++)
        {
            unsigned char val = ((unsigned char *)(header + 7))[i];
            if (!val) break;
            printf( "%c%02x", (i % 2) ? '-' : ' ', val );
        }
        printf( "\n" );
    }
    printf( "\nCharacter map:\n" );
    pos = header[0] * sizeof(*ptr);
    if (!(ptr = read_data( &pos, sizeof(*ptr) ))) return;
    uni2cp_offset = pos / sizeof(*ptr) + *ptr;
    if (!(ptr = read_data( &pos, 256 * sizeof(*ptr) ))) return;
    for (i = 0; i < 256; i++)
    {
        if (!(i % 16)) printf( "\n%02x:", i );
        printf( " %04x", ptr[i] );
    }
    printf( "\n" );
    if (!(ptr = read_data( &pos, sizeof(*ptr) ))) return;
    if (*ptr == 256)
    {
        if (!(ptr = read_data( &pos, 256 * sizeof(*ptr) ))) return;
        printf( "\nGlyph table:\n" );
        for (i = 0; i < 256; i++)
        {
            if (!(i % 16)) printf( "\n%02x:", i );
            printf( " %04x", ptr[i] );
        }
        printf( "\n" );
    }
    if (!(ptr = read_data( &pos, sizeof(*ptr) ))) return;
    if (*ptr)
    {
        if (!(ptr = read_data( &pos, (uni2cp_offset - pos) * sizeof(*ptr) ))) return;
        for (i = 0; i < 256; i++)
        {
            if (!ptr[i] || ptr[i] > pos - 256) continue;
            for (j = 0; j < 256; j++)
            {
                if (!(j % 16)) printf( "\n%02x%02x:", i, j );
                printf( " %04x", ptr[ptr[i] + j] );
            }
        }
        printf( "\n" );
    }
    printf( "\nUnicode table:\n" );
    pos = uni2cp_offset * sizeof(*ptr);
    if (header[2] == 2)
    {
        if (!(ptr = read_data( &pos, 65536 * sizeof(*ptr) ))) return;
        for (i = 0; i < 65536; i++)
        {
            if (!(i % 16)) printf( "\n%04x:", i );
            printf( " %04x", ptr[i] );
        }
        printf( "\n" );
    }
    else
    {
        const unsigned char *uni2cp;
        if (!(uni2cp = read_data( &pos, 65536 ))) return;
        for (i = 0; i < 65536; i++)
        {
            if (!(i % 16)) printf( "\n%04x:", i );
            printf( " %02x", uni2cp[i] );
        }
        printf( "\n" );
    }
    printf( "\n" );
}

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
};

static int offset_scale = 1;  /* older versions use byte offsets */

#define GET_TABLE(info,table) ((const void *)((const BYTE *)info + (info->table * offset_scale)))

static unsigned int get_utf16( const WCHAR *str )
{
    if (str[0] >= 0xd800 && str[0] <= 0xdbff &&
        str[1] >= 0xdc00 && str[1] <= 0xdfff)
        return 0x10000 + ((str[0] & 0x3ff) << 10) + (str[1] & 0x3ff);
    return str[0];
}

static BYTE rol( BYTE val, BYTE count )
{
    return (val << count) | (val >> (8 - count));
}

static unsigned char get_char_props( const struct norm_table *info, unsigned int ch )
{
    const BYTE *level1 = GET_TABLE( info, props_level1 );
    const BYTE *level2 = GET_TABLE( info, props_level2 );
    BYTE off = level1[ch / 128];

    if (!off || off >= 0xfb) return rol( off, 5 );
    return level2[(off - 1) * 128 + ch % 128];
}

static const WCHAR *get_decomposition( const struct norm_table *info,
                                       unsigned int ch, unsigned int *ret_len )
{
    const USHORT *hash_table = GET_TABLE( info, decomp_hash );
    const WCHAR *seq = GET_TABLE(info, decomp_seq );
    const WCHAR *ret;
    unsigned int i, pos, end, len, hash;

    *ret_len = 1 + (ch >= 0x10000);
    if (!info->decomp_size) return NULL;
    hash = ch % info->decomp_size;
    pos = hash_table[hash];
    if (pos >> 13)
    {
        if (get_char_props( info, ch ) != 0xbf) return NULL;
        ret = seq + (pos & 0x1fff);
        len = pos >> 13;
    }
    else
    {
        const struct { WCHAR src; USHORT dst; } *pairs = GET_TABLE( info, decomp_map );

        /* find the end of the hash bucket */
        for (i = hash + 1; i < info->decomp_size; i++) if (!(hash_table[i] >> 13)) break;
        if (i < info->decomp_size) end = hash_table[i];
        else for (end = pos; pairs[end].src; end++) ;

        for ( ; pos < end; pos++)
        {
            if (pairs[pos].src != (WCHAR)ch) continue;
            ret = seq + (pairs[pos].dst & 0x1fff);
            len = pairs[pos].dst >> 13;
            break;
        }
        if (pos >= end) return NULL;
    }

    if (len == 7) while (ret[len]) len++;
    *ret_len = len;
    return ret;
}

static int cmp_compos( const void *a, const void *b )
{
    int ret = ((unsigned int *)a)[0] - ((unsigned int *)b)[0];
    if (!ret) ret = ((unsigned int *)a)[1] - ((unsigned int *)b)[1];
    return ret;
}

static void dump_norm(void)
{
    const struct norm_table *info;
    const BYTE *classes;
    unsigned int i;
    char name[13];

    if (!(info = PRD( 0, sizeof(*info) ))) return;
    for (i = 0; i < sizeof(name); i++) name[i] = info->name[i];
    printf( "Name:    %s\n", name );
    switch (info->form)
    {
    case 1:  printf( "Form:    NFC\n" ); break;
    case 2:  printf( "Form:    NFD\n" ); break;
    case 5:  printf( "Form:    NFKC\n" ); break;
    case 6:  printf( "Form:    NFKD\n" ); break;
    case 13: printf( "Form:    IDNA\n" ); break;
    default: printf( "Form:    %u\n", info->form ); break;
    }
    printf( "Version: %u.%u.%u\n", info->version[0], info->version[1], info->version[2] );
    printf( "Factor:  %u\n", info->len_factor );

    if (info->classes == sizeof(*info) / 2) offset_scale = 2;
    classes = GET_TABLE( info, classes );

    printf( "\nCharacter classes:\n" );
    for (i = 0; i < 0x110000; i++)
    {
        BYTE flags = get_char_props( info, i );

        if (!(i % 16)) printf( "\n%06x:", i );
        if (!flags || (flags & 0x3f) == 0x3f)
        {
            static const char *flagstr[4] = { ".....", "Undef", "QC=No", "Inval" };
            printf( " %s", flagstr[flags >> 6] );
        }
        else
        {
            static const char flagschar[4] = ".+*M";
            BYTE class = classes[flags & 0x3f];
            printf( " %c.%03u", flagschar[flags >> 6], class );
        }
    }

    printf( "\n\nDecompositions:\n\n" );
    for (i = 0; i < 0x110000; i++)
    {
        unsigned int j, len;
        const WCHAR *decomp = get_decomposition( info, i, &len );
        if (!decomp) continue;
        printf( "%04x ->", i );
        for (j = 0; j < len; j++)
        {
            unsigned int ch = get_utf16( decomp + j );
            printf( " %04x", ch );
            if (ch >= 0x10000) j++;
        }
        printf( "\n" );
    }
    if (info->comp_size)
    {
        unsigned int pos, len = (dump_total_len - info->comp_seq * offset_scale) / sizeof(WCHAR);
        const WCHAR *seq = GET_TABLE( info, comp_seq );
        unsigned int *map = malloc( len * sizeof(*map) );

        printf( "\nCompositions:\n\n" );

        /* ignore hash table, simply dump all the sequences */
        for (i = pos = 0; i < len; pos += 3)
        {
            map[pos] = get_utf16( seq + i );
            i += 1 + (map[pos] >= 0x10000);
            map[pos+1] = get_utf16( seq + i );
            i += 1 + (map[pos+1] >= 0x10000);
            map[pos+2] = get_utf16( seq + i );
            i += 1 + (map[pos+2] >= 0x10000);
        }
        qsort( map, pos / 3, 3 * sizeof(*map), cmp_compos );
        for (i = 0; i < pos; i += 3) printf( "%04x %04x -> %04x\n", map[i], map[i + 1], map[i + 2] );
        free( map );
    }
    printf( "\n" );
}


struct sortguid
{
    GUID  id;          /* sort GUID */
    DWORD flags;       /* flags */
    DWORD compr;       /* offset to compression table */
    DWORD except;      /* exception table offset in sortkey table */
    DWORD ling_except; /* exception table offset for linguistic casing */
    DWORD casemap;     /* linguistic casemap table offset */
};

#define FLAG_HAS_3_BYTE_WEIGHTS 0x01
#define FLAG_REVERSEDIACRITICS  0x10
#define FLAG_DOUBLECOMPRESSION  0x20
#define FLAG_INVERSECASING      0x40

struct language_id
{
    DWORD offset;
    WCHAR name[32];
};

struct compression
{
    DWORD offset;
    WCHAR minchar, maxchar;
    WORD  len[8];
};

struct comprlang
{
    struct compression compr;
    WCHAR name[32];
};

static const char *get_sortkey( DWORD key )
{
    static char buffer[16];
    if (!key) return "....";
    if ((WORD)key == 0x200)
        sprintf( buffer, "expand %04x", key >> 16 );
    else
        sprintf( buffer, "%u.%u.%u.%u", (BYTE)(key >> 8), (BYTE)key, (BYTE)(key >> 16), (BYTE)(key >> 24) );
    return buffer;
}

static const void *dump_expansions( const DWORD *ptr )
{
    DWORD i, count = *ptr++;

    printf( "\nExpansions: (count=%04x)\n\n", count );
    for (i = 0; i < count; i++)
    {
        const WCHAR *p = (const WCHAR *)(ptr + i);
        printf( "  %04x: %04x %04x\n", i, p[0], p[1] );
    }
    return ptr + count;
}

static void dump_exceptions( const DWORD *sortkeys, DWORD offset )
{
    int i, j;
    const DWORD *table = sortkeys + offset;

    for (i = 0; i < 0x100; i++)
    {
        if (table[i] == i * 0x100) continue;
        for (j = 0; j < 0x100; j++)
        {
            if (sortkeys[table[i] + j] == sortkeys[i * 0x100 + j]) continue;
            printf( "    %04x: %s\n", i * 0x100 + j, get_sortkey( sortkeys[table[i] + j] ));
        }
    }
}

static const void *dump_compression( const struct compression *compr, const WCHAR *table )
{
    int i, j, k;
    const WCHAR *p = table + compr->offset;

    printf( "  min=%04x max=%04x counts=%u,%u,%u,%u,%u,%u,%u,%u\n",
            compr->minchar, compr->maxchar,
            compr->len[0], compr->len[1], compr->len[2], compr->len[3],
            compr->len[4], compr->len[5], compr->len[6], compr->len[7] );
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < compr->len[i]; j++)
        {
            printf( "    " );
            for (k = 0; k < i + 2; k++) printf( " %04x", *p++ );
            p = (const WCHAR *)(((ULONG_PTR)p + 3) & ~3);
            printf( " -> %s\n", get_sortkey( *(const DWORD *)p ));
            p += 2;
        }
    }
    return p;
}

static const void *dump_multiple_weights( const DWORD *ptr )
{
    int i, count = *ptr++;
    const WCHAR *p;

    printf( "\nMultiple weights: (count=%u)\n\n", count );
    p = (const WCHAR *)ptr;
    for (i = 0; i < count; i++)
    {
        BYTE weight = p[i];
        BYTE count = p[i] >> 8;
        printf( "%u - %u\n", weight, weight + count );
    }
    return ptr + (count + 1) / 2;
}

static void dump_sort( int old_version )
{
    const struct
    {
        DWORD sortkeys;
        DWORD casemaps;
        DWORD ctypes;
        DWORD sortids;
    } *header;

    const struct compression *compr;
    const struct sortguid *guids;
    const struct comprlang *comprlangs;
    const struct language_id *language_ids = NULL;
    const WORD *casemaps, *map;
    const DWORD *sortkeys, *ptr;
    const WCHAR *p = NULL;
    int i, j, size, len;
    int nb_casemaps = 0, casemap_offsets[16];

    if (!(header = PRD( 0, sizeof(*header) ))) return;

    if (!(sortkeys = PRD( header->sortkeys, header->casemaps - header->sortkeys ))) return;
    printf( "\nSort keys:\n" );
    for (i = 0; i < 0x10000; i++)
    {
        if (!(i % 8)) printf( "\n%04x:", i );
        printf( " %16s", get_sortkey( sortkeys[i] ));
    }
    printf( "\n\n" );

    size = (header->ctypes - header->casemaps) / sizeof(*casemaps);
    if (!(casemaps = PRD( header->casemaps, size * sizeof(*casemaps) ))) return;
    len = 0;
    if (old_version)
    {
        ptr = (const DWORD *)casemaps;
        len = *ptr++;
        language_ids = (const struct language_id *)ptr;
        casemaps = (const WORD *)(language_ids + len);
    }
    map = casemaps;
    while (size)
    {
        const WORD *upper = map + 2;
        const WORD *lower = map + 2 + map[1];
        const WORD *end = map + map[1] + 1 + map[map[1] + 1];

        if (map[0] != 1) break;
        printf( "\nCase mapping table %u:\n", nb_casemaps );
        casemap_offsets[nb_casemaps++] = map - casemaps;
        for (j = 0; j < len; j++)
        {
            if (language_ids[j].offset != map - casemaps) continue;
            printf( "Language: %s\n", get_unicode_str( language_ids[j].name, -1 ));
            break;
        }
        printf( "\nUpper-case table:\n" );
        dump_offset_table( upper, lower - upper );
        printf( "\n\nLower-case table:\n" );
        dump_offset_table( lower, end - lower );
        printf( "\n\n" );
        size -= (end - map);
        map = end;
    }

    if (!(p = PRD( header->ctypes, header->sortids - header->ctypes ))) return;
    printf( "\nCTYPE table:\n\n" );
    dump_ctype_table( p );

    printf( "\nSort tables:\n\n" );
    size = (dump_total_len - header->sortids) / sizeof(*ptr);
    if (!(ptr = PRD( header->sortids, size * sizeof(*ptr) ))) return;

    if (old_version)
    {
        len = *ptr++;
        for (i = 0; i < len; i++, ptr += 2) printf( "NLS version: %08x %08x\n", ptr[0], ptr[1] );
        len = *ptr++;
        for (i = 0; i < len; i++, ptr += 2) printf( "Defined version: %08x %08x\n", ptr[0], ptr[1] );
        len = *ptr++;
        printf( "\nReversed diacritics:\n\n" );
        for (i = 0; i < len; i++)
        {
            const WCHAR *name = (const WCHAR *)ptr;
            printf( "%s\n", get_unicode_str( name, -1 ));
            ptr += 16;
        }
        len = *ptr++;
        printf( "\nDouble compression:\n\n" );
        for (i = 0; i < len; i++)
        {
            const WCHAR *name = (const WCHAR *)ptr;
            printf( "%s\n", get_unicode_str( name, -1 ));
            ptr += 16;
        }
        ptr = dump_expansions( ptr );

        printf( "\nCompressions:\n" );
        size = *ptr++;
        comprlangs = (const struct comprlang *)ptr;
        for (i = 0; i < size; i++)
        {
            printf( "\n  %s\n", get_unicode_str( comprlangs[i].name, -1 ));
            ptr = dump_compression( &comprlangs[i].compr, (const WCHAR *)(comprlangs + size) );
        }

        ptr = dump_multiple_weights( ptr );

        size = *ptr++;
        printf( "\nJamo sort:\n\n" );
        for (i = 0; i < size; i++, ptr += 2)
        {
            const struct jamo { BYTE val[5], off, len; } *jamo = (const struct jamo *)ptr;
            printf( "%04x: %02x %02x %02x %02x %02x off=%02x len=%02x\n", 0x1100 + i, jamo->val[0],
                    jamo->val[1], jamo->val[2], jamo->val[3], jamo->val[4],
                    jamo->off, jamo->len );
        }

        size = *ptr++;
        printf( "\nJamo second chars:\n\n" );
        for (i = 0; i < size; i++, ptr += 2)
        {
            const struct jamo { WORD ch; BYTE val[5], len; } *jamo = (const struct jamo *)ptr;
            printf( "%02x: %04x: %02x %02x %02x %02x %02x len=%02x\n", i, jamo->ch, jamo->val[0],
                    jamo->val[1], jamo->val[2], jamo->val[3], jamo->val[4], jamo->len );
        }

        size = *ptr++;
        printf( "\nExceptions:\n" );
        language_ids = (const struct language_id *)ptr;
        for (i = 0; i < size; i++)
        {
            printf( "\n  %08x %s\n", language_ids[i].offset, get_unicode_str( language_ids[i].name, -1 ));
            dump_exceptions( sortkeys, language_ids[i].offset );
        }
    }
    else
    {
        int guid_count = ptr[1];
        printf( "NLS version: %08x\n\n", ptr[0] );
        printf( "Sort GUIDs:\n\n" );
        guids = (const struct sortguid *)(ptr + 2);
        for (i = 0; i < guid_count; i++)
        {
            for (j = 0; j < nb_casemaps; j++) if (casemap_offsets[j] == guids[i].casemap) break;
            printf( "  %s  flags=%08x compr=%08x casemap=%d\n", get_guid_str( &guids[i].id ),
                    guids[i].flags, guids[i].compr, j < nb_casemaps ? j : -1 );
        }

        ptr = dump_expansions( (const DWORD *)(guids + guid_count) );

        size = *ptr++;
        printf( "\nCompressions:\n" );
        compr = (const struct compression *)ptr;
        for (i = 0; i < size; i++)
        {
            printf( "\n" );
            for (j = 0; j < guid_count; j++)
                if (guids[j].compr == i) printf( "  %s\n", get_guid_str( &guids[j].id ));
            ptr = dump_compression( compr + i, (const WCHAR *)(compr + size) );
        }

        ptr = dump_multiple_weights( ptr );

        size = *ptr++;
        printf( "\nJamo sort:\n\n" );
        for (i = 0; i < size; i++)
        {
            static const WCHAR hangul_chars[] =
            {
                0xa960, 0xa961, 0xa962, 0xa963, 0xa964, 0xa965, 0xa966, 0xa967,
                0xa968, 0xa969, 0xa96a, 0xa96b, 0xa96c, 0xa96d, 0xa96e, 0xa96f,
                0xa970, 0xa971, 0xa972, 0xa973, 0xa974, 0xa975, 0xa976, 0xa977,
                0xa978, 0xa979, 0xa97a, 0xa97b, 0xa97c,
                0xd7b0, 0xd7b1, 0xd7b2, 0xd7b3, 0xd7b4, 0xd7b5, 0xd7b6, 0xd7b7,
                0xd7b8, 0xd7b9, 0xd7ba, 0xd7bb, 0xd7bc, 0xd7bd, 0xd7be, 0xd7bf,
                0xd7c0, 0xd7c1, 0xd7c2, 0xd7c3, 0xd7c4, 0xd7c5, 0xd7c6,
                0xd7cb, 0xd7cc, 0xd7cd, 0xd7ce, 0xd7cf,
                0xd7d0, 0xd7d1, 0xd7d2, 0xd7d3, 0xd7d4, 0xd7d5, 0xd7d6, 0xd7d7,
                0xd7d8, 0xd7d9, 0xd7da, 0xd7db, 0xd7dc, 0xd7dd, 0xd7de, 0xd7df,
                0xd7e0, 0xd7e1, 0xd7e2, 0xd7e3, 0xd7e4, 0xd7e5, 0xd7e6, 0xd7e7,
                0xd7e8, 0xd7e9, 0xd7ea, 0xd7eb, 0xd7ec, 0xd7ed, 0xd7ee, 0xd7ef,
                0xd7f0, 0xd7f1, 0xd7f2, 0xd7f3, 0xd7f4, 0xd7f5, 0xd7f6, 0xd7f7,
                0xd7f8, 0xd7f9, 0xd7fa, 0xd7fb
            };
            const BYTE *b = (const BYTE *)(ptr + 2 * i);
            WCHAR wc = i < 0x100 ? 0x1100 + i : hangul_chars[i - 0x100];
            printf( "%04x: %02x %02x %02x %02x %02x\n", wc, b[0], b[1], b[2], b[3], b[4] );
        }

        printf( "\nExceptions:\n" );
        for (i = 0; i < guid_count; i++)
        {
            if (!guids[i].except) continue;
            printf( "\n  %s\n", get_guid_str( &guids[i].id ));
            dump_exceptions( sortkeys, guids[i].except );
            if (!guids[i].ling_except) continue;
            printf( "\n  %s  LINGUISTIC_CASING\n", get_guid_str( &guids[i].id ));
            dump_exceptions( sortkeys, guids[i].ling_except );
        }
    }
    printf( "\n" );
}

void nls_dump(void)
{
    const char *name = strrchr( globals.input_name, '/' );
    if (name) name++;
    else name = globals.input_name;
    if (!strcasecmp( name, "l_intl.nls" )) return dump_casemap();
    if (!strncasecmp( name, "c_", 2 )) return dump_codepage();
    if (!strncasecmp( name, "norm", 4 )) return dump_norm();
    if (!strcasecmp( name, "sortdefault.nls" )) return dump_sort( 0 );
    if (!strncasecmp( name, "sort", 4 )) return dump_sort( 1 );
    fprintf( stderr, "Unrecognized file name '%s'\n", globals.input_name );
}

enum FileSig get_kind_nls(void)
{
    if (strlen( globals.input_name ) < 5) return SIG_UNKNOWN;
    if (strcasecmp( globals.input_name + strlen(globals.input_name) - 4, ".nls" )) return SIG_UNKNOWN;
    return SIG_NLS;
}
