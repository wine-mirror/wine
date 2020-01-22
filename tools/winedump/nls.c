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

void nls_dump(void)
{
    const char *name = strrchr( globals.input_name, '/' );
    if (name) name++;
    else name = globals.input_name;
    if (!strcasecmp( name, "l_intl.nls" )) return dump_casemap();
    if (!strncasecmp( name, "c_", 2 )) return dump_codepage();
    fprintf( stderr, "Unrecognized file name '%s'\n", globals.input_name );
}

enum FileSig get_kind_nls(void)
{
    if (strlen( globals.input_name ) < 5) return SIG_UNKNOWN;
    if (strcasecmp( globals.input_name + strlen(globals.input_name) - 4, ".nls" )) return SIG_UNKNOWN;
    return SIG_NLS;
}
