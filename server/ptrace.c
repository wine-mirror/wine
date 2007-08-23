/*
 * Server-side ptrace support
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_SYS_PTRACE_H
# include <sys/ptrace.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_THR_H
# include <sys/ucontext.h>
# include <sys/thr.h>
#endif
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "process.h"
#include "thread.h"

#ifdef USE_PTRACE

#ifndef PTRACE_CONT
#define PTRACE_CONT PT_CONTINUE
#endif
#ifndef PTRACE_SINGLESTEP
#define PTRACE_SINGLESTEP PT_STEP
#endif
#ifndef PTRACE_ATTACH
#define PTRACE_ATTACH PT_ATTACH
#endif
#ifndef PTRACE_DETACH
#define PTRACE_DETACH PT_DETACH
#endif
#ifndef PTRACE_PEEKDATA
#define PTRACE_PEEKDATA PT_READ_D
#endif
#ifndef PTRACE_POKEDATA
#define PTRACE_POKEDATA PT_WRITE_D
#endif
#ifndef PTRACE_PEEKUSER
#define PTRACE_PEEKUSER PT_READ_U
#endif
#ifndef PTRACE_POKEUSER
#define PTRACE_POKEUSER PT_WRITE_U
#endif

#ifdef PT_GETDBREGS
#define PTRACE_GETDBREGS PT_GETDBREGS
#endif
#ifdef PT_SETDBREGS
#define PTRACE_SETDBREGS PT_SETDBREGS
#endif

#ifndef HAVE_SYS_PTRACE_H
#define PT_CONTINUE 0
#define PT_ATTACH   1
#define PT_DETACH   2
#define PT_READ_D   3
#define PT_WRITE_D  4
#define PT_STEP     5
static inline int ptrace(int req, ...) { errno = EPERM; return -1; /*FAIL*/ }
#endif  /* HAVE_SYS_PTRACE_H */

/* handle a status returned by wait4 */
static int handle_child_status( struct thread *thread, int pid, int status, int want_sig )
{
    if (WIFSTOPPED(status))
    {
        int sig = WSTOPSIG(status);
        if (debug_level && thread)
            fprintf( stderr, "%04x: *signal* signal=%d\n", thread->id, sig );
        if (sig != want_sig)
        {
            /* ignore other signals for now */
            ptrace( PTRACE_CONT, pid, (caddr_t)1, sig );
        }
        return sig;
    }
    if (thread && (WIFSIGNALED(status) || WIFEXITED(status)))
    {
        thread->unix_pid = -1;
        thread->unix_tid = -1;
        if (debug_level)
        {
            if (WIFSIGNALED(status))
                fprintf( stderr, "%04x: *exited* signal=%d\n",
                         thread->id, WTERMSIG(status) );
            else
                fprintf( stderr, "%04x: *exited* status=%d\n",
                         thread->id, WEXITSTATUS(status) );
        }
    }
    return 0;
}

/* wait4 wrapper to handle missing __WALL flag in older kernels */
static inline pid_t wait4_wrapper( pid_t pid, int *status, int options, struct rusage *usage )
{
#ifdef __WALL
    static int wall_flag = __WALL;

    for (;;)
    {
        pid_t ret = wait4( pid, status, options | wall_flag, usage );
        if (ret != -1 || !wall_flag || errno != EINVAL) return ret;
        wall_flag = 0;
    }
#else
    return wait4( pid, status, options, usage );
#endif
}

/* handle a SIGCHLD signal */
void sigchld_callback(void)
{
    int pid, status;

    for (;;)
    {
        if (!(pid = wait4_wrapper( -1, &status, WUNTRACED | WNOHANG, NULL ))) break;
        if (pid != -1)
        {
            struct thread *thread = get_thread_from_tid( pid );
            if (!thread) thread = get_thread_from_pid( pid );
            handle_child_status( thread, pid, status, -1 );
        }
        else break;
    }
}

/* return the Unix pid to use in ptrace calls for a given process */
static int get_ptrace_pid( struct thread *thread )
{
#ifdef linux  /* linux always uses thread id */
    if (thread->unix_tid != -1) return thread->unix_tid;
#endif
    return thread->unix_pid;
}

