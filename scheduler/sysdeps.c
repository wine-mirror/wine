/*
 * System-dependent scheduler support
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "thread.h"

#ifdef __linux__
# ifdef HAVE_SCHED_H
#  include <sched.h>
# endif
# ifndef CLONE_VM
#  define CLONE_VM      0x00000100
#  define CLONE_FS      0x00000200
#  define CLONE_FILES   0x00000400
#  define CLONE_SIGHAND 0x00000800
#  define CLONE_PID     0x00001000
/* If we didn't get the flags, we probably didn't get the prototype either */
extern int clone( int (*fn)(void *arg), void *stack, int flags, void *arg );
# endif  /* CLONE_VM */
#endif  /* __linux__ */


#ifdef __linux__
/***********************************************************************
 *           __errno_location
 *
 * Get the per-thread errno location.
 */
int *__errno_location()
{
    static int static_errno;
    THDB *thdb = THREAD_Current();
    if (!thdb) return &static_errno;
    return &thdb->thread_errno;
}

/***********************************************************************
 *           __h_errno_location
 *
 * Get the per-thread h_errno location.
 */
int *__h_errno_location()
{
    static int static_h_errno;
    THDB *thdb = THREAD_Current();
    if (!thdb) return &static_h_errno;
    return &thdb->thread_h_errno;
}

/***********************************************************************
 *           SYSDEPS_StartThread
 *
 * Startup routine for a new thread.
 */
static void SYSDEPS_StartThread( THDB *thdb )
{
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)thdb->entry_point;
    thdb->unix_pid = getpid();
    SET_FS( thdb->teb_sel );
    ExitThread( func( thdb->entry_arg ) );
}
#endif  /* __linux__ */


/***********************************************************************
 *           SYSDEPS_SpawnThread
 *
 * Start running a new thread.
 * Return -1 on error, 0 if OK.
 */
int SYSDEPS_SpawnThread( THDB *thread )
{
#ifdef __linux__
    if (clone( (int (*)(void *))SYSDEPS_StartThread, thread->teb.stack_top,
               CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, thread ) < 0)
        return -1;
#else
    fprintf( stderr, "CreateThread: stub\n" );
#endif  /* __linux__ */
    return 0;
}


/***********************************************************************
 *           SYSDEPS_ExitThread
 *
 * Exit a running thread; must not return.
 */
void SYSDEPS_ExitThread(void)
{
    _exit( 0 );
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.89)
 */
TEB * WINAPI NtCurrentTeb(void)
{
#ifdef __i386__
    TEB *teb;
    WORD ds, fs;

    /* Check if we have a current thread */
    GET_DS( ds );
    GET_FS( fs );
    if (fs == ds) return NULL; /* FIXME: should be an assert */
    /* Get the TEB self-pointer */
    __asm__( ".byte 0x64\n\tmovl (%1),%0"
             : "=r" (teb) : "r" (&((TEB *)0)->self) );
    return teb;
#else
    if (!pCurrentThread) return NULL;
    return &pCurrentThread->teb;
#endif  /* __i386__ */
}
