/*
 * Emulation of processor ioports.
 *
 * Copyright 1995 Morten Welinder
 * Copyright 1998 Andreas Mohr, Ove Kaaven
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dosexe.h"
#include "vga.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/**********************************************************************
 *	    DOSVM_inport
 */
BOOL WINAPI DOSVM_inport( int port, int size, DWORD *res )
{
    switch (port)
    {
    case 0x60:
        *res = DOSVM_Int09ReadScan(NULL);
        break;
    case 0x22a:
    case 0x22c:
    case 0x22e:
        *res = (DWORD)SB_ioport_in( port );
	break;
    case 0x3ba:
    case 0x3c0:
    case 0x3c1:
    case 0x3c2:
    case 0x3c3:
    case 0x3c4:
    case 0x3c5:
    case 0x3c6:
    case 0x3c7:
    case 0x3c8:
    case 0x3c9:
    case 0x3ca:
    case 0x3cb:
    case 0x3cc:
    case 0x3cd:
    case 0x3ce:
    case 0x3cf:
    case 0x3d0:
    case 0x3d1:
    case 0x3d2:
    case 0x3d3:
    case 0x3d4:
    case 0x3d5:
    case 0x3d6:
    case 0x3d7:
    case 0x3d8:
    case 0x3d9:
    case 0x3da:
    case 0x3db:
    case 0x3dc:
    case 0x3dd:
    case 0x3de:
    case 0x3df:
        if(size > 1)
           FIXME("Trying to read more than one byte from VGA!\n");
        *res = (DWORD)VGA_ioport_in( port );
        break;
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0xC0:
    case 0xC2:
    case 0xC4:
    case 0xC6:
    case 0xC8:
    case 0xCA:
    case 0xCC:
    case 0xCE:
    case 0x87:
    case 0x83:
    case 0x81:
    case 0x82:
    case 0x8B:
    case 0x89:
    case 0x8A:
    case 0x487:
    case 0x483:
    case 0x481:
    case 0x482:
    case 0x48B:
    case 0x489:
    case 0x48A:
    case 0x08:
    case 0xD0:
    case 0x0D:
    case 0xDA:
        *res = (DWORD)DMA_ioport_in( port );
        break;
    default:
        return FALSE;  /* not handled */
    }
    return TRUE;  /* handled */
}


/**********************************************************************
 *	    DOSVM_outport
 */
BOOL WINAPI DOSVM_outport( int port, int size, DWORD value )
{
    switch (port)
    {
    case 0x20:
        DOSVM_PIC_ioport_out( port, (BYTE)value );
        break;
    case 0x226:
    case 0x22c:
        SB_ioport_out( port, (BYTE)value );
        break;
    case 0x3c0:
    case 0x3c1:
    case 0x3c2:
    case 0x3c3:
    case 0x3c4:
    case 0x3c5:
    case 0x3c6:
    case 0x3c7:
    case 0x3c8:
    case 0x3c9:
    case 0x3ca:
    case 0x3cb:
    case 0x3cc:
    case 0x3cd:
    case 0x3ce:
    case 0x3cf:
    case 0x3d0:
    case 0x3d1:
    case 0x3d2:
    case 0x3d3:
    case 0x3d4:
    case 0x3d5:
    case 0x3d6:
    case 0x3d7:
    case 0x3d8:
    case 0x3d9:
    case 0x3da:
    case 0x3db:
    case 0x3dc:
    case 0x3dd:
    case 0x3de:
    case 0x3df:
        VGA_ioport_out( port, LOBYTE(value) );
        if(size > 1) {
            VGA_ioport_out( port+1, HIBYTE(value) );
            if(size > 2) {
                VGA_ioport_out( port+2, LOBYTE(HIWORD(value)) );
                VGA_ioport_out( port+3, HIBYTE(HIWORD(value)) );
            }
        }
        break;
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0xC0:
    case 0xC2:
    case 0xC4:
    case 0xC6:
    case 0xC8:
    case 0xCA:
    case 0xCC:
    case 0xCE:
    case 0x87:
    case 0x83:
    case 0x81:
    case 0x82:
    case 0x8B:
    case 0x89:
    case 0x8A:
    case 0x487:
    case 0x483:
    case 0x481:
    case 0x482:
    case 0x48B:
    case 0x489:
    case 0x48A:
    case 0x08:
    case 0xD0:
    case 0x0B:
    case 0xD6:
    case 0x0A:
    case 0xD4:
    case 0x0F:
    case 0xDE:
    case 0x09:
    case 0xD2:
    case 0x0C:
    case 0xD8:
    case 0x0D:
    case 0xDA:
    case 0x0E:
    case 0xDC:
        DMA_ioport_out( port, (BYTE)value );
        break;
    default:
        return FALSE;  /* not handled */
    }
    return TRUE;  /* handled */
}
