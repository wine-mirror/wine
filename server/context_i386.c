/*
 * i386 register context support
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include "config.h"

#ifdef __i386__

#include <assert.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "winbase.h"
#include "winerror.h"

#include "thread.h"
#include "request.h"


#ifndef PTRACE_PEEKUSER
#define PTRACE_PEEKUSER PT_READ_U
#endif
#ifndef PTRACE_POKEUSER
#define PTRACE_POKEUSER PT_WRITE_U
#endif
#ifndef PTRACE_GETREGS
#define PTRACE_GETREGS PT_GETREGS
#endif
#ifndef PTRACE_GETFPREGS
#define PTRACE_GETFPREGS PT_GETFPREGS
#endif

#ifdef linux

/* debug register offset in struct user */
#define DR_OFFSET(dr) ((int)((((struct user *)0)->u_debugreg) + (dr)))

/* retrieve a debug register */
static inline int get_debug_reg( int pid, int num, DWORD *data )
{
    int res = ptrace( PTRACE_PEEKUSER, pid, DR_OFFSET(num), 0 );
    if ((res == -1) && errno)
    {
        file_set_error();
        return -1;
    }
    *data = res;
    return 0;
}

/* retrieve a thread context */
static void get_thread_context( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    int pid = thread->unix_pid;
    if (flags & CONTEXT_FULL)
    {
        struct user_regs_struct regs;
        if (ptrace( PTRACE_GETREGS, pid, 0, &regs ) == -1) goto error;
        if (flags & CONTEXT_INTEGER)
        {
            context->Eax = regs.eax;
            context->Ebx = regs.ebx;
            context->Ecx = regs.ecx;
            context->Edx = regs.edx;
            context->Esi = regs.esi;
            context->Edi = regs.edi;
        }
        if (flags & CONTEXT_CONTROL)
        {
            context->Ebp    = regs.ebp;
            context->Esp    = regs.esp;
            context->Eip    = regs.eip;
            context->SegCs  = regs.xcs & 0xffff;
            context->SegSs  = regs.xss & 0xffff;
            context->EFlags = regs.eflags;
        }
        if (flags & CONTEXT_SEGMENTS)
        {
            context->SegDs = regs.xds & 0xffff;
            context->SegEs = regs.xes & 0xffff;
            context->SegFs = regs.xfs & 0xffff;
            context->SegGs = regs.xgs & 0xffff;
        }
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        if (get_debug_reg( pid, 0, &context->Dr0 ) == -1) goto error;
        if (get_debug_reg( pid, 1, &context->Dr1 ) == -1) goto error;
        if (get_debug_reg( pid, 2, &context->Dr2 ) == -1) goto error;
        if (get_debug_reg( pid, 3, &context->Dr3 ) == -1) goto error;
        if (get_debug_reg( pid, 6, &context->Dr6 ) == -1) goto error;
        if (get_debug_reg( pid, 7, &context->Dr7 ) == -1) goto error;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* we can use context->FloatSave directly as it is using the */
        /* correct structure (the same as fsave/frstor) */
        if (ptrace( PTRACE_GETFPREGS, pid, 0, &context->FloatSave ) == -1) goto error;
        context->FloatSave.Cr0NpxState = 0;  /* FIXME */
    }
    return;
 error:
    file_set_error();
}


/* set a thread context */
static void set_thread_context( struct thread *thread, unsigned int flags, CONTEXT *context )
{
    int pid = thread->unix_pid;
    if (flags & CONTEXT_FULL)
    {
        struct user_regs_struct regs;
        if ((flags & CONTEXT_FULL) != CONTEXT_FULL)  /* need to preserve some registers */
        {
            if (ptrace( PTRACE_GETREGS, pid, 0, &regs ) == -1) goto error;
        }
        if (flags & CONTEXT_INTEGER)
        {
            regs.eax = context->Eax;
            regs.ebx = context->Ebx;
            regs.ecx = context->Ecx;
            regs.edx = context->Edx;
            regs.esi = context->Esi;
            regs.edi = context->Edi;
        }
        if (flags & CONTEXT_CONTROL)
        {
            regs.ebp = context->Ebp;
            regs.esp = context->Esp;
            regs.eip = context->Eip;
            regs.xcs = context->SegCs;
            regs.xss = context->SegSs;
            regs.eflags = context->EFlags;
        }
        if (flags & CONTEXT_SEGMENTS)
        {
            regs.xds = context->SegDs;
            regs.xes = context->SegEs;
            regs.xfs = context->SegFs;
            regs.xgs = context->SegGs;
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
        if (ptrace( PTRACE_SETFPREGS, pid, 0, &context->FloatSave ) == -1) goto error;
        context->FloatSave.Cr0NpxState = 0;  /* FIXME */
    }
    return;
 error:
    file_set_error();
}

#else  /* linux */
#error You must implement get/set_thread_context for your platform
#endif  /* linux */


/* copy a context structure according to the flags */
static void copy_context( CONTEXT *to, CONTEXT *from, int flags )
{
    if (flags & CONTEXT_CONTROL)
    {
        to->Ebp    = from->Ebp;
        to->Eip    = from->Eip;
        to->Esp    = from->Esp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Eax = from->Eax;
        to->Ebx = from->Ebx;
        to->Ecx = from->Ecx;
        to->Edx = from->Edx;
        to->Esi = from->Esi;
        to->Edi = from->Edi;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->SegDs = from->SegDs;
        to->SegEs = from->SegEs;
        to->SegFs = from->SegFs;
        to->SegGs = from->SegGs;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->Dr0 = from->Dr0;
        to->Dr1 = from->Dr1;
        to->Dr2 = from->Dr2;
        to->Dr3 = from->Dr3;
        to->Dr6 = from->Dr6;
        to->Dr7 = from->Dr7;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->FloatSave = from->FloatSave;
    }
}

/* retrieve the current context of a thread */
DECL_HANDLER(get_thread_context)
{
    struct thread *thread;
    CONTEXT *context;

    if ((thread = get_thread_from_handle( req->handle, THREAD_GET_CONTEXT )))
    {
        if ((context = get_debug_context( thread )))  /* thread is inside an exception event */
        {
            copy_context( &req->context, context, req->flags );
        }
        else
        {
            suspend_thread( thread, 0 );
            if (thread->attached) get_thread_context( thread, req->flags, &req->context );
            else set_error( ERROR_ACCESS_DENIED );
            resume_thread( thread );
        }
        release_object( thread );
    }
}


/* set the current context of a thread */
DECL_HANDLER(set_thread_context)
{
    struct thread *thread;
    CONTEXT *context;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT )))
    {
        if ((context = get_debug_context( thread )))  /* thread is inside an exception event */
        {
            copy_context( context, &req->context, req->flags );
        }
        else
        {
            suspend_thread( thread, 0 );
            if (thread->attached) set_thread_context( thread, req->flags, &req->context );
            else set_error( ERROR_ACCESS_DENIED );
            resume_thread( thread );
        }
        release_object( thread );
    }
}

#endif  /* __i386__ */
