/*
 * ffs function
 *
 * Copyright 2004 Hans Leidekker
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

#ifndef HAVE_FFS
int ffs( int x )
{
    unsigned int y = (unsigned int)x;

    if (y & 0x00000001) return 1;
    if (y & 0x00000002) return 2;
    if (y & 0x00000004) return 3;
    if (y & 0x00000008) return 4;
    if (y & 0x00000010) return 5;
    if (y & 0x00000020) return 6;
    if (y & 0x00000040) return 7;
    if (y & 0x00000080) return 8;
    if (y & 0x00000100) return 9;
    if (y & 0x00000200) return 10;
    if (y & 0x00000400) return 11;
    if (y & 0x00000800) return 12;
    if (y & 0x00001000) return 13;
    if (y & 0x00002000) return 14;
    if (y & 0x00004000) return 15;
    if (y & 0x00008000) return 16;
    if (y & 0x00010000) return 17;
    if (y & 0x00020000) return 18;
    if (y & 0x00040000) return 19;
    if (y & 0x00080000) return 20;
    if (y & 0x00100000) return 21;
    if (y & 0x00200000) return 22;
    if (y & 0x00400000) return 23;
    if (y & 0x00800000) return 24;
    if (y & 0x01000000) return 25;
    if (y & 0x02000000) return 26;
    if (y & 0x04000000) return 27;
    if (y & 0x08000000) return 28;
    if (y & 0x10000000) return 29;
    if (y & 0x20000000) return 30;
    if (y & 0x40000000) return 31;
    if (y & 0x80000000) return 32;

    return 0;
}
#endif /* HAVE_FFS */
