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
#include "registers.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"

static SEGPTR INT_Vectors[256];

  /* Ordinal number for interrupt 0 handler in WPROCS.DLL */
#define FIRST_INTERRUPT_ORDINAL 100


/**********************************************************************
 *	    INT_Init
 */
BOOL INT_Init(void)
{
    WORD vector;
    HMODULE16 hModule = GetModuleHandle( "WPROCS" );

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
void INT_DummyHandler( SIGCONTEXT context )
{
    WORD ordinal;
    char *name;
    STACK16FRAME *frame = CURRENT_STACK16;
    BUILTIN_GetEntryPoint( frame->entry_cs, frame->entry_ip, &ordinal, &name );
    INT_BARF( &context, ordinal - FIRST_INTERRUPT_ORDINAL );
}


/**********************************************************************
 *	    INT_Int11Handler
 *
 * Handler for int 11h (get equipment list).
 */
void INT_Int11Handler( SIGCONTEXT context )
{
    int diskdrives = 0;
    int parallelports = 0;
    int serialports = 0;
    int x;

/* borrowed from Ralph Brown's interrupt lists 

		    bits 15-14: number of parallel devices
		    bit     13: [Conv] Internal modem
		    bit     12: reserved
		    bits 11- 9: number of serial devices
		    bit      8: reserved
		    bits  7- 6: number of diskette drives minus one
		    bits  5- 4: Initial video mode:
				    00b = EGA,VGA,PGA
				    01b = 40 x 25 color
				    10b = 80 x 25 color
				    11b = 80 x 25 mono
		    bit      3: reserved
		    bit      2: [PS] =1 if pointing device
				[non-PS] reserved
		    bit      1: =1 if math co-processor
		    bit      0: =1 if diskette available for boot
*/
/*  Currently the only of these bits correctly set are:
		bits 15-14 		} Added by William Owen Smith, 
		bits 11-9		} wos@dcs.warwick.ac.uk
		bits 7-6
		bit  2			(always set)
*/

    if (DRIVE_IsValid(0)) diskdrives++;
    if (DRIVE_IsValid(1)) diskdrives++;
    if (diskdrives) diskdrives--;
	
    for (x=0; x!=MAX_PORTS; x++)
    {
	if (COM[x].devicename)
	    serialports++;
	if (LPT[x].devicename)
	    parallelports++;
    }
    if (serialports > 7)		/* 3 bits -- maximum value = 7 */
        serialports=7;
    if (parallelports > 3)		/* 2 bits -- maximum value = 3 */
        parallelports=3;
    
    AX_reg(&context) = (diskdrives << 6) | (serialports << 9) | 
                       (parallelports << 14) | 0x02;
}


/**********************************************************************
 *	    INT_Int12Handler
 *
 * Handler for int 12h (get memory size).
 */
void INT_Int12Handler( SIGCONTEXT context )
{
    AX_reg(&context) = 640;
}


/**********************************************************************
 *	    INT_Int15Handler
 *
 * Handler for int 15h.
 */
void INT_Int15Handler( SIGCONTEXT context )
{
    INT_BARF( &context, 0x15 );
}


/**********************************************************************
 *	    INT_Int16Handler
 *
 * Handler for int 16h (keyboard).
 */
void INT_Int16Handler( SIGCONTEXT context )
{
    INT_BARF( &context, 0x16 );
}
