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


/* wait for a ptraced child to get a certain signal */
/* if the signal is 0, we simply check if anything is pending and return at once */
void wait4_thread( struct thread *thread, int signal )
{
    int status;
    int pid;

 restart:
    pid = thread ? thread->unix_pid : -1;
    if ((pid = wait4( pid, &status, WUNTRACED | (signal ? 0 : WNOHANG), NULL )) == -1)
    {
        perror( "wait4" );
        return;
    }
    if (WIFSTOPPED(status))
    {
        int sig = WSTOPSIG(status);
        if (debug_level) fprintf( stderr, "ptrace: pid %d got sig %d\n", pid, sig );
        switch(sig)
        {
        case SIGSTOP:  /* continue at once if not suspended */
            if (!thread)
                if (!(thread = get_thread_from_pid( pid ))) break;
            if (!(thread->process->suspend + thread->suspend))
                ptrace( PTRACE_CONT, pid, 1, sig );
            break;
        default:  /* ignore other signals for now */
            ptrace( PTRACE_CONT, pid, 1, sig );
            break;
        }
        if (signal && sig != signal) goto restart;
    }
    else if (WIFSIGNALED(status))
    {
        int exit_code = WTERMSIG(status);
        if (debug_level)
            fprintf( stderr, "ptrace: pid %d killed by sig %d\n", pid, exit_code );
        if (!thread)
            if (!(thread = get_thread_from_pid( pid ))) return;
        if (thread->client) remove_client( thread->client, exit_code );
    }
    else if (WIFEXITED(status))
    {
        int exit_code = WEXITSTATUS(status);
        if (debug_level)
            fprintf( stderr, "ptrace: pid %d exited with status %d\n", pid, exit_code );
        if (!thread)
            if (!(thread = get_thread_from_pid( pid ))) return;
        if (thread->client) remove_client( thread->client, exit_code );
    }
    else fprintf( stderr, "wait4: pid %d unknown status %x\n", pid, status );
}

/* attach to a Unix thread */
static int attach_thread( struct thread *thread )
{
    /* this may fail if the client is already being debugged */
    if (!use_ptrace || (ptrace( PTRACE_ATTACH, thread->unix_pid, 0, 0 ) == -1)) return 0;
    if (debug_level) fprintf( stderr, "ptrace: attached to pid %d\n", thread->unix_pid );
    thread->attached = 1;
    wait4_thread( thread, SIGSTOP );
    return 1;
}

/* detach from a Unix thread and kill it */
void detach_thread( struct thread *thread )
{
    if (!thread->unix_pid) return;
    kill( thread->unix_pid, SIGTERM );
    if (thread->suspend + thread->process->suspend) continue_thread( thread );
    if (thread->attached)
    {
        wait4_thread( thread, SIGTERM );
        if (debug_level) fprintf( stderr, "ptrace: detaching from %d\n", thread->unix_pid );
        ptrace( PTRACE_DETACH, thread->unix_pid, 1, SIGTERM );
        thread->attached = 0;
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
