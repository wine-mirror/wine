/*
 * BIOS interrupt 17h handler
 */

#include <stdlib.h>

#include "miscemu.h"
#include "debug.h"
#include "msdos.h"
#include "drive.h"
#include "winnt.h"

/**********************************************************************
 *          INT_Int17Handler
 *
 * Handler for int 17h (printer - output character).
 */
void WINAPI INT_Int17Handler( CONTEXT *context )
{
    switch( AH_reg(context) )
    {
	case 0x01:		/* PRINTER - INITIALIZE */
	    FIXME(int17, "Initialize Printer - Not Supported\n");
	    AH_reg(context) = 0; /* time out */
            break;
	case 0x02:		/* PRINTER - GET STATUS */
	    FIXME(int17, "Get Printer Status - Not Supported\n");
            break;
	default:
	    AH_reg(context) = 0; /* time out */
	    INT_BARF( context, 0x17 );
    }
}
