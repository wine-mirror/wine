/*
 * BIOS interrupt 15h handler
 */

#include <stdlib.h>
#include "miscemu.h"
#include "debug.h"


/**********************************************************************
 *	    INT_Int15Handler
 *
 * Handler for int 15h (old cassette interrupt).
 */
void WINAPI INT_Int15Handler( CONTEXT *context )
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

    default:
        INT_BARF( context, 0x15 );
    }
}
