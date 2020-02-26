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

static unsigned short casemap( const unsigned short *table, unsigned int len, unsigned short ch )
{
    unsigned int off = table[ch >> 8] + ((ch >> 4) & 0x0f);
    if (off >= len) return 0;
    off = table[off] + (ch & 0x0f);
    if (off >= len) return 0;
    return ch + table[off];
}

static void dump_casemap(void)
{
    int i, ch;
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
    for (i = 0; i < 0x10000; i++)
    {
        if (!(i % 16)) printf( "\n%04x:", i );
        ch = casemap( upper, upper_len, i );
        if (ch == i) printf( " ...." );
        else printf( " %04x", ch );
    }
    printf( "\n\nLower-case table:\n" );
    for (i = 0; i < 0x10000; i++)
    {
        if (!(i % 16)) printf( "\n%04x:", i );
        ch = casemap( lower, lower_len, i );
        if (ch == i) printf( " ...." );
        else printf( " %04x", ch );
    }
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

void nls_dump(void)
{
    const char *name = strrchr( globals.input_name, '/' );
    if (name) name++;
    else name = globals.input_name;
    if (!strcasecmp( name, "l_intl.nls" )) return dump_casemap();
    if (!strncasecmp( name, "c_", 2 )) return dump_codepage();
    if (!strncasecmp( name, "norm", 4 )) return dump_norm();
    fprintf( stderr, "Unrecognized file name '%s'\n", globals.input_name );
}

enum FileSig get_kind_nls(void)
{
    if (strlen( globals.input_name ) < 5) return SIG_UNKNOWN;
    if (strcasecmp( globals.input_name + strlen(globals.input_name) - 4, ".nls" )) return SIG_UNKNOWN;
    return SIG_NLS;
}
