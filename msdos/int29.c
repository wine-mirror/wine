/*
 * DOS interrupt 29h handler
 */

#include "config.h"
#include "windef.h"
#include "winnt.h"
#include "console.h"

/**********************************************************************
 *	    INT_Int29Handler
 *
 * Handler for int 29h (fast console output)
 */
void WINAPI INT_Int29Handler( CONTEXT *context )
{
   /* Yes, it seems that this is really all this interrupt does. */
   CONSOLE_Write(AL_reg(context), 0, 0, 0);
}

