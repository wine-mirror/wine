/*
 * System-dependent scheduler support
 *
 * Copyright 1998 Alexandre Julliard
 */

/* Get pointers to the static errno and h_errno variables used by Xlib. This
   must be done before including <errno.h> makes the variables invisible.  */
extern int errno;
static int *perrno = &errno;
extern int h_errno;
static int *ph_errno = &h_errno;

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "thread.h"
#include "server.h"
#include "winbase.h"
#include "debug.h"

/* Xlib critical section (FIXME: does not belong here) */
CRITICAL_SECTION X11DRV_CritSection = { 0, };

#ifdef HAVE_CLONE_SYSCALL
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
#endif  /* HAVE_CLONE_SYSCALL */


#ifdef USE_THREADS
/***********************************************************************
 *           __errno_location
 *
 * Get the per-thread errno location.
 */
int *__errno_location()
{
    THDB *thdb = THREAD_Current();
    if (!thdb) return perrno;
#ifdef NO_REENTRANT_X11
    /* Use static libc errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == thdb->server_tid)
        return perrno;
#endif
    return &thdb->thread_errno;
}

/***********************************************************************
 *           __h_errno_location
 *
 * Get the per-thread h_errno location.
 */
int *__h_errno_location()
{
    THDB *thdb = THREAD_Current();
    if (!thdb) return ph_errno;
#ifdef NO_REENTRANT_X11
    /* Use static libc h_errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == thdb->server_tid)
        return ph_errno;
#endif
    return &thdb->thread_h_errno;
}

/***********************************************************************
 *           SYSDEPS_StartThread
 *
 * Startup routine for a new thread.
 */
static void SYSDEPS_StartThread( THDB *thdb )
{
    SET_FS( thdb->teb_sel );
    THREAD_Start( thdb );
}
#endif  /* USE_THREADS */


/***********************************************************************
 *           SYSDEPS_SpawnThread
 *
 * Start running a new thread.
 * Return -1 on error, 0 if OK.
 */
int SYSDEPS_SpawnThread( THDB *thread )
{
#ifdef USE_THREADS

#ifdef HAVE_CLONE_SYSCALL
    if (clone( (int (*)(void *))SYSDEPS_StartThread, thread->teb.stack_top,
               CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, thread ) < 0)
        return -1;
    /* FIXME: close the child socket in the parent process */
/*    close( thread->socket );*/
#endif

#ifdef HAVE_RFORK
    FIXME(thread, "Threads using rfork() not implemented\n" );
#endif

#else  /* !USE_THREADS */
    FIXME(thread, "CreateThread: stub\n" );
#endif  /* USE_THREADS */
    return 0;
}


/***********************************************************************
 *           SYSDEPS_ExitThread
 *
 * Exit a running thread; must not return.
 */
void SYSDEPS_ExitThread(void)
{
    THDB *thdb = THREAD_Current();
    close( thdb->socket );
    _exit( 0 );
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.89)
 *
 * This will crash and burn if called before threading is initialized
 */
TEB * WINAPI NtCurrentTeb(void)
{
#ifdef __i386__
    TEB *teb;

    /* Get the TEB self-pointer */
    __asm__( ".byte 0x64\n\tmovl (%1),%0"
             : "=r" (teb) : "r" (&((TEB *)0)->self) );
    return teb;
#else
    return &pCurrentThread->teb;
#endif  /* __i386__ */
}
