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
            if ((wprocs = GetModuleHandle16( "wprocs" )) < 32)
            {
                ERR("could not load wprocs.dll\n");
                return 0;
            }
        }
        INT_Vectors[intnum] = NE_GetEntryPoint( wprocs, FIRST_INTERRUPT + intnum );
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
 *	    INT_GetRMHandler
 *
 * Return the real mode interrupt vector for a given interrupt.
 */
FARPROC16 INT_GetRMHandler( BYTE intnum )
{
    return ((FARPROC16*)DOSMEM_MemoryBase(0))[intnum];
}


/**********************************************************************
 *	    INT_SetRMHandler
 *
 * Set the real mode interrupt handler for a given interrupt.
 */
void INT_SetRMHandler( BYTE intnum, FARPROC16 handler )
{
    TRACE("Set real mode interrupt vector %02x <- %04x:%04x\n",
                 intnum, HIWORD(handler), LOWORD(handler) );
    ((FARPROC16*)DOSMEM_MemoryBase(0))[intnum] = handler;
}


/**********************************************************************
 *	    INT_CtxGetHandler
 *
 * Return the interrupt vector for a given interrupt.
 */
FARPROC16 INT_CtxGetHandler( CONTEXT86 *context, BYTE intnum )
{
    if (ISV86(context))
        return ((FARPROC16*)V86BASE(context))[intnum];
    else
        return INT_GetPMHandler(intnum);
}


/**********************************************************************
 *	    INT_CtxSetHandler
 *
 * Set the interrupt handler for a given interrupt.
 */
void INT_CtxSetHandler( CONTEXT86 *context, BYTE intnum, FARPROC16 handler )
{
    if (ISV86(context)) {
        TRACE("Set real mode interrupt vector %02x <- %04x:%04x\n",
                     intnum, HIWORD(handler), LOWORD(handler) );
        ((FARPROC16*)V86BASE(context))[intnum] = handler;
    } else
        INT_SetPMHandler(intnum, handler);
}


/**********************************************************************
 *	    INT_RealModeInterrupt
 *
 * Handle real mode interrupts
 */
int INT_RealModeInterrupt( BYTE intnum, CONTEXT86 *context )
{
    /* we should really map to if1632/wprocs.spec, but not all
     * interrupt handlers are adapted to support real mode yet */
    switch (intnum) {
        case 0x09:
            INT_Int09Handler(context);
            break;
        case 0x10:
            INT_Int10Handler(context);
            break;
        case 0x11:
            INT_Int11Handler(context);
            break;
        case 0x12:
            INT_Int12Handler(context);
            break;
	case 0x13:
	    INT_Int13Handler(context);
            break;
        case 0x15:
            INT_Int15Handler(context);
            break;
        case 0x16:
            INT_Int16Handler(context);
            break;
        case 0x17:
            INT_Int17Handler(context);
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
        case 0x25:
            INT_Int25Handler(context);
            break;
        case 0x29:
            INT_Int29Handler(context);
            break;
        case 0x2a:
            INT_Int2aHandler(context);
            break; 
        case 0x2f:
            INT_Int2fHandler(context);
            break;
        case 0x31:
            INT_Int31Handler(context);
            break;
        case 0x33:
            INT_Int33Handler(context);
            break;
        default:
            FIXME("Unknown Interrupt in DOS mode: 0x%x\n", intnum);
            return 1;
    }
    return 0;
}
