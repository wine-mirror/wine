#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"


/**********************************************************************
 *	    INT_Int13Handler
 *
 * Handler for int 13h (disk I/O).
 */
void INT_Int13Handler( struct sigcontext_struct context )
{
    switch(AH_reg(&context))
    {
	case 0x00:                            /* RESET DISK SYSTEM     */
	case 0x04:                            /* VERIFY DISK SECTOR(S) */
		AH_reg(&context) = 0;
		break;
	       
	case 0x05:                                     /* FORMAT TRACK */
		AH_reg(&context) = 0x0c;
                SET_CFLAG(&context);
		break;

	case 0x06:             /* FORMAT TRACK AND SET BAD SECTOR FLAGS */
	case 0x07:             /* FORMAT DRIVE STARTING AT GIVEN TRACK  */ 
		AH_reg(&context) = 0x0c;
		break;

	case 0x08:                              /* GET DRIVE PARAMETERS  */
		AH_reg(&context) = (DL_reg(&context) & 0x80) ? 0x07 : 0x01;
                SET_CFLAG(&context);
		break;

        case 0x09:         /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
	case 0x0c:         /* SEEK TO CYLINDER                            */
	case 0x0d:         /* RESET HARD DISKS                            */
	case 0x10:         /* CHECK IF DRIVE READY                        */
	case 0x11:         /* RECALIBRATE DRIVE                           */
	case 0x14:         /* CONTROLLER INTERNAL DIAGNOSTIC              */
		AH_reg(&context) = 0;
		break;

	case 0x0e:                    /* READ SECTOR BUFFER (XT only)      */
	case 0x0f:                    /* WRITE SECTOR BUFFER (XT only)     */
        case 0x12:                    /* CONTROLLER RAM DIAGNOSTIC (XT,PS) */
	case 0x13:                    /* DRIVE DIAGNOSTIC (XT,PS)          */
		AH_reg(&context) = 0x01;
                SET_CFLAG(&context);
		break;

	default:
		INT_BARF( &context, 0x13 );
    }
}
