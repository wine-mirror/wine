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
#include <sys/time.h>
#include <sys/resource.h>
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_LWP_H
# include <sys/lwp.h>
#endif
#ifdef HAVE_UCONTEXT_H
# include <ucontext.h>
#endif
#include "wine/port.h"
#include "thread.h"
#include "selectors.h"
#include "server.h"
#include "winbase.h"
#include "global.h"
#include "wine/exception.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(thread)

/* Xlib critical section (FIXME: does not belong here) */
CRITICAL_SECTION X11DRV_CritSection = CRITICAL_SECTION_INIT;

#ifdef linux
# ifdef HAVE_SCHED_H
#  include <sched.h>
# endif
# ifndef CLONE_VM
#  define CLONE_VM      0x00000100
#  define CLONE_FS      0x00000200
#  define CLONE_FILES   0x00000400
#  define CLONE_SIGHAND 0x00000800
#  define CLONE_PID     0x00001000
# endif  /* CLONE_VM */
#endif  /* linux */

static int init_done;

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
#ifdef HAVE__THR_ERRNO
int *__thr_errno()
#endif
{
    if (!init_done) return perrno;
#ifdef NO_REENTRANT_X11
    /* Use static libc errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == GetCurrentThreadId())
        return perrno;
#endif
    return &NtCurrentTeb()->thread_errno;
}

/***********************************************************************
 *           __h_errno_location
 *
 * Get the per-thread h_errno location.
 */
int *__h_errno_location()
{
    if (!init_done) return ph_errno;
#ifdef NO_REENTRANT_X11
    /* Use static libc h_errno while running in Xlib. */
    if (X11DRV_CritSection.OwningThread == GetCurrentThreadId())
        return ph_errno;
#endif
    return &NtCurrentTeb()->thread_h_errno;
}

#endif /* NO_REENTRANT_LIBC */

/***********************************************************************
 *           SYSDEPS_SetCurThread
 *
 * Make 'thread' the current thread.
 */
void SYSDEPS_SetCurThread( TEB *teb )
{
#if defined(__i386__)
    /* On the i386, the current thread is in the %fs register */
    __set_fs( teb->teb_sel );
#elif defined(HAVE__LWP_CREATE)
    /* On non-i386 Solaris, we use the LWP private pointer */
    _lwp_setprivate( teb );
#endif

    init_done = 1;  /* now we can use threading routines */
}

/***********************************************************************
 *           SYSDEPS_StartThread
 *
 * Startup routine for a new thread.
 */
static void SYSDEPS_StartThread( TEB *teb )
{
    SYSDEPS_SetCurThread( teb );
    CLIENT_InitThread();
    SIGNAL_Init();
    __TRY
    {
        teb->startup();
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    SYSDEPS_ExitThread(0);  /* should never get here */
}


/***********************************************************************
 *           SYSDEPS_SpawnThread
 *
 * Start running a new thread.
 * Return -1 on error, 0 if OK.
 */
int SYSDEPS_SpawnThread( TEB *teb )
{
#ifndef NO_REENTRANT_LIBC

#ifdef linux
    const int flags = CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD;
    if (clone( (int (*)(void *))SYSDEPS_StartThread, teb->stack_top, flags, teb ) < 0)
        return -1;
    if (!(flags & CLONE_FILES)) close( teb->socket );  /* close the child socket in the parent */
    return 0;
#endif

#ifdef HAVE_RFORK
    const int flags = RFPROC | RFMEM; /*|RFFDG*/
    void **sp = (void **)teb->stack_top;
    *--sp = teb;
    *--sp = 0;
    *--sp = SYSDEPS_StartThread;
    __asm__ __volatile__(
    "pushl %2;\n\t"		/* flags */
    "pushl $0;\n\t"		/* 0 ? */
    "movl %1,%%eax;\n\t"	/* SYS_rfork */
    ".byte 0x9a; .long 0; .word 7;\n\t"	/* lcall 7:0... FreeBSD syscall */
    "cmpl $0, %%edx;\n\t"
    "je 1f;\n\t"
    "movl %0,%%esp;\n\t"	/* child -> new thread */
    "ret;\n"
    "1:\n\t"		/* parent -> caller thread */
    "addl $8,%%esp" :
    : "r" (sp), "g" (SYS_rfork), "g" (flags)
    : "eax", "edx");
    if (flags & RFFDG) close( teb->socket );  /* close the child socket in the parent */
    return 0;
#endif

#ifdef HAVE__LWP_CREATE
    ucontext_t context;
    _lwp_makecontext( &context, (void(*)(void *))SYSDEPS_StartThread, teb,
                      NULL, teb->stack_base, (char *)teb->stack_top - (char *)teb->stack_base );
    if ( _lwp_create( &context, 0, NULL ) )
        return -1;
    return 0;
#endif

#endif /* NO_REENTRANT_LIBC */

    FIXME("CreateThread: stub\n" );
    return 0;
}



/***********************************************************************
 *           SYSDEPS_ExitThread
 *
 * Exit a running thread; must not return.
 */
void SYSDEPS_ExitThread( int status )
{
    int socket = NtCurrentTeb()->socket;
    NtCurrentTeb()->socket = -1;
    close( socket );
#ifdef HAVE__LWP_CREATE
    _lwp_exit();
#endif
    _exit( status );
    /*
     * It is of course impossible to come here,
     * but it eliminates a compiler warning.
     */
    exit( status );
}


/***********************************************************************
 *           SYSDEPS_CallOnStack
 */
int SYSDEPS_DoCallOnStack( int (*func)(LPVOID), LPVOID arg )
{
    int retv = 0;

    __TRY
    {
        retv = func( arg );
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
        return 0;
    }
    __ENDTRY

    return retv;
}

#ifdef __i386__
int SYSDEPS_CallOnStack( LPVOID stackTop, LPVOID stackLow,
                         int (*func)(LPVOID), LPVOID arg );
__ASM_GLOBAL_FUNC( SYSDEPS_CallOnStack,
                   "pushl %ebp\n\t"
                   "movl %esp, %ebp\n\t"
                   ".byte 0x64; pushl 0x04\n\t"
                   ".byte 0x64; pushl 0x08\n\t"
                   "movl 8(%ebp), %esp\n\t"
                   "movl 12(%ebp), %eax\n\t"
                   ".byte 0x64; movl %esp, 0x04\n\t"
                   ".byte 0x64; movl %eax, 0x08\n\t"
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "call " __ASM_NAME("SYSDEPS_DoCallOnStack") "\n\t"
                   "leal -8(%ebp), %esp\n\t"
                   ".byte 0x64; popl 0x08\n\t"
                   ".byte 0x64; popl 0x04\n\t"
                   "popl %ebp\n\t"
                   "ret" );
#else
int SYSDEPS_CallOnStack( LPVOID stackTop, LPVOID stackLow,
                         int (*func)(LPVOID), LPVOID arg )
{
    return SYSDEPS_DoCallOnStack( func, arg );
}
#endif

/***********************************************************************
 *           SYSDEPS_SwitchToThreadStack
 */

static LPVOID SYSDEPS_LargeStackTop = NULL;
static LPVOID SYSDEPS_LargeStackLow = NULL;

void SYSDEPS_SwitchToThreadStack( void (*func)(void) )
{
    DWORD page_size = VIRTUAL_GetPageSize();
    DWORD cur_stack = (((DWORD)&func) + (page_size-1)) & ~(page_size-1);

    TEB *teb = NtCurrentTeb();
    LPVOID stackTop = teb->stack_top;
    LPVOID stackLow = teb->stack_low;

    struct rlimit rl;

    if ( getrlimit(RLIMIT_STACK, &rl) < 0 ) 
    {
        WARN("Can't get rlimit\n");
        rl.rlim_cur = 8*1024*1024;
    }

    teb->stack_top = (LPVOID) cur_stack;
    teb->stack_low = (LPVOID)(cur_stack - rl.rlim_cur);

    SYSDEPS_LargeStackTop = (LPVOID)(cur_stack - 2*page_size);
    SYSDEPS_LargeStackLow = (LPVOID)(cur_stack - rl.rlim_cur);

    SYSDEPS_CallOnStack( stackTop, stackLow, 
                         (int (*)(void *))func, NULL );
}

/***********************************************************************
 *           SYSDEPS_CallOnLargeStack
 */
int SYSDEPS_CallOnLargeStack( int (*func)(LPVOID), LPVOID arg )
{
    static int recurse = 0;
    int retv;

    if ( recurse++ == 0 && SYSDEPS_LargeStackTop )
        retv = SYSDEPS_CallOnStack( SYSDEPS_LargeStackTop,
                                    SYSDEPS_LargeStackLow, func, arg );
    else
        retv = func( arg );

    recurse--;
    return retv;
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.89)
 *
 * This will crash and burn if called before threading is initialized
 */
#ifdef __i386__
__ASM_GLOBAL_FUNC( NtCurrentTeb, ".byte 0x64\n\tmovl 0x18,%eax\n\tret" );
#elif defined(HAVE__LWP_CREATE)
struct _TEB * WINAPI NtCurrentTeb(void)
{
    extern void *_lwp_getprivate(void);
    return (struct _TEB *)_lwp_getprivate();
}
#else
# error NtCurrentTeb not defined for this architecture
#endif  /* __i386__ */
