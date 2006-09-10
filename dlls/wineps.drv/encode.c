/*
 *	PostScript filter functions
 *
 *	Copyright 2003  Huw D M Davies
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "psdrv.h"

DWORD RLE_encode(BYTE *in_buf, DWORD len, BYTE *out_buf)
{
    BYTE *next_in = in_buf, *next_out = out_buf;
    DWORD rl;

    while(next_in < in_buf + len) { 
        if(next_in + 1 < in_buf + len) {
            if(*next_in == *(next_in + 1)) {
                next_in += 2;
                rl = 2;
                while(next_in < in_buf + len && *next_in == *(next_in - 1) && rl < 128) {
                    next_in++;
                    rl++;
                }
                *next_out++ = 257 - rl;
                *next_out++ = *(next_in - 1);
            } else {
                rl = 0;
                next_out++;
                while(next_in < in_buf + len && rl < 128) {
                    if(next_in + 2 < in_buf + len && *next_in == *(next_in + 1)) {
                        if(rl == 127 || *next_in == *(next_in + 2))
                            break;
                    }
                    *next_out++ = *next_in++;
                    rl++;
                }
                *(next_out - rl - 1) = rl - 1;
            }
        } else { /* special case: begin of run on last byte */
            *next_out++ = 0;
            *next_out++ = *next_in;
            break;
        }
    }
    *next_out++ = 128;
    return next_out - out_buf;
}

DWORD ASCII85_encode(BYTE *in_buf, DWORD len, BYTE *out_buf)
{
    DWORD number;
    BYTE *next_in = in_buf, *next_out = out_buf;
    int i;

    while(next_in + 3 < in_buf + len) {
        number = (next_in[0] << 24) |
                 (next_in[1] << 16) |
                 (next_in[2] <<  8) |
                 (next_in[3]      );
        next_in += 4;
        if(number == 0)
            *next_out++ = 'z';
        else {
            for(i = 4; i >= 0; i--) {
                next_out[i] = number % 85 + '!';
                number /= 85;
            }
            next_out += 5;
        }
    }

    if(next_in != in_buf + len) {
        number = (*next_in++ << 24) & 0xff000000;
        if(next_in < in_buf + len)
            number |= ((*next_in++ << 16) & 0x00ff0000);
        if(next_in < in_buf + len)
            number |= ((*next_in++ <<  8) & 0x0000ff00);

        for(i = len % 4 + 1; i < 5; i++)
            number /= 85;

        for(i = len % 4; i >= 0; i--) {
            next_out[i] = number % 85 + '!';
            number /= 85;
        }
        next_out += len % 4 + 1;
    }

    return next_out - out_buf;    
}
