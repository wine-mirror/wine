/*
 * DOS interrupt 20h handler (TERMINATE PROGRAM)
 *
 * Copyright 1997 Andreas Mohr
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

/**********************************************************************
 *	    DOSVM_Int20Handler (WINEDOS16.132)
 *
 * Handler for int 20h.
 */
void WINAPI DOSVM_Int20Handler( CONTEXT86 *context )
{
    if (DOSVM_IsWin16())
        ExitThread( 0 );
    else if(ISV86(context))
        MZ_Exit( context, TRUE, 0 );
    else
        ERR( "Called from DOS protected mode\n" );
}
