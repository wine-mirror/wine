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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_PTHREAD_H
#define __WINE_WINE_PTHREAD_H

struct wine_pthread_functions;

#ifdef HAVE_PTHREAD_H

#define _GNU_SOURCE
#include <pthread.h>

#ifndef HAVE_PTHREAD_RWLOCK_T
typedef void *pthread_rwlock_t;
#endif
#ifndef HAVE_PTHREAD_RWLOCKATTR_T
typedef void *pthread_rwlockattr_t;
#endif

struct wine_pthread_functions
{
    size_t      size;
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

/* we don't want to include winnt.h here */
#ifndef DECLSPEC_NORETURN
# if defined(_MSC_VER) && (_MSC_VER >= 1200)
#  define DECLSPEC_NORETURN __declspec(noreturn)
# elif defined(__GNUC__)
#  define DECLSPEC_NORETURN __attribute__((noreturn))
# else
#  define DECLSPEC_NORETURN
# endif
#endif

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
    int            exit_status; /* thread exit status when calling wine_pthread_exit_thread */
};

extern void wine_pthread_init_process( const struct wine_pthread_functions *functions );
extern void wine_pthread_init_thread( struct wine_pthread_thread_info *info );
extern int wine_pthread_create_thread( struct wine_pthread_thread_info *info );
extern void wine_pthread_init_current_teb( struct wine_pthread_thread_info *info );
extern void *wine_pthread_get_current_teb(void);
extern void DECLSPEC_NORETURN wine_pthread_exit_thread( struct wine_pthread_thread_info *info );
extern void DECLSPEC_NORETURN wine_pthread_abort_thread( int status );

#endif  /* __WINE_WINE_PTHREAD_H */
