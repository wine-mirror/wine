/*
 * pthread emulation for re-entrant libcs
 *
 * We can't use pthreads directly, so why not let libcs
 * that want pthreads use Wine's own threading instead...
 *
 * Copyright 1999 Ove Kåven
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

struct _pthread_cleanup_buffer;

#include "config.h"
#include "wine/port.h"

#define _GNU_SOURCE /* we may need to override some GNU extensions */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>

#include "winbase.h"
#include "thread.h"
#include "winternl.h"

/* Currently this probably works only for glibc2,
 * which checks for the presence of double-underscore-prepended
 * pthread primitives, and use them if available.
 * If they are not available, the libc defaults to
 * non-threadsafe operation (not good). */

#if defined(__GLIBC__) || defined(__FreeBSD__)

#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif

#include <pthread.h>
#include <signal.h>

#define PSTR(str) __ASM_NAME(#str)

/* adapt as necessary (a construct like this is used in glibc sources) */
#define strong_alias(orig, alias) \
 asm(".globl " PSTR(alias) "\n" \
     "\t.set " PSTR(alias) "," PSTR(orig))

static int init_done;

static pid_t (*libc_fork)(void);
static int (*libc_sigaction)(int signum, const struct sigaction *act, struct sigaction *oldact);

void PTHREAD_init_done(void)
{
    init_done = 1;
    if (!libc_fork) libc_fork = dlsym( RTLD_NEXT, "fork" );
    if (!libc_sigaction) libc_sigaction = dlsym( RTLD_NEXT, "sigaction" );
}


/* NOTE: This is a truly extremely incredibly ugly hack!
 * But it does seem to work... */

/* assume that pthread_mutex_t has room for at least one pointer,
 * and hope that the users of pthread_mutex_t considers it opaque
 * (never checks what's in it)
 * also: assume that static initializer sets pointer to NULL
 */
typedef struct {
  CRITICAL_SECTION *critsect;
} *wine_mutex;

/* see wine_mutex above for comments */
typedef struct {
  RTL_RWLOCK *lock;
} *wine_rwlock;

typedef struct _wine_cleanup {
  void (*routine)(void *);
  void *arg;
} *wine_cleanup;

typedef const void *key_data;

#define FIRST_KEY 0
#define MAX_KEYS 16 /* libc6 doesn't use that many, but... */

#define P_OUTPUT(stuff) write(2,stuff,strlen(stuff))

void __pthread_initialize(void)
{
}

struct pthread_thread_init {
	 void* (*start_routine)(void*);
	 void* arg;
};

static DWORD CALLBACK pthread_thread_start(LPVOID data)
{
  struct pthread_thread_init init = *(struct pthread_thread_init*)data;
  HeapFree(GetProcessHeap(),0,data);
  return (DWORD)init.start_routine(init.arg);
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void*
        (*start_routine)(void *), void* arg)
{
  HANDLE hThread;
  struct pthread_thread_init* idata = HeapAlloc(GetProcessHeap(), 0,
		sizeof(struct pthread_thread_init));

  idata->start_routine = start_routine;
  idata->arg = arg;
  hThread = CreateThread( NULL, 0, pthread_thread_start, idata, 0,
                          (LPDWORD)thread);

  if(hThread)
    CloseHandle(hThread);
  else
  {
    HeapFree(GetProcessHeap(),0,idata); /* free idata struct on failure */
    return EAGAIN;
  }

  return 0;
}

int pthread_cancel(pthread_t thread)
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

int pthread_join(pthread_t thread, void **value_ptr)
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
int pthread_detach(pthread_t thread)
{
  P_OUTPUT("FIXME:pthread_detach\n");
  return 0;
}

/* FIXME: we have no equivalents in win32 for the policys */
/* so just keep this as a stub */
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
  P_OUTPUT("FIXME:pthread_attr_setschedpolicy\n");
  return 0;
}

/* FIXME: no win32 equivalent for scope */
int pthread_attr_setscope(pthread_attr_t *attr, int scope)
{
  P_OUTPUT("FIXME:pthread_attr_setscope\n");
  return 0; /* return success */
}

