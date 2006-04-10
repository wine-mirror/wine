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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "process.h"
#include "thread.h"

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

#ifndef HAVE_SYS_PTRACE_H
#define PT_CONTINUE 0
#define PT_ATTACH   1
#define PT_DETACH   2
#define PT_READ_D   3
#define PT_WRITE_D  4
#define PT_STEP     5
inline static int ptrace(int req, ...) { errno = EPERM; return -1; /*FAIL*/ }
#endif  /* HAVE_SYS_PTRACE_H */

static const int use_ptrace = 1;  /* set to 0 to disable ptrace */

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
        if (pid != -1) handle_child_status( get_thread_from_pid(pid), pid, status, -1 );
        else break;
    }
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

/* return the Unix pid to use in ptrace calls for a given thread */
int get_ptrace_pid( struct thread *thread )
{
    if (thread->unix_tid != -1) return thread->unix_tid;
    return thread->unix_pid;
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
    return (ret != -1);
}

/* suspend a thread to allow using ptrace on it */
/* you must do a resume_after_ptrace when finished with the thread */
int suspend_for_ptrace( struct thread *thread )
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

/* resume a thread after we have used ptrace on it */
void resume_after_ptrace( struct thread *thread )
{
    if (thread->unix_pid == -1) return;
    if (ptrace( PTRACE_DETACH, get_ptrace_pid(thread), (caddr_t)1, 0 ) == -1)
    {
        if (errno == ESRCH) thread->unix_pid = thread->unix_tid = -1;  /* thread got killed */
    }
}

/* read an int from a thread address space */
static int read_thread_int( struct thread *thread, const int *addr, int *data )
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
    if (mask != ~0)
    {
        if (read_thread_int( thread, addr, &res ) == -1) return -1;
        data = (data & mask) | (res & ~mask);
    }
    if ((res = ptrace( PTRACE_POKEDATA, get_ptrace_pid(thread), (caddr_t)addr, data )) == -1)
        file_set_error();
    return res;
}

/* read data from a process memory space */
int read_process_memory( struct process *process, const void *ptr, size_t size, char *dest )
{
    struct thread *thread = get_process_first_thread( process );
    unsigned int first_offset, last_offset, len;
    int data, *addr;

    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

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
static int check_process_write_access( struct thread *thread, int *addr, size_t len )
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
int write_process_memory( struct process *process, void *ptr, size_t size, const char *src )
{
    struct thread *thread = get_process_first_thread( process );
    int ret = 0, data = 0;
    size_t len;
    int *addr;
    unsigned int first_mask, first_offset, last_mask, last_offset;

    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

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
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
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
