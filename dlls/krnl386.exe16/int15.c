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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include "dosexe.h"
#include "wine/debug.h"
#include "wine/winbase16.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/**********************************************************************
 *	    DOSVM_Int15Handler
 *
 * Handler for int 15h
 */
void WINAPI DOSVM_Int15Handler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x4f: /*catch keyboard*/
        FIXME("INT15: intercept keyboard not handled yet\n");
        break;
    case 0x83: /* start timer*/
        switch(AL_reg(context))
        {
        case 0x00: /* Start Timer*/
            FIXME("INT15: Start Timer not handled yet\n");
            break;
        case 0x01: /* stop  timer*/
            FIXME("INT15: Stop Timer not handled yet\n");
            break;
        }
        break;
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
    case 0x85: /* sysreq - key  used*/
        FIXME("INT15: SysReq - Key not handled yet\n");
        break;
    case 0x86: /* wait*/
        FIXME("INT15: Wait not correctly handled yet\n");
        if ( AL_reg( context ) != 0x00 ) ERR("Invalid Input to Int15 function 0x86h AL != 0x00\n");
        break;
    case 0x87: /* move memory regions*/
        FIXME("INT15: Move memory regions not implemented\n");
        break;

    case 0x88: /* get size of memory above 1 M */
        SET_AX( context, 64 );  /* FIXME: are 64K ok? */
        RESET_CFLAG(context);
        break;
    case 0x89: /*  Switch to protected mode*/
        FIXME("INT15: switching to protected mode not supported\n");
        break;
    case 0x90:/* OS hook  - Device busy*/
        FIXME("INT15: OS hook - device busy\n");
        break;
    case 0x91: /* OS hook -  Device post*/
        FIXME("INT15: OS hook - device post\n");
        break;

    case 0xc0: /* GET CONFIGURATION */
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
    case 0xc3: /* set carry flag, so BorlandRTM doesn't  assume a Vectra/PS2*/
	FIXME("INT15: 0xc3\n");
	SET_AH( context , 0x86 );
	break;
    case 0xc4: /*  BIOS POS Program option select  */
	FIXME("INT15: option 0xc4 not handled!\n");
	break;

    default:
        INT_BARF( context, 0x15 );
    }
}
