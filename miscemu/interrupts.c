/*
 * Interrupt vectors emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "windows.h"
#include "miscemu.h"
#include "module.h"
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
    SEGPTR addr, dummyHandler;
    WORD vector;
    HMODULE hModule = GetModuleHandle( "WINPROCS" );

    dummyHandler = MODULE_GetEntryPoint( hModule, FIRST_INTERRUPT_ORDINAL+256);
    for (vector = 0; vector < 256; vector++)
    {
        addr = MODULE_GetEntryPoint( hModule, FIRST_INTERRUPT_ORDINAL+vector );
        INT_Vectors[vector] = addr ? addr : dummyHandler;
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
    dprintf_int( stddeb, "Get interrupt vector %02x -> %04x:%04x\n",
                 intnum, HIWORD(INT_Vectors[intnum]),
                 LOWORD(INT_Vectors[intnum]) );
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
void INT_DummyHandler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "Dummy handler called!\n" );
}

/**********************************************************************
 *	    INT_Int10Handler
 */
void INT_Int10Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 10 called indirectly through handler!\n" );
    do_int10( &context );
}


/**********************************************************************
 *	    INT_Int13Handler
 */
void INT_Int13Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 13 called indirectly through handler!\n" );
    do_int13( &context );
}


/**********************************************************************
 *	    INT_Int15Handler
 */
void INT_Int15Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 15 called indirectly through handler!\n" );
    do_int15( &context );
}


/**********************************************************************
 *	    INT_Int16Handler
 */
void INT_Int16Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 16 called indirectly through handler!\n" );
    do_int16( &context );
}


/**********************************************************************
 *	    INT_Int1aHandler
 */
void INT_Int1aHandler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 1a called indirectly through handler!\n" );
    do_int1a( &context );
}


/**********************************************************************
 *	    INT_Int21Handler
 */
void INT_Int21Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 21 called indirectly through handler!\n" );
    do_int21( &context );
}


/**********************************************************************
 *	    INT_Int25Handler
 */
void INT_Int25Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 25 called indirectly through handler!\n" );
    do_int25( &context );
}


/**********************************************************************
 *	    INT_Int26Handler
 */
void INT_Int26Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 26 called indirectly through handler!\n" );
    do_int26( &context );
}


/**********************************************************************
 *	    INT_Int2aHandler
 */
void INT_Int2aHandler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 2a called indirectly through handler!\n" );
    do_int2a( &context );
}


/**********************************************************************
 *	    INT_Int2fHandler
 */
void INT_Int2fHandler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 2f called indirectly through handler!\n" );
    do_int2f( &context );
}


/**********************************************************************
 *	    INT_Int31Handler
 */
void INT_Int31Handler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 31 called indirectly through handler!\n" );
    do_int31( &context );
}


/**********************************************************************
 *	    INT_Int5cHandler
 */
void INT_Int5cHandler( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int 5c called indirectly through handler!\n" );
    do_int5c( &context );
}
