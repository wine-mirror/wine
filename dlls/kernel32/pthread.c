/*
 * pthread emulation for re-entrant libcs
 *
 * Copyright 1999 Ove Kåven
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

#define _GNU_SOURCE /* we may need to override some GNU extensions */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "kernel_private.h"
#include "wine/pthread.h"

#define P_OUTPUT(stuff) write(2,stuff,strlen(stuff))

/* NOTE: This is a truly extremely incredibly ugly hack!
 * But it does seem to work... */

/* assume that pthread_mutex_t has room for at least one pointer,
 * and hope that the users of pthread_mutex_t considers it opaque
 * (never checks what's in it)
 * also: assume that static initializer sets pointer to NULL
 */
typedef struct
{
#ifdef __GLIBC__
  int reserved;
#endif
  CRITICAL_SECTION *critsect;
} *wine_mutex;

/* see wine_mutex above for comments */
typedef struct {
  RTL_RWLOCK *lock;
} *wine_rwlock;

struct pthread_thread_init
{
    void* (*start_routine)(void*);
    void* arg;
};

static DWORD CALLBACK pthread_thread_start(LPVOID data)
{
  struct pthread_thread_init init = *(struct pthread_thread_init*)data;
  HeapFree(GetProcessHeap(),0,data);
  return (DWORD_PTR)init.start_routine(init.arg);
}

static int wine_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void*
                               (*start_routine)(void *), void* arg)
{
  HANDLE hThread;
  struct pthread_thread_init* idata = HeapAlloc(GetProcessHeap(), 0, sizeof(struct pthread_thread_init));

  idata->start_routine = start_routine;
  idata->arg = arg;
  hThread = CreateThread( NULL, 0, pthread_thread_start, idata, 0, (LPDWORD)thread);

  if(hThread)
    CloseHandle(hThread);
  else
  {
    HeapFree(GetProcessHeap(),0,idata); /* free idata struct on failure */
    return EAGAIN;
  }

  return 0;
}

static int wine_pthread_cancel(pthread_t thread)
{
  HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)thread);

  if(!TerminateThread(hThread, 0))
  {
    CloseHandle(hThread);
    return EINVAL;      /* return error */
  }

  CloseHandle(hThread);

  return 0;             /* return success */
}

static int wine_pthread_join(pthread_t thread, void **value_ptr)
{
  HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)thread);

  WaitForSingleObject(hThread, INFINITE);
  if(!GetExitCodeThread(hThread, (LPDWORD)value_ptr))
  {
    CloseHandle(hThread);
    return EINVAL; /* FIXME: make this more correctly match */
  }                /* windows errors */

  CloseHandle(hThread);
  return 0;
}

/*FIXME: not sure what to do with this one... */
static int wine_pthread_detach(pthread_t thread)
{
  P_OUTPUT("FIXME:pthread_detach\n");
  return 0;
}

/***** MUTEXES *****/

static int wine_pthread_mutex_init(pthread_mutex_t *mutex,
                                   const pthread_mutexattr_t *mutexattr)
{
  /* glibc has a tendency to initialize mutexes very often, even
     in situations where they are not really used later on.

     As for us, initializing a mutex is very expensive, we postpone
     the real initialization until the time the mutex is first used. */

  ((wine_mutex)mutex)->critsect = NULL;
  return 0;
}

static void mutex_real_init( pthread_mutex_t *mutex )
{
  CRITICAL_SECTION *critsect = HeapAlloc(GetProcessHeap(), 0, sizeof(CRITICAL_SECTION));
  RtlInitializeCriticalSection(critsect);

  if (InterlockedCompareExchangePointer((void**)&(((wine_mutex)mutex)->critsect),critsect,NULL) != NULL) {
    /* too late, some other thread already did it */
    RtlDeleteCriticalSection(critsect);
    HeapFree(GetProcessHeap(), 0, critsect);
  }
}

static int wine_pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  RtlEnterCriticalSection(((wine_mutex)mutex)->critsect);
  return 0;
}

static int wine_pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  if (!RtlTryEnterCriticalSection(((wine_mutex)mutex)->critsect)) return EBUSY;
  return 0;
}

static int wine_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    CRITICAL_SECTION *crit = ((wine_mutex)mutex)->critsect;

    if (!crit) return 0;
    if (crit->OwningThread != ULongToHandle(GetCurrentThreadId())) return EPERM;
    RtlLeaveCriticalSection( crit );
    return 0;
}

