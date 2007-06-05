/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "dosexe.h"

#include "excpt.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "thread.h"

WINE_DEFAULT_DEBUG_CHANNEL(int31);

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
static WORD dpmi_flag;
static void* lastvalloced = NULL;
static BYTE DPMI_retval;

/**********************************************************************
 *          DOSVM_IsDos32
 * 
 * Return TRUE if we are in 32-bit protected mode DOS process.
 */
BOOL DOSVM_IsDos32(void)
{
  return (dpmi_flag & 1) ? TRUE : FALSE;
}


/**********************************************************************
 *          alloc_pm_selector
 *
 * Allocate a 64k sized selector corresponding to a real mode segment.
 */
static WORD alloc_pm_selector( WORD seg, unsigned char flags )
{
    WORD sel = wine_ldt_alloc_entries( 1 );

    if (sel)
    {
        LDT_ENTRY entry;
        wine_ldt_set_base( &entry, (void *)(seg << 4) );
        wine_ldt_set_limit( &entry, 0xffff );
        wine_ldt_set_flags( &entry, flags );
        wine_ldt_set_entry( sel, &entry );
    }
    return sel;
}


/**********************************************************************
 *          dpmi_exception_handler
 *
 * Handle EXCEPTION_VM86_STI exceptions generated
 * when there are pending asynchronous events.
 */
