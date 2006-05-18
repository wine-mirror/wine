/*
 * NetBIOS interrupt handling
 *
 * Copyright 1995 Alexandre Julliard, Alex Korobka
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

#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/***********************************************************************
 *           DOSVM_Int5cHandler (WINEDOS16.192)
 *
 * Called from NetBIOSCall16.
 */
void WINAPI DOSVM_Int5cHandler( CONTEXT86 *context )
{
    BYTE* ptr;
    ptr = MapSL( MAKESEGPTR(context->SegEs,BX_reg(context)) );
    FIXME("(%p): command code %02x (ignored)\n",context, *ptr);
    *(ptr+0x01) = 0xFB; /* NetBIOS emulator not found */
    SET_AL( context, 0xFB );
}
