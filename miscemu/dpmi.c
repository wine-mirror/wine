/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "heap.h"
#include "ldt.h"
#include "module.h"
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

extern void do_mscdex( SIGCONTEXT *context );

/**********************************************************************
 *	    INT_Int31Handler
 *
 * Handler for int 31h (DPMI).
 */
void INT_Int31Handler( SIGCONTEXT *context )
{
    DWORD dw;
    BYTE *ptr;

    RESET_CFLAG(context);
    switch(AX_reg(context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
        if (!(AX_reg(context) = AllocSelectorArray( CX_reg(context) )))
        {
            AX_reg(context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x0001:  /* Free LDT descriptor */
        if (FreeSelector( BX_reg(context) ))
        {
            AX_reg(context) = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            /* If a segment register contains the selector being freed, */
            /* set it to zero. */
            if (!((DS_reg(context)^BX_reg(context)) & ~3)) DS_reg(context) = 0;
            if (!((ES_reg(context)^BX_reg(context)) & ~3)) ES_reg(context) = 0;
#ifdef FS_reg
            if (!((FS_reg(context)^BX_reg(context)) & ~3)) FS_reg(context) = 0;
#endif
#ifdef GS_reg
            if (!((GS_reg(context)^BX_reg(context)) & ~3)) GS_reg(context) = 0;
#endif
        }
        break;

    case 0x0002:  /* Real mode segment to descriptor */
        {
            WORD entryPoint = 0;  /* KERNEL entry point for descriptor */
            switch(BX_reg(context))
            {
            case 0x0000: entryPoint = 183; break;  /* __0000H */
            case 0x0040: entryPoint = 193; break;  /* __0040H */
            case 0xa000: entryPoint = 174; break;  /* __A000H */
            case 0xb000: entryPoint = 181; break;  /* __B000H */
            case 0xb800: entryPoint = 182; break;  /* __B800H */
            case 0xc000: entryPoint = 195; break;  /* __C000H */
            case 0xd000: entryPoint = 179; break;  /* __D000H */
            case 0xe000: entryPoint = 190; break;  /* __E000H */
            case 0xf000: entryPoint = 194; break;  /* __F000H */
            default:
                fprintf( stderr, "DPMI: real-mode seg to descriptor %04x not possible\n",
                         BX_reg(context) );
                AX_reg(context) = 0x8011;
                SET_CFLAG(context);
                break;
            }
            if (entryPoint) 
                AX_reg(context) = LOWORD(MODULE_GetEntryPoint( 
                                                   GetModuleHandle( "KERNEL" ),
                                                   entryPoint ));
        }
        break;

    case 0x0003:  /* Get next selector increment */
        AX_reg(context) = __AHINCR;
        break;

    case 0x0004:  /* Lock selector (not supported) */
        AX_reg(context) = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0005:  /* Unlock selector (not supported) */
        AX_reg(context) = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0006:  /* Get selector base address */
        if (!(dw = GetSelectorBase( BX_reg(context) )))
        {
            AX_reg(context) = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            CX_reg(context) = HIWORD(dw);
            DX_reg(context) = LOWORD(dw);
        }
        break;

    case 0x0007:  /* Set selector base address */
        SetSelectorBase( BX_reg(context),
                         MAKELONG( DX_reg(context), CX_reg(context) ) );
        break;

    case 0x0008:  /* Set selector limit */
        SetSelectorLimit( BX_reg(context),
                          MAKELONG( DX_reg(context), CX_reg(context) ) );
        break;

    case 0x0009:  /* Set selector access rights */
        SelectorAccessRights( BX_reg(context), 1, CX_reg(context) );
        break;

    case 0x000a:  /* Allocate selector alias */
        if (!(AX_reg(context) = AllocCStoDSAlias( BX_reg(context) )))
        {
            AX_reg(context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x000b:  /* Get descriptor */
        {
            ldt_entry entry;
            LDT_GetEntry( SELECTOR_TO_ENTRY( BX_reg(context) ), &entry );
            /* FIXME: should use ES:EDI for 32-bit clients */
            LDT_EntryToBytes( PTR_SEG_OFF_TO_LIN( ES_reg(context),
                                                  DI_reg(context) ), &entry );
        }
        break;

    case 0x000c:  /* Set descriptor */
        {
            ldt_entry entry;
            LDT_BytesToEntry( PTR_SEG_OFF_TO_LIN( ES_reg(context),
                                                  DI_reg(context) ), &entry );
            LDT_GetEntry( SELECTOR_TO_ENTRY( BX_reg(context) ), &entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
        AX_reg(context) = 0x8011; /* descriptor unavailable */
        SET_CFLAG(context);
        break;

    case 0x0204:  /* Get protected mode interrupt vector */
	dw = (DWORD)INT_GetHandler( BL_reg(context) );
	CX_reg(context) = HIWORD(dw);
	DX_reg(context) = LOWORD(dw);
	break;

    case 0x0205:  /* Set protected mode interrupt vector */
	INT_SetHandler( BL_reg(context),
                        (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( CX_reg(context),
                                                          DX_reg(context) ));
	break;

    case 0x0300:  /* Simulate real mode interrupt 
        *  Interrupt number is in BL, flags are in BH
        *  ES:DI points to real-mode call structure  
        *  Currently we just print it out and return error.
        */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
            fprintf(stdnimp,
                    "RealModeInt %02x: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "                ESI=%08lx EDI=%08lx ES=%04x DS=%04x\n",
                    BL_reg(context), p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds );

            /* Compton's 1995 encyclopedia does its MSCDEX calls through
             * this interrupt.  Why?  Who knows...
             */
            if ((BL_reg(context) == 0x2f) && ((p->eax & 0xFF00) == 0x1500))
            {
                do_mscdex( context );
		break;
            }
            SET_CFLAG(context);
        }
        break;

    case 0x0301:  /* Call real mode procedure with far return */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
            fprintf(stdnimp,
                    "RealModeCall: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "              ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n",
                    p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds, p->cs, p->ip );
            SET_CFLAG(context);
        }
        break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
            fprintf(stdnimp,
                    "RealModeCallIret: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "                  ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n",
                    p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds, p->cs, p->ip );
            SET_CFLAG(context);
        }
        break;

    case 0x0400:  /* Get DPMI version */
        AX_reg(context) = 0x005a;  /* DPMI version 0.90 */
        BX_reg(context) = 0x0005;  /* Flags: 32-bit, virtual memory */
        CL_reg(context) = runtime_cpu ();
        DX_reg(context) = 0x0102;  /* Master/slave interrupt controller base*/
        break;

    case 0x0500:  /* Get free memory information */
        ptr = (BYTE *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
        *(DWORD *)ptr = 0x00ff0000; /* Largest block available */
        memset( ptr + 4, 0xff, 0x2c );  /* No other information supported */
        break;

    case 0x0501:  /* Allocate memory block */
        if (!(ptr = (BYTE *)HeapAlloc( SystemHeap, 0,MAKELONG( CX_reg(context),
                                                           BX_reg(context) ))))
        {
            AX_reg(context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(context);
        }
        else
        {
            BX_reg(context) = SI_reg(context) = HIWORD(ptr);
            CX_reg(context) = DI_reg(context) = LOWORD(ptr);
        }
        break;

    case 0x0502:  /* Free memory block */
        HeapFree( SystemHeap, 0,
                  (void *)MAKELONG( DI_reg(context), SI_reg(context) ) );
        break;

    case 0x0503:  /* Resize memory block */
        if (!(ptr = (BYTE *)HeapReAlloc( SystemHeap, 0,
                           (void *)MAKELONG(DI_reg(context),SI_reg(context)),
                                   MAKELONG(CX_reg(context),BX_reg(context)))))
        {
            AX_reg(context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(context);
        }
        else
        {
            BX_reg(context) = SI_reg(context) = HIWORD(ptr);
            CX_reg(context) = DI_reg(context) = LOWORD(ptr);
        }
        break;

    case 0x0600:  /* Lock linear region */
        break;  /* Just ignore it */

    case 0x0601:  /* Unlock linear region */
        break;  /* Just ignore it */

    case 0x0602:  /* Unlock real-mode region */
        break;  /* Just ignore it */

    case 0x0603:  /* Lock real-mode region */
        break;  /* Just ignore it */

    case 0x0604:  /* Get page size */
        BX_reg(context) = 0;
        CX_reg(context) = 4096;
	break;

    case 0x0702:  /* Mark page as demand-paging candidate */
        break;  /* Just ignore it */

    case 0x0703:  /* Discard page contents */
        break;  /* Just ignore it */

    default:
        INT_BARF( context, 0x31 );
        AX_reg(context) = 0x8001;  /* unsupported function */
        SET_CFLAG(context);
        break;
    }
}
