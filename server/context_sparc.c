/*
 * Sparc register context support
 *
 * Copyright (C) 2000 Ulrich Weigand
 */

#include "config.h"

#ifdef __sparc__

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "winbase.h"

#include "thread.h"
#include "request.h"


#if defined(__sun) || defined(__sun__)

/* retrieve a thread context */
static void get_thread_context( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    int pid = thread->unix_pid;
    if (flags & CONTEXT_FULL)
    {
        struct regs regs;
        if (ptrace( PTRACE_GETREGS, pid, 0, (int) &regs ) == -1) goto error;
        if (flags & CONTEXT_INTEGER)
        {
            context->g0 = 0;
            context->g1 = regs.r_g1;
            context->g2 = regs.r_g2;
            context->g3 = regs.r_g3;
            context->g4 = regs.r_g4;
            context->g5 = regs.r_g5;
            context->g6 = regs.r_g6;
            context->g7 = regs.r_g7;

            context->o0 = regs.r_o0;
            context->o1 = regs.r_o1;
            context->o2 = regs.r_o2;
            context->o3 = regs.r_o3;
            context->o4 = regs.r_o4;
            context->o5 = regs.r_o5;
            context->o6 = regs.r_o6;
            context->o7 = regs.r_o7;

            /* FIXME: local and in registers */
        }
        if (flags & CONTEXT_CONTROL)
        {
            context->psr = regs.r_psr;
            context->pc  = regs.r_pc;
            context->npc = regs.r_npc;
            context->y   = regs.r_y;
            context->wim = 0;  /* FIXME */
            context->tbr = 0;  /* FIXME */
        }
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* FIXME */
    }
    return;
 error:
    file_set_error();
}


/* set a thread context */
static void set_thread_context( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    /* FIXME */
}

#else  /* __sun__ */
#error You must implement get/set_thread_context for your platform
#endif  /* __sun__ */


/* copy a context structure according to the flags */
static void copy_context( CONTEXT *to, CONTEXT *from, int flags )
{
    if (flags & CONTEXT_CONTROL)
    {
        to->psr    = from->psr;
        to->pc     = from->pc;
        to->npc    = from->npc;
        to->y      = from->y;
        to->wim    = from->wim;
        to->tbr    = from->tbr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->g0 = from->g0;
        to->g1 = from->g1;
        to->g2 = from->g2;
        to->g3 = from->g3;
        to->g4 = from->g4;
        to->g5 = from->g5;
        to->g6 = from->g6;
        to->g7 = from->g7;
        to->o0 = from->o0;
        to->o1 = from->o1;
        to->o2 = from->o2;
        to->o3 = from->o3;
        to->o4 = from->o4;
        to->o5 = from->o5;
        to->o6 = from->o6;
        to->o7 = from->o7;
        to->l0 = from->l0;
        to->l1 = from->l1;
        to->l2 = from->l2;
        to->l3 = from->l3;
        to->l4 = from->l4;
        to->l5 = from->l5;
        to->l6 = from->l6;
        to->l7 = from->l7;
        to->i0 = from->i0;
        to->i1 = from->i1;
        to->i2 = from->i2;
        to->i3 = from->i3;
        to->i4 = from->i4;
        to->i5 = from->i5;
        to->i6 = from->i6;
        to->i7 = from->i7;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* FIXME */
    }
}

/* retrieve the current instruction pointer of a thread */
void *get_thread_ip( struct thread *thread )
{
    CONTEXT context;
    context.pc = 0;
    if (suspend_for_ptrace( thread ))
    {
        get_thread_context( thread, CONTEXT_CONTROL, &context );
        resume_thread( thread );
    }
    return (void *)context.pc;
}

/* retrieve the current context of a thread */
DECL_HANDLER(get_thread_context)
{
    struct thread *thread;
    int flags = req->flags & ~CONTEXT_SPARC;  /* get rid of CPU id */

    if ((thread = get_thread_from_handle( req->handle, THREAD_GET_CONTEXT )))
    {
        if (thread->context)  /* thread is inside an exception event */
        {
            copy_context( &req->context, thread->context, flags );
            flags = 0;
        }
        if (flags && suspend_for_ptrace( thread ))
        {
            get_thread_context( thread, flags, &req->context );
            resume_thread( thread );
        }
        release_object( thread );
    }
}


/* set the current context of a thread */
DECL_HANDLER(set_thread_context)
{
    struct thread *thread;
    int flags = req->flags & ~CONTEXT_SPARC;  /* get rid of CPU id */

    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT )))
    {
        if (thread->context)  /* thread is inside an exception event */
        {
            copy_context( thread->context, &req->context, flags );
            flags = 0;
        }
        if (flags && suspend_for_ptrace( thread ))
        {
            set_thread_context( thread, flags, &req->context );
            resume_thread( thread );
        }
        release_object( thread );
    }
}

#endif  /* __sparc__ */
