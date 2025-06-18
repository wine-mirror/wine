/*
 *  Copyright 2006 Mike McCormack
 *
 *  based on arc4.cpp - written and placed in the public domain by Wei Dai
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* http://cryptopp.sourceforge.net/docs/ref521/arc4_8cpp-source.html */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

struct ustring
{
    DWORD Length;
    DWORD MaximumLength;
    unsigned char *Buffer;
};

typedef struct tag_arc4_info {
    unsigned char state[256];
    unsigned char x, y;
} arc4_info;

static void arc4_init(arc4_info *a4i, const BYTE *key, unsigned int keyLen)
{
    unsigned int keyIndex = 0, stateIndex = 0;
    unsigned int i, a;

    a4i->x = a4i->y = 0;

    for (i=0; i<256; i++)
        a4i->state[i] = i;

    for (i=0; i<256; i++)
    {
        a = a4i->state[i];
        stateIndex += key[keyIndex] + a;
        stateIndex &= 0xff;
        a4i->state[i] = a4i->state[stateIndex];
        a4i->state[stateIndex] = a;
        if (++keyIndex >= keyLen)
            keyIndex = 0;
    }
}

static void arc4_ProcessString(arc4_info *a4i, BYTE *inoutString, unsigned int length)
{
    BYTE *const s=a4i->state;
    unsigned int x = a4i->x;
    unsigned int y = a4i->y;
    unsigned int a, b;

    while(length--)
    {
        x = (x+1) & 0xff;
        a = s[x];
        y = (y+a) & 0xff;
        b = s[y];
        s[x] = b;
        s[y] = a;
        *inoutString++ ^= s[(a+b) & 0xff];
    }

    a4i->x = x;
    a4i->y = y;
}

/******************************************************************************
 *     SystemFunction032   (cryptsp.@)
 */
NTSTATUS WINAPI SystemFunction032(struct ustring *data, const struct ustring *key)
{
    arc4_info a4i;

    arc4_init(&a4i, key->Buffer, key->Length);
    arc4_ProcessString(&a4i, data->Buffer, data->Length);

    return STATUS_SUCCESS;
}
