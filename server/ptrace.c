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
            if (thread && get_thread_single_step( thread ))
                ptrace( PTRACE_SINGLESTEP, pid, (caddr_t)1, sig );
            else
                ptrace( PTRACE_CONT, pid, (caddr_t)1, sig );
        }
        return sig;
    }
    if (thread && (WIFSIGNALED(status) || WIFEXITED(status)))
    {
        thread->attached = 0;
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
                thread->attached = 0;
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
            thread->attached = 0;
        }
    }
    return (ret != -1);
}

/* attach to a Unix process with ptrace */
int attach_process( struct process *process )
{
    struct thread *thread;
    int ret = 1;

    if (list_empty( &process->thread_list ))  /* need at least one running thread */
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread->attached) continue;
        if (suspend_for_ptrace( thread )) resume_after_ptrace( thread );
        else ret = 0;
    }
    return ret;
}

/* detach from a ptraced Unix process */
void detach_process( struct process *process )
{
    struct thread *thread;

    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread->attached)
        {
            /* make sure it is stopped */
            suspend_for_ptrace( thread );
            if (thread->unix_pid == -1) return;
            if (debug_level) fprintf( stderr, "%04x: *detached*\n", thread->id );
            ptrace( PTRACE_DETACH, get_ptrace_pid(thread), (caddr_t)1, 0 );
            thread->attached = 0;
        }
    }
}

/* suspend a thread to allow using ptrace on it */
/* you must do a resume_after_ptrace when finished with the thread */
int suspend_for_ptrace( struct thread *thread )
{
    /* can't stop a thread while initialisation is in progress */
    if (thread->unix_pid == -1 || !is_process_init_done(thread->process)) goto error;

    if (thread->attached)
    {
        if (!send_thread_signal( thread, SIGSTOP )) goto error;
    }
    else
    {
        /* this may fail if the client is already being debugged */
        if (!use_ptrace) goto error;
        if (ptrace( PTRACE_ATTACH, get_ptrace_pid(thread), 0, 0 ) == -1)
        {
            if (errno == ESRCH) thread->unix_pid = thread->unix_tid = -1;  /* thread got killed */
            goto error;
        }
        if (debug_level) fprintf( stderr, "%04x: *attached*\n", thread->id );
        thread->attached = 1;
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
    assert( thread->attached );
    ptrace( get_thread_single_step(thread) ? PTRACE_SINGLESTEP : PTRACE_CONT,
            get_ptrace_pid(thread), (caddr_t)1, 0 /* cancel the SIGSTOP */ );
}

/* read an int from a thread address space */
int read_thread_int( struct thread *thread, const int *addr, int *data )
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
int write_thread_int( struct thread *thread, int *addr, int data, unsigned int mask )
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