static int wine_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect) return 0;
  if (((wine_mutex)mutex)->critsect->RecursionCount) {
#if 0 /* there seems to be a bug in libc6 that makes this a bad idea */
    return EBUSY;
#else
    while (((wine_mutex)mutex)->critsect->RecursionCount)
      RtlLeaveCriticalSection(((wine_mutex)mutex)->critsect);
#endif
  }
  RtlDeleteCriticalSection(((wine_mutex)mutex)->critsect);
  HeapFree(GetProcessHeap(), 0, ((wine_mutex)mutex)->critsect);
  ((wine_mutex)mutex)->critsect = NULL;
  return 0;
}

/***** READ-WRITE LOCKS *****/

static void rwlock_real_init(pthread_rwlock_t *rwlock)
{
  RTL_RWLOCK *lock = HeapAlloc(GetProcessHeap(), 0, sizeof(RTL_RWLOCK));
  RtlInitializeResource(lock);

  if (InterlockedCompareExchangePointer((void**)&(((wine_rwlock)rwlock)->lock),lock,NULL) != NULL) {
    /* too late, some other thread already did it */
    RtlDeleteResource(lock);
    HeapFree(GetProcessHeap(), 0, lock);
  }
}

static int wine_pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *rwlock_attr)
{
  ((wine_rwlock)rwlock)->lock = NULL;
  return 0;
}

static int wine_pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock) return 0;
  RtlDeleteResource(((wine_rwlock)rwlock)->lock);
  HeapFree(GetProcessHeap(), 0, ((wine_rwlock)rwlock)->lock);
  return 0;
}

static int wine_pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  while(TRUE)
    if (RtlAcquireResourceShared(((wine_rwlock)rwlock)->lock, TRUE))
      return 0;
}

static int wine_pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  if (!RtlAcquireResourceShared(((wine_rwlock)rwlock)->lock, FALSE)) return EBUSY;
  return 0;
}

static int wine_pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  while(TRUE)
    if (RtlAcquireResourceExclusive(((wine_rwlock)rwlock)->lock, TRUE))
      return 0;
}

static int wine_pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  if (!RtlAcquireResourceExclusive(((wine_rwlock)rwlock)->lock, FALSE)) return EBUSY;
  return 0;
}

static int wine_pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock) return 0;
  RtlReleaseResource( ((wine_rwlock)rwlock)->lock );
  return 0;
}

/***** CONDITIONS *****/

/* The condition code is basically cut-and-pasted from Douglas
 * Schmidt's paper:
 *   "Strategies for Implementing POSIX Condition Variables on Win32",
 * at http://www.cs.wustl.edu/~schmidt/win32-cv-1.html and
 * http://www.cs.wustl.edu/~schmidt/win32-cv-2.html.
 * This paper formed the basis for the condition variable
 * impementation used in the ACE library.
 */

/* Possible problems with ACE:
 * - unimplemented pthread_mutexattr_init
 */
typedef struct {
  /* Number of waiting threads. */
  int waiters_count;

  /* Serialize access to <waiters_count>. */
  CRITICAL_SECTION waiters_count_lock;

  /*
   * Semaphore used to queue up threads waiting for the condition to
   * become signaled.
   */
  HANDLE sema;

  /*
   * An auto-reset event used by the broadcast/signal thread to wait
   * for all the waiting thread(s) to wake up and be released from the
   * semaphore.
   */
  HANDLE waiters_done;

  /*
   * Keeps track of whether we were broadcasting or signaling.  This
   * allows us to optimize the code if we're just signaling.
   */
  size_t was_broadcast;
} wine_cond_detail;

/* see wine_mutex above for comments */
typedef struct {
  wine_cond_detail *cond;
} *wine_cond;

static void wine_cond_real_init(pthread_cond_t *cond)
{
  wine_cond_detail *detail = HeapAlloc(GetProcessHeap(), 0, sizeof(wine_cond_detail));
  detail->waiters_count = 0;
  detail->was_broadcast = 0;
  detail->sema = CreateSemaphoreW( NULL, 0, 0x7fffffff, NULL );
  detail->waiters_done = CreateEventW( NULL, FALSE, FALSE, NULL );
  RtlInitializeCriticalSection (&detail->waiters_count_lock);

  if (InterlockedCompareExchangePointer((void**)&(((wine_cond)cond)->cond), detail, NULL) != NULL)
  {
    /* too late, some other thread already did it */
    P_OUTPUT("FIXME:pthread_cond_init:expect troubles...\n");
    CloseHandle(detail->sema);
    RtlDeleteCriticalSection(&detail->waiters_count_lock);
    CloseHandle(detail->waiters_done);
    HeapFree(GetProcessHeap(), 0, detail);
  }
}