/* return the Unix tid to use in ptrace calls for a given thread */
static int get_ptrace_tid( struct thread *thread )
{
    if (thread->unix_tid != -1) return thread->unix_tid;
    return thread->unix_pid;
}

/* wait for a ptraced child to get a certain signal */
static int wait4_thread( struct thread *thread, int signal )
{
    int res, status;

    start_watchdog();
    for (;;)
    {
        if ((res = wait4_wrapper( get_ptrace_pid(thread), &status, WUNTRACED, NULL )) == -1)
        {
            if (errno == EINTR)
            {
                if (!watchdog_triggered()) continue;
                if (debug_level) fprintf( stderr, "%04x: *watchdog* wait4 aborted\n", thread->id );
            }
            else if (errno == ECHILD)  /* must have died */
            {
                thread->unix_pid = -1;
                thread->unix_tid = -1;
            }
            else perror( "wait4" );
            stop_watchdog();
            return 0;
        }
        res = handle_child_status( thread, res, status, signal );
        if (!res || res == signal) break;
    }
    stop_watchdog();
    return (thread->unix_pid != -1);
}

/* send a signal to a specific thread */
static inline int tkill( int tgid, int pid, int sig )
{
#ifdef __linux__
    int ret = -ENOSYS;
# ifdef __i386__
    __asm__( "pushl %%ebx\n\t"
             "movl %2,%%ebx\n\t"
             "int $0x80\n\t"
             "popl %%ebx\n\t"
             : "=a" (ret)
             : "0" (270) /*SYS_tgkill*/, "r" (tgid), "c" (pid), "d" (sig) );
    if (ret == -ENOSYS)
        __asm__( "pushl %%ebx\n\t"
                 "movl %2,%%ebx\n\t"
                 "int $0x80\n\t"
                 "popl %%ebx\n\t"
                 : "=a" (ret)
                 : "0" (238) /*SYS_tkill*/, "r" (pid), "c" (sig) );
# elif defined(__x86_64__)
    __asm__( "syscall" : "=a" (ret)
             : "0" (200) /*SYS_tkill*/, "D" (pid), "S" (sig) );
# endif
    if (ret >= 0) return ret;
    errno = -ret;
    return -1;
#elif defined(__FreeBSD__) && defined(HAVE_THR_KILL2)
    return thr_kill2( tgid, pid, sig );
#else
    errno = ENOSYS;
    return -1;
#endif
}

/* initialize the process tracing mechanism */
void init_tracing_mechanism(void)
{
    /* no initialization needed for ptrace */
}

/* initialize the per-process tracing mechanism */
void init_process_tracing( struct process *process )
{
    /* ptrace setup is done on-demand */
}

/* terminate the per-process tracing mechanism */
void finish_process_tracing( struct process *process )
{
}

/* send a Unix signal to a specific thread */
int send_thread_signal( struct thread *thread, int sig )
{
    int ret = -1;

    if (thread->unix_pid != -1)
    {
        if (thread->unix_tid != -1)
        {
            ret = tkill( thread->unix_pid, thread->unix_tid, sig );
            if (ret == -1 && errno == ENOSYS) ret = kill( thread->unix_pid, sig );
        }
        else ret = kill( thread->unix_pid, sig );

        if (ret == -1 && errno == ESRCH) /* thread got killed */
        {
            thread->unix_pid = -1;
            thread->unix_tid = -1;
        }
    }
    if (debug_level && ret != -1)
        fprintf( stderr, "%04x: *sent signal* signal=%d\n", thread->id, sig );
    return (ret != -1);
}

/* resume a thread after we have used ptrace on it */
static void resume_after_ptrace( struct thread *thread )
{
    if (thread->unix_pid == -1) return;
    if (ptrace( PTRACE_DETACH, get_ptrace_pid(thread), (caddr_t)1, 0 ) == -1)
    {
        if (errno == ESRCH) thread->unix_pid = thread->unix_tid = -1;  /* thread got killed */
    }
}

