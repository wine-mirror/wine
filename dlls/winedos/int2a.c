/*
 * DOS interrupt 2ah handler
 */

#include <stdlib.h>
#include "msdos.h"
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/**********************************************************************
 *         DOSVM_Int2aHandler (WINEDOS16.142)
 *
 * Handler for int 2ah (network).
 */
void WINAPI DOSVM_Int2aHandler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x00:                             /* NETWORK INSTALLATION CHECK */
        break;

    default:
       INT_BARF( context, 0x2a );
    }
}
