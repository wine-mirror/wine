/*
 * DOS interrupt 20h handler (TERMINATE PROGRAM)
 */

#include <stdlib.h>
#include "winbase.h"
#include "miscemu.h"
/* #define DEBUG_INT */
#include "debugtools.h"
#include "callback.h"

/**********************************************************************
 *	    INT_Int20Handler
 *
 * Handler for int 20h.
 */
void WINAPI INT_Int20Handler( CONTEXT86 *context )
{
        if (Dosvm.Exit) Dosvm.Exit( context, TRUE, 0 );
        else ExitThread( 0 );
}