/* suspend a thread to allow using ptrace on it */
/* you must do a resume_after_ptrace when finished with the thread */
static int suspend_for_ptrace( struct thread *thread )
{
    /* can't stop a thread while initialisation is in progress */
    if (thread->unix_pid == -1 || !is_process_init_done(thread->process)) goto error;

    /* this may fail if the client is already being debugged */
    if (ptrace( PTRACE_ATTACH, get_ptrace_pid(thread), 0, 0 ) == -1)
    {
        if (errno == ESRCH) thread->unix_pid = thread->unix_tid = -1;  /* thread got killed */
        goto error;
    }
    if (wait4_thread( thread, SIGSTOP )) return 1;
    resume_after_ptrace( thread );
 error:
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}

/* read an int from a thread address space */
static int read_thread_int( struct thread *thread, int *addr, int *data )
{
    errno = 0;
    *data = ptrace( PTRACE_PEEKDATA, get_ptrace_pid(thread), (caddr_t)addr, 0 );
    if ( *data == -1 && errno)
    {
        file_set_error();
        return -1;
    }
    return 0;
}

/* write an int to a thread address space */
static int write_thread_int( struct thread *thread, int *addr, int data, unsigned int mask )
{
    int res;
    if (mask != ~0u)
    {
        if (read_thread_int( thread, addr, &res ) == -1) return -1;
        data = (data & mask) | (res & ~mask);
    }
    if ((res = ptrace( PTRACE_POKEDATA, get_ptrace_pid(thread), (caddr_t)addr, data )) == -1)
        file_set_error();
    return res;
}

/* return a thread of the process suitable for ptracing */
static struct thread *get_ptrace_thread( struct process *process )
{
    struct thread *thread;

    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread->unix_pid != -1) return thread;
    }
    set_error( STATUS_ACCESS_DENIED );  /* process is dead */
    return NULL;
}

/* read data from a process memory space */
int read_process_memory( struct process *process, const void *ptr, data_size_t size, char *dest )
{
    struct thread *thread = get_ptrace_thread( process );
    unsigned int first_offset, last_offset, len;
    int data, *addr;

    if (!thread) return 0;

    first_offset = (unsigned long)ptr % sizeof(int);
    last_offset = (size + first_offset) % sizeof(int);
    if (!last_offset) last_offset = sizeof(int);

    addr = (int *)((char *)ptr - first_offset);
    len = (size + first_offset + sizeof(int) - 1) / sizeof(int);

    if (suspend_for_ptrace( thread ))
    {
        if (len > 1)
        {
            if (read_thread_int( thread, addr++, &data ) == -1) goto done;
            memcpy( dest, (char *)&data + first_offset, sizeof(int) - first_offset );
            dest += sizeof(int) - first_offset;
            first_offset = 0;
            len--;
        }

        while (len > 1)
        {
            if (read_thread_int( thread, addr++, &data ) == -1) goto done;
            memcpy( dest, &data, sizeof(int) );
            dest += sizeof(int);
            len--;
        }

        if (read_thread_int( thread, addr++, &data ) == -1) goto done;
        memcpy( dest, (char *)&data + first_offset, last_offset - first_offset );
        len--;

    done:
        resume_after_ptrace( thread );
    }
    return !len;
}

/* make sure we can write to the whole address range */
/* len is the total size (in ints) */
static int check_process_write_access( struct thread *thread, int *addr, data_size_t len )
{
    int page = get_page_size() / sizeof(int);

    for (;;)
    {
        if (write_thread_int( thread, addr, 0, 0 ) == -1) return 0;
        if (len <= page) break;
        addr += page;
        len -= page;
    }
    return (write_thread_int( thread, addr + len - 1, 0, 0 ) != -1);
}

