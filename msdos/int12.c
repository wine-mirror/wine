/*
 * BIOS interrupt 12h handler
 */

#include "miscemu.h"

/**********************************************************************
 *	    INT_Int12Handler
 *
 * Handler for int 12h (get memory size).
 */
void INT_Int12Handler( CONTEXT *context )
{
    AX_reg(context) = 640;
}
