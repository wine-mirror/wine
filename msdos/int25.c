/*
 * DOS interrupt 25h handler
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "msdos.h"
#include "ldt.h"
#include "miscemu.h"
#include "drive.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(int)


/**********************************************************************
 *	    INT_Int25Handler
 *
 * Handler for int 25h (absolute disk read).
 */
void WINAPI INT_Int25Handler( CONTEXT *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN( context, DS_reg(context), EBX_reg(context) );
    DWORD begin, length;

    if (!DRIVE_IsValid(AL_reg(context)))
    {
        SET_CFLAG(context);
        AX_reg(context) = 0x0201;        /* unknown unit */
        return;
    }

    if (CX_reg(context) == 0xffff)
    {
        begin   = *(DWORD *)dataptr;
        length  = *(WORD *)(dataptr + 4);
        dataptr = (BYTE *)CTX_SEG_OFF_TO_LIN( context,
					*(WORD *)(dataptr + 8), *(DWORD *)(dataptr + 6) );
    }
    else
    {
        begin  = DX_reg(context);
        length = CX_reg(context);
    }
    TRACE(int, "int25: abs diskread, drive %d, sector %ld, "
                 "count %ld, buffer %p\n",
                 AL_reg(context), begin, length, dataptr);

	DRIVE_RawRead(AL_reg(context), begin, length, dataptr, TRUE);
    RESET_CFLAG(context);
}

