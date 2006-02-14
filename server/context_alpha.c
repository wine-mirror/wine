/*
 * Alpha register context support
 *
 * Copyright (C) 2004 Vincent Béron
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef __ALPHA__

#include <assert.h>
#include <errno.h>
#ifdef HAVE_SYS_REG_H
# include <sys/reg.h>
#endif
#include <stdarg.h>
#include <unistd.h>
#ifdef HAVE_SYS_PTRACE_H
# include <sys/ptrace.h>
#endif

#include "file.h"
#include "thread.h"
#include "request.h"

#ifdef HAVE_SYS_USER_H
# include <sys/user.h>
#endif

/* user definitions from asm/user.h */
struct kernel_user_struct
{
    unsigned long regs[EF_SIZE/8+32];
    size_t u_tsize;
    size_t u_dsize;
    size_t u_ssize;
    unsigned long start_code;
    unsigned long start_data;
    unsigned long start_stack;
    long int signal;
    struct regs * u_ar0;
    unsigned long magic;
    char u_comm[32];
};

/* get thread context */
static void get_thread_context( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    int pid = get_ptrace_pid(thread);
    if (flags & CONTEXT_FULL)
    {
        struct kernel_user_struct regs;
        if (ptrace( PTRACE_GETREGS, pid, 0, &regs ) == -1) goto error;
        if (flags & CONTEXT_INTEGER)
        {
            context->IntV0 = regs.regs[EF_V0];
            context->IntT0 = regs.regs[EF_T0];
            context->IntT1 = regs.regs[EF_T1];
            context->IntT2 = regs.regs[EF_T2];
            context->IntT3 = regs.regs[EF_T3];
            context->IntT4 = regs.regs[EF_T4];
            context->IntT5 = regs.regs[EF_T5];
            context->IntT6 = regs.regs[EF_T6];
            context->IntT7 = regs.regs[EF_T7];
            context->IntS0 = regs.regs[EF_S0];
            context->IntS1 = regs.regs[EF_S1];
            context->IntS2 = regs.regs[EF_S2];
            context->IntS3 = regs.regs[EF_S3];
            context->IntS4 = regs.regs[EF_S4];
            context->IntS5 = regs.regs[EF_S5];
            context->IntFp = regs.regs[EF_S6];
            context->IntA0 = regs.regs[EF_A0];
            context->IntA1 = regs.regs[EF_A1];
            context->IntA2 = regs.regs[EF_A2];
            context->IntA3 = regs.regs[EF_A3];
            context->IntA4 = regs.regs[EF_A4];
            context->IntA5 = regs.regs[EF_A5];
            context->IntT8 = regs.regs[EF_T8];
            context->IntT9 = regs.regs[EF_T9];
            context->IntT10 = regs.regs[EF_T10];
            context->IntT11 = regs.regs[EF_T11];
            context->IntT12 = regs.regs[EF_T12];
            context->IntAt = regs.regs[EF_AT];
            context->IntZero = 0;
        }
        if (flags & CONTEXT_CONTROL)
        {
            context->IntRa = regs.regs[EF_RA];
            context->IntGp = regs.regs[EF_GP];
            context->IntSp = regs.regs[EF_SP];
            context->Fir = regs.regs[EF_PC];
            context->Psr = regs.regs[EF_PS];
        }
        if (flags & CONTEXT_FLOATING_POINT)
        {
            context->FltF0 = regs.regs[EF_SIZE/8+0];
            context->FltF1 = regs.regs[EF_SIZE/8+1];
            context->FltF2 = regs.regs[EF_SIZE/8+2];
            context->FltF3 = regs.regs[EF_SIZE/8+3];
            context->FltF4 = regs.regs[EF_SIZE/8+4];
            context->FltF5 = regs.regs[EF_SIZE/8+5];
            context->FltF6 = regs.regs[EF_SIZE/8+6];
            context->FltF7 = regs.regs[EF_SIZE/8+7];
            context->FltF8 = regs.regs[EF_SIZE/8+8];
            context->FltF9 = regs.regs[EF_SIZE/8+9];
            context->FltF10 = regs.regs[EF_SIZE/8+10];
            context->FltF11 = regs.regs[EF_SIZE/8+11];
            context->FltF12 = regs.regs[EF_SIZE/8+12];
            context->FltF13 = regs.regs[EF_SIZE/8+13];
            context->FltF14 = regs.regs[EF_SIZE/8+14];
            context->FltF15 = regs.regs[EF_SIZE/8+15];
            context->FltF16 = regs.regs[EF_SIZE/8+16];
            context->FltF17 = regs.regs[EF_SIZE/8+17];
            context->FltF18 = regs.regs[EF_SIZE/8+18];
            context->FltF19 = regs.regs[EF_SIZE/8+19];
            context->FltF20 = regs.regs[EF_SIZE/8+20];
            context->FltF21 = regs.regs[EF_SIZE/8+21];
            context->FltF22 = regs.regs[EF_SIZE/8+22];
            context->FltF23 = regs.regs[EF_SIZE/8+23];
            context->FltF24 = regs.regs[EF_SIZE/8+24];
            context->FltF25 = regs.regs[EF_SIZE/8+25];
            context->FltF26 = regs.regs[EF_SIZE/8+26];
            context->FltF27 = regs.regs[EF_SIZE/8+27];
            context->FltF28 = regs.regs[EF_SIZE/8+28];
            context->FltF29 = regs.regs[EF_SIZE/8+29];
            context->FltF30 = regs.regs[EF_SIZE/8+30];
            context->FltF31 = 0;
            context->Fpcr = regs.regs[EF_SIZE/8+31];
            context->SoftFpcr = 0; /* FIXME */
        }
        context->ContextFlags |= flags & CONTEXT_FULL;
    }
    return;
 error:
    file_set_error();
}

