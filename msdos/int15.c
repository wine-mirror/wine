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

WINE_DEFAULT_DEBUG_CHANNEL(int);


/**********************************************************************
 *	    INT_Int15Handler (WPROCS.121)
 *
 * Handler for int 15h
 */
void WINAPI INT_Int15Handler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x84: /* read joystick information */
        FIXME("Read joystick information not implemented\n");

        /* FIXME: report status as if no game port exists */
        switch(DX_reg(context))
        {
        case 0x0: /* read joystick switches */
            AL_reg(context) = 0x0; /* all switches open */
            break;
        case 0x1: /* read joystick position */
            AX_reg(context) = 0x0;
            BX_reg(context) = 0x0;
            CX_reg(context) = 0x0;
            DX_reg(context) = 0x0;
            break;
	default:
            INT_BARF( context, 0x15 );
            break;
        }

        RESET_CFLAG(context);

        break;

    case 0x88: /* get size of memory above 1 M */
        AX_reg(context) = 64;  /* FIXME: are 64K ok? */
        RESET_CFLAG(context);
        break;

    case 0xc0: /* GET CONFIGURATION */
        if (ISV86(context)) /* real */
            context->SegEs = 0xf000;
        else
            context->SegEs = DOSMEM_BiosSysSeg;
        BX_reg(context) = 0xe6f5;
        AH_reg(context) = 0x0;
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
	    AH_reg(context) = 0x00; /* successful */
	    break;
	case 0x02: /* Set Sampling Rate */
	    /* BH = sampling rate */
	    FIXME("Set Sampling Rate - not implemented\n");
	    AH_reg(context) = 0x00; /* successful */
	    break;
	case 0x04: /* Get Pointing Device Type */
	    FIXME("Get Pointing Device Type - not implemented\n");
	    BH_reg(context) = 0x01;/*Device id FIXME what is it supposed to be?*/
	    break;
	default:
	    INT_BARF( context, 0x15 );
	}
        break;

    default:
        INT_BARF( context, 0x15 );
    }
}
