/*
 * DOS interrupt 4bh handler
 */

#include "miscemu.h"

/***********************************************************************
 *           INT_Int4bHandler
 *
 */
void WINAPI INT_Int4bHandler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x81: /* Virtual DMA Spec (IBM SCSI interface) */   
        if(AL_reg(context) != 0x02) /* if not install check */
        {
            SET_CFLAG(context);		 
            AL_reg(context) = 0x0f; /* function is not implemented */
        }
        break;
    default:
        INT_BARF(context, 0x4b);
    }
}
