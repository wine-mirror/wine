/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <unistd.h>
#include <string.h>

#include "config.h"
#include "windef.h"
#include "wine/winbase16.h"
#include "wine/port.h"
#include "builtin16.h"
#include "miscemu.h"
#include "msdos.h"
#include "dosexe.h"
#include "task.h"
#include "toolhelp.h"
#include "selectors.h"
#include "callback.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int31);

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
    DWORD proc_ofs,proc_sel;
    DWORD regs_ofs,regs_sel;
    struct tagRMCB *next;
} RMCB;

static RMCB *FirstRMCB = NULL;


static LPDOSTASK WINAPI DPMI_NoCurrent(void) { return NULL; }
DOSVM_TABLE Dosvm = { DPMI_NoCurrent };

static HMODULE DosModule;

/**********************************************************************
 *	    DPMI_LoadDosSystem
 */
BOOL DPMI_LoadDosSystem(void)
{
    if (DosModule) return TRUE;
    DosModule = LoadLibraryA( "winedos.dll" );
    if (!DosModule) {
        ERR("could not load winedos.dll, DOS subsystem unavailable\n");
        return FALSE;
    }
    Dosvm.Current    = (void *)GetProcAddress(DosModule, "GetCurrent");
    Dosvm.LoadDPMI   = (void *)GetProcAddress(DosModule, "LoadDPMI");
    Dosvm.LoadDosExe = (void *)GetProcAddress(DosModule, "LoadDosExe");
    Dosvm.Exec       = (void *)GetProcAddress(DosModule, "Exec");
    Dosvm.Exit       = (void *)GetProcAddress(DosModule, "Exit");
    Dosvm.Enter      = (void *)GetProcAddress(DosModule, "Enter");
    Dosvm.Wait       = (void *)GetProcAddress(DosModule, "Wait");
    Dosvm.QueueEvent = (void *)GetProcAddress(DosModule, "QueueEvent");
    Dosvm.OutPIC     = (void *)GetProcAddress(DosModule, "OutPIC");
    Dosvm.SetTimer   = (void *)GetProcAddress(DosModule, "SetTimer");
    Dosvm.GetTimer   = (void *)GetProcAddress(DosModule, "GetTimer");
    return TRUE;
}

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
				lastvalloced = (char *) lastvalloced + 0x10000;
			/* we failed to allocate one in the first round. 
			 * try non-linear
			 */
			if (!xflag && (lastvalloced<oldlastv)) { /* wrapped */
				FIXME("failed to allocate lineary growing memory (%d bytes), using non-linear growing...\n",len);
				xflag++;
			}
			/* if we even fail to allocate something in the next 
			 * round, return NULL
			 */
			if ((xflag==1) && (lastvalloced >= oldlastv))
				xflag++;
			if ((xflag==2) && (lastvalloced < oldlastv)) {
				FIXME("failed to allocate any memory of %d bytes!\n",len);
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
DPMI_xrealloc(LPVOID ptr,DWORD newsize) {
	MEMORY_BASIC_INFORMATION	mbi;
	LPVOID				newptr;

	newptr = DPMI_xalloc(newsize);
	if (ptr) {
		if (!VirtualQuery(ptr,&mbi,sizeof(mbi))) {
			FIXME("realloc of DPMI_xallocd region %p?\n",ptr);
			return NULL;
		}
		if (mbi.State == MEM_FREE) {
			FIXME("realloc of DPMI_xallocd region %p?\n",ptr);
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
static void INT_GetRealModeContext( REALMODECALL *call, CONTEXT86 *context )
{
    context->Eax    = call->eax;
    context->Ebx    = call->ebx;
    context->Ecx    = call->ecx;
    context->Edx    = call->edx;
    context->Esi    = call->esi;
    context->Edi    = call->edi;
    context->Ebp    = call->ebp;
    context->EFlags = call->fl | V86_FLAG;
    context->Eip    = call->ip;
    context->Esp    = call->sp;
    context->SegCs  = call->cs;
    context->SegDs  = call->ds;
    context->SegEs  = call->es;
    context->SegFs  = call->fs;
    context->SegGs  = call->gs;
    context->SegSs  = call->ss;
}


/**********************************************************************
 *	    INT_SetRealModeContext
 */
static void INT_SetRealModeContext( REALMODECALL *call, CONTEXT86 *context )
{
    call->eax = context->Eax;
    call->ebx = context->Ebx;
    call->ecx = context->Ecx;
    call->edx = context->Edx;
    call->esi = context->Esi;
    call->edi = context->Edi;
    call->ebp = context->Ebp;
    call->fl  = LOWORD(context->EFlags);
    call->ip  = LOWORD(context->Eip);
    call->sp  = LOWORD(context->Esp);
    call->cs  = context->SegCs;
    call->ds  = context->SegDs;
    call->es  = context->SegEs;
    call->fs  = context->SegFs;
    call->gs  = context->SegGs;
    call->ss  = context->SegSs;
}

#ifdef __i386__

void DPMI_CallRMCB32(RMCB *rmcb, UINT16 ss, DWORD esp, UINT16*es, DWORD*edi)
#if 0 /* original code, which early gccs puke on */
{
    int _clobber;
    __asm__ __volatile__(
        "pushl %%ebp\n"
        "pushl %%ebx\n"
        "pushl %%es\n"
        "pushl %%ds\n"
        "pushfl\n"
        "mov %7,%%es\n"
        "mov %5,%%ds\n"
        ".byte 0x36, 0xff, 0x18\n" /* lcall *%ss:(%eax) */
        "popl %%ds\n"
        "mov %%es,%0\n"
        "popl %%es\n"
        "popl %%ebx\n"
        "popl %%ebp\n"
    : "=d" (*es), "=D" (*edi), "=S" (_clobber), "=a" (_clobber), "=c" (_clobber)
    : "0" (ss), "2" (esp),
      "4" (rmcb->regs_sel), "1" (rmcb->regs_ofs),
      "3" (&rmcb->proc_ofs) );
}
#else /* code generated by a gcc new enough */
;
__ASM_GLOBAL_FUNC(DPMI_CallRMCB32,
    "pushl %ebp\n\t"
    "movl %esp,%ebp\n\t"
    "pushl %edi\n\t"
    "pushl %esi\n\t"
    "movl 0x8(%ebp),%eax\n\t"
    "movl 0x10(%ebp),%esi\n\t"
    "movl 0xc(%ebp),%edx\n\t"
    "movl 0x10(%eax),%ecx\n\t"
    "movl 0xc(%eax),%edi\n\t"
    "addl $0x4,%eax\n\t"
    "pushl %ebp\n\t"
    "pushl %ebx\n\t"
    "pushl %es\n\t"
    "pushl %ds\n\t"
    "pushfl\n\t"
    "mov %cx,%es\n\t"
    "mov %dx,%ds\n\t"
    ".byte 0x36, 0xff, 0x18\n\t" /* lcall *%ss:(%eax) */
    "popl %ds\n\t"
    "mov %es,%dx\n\t"
    "popl %es\n\t"
    "popl %ebx\n\t"
    "popl %ebp\n\t"
    "movl 0x14(%ebp),%eax\n\t"
    "movw %dx,(%eax)\n\t"
    "movl 0x18(%ebp),%edx\n\t"
    "movl %edi,(%edx)\n\t"
    "popl %esi\n\t"
    "popl %edi\n\t"
    "leave\n\t"
    "ret")
#endif

#endif /* __i386__ */

/**********************************************************************
 *	    DPMI_CallRMCBProc
 *
 * This routine does the hard work of calling a callback procedure.
 */
static void DPMI_CallRMCBProc( CONTEXT86 *context, RMCB *rmcb, WORD flag )
{
    if (IS_SELECTOR_SYSTEM( rmcb->proc_sel )) {
        /* Wine-internal RMCB, call directly */
        ((RMCBPROC)rmcb->proc_ofs)(context);
    } else {
#ifdef __i386__
        UINT16 ss,es;
        DWORD esp,edi;

        INT_SetRealModeContext(MapSL(MAKESEGPTR( rmcb->regs_sel, rmcb->regs_ofs )), context);
        ss = SELECTOR_AllocBlock( (void *)(context->SegSs<<4), 0x10000, WINE_LDT_FLAGS_DATA );
        esp = context->Esp;

        FIXME("untested!\n");

        /* The called proc ends with an IRET, and takes these parameters:
         * DS:ESI = pointer to real-mode SS:SP
         * ES:EDI = pointer to real-mode call structure
         * It returns:
         * ES:EDI = pointer to real-mode call structure (may be a copy)
         * It is the proc's responsibility to change the return CS:IP in the
         * real-mode call structure. */
        if (flag & 1) {
            /* 32-bit DPMI client */
            DPMI_CallRMCB32(rmcb, ss, esp, &es, &edi);
        } else {
            /* 16-bit DPMI client */
            CONTEXT86 ctx = *context;
            ctx.SegCs = rmcb->proc_sel;
            ctx.Eip   = rmcb->proc_ofs;
            ctx.SegDs = ss;
            ctx.Esi   = esp;
            ctx.SegEs = rmcb->regs_sel;
            ctx.Edi   = rmcb->regs_ofs;
            /* FIXME: I'm pretty sure this isn't right - should push flags first */
            wine_call_to_16_regs_short(&ctx, 0);
            es = ctx.SegEs;
            edi = ctx.Edi;
        }
	FreeSelector16(ss);
        INT_GetRealModeContext( MapSL( MAKESEGPTR( es, edi )), context);
#else
        ERR("RMCBs only implemented for i386\n");
#endif
    }
}


/**********************************************************************
 *	    DPMI_CallRMProc
 *
 * This routine does the hard work of calling a real mode procedure.
 */
int DPMI_CallRMProc( CONTEXT86 *context, LPWORD stack, int args, int iret )
{
    LPWORD stack16;
    LPVOID addr = NULL; /* avoid gcc warning */
    LPDOSTASK lpDosTask = Dosvm.Current();
    RMCB *CurrRMCB;
    int alloc = 0, already = 0;
    BYTE *code;

    TRACE("EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n",
                 context->Eax, context->Ebx, context->Ecx, context->Edx );
    TRACE("ESI=%08lx EDI=%08lx ES=%04lx DS=%04lx CS:IP=%04lx:%04x, %d WORD arguments, %s\n",
                 context->Esi, context->Edi, context->SegEs, context->SegDs,
                 context->SegCs, LOWORD(context->Eip), args, iret?"IRET":"FAR" );

callrmproc_again:

/* there might be some code that just jumps to RMCBs or the like,
   in which case following the jumps here might get us to a shortcut */
    code = CTX_SEG_OFF_TO_LIN(context, context->SegCs, context->Eip);
    switch (*code) {
    case 0xe9: /* JMP NEAR */
      context->Eip += 3 + *(WORD *)(code+1);
      /* yeah, I know these gotos don't look good... */
      goto callrmproc_again;
    case 0xea: /* JMP FAR */
      context->Eip = *(WORD *)(code+1);
      context->SegCs = *(WORD *)(code+3);
      /* ...but since the label is there anyway... */
      goto callrmproc_again;
    case 0xeb: /* JMP SHORT */
      context->Eip += 2 + *(signed char *)(code+1);
      /* ...because of other gotos below, so... */
      goto callrmproc_again;
    }

/* shortcut for chaining to internal interrupt handlers */
    if ((context->SegCs == 0xF000) && iret) {
        return INT_RealModeInterrupt( LOWORD(context->Eip)/4, context);
    }

/* shortcut for RMCBs */
    CurrRMCB = FirstRMCB;

    while (CurrRMCB && (HIWORD(CurrRMCB->address) != context->SegCs))
        CurrRMCB = CurrRMCB->next;

    if (!(CurrRMCB || lpDosTask)) {
        FIXME("DPMI real-mode call using DOS VM task system, not fully tested!\n");
        TRACE("creating VM86 task\n");
        if ((!DPMI_LoadDosSystem()) || !(lpDosTask = Dosvm.LoadDPMI() )) {
            ERR("could not setup VM86 task\n");
            return 1;
        }
    }
    if (!already) {
        if (!context->SegSs) {
            alloc = 1; /* allocate default stack */
            stack16 = addr = DOSMEM_GetBlock( 64, (UINT16 *)&(context->SegSs) );
            context->Esp = 64-2;
            stack16 += 32-1;
            if (!addr) {
                ERR("could not allocate default stack\n");
                return 1;
            }
        } else {
            stack16 = CTX_SEG_OFF_TO_LIN(context, context->SegSs, context->Esp);
        }
        context->Esp -= (args + (iret?1:0)) * sizeof(WORD);
        stack16 -= args;
        if (args) memcpy(stack16, stack, args*sizeof(WORD) );
        /* push flags if iret */
        if (iret) {
            stack16--; args++;
            *stack16 = LOWORD(context->EFlags);
        }
        /* push return address (return to interrupt wrapper) */
        *(--stack16) = DOSMEM_wrap_seg;
        *(--stack16) = 0;
        /* adjust stack */
        context->Esp -= 2*sizeof(WORD);
        already = 1;
    }

    if (CurrRMCB) {
        /* RMCB call, invoke protected-mode handler directly */
        DPMI_CallRMCBProc(context, CurrRMCB, lpDosTask ? lpDosTask->dpmi_flag : 0);
        /* check if we returned to where we thought we would */
        if ((context->SegCs != DOSMEM_wrap_seg) ||
            (LOWORD(context->Eip) != 0)) {
            /* we need to continue at different address in real-mode space,
               so we need to set it all up for real mode again */
            goto callrmproc_again;
        }
    } else {
        TRACE("entering real mode...\n");
        Dosvm.Enter( context );
        TRACE("returned from real-mode call\n");
    }
    if (alloc) DOSMEM_FreeBlock( addr );
    return 0;
}


/**********************************************************************
 *	    CallRMInt
 */
static void CallRMInt( CONTEXT86 *context )
{
    CONTEXT86 realmode_ctx;
    FARPROC16 rm_int = INT_GetRMHandler( BL_reg(context) );
    REALMODECALL *call = MapSL( MAKESEGPTR( context->SegEs, DI_reg(context) ));
    INT_GetRealModeContext( call, &realmode_ctx );

    /* we need to check if a real-mode program has hooked the interrupt */
    if (HIWORD(rm_int)!=0xF000) {
        /* yup, which means we need to switch to real mode... */
        realmode_ctx.SegCs = HIWORD(rm_int);
        realmode_ctx.Eip   = LOWORD(rm_int);
        if (DPMI_CallRMProc( &realmode_ctx, NULL, 0, TRUE))
          SET_CFLAG(context);
    } else {
        RESET_CFLAG(context);
        /* use the IP we have instead of BL_reg, in case some apps
           decide to move interrupts around for whatever reason... */
        if (INT_RealModeInterrupt( LOWORD(rm_int)/4, &realmode_ctx ))
          SET_CFLAG(context);
        if (context->EFlags & 1) {
          FIXME("%02x: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n",
                BL_reg(context), realmode_ctx.Eax, realmode_ctx.Ebx,
                realmode_ctx.Ecx, realmode_ctx.Edx);
          FIXME("      ESI=%08lx EDI=%08lx DS=%04lx ES=%04lx\n",
                realmode_ctx.Esi, realmode_ctx.Edi,
                realmode_ctx.SegDs, realmode_ctx.SegEs );
        }
    }
    INT_SetRealModeContext( call, &realmode_ctx );
}


static void CallRMProc( CONTEXT86 *context, int iret )
{
    REALMODECALL *p = MapSL( MAKESEGPTR( context->SegEs, DI_reg(context) ));
    CONTEXT86 context16;

    TRACE("RealModeCall: EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n",
	p->eax, p->ebx, p->ecx, p->edx);
    TRACE("              ESI=%08lx EDI=%08lx ES=%04x DS=%04x CS:IP=%04x:%04x, %d WORD arguments, %s\n",
	p->esi, p->edi, p->es, p->ds, p->cs, p->ip, CX_reg(context), iret?"IRET":"FAR" );

    if (!(p->cs) && !(p->ip)) { /* remove this check
                                   if Int21/6501 case map function
                                   has been implemented */
	SET_CFLAG(context);
	return;
    }
    INT_GetRealModeContext(p, &context16);
    DPMI_CallRMProc( &context16, ((LPWORD)MapSL(MAKESEGPTR(context->SegSs, LOWORD(context->Esp))))+3,
                     CX_reg(context), iret );
    INT_SetRealModeContext(p, &context16);
}


static RMCB *DPMI_AllocRMCB( void )
{
    RMCB *NewRMCB = HeapAlloc(GetProcessHeap(), 0, sizeof(RMCB));
    UINT16 uParagraph;

    if (NewRMCB)
    {
	LPVOID RMCBmem = DOSMEM_GetBlock(4, &uParagraph);
	LPBYTE p = RMCBmem;

	*p++ = 0xcd; /* RMCB: */
	*p++ = 0x31; /* int $0x31 */
/* it is the called procedure's task to change the return CS:EIP
   the DPMI 0.9 spec states that if it doesn't, it will be called again */
	*p++ = 0xeb;
	*p++ = 0xfc; /* jmp RMCB */
	NewRMCB->address = MAKELONG(0, uParagraph);
	NewRMCB->next = FirstRMCB;
	FirstRMCB = NewRMCB;
    }
    return NewRMCB;
}


static void AllocRMCB( CONTEXT86 *context )
{
    RMCB *NewRMCB = DPMI_AllocRMCB();

    TRACE("Function to call: %04x:%04x\n", (WORD)context->SegDs, SI_reg(context) );

    if (NewRMCB)
    {
	/* FIXME: if 32-bit DPMI client, use ESI and EDI */
	NewRMCB->proc_ofs = LOWORD(context->Esi);
	NewRMCB->proc_sel = context->SegDs;
	NewRMCB->regs_ofs = LOWORD(context->Edi);
	NewRMCB->regs_sel = context->SegEs;
	SET_LOWORD( context->Ecx, HIWORD(NewRMCB->address) );
	SET_LOWORD( context->Edx, LOWORD(NewRMCB->address) );
    }
    else
    {
	SET_LOWORD( context->Eax, 0x8015 ); /* callback unavailable */
	SET_CFLAG(context);
    }
}


FARPROC16 WINAPI DPMI_AllocInternalRMCB( RMCBPROC proc )
{
    RMCB *NewRMCB = DPMI_AllocRMCB();

    if (NewRMCB) {
        NewRMCB->proc_ofs = (DWORD)proc;
        NewRMCB->proc_sel = 0;
        NewRMCB->regs_ofs = 0;
        NewRMCB->regs_sel = 0;
        return (FARPROC16)(NewRMCB->address);
    }
    return NULL;
}


static int DPMI_FreeRMCB( DWORD address )
{
    RMCB *CurrRMCB = FirstRMCB;
    RMCB *PrevRMCB = NULL;

    while (CurrRMCB && (CurrRMCB->address != address))
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
	DOSMEM_FreeBlock(DOSMEM_MapRealToLinear(CurrRMCB->address));
	HeapFree(GetProcessHeap(), 0, CurrRMCB);
	return 0;
    }
    return 1;
}


static void FreeRMCB( CONTEXT86 *context )
{
    FIXME("callback address: %04x:%04x\n",
          CX_reg(context), DX_reg(context));

    if (DPMI_FreeRMCB(MAKELONG(DX_reg(context), CX_reg(context)))) {
	SET_LOWORD( context->Eax, 0x8024 ); /* invalid callback address */
	SET_CFLAG(context);
    }
}


void WINAPI DPMI_FreeInternalRMCB( FARPROC16 proc )
{
    DPMI_FreeRMCB( (DWORD)proc );
}


/* (see dosmem.c, function DOSMEM_InitDPMI) */

static void StartPM( CONTEXT86 *context, LPDOSTASK lpDosTask )
{
    UINT16 cs, ss, ds, es;
    CONTEXT86 pm_ctx;
    DWORD psp_ofs = (DWORD)(lpDosTask->psp_seg<<4);
    PDB16 *psp = (PDB16 *)psp_ofs;
    HANDLE16 env_seg = psp->environment;
    unsigned char selflags = WINE_LDT_FLAGS_DATA;

    RESET_CFLAG(context);
    lpDosTask->dpmi_flag = AX_reg(context);
/* our mode switch wrapper have placed the desired CS into DX */
    cs = SELECTOR_AllocBlock( (void *)(DX_reg(context)<<4), 0x10000, WINE_LDT_FLAGS_CODE );
/* due to a flaw in some CPUs (at least mine), it is best to mark stack segments as 32-bit if they
   can be used in 32-bit code. Otherwise, these CPUs may not set the high word of esp during a
   ring transition (from kernel code) to the 16-bit stack, and this causes trouble if executing
   32-bit code using this stack. */
    if (lpDosTask->dpmi_flag & 1) selflags |= WINE_LDT_FLAGS_32BIT;
    ss = SELECTOR_AllocBlock( (void *)(context->SegSs<<4), 0x10000, selflags );
/* do the same for the data segments, just in case */
    if (context->SegDs == context->SegSs) ds = ss;
    else ds = SELECTOR_AllocBlock( (void *)(context->SegDs<<4), 0x10000, selflags );
    es = SELECTOR_AllocBlock( psp, 0x100, selflags );
/* convert environment pointer, as the spec says, but we're a bit lazy about the size here... */
    psp->environment = SELECTOR_AllocBlock( (void *)(env_seg<<4), 0x10000, WINE_LDT_FLAGS_DATA );

    pm_ctx = *context;
    pm_ctx.SegCs = DOSMEM_dpmi_sel;
/* our mode switch wrapper expects the new CS in DX, and the new SS in AX */
    pm_ctx.Eax   = ss;
    pm_ctx.Edx   = cs;
    pm_ctx.SegDs = ds;
    pm_ctx.SegEs = es;
    pm_ctx.SegFs = 0;
    pm_ctx.SegGs = 0;

    TRACE("DOS program is now entering protected mode\n");
    wine_call_to_16_regs_short(&pm_ctx, 0);

    /* in the current state of affairs, we won't ever actually return here... */
    /* we should have int21/ah=4c do it someday, though... */

    FreeSelector16(psp->environment);
    psp->environment = env_seg;
    FreeSelector16(es);
    if (ds != ss) FreeSelector16(ds);
    FreeSelector16(ss);
    FreeSelector16(cs);
}

/* DPMI Raw Mode Switch handler */

#if 0
void WINAPI DPMI_RawModeSwitch( SIGCONTEXT *context )
{
  LPDOSTASK lpDosTask = Dosvm.Current();
  CONTEXT86 rm_ctx;
  int ret;

  if (!lpDosTask) {
    /* we could probably start a DPMI-only dosmod task here, but I doubt
       anything other than real DOS apps want to call raw mode switch */
    ERR("attempting raw mode switch without DOS task!\n");
    ExitProcess(1);
  }
  /* initialize real-mode context as per spec */
  memset(&rm_ctx, 0, sizeof(rm_ctx));
  rm_ctx.SegDs  = AX_sig(context);
  rm_ctx.SegEs  = CX_sig(context);
  rm_ctx.SegSs  = DX_sig(context);
  rm_ctx.Esp    = EBX_sig(context);
  rm_ctx.SegCs  = SI_sig(context);
  rm_ctx.Eip    = EDI_sig(context);
  rm_ctx.Ebp    = EBP_sig(context);
  rm_ctx.SegFs  = 0;
  rm_ctx.SegGs  = 0;
  rm_ctx.EFlags = EFL_sig(context); /* at least we need the IF flag */

  /* enter real mode again */
  TRACE("re-entering real mode at %04lx:%04lx\n",rm_ctx.SegCs,rm_ctx.Eip);
  ret = Dosvm.Enter( &rm_ctx );
  /* when the real-mode stuff call its mode switch address,
     Dosvm.Enter will return and we will continue here */

  if (ret<0) {
    /* if the sync was lost, there's no way to recover */
    ExitProcess(1);
  }

  /* alter protected-mode context as per spec */
  DS_sig(context)  = LOWORD(rm_ctx.Eax);
  ES_sig(context)  = LOWORD(rm_ctx.Ecx);
  SS_sig(context)  = LOWORD(rm_ctx.Edx);
  ESP_sig(context) = rm_ctx.Ebx;
  CS_sig(context)  = LOWORD(rm_ctx.Esi);
  EIP_sig(context) = rm_ctx.Edi;
  EBP_sig(context) = rm_ctx.Ebp;
  FS_sig(context) = 0;
  GS_sig(context) = 0;

  /* Return to new address and hope that we didn't mess up */
  TRACE("re-entering protected mode at %04x:%08lx\n",
	CS_sig(context), EIP_sig(context));
}
#endif


/**********************************************************************
 *	    INT_Int31Handler (WPROCS.149)
 *
 * Handler for int 31h (DPMI).
 */

void WINAPI INT_Int31Handler( CONTEXT86 *context )
{
    /*
     * Note: For Win32s processes, the whole linear address space is
     *       shifted by 0x10000 relative to the OS linear address space.
     *       See the comment in msdos/vxd.c.
     */
    DWORD dw;
    BYTE *ptr;

    LPDOSTASK lpDosTask = Dosvm.Current();

    if (ISV86(context) && lpDosTask) {
        /* Called from real mode, check if it's our wrapper */
        TRACE("called from real mode\n");
        if (context->SegCs==DOSMEM_dpmi_seg) {
            /* This is the protected mode switch */
            StartPM(context,lpDosTask);
            return;
        } else
        if (context->SegCs==DOSMEM_xms_seg) {
            /* This is the XMS driver entry point */
            XMS_Handler(context);
            return;
        } else
        {
            /* Check for RMCB */
            RMCB *CurrRMCB = FirstRMCB;
            
            while (CurrRMCB && (HIWORD(CurrRMCB->address) != context->SegCs))
                CurrRMCB = CurrRMCB->next;
            
            if (CurrRMCB) {
                /* RMCB call, propagate to protected-mode handler */
                DPMI_CallRMCBProc(context, CurrRMCB, lpDosTask->dpmi_flag);
                return;
            }
        }
    }

    RESET_CFLAG(context);
    switch(AX_reg(context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
    	TRACE("allocate LDT descriptors (%d)\n",CX_reg(context));
        if (!(context->Eax = AllocSelectorArray16( CX_reg(context) )))
        {
    	    TRACE("failed\n");
            context->Eax = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
	TRACE("success, array starts at 0x%04x\n",AX_reg(context));
        break;

    case 0x0001:  /* Free LDT descriptor */
    	TRACE("free LDT descriptor (0x%04x)\n",BX_reg(context));
        if (FreeSelector16( BX_reg(context) ))
        {
            context->Eax = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            /* If a segment register contains the selector being freed, */
            /* set it to zero. */
            if (!((context->SegDs^BX_reg(context)) & ~3)) context->SegDs = 0;
            if (!((context->SegEs^BX_reg(context)) & ~3)) context->SegEs = 0;
            if (!((context->SegFs^BX_reg(context)) & ~3)) context->SegFs = 0;
            if (!((context->SegGs^BX_reg(context)) & ~3)) context->SegGs = 0;
        }
        break;

    case 0x0002:  /* Real mode segment to descriptor */
    	TRACE("real mode segment to descriptor (0x%04x)\n",BX_reg(context));
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
	    	context->Eax = DOSMEM_AllocSelector(BX_reg(context));
                break;
            }
            if (entryPoint) 
                context->Eax = LOWORD(GetProcAddress16( GetModuleHandle16( "KERNEL" ),
                                                        (LPCSTR)(ULONG_PTR)entryPoint ));
        }
        break;

    case 0x0003:  /* Get next selector increment */
    	TRACE("get selector increment (__AHINCR)\n");
        context->Eax = __AHINCR;
        break;

    case 0x0004:  /* Lock selector (not supported) */
    	FIXME("lock selector not supported\n");
        context->Eax = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0005:  /* Unlock selector (not supported) */
    	FIXME("unlock selector not supported\n");
        context->Eax = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0006:  /* Get selector base address */
    	TRACE("get selector base address (0x%04x)\n",BX_reg(context));
        if (!(dw = GetSelectorBase( BX_reg(context) )))
        {
            context->Eax = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            CX_reg(context) = HIWORD(W32S_WINE2APP(dw));
            DX_reg(context) = LOWORD(W32S_WINE2APP(dw));
        }
        break;

    case 0x0007:  /* Set selector base address */
    	TRACE("set selector base address (0x%04x,0x%08lx)\n",
                     BX_reg(context),
                     W32S_APP2WINE(MAKELONG(DX_reg(context),CX_reg(context))));
        dw = W32S_APP2WINE(MAKELONG(DX_reg(context), CX_reg(context)));
        if (dw < 0x10000)
            /* app wants to access lower 64K of DOS memory, map it in now */
            DOSMEM_Init(TRUE);
        SetSelectorBase(BX_reg(context), dw);
        break;

    case 0x0008:  /* Set selector limit */
    	TRACE("set selector limit (0x%04x,0x%08lx)\n",BX_reg(context),MAKELONG(DX_reg(context),CX_reg(context)));
        dw = MAKELONG( DX_reg(context), CX_reg(context) );
        SetSelectorLimit16( BX_reg(context), dw );
        break;

    case 0x0009:  /* Set selector access rights */
    	TRACE("set selector access rights(0x%04x,0x%04x)\n",BX_reg(context),CX_reg(context));
        SelectorAccessRights16( BX_reg(context), 1, CX_reg(context) );
        break;

    case 0x000a:  /* Allocate selector alias */
    	TRACE("allocate selector alias (0x%04x)\n",BX_reg(context));
        if (!(AX_reg(context) = AllocCStoDSAlias16( BX_reg(context) )))
        {
            AX_reg(context) = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x000b:  /* Get descriptor */
    	TRACE("get descriptor (0x%04x)\n",BX_reg(context));
        {
            LDT_ENTRY entry;
            wine_ldt_set_base( &entry, (void*)W32S_WINE2APP(wine_ldt_get_base(&entry)) );
            /* FIXME: should use ES:EDI for 32-bit clients */
            *(LDT_ENTRY *)MapSL( MAKESEGPTR( context->SegEs, LOWORD(context->Edi) )) = entry;
        }
        break;

    case 0x000c:  /* Set descriptor */
    	TRACE("set descriptor (0x%04x)\n",BX_reg(context));
        {
            LDT_ENTRY entry = *(LDT_ENTRY *)MapSL(MAKESEGPTR( context->SegEs, LOWORD(context->Edi)));
            wine_ldt_set_base( &entry, (void*)W32S_APP2WINE(wine_ldt_get_base(&entry)) );
            wine_ldt_set_entry( LOWORD(context->Ebx), &entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
    	FIXME("allocate descriptor (0x%04x), stub!\n",BX_reg(context));
        AX_reg(context) = 0x8011; /* descriptor unavailable */
        SET_CFLAG(context);
        break;
    case 0x0100:  /* Allocate DOS memory block */
        TRACE("allocate DOS memory block (0x%x paragraphs)\n",BX_reg(context));
        dw = GlobalDOSAlloc16((DWORD)BX_reg(context)<<4);
        if (dw) {
            AX_reg(context) = HIWORD(dw);
            DX_reg(context) = LOWORD(dw);
        } else {
            AX_reg(context) = 0x0008; /* insufficient memory */
            BX_reg(context) = DOSMEM_Available()>>4;
            SET_CFLAG(context);
        }
        break;
    case 0x0101:  /* Free DOS memory block */
        TRACE("free DOS memory block (0x%04x)\n",DX_reg(context));
        dw = GlobalDOSFree16(DX_reg(context));
        if (!dw) {
            AX_reg(context) = 0x0009; /* memory block address invalid */
            SET_CFLAG(context);
        }
        break;
    case 0x0200: /* get real mode interrupt vector */
	FIXME("get realmode interupt vector(0x%02x) unimplemented.\n",
	      BL_reg(context));
    	SET_CFLAG(context);
        break;
    case 0x0201: /* set real mode interrupt vector */
	FIXME("set realmode interrupt vector(0x%02x,0x%04x:0x%04x) unimplemented\n", BL_reg(context),CX_reg(context),DX_reg(context));
    	SET_CFLAG(context);
        break;
    case 0x0204:  /* Get protected mode interrupt vector */
    	TRACE("get protected mode interrupt handler (0x%02x), stub!\n",BL_reg(context));
	dw = (DWORD)INT_GetPMHandler( BL_reg(context) );
	CX_reg(context) = HIWORD(dw);
	DX_reg(context) = LOWORD(dw);
	break;

    case 0x0205:  /* Set protected mode interrupt vector */
    	TRACE("set protected mode interrupt handler (0x%02x,%p), stub!\n",
            BL_reg(context),MapSL(MAKESEGPTR(CX_reg(context),DX_reg(context))));
	INT_SetPMHandler( BL_reg(context),
                          (FARPROC16)MAKESEGPTR( CX_reg(context), DX_reg(context) ));
	break;

    case 0x0300:  /* Simulate real mode interrupt */
        CallRMInt( context );
        break;

    case 0x0301:  /* Call real mode procedure with far return */
	CallRMProc( context, FALSE );
	break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
	CallRMProc( context, TRUE );
        break;

    case 0x0303:  /* Allocate Real Mode Callback Address */
	AllocRMCB( context );
	break;

    case 0x0304:  /* Free Real Mode Callback Address */
	FreeRMCB( context );
	break;

    case 0x0305:  /* Get State Save/Restore Addresses */
        TRACE("get state save/restore addresses\n");
        /* we probably won't need this kind of state saving */
        AX_reg(context) = 0;
	/* real mode: just point to the lret */
	BX_reg(context) = DOSMEM_wrap_seg;
	context->Ecx = 2;
	/* protected mode: don't have any handler yet... */
	FIXME("no protected-mode dummy state save/restore handler yet\n");
	SI_reg(context) = 0;
	context->Edi = 0;
        break;

    case 0x0306:  /* Get Raw Mode Switch Addresses */
        TRACE("get raw mode switch addresses\n");
        /* real mode, point to standard DPMI return wrapper */
        BX_reg(context) = DOSMEM_wrap_seg;
        context->Ecx = 0;
        /* protected mode, point to DPMI call wrapper */
        SI_reg(context) = DOSMEM_dpmi_sel;
        context->Edi = 8; /* offset of the INT 0x31 call */
	break;
    case 0x0400:  /* Get DPMI version */
        TRACE("get DPMI version\n");
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
        TRACE("get free memory information\n");
        {
            MEMMANINFO mmi;

            mmi.dwSize = sizeof(mmi);
            MemManInfo16(&mmi);
            ptr = MapSL(MAKESEGPTR(context->SegEs,DI_reg(context)));
            /* the layout is just the same as MEMMANINFO, but without
             * the dwSize entry.
             */
            memcpy(ptr,((char*)&mmi)+4,sizeof(mmi)-4);
            break;
        }
    case 0x0501:  /* Allocate memory block */
        TRACE("allocate memory block (%ld)\n",MAKELONG(CX_reg(context),BX_reg(context)));
	if (!(ptr = (BYTE *)DPMI_xalloc(MAKELONG(CX_reg(context), BX_reg(context)))))
        {
            AX_reg(context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(context);
        } else {
            BX_reg(context) = SI_reg(context) = HIWORD(W32S_WINE2APP(ptr));
            CX_reg(context) = DI_reg(context) = LOWORD(W32S_WINE2APP(ptr));
        }
        break;

    case 0x0502:  /* Free memory block */
        TRACE("free memory block (0x%08lx)\n",
                     W32S_APP2WINE(MAKELONG(DI_reg(context),SI_reg(context))));
	DPMI_xfree( (void *)W32S_APP2WINE(MAKELONG(DI_reg(context), SI_reg(context))) );
        break;

    case 0x0503:  /* Resize memory block */
        TRACE("resize memory block (0x%08lx,%ld)\n",
                     W32S_APP2WINE(MAKELONG(DI_reg(context),SI_reg(context))),
                     MAKELONG(CX_reg(context),BX_reg(context)));
        if (!(ptr = (BYTE *)DPMI_xrealloc(
                (void *)W32S_APP2WINE(MAKELONG(DI_reg(context),SI_reg(context))),
                MAKELONG(CX_reg(context),BX_reg(context)))))
        {
            AX_reg(context) = 0x8012;  /* linear memory not available */
            SET_CFLAG(context);
        } else {
            BX_reg(context) = SI_reg(context) = HIWORD(W32S_WINE2APP(ptr));
            CX_reg(context) = DI_reg(context) = LOWORD(W32S_WINE2APP(ptr));
        }
        break;

    case 0x0507:  /* Modify page attributes */
        FIXME("modify page attributes unimplemented\n");
        break;  /* Just ignore it */

    case 0x0600:  /* Lock linear region */
        FIXME("lock linear region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0601:  /* Unlock linear region */
        FIXME("unlock linear region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0602:  /* Unlock real-mode region */
        FIXME("unlock realmode region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0603:  /* Lock real-mode region */
        FIXME("lock realmode region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0604:  /* Get page size */
        TRACE("get pagesize\n");
        BX_reg(context) = 0;
        CX_reg(context) = getpagesize();
	break;

    case 0x0702:  /* Mark page as demand-paging candidate */
        FIXME("mark page as demand-paging candidate\n");
        break;  /* Just ignore it */

    case 0x0703:  /* Discard page contents */
        FIXME("discard page contents\n");
        break;  /* Just ignore it */

     case 0x0800:  /* Physical address mapping */
        FIXME("map real to linear (0x%08lx)\n",MAKELONG(CX_reg(context),BX_reg(context)));
         if(!(ptr=DOSMEM_MapRealToLinear(MAKELONG(CX_reg(context),BX_reg(context)))))
         {
             AX_reg(context) = 0x8021; 
             SET_CFLAG(context);
         }
         else
         {
             BX_reg(context) = HIWORD(W32S_WINE2APP(ptr));
             CX_reg(context) = LOWORD(W32S_WINE2APP(ptr));
             RESET_CFLAG(context);
         }
         break;

    default:
        INT_BARF( context, 0x31 );
        AX_reg(context) = 0x8001;  /* unsupported function */
        SET_CFLAG(context);
        break;
    }

}
