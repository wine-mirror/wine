/*
 * BIOS interrupt 15h handler
 */

#include <stdlib.h>
#include "miscemu.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int)


/**********************************************************************
 *	    INT_Int15Handler
 *
 * Handler for int 15h (old cassette interrupt).
 */
void WINAPI INT_Int15Handler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x88: /* get size of memory above 1 M */
        AX_reg(context) = 64;  /* FIXME: are 64K ok? */
        RESET_CFLAG(context);
        break;

    case 0xc0: /* GET CONFIGURATION */
        if (ISV86(context)) /* real */
            ES_reg(context) = 0xf000;
        else
            ES_reg(context) = DOSMEM_BiosSysSeg;
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
	    BH_reg(context) = 0x01;/*Device id FIXME what is it suposed to be?*/
	    break;
	default:
	    INT_BARF( context, 0x15 );
	}
        break;

    default:
        INT_BARF( context, 0x15 );
    }
}
