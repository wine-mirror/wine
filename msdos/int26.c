/*
 * DOS interrupt 26h handler
 */

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "msdos.h"
#include "ldt.h"
#include "miscemu.h"
#include "drive.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int)

/**********************************************************************
 *	    INT_Int26Handler
 *
 * Handler for int 26h (absolute disk read).
 */
void WINAPI INT_Int26Handler( CONTEXT86 *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN( context, context->SegDs, context->Ebx );
    DWORD begin, length;

    if (!DRIVE_IsValid(LOBYTE(context->Eax)))
    {
        SET_CFLAG(context);
        AX_reg(context) = 0x0201;        /* unknown unit */
        return;
    }

    if (LOWORD(context->Ecx) == 0xffff)
    {
        begin   = *(DWORD *)dataptr;
        length  = *(WORD *)(dataptr + 4);
        dataptr = (BYTE *)CTX_SEG_OFF_TO_LIN( context,
                                        *(WORD *)(dataptr + 8), *(DWORD *)(dataptr + 6) );
    }
    else
    {
        begin  = LOWORD(context->Edx);
        length = LOWORD(context->Ecx);
    }
		
    TRACE("int26: abs diskwrite, drive %d, sector %ld, "
                 "count %ld, buffer %p\n",
                 AL_reg(context), begin, length, dataptr );

	DRIVE_RawWrite(LOBYTE(context->Eax), begin, length, dataptr, TRUE);
    RESET_CFLAG(context);
}
