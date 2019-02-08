/*
 * Unicode normalization functions
 *
 * Copyright 2019 Huw Davies
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

#include "wine/unicode.h"

extern WCHAR wine_compose( const WCHAR *str ) DECLSPEC_HIDDEN;
extern unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen ) DECLSPEC_HIDDEN;
extern const unsigned short combining_class_table[] DECLSPEC_HIDDEN;

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

unsigned int wine_decompose_string( int flags, const WCHAR *src, unsigned int src_len,
                                    WCHAR *dst, unsigned int dst_len )
{
    unsigned int src_pos, dst_pos = 0, decomp_len;

    for (src_pos = 0; src_pos < src_len; src_pos++)
    {
        if (dst_pos == dst_len) return 0;
        decomp_len = wine_decompose( flags, src[src_pos], dst + dst_pos, dst_len - dst_pos );
        if (decomp_len == 0) return 0;
        dst_pos += decomp_len;
    }

    if (flags & WINE_DECOMPOSE_REORDER) canonical_order_string( dst, dst_pos );
    return dst_pos;
}

static BOOL is_blocked( WCHAR *starter, WCHAR *ptr )
{
    if (ptr == starter + 1) return FALSE;
    /* Because the string is already canonically ordered, the chars are blocked
       only if the previous char's combining class is equal to the test char. */
    if (get_combining_class( *(ptr - 1) ) == get_combining_class( *ptr )) return TRUE;
    return FALSE;
}

unsigned int wine_compose_string( WCHAR *str, unsigned int len )
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
