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
#include "drive.h"
#include "msdos.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"

#define DOS_GET_DRIVE(reg) ((reg) ? (reg) - 1 : DRIVE_GetCurrentDrive())

void CreateBPB(int drive, BYTE *data);  /* defined in int21.c */


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
    WORD  fl;
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
 *	    INT_GetRealModeContext
 */
static void INT_GetRealModeContext( REALMODECALL *call, CONTEXT *context )
{
    EAX_reg(context) = call->eax;
    EBX_reg(context) = call->ebx;
    ECX_reg(context) = call->ecx;
    EDX_reg(context) = call->edx;
    ESI_reg(context) = call->esi;
    EDI_reg(context) = call->edi;
    EBP_reg(context) = call->ebp;
    EFL_reg(context) = call->fl;
    EIP_reg(context) = call->ip;
    ESP_reg(context) = call->sp;
    CS_reg(context)  = call->cs;
    DS_reg(context)  = call->ds;
    ES_reg(context)  = call->es;
    FS_reg(context)  = call->fs;
    GS_reg(context)  = call->gs;
}


/**********************************************************************
 *	    INT_SetRealModeContext
 */
static void INT_SetRealModeContext( REALMODECALL *call, CONTEXT *context )
{
    call->eax = EAX_reg(context);
    call->ebx = EBX_reg(context);
    call->ecx = ECX_reg(context);
    call->edx = EDX_reg(context);
    call->esi = ESI_reg(context);
    call->edi = EDI_reg(context);
    call->ebp = EBP_reg(context);
    call->fl  = FL_reg(context);
    call->ip  = IP_reg(context);
    call->sp  = SP_reg(context);
    call->cs  = CS_reg(context);
    call->ds  = DS_reg(context);
    call->es  = ES_reg(context);
    call->fs  = FS_reg(context);
    call->gs  = GS_reg(context);
}


/**********************************************************************
 *	    INT_DoRealModeInt
 */
static void INT_DoRealModeInt( CONTEXT *context )
{
    CONTEXT realmode_ctx;
    REALMODECALL *call = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context),
                                                          DI_reg(context) );
    INT_GetRealModeContext( call, &realmode_ctx );

    RESET_CFLAG(context);
    switch (BL_reg(context))
    {
    case 0x2f:	/* int2f */
        switch (AH_reg(&realmode_ctx))
        {
        case 0x15:
            /* MSCDEX hook */
            do_mscdex( &realmode_ctx );
            break;
        default:
            SET_CFLAG(context);
            break;
        }
        break;
    case 0x21:	/* int21 */
        switch (AH_reg(&realmode_ctx))
        {
        case 0x52:
            ES_reg(&realmode_ctx) = 0;
            EBX_reg(&realmode_ctx) = 0;
            break;
        case 0x65:
            switch (AL_reg(&realmode_ctx))
            {
            case 0x06:
                {/* get collate table */
                    /* ES:DI is a REALMODE pointer to 5 byte dosmem 
                     * we fill that with 0x6, realmode pointer to collateTB
                     */
                    char *table = DOSMEM_MapRealToLinear(
                       MAKELONG(EDI_reg(&realmode_ctx),ES_reg(&realmode_ctx)));
                    *(BYTE*)table      = 0x06;
                    *(DWORD*)(table+1) = DOSMEM_CollateTable;
                    CX_reg(&realmode_ctx) = 258;/*FIXME: size of table?*/
                    break;
                }
            default:
                SET_CFLAG(context);
                break;
            }
            break;
        case 0x44:
            switch (AL_reg(&realmode_ctx))
            {
            case 0x0D:
                {/* generic block device request */
                    BYTE *dataptr = DOSMEM_MapRealToLinear(
                       MAKELONG(EDX_reg(&realmode_ctx),DS_reg(&realmode_ctx)));
                    int drive = DOS_GET_DRIVE(BL_reg(&realmode_ctx));
                    if (CH_reg(&realmode_ctx) != 0x08)
                    {
                        SET_CFLAG(context);
                        break;
                    }
                    switch (CL_reg(&realmode_ctx))
                    {
                    case 0x66:
                        {
			    char    label[12],fsname[9],path[4];
			    DWORD   serial;

			    strcpy(path,"x:\\");path[0]=drive+'A';
			    GetVolumeInformation32A(path,label,12,&serial,NULL,NULL,fsname,9);
			    *(WORD*)dataptr         = 0;
			    memcpy(dataptr+2,&serial,4);
			    memcpy(dataptr+6,label  ,11);
			    memcpy(dataptr+17,fsname,8);
			    break;
                        }
                    case 0x60:	/* get device parameters */
                                /* used by defrag.exe of win95 */
                        memset(dataptr, 0, 0x26);
                        dataptr[0] = 0x04;
                        dataptr[6] = 0; /* media type */
                        if (drive > 1)
                        {
                            dataptr[1] = 0x05; /* fixed disk */
                            setword(&dataptr[2], 0x01); /* non removable */
                            setword(&dataptr[4], 0x300); /* # of cylinders */
                        }
                        else
                        {
                            dataptr[1] = 0x07; /* block dev, floppy */
                            setword(&dataptr[2], 0x02); /* removable */
                            setword(&dataptr[4], 80); /* # of cylinders */
                        }
                        CreateBPB(drive, &dataptr[7]);
                        break;
                    default:
                        SET_CFLAG(context);
                        break;
                    }
                }
            break;
            default:
                SET_CFLAG(context);
                break;
            }
            break;
        default:
            SET_CFLAG(context);
            break;
        }
        break;
    default:
        SET_CFLAG(context);
        break;
    }

    if (EFL_reg(context)&1)
        fprintf(stdnimp,
                "RealModeInt %02x: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                "                ESI=%08lx EDI=%08lx DS=%04lx ES=%04lx\n",
                BL_reg(context), EAX_reg(&realmode_ctx),
                EBX_reg(&realmode_ctx), ECX_reg(&realmode_ctx),
                EDX_reg(&realmode_ctx), ESI_reg(&realmode_ctx),
                EDI_reg(&realmode_ctx), DS_reg(&realmode_ctx),
                ES_reg(&realmode_ctx) );
    INT_SetRealModeContext( call, &realmode_ctx );
}


