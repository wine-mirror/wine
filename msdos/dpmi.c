/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "heap.h"
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "miscemu.h"
#include "drive.h"
#include "msdos.h"
#include "toolhelp.h"
#include "debug.h"
#include "selectors.h"
#include "thread.h"
#include "process.h"
#include "stackframe.h"
#include "callback.h"

#define DOS_GET_DRIVE(reg) ((reg) ? (reg) - 1 : DRIVE_GetCurrentDrive())

void CreateBPB(int drive, BYTE *data, BOOL16 limited);  /* defined in int21.c */

static void* lastvalloced = NULL;

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



typedef struct tagRMCB {
    DWORD address;
    struct tagRMCB *next;

} RMCB;

static RMCB *FirstRMCB = NULL;

/**********************************************************************
 *	    DPMI_xalloc
 * special virtualalloc, allocates lineary monoton growing memory.
 * (the usual VirtualAlloc does not satisfy that restriction)
 */
static LPVOID
DPMI_xalloc(int len) {
	LPVOID	ret;
	LPVOID	oldlastv = lastvalloced;

	if (lastvalloced) {
		int	xflag = 0;
		ret = NULL;
		while (!ret) {
			ret=VirtualAlloc(lastvalloced,len,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
			if (!ret)
				lastvalloced+=0x10000;
			/* we failed to allocate one in the first round. 
			 * try non-linear
			 */
			if (!xflag && (lastvalloced<oldlastv)) { /* wrapped */
				FIXME(int31,"failed to allocate lineary growing memory (%d bytes), using non-linear growing...\n",len);
				xflag++;
			}
			/* if we even fail to allocate something in the next 
			 * round, return NULL
			 */
			if ((xflag==1) && (lastvalloced >= oldlastv))
				xflag++;
			if ((xflag==2) && (lastvalloced < oldlastv)) {
				FIXME(int31,"failed to allocate any memory of %d bytes!\n",len);
				return NULL;
			}
		}
	} else
		 ret=VirtualAlloc(NULL,len,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	lastvalloced = (LPVOID)(((DWORD)ret+len+0xffff)&~0xffff);
	return ret;
}

static void
DPMI_xfree(LPVOID ptr) {
	VirtualFree(ptr,0,MEM_RELEASE);
}

/* FIXME: perhaps we could grow this mapped area... */
static LPVOID
DPMI_xrealloc(LPVOID ptr,int newsize) {
	MEMORY_BASIC_INFORMATION	mbi;
	LPVOID				newptr;

	newptr = DPMI_xalloc(newsize);
	if (ptr) {
		if (!VirtualQuery(ptr,&mbi,sizeof(mbi))) {
			FIXME(int31,"realloc of DPMI_xallocd region %p?\n",ptr);
			return NULL;
		}
		if (mbi.State == MEM_FREE) {
			FIXME(int31,"realloc of DPMI_xallocd region %p?\n",ptr);
			return NULL;
		}
		/* We do not shrink allocated memory. most reallocs
		 * only do grows anyway
		 */
		if (newsize<=mbi.RegionSize)
			return ptr;
		memcpy(newptr,ptr,mbi.RegionSize);
		DPMI_xfree(ptr);
	}
	return newptr;
}
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
            do_mscdex( &realmode_ctx, 1 );
            break;
        case 0x7a:
            /* NOVELL NetWare */
            switch (AL_reg(&realmode_ctx))
	    {
                case 0x20:  /* Get VLM Call Address */
                    /* return nothing -> NetWare not installed */
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
                        CreateBPB(drive, &dataptr[7], FALSE);
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
        case 0x58: /* GET OR SET MEMORY/UMB ALLOCATION STRATEGY */
            TRACE(int31,"GET OR SET MEMORY/UMB ALLOCATION STRATEGY subfunction %d\n",
                  AL_reg(context));
            switch (AL_reg(context))
            {
            case 0x00:
                AX_reg(context) = 1;
                break;
            case 0x02:
                AX_reg(context) = 0;
                break;
            case 0x01:
            case 0x03:
                break;
            }
            RESET_CFLAG(context);
            break;
	case 0x60: {/* CANONICALIZE PATH */
		LPCSTR path = (LPCSTR)DOSMEM_MapRealToLinear((DS_reg(&realmode_ctx)<<16)+SI_reg(&realmode_ctx));

		TRACE(int31,"TRUENAME %s\n",path);
		if (!GetFullPathName32A( 
		    path,
		    128,
		    (LPSTR)DOSMEM_MapRealToLinear(
		    (ES_reg(&realmode_ctx)<<16)+DI_reg(&realmode_ctx)),
		    NULL
		))
		    SET_CFLAG(context);
		else
		    AX_reg(&realmode_ctx) = 0;
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

    if (EFL_reg(context)&1) {
      FIXME(int31,"%02x: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n",
	    BL_reg(context), EAX_reg(&realmode_ctx), EBX_reg(&realmode_ctx), 
	    ECX_reg(&realmode_ctx), EDX_reg(&realmode_ctx));
      FIXME(int31,"      ESI=%08lx EDI=%08lx DS=%04lx ES=%04lx\n",
	    ESI_reg(&realmode_ctx), EDI_reg(&realmode_ctx), 
	    DS_reg(&realmode_ctx), ES_reg(&realmode_ctx) );
    }
    INT_SetRealModeContext( call, &realmode_ctx );
}


static void CallRMProcFar( CONTEXT *context )
{
    REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
    CONTEXT context16;
    THDB *thdb = THREAD_Current();
    WORD argsize, sel;
    LPVOID addr;
    SEGPTR seg_addr;

    TRACE(int31, "RealModeCall: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n",
	p->eax, p->ebx, p->ecx, p->edx);
    TRACE(int31, "              ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x, %d WORD arguments\n",
	p->esi, p->edi, p->es, p->ds, p->cs, p->ip, CX_reg(context) );

    if (!(p->cs) && !(p->ip)) { /* remove this check
                                   if Int21/6501 case map function
                                   has been implemented */
	SET_CFLAG(context);
	return;
    }
    INT_GetRealModeContext(p, &context16);

    addr = DOSMEM_MapRealToLinear(MAKELONG(p->ip, p->cs));
    sel = SELECTOR_AllocBlock( addr, 0x10000, SEGMENT_CODE, FALSE, FALSE );
    seg_addr = PTR_SEG_OFF_TO_SEGPTR( sel, 0 );

    CS_reg(&context16) = HIWORD(seg_addr);
    IP_reg(&context16) = LOWORD(seg_addr);
    EBP_reg(&context16) = OFFSETOF( thdb->cur_stack )
                               + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = CX_reg(context)*sizeof(WORD);
    memcpy( ((LPBYTE)THREAD_STACK16(thdb))-argsize,
    (LPBYTE)PTR_SEG_OFF_TO_LIN(SS_reg(context), SP_reg(context))+6, argsize );

    Callbacks->CallRegisterShortProc(&context16, argsize);

    UnMapLS(seg_addr);
    INT_SetRealModeContext(p, &context16);
}


void WINAPI RMCallbackProc( FARPROC16 pmProc, REALMODECALL *rmc )
{
    CONTEXT ctx;
    INT_GetRealModeContext(rmc, &ctx);
    Callbacks->CallRegisterShortProc(&ctx, 0);
}


static void AllocRMCB( CONTEXT *context )
{
    RMCB *NewRMCB = HeapAlloc(GetProcessHeap(), 0, sizeof(RMCB));
    REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
    UINT16 uParagraph;

    FIXME(int31, "EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n", p->eax, p->ebx, p->ecx, p->edx);
    FIXME(int31, "           ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n", p->esi, p->edi, p->es, p->ds, p->cs, p->ip);
    FIXME(int31, "           Function to call: %04x:%04x\n",
         (WORD)DS_reg(context), SI_reg(context) );

    if (NewRMCB)
    {
	LPVOID RMCBmem = DOSMEM_GetBlock(0, 20, &uParagraph);
	LPBYTE p = RMCBmem;

	*p++ = 0x68; /* pushl */
	*(FARPROC16 *)p =
	PTR_SEG_OFF_TO_LIN(ES_reg(context), SI_reg(context)); /* pmode proc to call */
	p+=4;
	*p++ = 0x68; /* pushl */
	*(LPVOID *)p =
	PTR_SEG_OFF_TO_LIN(ES_reg(context), DI_reg(context));
	p+=4;
	*p++ = 0x9a; /* lcall */
	*(FARPROC16 *)p = (FARPROC16)RMCallbackProc; /* FIXME ? */
	p+=4;
	GET_CS(*(WORD *)p);
	p+=2;
	*p++=0xc3; /* retf */
	NewRMCB->address = MAKELONG(0, uParagraph);
	NewRMCB->next = FirstRMCB;
	FirstRMCB = NewRMCB;
	CX_reg(context) = uParagraph;
	DX_reg(context) = 0;
    }
    else
    {
	AX_reg(context) = 0x8015; /* callback unavailable */
	SET_CFLAG(context);
    }
}


static void FreeRMCB( CONTEXT *context )
{
    RMCB *CurrRMCB = FirstRMCB;
    RMCB *PrevRMCB = NULL;

    FIXME(int31, "callback address: %04x:%04x\n",
          CX_reg(context), DX_reg(context));

    while (CurrRMCB && (CurrRMCB->address != MAKELONG(DX_reg(context), CX_reg(context))))
    {
	PrevRMCB = CurrRMCB;
	CurrRMCB = CurrRMCB->next;
    }
    if (CurrRMCB)
    {
	if (PrevRMCB)
	PrevRMCB->next = CurrRMCB->next;
	    else
	FirstRMCB = CurrRMCB->next;
	DOSMEM_FreeBlock(0, DOSMEM_MapRealToLinear(CurrRMCB->address));
	HeapFree(GetProcessHeap(), 0, CurrRMCB);
    }
    else
    {
	AX_reg(context) = 0x8024; /* invalid callback address */
	SET_CFLAG(context);
    }
}


/**********************************************************************
 *	    INT_Int31Handler
 *
 * Handler for int 31h (DPMI).
 */

void WINAPI INT_Int31Handler( CONTEXT *context )
{
    /*
     * Note: For Win32s processes, the whole linear address space is
     *       shifted by 0x10000 relative to the OS linear address space.
     *       See the comment in msdos/vxd.c.
     */
    DWORD offset = PROCESS_Current()->flags & PDB32_WIN32S_PROC ? 0x10000 : 0;
    #define AppToWine(addr) ((addr)? ((DWORD)(addr)) + offset : 0)
    #define WineToApp(addr) ((addr)? ((DWORD)(addr)) - offset : 0)

    DWORD dw;
    BYTE *ptr;

    RESET_CFLAG(context);
    switch(AX_reg(context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
    	TRACE(int31,"allocate LDT descriptors (%d)\n",CX_reg(context));
        if (!(AX_reg(context) = AllocSelectorArray( CX_reg(context) )))
        {
    	    TRACE(int31,"failed\n");
            AX_reg(context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
	TRACE(int31,"success, array starts at 0x%04x\n",AX_reg(context));
        break;

    case 0x0001:  /* Free LDT descriptor */
    	TRACE(int31,"free LDT descriptor (0x%04x)\n",BX_reg(context));
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
    	TRACE(int31,"real mode segment to descriptor (0x%04x)\n",BX_reg(context));
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
                AX_reg(context) = LOWORD(NE_GetEntryPoint( 
                                                 GetModuleHandle16( "KERNEL" ),
                                                 entryPoint ));
        }
        break;

    case 0x0003:  /* Get next selector increment */
    	TRACE(int31,"get selector increment (__AHINCR)\n");
        AX_reg(context) = __AHINCR;
        break;

    case 0x0004:  /* Lock selector (not supported) */
    	FIXME(int31,"lock selector not supported\n");
        AX_reg(context) = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0005:  /* Unlock selector (not supported) */
    	FIXME(int31,"unlock selector not supported\n");
        AX_reg(context) = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0006:  /* Get selector base address */
    	TRACE(int31,"get selector base address (0x%04x)\n",BX_reg(context));
        if (!(dw = GetSelectorBase( BX_reg(context) )))
        {
            AX_reg(context) = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            CX_reg(context) = HIWORD(WineToApp(dw));
            DX_reg(context) = LOWORD(WineToApp(dw));
        }
        break;

    case 0x0007:  /* Set selector base address */
    	TRACE(int31, "set selector base address (0x%04x,0x%08lx)\n",
                     BX_reg(context),
                     AppToWine(MAKELONG(DX_reg(context),CX_reg(context))));
        SetSelectorBase(BX_reg(context),
                        AppToWine(MAKELONG(DX_reg(context), CX_reg(context))));
        break;

    case 0x0008:  /* Set selector limit */
    	TRACE(int31,"set selector limit (0x%04x,0x%08lx)\n",BX_reg(context),MAKELONG(DX_reg(context),CX_reg(context)));
        SetSelectorLimit( BX_reg(context),
                          MAKELONG( DX_reg(context), CX_reg(context) ) );
        break;

    case 0x0009:  /* Set selector access rights */
    	TRACE(int31,"set selector access rights(0x%04x,0x%04x)\n",BX_reg(context),CX_reg(context));
        SelectorAccessRights( BX_reg(context), 1, CX_reg(context) );
        break;

    case 0x000a:  /* Allocate selector alias */
    	TRACE(int31,"allocate selector alias (0x%04x)\n",BX_reg(context));
        if (!(AX_reg(context) = AllocCStoDSAlias( BX_reg(context) )))
        {
            AX_reg(context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x000b:  /* Get descriptor */
    	TRACE(int31,"get descriptor (0x%04x)\n",BX_reg(context));
        {
            ldt_entry entry;
            LDT_GetEntry( SELECTOR_TO_ENTRY( BX_reg(context) ), &entry );
            entry.base = WineToApp(entry.base);

            /* FIXME: should use ES:EDI for 32-bit clients */
            LDT_EntryToBytes( PTR_SEG_OFF_TO_LIN( ES_reg(context),
                                                  DI_reg(context) ), &entry );
        }
        break;

    case 0x000c:  /* Set descriptor */
    	TRACE(int31,"set descriptor (0x%04x)\n",BX_reg(context));
        {
            ldt_entry entry;
            LDT_BytesToEntry( PTR_SEG_OFF_TO_LIN( ES_reg(context),
                                                  DI_reg(context) ), &entry );
            entry.base = AppToWine(entry.base);

            LDT_SetEntry( SELECTOR_TO_ENTRY( BX_reg(context) ), &entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
    	FIXME(int31,"allocate descriptor (0x%04x), stub!\n",BX_reg(context));
        AX_reg(context) = 0x8011; /* descriptor unavailable */
        SET_CFLAG(context);
        break;
    case 0x0200: /* get real mode interrupt vector */
	FIXME(int31,"get realmode interupt vector(0x%02x) unimplemented.\n",
	      BL_reg(context));
    	SET_CFLAG(context);
        break;
    case 0x0201: /* set real mode interrupt vector */
	FIXME(int31, "set realmode interupt vector(0x%02x,0x%04x:0x%04x) unimplemented\n", BL_reg(context),CX_reg(context),DX_reg(context));
    	SET_CFLAG(context);
        break;
    case 0x0204:  /* Get protected mode interrupt vector */
    	TRACE(int31,"get protected mode interrupt handler (0x%02x), stub!\n",BL_reg(context));
	dw = (DWORD)INT_GetHandler( BL_reg(context) );
	CX_reg(context) = HIWORD(dw);
	DX_reg(context) = LOWORD(dw);
	break;

    case 0x0205:  /* Set protected mode interrupt vector */
    	TRACE(int31,"set protected mode interrupt handler (0x%02x,%p), stub!\n",
            BL_reg(context),PTR_SEG_OFF_TO_LIN(CX_reg(context),DX_reg(context)));
	INT_SetHandler( BL_reg(context),
                        (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( CX_reg(context),
                                                          DX_reg(context) ));
	break;

    case 0x0300:  /* Simulate real mode interrupt */
        INT_DoRealModeInt( context );
        break;

    case 0x0301:  /* Call real mode procedure with far return */
	CallRMProcFar( context );
	break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
        {
            REALMODECALL *p = (REALMODECALL *)PTR_SEG_OFF_TO_LIN( ES_reg(context), DI_reg(context) );
            FIXME(int31, "RealModeCallIret: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n", p->eax, p->ebx, p->ecx, p->edx);
            FIXME(int31, "                  ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x\n", p->esi, p->edi, p->es, p->ds, p->cs, p->ip );
            SET_CFLAG(context);
        }
        break;

    case 0x0303:  /* Allocate Real Mode Callback Address */
	AllocRMCB( context );
	break;

    case 0x0304:  /* Free Real Mode Callback Address */
	FreeRMCB( context );
	break;

    case 0x0400:  /* Get DPMI version */
        TRACE(int31,"get DPMI version\n");
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
        TRACE(int31,"get free memory information\n");
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
        TRACE(int31,"allocate memory block (%ld)\n",MAKELONG(CX_reg(context),BX_reg(context)));
	if (!(ptr = (BYTE *)DPMI_xalloc(MAKELONG(CX_reg(context), BX_reg(context)))))
        {
            AX_reg(context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(context);
        } else {
            BX_reg(context) = SI_reg(context) = HIWORD(WineToApp(ptr));
            CX_reg(context) = DI_reg(context) = LOWORD(WineToApp(ptr));
        }
        break;

    case 0x0502:  /* Free memory block */
        TRACE(int31, "free memory block (0x%08lx)\n",
                     AppToWine(MAKELONG(DI_reg(context),SI_reg(context))));
	DPMI_xfree( (void *)AppToWine(MAKELONG(DI_reg(context), 
                                               SI_reg(context))) );
        break;

    case 0x0503:  /* Resize memory block */
        TRACE(int31, "resize memory block (0x%08lx,%ld)\n",
                     AppToWine(MAKELONG(DI_reg(context),SI_reg(context))),
                     MAKELONG(CX_reg(context),BX_reg(context)));
        if (!(ptr = (BYTE *)DPMI_xrealloc(
                (void *)AppToWine(MAKELONG(DI_reg(context),SI_reg(context))),
                MAKELONG(CX_reg(context),BX_reg(context)))))
        {
            AX_reg(context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(context);
        } else {
            BX_reg(context) = SI_reg(context) = HIWORD(WineToApp(ptr));
            CX_reg(context) = DI_reg(context) = LOWORD(WineToApp(ptr));
        }
        break;

    case 0x0600:  /* Lock linear region */
        FIXME(int31,"lock linear region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0601:  /* Unlock linear region */
        FIXME(int31,"unlock linear region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0602:  /* Unlock real-mode region */
        FIXME(int31,"unlock realmode region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0603:  /* Lock real-mode region */
        FIXME(int31,"lock realmode region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0604:  /* Get page size */
        TRACE(int31,"get pagesize\n");
        BX_reg(context) = 0;
        CX_reg(context) = VIRTUAL_GetPageSize();
	break;

    case 0x0702:  /* Mark page as demand-paging candidate */
        FIXME(int31,"mark page as demand-paging candidate\n");
        break;  /* Just ignore it */

    case 0x0703:  /* Discard page contents */
        FIXME(int31,"discard page contents\n");
        break;  /* Just ignore it */

     case 0x0800:  /* Physical address mapping */
        FIXME(int31,"map real to linear (0x%08lx)\n",MAKELONG(CX_reg(context),BX_reg(context)));
         if(!(ptr=DOSMEM_MapRealToLinear(MAKELONG(CX_reg(context),BX_reg(context)))))
         {
             AX_reg(context) = 0x8021; 
             SET_CFLAG(context);
         }
         else
         {
             BX_reg(context) = HIWORD(WineToApp(ptr));
             CX_reg(context) = LOWORD(WineToApp(ptr));
             RESET_CFLAG(context);
         }
         break;

    default:
        INT_BARF( context, 0x31 );
        AX_reg(context) = 0x8001;  /* unsupported function */
        SET_CFLAG(context);
        break;
    }

    #undef AppToWine
    #undef WineToApp
}