static WINE_EXCEPTION_FILTER(dpmi_exception_handler)
{
#ifdef __i386__
    EXCEPTION_RECORD *rec = GetExceptionInformation()->ExceptionRecord;
    CONTEXT *context = GetExceptionInformation()->ContextRecord;

    if (rec->ExceptionCode == EXCEPTION_VM86_STI)
    {
        if (ISV86(context))
            ERR( "Real mode STI caught by protected mode handler!\n" );
        DOSVM_SendQueuedEvents(context);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    else if (rec->ExceptionCode == EXCEPTION_VM86_INTx)
    {
        if (ISV86(context))
            ERR( "Real mode INTx caught by protected mode handler!\n" );
        DPMI_retval = (BYTE)rec->ExceptionInformation[0];
        return EXCEPTION_EXECUTE_HANDLER;
    }

#endif
    return EXCEPTION_CONTINUE_SEARCH;
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

/**********************************************************************
 *          DPMI_xalloc
 * special virtualalloc, allocates lineary monoton growing memory.
 * (the usual VirtualAlloc does not satisfy that restriction)
 */
static LPVOID DPMI_xalloc( DWORD len ) 
{
    LPVOID  ret;
    LPVOID  oldlastv = lastvalloced;

    if (lastvalloced) 
    {
        int xflag = 0;

        ret = NULL;
        while (!ret) 
        {
            ret = VirtualAlloc( lastvalloced, len,
                                MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE );
            if (!ret)
                lastvalloced = (char *) lastvalloced + 0x10000;

            /* we failed to allocate one in the first round.
             * try non-linear
             */
            if (!xflag && (lastvalloced<oldlastv)) 
            { 
                /* wrapped */
                FIXME( "failed to allocate linearly growing memory (%d bytes), "
                       "using non-linear growing...\n", len );
                xflag++;
            }

            /* if we even fail to allocate something in the next
             * round, return NULL
             */
            if ((xflag==1) && (lastvalloced >= oldlastv))
                xflag++;

            if ((xflag==2) && (lastvalloced < oldlastv)) {
                FIXME( "failed to allocate any memory of %d bytes!\n", len );
                return NULL;
            }
        }
    } 
    else
    {
        ret = VirtualAlloc( NULL, len, 
                            MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    }

    lastvalloced = (LPVOID)(((DWORD)ret+len+0xffff)&~0xffff);
    return ret;
}

/**********************************************************************
 *          DPMI_xfree
 */
static void DPMI_xfree( LPVOID ptr ) 
{
    VirtualFree( ptr, 0, MEM_RELEASE );
}

/**********************************************************************
 *          DPMI_xrealloc
 *
 * FIXME: perhaps we could grow this mapped area... 
 */
static LPVOID DPMI_xrealloc( LPVOID ptr, DWORD newsize ) 
{
    MEMORY_BASIC_INFORMATION        mbi;
    LPVOID                          newptr;

    newptr = DPMI_xalloc( newsize );
    if (ptr) 
    {
        if (!VirtualQuery(ptr,&mbi,sizeof(mbi))) 
        {
            FIXME( "realloc of DPMI_xallocd region %p?\n", ptr );
            return NULL;
        }

        if (mbi.State == MEM_FREE) 
        {
            FIXME( "realloc of DPMI_xallocd region %p?\n", ptr );
            return NULL;
        }

        /* We do not shrink allocated memory. most reallocs
         * only do grows anyway
         */
        if (newsize <= mbi.RegionSize)
            return ptr;

        memcpy( newptr, ptr, mbi.RegionSize );
        DPMI_xfree( ptr );
    }

    return newptr;
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
    DWORD old_vif = NtCurrentTeb()->dpmi_vif;

    /* Disable virtual interrupts. */
    NtCurrentTeb()->dpmi_vif = 0;

    if (wine_ldt_is_system( rmcb->proc_sel )) {
        /* Wine-internal RMCB, call directly */
        ((RMCBPROC)rmcb->proc_ofs)(context);
    } else __TRY {
#ifdef __i386__
        UINT16 ss,es;
        DWORD esp,edi;

        INT_SetRealModeContext(MapSL(MAKESEGPTR( rmcb->regs_sel, rmcb->regs_ofs )), context);
        ss = alloc_pm_selector( context->SegSs, WINE_LDT_FLAGS_DATA );
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
            WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)&ctx );
            es = ctx.SegEs;
            edi = ctx.Edi;
        }
        wine_ldt_free_entries( ss, 1 );
        INT_GetRealModeContext( MapSL( MAKESEGPTR( es, edi )), context);
#else
        ERR("RMCBs only implemented for i386\n");
#endif
    } __EXCEPT(dpmi_exception_handler) { } __ENDTRY
        
    /* Restore virtual interrupt flag. */
    NtCurrentTeb()->dpmi_vif = old_vif;                                 
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
    RMCB *CurrRMCB;
    int alloc = 0, already = 0;
    BYTE *code;

    TRACE("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\n",
                 context->Eax, context->Ebx, context->Ecx, context->Edx );
    TRACE("ESI=%08x EDI=%08x ES=%04x DS=%04x CS:IP=%04x:%04x, %d WORD arguments, %s\n",
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
    if ((context->SegCs == 0xF000) && iret)
    {
        DOSVM_CallBuiltinHandler( context, LOWORD(context->Eip)/4 );
        return 0;
    }

/* shortcut for RMCBs */
    CurrRMCB = FirstRMCB;

    while (CurrRMCB && (HIWORD(CurrRMCB->address) != context->SegCs))
        CurrRMCB = CurrRMCB->next;

    if (!CurrRMCB && !MZ_Current())
    {
        FIXME("DPMI real-mode call using DOS VM task system, not fully tested!\n");
        TRACE("creating VM86 task\n");
        MZ_AllocDPMITask();
    }
    if (!already) {
        if (!context->SegSs) {
            alloc = 1; /* allocate default stack */
            stack16 = addr = DOSMEM_AllocBlock( 64, (UINT16 *)&(context->SegSs) );
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
        *(--stack16) = DOSVM_dpmi_segments->wrap_seg;
        *(--stack16) = 0;
        /* adjust stack */
        context->Esp -= 2*sizeof(WORD);
        already = 1;
    }

    if (CurrRMCB) {
        /* RMCB call, invoke protected-mode handler directly */
        DPMI_CallRMCBProc(context, CurrRMCB, dpmi_flag);
        /* check if we returned to where we thought we would */
        if ((context->SegCs != DOSVM_dpmi_segments->wrap_seg) ||
            (LOWORD(context->Eip) != 0)) {
            /* we need to continue at different address in real-mode space,
               so we need to set it all up for real mode again */
            goto callrmproc_again;
        }
    } else {
        TRACE("entering real mode...\n");
        DOSVM_Enter( context );
        TRACE("returned from real-mode call\n");
    }
    if (alloc) DOSMEM_FreeBlock( addr );
    return 0;
}


/**********************************************************************
 *	    CallRMInt
 */
static void DOSVM_CallRMInt( CONTEXT86 *context )
{
    CONTEXT86 realmode_ctx;
    FARPROC16 rm_int = DOSVM_GetRMHandler( BL_reg(context) );
    REALMODECALL *call = CTX_SEG_OFF_TO_LIN( context, 
                                             context->SegEs, 
                                             context->Edi );
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
        DOSVM_CallBuiltinHandler( &realmode_ctx, LOWORD(rm_int)/4 );
    }
    INT_SetRealModeContext( call, &realmode_ctx );
}


/**********************************************************************
 *	    CallRMProc
 */
static void DOSVM_CallRMProc( CONTEXT86 *context, int iret )
{
    REALMODECALL *p = CTX_SEG_OFF_TO_LIN( context, 
                                          context->SegEs, 
                                          context->Edi );
    CONTEXT86 context16;

    TRACE("RealModeCall: EAX=%08x EBX=%08x ECX=%08x EDX=%08x\n",
          p->eax, p->ebx, p->ecx, p->edx);
    TRACE("              ESI=%08x EDI=%08x ES=%04x DS=%04x CS:IP=%04x:%04x, %d WORD arguments, %s\n",
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


/* (see dosmem.c, function DOSMEM_InitDPMI) */
static void StartPM( CONTEXT86 *context )
{
    UINT16 cs, ss, ds, es;
    CONTEXT86 pm_ctx;
    DWORD psp_ofs = (DWORD)(DOSVM_psp<<4);
    PDB16 *psp = (PDB16 *)psp_ofs;
    HANDLE16 env_seg = psp->environment;
    unsigned char selflags = WINE_LDT_FLAGS_DATA;

    RESET_CFLAG(context);
    dpmi_flag = AX_reg(context);
/* our mode switch wrapper have placed the desired CS into DX */
    cs = alloc_pm_selector( context->Edx, WINE_LDT_FLAGS_CODE );
/* due to a flaw in some CPUs (at least mine), it is best to mark stack segments as 32-bit if they
   can be used in 32-bit code. Otherwise, these CPUs may not set the high word of esp during a
   ring transition (from kernel code) to the 16-bit stack, and this causes trouble if executing
   32-bit code using this stack. */
    if (dpmi_flag & 1) selflags |= WINE_LDT_FLAGS_32BIT;
    ss = alloc_pm_selector( context->SegSs, selflags );
/* do the same for the data segments, just in case */
    if (context->SegDs == context->SegSs) ds = ss;
    else ds = alloc_pm_selector( context->SegDs, selflags );
    es = alloc_pm_selector( DOSVM_psp, selflags );
/* convert environment pointer, as the spec says, but we're a bit lazy about the size here... */
    psp->environment = alloc_pm_selector( env_seg, WINE_LDT_FLAGS_DATA );

    pm_ctx = *context;
    pm_ctx.SegCs = DOSVM_dpmi_segments->dpmi_sel;
/* our mode switch wrapper expects the new CS in DX, and the new SS in AX */
    pm_ctx.Eax   = ss;
    pm_ctx.Edx   = cs;
    pm_ctx.SegDs = ds;
    pm_ctx.SegEs = es;
    pm_ctx.SegFs = wine_get_fs();
    pm_ctx.SegGs = wine_get_gs();
    pm_ctx.EFlags &= ~V86_FLAG;

    TRACE("DOS program is now entering %d-bit protected mode\n", 
          DOSVM_IsDos32() ? 32 : 16);

    __TRY 
    {
        WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)&pm_ctx );
    } 
    __EXCEPT(dpmi_exception_handler) 
    { 
    } 
    __ENDTRY

    TRACE( "Protected mode DOS program is terminating\n" );

    /*
     * FIXME: Instead of calling ExitThread, we should release all
     *        allocated protected mode resources and call MZ_Exit
     *        using real mode context. See DPMI specification.
     */
    ExitThread( DPMI_retval );

#if 0
    wine_ldt_free_entries( psp->environment, 1 );
    psp->environment = env_seg;
    wine_ldt_free_entries(es,1);
    if (ds != ss) wine_ldt_free_entries(ds,1);
    wine_ldt_free_entries(ss,1);
    wine_ldt_free_entries(cs,1);
#endif
}

static RMCB *DPMI_AllocRMCB( void )
{
    RMCB *NewRMCB = HeapAlloc(GetProcessHeap(), 0, sizeof(RMCB));
    UINT16 uParagraph;

    if (NewRMCB)
    {
	LPVOID RMCBmem = DOSMEM_AllocBlock(4, &uParagraph);
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
	DOSMEM_FreeBlock(PTR_REAL_TO_LIN(SELECTOROF(CurrRMCB->address),OFFSETOF(CurrRMCB->address)));
	HeapFree(GetProcessHeap(), 0, CurrRMCB);
	return 0;
    }
    return 1;
}


void WINAPI DPMI_FreeInternalRMCB( FARPROC16 proc )
{
    DPMI_FreeRMCB( (DWORD)proc );
}


/**********************************************************************
 *	    DOSVM_RawModeSwitchHandler
 *
 * DPMI Raw Mode Switch handler
 */
void WINAPI DOSVM_RawModeSwitchHandler( CONTEXT86 *context )
{
  CONTEXT86 rm_ctx;
  int ret;

  /* initialize real-mode context as per spec */
  memset(&rm_ctx, 0, sizeof(rm_ctx));
  rm_ctx.SegDs  = AX_reg(context);
  rm_ctx.SegEs  = CX_reg(context);
  rm_ctx.SegSs  = DX_reg(context);
  rm_ctx.Esp    = context->Ebx;
  rm_ctx.SegCs  = SI_reg(context);
  rm_ctx.Eip    = context->Edi;
  rm_ctx.Ebp    = context->Ebp;
  rm_ctx.SegFs  = 0;
  rm_ctx.SegGs  = 0;

  /* Copy interrupt state. */
  if (NtCurrentTeb()->dpmi_vif)
      rm_ctx.EFlags = V86_FLAG | VIF_MASK;
  else
      rm_ctx.EFlags = V86_FLAG;

  /* enter real mode again */
  TRACE("re-entering real mode at %04x:%04x\n",rm_ctx.SegCs,rm_ctx.Eip);
  ret = DOSVM_Enter( &rm_ctx );
  /* when the real-mode stuff call its mode switch address,
     DOSVM_Enter will return and we will continue here */

  if (ret<0) {
    ERR("Sync lost!\n");
    /* if the sync was lost, there's no way to recover */
    ExitProcess(1);
  }

  /* alter protected-mode context as per spec */
  context->SegDs   = LOWORD(rm_ctx.Eax);
  context->SegEs   = LOWORD(rm_ctx.Ecx);
  context->SegSs   = LOWORD(rm_ctx.Edx);
  context->Esp     = rm_ctx.Ebx;
  context->SegCs   = LOWORD(rm_ctx.Esi);
  context->Eip     = rm_ctx.Edi;
  context->Ebp     = rm_ctx.Ebp;
  context->SegFs   = 0;
  context->SegGs   = 0;

  /* Copy interrupt state. */
  if (rm_ctx.EFlags & VIF_MASK)
      NtCurrentTeb()->dpmi_vif = 1;
  else
      NtCurrentTeb()->dpmi_vif = 0;

  /* Return to new address and hope that we didn't mess up */
  TRACE("re-entering protected mode at %04x:%08x\n",
      context->SegCs, context->Eip);
}


/**********************************************************************
 *	    AllocRMCB
 */
static void DOSVM_AllocRMCB( CONTEXT86 *context )
{
    RMCB *NewRMCB = DPMI_AllocRMCB();

    TRACE("Function to call: %04x:%04x\n", (WORD)context->SegDs, SI_reg(context) );

    if (NewRMCB)
    {
       NewRMCB->proc_ofs = DOSVM_IsDos32() ? context->Esi : LOWORD(context->Esi);
	NewRMCB->proc_sel = context->SegDs;
       NewRMCB->regs_ofs = DOSVM_IsDos32() ? context->Edi : LOWORD(context->Edi);
	NewRMCB->regs_sel = context->SegEs;
	SET_CX( context, HIWORD(NewRMCB->address) );
	SET_DX( context, LOWORD(NewRMCB->address) );
    }
    else
    {
	SET_AX( context, 0x8015 ); /* callback unavailable */
	SET_CFLAG(context);
    }
}


/**********************************************************************
 *	    FreeRMCB
 */
static void DOSVM_FreeRMCB( CONTEXT86 *context )
{
    FIXME("callback address: %04x:%04x\n",
          CX_reg(context), DX_reg(context));

    if (DPMI_FreeRMCB(MAKELONG(DX_reg(context), CX_reg(context)))) {
	SET_AX( context, 0x8024 ); /* invalid callback address */
	SET_CFLAG(context);
    }
}


/**********************************************************************
 *         DOSVM_CheckWrappers
 *
 * Check if this was really a wrapper call instead of an interrupt.
 */
BOOL DOSVM_CheckWrappers( CONTEXT86 *context )
{
    if (context->SegCs==DOSVM_dpmi_segments->dpmi_seg) {
        /* This is the protected mode switch */
        StartPM(context);
        return TRUE;
    }
    else if (context->SegCs==DOSVM_dpmi_segments->xms_seg)
    {
        /* This is the XMS driver entry point */
        XMS_Handler(context);
        return TRUE;
    }
    else
    {
        /* Check for RMCB */
        RMCB *CurrRMCB = FirstRMCB;

        while (CurrRMCB && (HIWORD(CurrRMCB->address) != context->SegCs))
            CurrRMCB = CurrRMCB->next;

        if (CurrRMCB) {
            /* RMCB call, propagate to protected-mode handler */
            DPMI_CallRMCBProc(context, CurrRMCB, dpmi_flag);
            return TRUE;
        }
    }

    return FALSE;
}

/**********************************************************************
 *         DOSVM_Int31Handler (WINEDOS16.149)
 *
 * Handler for int 31h (DPMI).
 */
void WINAPI DOSVM_Int31Handler( CONTEXT86 *context )
{
    RESET_CFLAG(context);
    switch(AX_reg(context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
        TRACE( "allocate LDT descriptors (%d)\n", CX_reg(context) );
        {
            WORD sel =  AllocSelectorArray16( CX_reg(context) );
            if(!sel) 
            {
               TRACE( "failed\n" );
               SET_AX( context, 0x8011 ); /* descriptor unavailable */
               SET_CFLAG( context );
            } 
            else 
            { 
                TRACE( "success, array starts at 0x%04x\n", sel );
                SET_AX( context, sel );      
            }
        }
        break;

    case 0x0001:  /* Free LDT descriptor */
        TRACE( "free LDT descriptor (0x%04x)\n", BX_reg(context) );
        if (FreeSelector16( BX_reg(context) ))
        {
            SET_AX( context, 0x8022 );  /* invalid selector */
            SET_CFLAG( context );
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
        TRACE( "real mode segment to descriptor (0x%04x)\n", BX_reg(context) );
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
                FIXME("Real mode segment (%x) to descriptor: no longer supported\n",
                      BX_reg(context));
                SET_CFLAG( context );
                break;
            }
            if (entryPoint)
            {
                FARPROC16 proc = GetProcAddress16( GetModuleHandle16( "KERNEL" ),
                                                   (LPCSTR)(ULONG_PTR)entryPoint );
                SET_AX( context, LOWORD(proc) );
            }
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
        TRACE( "get selector base address (0x%04x)\n", BX_reg(context) );
        {
            LDT_ENTRY entry;
            WORD sel = BX_reg(context);
            wine_ldt_get_entry( sel, &entry );
            if (wine_ldt_is_empty(&entry))
            {
                context->Eax = 0x8022;  /* invalid selector */
                SET_CFLAG(context);
            }
            else
            {
                void *base = wine_ldt_get_base(&entry);
                SET_CX( context, HIWORD(base) );
                SET_DX( context, LOWORD(base) );
            }
        }
        break;

    case 0x0007:  /* Set selector base address */
        {
            DWORD base = MAKELONG( DX_reg(context), CX_reg(context) );
            WORD  sel = BX_reg(context);
            TRACE( "set selector base address (0x%04x,0x%08x)\n", sel, base );

            /* check if Win16 app wants to access lower 64K of DOS memory */
            if (base < 0x10000 && DOSVM_IsWin16())
                DOSMEM_MapDosLayout();

            SetSelectorBase( sel, base );
        }
        break;

    case 0x0008:  /* Set selector limit */
        {
            DWORD limit = MAKELONG( DX_reg(context), CX_reg(context) );
            TRACE( "set selector limit (0x%04x,0x%08x)\n",
                   BX_reg(context), limit );
            SetSelectorLimit16( BX_reg(context), limit );
        }
        break;

    case 0x0009:  /* Set selector access rights */
        TRACE( "set selector access rights(0x%04x,0x%04x)\n",
               BX_reg(context), CX_reg(context) );
        SelectorAccessRights16( BX_reg(context), 1, CX_reg(context) );
        break;

    case 0x000a:  /* Allocate selector alias */
        TRACE( "allocate selector alias (0x%04x)\n", BX_reg(context) );
        SET_AX( context, AllocCStoDSAlias16( BX_reg(context) ) );
        if (!AX_reg(context))
        {
            SET_AX( context, 0x8011 );  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x000b:  /* Get descriptor */
        TRACE( "get descriptor (0x%04x)\n", BX_reg(context) );
        {
            LDT_ENTRY *entry = (LDT_ENTRY*)CTX_SEG_OFF_TO_LIN( context,
                                                               context->SegEs, 
                                                               context->Edi );
            wine_ldt_get_entry( BX_reg(context), entry );
        }
        break;

    case 0x000c:  /* Set descriptor */
        TRACE( "set descriptor (0x%04x)\n", BX_reg(context) );
        {
            LDT_ENTRY *entry = (LDT_ENTRY*)CTX_SEG_OFF_TO_LIN( context,
                                                               context->SegEs, 
                                                               context->Edi );
            wine_ldt_set_entry( BX_reg(context), entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
        FIXME( "allocate descriptor (0x%04x), stub!\n", BX_reg(context) );
        SET_AX( context, 0x8011 ); /* descriptor unavailable */
        SET_CFLAG( context );
        break;

    case 0x000e:  /* Get Multiple Descriptors (1.0) */
        FIXME( "get multiple descriptors - unimplemented\n" );
        break;

    case 0x000f:  /* Set Multiple Descriptors (1.0) */
        FIXME( "set multiple descriptors - unimplemented\n" );
        break;

    case 0x0100:  /* Allocate DOS memory block */
        TRACE( "allocate DOS memory block (0x%x paragraphs)\n", BX_reg(context) );
        {
            DWORD dw = GlobalDOSAlloc16( (DWORD)BX_reg(context) << 4 );
            if (dw) {
                SET_AX( context, HIWORD(dw) );
                SET_DX( context, LOWORD(dw) );
            } else {
                SET_AX( context, 0x0008 ); /* insufficient memory */
                SET_BX( context, DOSMEM_Available() >> 4 );
                SET_CFLAG(context);
            }
            break;
        }

    case 0x0101:  /* Free DOS memory block */
        TRACE( "free DOS memory block (0x%04x)\n", DX_reg(context) );
        {
            WORD error = GlobalDOSFree16( DX_reg(context) );
            if (error) {
                SET_AX( context, 0x0009 ); /* memory block address invalid */
                SET_CFLAG( context );
            }
        }
        break;

    case 0x0102: /* Resize DOS Memory Block */
        FIXME( "resize DOS memory block (0x%04x, 0x%x paragraphs) - unimplemented\n", 
               DX_reg(context), BX_reg(context) );
        break;

    case 0x0200: /* get real mode interrupt vector */
        TRACE( "get realmode interupt vector (0x%02x)\n",
               BL_reg(context) );
        {
            FARPROC16 proc = DOSVM_GetRMHandler( BL_reg(context) );
            SET_CX( context, SELECTOROF(proc) );
            SET_DX( context, OFFSETOF(proc) );
        }
        break;

    case 0x0201: /* set real mode interrupt vector */
        TRACE( "set realmode interrupt vector (0x%02x, 0x%04x:0x%04x)\n", 
               BL_reg(context), CX_reg(context), DX_reg(context) );
        DOSVM_SetRMHandler( BL_reg(context), 
                            (FARPROC16)MAKESEGPTR(CX_reg(context), DX_reg(context)) );
        break;

    case 0x0202:  /* Get Processor Exception Handler Vector */
        FIXME( "Get Processor Exception Handler Vector (0x%02x)\n",
               BL_reg(context) );
        if (DOSVM_IsDos32()) 
        {
            SET_CX( context, 0 );
            context->Edx = 0;
        } 
        else 
        {
            SET_CX( context, 0 );
            SET_DX( context, 0 );
        }
        break;

    case 0x0203:  /* Set Processor Exception Handler Vector */
         FIXME( "Set Processor Exception Handler Vector (0x%02x)\n",
                BL_reg(context) );
         break;

    case 0x0204:  /* Get protected mode interrupt vector */
        TRACE("get protected mode interrupt handler (0x%02x)\n",
              BL_reg(context));
        if (DOSVM_IsDos32()) 
        {
            FARPROC48 handler = DOSVM_GetPMHandler48( BL_reg(context) );
            SET_CX( context, handler.selector );
            context->Edx = handler.offset;
        } 
        else 
        {
            FARPROC16 handler = DOSVM_GetPMHandler16( BL_reg(context) );
            SET_CX( context, SELECTOROF(handler) );
            SET_DX( context, OFFSETOF(handler) );
        }
        break;

    case 0x0205:  /* Set protected mode interrupt vector */
        TRACE("set protected mode interrupt handler (0x%02x,0x%04x:0x%08x)\n",
              BL_reg(context), CX_reg(context), context->Edx);
        if (DOSVM_IsDos32()) 
        {
            FARPROC48 handler;
            handler.selector = CX_reg(context);
            handler.offset = context->Edx;
            DOSVM_SetPMHandler48( BL_reg(context), handler );
        } 
        else 
        {
            FARPROC16 handler;
            handler = (FARPROC16)MAKESEGPTR( CX_reg(context), DX_reg(context)); 
            DOSVM_SetPMHandler16( BL_reg(context), handler );
        }
        break;

    case 0x0300:  /* Simulate real mode interrupt */
        TRACE( "Simulate real mode interrupt %02x.\n", BL_reg(context));
        DOSVM_CallRMInt( context );
        break;

    case 0x0301:  /* Call real mode procedure with far return */
        TRACE( "Call real mode procedure with far return.\n" );
        DOSVM_CallRMProc( context, FALSE );
        break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
        TRACE( "Call real mode procedure with interrupt return.\n" );
        DOSVM_CallRMProc( context, TRUE );
        break;

    case 0x0303:  /* Allocate Real Mode Callback Address */
        TRACE( "Allocate real mode callback address.\n" );
        DOSVM_AllocRMCB( context );
        break;

    case 0x0304:  /* Free Real Mode Callback Address */
        TRACE( "Free real mode callback address.\n" );
        DOSVM_FreeRMCB( context );
        break;

    case 0x0305:  /* Get State Save/Restore Addresses */
        TRACE("get state save/restore addresses\n");
        /* we probably won't need this kind of state saving */
        SET_AX( context, 0 );

        /* real mode: just point to the lret */
        SET_BX( context, DOSVM_dpmi_segments->wrap_seg );
        SET_CX( context, 2 );

        /* protected mode: don't have any handler yet... */
        /* FIXME: Use DI in 16-bit DPMI and EDI in 32-bit DPMI */
        FIXME("no protected-mode dummy state save/restore handler yet\n");
        SET_SI( context, 0 );
        context->Edi = 0;
        break;

    case 0x0306:  /* Get Raw Mode Switch Addresses */
        TRACE("get raw mode switch addresses\n");

        /* real mode, point to standard DPMI return wrapper */
        SET_BX( context, DOSVM_dpmi_segments->wrap_seg );
        SET_CX( context, 0 );

        /* protected mode, point to DPMI call wrapper */
        /* FIXME: Use DI in 16-bit DPMI and EDI in 32-bit DPMI */
        /* FIXME: Doesn't work in DPMI32... */
        SET_SI( context, DOSVM_dpmi_segments->dpmi_sel );
        context->Edi = 8; /* offset of the INT 0x31 call */
        break;

    case 0x0400:  /* Get DPMI version */
        TRACE("get DPMI version\n");
        {
            SYSTEM_INFO si;

            GetSystemInfo(&si);
            SET_AX( context, 0x005a );  /* DPMI version 0.90 */
            SET_BX( context, 0x0005 );  /* Flags: 32-bit, virtual memory */
            SET_CL( context, si.wProcessorLevel );
            SET_DX( context, 0x0870 );  /* Master/slave interrupt controller base */
        }
        break;

    case 0x0401:  /* Get DPMI Capabilities (1.0) */
        FIXME( "get dpmi capabilities - unimplemented\n");
        break;

    case 0x0500:  /* Get free memory information */
        TRACE("get free memory information\n");
        {
            MEMORYSTATUS status;

            /* the layout is just the same as MEMMANINFO, but without
             * the dwSize entry.
             */
            struct
            {
                DWORD dwLargestFreeBlock;
                DWORD dwMaxPagesAvailable;
                DWORD dwMaxPagesLockable;
                DWORD dwTotalLinearSpace;
                DWORD dwTotalUnlockedPages;
                DWORD dwFreePages;
                DWORD dwTotalPages;
                DWORD dwFreeLinearSpace;
                DWORD dwSwapFilePages;
                WORD  wPageSize;
            } *info = CTX_SEG_OFF_TO_LIN( context, context->SegEs, context->Edi );

            GlobalMemoryStatus( &status );
            info->wPageSize            = getpagesize();
            info->dwLargestFreeBlock   = status.dwAvailVirtual;
            info->dwMaxPagesAvailable  = info->dwLargestFreeBlock / info->wPageSize;
            info->dwMaxPagesLockable   = info->dwMaxPagesAvailable;
            info->dwTotalLinearSpace   = status.dwTotalVirtual / info->wPageSize;
            info->dwTotalUnlockedPages = info->dwTotalLinearSpace;
            info->dwFreePages          = info->dwMaxPagesAvailable;
            info->dwTotalPages         = info->dwTotalLinearSpace;
            info->dwFreeLinearSpace    = info->dwMaxPagesAvailable;
            info->dwSwapFilePages      = status.dwTotalPageFile / info->wPageSize;
            break;
        }

    case 0x0501:  /* Allocate memory block */
        {
            DWORD size = MAKELONG( CX_reg(context), BX_reg(context) );
            BYTE *ptr;

            TRACE( "allocate memory block (%d bytes)\n", size );

            ptr = (BYTE *)DPMI_xalloc( size );
            if (!ptr)
            {
                SET_AX( context, 0x8012 );  /* linear memory not available */
                SET_CFLAG(context);
            } 
            else 
            {
                SET_BX( context, HIWORD(ptr) );
                SET_CX( context, LOWORD(ptr) );
                SET_SI( context, HIWORD(ptr) );
                SET_DI( context, LOWORD(ptr) );
            }
            break;
        }

    case 0x0502:  /* Free memory block */
        {
            DWORD handle = MAKELONG( DI_reg(context), SI_reg(context) );
            TRACE( "free memory block (0x%08x)\n", handle );
            DPMI_xfree( (void *)handle );
        }
        break;

    case 0x0503:  /* Resize memory block */
        {
            DWORD size = MAKELONG( CX_reg(context), BX_reg(context) );
            DWORD handle = MAKELONG( DI_reg(context), SI_reg(context) );
            BYTE *ptr;

            TRACE( "resize memory block (0x%08x, %d bytes)\n", handle, size );

            ptr = (BYTE *)DPMI_xrealloc( (void *)handle, size );
            if (!ptr)
            {
                SET_AX( context, 0x8012 );  /* linear memory not available */
                SET_CFLAG(context);
            } else {
                SET_BX( context, HIWORD(ptr) );
                SET_CX( context, LOWORD(ptr) );
                SET_SI( context, HIWORD(ptr) );
                SET_DI( context, LOWORD(ptr) );
            }
        }
        break;

    case 0x0507:  /* Set page attributes (1.0) */
        FIXME( "set page attributes - unimplemented\n" );
        break;  /* Just ignore it */

    case 0x0600:  /* Lock linear region */
        TRACE( "lock linear region - ignored (no paging)\n" );
        break;

    case 0x0601:  /* Unlock linear region */
        TRACE( "unlock linear region - ignored (no paging)\n" );
        break;

    case 0x0602:  /* Mark real mode region as pageable */
        TRACE( "mark real mode region as pageable - ignored (no paging)\n" );
        break;

    case 0x0603:  /* Relock real mode region */
        TRACE( "relock real mode region - ignored (no paging)\n" );
        break;

    case 0x0604:  /* Get page size */
        TRACE("get pagesize\n");
        SET_BX( context, HIWORD(getpagesize()) );
        SET_CX( context, LOWORD(getpagesize()) );
        break;

    case 0x0700: /* Mark pages as paging candidates */
        TRACE( "mark pages as paging candidates - ignored (no paging)\n" );
        break;

    case 0x0701: /* Discard pages */
        TRACE( "discard pages - ignored (no paging)\n" );
        break;

    case 0x0702:  /* Mark page as demand-paging candidate */
        TRACE( "mark page as demand-paging candidate - ignored (no paging)\n" );
        break;

    case 0x0703:  /* Discard page contents */
        TRACE( "discard page contents - ignored (no paging)\n" );
        break;

    case 0x0800:  /* Physical address mapping */
        FIXME( "physical address mapping (0x%08x) - unimplemented\n",
               MAKELONG(CX_reg(context),BX_reg(context)) );
        break;

    case 0x0900:  /* Get and Disable Virtual Interrupt State */
        TRACE( "Get and Disable Virtual Interrupt State: %d\n",
               NtCurrentTeb()->dpmi_vif );
        SET_AL( context, NtCurrentTeb()->dpmi_vif ? 1 : 0 );
        NtCurrentTeb()->dpmi_vif = 0;
        break;

    case 0x0901:  /* Get and Enable Virtual Interrupt State */
        TRACE( "Get and Enable Virtual Interrupt State: %d\n",
               NtCurrentTeb()->dpmi_vif );
        SET_AL( context, NtCurrentTeb()->dpmi_vif ? 1 : 0 );
        NtCurrentTeb()->dpmi_vif = 1;
        break;

    case 0x0902:  /* Get Virtual Interrupt State */
        TRACE( "Get Virtual Interrupt State: %d\n",
               NtCurrentTeb()->dpmi_vif );
        SET_AL( context, NtCurrentTeb()->dpmi_vif ? 1 : 0 );
        break;

    case 0x0e00:  /* Get Coprocessor Status (1.0) */
        /*
         * Return status in AX bits:
         * B0    - MPv (MP bit in the virtual MSW/CR0)
         *         0 = numeric coprocessor is disabled for this client
         *         1 = numeric coprocessor is enabled for this client
         * B1    - EMv (EM bit in the virtual MSW/CR0)
         *         0 = client is not emulating coprocessor instructions
         *         1 = client is emulating coprocessor instructions
         * B2    - MPr (MP bit from the actual MSW/CR0)
         *         0 = numeric coprocessor is not present
         *         1 = numeric coprocessor is present
         * B3    - EMr (EM bit from the actual MSW/CR0)
         *         0 = host is not emulating coprocessor instructions
         *         1 = host is emulating coprocessor instructions
         * B4-B7 - coprocessor type
         *         00H = no coprocessor
         *         02H = 80287
         *         03H = 80387
         *         04H = 80486 with numeric coprocessor
         *         05H-0FH = reserved for future numeric processors
         */
        TRACE( "Get Coprocessor Status\n" );
        SET_AX( context, 69 ); /* 486, coprocessor present and enabled */ 
        break;

    case 0x0e01: /* Set Coprocessor Emulation (1.0) */
        /*
         * See function 0x0e00.
         * BX bit B0 is new value for MPv.
         * BX bit B1 is new value for EMv.
         */
        if (BX_reg(context) != 1)
            FIXME( "Set Coprocessor Emulation to %d - unimplemented\n", 
                   BX_reg(context) );
        else
            TRACE( "Set Coprocessor Emulation - ignored\n" );
        break;

    default:
        INT_BARF( context, 0x31 );
        SET_AX( context, 0x8001 );  /* unsupported function */
        SET_CFLAG(context);
        break;
    }  
}
