/*
 * BIOS interrupt 11h handler
 */

#include <stdio.h>
#include <stdlib.h>
#include "miscemu.h"
#include "msdos.h"
#include "drive.h"
#include "stddebug.h"
#include "debug.h"

/**********************************************************************
 *	    INT_Int11Handler
 *
 * Handler for int 11h (get equipment list).
 */
void INT_Int11Handler( CONTEXT *context )
{
    int diskdrives = 0;
    int parallelports = 0;
    int serialports = 0;
    int x;

/* borrowed from Ralph Brown's interrupt lists 

		    bits 15-14: number of parallel devices
		    bit     13: [Conv] Internal modem
		    bit     12: reserved
		    bits 11- 9: number of serial devices
		    bit      8: reserved
		    bits  7- 6: number of diskette drives minus one
		    bits  5- 4: Initial video mode:
				    00b = EGA,VGA,PGA
				    01b = 40 x 25 color
				    10b = 80 x 25 color
				    11b = 80 x 25 mono
		    bit      3: reserved
		    bit      2: [PS] =1 if pointing device
				[non-PS] reserved
		    bit      1: =1 if math co-processor
		    bit      0: =1 if diskette available for boot
*/
/*  Currently the only of these bits correctly set are:
		bits 15-14 		} Added by William Owen Smith, 
		bits 11-9		} wos@dcs.warwick.ac.uk
		bits 7-6
		bit  2			(always set)
*/

    if (DRIVE_IsValid(0)) diskdrives++;
    if (DRIVE_IsValid(1)) diskdrives++;
    if (diskdrives) diskdrives--;
	
    for (x=0; x!=MAX_PORTS; x++)
    {
	if (COM[x].devicename)
	    serialports++;
	if (LPT[x].devicename)
	    parallelports++;
    }
    if (serialports > 7)		/* 3 bits -- maximum value = 7 */
        serialports=7;
    if (parallelports > 3)		/* 2 bits -- maximum value = 3 */
        parallelports=3;
    
    AX_reg(context) = (diskdrives << 6) | (serialports << 9) | 
                      (parallelports << 14) | 0x02;
}
