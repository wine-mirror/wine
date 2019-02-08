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

extern unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen ) DECLSPEC_HIDDEN;

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
    return dst_pos;
}
