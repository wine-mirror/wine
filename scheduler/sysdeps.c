/*
 * System-dependent scheduler support
 *
 * Copyright 1998 Alexandre Julliard
 */

#include "config.h"

/* Get pointers to the static errno and h_errno variables used by Xlib. This
   must be done before including <errno.h> makes the variables invisible.  */
extern int errno;
static int *perrno = &errno;
extern int h_errno;
static int *ph_errno = &h_errno;

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_LWP_H
# include <sys/lwp.h>
#endif
#ifdef HAVE_UCONTEXT_H
# include <ucontext.h>
#endif
#include "thread.h"
#include "server.h"
#include "winbase.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(thread)

#ifdef linux
#define HAVE_CLONE_SYSCALL
#endif

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


#ifndef NO_REENTRANT_LIBC

/***********************************************************************
 *           __errno_location/__error/___errno
 *
 * Get the per-thread errno location.
 */
#ifdef HAVE__ERRNO_LOCATION
int *__errno_location()
#endif
#ifdef HAVE__ERROR
int *__error()
#endif
#ifdef HAVE___ERRNO
int *___errno()
#endif
{
    THDB *thdb = THREAD_Current();
    if (!thdb) return perrno;
#ifdef NO_REENTRANT_X11
    /* Use static libc errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == (HANDLE)thdb->server_tid)
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
    if (X11DRV_CritSection.OwningThread == (HANDLE)thdb->server_tid)
        return ph_errno;
#endif
    return &thdb->thread_h_errno;
}

#endif /* NO_REENTRANT_LIBC */

/***********************************************************************
 *           SYSDEPS_StartThread
 *
 * Startup routine for a new thread.
 */
static void SYSDEPS_StartThread( THDB *thdb )
{
    SET_CUR_THREAD( thdb );
    CLIENT_InitThread();
    thdb->startup();
    _exit(0);  /* should never get here */
}


/***********************************************************************
 *           SYSDEPS_SpawnThread
 *
 * Start running a new thread.
 * Return -1 on error, 0 if OK.
 */
int SYSDEPS_SpawnThread( THDB *thread )
{
#ifndef NO_REENTRANT_LIBC

#ifdef HAVE_CLONE_SYSCALL
    if (clone( (int (*)(void *))SYSDEPS_StartThread, thread->teb.stack_top,
               CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, thread ) < 0)
        return -1;
    /* FIXME: close the child socket in the parent process */
/*    close( thread->socket );*/
    return 0;
#endif

#ifdef HAVE_RFORK
    DWORD *sp = (DWORD *)thread->teb.stack_top;
    *--sp = (DWORD)thread;
    *--sp = 0;
    *--sp = (DWORD)SYSDEPS_StartThread;
    __asm__(
    "pushl %2;\n\t"		/* RFPROC|RMEM */
    "pushl $0;\n\t"		/* 0 ? */
    "movl %1,%%eax;\n\t"	/* SYS_rfork */
    ".byte 0x9a; .long 0; .word 7;\n\t"	/* lcall 7:0... FreeBSD syscall */
    "cmpl $0, %%edx;\n\t"
    "je 1f;\n\t"
    "movl %0,%%esp;\n\t"	/* father -> new thread */
    "ret;\n"
    "1:\n\t"		/* child -> caller thread */
    "addl $8,%%esp" :
    : "r" (sp), "g" (SYS_rfork), "g" (RFPROC|RFMEM)
    : "eax", "edx");
    return 0;
#endif

#ifdef HAVE__LWP_CREATE
    ucontext_t context;
    _lwp_makecontext( &context, (void(*)(void *))SYSDEPS_StartThread, thread,
                      NULL, thread->stack_base, thread->teb.stack_top - thread->stack_base );
    if ( _lwp_create( &context, 0, NULL ) )
        return -1;
    return 0;
#endif

#endif /* NO_REENTRANT_LIBC */

    FIXME(thread, "CreateThread: stub\n" );
    return 0;
}



/***********************************************************************
 *           SYSDEPS_ExitThread
 *
 * Exit a running thread; must not return.
 * Must not make any reference to the thread structures (THDB etc.) as
 * they have already been deleted.
 */
void SYSDEPS_ExitThread(void)
{
#ifdef HAVE__LWP_CREATE
    _lwp_exit();
#endif

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
