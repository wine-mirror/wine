/*
 * BIOS interrupt 13h handler
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef linux
#include <linux/fd.h>
#endif
#include "miscemu.h"
/* #define DEBUG_INT */
#include "debug.h"
#include "drive.h"

/**********************************************************************
 *	    INT_Int13Handler
 *
 * Handler for int 13h (disk I/O).
 */
void WINAPI INT_Int13Handler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
	case 0x00:                            /* RESET DISK SYSTEM     */
		break; /* no return ? */
	case 0x01:			      /* STATUS OF DISK SYSTEM */
		AL_reg(context) = 0; /* successful completion */
		break;
	case 0x02:			      /* READ SECTORS INTO MEMORY */
		AL_reg(context) = 0; /* number of sectors read */
		AH_reg(context) = 0; /* status */
		break;
	case 0x03:			      /* WRITE SECTORS FROM MEMORY */
		break; /* no return ? */
	case 0x04:                            /* VERIFY DISK SECTOR(S) */
		AL_reg(context) = 0; /* number of sectors verified */
		AH_reg(context) = 0;
		break;
	       
	case 0x05:                                     /* FORMAT TRACK */
        case 0x06:             /* FORMAT TRACK AND SET BAD SECTOR FLAGS */
        case 0x07:             /* FORMAT DRIVE STARTING AT GIVEN TRACK  */
            /* despite what Ralf Brown says, 0x06 and 0x07 seem to
             * set CFLAG, too (at least my BIOS does that) */
            AH_reg(context) = 0x0c;
            SET_CFLAG(context);
            break;

	case 0x08:                              /* GET DRIVE PARAMETERS  */
		if (DL_reg(context) & 0x80) { /* hard disk ? */
			AH_reg(context) = 0x07;
			SET_CFLAG(context);
		}
		else { /* floppy disk */
#ifdef linux
			unsigned int i, nr_of_drives = 0;			
			BYTE drive_nr = DL_reg(context);
                        int floppy_fd;
                        struct floppy_drive_params floppy_parm;

                        AH_reg(context) = 0x00; /* success */

                        for (i = 0; i < MAX_DOS_DRIVES; i++)
                        if (DRIVE_GetType(i) == TYPE_FLOPPY) nr_of_drives++;
                        DL_reg(context) = nr_of_drives;

			if (drive_nr > 1) { /* invalid drive ? */
				BX_reg(context) = 0;
				CX_reg(context) = 0;
				DH_reg(context) = 0;
				break;
			}

                        if ( (floppy_fd = DRIVE_OpenDevice( drive_nr, O_NONBLOCK)) == -1)
                        {
                                WARN(int, "(GET DRIVE PARAMETERS): Can't determine floppy geometry !\n");
                                BX_reg(context) = 0;
                                CX_reg(context) = 0;
                                DH_reg(context) = 0;
                                break;
                        }
                        ioctl(floppy_fd, FDGETDRVPRM, &floppy_parm);
                        close(floppy_fd);

			BL_reg(context) = floppy_parm.cmos;

			/* CH = low eight bits of max cyl
			   CL = max sec nr (bits 5-0),
			   hi two bits of max cyl (bits 7-6)
			   DH = max head nr */ 
			DH_reg(context) = 0x01;
			switch (BL_reg(context))
			{
			    case 0: /* no drive */
				CX_reg(context) = 0x0;
				DX_reg(context) = 0x0;
				break;
			    case 1: /* 360 K */
				CX_reg(context) = 0x2709;
				break;
			    case 2: /* 1.2 M */
				CX_reg(context) = 0x4f0f;
				break;
			    case 3: /* 720 K */
				CX_reg(context) = 0x4f09;
				break;
			    case 4: /* 1.44 M */
				CX_reg(context) = 0x4f12;
				break;
			    case 5:
			    case 6: /* 2.88 M */
				CX_reg(context) = 0x4f24;
				break;
			}
			ES_reg(context) = 0x0000; /* FIXME: drive parameter table */
			DI_reg(context) = 0x0000;
#else
                	AH_reg(context) = 0x01;
                	SET_CFLAG(context);
                	break;
#endif
		}
		break;

        case 0x09:         /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
	case 0x0a:         /* FIXED DISK - READ LONG (XT,AT,XT286,PS)     */
	case 0x0b:	   /* FIXED DISK - WRITE LONG (XT,AT,XT286,PS)    */
	case 0x0c:         /* SEEK TO CYLINDER                            */
	case 0x0d:         /* ALTERNATE RESET HARD DISKS                  */
	case 0x10:         /* CHECK IF DRIVE READY                        */
	case 0x11:         /* RECALIBRATE DRIVE                           */
	case 0x14:         /* CONTROLLER INTERNAL DIAGNOSTIC              */
		AH_reg(context) = 0;
		break;

	case 0x15:	   /* GET DISK TYPE (AT,XT2,XT286,CONV,PS) */
		if (DL_reg(context) & 0x80) { /* hard disk ? */
			AH_reg(context) = 3; /* fixed disk */
			SET_CFLAG(context);
		}
		else { /* floppy disk ? */
			AH_reg(context) = 2; /* floppy with change detection */
			SET_CFLAG(context);
		}
		break;
	case 0x0e:                    /* READ SECTOR BUFFER (XT only)      */
	case 0x0f:                    /* WRITE SECTOR BUFFER (XT only)     */
        case 0x12:                    /* CONTROLLER RAM DIAGNOSTIC (XT,PS) */
	case 0x13:                    /* DRIVE DIAGNOSTIC (XT,PS)          */
		AH_reg(context) = 0x01;
                SET_CFLAG(context);
		break;

	case 0x16:	   /* FLOPPY - CHANGE OF DISK STATUS */
		AH_reg(context) = 0; /* FIXME - no change */
		break;
	case 0x17:	   /* SET DISK TYPE FOR FORMAT */
		if (DL_reg(context) < 4)
		    AH_reg(context) = 0x00; /* successful completion */
		else
		    AH_reg(context) = 0x01; /* error */
		break;
	case 0x18:         /* SET MEDIA TYPE FOR FORMAT */
		if (DL_reg(context) < 4)
		    AH_reg(context) = 0x00; /* successful completion */
		else
		    AH_reg(context) = 0x01; /* error */
		break;
	case 0x19:	   /* FIXED DISK - PARK HEADS */
	default:
		INT_BARF( context, 0x13 );
    }
}
