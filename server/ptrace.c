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
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_PTRACE_H
# include <sys/ptrace.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif
#ifdef HAVE_SYS_THR_H
# include <sys/thr.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
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

#ifndef __WALL
#define __WALL 0
#endif

static const char *get_signal_name( int sig )
{
    static char buffer[20];
    switch (sig)
    {
#define X(x) case x: return #x
    X(SIGABRT);
    X(SIGBUS);
    X(SIGFPE);
    X(SIGILL);
    X(SIGINT);
    X(SIGQUIT);
    X(SIGSEGV);
    X(SIGSTOP);
    X(SIGTRAP);
    X(SIGUSR1);
    X(SIGUSR2);
#undef X
    default:
        sprintf( buffer, "signal=%d", sig );
        return buffer;
    }
}

/* handle a status returned by waitpid */
static int handle_child_status( struct thread *thread, int pid, int status, int want_sig )
{
    if (WIFSTOPPED(status))
    {
        int sig = WSTOPSIG(status);
        if (debug_level && thread)
            fprintf( stderr, "%04x: *signal* %s\n", thread->id, get_signal_name( sig ));
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
                fprintf( stderr, "%04x: *exited* %s\n",
                         thread->id, get_signal_name( WTERMSIG(status) ));
            else
                fprintf( stderr, "%04x: *exited* status=%d\n",
                         thread->id, WEXITSTATUS(status) );
        }
    }
    return 0;
}

