/*
 * Server-side ptrace support
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <unistd.h>

#include "process.h"
#include "thread.h"


#ifndef PTRACE_CONT
#define PTRACE_CONT PT_CONTINUE
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

static const int use_ptrace = 1;  /* set to 0 to disable ptrace */

/* handle a status returned by wait4 */
static int handle_child_status( struct thread *thread, int pid, int status )
{
    if (WIFSTOPPED(status))
    {
        int sig = WSTOPSIG(status);
        if (debug_level && thread)
            fprintf( stderr, "%08x: *signal* signal=%d\n", (unsigned int)thread, sig );
        switch(sig)
        {
        case SIGSTOP:  /* continue at once if not suspended */
            if (!thread || !(thread->process->suspend + thread->suspend))
                ptrace( PTRACE_CONT, pid, 1, sig );
            break;
        default:  /* ignore other signals for now */
            ptrace( PTRACE_CONT, pid, 1, sig );
            break;
        }
        return sig;
    }
    if (thread && (WIFSIGNALED(status) || WIFEXITED(status)))
    {
        thread->attached = 0;
        thread->unix_pid = 0;
        if (debug_level)
        {
            if (WIFSIGNALED(status))
                fprintf( stderr, "%08x: *exited* signal=%d\n",
                         (unsigned int)thread, WTERMSIG(status) );
            else
                fprintf( stderr, "%08x: *exited* status=%d\n",
                         (unsigned int)thread, WEXITSTATUS(status) );
        }
    }
    return 0;
}

/* handle a SIGCHLD signal */
void sigchld_handler()
{
    int pid, status;

    for (;;)
    {
        if (!(pid = wait4( -1, &status, WUNTRACED | WNOHANG, NULL ))) break;
        if (pid != -1) handle_child_status( get_thread_from_pid(pid), pid, status );
        else break;
    }
}

/* wait for a ptraced child to get a certain signal */
void wait4_thread( struct thread *thread, int signal )
{
    int res, status;

    do
    {
        if ((res = wait4( thread->unix_pid, &status, WUNTRACED, NULL )) == -1)
        {
            perror( "wait4" );
            return;
        }
        res = handle_child_status( thread, res, status );
    } while (res && res != signal);
}

/* attach to a Unix thread */
static int attach_thread( struct thread *thread )
{
    /* this may fail if the client is already being debugged */
    if (!use_ptrace || (ptrace( PTRACE_ATTACH, thread->unix_pid, 0, 0 ) == -1)) return 0;
    if (debug_level) fprintf( stderr, "%08x: *attached*\n", (unsigned int)thread );
    thread->attached = 1;
    wait4_thread( thread, SIGSTOP );
    return 1;
}

/* detach from a Unix thread and kill it */
void detach_thread( struct thread *thread, int sig )
{
    if (!thread->unix_pid) return;
    if (thread->attached)
    {
        /* make sure it is stopped */
        if (!(thread->suspend + thread->process->suspend)) stop_thread( thread );
        if (sig) kill( thread->unix_pid, sig );
        if (debug_level) fprintf( stderr, "%08x: *detached*\n", (unsigned int)thread );
        ptrace( PTRACE_DETACH, thread->unix_pid, 1, sig );
        thread->attached = 0;
    }
    else
    {
        if (sig) kill( thread->unix_pid, sig );
        if (thread->suspend + thread->process->suspend) continue_thread( thread );
    }
}

/* stop a thread (at the Unix level) */
void stop_thread( struct thread *thread )
{
    /* can't stop a thread while initialisation is in progress */
    if (!thread->unix_pid || thread->process->init_event) return;
    /* first try to attach to it */
    if (!thread->attached)
        if (attach_thread( thread )) return;  /* this will have stopped it */
    /* attached already, or attach failed -> send a signal */
    kill( thread->unix_pid, SIGSTOP );
    if (thread->attached) wait4_thread( thread, SIGSTOP );
}

/* make a thread continue (at the Unix level) */
void continue_thread( struct thread *thread )
{
    if (!thread->unix_pid) return;
    if (!thread->attached) kill( thread->unix_pid, SIGCONT );
    else ptrace( PTRACE_CONT, thread->unix_pid, 1, SIGSTOP );
}

/* suspend a thread to allow using ptrace on it */
/* you must do a resume_thread when finished with the thread */
int suspend_for_ptrace( struct thread *thread )
{
    if (thread->attached)
    {
        suspend_thread( thread, 0 );
        return 1;
    }
    if (attach_thread( thread ))
    {
        /* the attach will have suspended it */
        thread->suspend++;
        return 1;
    }
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}

/* read an int from a thread address space */
int read_thread_int( struct thread *thread, const int *addr, int *data )
{
    if (((*data = ptrace( PTRACE_PEEKDATA, thread->unix_pid, addr, 0 )) == -1) && errno)
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
    if ((res = ptrace( PTRACE_POKEDATA, thread->unix_pid, addr, data )) == -1) file_set_error();
    return res;
}
