/*
 * BIOS interrupt 17h handler
 *
 * Copyright 1998 Carl van Schaik
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

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/**********************************************************************
 *          DOSVM_Int17Handler (WINEDOS16.123)
 *
 * Handler for int 17h (printer - output character).
 */
void WINAPI DOSVM_Int17Handler( CONTEXT86 *context )
{
    switch( AH_reg(context) )
    {
       case 0x00:/* Send character*/
	    FIXME("Send character not supported yet\n");
	    SET_AH( context, 0x00 );/*Timeout*/
	    break;
	case 0x01:		/* PRINTER - INITIALIZE */
	    FIXME("Initialize Printer - Not Supported\n");
	    SET_AH( context, 0x30 ); /* selected | out of paper */
            break;
	case 0x02:		/* PRINTER - GET STATUS */
	    FIXME("Get Printer Status - Not Supported\n");
            break;
	default:
	    SET_AH( context, 0 ); /* time out */
	    INT_BARF( context, 0x17 );
    }
}