/* handle a SIGCHLD signal */
void sigchld_callback(void)
{
    int pid, status;

    for (;;)
    {
        if (!(pid = waitpid( -1, &status, WUNTRACED | WNOHANG | __WALL ))) break;
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
    return thread->unix_tid;
#endif
    return thread->unix_pid;
}

/* wait for a ptraced child to get a certain signal */
static int waitpid_thread( struct thread *thread, int signal )
{
    int res, status;

    start_watchdog();
    for (;;)
    {
        if ((res = waitpid( get_ptrace_pid(thread), &status, WUNTRACED | __WALL )) == -1)
        {
            if (errno == EINTR)
            {
                if (!watchdog_triggered()) continue;
                if (debug_level) fprintf( stderr, "%04x: *watchdog* waitpid aborted\n", thread->id );
            }
            else if (errno == ECHILD)  /* must have died */
            {
                thread->unix_pid = -1;
                thread->unix_tid = -1;
            }
            else perror( "waitpid" );
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
    int ret = syscall( __NR_tgkill, tgid, pid, sig );
    if (ret < 0 && errno == ENOSYS) ret = syscall( __NR_tkill, pid, sig );
    return ret;
#elif (defined(__FreeBSD__) || defined (__FreeBSD_kernel__)) && defined(HAVE_THR_KILL2)
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

    if (thread->unix_tid != -1)
    {
        ret = tkill( thread->unix_pid, thread->unix_tid, sig );
        if (ret == -1 && errno == ENOSYS) ret = kill( thread->unix_pid, sig );
    }
    if (ret == -1 && errno == ESRCH) /* thread got killed */
    {
        thread->unix_pid = -1;
        thread->unix_tid = -1;
    }

    if (debug_level && ret != -1)
        fprintf( stderr, "%04x: *sent signal* %s\n", thread->id, get_signal_name( sig ));
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
    if (waitpid_thread( thread, SIGSTOP )) return 1;
    resume_after_ptrace( thread );
 error:
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}

/* read a long from a thread address space */
static int read_thread_long( struct thread *thread, void *addr, unsigned long *data )
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

/* write a long to a thread address space */
static long write_thread_long( struct thread *thread, void *addr, unsigned long data, unsigned long mask )
{
    unsigned long old_data;
    int res;

    if (mask != ~0ul)
    {
        if (read_thread_long( thread, addr, &old_data ) == -1) return -1;
        data = (data & mask) | (old_data & ~mask);
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
    set_error( STATUS_PROCESS_IS_TERMINATING );  /* process is dead */
    return NULL;
}

#ifdef HAVE_PROCESS_VM_READV
static int read_process_memory_vm( struct thread *thread, client_ptr_t ptr, data_size_t size, char *dest )
{
    static int not_supported;
    struct iovec local, remote;
    ssize_t len;

    if (not_supported) return -1;
    if (thread->unix_pid == -1 || !is_process_init_done( thread->process ))
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    local.iov_len = remote.iov_len = size;
    local.iov_base = dest;
    remote.iov_base = (void *)(unsigned long)ptr;
    len = process_vm_readv( thread->unix_pid, &local, 1, &remote, 1, 0 );
    if (len < 0 && (errno == ENOSYS || errno == EPERM))
    {
        not_supported = 1;
        return -1;
    }
    if (len == size) return 1;
    set_error( len >= 0 ? STATUS_PARTIAL_COPY : STATUS_ACCESS_DENIED );
    return 0;
}
#else
static int read_process_memory_vm( struct thread *thread, client_ptr_t ptr, data_size_t size, char *dest )
{
    return -1;
}
#endif

static int read_process_memory_ptrace( struct thread *thread, client_ptr_t ptr, data_size_t size, char *dest )
{
    unsigned int first_offset, last_offset, len;
    unsigned long data, *addr;

    first_offset = ptr % sizeof(long);
    last_offset = (size + first_offset) % sizeof(long);
    if (!last_offset) last_offset = sizeof(long);

    addr = (unsigned long *)(unsigned long)(ptr - first_offset);
    len = (size + first_offset + sizeof(long) - 1) / sizeof(long);

    if (suspend_for_ptrace( thread ))
    {
        if (len > 3)  /* /proc/pid/mem should be faster for large sizes */
        {
            char procmem[24];
            int fd;

            snprintf( procmem, sizeof(procmem), "/proc/%u/mem", thread->process->unix_pid );
            if ((fd = open( procmem, O_RDONLY )) != -1)
            {
                ssize_t ret = pread( fd, dest, size, ptr );
                close( fd );
                if (ret == size)
                {
                    len = 0;
                    goto done;
                }
            }
        }

        if (len > 1)
        {
            if (read_thread_long( thread, addr++, &data ) == -1) goto done;
            dest = mem_append( dest, (char *)&data + first_offset, sizeof(long) - first_offset );
            first_offset = 0;
            len--;
        }

        while (len > 1)
        {
            if (read_thread_long( thread, addr++, &data ) == -1) goto done;
            dest = mem_append( dest, &data, sizeof(long) );
            len--;
        }

        if (read_thread_long( thread, addr++, &data ) == -1) goto done;
        memcpy( dest, (char *)&data + first_offset, last_offset - first_offset );
        len--;

    done:
        resume_after_ptrace( thread );
    }
    return !len;
}

/* read data from a process memory space */
int read_process_memory( struct process *process, client_ptr_t ptr, data_size_t size, char *dest )
{
    struct thread *thread = get_ptrace_thread( process );
    int ret;

    if (!thread) return 0;

    if ((unsigned long)ptr != ptr)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    if ((ret = read_process_memory_vm( thread, ptr, size, dest )) != -1) return ret;
    return read_process_memory_ptrace( thread, ptr, size, dest );
}

/* make sure we can write to the whole address range */
/* len is the total size (in longs) */
static int check_process_write_access( struct thread *thread, long *addr, data_size_t len )
{
    size_t page = get_page_size() / sizeof(long);

    for (;;)
    {
        if (write_thread_long( thread, addr, 0, 0 ) == -1) return 0;
        if (len <= page) break;
        addr += page;
        len -= page;
    }
    return (write_thread_long( thread, addr + len - 1, 0, 0 ) != -1);
}

/* write data to a process memory space */
int write_process_memory( struct process *process, client_ptr_t ptr, data_size_t size, const char *src,
                          data_size_t *written )
{
    struct thread *thread = get_ptrace_thread( process );
    int ret = 0;
    long data = 0;
    data_size_t len;
    long *addr;
    unsigned long first_mask, first_offset, last_mask, last_offset;

    if (!thread) return 0;

    if ((unsigned long)ptr != ptr)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    /* compute the mask for the first long */
    first_mask = ~0;
    first_offset = ptr % sizeof(long);
    memset( &first_mask, 0, first_offset );

    /* compute the mask for the last long */
    last_offset = (size + first_offset) % sizeof(long);
    if (!last_offset) last_offset = sizeof(long);
    last_mask = 0;
    memset( &last_mask, 0xff, last_offset );

    addr = (long *)(unsigned long)(ptr - first_offset);
    len = (size + first_offset + sizeof(long) - 1) / sizeof(long);

    if (suspend_for_ptrace( thread ))
    {
        if (!check_process_write_access( thread, addr, len ))
        {
            set_error( STATUS_ACCESS_DENIED );
            goto done;
        }

        if (len > 3)
        {
            char procmem[24];
            int fd;

            snprintf( procmem, sizeof(procmem), "/proc/%u/mem", process->unix_pid );
            if ((fd = open( procmem, O_WRONLY )) != -1)
            {
                ssize_t r = pwrite( fd, src, size, ptr );
                close( fd );
                if (r == size)
                {
                    ret = 1;
                    goto done;
                }
            }
        }

        /* first word is special */
        if (len > 1)
        {
            memcpy( (char *)&data + first_offset, src, sizeof(long) - first_offset );
            src += sizeof(long) - first_offset;
            if (write_thread_long( thread, addr++, data, first_mask ) == -1) goto done;
            first_offset = 0;
            len--;
        }
        else last_mask &= first_mask;

        while (len > 1)
        {
            memcpy( &data, src, sizeof(long) );
            src += sizeof(long);
            if (write_thread_long( thread, addr++, data, ~0ul ) == -1) goto done;
            len--;
        }

        /* last word is special too */
        memcpy( (char *)&data + first_offset, src, last_offset - first_offset );
        if (write_thread_long( thread, addr, data, last_mask ) == -1) goto done;
        ret = 1;

    done:
        resume_after_ptrace( thread );
    }
    if (ret && written) *written = size;
    return ret;
}


#if defined(linux) && (defined(HAVE_SYS_USER_H) || defined(HAVE_ASM_USER_H)) \
    && (defined(__i386__) || defined(__x86_64__))

#ifdef HAVE_SYS_USER_H
#include <sys/user.h>
#elif defined(HAVE_ASM_USER_H)
#include <asm/user.h>
#endif

/* debug register offset in struct user */
#define DR_OFFSET(dr) ((((struct user *)0)->u_debugreg) + (dr))

/* initialize registers in new thread if necessary */
void init_thread_context( struct thread *thread )
{
    /* Linux doesn't clear all registers, but hopefully enough to avoid spurious breakpoints */
    thread->system_regs = 0;
}

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, struct context_data *context, unsigned int flags )
{
    int i;
    long data[8];

    /* all other regs are handled on the client side */
    assert( flags == SERVER_CTX_DEBUG_REGISTERS );

    if (!(thread->system_regs & SERVER_CTX_DEBUG_REGISTERS))
    {
        /* caller has initialized everything to 0 already, just return */
        context->flags |= SERVER_CTX_DEBUG_REGISTERS;
        return;
    }

    if (!suspend_for_ptrace( thread )) return;

    for (i = 0; i < 8; i++)
    {
        if (i == 4 || i == 5) continue;
        errno = 0;
        data[i] = ptrace( PTRACE_PEEKUSER, thread->unix_tid, DR_OFFSET(i), 0 );
        if ((data[i] == -1) && errno)
        {
            file_set_error();
            goto done;
        }
    }
    switch (context->machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        context->debug.i386_regs.dr0 = data[0];
        context->debug.i386_regs.dr1 = data[1];
        context->debug.i386_regs.dr2 = data[2];
        context->debug.i386_regs.dr3 = data[3];
        context->debug.i386_regs.dr6 = data[6];
        context->debug.i386_regs.dr7 = data[7];
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        context->debug.x86_64_regs.dr0 = data[0];
        context->debug.x86_64_regs.dr1 = data[1];
        context->debug.x86_64_regs.dr2 = data[2];
        context->debug.x86_64_regs.dr3 = data[3];
        context->debug.x86_64_regs.dr6 = data[6];
        context->debug.x86_64_regs.dr7 = data[7];
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        goto done;
    }
    context->flags |= SERVER_CTX_DEBUG_REGISTERS;
done:
    resume_after_ptrace( thread );
}

/* set the thread x86 registers */
void set_thread_context( struct thread *thread, const struct context_data *context, unsigned int flags )
{
    int pid = thread->unix_tid;

    /* all other regs are handled on the client side */
    assert( flags == SERVER_CTX_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

    switch (context->machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), 0 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(0), context->debug.i386_regs.dr0 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(1), context->debug.i386_regs.dr1 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(2), context->debug.i386_regs.dr2 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(3), context->debug.i386_regs.dr3 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(6), context->debug.i386_regs.dr6 ) == -1) goto error;
        /* Linux 2.6.33+ needs enable bits set briefly to update value returned by PEEKUSER later */
        ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), context->debug.i386_regs.dr7 | 0x55 );
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), context->debug.i386_regs.dr7 ) == -1) goto error;
        thread->system_regs |= SERVER_CTX_DEBUG_REGISTERS;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), 0 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(0), context->debug.x86_64_regs.dr0 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(1), context->debug.x86_64_regs.dr1 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(2), context->debug.x86_64_regs.dr2 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(3), context->debug.x86_64_regs.dr3 ) == -1) goto error;
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(6), context->debug.x86_64_regs.dr6 ) == -1) goto error;
        ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), context->debug.x86_64_regs.dr7 | 0x55 );
        if (ptrace( PTRACE_POKEUSER, pid, DR_OFFSET(7), context->debug.x86_64_regs.dr7 ) == -1) goto error;
        thread->system_regs |= SERVER_CTX_DEBUG_REGISTERS;
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
    }
    resume_after_ptrace( thread );
    return;
 error:
    file_set_error();
    resume_after_ptrace( thread );
}