/* write data to a process memory space */
int write_process_memory( struct process *process, void *ptr, data_size_t size, const char *src )
{
    struct thread *thread = get_ptrace_thread( process );
    int ret = 0, data = 0;
    data_size_t len;
    int *addr;
    unsigned int first_mask, first_offset, last_mask, last_offset;

    if (!thread) return 0;

    /* compute the mask for the first int */
    first_mask = ~0;
    first_offset = (unsigned long)ptr % sizeof(int);
    memset( &first_mask, 0, first_offset );

    /* compute the mask for the last int */
    last_offset = (size + first_offset) % sizeof(int);
    if (!last_offset) last_offset = sizeof(int);
    last_mask = 0;
    memset( &last_mask, 0xff, last_offset );

    addr = (int *)((char *)ptr - first_offset);
    len = (size + first_offset + sizeof(int) - 1) / sizeof(int);

    if (suspend_for_ptrace( thread ))
    {
        if (!check_process_write_access( thread, addr, len ))
        {
            set_error( STATUS_ACCESS_DENIED );
            goto done;
        }
        /* first word is special */
        if (len > 1)
        {
            memcpy( (char *)&data + first_offset, src, sizeof(int) - first_offset );
            src += sizeof(int) - first_offset;
            if (write_thread_int( thread, addr++, data, first_mask ) == -1) goto done;
            first_offset = 0;
            len--;
        }
        else last_mask &= first_mask;

        while (len > 1)
        {
            memcpy( &data, src, sizeof(int) );
            src += sizeof(int);
            if (write_thread_int( thread, addr++, data, ~0 ) == -1) goto done;
            len--;
        }

        /* last word is special too */
        memcpy( (char *)&data + first_offset, src, last_offset - first_offset );
        if (write_thread_int( thread, addr, data, last_mask ) == -1) goto done;
        ret = 1;

    done:
        resume_after_ptrace( thread );
    }
    return ret;
}

/* retrieve an LDT selector entry */
void get_selector_entry( struct thread *thread, int entry, unsigned int *base,
                         unsigned int *limit, unsigned char *flags )
{
    if (!thread->process->ldt_copy)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (entry >= 8192)
    {
        set_error( STATUS_ACCESS_VIOLATION );
        return;
    }
    if (suspend_for_ptrace( thread ))
    {
        unsigned char flags_buf[4];
        int *addr = (int *)thread->process->ldt_copy + entry;
        if (read_thread_int( thread, addr, (int *)base ) == -1) goto done;
        if (read_thread_int( thread, addr + 8192, (int *)limit ) == -1) goto done;
        addr = (int *)thread->process->ldt_copy + 2*8192 + (entry >> 2);
        if (read_thread_int( thread, addr, (int *)flags_buf ) == -1) goto done;
        *flags = flags_buf[entry & 3];
    done:
        resume_after_ptrace( thread );
    }
}


#if defined(linux) && (defined(__i386__) || defined(__x86_64__))

#ifdef HAVE_SYS_USER_H
# include <sys/user.h>
#endif

/* debug register offset in struct user */
#define DR_OFFSET(dr) ((((struct user *)0)->u_debugreg) + (dr))

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, CONTEXT *context, unsigned int flags )
{
    int i, pid = get_ptrace_tid(thread);
    long data[8];

    /* all other regs are handled on the client side */
    assert( (flags | CONTEXT_i386) == CONTEXT_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

    for (i = 0; i < 8; i++)
    {
        if (i == 4 || i == 5) continue;
        errno = 0;
        data[i] = ptrace( PTRACE_PEEKUSER, pid, DR_OFFSET(i), 0 );
        if ((data[i] == -1) && errno)
        {
            file_set_error();
            goto done;
        }
    }
    context->Dr0 = data[0];
    context->Dr1 = data[1];
    context->Dr2 = data[2];
    context->Dr3 = data[3];
    context->Dr6 = data[6];
    context->Dr7 = data[7];
    context->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
done:
    resume_after_ptrace( thread );
}

/* set the thread x86 registers */
void set_thread_context( struct thread *thread, const CONTEXT *context, unsigned int flags )
{
    int pid = get_ptrace_tid( thread );

    /* all other regs are handled on the client side */
    assert( (flags | CONTEXT_i386) == CONTEXT_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

    if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(0), context->Dr0 ) == -1) goto error;
    if (thread->context) thread->context->Dr0 = context->Dr0;
    if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(1), context->Dr1 ) == -1) goto error;
    if (thread->context) thread->context->Dr1 = context->Dr1;
    if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(2), context->Dr2 ) == -1) goto error;
    if (thread->context) thread->context->Dr2 = context->Dr2;
    if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(3), context->Dr3 ) == -1) goto error;
    if (thread->context) thread->context->Dr3 = context->Dr3;
    if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(6), context->Dr6 ) == -1) goto error;
    if (thread->context) thread->context->Dr6 = context->Dr6;
    if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), context->Dr7 ) == -1) goto error;
    if (thread->context) thread->context->Dr7 = context->Dr7;
    resume_after_ptrace( thread );
    return;
 error:
    file_set_error();
    resume_after_ptrace( thread );
}

