/*
 * Interrupt vectors emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <sys/types.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "miscemu.h"
#include "msdos.h"
#include "module.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int);

static FARPROC16 INT_Vectors[256];

/* Ordinal number for interrupt 0 handler in WPROCS.DLL */
#define FIRST_INTERRUPT 100


/**********************************************************************
 *	    INT_GetPMHandler
 *
 * Return the protected mode interrupt vector for a given interrupt.
 */
FARPROC16 INT_GetPMHandler( BYTE intnum )
{
    if (!INT_Vectors[intnum])
    {
        static HMODULE16 wprocs;
        if (!wprocs)
        {
            if (((wprocs = GetModuleHandle16( "wprocs" )) < 32) &&
                ((wprocs = LoadLibrary16( "wprocs" )) < 32))
            {
                ERR("could not load wprocs.dll\n");
                return 0;
            }
        }
        if (!(INT_Vectors[intnum] = GetProcAddress16( wprocs, (LPCSTR)(FIRST_INTERRUPT + intnum))))
        {
            WARN("int%x not implemented, returning dummy handler\n", intnum );
            INT_Vectors[intnum] = GetProcAddress16( wprocs, (LPCSTR)(FIRST_INTERRUPT + 256) );
        }
    }
    return INT_Vectors[intnum];
}


/**********************************************************************
 *	    INT_SetPMHandler
 *
 * Set the protected mode interrupt handler for a given interrupt.
 */
void INT_SetPMHandler( BYTE intnum, FARPROC16 handler )
{
    TRACE("Set protected mode interrupt vector %02x <- %04x:%04x\n",
                 intnum, HIWORD(handler), LOWORD(handler) );
    INT_Vectors[intnum] = handler;
}


/**********************************************************************
 *         INT_DefaultHandler (WPROCS.356)
 *
 * Default interrupt handler.
 */
void WINAPI INT_DefaultHandler( CONTEXT86 *context )
{
}
