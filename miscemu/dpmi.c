/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "ldt.h"
#include "registers.h"
#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"


/**********************************************************************
 *	    INT_Int31Handler
 *
 * Handler for int 31h (DPMI).
 */
void INT_Int31Handler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    DWORD dw;
    BYTE *ptr;

    ResetCflag;
    switch(AX)
    {
    case 0x0000:  /* Allocate LDT descriptors */
        if (!(AX = AllocSelectorArray( CX )))
        {
            AX = 0x8011;  /* descriptor unavailable */
            SetCflag;
        }
        break;

    case 0x0001:  /* Free LDT descriptor */
        if (FreeSelector( BX ))
        {
            AX = 0x8022;  /* invalid selector */
            SetCflag;
        }
        break;

    case 0x0003:  /* Get next selector increment */
        AX = __AHINCR;
        break;

    case 0x0004:  /* Lock selector (not supported) */
        AX = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0005:  /* Unlock selector (not supported) */
        AX = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0006:  /* Get selector base address */
        if (!(dw = GetSelectorBase( BX )))
        {
            AX = 0x8022;  /* invalid selector */
            SetCflag;
        }
        else
        {
            CX = HIWORD(dw);
            DX = LOWORD(dw);
        }
        break;

    case 0x0007:  /* Set selector base address */
        SetSelectorBase( BX, MAKELONG( DX, CX ) );
        break;

    case 0x0008:  /* Set selector limit */
        SetSelectorLimit( BX, MAKELONG( DX, CX ) );
        break;

    case 0x0009:  /* Set selector access rights */
        SelectorAccessRights( BX, 1, CX );

    case 0x000a:  /* Allocate selector alias */
        if (!(AX = AllocCStoDSAlias( BX )))
        {
            AX = 0x8011;  /* descriptor unavailable */
            SetCflag;
        }
        break;

    case 0x000b:  /* Get descriptor */
        {
            ldt_entry entry;
            LDT_GetEntry( SELECTOR_TO_ENTRY(BX), &entry );
            /* FIXME: should use ES:EDI for 32-bit clients */
            LDT_EntryToBytes( PTR_SEG_OFF_TO_LIN( ES, DI ), &entry );
        }
        break;

    case 0x000c:  /* Set descriptor */
        {
            ldt_entry entry;
            LDT_BytesToEntry( PTR_SEG_OFF_TO_LIN( ES, DI ), &entry );
            LDT_GetEntry( SELECTOR_TO_ENTRY(BX), &entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
        AX = 0x8011; /* descriptor unavailable */
        SetCflag;
        break;

    case 0x0204:  /* Get protected mode interrupt vector */
	dw = (DWORD)INT_GetHandler( BL );
	CX = HIWORD(dw);
	DX = LOWORD(dw);
	break;

    case 0x0205:  /* Set protected mode interrupt vector */
	INT_SetHandler( BL, (SEGPTR)MAKELONG( DX, CX ) );
	break;

    case 0x0400:  /* Get DPMI version */
        AX = 0x005a;  /* DPMI version 0.90 */
        BX = 0x0005;  /* Flags: 32-bit, virtual memory */
        CL = 3;       /* CPU type: 386 */
        DX = 0x0102;  /* Master and slave interrupt controller base */
        break;

    case 0x0500:  /* Get free memory information */
        ptr = (BYTE *)PTR_SEG_OFF_TO_LIN( ES, DI );
        *(DWORD *)ptr = 0x00ff0000; /* Largest block available */
        memset( ptr + 4, 0xff, 0x2c );  /* No other information supported */
        break;

    case 0x0501:  /* Allocate memory block */
        if (!(ptr = (BYTE *)malloc( MAKELONG( CX, BX ) )))
        {
            AX = 0x8012;  /* linear memory not available */
            SetCflag;
        }
        else
        {
            BX = SI = HIWORD(ptr);
            CX = DI = LOWORD(ptr);
        }
        break;

    case 0x0502:  /* Free memory block */
        free( (void *)MAKELONG( DI, SI ) );
        break;

    case 0x0503:  /* Resize memory block */
        if (!(ptr = (BYTE *)realloc( (void *)MAKELONG(DI,SI),MAKELONG(CX,BX))))
        {
            AX = 0x8012;  /* linear memory not available */
            SetCflag;
        }
        else
        {
            BX = SI = HIWORD(ptr);
            CX = DI = LOWORD(ptr);
        }
        break;

    case 0x0600:  /* Lock linear region */
        break;  /* Just ignore it */

    case 0x0601:  /* Unlock linear region */
        break;  /* Just ignore it */

    default:
        INT_BARF( 0x31 );
        AX = 0x8001;  /* unsupported function */
        SetCflag;
        break;
    }
#undef context
}
