/*
 * Interrupt vectors emulation
 *
 * Copyright 1995 Alexandre Julliard
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

#include <sys/types.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "miscemu.h"
#include "msdos.h"
#include "module.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

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
