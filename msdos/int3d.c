/*
 * DOS interrupt 3d handler.
 * Copyright 1997 Len White
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include "msdos.h"
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/**********************************************************************
 *          INT_Int3dHandler (WPROCS.161)
 *
 * Handler for int 3d (FLOATING POINT EMULATION - STANDALONE FWAIT).
 */
void WINAPI INT_Int3dHandler(CONTEXT86 *context)
{
    switch(AH_reg(context))
    {
    case 0x00:
        break;

    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0xb:
        AH_reg(context) = 0;
        break;

    default:
        INT_BARF( context, 0x3d );
    }
}

