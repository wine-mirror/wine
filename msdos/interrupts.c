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
