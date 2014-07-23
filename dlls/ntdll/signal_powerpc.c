/*
 * PowerPC signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
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

#ifdef __powerpc__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

static pthread_key_t teb_key;

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

/* All Registers access - only for local access */
# define REG_sig(reg_name, context)		((context)->uc_mcontext.regs->reg_name)


/* Gpr Registers access  */
# define GPR_sig(reg_num, context)		REG_sig(gpr[reg_num], context)

# define IAR_sig(context)			REG_sig(nip, context)	/* Program counter */
# define MSR_sig(context)			REG_sig(msr, context)   /* Machine State Register (Supervisor) */
# define CTR_sig(context)			REG_sig(ctr, context)   /* Count register */

# define XER_sig(context)			REG_sig(xer, context) /* User's integer exception register */
# define LR_sig(context)			REG_sig(link, context) /* Link register */
# define CR_sig(context)			REG_sig(ccr, context) /* Condition register */

/* Float Registers access  */
# define FLOAT_sig(reg_num, context)		(((double*)((char*)((context)->uc_mcontext.regs+48*4)))[reg_num])

# define FPSCR_sig(context)			(*(int*)((char*)((context)->uc_mcontext.regs+(48+32*2)*4)))

/* Exception Registers access */
# define DAR_sig(context)			REG_sig(dar, context)
# define DSISR_sig(context)			REG_sig(dsisr, context)
# define TRAP_sig(context)			REG_sig(trap, context)

#endif /* linux */

#ifdef __APPLE__

/* All Registers access - only for local access */
# define REG_sig(reg_name, context)		((context)->uc_mcontext->ss.reg_name)
# define FLOATREG_sig(reg_name, context)	((context)->uc_mcontext->fs.reg_name)
# define EXCEPREG_sig(reg_name, context)	((context)->uc_mcontext->es.reg_name)
# define VECREG_sig(reg_name, context)		((context)->uc_mcontext->vs.reg_name)

/* Gpr Registers access */
# define GPR_sig(reg_num, context)		REG_sig(r##reg_num, context)

# define IAR_sig(context)			REG_sig(srr0, context)	/* Program counter */
# define MSR_sig(context)			REG_sig(srr1, context)  /* Machine State Register (Supervisor) */
# define CTR_sig(context)			REG_sig(ctr, context)

# define XER_sig(context)			REG_sig(xer, context) /* Link register */
# define LR_sig(context)			REG_sig(lr, context)  /* User's integer exception register */
# define CR_sig(context)			REG_sig(cr, context)  /* Condition register */

/* Float Registers access */
# define FLOAT_sig(reg_num, context)		FLOATREG_sig(fpregs[reg_num], context)

# define FPSCR_sig(context)			FLOATREG_sig(fpscr, context)

/* Exception Registers access */
# define DAR_sig(context)			EXCEPREG_sig(dar, context)     /* Fault registers for coredump */
# define DSISR_sig(context)			EXCEPREG_sig(dsisr, context)
# define TRAP_sig(context)			EXCEPREG_sig(exception, context) /* number of powerpc exception taken */

/* Signal defs : Those are undefined on darwin
SIGBUS
#undef BUS_ADRERR
#undef BUS_OBJERR
SIGILL
#undef ILL_ILLOPN
#undef ILL_ILLTRP
#undef ILL_ILLADR
#undef ILL_COPROC
#undef ILL_PRVREG
#undef ILL_BADSTK
SIGTRAP
#undef TRAP_BRKPT
#undef TRAP_TRACE
SIGFPE
*/

#endif /* __APPLE__ */



typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];

/***********************************************************************
 *           dispatch_signal
 */
static inline int dispatch_signal(unsigned int sig)
{
    if (handlers[sig] == NULL) return 0;
    return handlers[sig](sig);
}