/* set a thread context */
static void set_thread_context( struct thread *thread, unsigned int flags, const CONTEXT *context )
{
    int pid = get_ptrace_pid(thread);
    if (flags & CONTEXT_FULL)
    {
        struct kernel_user_struct regs;
        if (ptrace( PTRACE_GETREGS, pid, 0, &regs ) == -1) goto error;
        if (flags & CONTEXT_INTEGER)
        {
            regs.regs[EF_V0] = context->IntV0;
            regs.regs[EF_T0] = context->IntT0;
            regs.regs[EF_T1] = context->IntT1;
            regs.regs[EF_T2] = context->IntT2;
            regs.regs[EF_T3] = context->IntT3;
            regs.regs[EF_T4] = context->IntT4;
            regs.regs[EF_T5] = context->IntT5;
            regs.regs[EF_T6] = context->IntT6;
            regs.regs[EF_T7] = context->IntT7;
            regs.regs[EF_S0] = context->IntS0;
            regs.regs[EF_S1] = context->IntS1;
            regs.regs[EF_S2] = context->IntS2;
            regs.regs[EF_S3] = context->IntS3;
            regs.regs[EF_S4] = context->IntS4;
            regs.regs[EF_S5] = context->IntS5;
            regs.regs[EF_S6] = context->IntFp;
            regs.regs[EF_A0] = context->IntA0;
            regs.regs[EF_A1] = context->IntA1;
            regs.regs[EF_A2] = context->IntA2;
            regs.regs[EF_A3] = context->IntA3;
            regs.regs[EF_A4] = context->IntA4;
            regs.regs[EF_A5] = context->IntA5;
            regs.regs[EF_T8] = context->IntT8;
            regs.regs[EF_T9] = context->IntT9;
            regs.regs[EF_T10] = context->IntT10;
            regs.regs[EF_T11] = context->IntT11;
            regs.regs[EF_T12] = context->IntT12;
            regs.regs[EF_AT] = context->IntAt;
        }
        if (flags & CONTEXT_CONTROL)
        {
            regs.regs[EF_RA] = context->IntRa;
            regs.regs[EF_GP] = context->IntGp;
            regs.regs[EF_SP] = context->IntSp;
            regs.regs[EF_PC] = context->Fir;
            regs.regs[EF_PS] = context->Psr;
        }
        if (flags & CONTEXT_FLOATING_POINT)
        {
            regs.regs[EF_SIZE/8+0] = context->FltF0;
            regs.regs[EF_SIZE/8+1] = context->FltF1;
            regs.regs[EF_SIZE/8+2] = context->FltF2;
            regs.regs[EF_SIZE/8+3] = context->FltF3;
            regs.regs[EF_SIZE/8+4] = context->FltF4;
            regs.regs[EF_SIZE/8+5] = context->FltF5;
            regs.regs[EF_SIZE/8+6] = context->FltF6;
            regs.regs[EF_SIZE/8+7] = context->FltF7;
            regs.regs[EF_SIZE/8+8] = context->FltF8;
            regs.regs[EF_SIZE/8+9] = context->FltF9;
            regs.regs[EF_SIZE/8+10] = context->FltF10;
            regs.regs[EF_SIZE/8+11] = context->FltF11;
            regs.regs[EF_SIZE/8+12] = context->FltF12;
            regs.regs[EF_SIZE/8+13] = context->FltF13;
            regs.regs[EF_SIZE/8+14] = context->FltF14;
            regs.regs[EF_SIZE/8+15] = context->FltF15;
            regs.regs[EF_SIZE/8+16] = context->FltF16;
            regs.regs[EF_SIZE/8+17] = context->FltF17;
            regs.regs[EF_SIZE/8+18] = context->FltF18;
            regs.regs[EF_SIZE/8+19] = context->FltF19;
            regs.regs[EF_SIZE/8+20] = context->FltF20;
            regs.regs[EF_SIZE/8+21] = context->FltF21;
            regs.regs[EF_SIZE/8+22] = context->FltF22;
            regs.regs[EF_SIZE/8+23] = context->FltF23;
            regs.regs[EF_SIZE/8+24] = context->FltF24;
            regs.regs[EF_SIZE/8+25] = context->FltF25;
            regs.regs[EF_SIZE/8+26] = context->FltF26;
            regs.regs[EF_SIZE/8+27] = context->FltF27;
            regs.regs[EF_SIZE/8+28] = context->FltF28;
            regs.regs[EF_SIZE/8+29] = context->FltF29;
            regs.regs[EF_SIZE/8+30] = context->FltF30;
            regs.regs[EF_SIZE/8+31] = context->Fpcr;
        }
        if (ptrace( PTRACE_SETREGS, pid, 0, &regs ) == -1) goto error;
    }
    return;
 error:
    file_set_error();
}

