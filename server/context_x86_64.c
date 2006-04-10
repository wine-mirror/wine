/*
 * x86-64 register context support
 *
 * Copyright (C) 1999, 2005 Alexandre Julliard
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

#ifdef __x86_64__

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef HAVE_SYS_PTRACE_H
# include <sys/ptrace.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"

#include "file.h"
#include "thread.h"
#include "request.h"

#ifdef __linux__

#ifdef HAVE_SYS_USER_H
# include <sys/user.h>
#endif

/* debug register offset in struct user */
#define DR_OFFSET(dr) ((unsigned long)((((struct user *)0)->u_debugreg) + (dr)))

/* retrieve a debug register */
static inline int get_debug_reg( int pid, int num, DWORD64 *data )
{
    int res;
    errno = 0;
    res = ptrace( PTRACE_PEEKUSER, pid, DR_OFFSET(num), 0 );
    if ((res == -1) && errno)
    {
        file_set_error();
        return -1;
    }
    *data = res;
    return 0;
}

/* retrieve a thread context */
static void get_thread_context_ptrace( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    int pid = get_ptrace_pid(thread);
    if (flags & CONTEXT_FULL)
    {
        struct user_regs_struct regs;
        if (ptrace( PTRACE_GETREGS, pid, 0, &regs ) == -1) goto error;
        if (flags & CONTEXT_INTEGER)
        {
            context->Rax = regs.rax;
            context->Rbx = regs.rbx;
            context->Rcx = regs.rcx;
            context->Rdx = regs.rdx;
            context->Rsi = regs.rsi;
            context->Rdi = regs.rdi;
            context->R8  = regs.r8;
            context->R9  = regs.r9;
            context->R10 = regs.r10;
            context->R11 = regs.r11;
            context->R12 = regs.r12;
            context->R13 = regs.r13;
            context->R14 = regs.r14;
            context->R15 = regs.r15;
        }
        if (flags & CONTEXT_CONTROL)
        {
            context->Rbp    = regs.rbp;
            context->Rsp    = regs.rsp;
            context->Rip    = regs.rip;
            context->SegCs  = regs.cs;
            context->SegSs  = regs.ss;
            context->EFlags = regs.eflags;
        }
        if (flags & CONTEXT_SEGMENTS)
        {
            context->SegDs = regs.ds;
            context->SegEs = regs.es;
            context->SegFs = regs.fs;
            context->SegGs = regs.gs;
        }
        context->ContextFlags |= flags & CONTEXT_FULL;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        if (get_debug_reg( pid, 0, &context->Dr0 ) == -1) goto error;
        if (get_debug_reg( pid, 1, &context->Dr1 ) == -1) goto error;
        if (get_debug_reg( pid, 2, &context->Dr2 ) == -1) goto error;
        if (get_debug_reg( pid, 3, &context->Dr3 ) == -1) goto error;
        if (get_debug_reg( pid, 6, &context->Dr6 ) == -1) goto error;
        if (get_debug_reg( pid, 7, &context->Dr7 ) == -1) goto error;
        context->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* we can use context->FloatSave directly as it is using the */
        /* correct structure (the same as fsave/frstor) */
        if (ptrace( PTRACE_GETFPREGS, pid, 0, &context->u.FltSave ) == -1) goto error;
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    return;
 error:
    file_set_error();
}


/* set a thread context */
static void set_thread_context_ptrace( struct thread *thread, unsigned int flags, const CONTEXT *context )
{
    int pid = get_ptrace_pid(thread);
    if (flags & CONTEXT_FULL)
    {
        struct user_regs_struct regs;

        /* need to preserve some registers (at a minimum orig_eax must always be preserved) */
        if (ptrace( PTRACE_GETREGS, pid, 0, &regs ) == -1) goto error;

        if (flags & CONTEXT_INTEGER)
        {
            regs.rax = context->Rax;
            regs.rbx = context->Rbx;
            regs.rcx = context->Rcx;
            regs.rdx = context->Rdx;
            regs.rsi = context->Rsi;
            regs.rdi = context->Rdi;
            regs.r8  = context->R8;
            regs.r9  = context->R9;
            regs.r10 = context->R10;
            regs.r11 = context->R11;
            regs.r12 = context->R12;
            regs.r13 = context->R13;
            regs.r14 = context->R14;
            regs.r15 = context->R15;
        }
        if (flags & CONTEXT_CONTROL)
        {
            regs.rbp = context->Rbp;
            regs.rip = context->Rip;
            regs.rsp = context->Rsp;
            regs.cs  = context->SegCs;
            regs.ss  = context->SegSs;
            regs.eflags = context->EFlags;
        }
        if (flags & CONTEXT_SEGMENTS)
        {
            regs.ds = context->SegDs;
            regs.es = context->SegEs;
            regs.fs = context->SegFs;
            regs.gs = context->SegGs;
        }
        if (ptrace( PTRACE_SETREGS, pid, 0, &regs ) == -1) goto error;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(0), context->Dr0 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(1), context->Dr1 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(2), context->Dr2 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(3), context->Dr3 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(6), context->Dr6 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), context->Dr7 ) == -1) goto error;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* we can use context->FloatSave directly as it is using the */
        /* correct structure (the same as fsave/frstor) */
        if (ptrace( PTRACE_SETFPREGS, pid, 0, &context->u.FltSave ) == -1) goto error;
    }
    return;
 error:
    file_set_error();
}