#elif defined(__i386__) && defined(PTRACE_GETDBREGS) && defined(PTRACE_SETDBREGS) && \
    (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__))

#include <machine/reg.h>

/* initialize registers in new thread if necessary */
void init_thread_context( struct thread *thread )
{
    if (!(thread->system_regs & SERVER_CTX_DEBUG_REGISTERS)) return;

    /* FreeBSD doesn't clear the debug registers in new threads */
    if (suspend_for_ptrace( thread ))
    {
        struct dbreg dbregs;

        memset( &dbregs, 0, sizeof(dbregs) );
        ptrace( PTRACE_SETDBREGS, thread->unix_tid, (caddr_t)&dbregs, 0 );
        resume_after_ptrace( thread );
    }
    thread->system_regs = 0;
}

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, struct context_data *context, unsigned int flags )
{
    struct dbreg dbregs;

    /* all other regs are handled on the client side */
    assert( flags == SERVER_CTX_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

    if (ptrace( PTRACE_GETDBREGS, thread->unix_tid, (caddr_t) &dbregs, 0 ) == -1) file_set_error();
    else
    {
#ifdef DBREG_DRX
        /* needed for FreeBSD, the structure fields have changed under 5.x */
        context->debug.i386_regs.dr0 = DBREG_DRX((&dbregs), 0);
        context->debug.i386_regs.dr1 = DBREG_DRX((&dbregs), 1);
        context->debug.i386_regs.dr2 = DBREG_DRX((&dbregs), 2);
        context->debug.i386_regs.dr3 = DBREG_DRX((&dbregs), 3);
        context->debug.i386_regs.dr6 = DBREG_DRX((&dbregs), 6);
        context->debug.i386_regs.dr7 = DBREG_DRX((&dbregs), 7);
#elif defined(__NetBSD__)
        context->debug.i386_regs.dr0 = dbregs.dr[0];
        context->debug.i386_regs.dr1 = dbregs.dr[1];
        context->debug.i386_regs.dr2 = dbregs.dr[2];
        context->debug.i386_regs.dr3 = dbregs.dr[3];
        context->debug.i386_regs.dr6 = dbregs.dr[6];
        context->debug.i386_regs.dr7 = dbregs.dr[7];
#else
        context->debug.i386_regs.dr0 = dbregs.dr0;
        context->debug.i386_regs.dr1 = dbregs.dr1;
        context->debug.i386_regs.dr2 = dbregs.dr2;
        context->debug.i386_regs.dr3 = dbregs.dr3;
        context->debug.i386_regs.dr6 = dbregs.dr6;
        context->debug.i386_regs.dr7 = dbregs.dr7;
#endif
        context->flags |= SERVER_CTX_DEBUG_REGISTERS;
    }
    resume_after_ptrace( thread );
}

/* set the thread x86 registers */
void set_thread_context( struct thread *thread, const struct context_data *context, unsigned int flags )
{
    struct dbreg dbregs;

    /* all other regs are handled on the client side */
    assert( flags == SERVER_CTX_DEBUG_REGISTERS );

    if (!suspend_for_ptrace( thread )) return;

#ifdef DBREG_DRX
    /* needed for FreeBSD, the structure fields have changed under 5.x */
    DBREG_DRX((&dbregs), 0) = context->debug.i386_regs.dr0;
    DBREG_DRX((&dbregs), 1) = context->debug.i386_regs.dr1;
    DBREG_DRX((&dbregs), 2) = context->debug.i386_regs.dr2;
    DBREG_DRX((&dbregs), 3) = context->debug.i386_regs.dr3;
    DBREG_DRX((&dbregs), 4) = 0;
    DBREG_DRX((&dbregs), 5) = 0;
    DBREG_DRX((&dbregs), 6) = context->debug.i386_regs.dr6;
    DBREG_DRX((&dbregs), 7) = context->debug.i386_regs.dr7;
#elif defined(__NetBSD__)
    dbregs.dr[0] = context->debug.i386_regs.dr0;
    dbregs.dr[1] = context->debug.i386_regs.dr1;
    dbregs.dr[2] = context->debug.i386_regs.dr2;
    dbregs.dr[3] = context->debug.i386_regs.dr3;
    dbregs.dr[4] = 0;
    dbregs.dr[5] = 0;
    dbregs.dr[6] = context->debug.i386_regs.dr6;
    dbregs.dr[7] = context->debug.i386_regs.dr7;
#else
    dbregs.dr0 = context->debug.i386_regs.dr0;
    dbregs.dr1 = context->debug.i386_regs.dr1;
    dbregs.dr2 = context->debug.i386_regs.dr2;
    dbregs.dr3 = context->debug.i386_regs.dr3;
    dbregs.dr4 = 0;
    dbregs.dr5 = 0;
    dbregs.dr6 = context->debug.i386_regs.dr6;
    dbregs.dr7 = context->debug.i386_regs.dr7;
#endif
    if (ptrace( PTRACE_SETDBREGS, thread->unix_tid, (caddr_t)&dbregs, 0 ) != -1)
    {
        thread->system_regs |= SERVER_CTX_DEBUG_REGISTERS;
    }
    else file_set_error();

    resume_after_ptrace( thread );
}

#else  /* linux || __FreeBSD__ */

/* initialize registers in new thread if necessary */
void init_thread_context( struct thread *thread )
{
}

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, struct context_data *context, unsigned int flags )
{
}

/* set the thread x86 debug registers */
void set_thread_context( struct thread *thread, const struct context_data *context, unsigned int flags )
{
}

#endif  /* linux || __FreeBSD__ */

#endif  /* USE_PTRACE */
