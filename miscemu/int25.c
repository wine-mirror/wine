#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "registers.h"
#include "msdos.h"
#include "ldt.h"
#include "wine.h"
#include "miscemu.h"
#include "dos_fs.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

/**********************************************************************
 *	    INT_Int25Handler
 *
 * Handler for int 25h (absolute disk read).
 */
void INT_Int25Handler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
	BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS, BX);
	DWORD begin, length;

        if(!DOS_ValidDrive(AL))
        {
            SetCflag;
            AX = 0x0101;        /* unknown unit */
            return;
        }

	if (CX == 0xffff) {
		begin = getdword(dataptr);
		length = getword(&dataptr[4]);
		dataptr = (BYTE *) PTR_SEG_TO_LIN(getdword(&dataptr[6]));
			
	} else {
		begin = DX;
		length = CX;
	}
	dprintf_int(stdnimp, "int25: abs diskread, drive %d, sector %ld, "
	"count %ld, buffer %d\n", AL, begin, length, (int) dataptr);

	memset(dataptr, 0, length * 512);

	if (begin == 0 && length > 1) 
		*(dataptr + 512) = 0xf8;

	if (begin == 1) 
		*dataptr = 0xf8;

	ResetCflag;
#undef context
}