/* copy a context structure according to the flags */
static void copy_context( CONTEXT *to, const CONTEXT *from, unsigned int flags )
{
    if (flags & CONTEXT_CONTROL)
    {
        to->IntRa = from->IntRa;
        to->IntGp = from->IntGp;
        to->IntSp = from->IntSp;
        to->Fir = from->Fir;
        to->Psr = from->Psr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->IntV0 = from->IntV0;
        to->IntT0 = from->IntT0;
        to->IntT1 = from->IntT1;
        to->IntT2 = from->IntT2;
        to->IntT3 = from->IntT3;
        to->IntT4 = from->IntT4;
        to->IntT5 = from->IntT5;
        to->IntT6 = from->IntT6;
        to->IntT7 = from->IntT7;
        to->IntS0 = from->IntS0;
        to->IntS1 = from->IntS1;
        to->IntS2 = from->IntS2;
        to->IntS3 = from->IntS3;
        to->IntS4 = from->IntS4;
        to->IntS5 = from->IntS5;
        to->IntFp = from->IntFp;
        to->IntA0 = from->IntA0;
        to->IntA1 = from->IntA1;
        to->IntA2 = from->IntA2;
        to->IntA3 = from->IntA3;
        to->IntA4 = from->IntA4;
        to->IntA5 = from->IntA5;
        to->IntT8 = from->IntT8;
        to->IntT9 = from->IntT9;
        to->IntT10 = from->IntT10;
        to->IntT11 = from->IntT11;
        to->IntT12 = from->IntT12;
        to->IntAt = from->IntAt;
        to->IntZero = from->IntZero;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->FltF0 = from->FltF0;
        to->FltF1 = from->FltF1;
        to->FltF2 = from->FltF2;
        to->FltF3 = from->FltF3;
        to->FltF4 = from->FltF4;
        to->FltF5 = from->FltF5;
        to->FltF6 = from->FltF6;
        to->FltF7 = from->FltF7;
        to->FltF8 = from->FltF8;
        to->FltF9 = from->FltF9;
        to->FltF10 = from->FltF10;
        to->FltF11 = from->FltF11;
        to->FltF12 = from->FltF12;
        to->FltF13 = from->FltF13;
        to->FltF14 = from->FltF14;
        to->FltF15 = from->FltF15;
        to->FltF16 = from->FltF16;
        to->FltF17 = from->FltF17;
        to->FltF18 = from->FltF18;
        to->FltF19 = from->FltF19;
        to->FltF20 = from->FltF20;
        to->FltF21 = from->FltF21;
        to->FltF22 = from->FltF22;
        to->FltF23 = from->FltF23;
        to->FltF24 = from->FltF24;
        to->FltF25 = from->FltF25;
        to->FltF26 = from->FltF26;
        to->FltF27 = from->FltF27;
        to->FltF28 = from->FltF28;
        to->FltF29 = from->FltF29;
        to->FltF30 = from->FltF30;
        to->FltF31 = from->FltF31;
        to->Fpcr = from->Fpcr;
        to->SoftFpcr = from->SoftFpcr;
    }
    to->ContextFlags |= flags;
}