/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const ucontext_t *sigcontext )
{

#define C(x) context->Gpr##x = GPR_sig(x,sigcontext)
        /* Save Gpr registers */
	C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C

	context->Iar = IAR_sig(sigcontext);  /* Program Counter */
	context->Msr = MSR_sig(sigcontext);  /* Machine State Register (Supervisor) */
	context->Ctr = CTR_sig(sigcontext);
        
        context->Xer = XER_sig(sigcontext);
	context->Lr  = LR_sig(sigcontext);
	context->Cr  = CR_sig(sigcontext);
        
        /* Saving Exception regs */
        context->Dar   = DAR_sig(sigcontext);
        context->Dsisr = DSISR_sig(sigcontext);
        context->Trap  = TRAP_sig(sigcontext);
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{

#define C(x)  GPR_sig(x,sigcontext) = context->Gpr##x
	C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C

        IAR_sig(sigcontext) = context->Iar;  /* Program Counter */
        MSR_sig(sigcontext) = context->Msr;  /* Machine State Register (Supervisor) */
        CTR_sig(sigcontext) = context->Ctr;
        
        XER_sig(sigcontext) = context->Xer;
        LR_sig(sigcontext) = context->Lr;
	CR_sig(sigcontext) = context->Cr;
        
        /* Setting Exception regs */
        DAR_sig(sigcontext) = context->Dar;
        DSISR_sig(sigcontext) = context->Dsisr;
        TRAP_sig(sigcontext) = context->Trap;
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
static inline void save_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
#define C(x)   context->Fpr##x = FLOAT_sig(x,sigcontext)
        C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C
        context->Fpscr = FPSCR_sig(sigcontext);
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static inline void restore_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
#define C(x)  FLOAT_sig(x,sigcontext) = context->Fpr##x
        C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C
        FPSCR_sig(sigcontext) = context->Fpscr;
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
void WINAPI RtlCaptureContext( CONTEXT *context )
{
    FIXME("not implemented\n");
    memset( context, 0, sizeof(*context) );
}


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context.
 */
void set_cpu_context( const CONTEXT *context )
{
    FIXME("not implemented\n");
}


/***********************************************************************
 *           copy_context
 *
 * Copy a register context according to the flags.
 */
void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags )
{
    if (flags & CONTEXT_CONTROL)
    {
        to->Msr   = from->Msr;
        to->Ctr   = from->Ctr;
        to->Iar   = from->Iar;
        to->Lr    = from->Lr;
        to->Dar   = from->Dar;
        to->Dsisr = from->Dsisr;
        to->Trap  = from->Trap;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Gpr0  = from->Gpr0;
        to->Gpr1  = from->Gpr1;
        to->Gpr2  = from->Gpr2;
        to->Gpr3  = from->Gpr3;
        to->Gpr4  = from->Gpr4;
        to->Gpr5  = from->Gpr5;
        to->Gpr6  = from->Gpr6;
        to->Gpr7  = from->Gpr7;
        to->Gpr8  = from->Gpr8;
        to->Gpr9  = from->Gpr9;
        to->Gpr10 = from->Gpr10;
        to->Gpr11 = from->Gpr11;
        to->Gpr12 = from->Gpr12;
        to->Gpr13 = from->Gpr13;
        to->Gpr14 = from->Gpr14;
        to->Gpr15 = from->Gpr15;
        to->Gpr16 = from->Gpr16;
        to->Gpr17 = from->Gpr17;
        to->Gpr18 = from->Gpr18;
        to->Gpr19 = from->Gpr19;
        to->Gpr20 = from->Gpr20;
        to->Gpr21 = from->Gpr21;
        to->Gpr22 = from->Gpr22;
        to->Gpr23 = from->Gpr23;
        to->Gpr24 = from->Gpr24;
        to->Gpr25 = from->Gpr25;
        to->Gpr26 = from->Gpr26;
        to->Gpr27 = from->Gpr27;
        to->Gpr28 = from->Gpr28;
        to->Gpr29 = from->Gpr29;
        to->Gpr30 = from->Gpr30;
        to->Gpr31 = from->Gpr31;
        to->Xer   = from->Xer;
        to->Cr    = from->Cr;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->Fpr0  = from->Fpr0;
        to->Fpr1  = from->Fpr1;
        to->Fpr2  = from->Fpr2;
        to->Fpr3  = from->Fpr3;
        to->Fpr4  = from->Fpr4;
        to->Fpr5  = from->Fpr5;
        to->Fpr6  = from->Fpr6;
        to->Fpr7  = from->Fpr7;
        to->Fpr8  = from->Fpr8;
        to->Fpr9  = from->Fpr9;
        to->Fpr10 = from->Fpr10;
        to->Fpr11 = from->Fpr11;
        to->Fpr12 = from->Fpr12;
        to->Fpr13 = from->Fpr13;
        to->Fpr14 = from->Fpr14;
        to->Fpr15 = from->Fpr15;
        to->Fpr16 = from->Fpr16;
        to->Fpr17 = from->Fpr17;
        to->Fpr18 = from->Fpr18;
        to->Fpr19 = from->Fpr19;
        to->Fpr20 = from->Fpr20;
        to->Fpr21 = from->Fpr21;
        to->Fpr22 = from->Fpr22;
        to->Fpr23 = from->Fpr23;
        to->Fpr24 = from->Fpr24;
        to->Fpr25 = from->Fpr25;
        to->Fpr26 = from->Fpr26;
        to->Fpr27 = from->Fpr27;
        to->Fpr28 = from->Fpr28;
        to->Fpr29 = from->Fpr29;
        to->Fpr30 = from->Fpr30;
        to->Fpr31 = from->Fpr31;
        to->Fpscr = from->Fpscr;
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags;  /* no CPU id? */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_POWERPC;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.powerpc_regs.iar   = from->Iar;
        to->ctl.powerpc_regs.msr   = from->Msr;
        to->ctl.powerpc_regs.ctr   = from->Ctr;
        to->ctl.powerpc_regs.lr    = from->Lr;
        to->ctl.powerpc_regs.dar   = from->Dar;
        to->ctl.powerpc_regs.dsisr = from->Dsisr;
        to->ctl.powerpc_regs.trap  = from->Trap;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.powerpc_regs.gpr[0]  = from->Gpr0;
        to->integer.powerpc_regs.gpr[1]  = from->Gpr1;
        to->integer.powerpc_regs.gpr[2]  = from->Gpr2;
        to->integer.powerpc_regs.gpr[3]  = from->Gpr3;
        to->integer.powerpc_regs.gpr[4]  = from->Gpr4;
        to->integer.powerpc_regs.gpr[5]  = from->Gpr5;
        to->integer.powerpc_regs.gpr[6]  = from->Gpr6;
        to->integer.powerpc_regs.gpr[7]  = from->Gpr7;
        to->integer.powerpc_regs.gpr[8]  = from->Gpr8;
        to->integer.powerpc_regs.gpr[9]  = from->Gpr9;
        to->integer.powerpc_regs.gpr[10] = from->Gpr10;
        to->integer.powerpc_regs.gpr[11] = from->Gpr11;
        to->integer.powerpc_regs.gpr[12] = from->Gpr12;
        to->integer.powerpc_regs.gpr[13] = from->Gpr13;
        to->integer.powerpc_regs.gpr[14] = from->Gpr14;
        to->integer.powerpc_regs.gpr[15] = from->Gpr15;
        to->integer.powerpc_regs.gpr[16] = from->Gpr16;
        to->integer.powerpc_regs.gpr[17] = from->Gpr17;
        to->integer.powerpc_regs.gpr[18] = from->Gpr18;
        to->integer.powerpc_regs.gpr[19] = from->Gpr19;
        to->integer.powerpc_regs.gpr[20] = from->Gpr20;
        to->integer.powerpc_regs.gpr[21] = from->Gpr21;
        to->integer.powerpc_regs.gpr[22] = from->Gpr22;
        to->integer.powerpc_regs.gpr[23] = from->Gpr23;
        to->integer.powerpc_regs.gpr[24] = from->Gpr24;
        to->integer.powerpc_regs.gpr[25] = from->Gpr25;
        to->integer.powerpc_regs.gpr[26] = from->Gpr26;
        to->integer.powerpc_regs.gpr[27] = from->Gpr27;
        to->integer.powerpc_regs.gpr[28] = from->Gpr28;
        to->integer.powerpc_regs.gpr[29] = from->Gpr29;
        to->integer.powerpc_regs.gpr[30] = from->Gpr30;
        to->integer.powerpc_regs.gpr[31] = from->Gpr31;
        to->integer.powerpc_regs.xer     = from->Xer;
        to->integer.powerpc_regs.cr      = from->Cr;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        to->fp.powerpc_regs.fpr[0]  = from->Fpr0;
        to->fp.powerpc_regs.fpr[1]  = from->Fpr1;
        to->fp.powerpc_regs.fpr[2]  = from->Fpr2;
        to->fp.powerpc_regs.fpr[3]  = from->Fpr3;
        to->fp.powerpc_regs.fpr[4]  = from->Fpr4;
        to->fp.powerpc_regs.fpr[5]  = from->Fpr5;
        to->fp.powerpc_regs.fpr[6]  = from->Fpr6;
        to->fp.powerpc_regs.fpr[7]  = from->Fpr7;
        to->fp.powerpc_regs.fpr[8]  = from->Fpr8;
        to->fp.powerpc_regs.fpr[9]  = from->Fpr9;
        to->fp.powerpc_regs.fpr[10] = from->Fpr10;
        to->fp.powerpc_regs.fpr[11] = from->Fpr11;
        to->fp.powerpc_regs.fpr[12] = from->Fpr12;
        to->fp.powerpc_regs.fpr[13] = from->Fpr13;
        to->fp.powerpc_regs.fpr[14] = from->Fpr14;
        to->fp.powerpc_regs.fpr[15] = from->Fpr15;
        to->fp.powerpc_regs.fpr[16] = from->Fpr16;
        to->fp.powerpc_regs.fpr[17] = from->Fpr17;
        to->fp.powerpc_regs.fpr[18] = from->Fpr18;
        to->fp.powerpc_regs.fpr[19] = from->Fpr19;
        to->fp.powerpc_regs.fpr[20] = from->Fpr20;
        to->fp.powerpc_regs.fpr[21] = from->Fpr21;
        to->fp.powerpc_regs.fpr[22] = from->Fpr22;
        to->fp.powerpc_regs.fpr[23] = from->Fpr23;
        to->fp.powerpc_regs.fpr[24] = from->Fpr24;
        to->fp.powerpc_regs.fpr[25] = from->Fpr25;
        to->fp.powerpc_regs.fpr[26] = from->Fpr26;
        to->fp.powerpc_regs.fpr[27] = from->Fpr27;
        to->fp.powerpc_regs.fpr[28] = from->Fpr28;
        to->fp.powerpc_regs.fpr[29] = from->Fpr29;
        to->fp.powerpc_regs.fpr[30] = from->Fpr30;
        to->fp.powerpc_regs.fpr[31] = from->Fpr31;
        to->fp.powerpc_regs.fpscr   = from->Fpscr;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           context_from_server
 *
 * Convert a register context from the server format.
 */
NTSTATUS context_from_server( CONTEXT *to, const context_t *from )
{
    if (from->cpu != CPU_POWERPC) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = 0;  /* no CPU id? */
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Msr   = from->ctl.powerpc_regs.msr;
        to->Ctr   = from->ctl.powerpc_regs.ctr;
        to->Iar   = from->ctl.powerpc_regs.iar;
        to->Lr    = from->ctl.powerpc_regs.lr;
        to->Dar   = from->ctl.powerpc_regs.dar;
        to->Dsisr = from->ctl.powerpc_regs.dsisr;
        to->Trap  = from->ctl.powerpc_regs.trap;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->Gpr0  = from->integer.powerpc_regs.gpr[0];
        to->Gpr1  = from->integer.powerpc_regs.gpr[1];
        to->Gpr2  = from->integer.powerpc_regs.gpr[2];
        to->Gpr3  = from->integer.powerpc_regs.gpr[3];
        to->Gpr4  = from->integer.powerpc_regs.gpr[4];
        to->Gpr5  = from->integer.powerpc_regs.gpr[5];
        to->Gpr6  = from->integer.powerpc_regs.gpr[6];
        to->Gpr7  = from->integer.powerpc_regs.gpr[7];
        to->Gpr8  = from->integer.powerpc_regs.gpr[8];
        to->Gpr9  = from->integer.powerpc_regs.gpr[9];
        to->Gpr10 = from->integer.powerpc_regs.gpr[10];
        to->Gpr11 = from->integer.powerpc_regs.gpr[11];
        to->Gpr12 = from->integer.powerpc_regs.gpr[12];
        to->Gpr13 = from->integer.powerpc_regs.gpr[13];
        to->Gpr14 = from->integer.powerpc_regs.gpr[14];
        to->Gpr15 = from->integer.powerpc_regs.gpr[15];
        to->Gpr16 = from->integer.powerpc_regs.gpr[16];
        to->Gpr17 = from->integer.powerpc_regs.gpr[17];
        to->Gpr18 = from->integer.powerpc_regs.gpr[18];
        to->Gpr19 = from->integer.powerpc_regs.gpr[19];
        to->Gpr20 = from->integer.powerpc_regs.gpr[20];
        to->Gpr21 = from->integer.powerpc_regs.gpr[21];
        to->Gpr22 = from->integer.powerpc_regs.gpr[22];
        to->Gpr23 = from->integer.powerpc_regs.gpr[23];
        to->Gpr24 = from->integer.powerpc_regs.gpr[24];
        to->Gpr25 = from->integer.powerpc_regs.gpr[25];
        to->Gpr26 = from->integer.powerpc_regs.gpr[26];
        to->Gpr27 = from->integer.powerpc_regs.gpr[27];
        to->Gpr28 = from->integer.powerpc_regs.gpr[28];
        to->Gpr29 = from->integer.powerpc_regs.gpr[29];
        to->Gpr30 = from->integer.powerpc_regs.gpr[30];
        to->Gpr31 = from->integer.powerpc_regs.gpr[31];
        to->Xer   = from->integer.powerpc_regs.xer;
        to->Cr    = from->integer.powerpc_regs.cr;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        to->Fpr0  = from->fp.powerpc_regs.fpr[0];
        to->Fpr1  = from->fp.powerpc_regs.fpr[1];
        to->Fpr2  = from->fp.powerpc_regs.fpr[2];
        to->Fpr3  = from->fp.powerpc_regs.fpr[3];
        to->Fpr4  = from->fp.powerpc_regs.fpr[4];
        to->Fpr5  = from->fp.powerpc_regs.fpr[5];
        to->Fpr6  = from->fp.powerpc_regs.fpr[6];
        to->Fpr7  = from->fp.powerpc_regs.fpr[7];
        to->Fpr8  = from->fp.powerpc_regs.fpr[8];
        to->Fpr9  = from->fp.powerpc_regs.fpr[9];
        to->Fpr10 = from->fp.powerpc_regs.fpr[10];
        to->Fpr11 = from->fp.powerpc_regs.fpr[11];
        to->Fpr12 = from->fp.powerpc_regs.fpr[12];
        to->Fpr13 = from->fp.powerpc_regs.fpr[13];
        to->Fpr14 = from->fp.powerpc_regs.fpr[14];
        to->Fpr15 = from->fp.powerpc_regs.fpr[15];
        to->Fpr16 = from->fp.powerpc_regs.fpr[16];
        to->Fpr17 = from->fp.powerpc_regs.fpr[17];
        to->Fpr18 = from->fp.powerpc_regs.fpr[18];
        to->Fpr19 = from->fp.powerpc_regs.fpr[19];
        to->Fpr20 = from->fp.powerpc_regs.fpr[20];
        to->Fpr21 = from->fp.powerpc_regs.fpr[21];
        to->Fpr22 = from->fp.powerpc_regs.fpr[22];
        to->Fpr23 = from->fp.powerpc_regs.fpr[23];
        to->Fpr24 = from->fp.powerpc_regs.fpr[24];
        to->Fpr25 = from->fp.powerpc_regs.fpr[25];
        to->Fpr26 = from->fp.powerpc_regs.fpr[26];
        to->Fpr27 = from->fp.powerpc_regs.fpr[27];
        to->Fpr28 = from->fp.powerpc_regs.fpr[28];
        to->Fpr29 = from->fp.powerpc_regs.fpr[29];
        to->Fpr30 = from->fp.powerpc_regs.fpr[30];
        to->Fpr31 = from->fp.powerpc_regs.fpr[31];
        to->Fpscr = from->fp.powerpc_regs.fpscr;
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_POINTERS ptrs;

    FIXME( "not implemented on PowerPC\n" );

    /* hack: call unhandled exception filter directly */
    ptrs.ExceptionRecord = rec;
    ptrs.ContextRecord = context;
    unhandled_exception_filter( &ptrs );
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		raise_exception
 *
 * Implementation of NtRaiseException.
 */
static NTSTATUS raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status;

    if (first_chance)
    {
        DWORD c;

        TRACE( "code=%x flags=%x addr=%p ip=%x tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Iar, GetCurrentThreadId() );
        for (c = 0; c < rec->NumberParameters; c++)
            TRACE( " info[%d]=%08lx\n", c, rec->ExceptionInformation[c] );
        if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        {
            if (rec->ExceptionInformation[1] >> 16)
                MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
            else
                MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        }
        else
        {
            /* FIXME: dump context */
        }

        status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
            return STATUS_SUCCESS;

        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
            return STATUS_SUCCESS;

        if ((status = call_stack_handlers( rec, context )) != STATUS_UNHANDLED_EXCEPTION)
            return status;
    }

    /* last chance exception */

    status = send_debug_event( rec, FALSE, context );
    if (status != DBG_CONTINUE)
    {
        if (rec->ExceptionFlags & EH_STACK_INVALID)
            ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            ERR("Unhandled exception code %x flags %x addr %p\n",
                rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_context( &context, sigcontext );

    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.Iar;
    rec.NumberParameters = 0;

    switch (signal)
    {
    case SIGSEGV:
    	switch (siginfo->si_code & 0xffff)
        {
	case SEGV_MAPERR:
	case SEGV_ACCERR:
            rec.NumberParameters = 2;
            rec.ExceptionInformation[0] = 0; /* FIXME ? */
            rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
            if (!(rec.ExceptionCode = virtual_handle_fault(siginfo->si_addr, rec.ExceptionInformation[0])))
                goto done;
            break;
	default:
            FIXME("Unhandled SIGSEGV/%x\n",siginfo->si_code);
            break;
	}
    	break;
    case SIGBUS:
    	switch (siginfo->si_code & 0xffff)
        {
	case BUS_ADRALN:
            rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
            break;
#ifdef BUS_ADRERR
	case BUS_ADRERR:
#endif
#ifdef BUS_OBJERR
	case BUS_OBJERR:
            /* FIXME: correct for all cases ? */
            rec.NumberParameters = 2;
            rec.ExceptionInformation[0] = 0; /* FIXME ? */
            rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
            if (!(rec.ExceptionCode = virtual_handle_fault(siginfo->si_addr, rec.ExceptionInformation[0])))
                goto done;
            break;
#endif
	default:
            FIXME("Unhandled SIGBUS/%x\n",siginfo->si_code);
            break;
	}
    	break;
    case SIGILL:
    	switch (siginfo->si_code & 0xffff)
        {
	case ILL_ILLOPC: /* illegal opcode */
#ifdef ILL_ILLOPN
	case ILL_ILLOPN: /* illegal operand */
#endif
#ifdef ILL_ILLADR
	case ILL_ILLADR: /* illegal addressing mode */
#endif
#ifdef ILL_ILLTRP
	case ILL_ILLTRP: /* illegal trap */
#endif
#ifdef ILL_COPROC
	case ILL_COPROC: /* coprocessor error */
#endif
            rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
            break;
	case ILL_PRVOPC: /* privileged opcode */
#ifdef ILL_PRVREG
	case ILL_PRVREG: /* privileged register */
#endif
            rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
            break;
#ifdef ILL_BADSTK
	case ILL_BADSTK: /* internal stack error */
            rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
            break;
#endif
	default:
            FIXME("Unhandled SIGILL/%x\n", siginfo->si_code);
            break;
	}
    	break;
    }
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
done:
    restore_context( &context, sigcontext );
}

/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_context( &context, sigcontext );

    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Iar;
    rec.NumberParameters = 0;

    /* FIXME: check if we might need to modify PC */
    switch (siginfo->si_code & 0xffff)
    {
#ifdef TRAP_BRKPT
    case TRAP_BRKPT:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
    	break;
#endif
#ifdef TRAP_TRACE
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
    	break;
#endif
    default:
        FIXME("Unhandled SIGTRAP/%x\n", siginfo->si_code);
        break;
    }
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, sigcontext );
}

/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_fpu( &context, sigcontext );
    save_context( &context, sigcontext );

    switch (siginfo->si_code & 0xffff )
    {
#ifdef FPE_FLTSUB
    case FPE_FLTSUB:
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
#endif
#ifdef FPE_INTDIV
    case FPE_INTDIV:
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_INTOVF
    case FPE_INTOVF:
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTDIV
    case FPE_FLTDIV:
        rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_FLTOVF
    case FPE_FLTOVF:
        rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTUND
    case FPE_FLTUND:
        rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
#endif
#ifdef FPE_FLTRES
    case FPE_FLTRES:
        rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
#endif
#ifdef FPE_FLTINV
    case FPE_FLTINV:
#endif
    default:
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Iar;
    rec.NumberParameters = 0;
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );

    restore_context( &context, sigcontext );
    restore_fpu( &context, sigcontext );
}

/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD rec;
        CONTEXT context;
        NTSTATUS status;

        save_context( &context, sigcontext );
        rec.ExceptionCode    = CONTROL_C_EXIT;
        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context.Iar;
        rec.NumberParameters = 0;
        status = raise_exception( &rec, &context, TRUE );
        if (status) raise_status( status, &rec );
        restore_context( &context, sigcontext );
    }
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_context( &context, sigcontext );
    rec.ExceptionCode    = EXCEPTION_WINE_ASSERTION;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Iar;
    rec.NumberParameters = 0;
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, sigcontext );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    CONTEXT context;

    save_context( &context, sigcontext );
    wait_suspend( &context );
    restore_context( &context, sigcontext );
}


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int CDECL __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig > sizeof(handlers) / sizeof(handlers[0])) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB **teb )
{
    static size_t sigstack_zero_bits;
    SIZE_T size;
    NTSTATUS status;

    if (!sigstack_zero_bits)
    {
        size_t min_size = page_size;  /* this is just for the TEB, we don't use a signal stack yet */
        /* find the first power of two not smaller than min_size */
        while ((1u << sigstack_zero_bits) < min_size) sigstack_zero_bits++;
        assert( sizeof(TEB) <= min_size );
    }

    size = 1 << sigstack_zero_bits;
    *teb = NULL;
    if (!(status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)teb, sigstack_zero_bits,
                                            &size, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE )))
    {
        (*teb)->Tib.Self = &(*teb)->Tib;
        (*teb)->Tib.ExceptionList = (void *)~0UL;
    }
    return status;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    SIZE_T size;

    if (teb->DeallocationStack)
    {
        size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &teb->DeallocationStack, &size, MEM_RELEASE );
    }
    size = 0;
    NtFreeVirtualMemory( NtCurrentProcess(), (void **)&teb, &size, MEM_RELEASE );
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    static BOOL init_done;

    if (!init_done)
    {
        pthread_key_create( &teb_key, NULL );
        init_done = TRUE;
    }
    pthread_setspecific( teb_key, teb );
}


