/*
 * Int 4B handling
 *
 */

#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"
#include "registers.h"


/***********************************************************************
 *           INT_Int4bHandler
 *
 */
void INT_Int4bHandler( struct sigcontext_struct context )
{

  switch(AH_reg(&context))
   {
	case 0x81: /* Virtual DMA Spec (IBM SCSI interface) */   
             if(AL_reg(&context) != 0x02) /* if not install check */
		{
		 SET_CFLAG(&context);		 
		 AL_reg(&context) = 0x0f; /* function is not implemented */
		}
	     break;
        default:
	     INT_BARF(&context, 0x4b);
    }
}

