/*
 * BIOS interrupt 15h handler
 *
 * Copyright 1997 Jan Willamowius
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
#include "miscemu.h"
#include "wine/debug.h"
#include "wine/winbase16.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/**********************************************************************
 *	    DOSVM_Int15Handler (WINEDOS16.121)
 *
 * Handler for int 15h
 */
void WINAPI DOSVM_Int15Handler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x84: /* read joystick information */
        FIXME("Read joystick information not implemented\n");

        /* FIXME: report status as if no game port exists */
        switch(DX_reg(context))
        {
        case 0x0: /* read joystick switches */
            SET_AL( context, 0x0 ); /* all switches open */
            break;
        case 0x1: /* read joystick position */
            SET_AX( context, 0x0 );
            SET_BX( context, 0x0 );
            SET_CX( context, 0x0 );
            SET_DX( context, 0x0 );
            break;
	default:
            INT_BARF( context, 0x15 );
            break;
        }

        RESET_CFLAG(context);
        break;

    case 0x88: /* get size of memory above 1 M */
        SET_AX( context, 64 );  /* FIXME: are 64K ok? */
        RESET_CFLAG(context);
        break;

    case 0xc0: /* GET CONFIGURATION */
        if (ISV86(context))
        {
            /* real mode segment */
            context->SegEs = 0xf000;
        }
        else
        {
            /* KERNEL.194: __F000H - protected mode selector */
            FARPROC16 proc = GetProcAddress16( GetModuleHandle16("KERNEL"),
                                               (LPCSTR)(ULONG_PTR)194 );
            context->SegEs = LOWORD(proc);
        }
        SET_BX( context, 0xe6f5 );
        SET_AH( context, 0x0 );
        RESET_CFLAG(context);
        break;

    case 0xc2:
	switch(AL_reg(context))
	{
	case 0x00: /* Enable-Disable Pointing Device (mouse) */
	    /* BH = newstate, 00h = disabled 01h = enabled */
	    switch(BH_reg(context))
	    {
	        case 0x00:
	    	    FIXME("Disable Pointing Device - not implemented\n");
		    break;
	    	case 0x01:
	    	    FIXME("Enable Pointing Device - not implemented\n");
		    break;
	    	default:
		    INT_BARF( context, 0x15 );
		    break;
	    }
	    SET_AH( context, 0x00 ); /* successful */
	    break;
	case 0x02: /* Set Sampling Rate */
	    /* BH = sampling rate */
	    FIXME("Set Sampling Rate - not implemented\n");
	    SET_AH( context, 0x00 ); /* successful */
	    break;
	case 0x04: /* Get Pointing Device Type */
	    FIXME("Get Pointing Device Type - not implemented\n");
            /* FIXME: BH = Device id, What is it supposed to be? */
	    SET_BH( context, 0x01 );
	    break;
	default:
	    INT_BARF( context, 0x15 );
	}
        break;

    default:
        INT_BARF( context, 0x15 );
    }
}