static int wine_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
  /* The same as for wine_pthread_mutex_init, we postpone initialization
     until condition is really used.*/
  ((wine_cond)cond)->cond = NULL;
  return 0;
}

static int wine_pthread_cond_destroy(pthread_cond_t *cond)
{
  wine_cond_detail *detail = ((wine_cond)cond)->cond;

  if (!detail) return 0;
  CloseHandle(detail->sema);
  RtlDeleteCriticalSection(&detail->waiters_count_lock);
  CloseHandle(detail->waiters_done);
  HeapFree(GetProcessHeap(), 0, detail);
  ((wine_cond)cond)->cond = NULL;
  return 0;
}

static int wine_pthread_cond_signal(pthread_cond_t *cond)
{
  int have_waiters;
  wine_cond_detail *detail;

  if ( !((wine_cond)cond)->cond ) wine_cond_real_init(cond);
  detail = ((wine_cond)cond)->cond;

  RtlEnterCriticalSection (&detail->waiters_count_lock);
  have_waiters = detail->waiters_count > 0;
  RtlLeaveCriticalSection (&detail->waiters_count_lock);

  /* If there aren't any waiters, then this is a no-op. */
  if (have_waiters)
    ReleaseSemaphore(detail->sema, 1, NULL);

  return 0;
}

static int wine_pthread_cond_broadcast(pthread_cond_t *cond)
{
  int have_waiters = 0;
  wine_cond_detail *detail;

  if ( !((wine_cond)cond)->cond ) wine_cond_real_init(cond);
  detail = ((wine_cond)cond)->cond;

  /*
   * This is needed to ensure that <waiters_count> and <was_broadcast> are
   * consistent relative to each other.
   */
  RtlEnterCriticalSection (&detail->waiters_count_lock);

  if (detail->waiters_count > 0) {
    /*
     * We are broadcasting, even if there is just one waiter...
     * Record that we are broadcasting, which helps optimize
     * <pthread_cond_wait> for the non-broadcast case.
     */
    detail->was_broadcast = 1;
    have_waiters = 1;
  }

  if (have_waiters) {
    /* Wake up all the waiters atomically. */
    ReleaseSemaphore(detail->sema, detail->waiters_count, NULL);

    RtlLeaveCriticalSection (&detail->waiters_count_lock);

    /* Wait for all the awakened threads to acquire the counting semaphore. */
    WaitForSingleObject (detail->waiters_done, INFINITE);

    /*
     * This assignment is okay, even without the <waiters_count_lock> held
     * because no other waiter threads can wake up to access it.
     */
    detail->was_broadcast = 0;
  }
  else
    RtlLeaveCriticalSection (&detail->waiters_count_lock);
  return 0;
}

static int wine_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  wine_cond_detail *detail;
  int last_waiter;

  if ( !((wine_cond)cond)->cond ) wine_cond_real_init(cond);
  detail = ((wine_cond)cond)->cond;

  /* Avoid race conditions. */
  RtlEnterCriticalSection (&detail->waiters_count_lock);
  detail->waiters_count++;
  RtlLeaveCriticalSection (&detail->waiters_count_lock);

  RtlLeaveCriticalSection ( ((wine_mutex)mutex)->critsect );
  WaitForSingleObject(detail->sema, INFINITE);

  /* Reacquire lock to avoid race conditions. */
  RtlEnterCriticalSection (&detail->waiters_count_lock);

  /* We're no longer waiting... */
  detail->waiters_count--;

  /* Check to see if we're the last waiter after <pthread_cond_broadcast>. */
  last_waiter = detail->was_broadcast && detail->waiters_count == 0;

  RtlLeaveCriticalSection (&detail->waiters_count_lock);

  /*
   * If we're the last waiter thread during this particular broadcast
   * then let all the other threads proceed.
   */
  if (last_waiter) SetEvent(detail->waiters_done);
  RtlEnterCriticalSection (((wine_mutex)mutex)->critsect);
  return 0;
}

