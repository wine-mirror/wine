/*
 * Wine threading routines using the pthread library
 *
 * Copyright 2003 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_PTHREAD_H

#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_MACH_MACH_H
#include <mach/mach.h>
#endif
#ifdef HAVE_SYS_THR_H
#include <sys/ucontext.h>
#include <sys/thr.h>
#endif

#include "wine/library.h"
#include "wine/pthread.h"

static int init_done;
static int nb_threads = 1;

#ifndef __i386__
static pthread_key_t teb_key;
#endif

/***********************************************************************
 *           init_process
 *
 * Initialization for a newly created process.
 */
static void init_process( const struct wine_pthread_callbacks *callbacks, size_t size )
{
    init_done = 1;
}


/***********************************************************************
 *           init_thread
 *
 * Initialization for a newly created thread.
 */
static void init_thread( struct wine_pthread_thread_info *info )
{
    /* retrieve the stack info (except for main thread) */
    if (init_done)
    {
#ifdef HAVE_PTHREAD_GETATTR_NP
        pthread_attr_t attr;
        pthread_getattr_np( pthread_self(), &attr );
        pthread_attr_getstack( &attr, &info->stack_base, &info->stack_size );
        pthread_attr_destroy( &attr );
#elif defined(HAVE_PTHREAD_ATTR_GET_NP)
        pthread_attr_t attr;
        pthread_attr_init( &attr );
        pthread_attr_get_np( pthread_self(), &attr );
        pthread_attr_getstack( &attr, &info->stack_base, &info->stack_size );
        pthread_attr_destroy( &attr );
#elif defined(HAVE_PTHREAD_GET_STACKSIZE_NP) && defined(HAVE_PTHREAD_GET_STACKADDR_NP)
        char dummy;
        info->stack_size = pthread_get_stacksize_np(pthread_self());
        info->stack_base = pthread_get_stackaddr_np(pthread_self());
        /* if base is too large assume it's the top of the stack instead */
        if ((char *)info->stack_base > &dummy)
            info->stack_base = (char *)info->stack_base - info->stack_size;
#else
        /* assume that the stack allocation is page aligned */
        char dummy;
        size_t page_size = getpagesize();
        char *stack_top = (char *)((unsigned long)&dummy & ~(page_size - 1)) + page_size;
        info->stack_base = stack_top - info->stack_size;
#endif
    }
}


/***********************************************************************
 *           create_thread
 */
static int create_thread( struct wine_pthread_thread_info *info )
{
    pthread_t id;
    pthread_attr_t attr;
    int ret = 0;

    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, info->stack_size );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM ); /* force creating a kernel thread on Solaris */
    interlocked_xchg_add( &nb_threads, 1 );
    if (pthread_create( &id, &attr, (void * (*)(void *))info->entry, info ))
    {
        interlocked_xchg_add( &nb_threads, -1 );
        ret = -1;
    }
    pthread_attr_destroy( &attr );
    return ret;
}


/***********************************************************************
 *           init_current_teb
 *
 * Set the current TEB for a new thread.
 */
static void init_current_teb( struct wine_pthread_thread_info *info )
{
#ifdef __i386__
    /* On the i386, the current thread is in the %fs register */
    LDT_ENTRY fs_entry;

    wine_ldt_set_base( &fs_entry, info->teb_base );
    wine_ldt_set_limit( &fs_entry, info->teb_size - 1 );
    wine_ldt_set_flags( &fs_entry, WINE_LDT_FLAGS_DATA|WINE_LDT_FLAGS_32BIT );
    wine_ldt_init_fs( info->teb_sel, &fs_entry );
#else
    if (!init_done)  /* first thread */
        pthread_key_create( &teb_key, NULL );
    pthread_setspecific( teb_key, info->teb_base );
#endif

    /* set pid and tid */
    info->pid = getpid();
#ifdef __sun
    info->tid = pthread_self();  /* this should return the lwp id on solaris */
#elif defined(__APPLE__)
    info->tid = mach_thread_self();
#elif defined(__FreeBSD__)
    {
        long lwpid;
        thr_self( &lwpid );
        info->tid = (int) lwpid;
    }
#else
    info->tid = gettid();
#endif
}


/***********************************************************************
 *           get_current_teb
 */
static void *get_current_teb(void)
{
#ifdef __i386__
    void *ret;
    __asm__( ".byte 0x64\n\tmovl 0x18,%0" : "=r" (ret) );
    return ret;
#else
    return pthread_getspecific( teb_key );
#endif
}


/***********************************************************************
 *           exit_thread
 */
static void DECLSPEC_NORETURN exit_thread( struct wine_pthread_thread_info *info )
{
    if (interlocked_xchg_add( &nb_threads, -1 ) <= 1) exit( info->exit_status );
    wine_ldt_free_fs( info->teb_sel );
    if (info->teb_size) munmap( info->teb_base, info->teb_size );
    pthread_exit( (void *)info->exit_status );
}


/***********************************************************************
 *           abort_thread
 */
static void DECLSPEC_NORETURN abort_thread( long status )
{
    if (interlocked_xchg_add( &nb_threads, -1 ) <= 1) _exit( status );
    pthread_exit( (void *)status );
}


/***********************************************************************
 *           pthread_functions
 */
const struct wine_pthread_functions pthread_functions =
{
    init_process,
    init_thread,
    create_thread,
    init_current_teb,
    get_current_teb,
    exit_thread,
    abort_thread,
    pthread_sigmask
};

#endif  /* HAVE_PTHREAD_H */