/* FIXME: no win32 equivalent for schedule param */
int pthread_attr_setschedparam(pthread_attr_t *attr,
    const struct sched_param *param)
{
  P_OUTPUT("FIXME:pthread_attr_setschedparam\n");
  return 0; /* return success */
}

int __pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
  static pthread_once_t the_once = PTHREAD_ONCE_INIT;
  LONG once_now;

  memcpy(&once_now,&the_once,sizeof(once_now));
  if (InterlockedCompareExchange((LONG*)once_control, once_now+1, once_now) == once_now)
    (*init_routine)();
  return 0;
}
strong_alias(__pthread_once, pthread_once);

void __pthread_kill_other_threads_np(void)
{
    /* we don't need to do anything here */
}
strong_alias(__pthread_kill_other_threads_np, pthread_kill_other_threads_np);

/***** atfork *****/

#define MAX_ATFORK 8  /* libc doesn't need that many anyway */

static CRITICAL_SECTION atfork_section = CRITICAL_SECTION_INIT("atfork_section");
typedef void (*atfork_handler)();
static atfork_handler atfork_prepare[MAX_ATFORK];
static atfork_handler atfork_parent[MAX_ATFORK];
static atfork_handler atfork_child[MAX_ATFORK];
static int atfork_count;

int __pthread_atfork(void (*prepare)(void),
		     void (*parent)(void),
		     void (*child)(void))
{
    if (init_done) EnterCriticalSection( &atfork_section );
    assert( atfork_count < MAX_ATFORK );
    atfork_prepare[atfork_count] = prepare;
    atfork_parent[atfork_count] = parent;
    atfork_child[atfork_count] = child;
    atfork_count++;
    if (init_done) LeaveCriticalSection( &atfork_section );
    return 0;
}
strong_alias(__pthread_atfork, pthread_atfork);

pid_t __fork(void)
{
    pid_t pid;
    int i;

    if (!libc_fork)
    {
        libc_fork = dlsym( RTLD_NEXT, "fork" );
        assert( libc_fork );
    }
    EnterCriticalSection( &atfork_section );
    /* prepare handlers are called in reverse insertion order */
    for (i = atfork_count - 1; i >= 0; i--) if (atfork_prepare[i]) atfork_prepare[i]();
    if (!(pid = libc_fork()))
    {
        InitializeCriticalSection( &atfork_section );
        for (i = 0; i < atfork_count; i++) if (atfork_child[i]) atfork_child[i]();
    }
    else
    {
        for (i = 0; i < atfork_count; i++) if (atfork_parent[i]) atfork_parent[i]();
        LeaveCriticalSection( &atfork_section );
    }
    return pid;
}
strong_alias(__fork, fork);

/***** MUTEXES *****/

int __pthread_mutex_init(pthread_mutex_t *mutex,
                        const pthread_mutexattr_t *mutexattr)
{
  /* glibc has a tendency to initialize mutexes very often, even
     in situations where they are not really used later on.

     As for us, initializing a mutex is very expensive, we postpone
     the real initialization until the time the mutex is first used. */

  ((wine_mutex)mutex)->critsect = NULL;
  return 0;
}
strong_alias(__pthread_mutex_init, pthread_mutex_init);

static void mutex_real_init( pthread_mutex_t *mutex )
{
  CRITICAL_SECTION *critsect = HeapAlloc(GetProcessHeap(), 0, sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(critsect);

  if (InterlockedCompareExchangePointer((void**)&(((wine_mutex)mutex)->critsect),critsect,NULL) != NULL) {
    /* too late, some other thread already did it */
    DeleteCriticalSection(critsect);
    HeapFree(GetProcessHeap(), 0, critsect);
  }
}

int __pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (!init_done) return 0;
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  EnterCriticalSection(((wine_mutex)mutex)->critsect);
  return 0;
}
strong_alias(__pthread_mutex_lock, pthread_mutex_lock);

int __pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (!init_done) return 0;
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  if (!TryEnterCriticalSection(((wine_mutex)mutex)->critsect)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}
strong_alias(__pthread_mutex_trylock, pthread_mutex_trylock);

int __pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect) return 0;
  LeaveCriticalSection(((wine_mutex)mutex)->critsect);
  return 0;
}
strong_alias(__pthread_mutex_unlock, pthread_mutex_unlock);

