/*
 * DOS interrupt 26h handler
 *
 * Copyright 1997 Andreas Mohr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "dosexe.h"
#include "drive.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/***********************************************************************
 *           DOSVM_RawWrite
 *
 * Write raw sectors to a device.
 */
BOOL DOSVM_RawWrite(BYTE drive, DWORD begin, DWORD nr_sect, BYTE *dataptr, BOOL fake_success)
{
    int fd;

    if ((fd = DRIVE_OpenDevice( drive, O_RDONLY )) != -1)
    {
        lseek( fd, begin * 512, SEEK_SET );
        /* FIXME: check errors */
        write( fd, dataptr, nr_sect * 512 );
        close( fd );
    }
    else if (!fake_success)
        return FALSE;

    return TRUE;
}


/**********************************************************************
 *	    DOSVM_Int26Handler (WINEDOS16.138)
 *
 * Handler for int 26h (absolute disk read).
 */
void WINAPI DOSVM_Int26Handler( CONTEXT86 *context )
{
    WCHAR drivespec[4] = {'A', ':', '\\', 0};
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN( context, context->SegDs, context->Ebx );
    DWORD begin;
    DWORD length;

    drivespec[0] += AL_reg( context );

    if (GetDriveTypeW( drivespec ) == DRIVE_NO_ROOT_DIR || 
        GetDriveTypeW( drivespec ) == DRIVE_UNKNOWN)
    {
        SET_CFLAG( context );
        SET_AX( context, 0x0201 ); /* unknown unit */
        return;
    }

    if (CX_reg( context ) == 0xffff)
    {
        begin   = *(DWORD *)dataptr;
        length  = *(WORD *)(dataptr + 4);
        dataptr = (BYTE *)CTX_SEG_OFF_TO_LIN( context,
                                              *(WORD *)(dataptr + 8), 
                                              *(DWORD *)(dataptr + 6) );
    }
    else
    {
        begin  = DX_reg( context );
        length = CX_reg( context );
    }

    TRACE( "abs diskwrite, drive %d, sector %ld, "
           "count %ld, buffer %p\n",
           AL_reg( context ), begin, length, dataptr );

    DOSVM_RawWrite( AL_reg( context ), begin, length, dataptr, TRUE );
    RESET_CFLAG( context );
}
