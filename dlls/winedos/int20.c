/*
 * DOS interrupt 20h handler (TERMINATE PROGRAM)
 */

#include <stdlib.h>
#include "winbase.h"
#include "dosexe.h"
#include "miscemu.h"

/**********************************************************************
 *	    DOSVM_Int20Handler
 *
 * Handler for int 20h.
 */
void WINAPI DOSVM_Int20Handler( CONTEXT86 *context )
{
    MZ_Exit( context, TRUE, 0 );
}