/* retrieve the current instruction pointer of a thread */
void *get_thread_ip( struct thread *thread )
{
    CONTEXT context;
    context.Fir = 0;
    if (suspend_for_ptrace( thread ))
    {
        get_thread_context( thread, CONTEXT_CONTROL, &context );
        resume_after_ptrace( thread );
    }
    return (void *)context.Fir;
}

/* determine if we should continue the thread in single-step mode */
int get_thread_single_step( struct thread *thread )
{
    return 0;  /* FIXME */
}

/* send a signal to a specific thread */
int tkill( int tgid, int pid, int sig )
{
    /* FIXME: should do something here */
    errno = ENOSYS;
    return -1;
}

/* retrieve the thread context */
void get_thread_context( struct thread *thread, CONTEXT *context, unsigned int flags )
{
    context->ContextFlags |= CONTEXT_ALPHA;
    flags &= ~CONTEXT_ALPHA;  /* get rid of CPU id */

    if (thread->context)  /* thread is inside an exception event or suspended */
    {
        copy_context( context, thread->context, flags );
    }
    else if (flags && suspend_for_ptrace( thread ))
    {
        get_thread_context_ptrace( thread, flags, context );
        resume_after_ptrace( thread );
    }
}

/* set the thread context */
void set_thread_context( struct thread *thread, const CONTEXT *context, unsigned int flags )
{
    flags &= ~CONTEXT_ALPHA;  /* get rid of CPU id */

    if (thread->context)  /* thread is inside an exception event or suspended */
    {
        copy_context( thread->context, context, flags );
    }
    else if (flags && suspend_for_ptrace( thread ))
    {
        set_thread_context_ptrace( thread, flags, context );
        resume_after_ptrace( thread );
    }
}

#endif  /* __ALPHA__ */
