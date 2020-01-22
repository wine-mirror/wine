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

void nls_dump(void)
{
    const char *name = strrchr( globals.input_name, '/' );
    if (name) name++;
    else name = globals.input_name;
    if (!strcasecmp( name, "l_intl.nls" )) return dump_casemap();
    fprintf( stderr, "Unrecognized file name '%s'\n", globals.input_name );
}

enum FileSig get_kind_nls(void)
{
    if (strlen( globals.input_name ) < 5) return SIG_UNKNOWN;
    if (strcasecmp( globals.input_name + strlen(globals.input_name) - 4, ".nls" )) return SIG_UNKNOWN;
    return SIG_NLS;
}