#else  /* linux */
#error You must implement get/set_thread_context_ptrace for your platform
#endif  /* linux */


/* copy a context structure according to the flags */
static void copy_context( CONTEXT *to, const CONTEXT *from, unsigned int flags )
{
    if (flags & CONTEXT_CONTROL)
    {
        to->Rbp    = from->Rbp;
        to->Rip    = from->Rip;
        to->Rsp    = from->Rsp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
        to->MxCsr  = from->MxCsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Rax = from->Rax;
        to->Rcx = from->Rcx;
        to->Rdx = from->Rdx;
        to->Rbx = from->Rbx;
        to->Rsi = from->Rsi;
        to->Rdi = from->Rdi;
        to->R8  = from->R8;
        to->R9  = from->R9;
        to->R10 = from->R10;
        to->R11 = from->R11;
        to->R12 = from->R12;
        to->R13 = from->R13;
        to->R14 = from->R14;
        to->R15 = from->R15;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->SegDs = from->SegDs;
        to->SegEs = from->SegEs;
        to->SegFs = from->SegFs;
        to->SegGs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->u.FltSave = from->u.FltSave;
    }
    /* we don't bother copying the debug registers, since they */
    /* always need to be accessed by ptrace anyway */
    to->ContextFlags |= flags & ~CONTEXT_DEBUG_REGISTERS;
}

/* retrieve the current instruction pointer of a thread */
void *get_thread_ip( struct thread *thread )
{
    CONTEXT context;
    context.Rip = 0;
    if (suspend_for_ptrace( thread ))
    {
        get_thread_context_ptrace( thread, CONTEXT_CONTROL, &context );
        resume_after_ptrace( thread );
    }
    return (void *)context.Rip;
}

/* send a signal to a specific thread */
int tkill( int tgid, int pid, int sig )
{
#ifdef __linux__
    int ret;
    __asm__( "syscall" : "=a" (ret)
             : "0" (200) /*SYS_tkill*/, "D" (pid), "S" (sig) );
    if (ret >= 0) return ret;
    errno = -ret;
    return -1;
#else
    errno = ENOSYS;
    return -1;
#endif
}

/* retrieve the thread context */
void get_thread_context( struct thread *thread, CONTEXT *context, unsigned int flags )
{
    context->ContextFlags |= CONTEXT_AMD64;
    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */

    if (thread->context)  /* thread is inside an exception event or suspended */
    {
        copy_context( context, thread->context, flags );
        flags &= CONTEXT_DEBUG_REGISTERS;
    }

    if (flags && suspend_for_ptrace( thread ))
    {
        get_thread_context_ptrace( thread, flags, context );
        resume_after_ptrace( thread );
    }
}

/* set the thread context */
void set_thread_context( struct thread *thread, const CONTEXT *context, unsigned int flags )
{
    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */

    if (thread->context)  /* thread is inside an exception event or suspended */
    {
        copy_context( thread->context, context, flags );
        flags &= CONTEXT_DEBUG_REGISTERS;
    }

    if (flags && suspend_for_ptrace( thread ))
    {
        set_thread_context_ptrace( thread, flags, context );
        resume_after_ptrace( thread );
    }
}

#endif  /* __x86_64__ */
