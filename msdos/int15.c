/*
 * BIOS interrupt 15h handler
 */

#include <stdio.h>
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

    default:
        INT_BARF( context, 0x15 );
    }
}
