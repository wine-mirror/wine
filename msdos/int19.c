/*
 * BIOS interrupt 19h handler
 */

#include <stdlib.h>
#include "miscemu.h"
#include "debug.h"


/**********************************************************************
 *          INT_Int19Handler
 *
 * Handler for int 19h (Reboot).
 */
void WINAPI INT_Int19Handler( CONTEXT *context )
{
    WARN(int19, "Attempted Reboot\n");
}
