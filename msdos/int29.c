/*
 * DOS interrupt 29h handler
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ldt.h"
#include "drive.h"
#include "msdos.h"
#include "miscemu.h"
#include "module.h"

/**********************************************************************
 *	    INT_Int29Handler
 *
 * Handler for int 29h (fast console output)
 */
void WINAPI INT_Int29Handler( CONTEXT *context )
{
   /* Yes, it seems that this is really all this interrupt does. */
   _lwrite16( 1, &AL_reg(context), 1);
}

