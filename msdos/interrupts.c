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

static INTPROC INT_WineHandler[256] = {
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 INT_Int10Handler, INT_Int11Handler, INT_Int12Handler, INT_Int13Handler,
 0, INT_Int15Handler, INT_Int16Handler, INT_Int17Handler,
 0, 0, INT_Int1aHandler, 0, 0, 0, 0, 0,
 INT_Int20Handler, DOS3Call, 0, 0, 0, INT_Int25Handler, 0, 0,
 0, INT_Int29Handler, INT_Int2aHandler, 0, 0, 0, 0, INT_Int2fHandler,
 0, INT_Int31Handler, 0, INT_Int33Handler
};

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
 *	    INT_GetRMHandler
 *
 * Return the real mode interrupt vector for a given interrupt.
 */
FARPROC16 INT_GetRMHandler( BYTE intnum )
{
    return ((FARPROC16*)DOSMEM_SystemBase())[intnum];
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
    ((FARPROC16*)DOSMEM_SystemBase())[intnum] = handler;
}


/**********************************************************************
 *	    INT_CtxGetHandler
 *
 * Return the interrupt vector for a given interrupt.
 */
FARPROC16 INT_CtxGetHandler( CONTEXT86 *context, BYTE intnum )
{
    if (ISV86(context))
        return INT_GetRMHandler(intnum);
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
    if (ISV86(context))
        INT_SetRMHandler(intnum, handler);
    else
        INT_SetPMHandler(intnum, handler);
}


/**********************************************************************
 *	    INT_GetWineHandler
 *
 * Return the Wine interrupt handler for a given interrupt.
 */
INTPROC INT_GetWineHandler( BYTE intnum )
{
    return INT_WineHandler[intnum];
}


/**********************************************************************
 *	    INT_SetWineHandler
 *
 * Set the Wine interrupt handler for a given interrupt.
 */
void INT_SetWineHandler( BYTE intnum, INTPROC handler )
{
    TRACE("Set Wine interrupt vector %02x <- %p\n", intnum, handler );
    INT_WineHandler[intnum] = handler;
}


/**********************************************************************
 *	    INT_RealModeInterrupt
 *
 * Handle real mode interrupts
 */
int INT_RealModeInterrupt( BYTE intnum, CONTEXT86 *context )
{
    if (INT_WineHandler[intnum]) {
        (*INT_WineHandler[intnum])(context);
        return 0;
    }
    FIXME("Unknown Interrupt in DOS mode: 0x%x\n", intnum);
    return 1;
}


/**********************************************************************
 *         INT_DefaultHandler (WPROCS.356)
 *
 * Default interrupt handler.
 */
void WINAPI INT_DefaultHandler( CONTEXT86 *context )
{
}
