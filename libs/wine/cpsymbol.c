/*
 * CP_SYMBOL support
 *
 * Copyright 2000 Alexandre Julliard
 * Copyright 2004 Rein Klazes
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

#include "wine/asm.h"

#ifdef __ASM_OBSOLETE

#include "wine/unicode.h"

/* return -1 on dst buffer overflow */
int wine_cpsymbol_mbstowcs_obsolete( const char *src, int srclen, WCHAR *dst, int dstlen)
{
    int len, i;

    if (dstlen == 0) return srclen;
    len = dstlen > srclen ? srclen : dstlen;
    for (i = 0; i < len; i++)
    {
        unsigned char c = src[i];
        dst[i] = (c < 0x20) ? c : c + 0xf000;
    }
    if (srclen > len) return -1;
    return len;
}

/* return -1 on dst buffer overflow, -2 on invalid character */
int wine_cpsymbol_wcstombs_obsolete( const WCHAR *src, int srclen, char *dst, int dstlen)
{
    int len, i;

    if (dstlen == 0) return srclen;
    len = dstlen > srclen ? srclen : dstlen;
    for (i = 0; i < len; i++)
    {
        if (src[i] < 0x20)
            dst[i] = src[i];
        else if (src[i] >= 0xf020 && src[i] < 0xf100)
            dst[i] = src[i] - 0xf000;
        else
            return -2;
    }
    if (srclen > len) return -1;
    return len;
}

__ASM_OBSOLETE(wine_cpsymbol_mbstowcs);
__ASM_OBSOLETE(wine_cpsymbol_wcstombs);

#endif /* __ASM_OBSOLETE */
