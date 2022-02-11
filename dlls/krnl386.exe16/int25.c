/*
 * DOS interrupt 25h handler
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
#include <string.h>
#include <fcntl.h>

#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/***********************************************************************
 *           DOSVM_RawRead
 *
 * Read raw sectors from a device.
 */
BOOL DOSVM_RawRead(BYTE drive, DWORD begin, DWORD nr_sect, BYTE *dataptr, BOOL fake_success)
{    
    WCHAR root[] = {'\\','\\','.','\\','A',':',0};
    HANDLE h;

    TRACE( "abs diskread, drive %d, sector %ld, "
           "count %ld, buffer %p\n",
           drive, begin, nr_sect, dataptr );

    root[4] += drive;
    h = CreateFileW(root, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        DWORD r;
        SetFilePointer(h, begin * 512, NULL, SEEK_SET );
        /* FIXME: check errors */
        ReadFile(h, dataptr, nr_sect * 512, &r, NULL );
        CloseHandle(h);
    }
    else
    {
        memset( dataptr, 0, nr_sect * 512 );
        if (fake_success)
        {
            /* FIXME: explain what happens here */
            if (begin == 0 && nr_sect > 1) *(dataptr + 512) = 0xf8;
            if (begin == 1) *dataptr = 0xf8;
        }
        else
            return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *	    DOSVM_Int25Handler
 *
 * Handler for int 25h (absolute disk read).
 */
void WINAPI DOSVM_Int25Handler( CONTEXT *context )
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

    DOSVM_RawRead( AL_reg( context ), begin, length, dataptr, TRUE );
    RESET_CFLAG( context );
}
