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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/***********************************************************************
 *           DOSVM_RawWrite
 *
 * Write raw sectors to a device.
 */
BOOL DOSVM_RawWrite(BYTE drive, DWORD begin, DWORD nr_sect, BYTE *dataptr, BOOL fake_success)
{
    WCHAR root[] = {'\\','\\','.','\\','A',':',0};
    HANDLE h;
    DWORD w;

    TRACE( "abs diskwrite, drive %d, sector %ld, "
           "count %ld, buffer %p\n",
           drive, begin, nr_sect, dataptr );

    root[4] += drive;
    h = CreateFileW(root, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    0, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(h, begin * 512, NULL, SEEK_SET );
        /* FIXME: check errors */
        WriteFile( h, dataptr, nr_sect * 512, &w, NULL );
        CloseHandle( h );
    }
    else if (!fake_success)
        return FALSE;

    return TRUE;
}


/**********************************************************************
 *	    DOSVM_Int26Handler
 *
 * Handler for int 26h (absolute disk write).
 */
void WINAPI DOSVM_Int26Handler( CONTEXT *context )
{
    WCHAR drivespec[] = {'A', ':', '\\', 0};
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

    DOSVM_RawWrite( AL_reg( context ), begin, length, dataptr, TRUE );
    RESET_CFLAG( context );
}
