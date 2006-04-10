/*
 * PowerPC register context support
 *
 * Copyright (C) 2002 Marcus Meissner, SuSE Linux AG.
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

#ifdef __powerpc__

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_SYS_REG_H
# include <sys/reg.h>
#endif
#include <stdarg.h>
#include <unistd.h>
#ifdef HAVE_SYS_PTRACE_H
# include <sys/ptrace.h>
#endif

#ifndef PTRACE_PEEKUSER
# ifdef PT_READ_D
#  define PTRACE_PEEKUSER PT_READ_D
# endif
#endif /* PTRACE_PEEKUSER */

#ifndef PTRACE_POKEUSER
# ifdef PT_WRITE_D
#  define PTRACE_POKEUSER PT_WRITE_D
# endif
#endif /* PTRACE_POKEUSER */

#include "windef.h"

#include "file.h"
#include "thread.h"
#include "request.h"

/* retrieve a thread context */
static void get_thread_context_ptrace( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    int pid = get_ptrace_pid(thread);
    if (flags & CONTEXT_INTEGER)
    {
#define XREG(x,y) if (ptrace( PTRACE_PEEKUSER, pid, (void*)(x<<2), &context->y) == -1) goto error;
#define IREG(x) if (ptrace( PTRACE_PEEKUSER, pid, (void*)(x<<2), &context->Gpr##x) == -1) goto error;
        IREG(0); IREG(1); IREG(2); IREG(3); IREG(4); IREG(5); IREG(6);
        IREG(7); IREG(8); IREG(9); IREG(10); IREG(11); IREG(12); IREG(13);
        IREG(14); IREG(15); IREG(16); IREG(17); IREG(18); IREG(19);
        IREG(20); IREG(21); IREG(22); IREG(23); IREG(24); IREG(25);
        IREG(26); IREG(27); IREG(28); IREG(29); IREG(30); IREG(31);
#undef IREG
        XREG(37,Xer);
        XREG(38,Cr);
        context->ContextFlags |= CONTEXT_INTEGER;
    }
    if (flags & CONTEXT_CONTROL)
    {
        XREG(32,Iar);
        XREG(33,Msr);
        XREG(35,Ctr);
        XREG(36,Lr); /* 36 is LNK ... probably Lr ? */
        context->ContextFlags |= CONTEXT_CONTROL;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
#define FREG(x) if (ptrace( PTRACE_PEEKUSER, pid, (void*)((48+x*2)<<2), &context->Fpr##x) == -1) goto error;
	FREG(0);
	FREG(1);
	FREG(2);
	FREG(3);
	FREG(4);
	FREG(5);
	FREG(6);
	FREG(7);
	FREG(8);
	FREG(9);
	FREG(10);
	FREG(11);
	FREG(12);
	FREG(13);
	FREG(14);
	FREG(15);
	FREG(16);
	FREG(17);
	FREG(18);
	FREG(19);
	FREG(20);
	FREG(21);
	FREG(22);
	FREG(23);
	FREG(24);
	FREG(25);
	FREG(26);
	FREG(27);
	FREG(28);
	FREG(29);
	FREG(30);
	FREG(31);
	XREG((48+32*2),Fpscr);
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    return;
 error:
    file_set_error();
}
#undef XREG
#undef IREG
#undef FREG

#define XREG(x,y) if (ptrace( PTRACE_POKEUSER, pid, (void*)(x<<2), &context->y) == -1) goto error;
#define IREG(x) if (ptrace( PTRACE_POKEUSER, pid, (void*)(x<<2), &context->Gpr##x) == -1) goto error;
#define FREG(x) if (ptrace( PTRACE_POKEUSER, pid, (void*)((48+x*2)<<2), &context->Fpr##x) == -1) goto error;
/* set a thread context */
static void set_thread_context_ptrace( struct thread *thread, unsigned int flags, const CONTEXT *context )
{
    int pid = get_ptrace_pid(thread);
    if (flags & CONTEXT_FULL)
    {
        if (flags & CONTEXT_INTEGER)
        {
	    IREG(0); IREG(1); IREG(2); IREG(3); IREG(4); IREG(5); IREG(6);
	    IREG(7); IREG(8); IREG(9); IREG(10); IREG(11); IREG(12); IREG(13);
	    IREG(14); IREG(15); IREG(16); IREG(17); IREG(18); IREG(19);
	    IREG(20); IREG(21); IREG(22); IREG(23); IREG(24); IREG(25);
	    IREG(26); IREG(27); IREG(28); IREG(29); IREG(30); IREG(31);
	    XREG(37,Xer);
	    XREG(38,Cr);

        }
        if (flags & CONTEXT_CONTROL)
        {
	    XREG(32,Iar);
	    XREG(33,Msr);
	    XREG(35,Ctr);
	    XREG(36,Lr);
        }
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
	FREG(0);
	FREG(1);
	FREG(2);
	FREG(3);
	FREG(4);
	FREG(5);
	FREG(6);
	FREG(7);
	FREG(8);
	FREG(9);
	FREG(10);
	FREG(11);
	FREG(12);
	FREG(13);
	FREG(14);
	FREG(15);
	FREG(16);
	FREG(17);
	FREG(18);
	FREG(19);
	FREG(20);
	FREG(21);
	FREG(22);
	FREG(23);
	FREG(24);
	FREG(25);
	FREG(26);
	FREG(27);
	FREG(28);
	FREG(29);
	FREG(30);
	FREG(31);
#undef FREG
	XREG((48+32*2),Fpscr);
    }
    return;
 error:
    file_set_error();
}
#undef XREG
#undef IREG
#undef FREG

#define IREG(x) to->Gpr##x = from->Gpr##x;
#define FREG(x) to->Fpr##x = from->Fpr##x;
#define CREG(x) to->x = from->x;
/* copy a context structure according to the flags */
static void copy_context( CONTEXT *to, const CONTEXT *from, unsigned int flags )
{
    if (flags & CONTEXT_CONTROL)
    {
    	CREG(Msr);
    	CREG(Ctr);
    	CREG(Iar);
        to->ContextFlags |= CONTEXT_CONTROL;
    }
    if (flags & CONTEXT_INTEGER)
    {
	IREG(0); IREG(1); IREG(2); IREG(3); IREG(4); IREG(5); IREG(6);
	IREG(7); IREG(8); IREG(9); IREG(10); IREG(11); IREG(12); IREG(13);
	IREG(14); IREG(15); IREG(16); IREG(17); IREG(18); IREG(19);
	IREG(20); IREG(21); IREG(22); IREG(23); IREG(24); IREG(25);
	IREG(26); IREG(27); IREG(28); IREG(29); IREG(30); IREG(31);
	CREG(Xer);
	CREG(Cr);
        to->ContextFlags |= CONTEXT_INTEGER;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
	FREG(0);
	FREG(1);
	FREG(2);
	FREG(3);
	FREG(4);
	FREG(5);
	FREG(6);
	FREG(7);
	FREG(8);
	FREG(9);
	FREG(10);
	FREG(11);
	FREG(12);
	FREG(13);
	FREG(14);
	FREG(15);
	FREG(16);
	FREG(17);
	FREG(18);
	FREG(19);
	FREG(20);
	FREG(21);
	FREG(22);
	FREG(23);
	FREG(24);
	FREG(25);
	FREG(26);
	FREG(27);
	FREG(28);
	FREG(29);
	FREG(30);
	FREG(31);
	CREG(Fpscr);
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
}

/* retrieve the current instruction pointer of a thread */
void *get_thread_ip( struct thread *thread )
{
    CONTEXT context;
    context.Iar = 0;
    if (suspend_for_ptrace( thread ))
    {
        get_thread_context_ptrace( thread, CONTEXT_CONTROL, &context );
        resume_after_ptrace( thread );
    }
    return (void *)context.Iar;
}

/* retrieve the thread context */
void get_thread_context( struct thread *thread, CONTEXT *context, unsigned int flags )
{
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

#endif  /* __powerpc__ */
