/*
 * DOS interrupt 4bh handler
 */

#include <stdio.h>
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/***********************************************************************
 *           DOSVM_Int4bHandler (WINEDOS16.175)
 *
 */
void WINAPI DOSVM_Int4bHandler( CONTEXT86 *context )
{
    switch(AH_reg(context))
    {
    case 0x81: /* Virtual DMA Spec (IBM SCSI interface) */
        if(AL_reg(context) != 0x02) /* if not install check */
        {
            SET_CFLAG(context);
            SET_AL( context, 0x0f ); /* function is not implemented */
        }
        break;
    default:
        INT_BARF(context, 0x4b);
    }
}
