#include <stdio.h>
#include <stdlib.h>
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"


/**********************************************************************
 *	    INT_Int10Handler
 *
 * Handler for int 10h (video).
 */
void INT_Int10Handler( SIGCONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x0f:
        AL_reg(context) = 0x5b;
        break;

    case 0x12:
        if (BL_reg(context) == 0x10)
        {
            BX_reg(context) = 0x0003;
            CX_reg(context) = 0x0009;
        }
        break;
			
    case 0x1a:
        BX_reg(context) = 0x0008;
        break;

    default:
        INT_BARF( context, 0x10 );
    }
}
