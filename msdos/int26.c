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
#include "msdos.h"
#include "miscemu.h"
#include "drive.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/**********************************************************************
 *	    INT_Int26Handler (WPROCS.138)
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
        SET_AX( context, 0x0201 );        /* unknown unit */
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
