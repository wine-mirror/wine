/*
 * DOS interrupt 41h handler  -- Windows Kernel Debugger
 *
 * Check debugsys.inc from the DDK for docu.
 *
 * Copyright 1998 Ulrich Weigand
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

#include <stdio.h>
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/***********************************************************************
 *           DOSVM_Int41Handler (WINEDOS16.165)
 *
 */
void WINAPI DOSVM_Int41Handler( CONTEXT86 *context )
{
    if ( ISV86(context) )
    {
        /* Real-mode debugger services */
        switch ( AX_reg(context) )
        {
        default:
            INT_BARF( context, 0x41 );
            break;
        }
    }
    else
    {
        /* Protected-mode debugger services */
        switch ( AX_reg(context) )
        {
        case 0x4f:
        case 0x50:
        case 0x150:
        case 0x51:
        case 0x52:
        case 0x152:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
            /* Notifies the debugger of a lot of stuff. We simply ignore it
               for now, but some of the info might actually be useful ... */
            break;

        default:
            INT_BARF( context, 0x41 );
            break;
        }
    }
}