static int wine_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                       const struct timespec *abstime)
{
  DWORD ms = abstime->tv_sec * 1000 + abstime->tv_nsec / 1000000;
  int last_waiter;
  wine_cond_detail *detail;

  if ( !((wine_cond)cond)->cond ) wine_cond_real_init(cond);
  detail = ((wine_cond)cond)->cond;

  /* Avoid race conditions. */
  RtlEnterCriticalSection (&detail->waiters_count_lock);
  detail->waiters_count++;
  RtlLeaveCriticalSection (&detail->waiters_count_lock);

  RtlLeaveCriticalSection (((wine_mutex)mutex)->critsect);
  WaitForSingleObject (detail->sema, ms);

  /* Reacquire lock to avoid race conditions. */
  RtlEnterCriticalSection (&detail->waiters_count_lock);

  /* We're no longer waiting... */
  detail->waiters_count--;

  /* Check to see if we're the last waiter after <pthread_cond_broadcast>. */
  last_waiter = detail->was_broadcast && detail->waiters_count == 0;

  RtlLeaveCriticalSection (&detail->waiters_count_lock);

  /*
   * If we're the last waiter thread during this particular broadcast
   * then let all the other threads proceed.
   */
  if (last_waiter) SetEvent (detail->waiters_done);
  RtlEnterCriticalSection (((wine_mutex)mutex)->critsect);
  return 0;
}

/***** MISC *****/

static pthread_t wine_pthread_self(void)
{
  return (pthread_t)GetCurrentThreadId();
}

static int wine_pthread_equal(pthread_t thread1, pthread_t thread2)
{
  return (DWORD)thread1 == (DWORD)thread2;
}

static void wine_pthread_exit(void *retval, char *currentframe)
{
    ExitThread((DWORD_PTR)retval);
}

static void *wine_get_thread_data(void)
{
    return kernel_get_thread_data()->pthread_data;
}

static void wine_set_thread_data( void *data )
{
    kernel_get_thread_data()->pthread_data = data;
}

static const struct wine_pthread_callbacks callbacks =
{
    wine_get_thread_data,           /* ptr_get_thread_data */
    wine_set_thread_data,           /* ptr_set_thread_data */
    wine_pthread_self,              /* ptr_pthread_self */
    wine_pthread_equal,             /* ptr_pthread_equal */
    wine_pthread_create,            /* ptr_pthread_create */
    wine_pthread_cancel,            /* ptr_pthread_cancel */
    wine_pthread_join,              /* ptr_pthread_join */
    wine_pthread_detach,            /* ptr_pthread_detach */
    wine_pthread_exit,              /* ptr_pthread_exit */
    wine_pthread_mutex_init,        /* ptr_pthread_mutex_init */
    wine_pthread_mutex_lock,        /* ptr_pthread_mutex_lock */
    wine_pthread_mutex_trylock,     /* ptr_pthread_mutex_trylock */
    wine_pthread_mutex_unlock,      /* ptr_pthread_mutex_unlock */
    wine_pthread_mutex_destroy,     /* ptr_pthread_mutex_destroy */
    wine_pthread_rwlock_init,       /* ptr_pthread_rwlock_init */
    wine_pthread_rwlock_destroy,    /* ptr_pthread_rwlock_destroy */
    wine_pthread_rwlock_rdlock,     /* ptr_pthread_rwlock_rdlock */
    wine_pthread_rwlock_tryrdlock,  /* ptr_pthread_rwlock_tryrdlock */
    wine_pthread_rwlock_wrlock,     /* ptr_pthread_rwlock_wrlock */
    wine_pthread_rwlock_trywrlock,  /* ptr_pthread_rwlock_trywrlock */
    wine_pthread_rwlock_unlock,     /* ptr_pthread_rwlock_unlock */
    wine_pthread_cond_init,         /* ptr_pthread_cond_init */
    wine_pthread_cond_destroy,      /* ptr_pthread_cond_destroy */
    wine_pthread_cond_signal,       /* ptr_pthread_cond_signal */
    wine_pthread_cond_broadcast,    /* ptr_pthread_cond_broadcast */
    wine_pthread_cond_wait,         /* ptr_pthread_cond_wait */
    wine_pthread_cond_timedwait     /* ptr_pthread_cond_timedwait */
};

static struct wine_pthread_functions pthread_functions;

void PTHREAD_Init(void)
{
    wine_pthread_get_functions( &pthread_functions, sizeof(pthread_functions) );
    pthread_functions.init_process( &callbacks, sizeof(callbacks) );
}

#endif /* HAVE_PTHREAD_H */