/**********************************************************************
 *	    INT_Int31Handler
 *
 * Handler for int 31h (DPMI).
 */
void WINAPI INT_Int31Handler( CONTEXT *context )
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
            if (!((FS_reg(context)^BX_reg(context)) & ~3)) FS_reg(context) = 0;
            if (!((GS_reg(context)^BX_reg(context)) & ~3)) GS_reg(context) = 0;
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
	    	AX_reg(context) = DOSMEM_AllocSelector(BX_reg(context));
                break;
            }
            if (entryPoint) 
                AX_reg(context) = LOWORD(MODULE_GetEntryPoint( 
                                                 GetModuleHandle16( "KERNEL" ),
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
    case 0x0200: /* get real mode interrupt vector */
	fprintf(stdnimp,
		"int31: get realmode interupt vector(0x%02x) unimplemented.\n",
		BL_reg(context)
	);
    	SET_CFLAG(context);
        break;
    case 0x0201: /* set real mode interrupt vector */
	fprintf(stdnimp,
		"int31: set realmode interupt vector(0x%02x,0x%04x:0x%04x) unimplemented\n",
		BL_reg(context),CX_reg(context),DX_reg(context)
	);
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

    case 0x0300:  /* Simulate real mode interrupt */
        INT_DoRealModeInt( context );
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

    case 0x0303:  /* Allocate Real Mode Callback Address */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
            fprintf(stdnimp,
                    "AllocRMCB: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
                    "           ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n"
		    "	Function to call: %04x:%04x\n",
                    p->eax, p->ebx, p->ecx, p->edx,
                    p->esi, p->edi, p->es, p->ds, p->cs, p->ip,
		    (WORD)DS_reg(context), SI_reg(context) );
            SET_CFLAG(context);
        }
        break;

    case 0x0304:  /* Free Real Mode Callback Address */
	{
	    fprintf(stdnimp,
		    "FreeRMCB: callback address: %04x:%04x\n",
		    CX_reg(context), DX_reg(context));
	    SET_CFLAG(context);
	}
	break;

    case 0x0400:  /* Get DPMI version */
    	{
	    SYSTEM_INFO si;

	    GetSystemInfo(&si);
	    AX_reg(context) = 0x005a;  /* DPMI version 0.90 */
	    BX_reg(context) = 0x0005;  /* Flags: 32-bit, virtual memory */
	    CL_reg(context) = si.wProcessorLevel;
	    DX_reg(context) = 0x0102;  /* Master/slave interrupt controller base*/
	    break;
	}
    case 0x0500:  /* Get free memory information */
        {
            MEMMANINFO mmi;

            mmi.dwSize = sizeof(mmi);
            MemManInfo(&mmi);
            ptr = (BYTE *)PTR_SEG_OFF_TO_LIN(ES_reg(context),DI_reg(context));
            /* the layout is just the same as MEMMANINFO, but without
             * the dwSize entry.
             */
            memcpy(ptr,((char*)&mmi)+4,sizeof(mmi)-4);
            break;
        }
    case 0x0501:  /* Allocate memory block */
        if (!(ptr = (BYTE *)HeapAlloc( GetProcessHeap(), 0,
                                       MAKELONG( CX_reg(context),
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
        HeapFree( GetProcessHeap(), 0,
                  (void *)MAKELONG( DI_reg(context), SI_reg(context) ) );
        break;

    case 0x0503:  /* Resize memory block */
        if (!(ptr = (BYTE *)HeapReAlloc( GetProcessHeap(), 0,
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