int __pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect) return 0;
  if (((wine_mutex)mutex)->critsect->RecursionCount) {
#if 0 /* there seems to be a bug in libc6 that makes this a bad idea */
    return EBUSY;
#else
    while (((wine_mutex)mutex)->critsect->RecursionCount)
      LeaveCriticalSection(((wine_mutex)mutex)->critsect);
#endif
  }
  DeleteCriticalSection(((wine_mutex)mutex)->critsect);
  HeapFree(GetProcessHeap(), 0, ((wine_mutex)mutex)->critsect);
  return 0;
}
strong_alias(__pthread_mutex_destroy, pthread_mutex_destroy);


/***** MUTEX ATTRIBUTES *****/
/* just dummies, since critical sections are always recursive */

int __pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
  return 0;
}
strong_alias(__pthread_mutexattr_init, pthread_mutexattr_init);

int __pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
  return 0;
}
strong_alias(__pthread_mutexattr_destroy, pthread_mutexattr_destroy);

int __pthread_mutexattr_setkind_np(pthread_mutexattr_t *attr, int kind)
{
  return 0;
}
strong_alias(__pthread_mutexattr_setkind_np, pthread_mutexattr_setkind_np);

int __pthread_mutexattr_getkind_np(pthread_mutexattr_t *attr, int *kind)
{
  *kind = PTHREAD_MUTEX_RECURSIVE;
  return 0;
}
strong_alias(__pthread_mutexattr_getkind_np, pthread_mutexattr_getkind_np);

int __pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
  return 0;
}
strong_alias(__pthread_mutexattr_settype, pthread_mutexattr_settype);

int __pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *kind)
{
  *kind = PTHREAD_MUTEX_RECURSIVE;
  return 0;
}
strong_alias(__pthread_mutexattr_gettype, pthread_mutexattr_gettype);


/***** THREAD-SPECIFIC VARIABLES (KEYS) *****/

int __pthread_key_create(pthread_key_t *key, void (*destr_function)(void *))
{
  static LONG keycnt = FIRST_KEY;
  *key = InterlockedExchangeAdd(&keycnt, 1);
  return 0;
}
strong_alias(__pthread_key_create, pthread_key_create);

int __pthread_key_delete(pthread_key_t key)
{
  return 0;
}
strong_alias(__pthread_key_delete, pthread_key_delete);

int __pthread_setspecific(pthread_key_t key, const void *pointer)
{
  TEB *teb = NtCurrentTeb();
  if (!teb->pthread_data) {
    teb->pthread_data = calloc(MAX_KEYS,sizeof(key_data));
  }
  ((key_data*)(teb->pthread_data))[key] = pointer;
  return 0;
}
strong_alias(__pthread_setspecific, pthread_setspecific);

void *__pthread_getspecific(pthread_key_t key)
{
  TEB *teb = NtCurrentTeb();
  if (!teb) return NULL;
  if (!teb->pthread_data) return NULL;
  return (void *)(((key_data*)(teb->pthread_data))[key]);
}
strong_alias(__pthread_getspecific, pthread_getspecific);


/***** "EXCEPTION" FRAMES *****/
/* not implemented right now */

void _pthread_cleanup_push(struct _pthread_cleanup_buffer *buffer, void (*routine)(void *), void *arg)
{
  ((wine_cleanup)buffer)->routine = routine;
  ((wine_cleanup)buffer)->arg = arg;
}

void _pthread_cleanup_pop(struct _pthread_cleanup_buffer *buffer, int execute)
{
  if (execute) (*(((wine_cleanup)buffer)->routine))(((wine_cleanup)buffer)->arg);
}

void _pthread_cleanup_push_defer(struct _pthread_cleanup_buffer *buffer, void (*routine)(void *), void *arg)
{
  _pthread_cleanup_push(buffer, routine, arg);
}

void _pthread_cleanup_pop_restore(struct _pthread_cleanup_buffer *buffer, int execute)
{
  _pthread_cleanup_pop(buffer, execute);
}