#elif defined(__i386__) && defined(PTRACE_GETDBREGS) && defined(PTRACE_SETDBREGS) && \
    (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__NetBSD__))

#include <machine/reg.h>

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, CONTEXT *context, unsigned int flags )
{
    int pid = get_ptrace_tid(thread);
    struct dbreg dbregs;

    /* all other regs are handled on the client side */
    assert( (flags | CONTEXT_i386) == CONTEXT_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

    if (ptrace( PTRACE_GETDBREGS, pid, (caddr_t) &dbregs, 0 ) == -1) file_set_error();
    else
    {
#ifdef DBREG_DRX
        /* needed for FreeBSD, the structure fields have changed under 5.x */
        context->Dr0 = DBREG_DRX((&dbregs), 0);
        context->Dr1 = DBREG_DRX((&dbregs), 1);
        context->Dr2 = DBREG_DRX((&dbregs), 2);
        context->Dr3 = DBREG_DRX((&dbregs), 3);
        context->Dr6 = DBREG_DRX((&dbregs), 6);
        context->Dr7 = DBREG_DRX((&dbregs), 7);
#else
        context->Dr0 = dbregs.dr0;
        context->Dr1 = dbregs.dr1;
        context->Dr2 = dbregs.dr2;
        context->Dr3 = dbregs.dr3;
        context->Dr6 = dbregs.dr6;
        context->Dr7 = dbregs.dr7;
#endif
        context->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
    }
    resume_after_ptrace( thread );
}

/* set the thread x86 registers */
void set_thread_context( struct thread *thread, const CONTEXT *context, unsigned int flags )
{
    int pid = get_ptrace_tid(thread);
    struct dbreg dbregs;

    /* all other regs are handled on the client side */
    assert( (flags | CONTEXT_i386) == CONTEXT_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

#ifdef DBREG_DRX
    /* needed for FreeBSD, the structure fields have changed under 5.x */
    DBREG_DRX((&dbregs), 0) = context->Dr0;
    DBREG_DRX((&dbregs), 1) = context->Dr1;
    DBREG_DRX((&dbregs), 2) = context->Dr2;
    DBREG_DRX((&dbregs), 3) = context->Dr3;
    DBREG_DRX((&dbregs), 4) = 0;
    DBREG_DRX((&dbregs), 5) = 0;
    DBREG_DRX((&dbregs), 6) = context->Dr6;
    DBREG_DRX((&dbregs), 7) = context->Dr7;
#else
    dbregs.dr0 = context->Dr0;
    dbregs.dr1 = context->Dr1;
    dbregs.dr2 = context->Dr2;
    dbregs.dr3 = context->Dr3;
    dbregs.dr4 = 0;
    dbregs.dr5 = 0;
    dbregs.dr6 = context->Dr6;
    dbregs.dr7 = context->Dr7;
#endif
    if (ptrace( PTRACE_SETDBREGS, pid, (caddr_t) &dbregs, 0 ) == -1) file_set_error();
    else if (thread->context)  /* update the cached values */
    {
        thread->context->Dr0 = context->Dr0;
        thread->context->Dr1 = context->Dr1;
        thread->context->Dr2 = context->Dr2;
        thread->context->Dr3 = context->Dr3;
        thread->context->Dr6 = context->Dr6;
        thread->context->Dr7 = context->Dr7;
    }
    resume_after_ptrace( thread );
}

#else  /* linux || __FreeBSD__ */

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, CONTEXT *context, unsigned int flags )
{
}

/* set the thread x86 debug registers */
void set_thread_context( struct thread *thread, const CONTEXT *context, unsigned int flags )
{
}

#endif  /* linux || __FreeBSD__ */

#endif  /* USE_PTRACE */
