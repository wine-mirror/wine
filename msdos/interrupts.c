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
static FARPROC48 INT_Vectors48[256];

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
 *         INT_GetPMHandler48
 *
 * Return the protected mode interrupt vector for a given interrupt.
 * Used to get 48-bit pointer for 32-bit interrupt handlers in DPMI32.
 */
FARPROC48 INT_GetPMHandler48( BYTE intnum )
{
    if (!INT_Vectors48[intnum].selector)
    {
        INT_Vectors48[intnum].selector = DOSMEM_dpmi_segments.int48_sel;
        INT_Vectors48[intnum].offset = 4 * intnum;
    }
    return INT_Vectors48[intnum];
}


/**********************************************************************
 *         INT_SetPMHandler48
 *
 * Set the protected mode interrupt handler for a given interrupt.
 * Used to set 48-bit pointer for 32-bit interrupt handlers in DPMI32.
 */
void INT_SetPMHandler48( BYTE intnum, FARPROC48 handler )
{
    TRACE("Set 32-bit protected mode interrupt vector %02x <- %04x:%08lx\n",
          intnum, handler.selector, handler.offset );
    INT_Vectors48[intnum] = handler;
}


/**********************************************************************
 *         INT_DefaultHandler (WPROCS.356)
 *
 * Default interrupt handler.
 */
void WINAPI INT_DefaultHandler( CONTEXT86 *context )
{
}
