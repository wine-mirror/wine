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
#include "debug.h"

/**********************************************************************
 *	    INT_Int26Handler
 *
 * Handler for int 26h (absolute disk read).
 */
void WINAPI INT_Int26Handler( CONTEXT *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN( context, DS_reg(context), EBX_reg(context) );
    DWORD begin, length;
    int fd;

    if (!DRIVE_IsValid(AL_reg(context)))
    {
        SET_CFLAG(context);
        AX_reg(context) = 0x0101;        /* unknown unit */
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
		
    TRACE(int,"int26: abs diskwrite, drive %d, sector %ld, "
                 "count %ld, buffer %d\n",
                 AL_reg(context), begin, length, (int) dataptr );

    if ((fd = DRIVE_OpenDevice( AL_reg(context), O_WRONLY )) != -1)
    {
        lseek( fd, begin * 512, SEEK_SET );
        /* FIXME: check errors */
        write( fd, dataptr, length * 512 );
        close( fd );
    }

    RESET_CFLAG(context);
}
