/*
 * Interrupt vectors emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "windows.h"
#include "miscemu.h"
#include "dos_fs.h"
#include "module.h"
#include "registers.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"

static SEGPTR INT_Vectors[256];

  /* Ordinal number for interrupt 0 handler in WINPROCS.DLL */
#define FIRST_INTERRUPT_ORDINAL 100


/**********************************************************************
 *	    INT_Init
 */
BOOL INT_Init(void)
{
    WORD vector;
    HMODULE hModule = GetModuleHandle( "WINPROCS" );

    for (vector = 0; vector < 256; vector++)
    {
        if (!(INT_Vectors[vector] = MODULE_GetEntryPoint( hModule,
		                             FIRST_INTERRUPT_ORDINAL+vector )))
	{
	    fprintf(stderr,"Internal error: no vector for int %02x\n",vector);
	    return FALSE;
	}
    }
    return TRUE;
}


/**********************************************************************
 *	    INT_GetHandler
 *
 * Return the interrupt vector for a given interrupt.
 */
SEGPTR INT_GetHandler( BYTE intnum )
{
    return INT_Vectors[intnum];
}


/**********************************************************************
 *	    INT_SetHandler
 *
 * Set the interrupt handler for a given interrupt.
 */
void INT_SetHandler( BYTE intnum, SEGPTR handler )
{
    dprintf_int( stddeb, "Set interrupt vector %02x <- %04x:%04x\n",
                 intnum, HIWORD(handler), LOWORD(handler) );
    INT_Vectors[intnum] = handler;
}


/**********************************************************************
 *	    INT_DummyHandler
 */
void INT_DummyHandler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    INT_BARF( CURRENT_STACK16->ordinal_number - FIRST_INTERRUPT_ORDINAL );
#undef context
}


/**********************************************************************
 *	    INT_Int11Handler
 *
 * Handler for int 11h (get equipment list).
 */
void INT_Int11Handler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    AX = DOS_GetEquipment();
#undef context
}


/**********************************************************************
 *	    INT_Int12Handler
 *
 * Handler for int 12h (get memory size).
 */
void INT_Int12Handler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    AX = 640;
#undef context
}


/**********************************************************************
 *	    INT_Int15Handler
 *
 * Handler for int 15h.
 */
void INT_Int15Handler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    INT_BARF( 0x15 );
#undef context
}


/**********************************************************************
 *	    INT_Int16Handler
 *
 * Handler for int 16h (keyboard).
 */
void INT_Int16Handler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    INT_BARF( 0x16 );
#undef context
}
