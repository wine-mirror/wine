#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"
#include "registers.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

/**********************************************************************
 *	    INT_Int2aHandler
 *
 * Handler for int 2ah (network).
 */
void INT_Int2aHandler( SIGCONTEXT context )
{
    switch(AH_reg(&context))
    {
    case 0x00:                             /* NETWORK INSTALLATION CHECK */
        break;
		
    default:
	INT_BARF( &context, 0x2a );
    }
}
