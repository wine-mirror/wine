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

struct thread_cleanup_info
{
    void *stack_base;
    int   stack_size;
    int   status;
};

/* temporary stacks used on thread exit */
#define TEMP_STACK_SIZE 1024
#define NB_TEMP_STACKS  8
static char temp_stacks[NB_TEMP_STACKS][TEMP_STACK_SIZE];
static LONG next_temp_stack;  /* next temp stack to use */


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

#ifdef HAVE_NPTL
    teb->pthread_data = (void *)pthread_self();
#else
    wine_pthread_init_thread();
#endif
}


/***********************************************************************
 *           get_temp_stack
 *
 * Get a temporary stack address to run the thread exit code on.
 */
inline static char *get_temp_stack(void)
{
    unsigned int next = interlocked_xchg_add( &next_temp_stack, 1 );
    return temp_stacks[next % NB_TEMP_STACKS] + TEMP_STACK_SIZE;
}


/***********************************************************************
 *           cleanup_thread
 *
 * Cleanup the remains of a thread. Runs on a temporary stack.
 */
static void cleanup_thread( void *ptr )
{
    /* copy the info structure since it is on the stack we will free */
    struct thread_cleanup_info info = *(struct thread_cleanup_info *)ptr;
    munmap( info.stack_base, info.stack_size );
    wine_ldt_free_fs( wine_get_fs() );
#ifdef HAVE__LWP_CREATE
    _lwp_exit();
#endif
    _exit( info.status );
}


/***********************************************************************
 *           SYSDEPS_SpawnThread
 *
 * Start running a new thread.
 * Return -1 on error, 0 if OK.
 */
int SYSDEPS_SpawnThread( void (*func)(TEB *), TEB *teb )
{
#ifdef HAVE_NPTL
    pthread_t id;
    pthread_attr_t attr;

    pthread_attr_init( &attr );
    pthread_attr_setstack( &attr, teb->DeallocationStack,
                           (char *)teb->Tib.StackBase - (char *)teb->DeallocationStack );
    if (pthread_create( &id, &attr, (void * (*)(void *))func, teb )) return -1;
    return 0;
#elif defined(HAVE_CLONE)
    if (clone( (int (*)(void *))func, teb->Tib.StackBase,
               CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, teb ) < 0)
        return -1;
    return 0;
#elif defined(HAVE_RFORK)
    void **sp = (void **)teb->Tib.StackBase;
    *--sp = teb;
    *--sp = 0;
    *--sp = func;
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
    : "r" (sp), "g" (SYS_rfork), "g" (RFPROC | RFMEM)
    : "eax", "edx");
    return 0;
#elif defined(HAVE__LWP_CREATE)
    ucontext_t context;
    _lwp_makecontext( &context, (void(*)(void *))func, teb,
                      NULL, teb->DeallocationStack, (char *)teb->Tib.StackBase - (char *)teb->DeallocationStack );
    if ( _lwp_create( &context, 0, NULL ) )
        return -1;
    return 0;
#endif

    FIXME("CreateThread: stub\n" );
    return -1;
}


/***********************************************************************
 *           SYSDEPS_ExitThread
 *
 * Exit a running thread; must not return.
 */
void SYSDEPS_ExitThread( int status )
{
    TEB *teb = NtCurrentTeb();
    DWORD size = 0;

#ifdef HAVE_NPTL
    static TEB *teb_to_free;
    TEB *free_teb;

    if ((free_teb = interlocked_xchg_ptr( (void **)&teb_to_free, teb )) != NULL)
    {
        void *ptr;

        TRACE("freeing prev teb %p stack %p fs %04x\n",
              free_teb, free_teb->DeallocationStack, free_teb->teb_sel );

        pthread_join( (pthread_t)free_teb->pthread_data, &ptr );
        wine_ldt_free_fs( free_teb->teb_sel );
        ptr = free_teb->DeallocationStack;
        NtFreeVirtualMemory( GetCurrentProcess(), &ptr, &size, MEM_RELEASE );
    }
    SYSDEPS_AbortThread( status );
#else
    struct thread_cleanup_info info;
    MEMORY_BASIC_INFORMATION meminfo;

    NtQueryVirtualMemory( GetCurrentProcess(), teb->Tib.StackBase, MemoryBasicInformation,
                          &meminfo, sizeof(meminfo), NULL );
    info.stack_base = meminfo.AllocationBase;
    info.stack_size = meminfo.RegionSize + ((char *)teb->Tib.StackBase - (char *)meminfo.AllocationBase);
    info.status     = status;

    SIGNAL_Block();
    size = 0;
    NtFreeVirtualMemory( GetCurrentProcess(), &teb->DeallocationStack, &size, MEM_RELEASE | MEM_SYSTEM );
    close( teb->wait_fd[0] );
    close( teb->wait_fd[1] );
    close( teb->reply_fd );
    close( teb->request_fd );
    SIGNAL_Reset();
    wine_switch_to_stack( cleanup_thread, &info, get_temp_stack() );
#endif
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
#ifdef HAVE_NPTL
    pthread_exit( (void *)status );
#endif
    SIGNAL_Reset();
#ifdef HAVE__LWP_CREATE
    _lwp_exit();
#endif
    for (;;)  /* avoid warning */
        _exit( status );
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
