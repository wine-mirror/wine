/*
 * Emulation of processor ioports.
 *
 * Copyright 1995 Morten Welinder
 * Copyright 1998 Andreas Mohr, Ove Kaaven
 */

#include "config.h"

#include "windef.h"
#include "dosexe.h"
#include "vga.h"


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
    case 0x3ba:
    case 0x3da:
        *res = (DWORD)VGA_ioport_in( port );
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
    case 0x3c8:
    case 0x3c9:
        VGA_ioport_out( port, (BYTE)value );
        break;
    default:
        return FALSE;  /* not handled */
    }
    return TRUE;  /* handled */
}
