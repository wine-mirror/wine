/*
 * Interrupt vectors emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <sys/types.h>

#include "windows.h"
#include "drive.h"
#include "miscemu.h"
#include "msdos.h"
#include "module.h"
#include "stackframe.h"
#include "debug.h"

static FARPROC16 INT_Vectors[256];


/**********************************************************************
 *	    INT_GetHandler
 *
 * Return the interrupt vector for a given interrupt.
 */
FARPROC16 INT_GetHandler( BYTE intnum )
{
    return INT_Vectors[intnum];
}


/**********************************************************************
 *	    INT_SetHandler
 *
 * Set the interrupt handler for a given interrupt.
 */
void INT_SetHandler( BYTE intnum, FARPROC16 handler )
{
    TRACE(int, "Set interrupt vector %02x <- %04x:%04x\n",
                 intnum, HIWORD(handler), LOWORD(handler) );
    INT_Vectors[intnum] = handler;
}


/**********************************************************************
 *	    INT_RealModeInterrupt
 *
 * Handle real mode interrupts
 */
int INT_RealModeInterrupt( BYTE intnum, PCONTEXT context )
{
    /* we should really map to if1632/wprocs.spec, but not all
     * interrupt handlers are adapted to support real mode yet */
    switch (intnum) {
        case 0x10:
            INT_Int10Handler(context);
            break;
        case 0x1a:
            INT_Int1aHandler(context);
            break;
        case 0x20:
            INT_Int20Handler(context);
            break;
        case 0x21:
            DOS3Call(context);
            break;
        case 0x2f:
            INT_Int2fHandler(context);
            break;
        default:
            return 1;
    }
    return 0;
}
