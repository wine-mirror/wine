#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"

int do_int13(struct sigcontext_struct *context)
{
	switch(AH) {
	case 0x00:                            /* RESET DISK SYSTEM     */
	case 0x04:                            /* VERIFY DISK SECTOR(S) */
		AH = 0;
		break;
	       
	case 0x05:                                     /* FORMAT TRACK */
		AH = 0x0c;
		SetCflag;
		break;

	case 0x06:             /* FORMAT TRACK AND SET BAD SECTOR FLAGS */
	case 0x07:             /* FORMAT DRIVE STARTING AT GIVEN TRACK  */ 
		AH = 0x0c;
		break;

	case 0x08:                              /* GET DRIVE PARAMETERS  */
		AH = (DL & 0x80) ? 0x07 : 0x01;
		SetCflag;
		break;

        case 0x09:         /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
	case 0x0c:         /* SEEK TO CYLINDER                            */
	case 0x0d:         /* RESET HARD DISKS                            */
	case 0x10:         /* CHECK IF DRIVE READY                        */
	case 0x11:         /* RECALIBRATE DRIVE                           */
	case 0x14:         /* CONTROLLER INTERNAL DIAGNOSTIC              */
		AH = 0;
		break;

	case 0x0e:                    /* READ SECTOR BUFFER (XT only)      */
	case 0x0f:                    /* WRITE SECTOR BUFFER (XT only)     */
        case 0x12:                    /* CONTROLLER RAM DIAGNOSTIC (XT,PS) */
	case 0x13:                    /* DRIVE DIAGNOSTIC (XT,PS)          */
		AH = 0x01;
		SetCflag;
		break;

	default:
		IntBarf(0x13, context);
	};
	return 1;
}