/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_RESTART | SA_SIGINFO;

    sig_act.sa_sigaction = int_handler;
    if (sigaction( SIGINT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = fpe_handler;
    if (sigaction( SIGFPE, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = abrt_handler;
    if (sigaction( SIGABRT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = quit_handler;
    if (sigaction( SIGQUIT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = usr1_handler;
    if (sigaction( SIGUSR1, &sig_act, NULL ) == -1) goto error;

    sig_act.sa_sigaction = segv_handler;
    if (sigaction( SIGSEGV, &sig_act, NULL ) == -1) goto error;
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
#ifdef SIGBUS
    if (sigaction( SIGBUS, &sig_act, NULL ) == -1) goto error;
#endif

#ifdef SIGTRAP
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
#endif
    return;

 error:
    perror("sigaction");
    exit(1);
}


/**********************************************************************
 *              __wine_enter_vm86   (NTDLL.@)
 */
void __wine_enter_vm86( CONTEXT *context )
{
    MESSAGE("vm86 mode not supported on this platform\n");
}

/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( PVOID pEndFrame, PVOID targetIp, PEXCEPTION_RECORD pRecord, PVOID retval )
{
    FIXME( "Not implemented on PowerPC\n" );
}

/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status = raise_exception( rec, context, first_chance );
    if (status == STATUS_SUCCESS) NtSetContextThread( GetCurrentThread(), context );
    return status;
}

/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI RtlRaiseException( EXCEPTION_RECORD *rec )
{
    CONTEXT context;
    NTSTATUS status;

    RtlCaptureContext( &context );
    rec->ExceptionAddress = (void *)context.Iar;
    status = raise_exception( rec, &context, TRUE );
    if (status) raise_status( status, rec );
}

/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%d, %d, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/***********************************************************************
 *           call_thread_entry_point
 */
void call_thread_entry_point( LPTHREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        exit_thread( entry( arg ));
    }
    __EXCEPT(unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}

/***********************************************************************
 *           RtlExitUserThread  (NTDLL.@)
 */
void WINAPI RtlExitUserThread( ULONG status )
{
    exit_thread( status );
}

/***********************************************************************
 *           abort_thread
 */
void abort_thread( int status )
{
    terminate_thread( status );
}

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
void WINAPI DbgBreakPoint(void)
{
     kill(getpid(), SIGTRAP);
}

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
void WINAPI DbgUserBreakPoint(void)
{
     kill(getpid(), SIGTRAP);
}

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __powerpc__ */
