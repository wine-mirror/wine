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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

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
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "thread.h"
#include "winternl.h"
#include "wine/pthread.h"

#define P_OUTPUT(stuff) write(2,stuff,strlen(stuff))

static const struct wine_pthread_functions functions;

DECL_GLOBAL_CONSTRUCTOR(pthread_init) { wine_pthread_init_process( &functions ); }

static inline int init_done(void) { return GetProcessHeap() != 0; }

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
  return (DWORD)init.start_routine(init.arg);
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
  if (!init_done()) return 0;
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  RtlEnterCriticalSection(((wine_mutex)mutex)->critsect);
  return 0;
}

static int wine_pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (!init_done()) return 0;
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  if (!RtlTryEnterCriticalSection(((wine_mutex)mutex)->critsect)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}

static int wine_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect) return 0;
  RtlLeaveCriticalSection(((wine_mutex)mutex)->critsect);
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
  if (!init_done()) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  while(TRUE)
    if (RtlAcquireResourceShared(((wine_rwlock)rwlock)->lock, TRUE))
      return 0;
}

static int wine_pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
  if (!init_done()) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  if (!RtlAcquireResourceShared(((wine_rwlock)rwlock)->lock, FALSE)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}

static int wine_pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
  if (!init_done()) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  while(TRUE)
    if (RtlAcquireResourceExclusive(((wine_rwlock)rwlock)->lock, TRUE))
      return 0;
}

static int wine_pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
  if (!init_done()) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  if (!RtlAcquireResourceExclusive(((wine_rwlock)rwlock)->lock, FALSE)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}

static int wine_pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock) return 0;
  RtlReleaseResource( ((wine_rwlock)rwlock)->lock );
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
    ExitThread((DWORD)retval);
}

static void *wine_get_thread_data(void)
{
    return NtCurrentTeb()->pthread_data;
}

static void wine_set_thread_data( void *data )
{
    NtCurrentTeb()->pthread_data = data;
}

static const struct wine_pthread_functions functions =
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
    wine_pthread_rwlock_unlock      /* ptr_pthread_rwlock_unlock */
};
