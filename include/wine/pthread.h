/*
 * Definitions for Wine pthread emulation
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

#ifndef __WINE_WINE_PTHREAD_H
#define __WINE_WINE_PTHREAD_H

struct wine_pthread_callbacks;

#include <signal.h>

#ifndef HAVE_SIGSET_T
struct sigset_t;
typedef struct sigset_t sigset_t;
#endif

#ifdef HAVE_PTHREAD_H

#define _GNU_SOURCE
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif

#ifndef HAVE_PTHREAD_RWLOCK_T
typedef void *pthread_rwlock_t;
#endif
#ifndef HAVE_PTHREAD_RWLOCKATTR_T
typedef void *pthread_rwlockattr_t;
#endif

struct wine_pthread_callbacks
{
    void *    (*ptr_get_thread_data)(void);
    void      (*ptr_set_thread_data)(void *data);
    pthread_t (*ptr_pthread_self)(void);
    int       (*ptr_pthread_equal)(pthread_t thread1, pthread_t thread2);
    int       (*ptr_pthread_create)(pthread_t* thread, const pthread_attr_t* attr,
                                    void* (*start_routine)(void *), void* arg);
    int       (*ptr_pthread_cancel)(pthread_t thread);
    int       (*ptr_pthread_join)(pthread_t thread, void **value_ptr);
    int       (*ptr_pthread_detach)(pthread_t thread);
    void      (*ptr_pthread_exit)(void *retval, char *currentframe);
    int       (*ptr_pthread_mutex_init)(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
    int       (*ptr_pthread_mutex_lock)(pthread_mutex_t *mutex);
    int       (*ptr_pthread_mutex_trylock)(pthread_mutex_t *mutex);
    int       (*ptr_pthread_mutex_unlock)(pthread_mutex_t *mutex);
    int       (*ptr_pthread_mutex_destroy)(pthread_mutex_t *mutex);
    int       (*ptr_pthread_rwlock_init)(pthread_rwlock_t *rwlock,
                                         const pthread_rwlockattr_t *rwlock_attr);
    int       (*ptr_pthread_rwlock_destroy)(pthread_rwlock_t *rwlock);
    int       (*ptr_pthread_rwlock_rdlock)(pthread_rwlock_t *rwlock);
    int       (*ptr_pthread_rwlock_tryrdlock)(pthread_rwlock_t *rwlock);
    int       (*ptr_pthread_rwlock_wrlock)(pthread_rwlock_t *rwlock);
    int       (*ptr_pthread_rwlock_trywrlock)(pthread_rwlock_t *rwlock);
    int       (*ptr_pthread_rwlock_unlock)(pthread_rwlock_t *rwlock);
    int       (*ptr_pthread_cond_init)(pthread_cond_t *cond, const pthread_condattr_t *cond_attr);
    int       (*ptr_pthread_cond_destroy)(pthread_cond_t *cond);
    int       (*ptr_pthread_cond_signal)(pthread_cond_t *cond);
    int       (*ptr_pthread_cond_broadcast)(pthread_cond_t *cond);
    int       (*ptr_pthread_cond_wait)(pthread_cond_t *cond, pthread_mutex_t *mutex);
    int       (*ptr_pthread_cond_timedwait)(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                            const struct timespec *abstime);
};

#endif /* HAVE_PTHREAD_H */

/* thread information used to creating and exiting threads */
struct wine_pthread_thread_info
{
    void          *stack_base;  /* base address of the stack */
    size_t         stack_size;  /* size of the stack */
    void          *teb_base;    /* base address of the TEB */
    size_t         teb_size;    /* size of the TEB (possibly including signal stack) */
    unsigned short teb_sel;     /* selector to use for TEB */
    int            pid;         /* Unix process id */
    int            tid;         /* Unix thread id */
    void         (*entry)( struct wine_pthread_thread_info *info );  /* thread entry point */
    long           exit_status; /* thread exit status when calling wine_pthread_exit_thread */
};

struct wine_pthread_functions
{
    void   (*init_process)( const struct wine_pthread_callbacks *callbacks, size_t size );
    void   (*init_thread)( struct wine_pthread_thread_info *info );
    int    (*create_thread)( struct wine_pthread_thread_info *info );
    void   (*init_current_teb)( struct wine_pthread_thread_info *info );
    void * (*get_current_teb)(void);
#ifdef __GNUC__
    void   (* __attribute__((noreturn)) exit_thread)( struct wine_pthread_thread_info *info );
    void   (* __attribute__((noreturn)) abort_thread)( long status );
#else
    void   (*exit_thread)( struct wine_pthread_thread_info *info );
    void   (*abort_thread)( long status );
#endif
    int    (*sigprocmask)( int how, const sigset_t *newset, sigset_t *oldset );
};

extern void wine_pthread_get_functions( struct wine_pthread_functions *functions, size_t size );
extern void wine_pthread_set_functions( const struct wine_pthread_functions *functions, size_t size );

#endif  /* __WINE_WINE_PTHREAD_H */
