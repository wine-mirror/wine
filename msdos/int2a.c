/*
 * DOS interrupt 2ah handler
 */

#include <stdlib.h>
#include "msdos.h"
#include "miscemu.h"
/* #define DEBUG_INT */
#include "debug.h"

/**********************************************************************
 *	    INT_Int2aHandler
 *
 * Handler for int 2ah (network).
 */
void WINAPI INT_Int2aHandler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x00:                             /* NETWORK INSTALLATION CHECK */
        break;
		
    default:
	INT_BARF( context, 0x2a );
    }
}
