/*
 * System-dependent scheduler support
 *
 * Copyright 1998 Alexandre Julliard
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
#include "wine/port.h"

#include <signal.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_LWP_H
# include <sys/lwp.h>
#endif
#ifdef HAVE_UCONTEXT_H
# include <ucontext.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#include "ntstatus.h"
#include "thread.h"
#include "wine/pthread.h"
#include "wine/server.h"
#include "winbase.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 *           SYSDEPS_SetCurThread
 *
 * Make 'thread' the current thread.
 */
void SYSDEPS_SetCurThread( TEB *teb )
{
#if defined(__i386__)
    /* On the i386, the current thread is in the %fs register */
    LDT_ENTRY fs_entry;

    wine_ldt_set_base( &fs_entry, teb );
    wine_ldt_set_limit( &fs_entry, 0xfff );
    wine_ldt_set_flags( &fs_entry, WINE_LDT_FLAGS_DATA|WINE_LDT_FLAGS_32BIT );
    wine_ldt_init_fs( teb->teb_sel, &fs_entry );
#elif defined(__powerpc__)
    /* On PowerPC, the current TEB is in the gpr13 register */
# ifdef __APPLE__
    __asm__ __volatile__("mr r13, %0" : : "r" (teb));
# else
    __asm__ __volatile__("mr 2, %0" : : "r" (teb));
# endif
#elif defined(HAVE__LWP_CREATE)
    /* On non-i386 Solaris, we use the LWP private pointer */
    _lwp_setprivate( teb );
#endif
    wine_pthread_init_thread();
}


/***********************************************************************
 *           SYSDEPS_AbortThread
 *
 * Same as SYSDEPS_ExitThread, but must not do anything that requires a server call.
 */
void SYSDEPS_AbortThread( int status )
{
    SIGNAL_Block();
    close( NtCurrentTeb()->wait_fd[0] );
    close( NtCurrentTeb()->wait_fd[1] );
    close( NtCurrentTeb()->reply_fd );
    close( NtCurrentTeb()->request_fd );
    wine_pthread_abort_thread( status );
}

/***********************************************************************
 *           SYSDEPS_GetUnixTid
 *
 * Get the Unix tid of the current thread.
 */
int SYSDEPS_GetUnixTid(void)
{
#ifdef HAVE__LWP_SELF
    return _lwp_self();
#elif defined(__linux__) && defined(__i386__)
    int ret;
    __asm__("int $0x80" : "=a" (ret) : "0" (224) /* SYS_gettid */);
    if (ret < 0) ret = -1;
    return ret;
#else
    return -1;
#endif
}

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 *
 * This will crash and burn if called before threading is initialized
 */
#if defined(__i386__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( NtCurrentTeb, ".byte 0x64\n\tmovl 0x18,%eax\n\tret" );
#elif defined(__i386__) && defined(_MSC_VER)
/* Nothing needs to be done. MS C "magically" exports the inline version from winnt.h */
#elif defined(HAVE__LWP_CREATE)
/***********************************************************************
 *		NtCurrentTeb (NTDLL.@)
 */
struct _TEB * WINAPI NtCurrentTeb(void)
{
    extern void *_lwp_getprivate(void);
    return (struct _TEB *)_lwp_getprivate();
}
#elif defined(__powerpc__)
# ifdef __APPLE__
__ASM_GLOBAL_FUNC( NtCurrentTeb, "\n\tmr r3,r13\n\tblr" );
# else
__ASM_GLOBAL_FUNC( NtCurrentTeb, "\n\tmr 3,2\n\tblr" );
# endif
#else
# error NtCurrentTeb not defined for this architecture
#endif  /* __i386__ */