/***** CONDITIONS *****/
/* not implemented right now */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
  P_OUTPUT("FIXME:pthread_cond_init\n");
  return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
  P_OUTPUT("FIXME:pthread_cond_destroy\n");
  return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
  P_OUTPUT("FIXME:pthread_cond_signal\n");
  return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
  P_OUTPUT("FIXME:pthread_cond_broadcast\n");
  return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  P_OUTPUT("FIXME:pthread_cond_wait\n");
  return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
  P_OUTPUT("FIXME:pthread_cond_timedwait\n");
  return 0;
}

/**** CONDITION ATTRIBUTES *****/
/* not implemented right now */

int pthread_condattr_init(pthread_condattr_t *attr)
{
  return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
  return 0;
}

#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 2)
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

int __pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *rwlock_attr)
{
  ((wine_rwlock)rwlock)->lock = NULL;
  return 0;
}
strong_alias(__pthread_rwlock_init, pthread_rwlock_init);

int __pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock) return 0;
  RtlDeleteResource(((wine_rwlock)rwlock)->lock);
  HeapFree(GetProcessHeap(), 0, ((wine_rwlock)rwlock)->lock);
  return 0;
}
strong_alias(__pthread_rwlock_destroy, pthread_rwlock_destroy);

int __pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
  if (!init_done) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  while(TRUE)
    if (RtlAcquireResourceShared(((wine_rwlock)rwlock)->lock, TRUE))
      return 0;
}
strong_alias(__pthread_rwlock_rdlock, pthread_rwlock_rdlock);

int __pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
  if (!init_done) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  if (!RtlAcquireResourceShared(((wine_rwlock)rwlock)->lock, FALSE)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}
strong_alias(__pthread_rwlock_tryrdlock, pthread_rwlock_tryrdlock);

int __pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
  if (!init_done) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  while(TRUE)
    if (RtlAcquireResourceExclusive(((wine_rwlock)rwlock)->lock, TRUE))
      return 0;
}
strong_alias(__pthread_rwlock_wrlock, pthread_rwlock_wrlock);

int __pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
  if (!init_done) return 0;
  if (!((wine_rwlock)rwlock)->lock)
    rwlock_real_init( rwlock );

  if (!RtlAcquireResourceExclusive(((wine_rwlock)rwlock)->lock, FALSE)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}
strong_alias(__pthread_rwlock_trywrlock, pthread_rwlock_trywrlock);

int __pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
  if (!((wine_rwlock)rwlock)->lock) return 0;
  RtlReleaseResource( ((wine_rwlock)rwlock)->lock );
  return 0;
}
strong_alias(__pthread_rwlock_unlock, pthread_rwlock_unlock);

/**** READ-WRITE LOCK ATTRIBUTES *****/
/* not implemented right now */

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
  return 0;
}

int __pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
  return 0;
}
strong_alias(__pthread_rwlockattr_destroy, pthread_rwlockattr_destroy);

int pthread_rwlockattr_getkind_np(const pthread_rwlockattr_t *attr, int *pref)
{
  *pref = 0;
  return 0;
}

int pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *attr, int pref)
{
  return 0;
}
#endif /* glibc 2.2 */

/***** MISC *****/

pthread_t pthread_self(void)
{
  return (pthread_t)GetCurrentThreadId();
}

int pthread_equal(pthread_t thread1, pthread_t thread2)
{
  return (DWORD)thread1 == (DWORD)thread2;
}

void pthread_exit(void *retval)
{
  /* FIXME: pthread cleanup */
  ExitThread((DWORD)retval);
}

int pthread_setcanceltype(int type, int *oldtype)
{
  if (oldtype) *oldtype = PTHREAD_CANCEL_ASYNCHRONOUS;
  return 0;
}

/***** ANTI-OVERRIDES *****/
/* pthreads tries to override these, point them back to libc */

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    if (!libc_sigaction)
    {
        libc_sigaction = dlsym( RTLD_NEXT, "sigaction" );
        assert( libc_sigaction );
    }
    return libc_sigaction(signum, act, oldact);
}

#else /* __GLIBC__ || __FREEBSD__ */

void PTHREAD_init_done(void)
{
}

#endif /* __GLIBC__ || __FREEBSD__ */
