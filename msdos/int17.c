/*
 * BIOS interrupt 17h handler
 */

#include <stdlib.h>

#include "miscemu.h"
#include "debugtools.h"
#include "msdos.h"
#include "drive.h"
#include "winnt.h"

DEFAULT_DEBUG_CHANNEL(int17)

/**********************************************************************
 *          INT_Int17Handler
 *
 * Handler for int 17h (printer - output character).
 */
void WINAPI INT_Int17Handler( CONTEXT86 *context )
{
    switch( AH_reg(context) )
    {
	case 0x01:		/* PRINTER - INITIALIZE */
	    FIXME("Initialize Printer - Not Supported\n");
	    AH_reg(context) = 0; /* time out */
            break;
	case 0x02:		/* PRINTER - GET STATUS */
	    FIXME("Get Printer Status - Not Supported\n");
            break;
	default:
	    AH_reg(context) = 0; /* time out */
	    INT_BARF( context, 0x17 );
    }
}
