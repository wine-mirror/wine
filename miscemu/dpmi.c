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


/* Structure for real-mode callbacks */
typedef struct
{
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD reserved;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;
    WORD  flags;
    WORD  es;
    WORD  ds;
    WORD  fs;
    WORD  gs;
    WORD  ip;
    WORD  cs;
    WORD  sp;
    WORD  ss;
} REALMODECALL;

/**********************************************************************
 *	    INT_Int31Handler
 *
 * Handler for int 31h (DPMI).
 */
void INT_Int31Handler( struct sigcontext_struct context )
{
    DWORD dw;
    BYTE *ptr;

    RESET_CFLAG(&context);
    switch(AX_reg(&context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
        if (!(AX_reg(&context) = AllocSelectorArray( CX_reg(&context) )))
        {
            AX_reg(&context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(&context);
        }
        break;

    case 0x0001:  /* Free LDT descriptor */
        if (FreeSelector( BX_reg(&context) ))
        {
            AX_reg(&context) = 0x8022;  /* invalid selector */
            SET_CFLAG(&context);
        }
        break;

    case 0x0003:  /* Get next selector increment */
        AX_reg(&context) = __AHINCR;
        break;

    case 0x0004:  /* Lock selector (not supported) */
        AX_reg(&context) = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0005:  /* Unlock selector (not supported) */
        AX_reg(&context) = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0006:  /* Get selector base address */
        if (!(dw = GetSelectorBase( BX_reg(&context) )))
        {
            AX_reg(&context) = 0x8022;  /* invalid selector */
            SET_CFLAG(&context);
        }
        else
        {
            CX_reg(&context) = HIWORD(dw);
            DX_reg(&context) = LOWORD(dw);
        }
        break;

    case 0x0007:  /* Set selector base address */
        SetSelectorBase( BX_reg(&context),
                         MAKELONG( DX_reg(&context), CX_reg(&context) ) );
        break;

    case 0x0008:  /* Set selector limit */
        SetSelectorLimit( BX_reg(&context),
                          MAKELONG( DX_reg(&context), CX_reg(&context) ) );
        break;

    case 0x0009:  /* Set selector access rights */
        SelectorAccessRights( BX_reg(&context), 1, CX_reg(&context) );

    case 0x000a:  /* Allocate selector alias */
        if (!(AX_reg(&context) = AllocCStoDSAlias( BX_reg(&context) )))
        {
            AX_reg(&context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(&context);
        }
        break;

    case 0x000b:  /* Get descriptor */
        {
            ldt_entry entry;
            LDT_GetEntry( SELECTOR_TO_ENTRY( BX_reg(&context) ), &entry );
            /* FIXME: should use ES:EDI for 32-bit clients */
            LDT_EntryToBytes( PTR_SEG_OFF_TO_LIN( ES_reg(&context),
                                                  DI_reg(&context) ), &entry );
        }
        break;

    case 0x000c:  /* Set descriptor */
        {
            ldt_entry entry;
            LDT_BytesToEntry( PTR_SEG_OFF_TO_LIN( ES_reg(&context),
                                                  DI_reg(&context) ), &entry );
            LDT_GetEntry( SELECTOR_TO_ENTRY( BX_reg(&context) ), &entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
        AX_reg(&context) = 0x8011; /* descriptor unavailable */
        SET_CFLAG(&context);
        break;

    case 0x0204:  /* Get protected mode interrupt vector */
	dw = (DWORD)INT_GetHandler( BL_reg(&context) );
	CX_reg(&context) = HIWORD(dw);
	DX_reg(&context) = LOWORD(dw);
	break;

    case 0x0205:  /* Set protected mode interrupt vector */
	INT_SetHandler( BL_reg(&context),
                       (SEGPTR)MAKELONG( DX_reg(&context), CX_reg(&context) ));
	break;

    case 0x0300:  /* Simulate real mode interrupt 
        *  Interrupt number is in BL, flags are in BH
        *  ES:DI points to real-mode call structure  
        *  Currently we just print it out and return error.
        */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(&context), DI_reg(&context) );
            fprintf(stdnimp,
                    "RealModeInt %02x: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "                ESI=%08lx EDI=%08lx ES=%04x DS=%04x\n",
                    BL_reg(&context), p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds );
            SET_CFLAG(&context);
        }
        break;

    case 0x0301:  /* Call real mode procedure with far return */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(&context), DI_reg(&context) );
            fprintf(stdnimp,
                    "RealModeCall: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "              ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n",
                    p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds, p->cs, p->ip );
            SET_CFLAG(&context);
        }
        break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(&context), DI_reg(&context) );
            fprintf(stdnimp,
                    "RealModeCallIret: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "                  ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n",
                    p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds, p->cs, p->ip );
            SET_CFLAG(&context);
        }
        break;

    case 0x0400:  /* Get DPMI version */
        AX_reg(&context) = 0x005a;  /* DPMI version 0.90 */
        BX_reg(&context) = 0x0005;  /* Flags: 32-bit, virtual memory */
        CL_reg(&context) = runtime_cpu ();
        DX_reg(&context) = 0x0102;  /* Master/slave interrupt controller base*/
        break;

    case 0x0500:  /* Get free memory information */
        ptr = (BYTE *)PTR_SEG_OFF_TO_LIN( ES_reg(&context), DI_reg(&context) );
        *(DWORD *)ptr = 0x00ff0000; /* Largest block available */
        memset( ptr + 4, 0xff, 0x2c );  /* No other information supported */
        break;

    case 0x0501:  /* Allocate memory block */
        if (!(ptr = (BYTE *)malloc( MAKELONG( CX_reg(&context),
                                              BX_reg(&context) ) )))
        {
            AX_reg(&context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(&context);
        }
        else
        {
            BX_reg(&context) = SI_reg(&context) = HIWORD(ptr);
            CX_reg(&context) = DI_reg(&context) = LOWORD(ptr);
        }
        break;

    case 0x0502:  /* Free memory block */
        free( (void *)MAKELONG( DI_reg(&context), SI_reg(&context) ) );
        break;

    case 0x0503:  /* Resize memory block */
        if (!(ptr = (BYTE *)realloc( (void *)MAKELONG(DI_reg(&context),SI_reg(&context)),
                                     MAKELONG(CX_reg(&context),BX_reg(&context)))))
        {
            AX_reg(&context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(&context);
        }
        else
        {
            BX_reg(&context) = SI_reg(&context) = HIWORD(ptr);
            CX_reg(&context) = DI_reg(&context) = LOWORD(ptr);
        }
        break;

    case 0x0600:  /* Lock linear region */
        break;  /* Just ignore it */

    case 0x0601:  /* Unlock linear region */
        break;  /* Just ignore it */

    default:
        INT_BARF( &context, 0x31 );
        AX_reg(&context) = 0x8001;  /* unsupported function */
        SET_CFLAG(&context);
        break;
    }
}
