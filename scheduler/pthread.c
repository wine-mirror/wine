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

#include "config.h"
#include "wine/port.h"

#ifndef HAVE_NPTL

struct _pthread_cleanup_buffer;

#define _GNU_SOURCE /* we may need to override some GNU extensions */

#include <assert.h>
#include <errno.h>
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
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

#include "winbase.h"
#include "thread.h"
#include "winternl.h"

/* default errno before threading is initialized */
static int *default_errno_location(void)
{
    static int static_errno;
    return &static_errno;
}

/* default h_errno before threading is initialized */
static int *default_h_errno_location(void)
{
    static int static_h_errno;
    return &static_h_errno;
}

/* errno once threading is working */
static int *thread_errno_location(void)
{
    return &NtCurrentTeb()->thread_errno;
}

/* h_errno once threading is working */
static int *thread_h_errno_location(void)
{
    return &NtCurrentTeb()->thread_h_errno;
}

static int* (*errno_location_ptr)(void) = default_errno_location;
static int* (*h_errno_location_ptr)(void) = default_h_errno_location;

/***********************************************************************
 *           __errno_location/__error/__errno/___errno/__thr_errno
 *
 * Get the per-thread errno location.
 */
int *__errno_location(void) { return errno_location_ptr(); }  /* Linux */
int *__error(void)          { return errno_location_ptr(); }  /* FreeBSD */
int *__errno(void)          { return errno_location_ptr(); }  /* NetBSD */
int *___errno(void)         { return errno_location_ptr(); }  /* Solaris */
int *__thr_errno(void)      { return errno_location_ptr(); }  /* UnixWare */

/***********************************************************************
 *           __h_errno_location
 *
 * Get the per-thread h_errno location.
 */
int *__h_errno_location(void)
{
    return h_errno_location_ptr();
}


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

#define P_OUTPUT(stuff) write(2,stuff,strlen(stuff))

#define PSTR(str) __ASM_NAME(#str)

/* adapt as necessary (a construct like this is used in glibc sources) */
#define strong_alias(orig, alias) \
 asm(".globl " PSTR(alias) "\n" \
     "\t.set " PSTR(alias) "," PSTR(orig))

/* thread descriptor */

#define FIRST_KEY 0
#define MAX_KEYS 16 /* libc6 doesn't use that many, but... */
#define MAX_TSD  16

struct fork_block;

struct pthread_descr_struct
{
    char               dummy[2048];
    struct __res_state res_state;
    const void        *key_data[MAX_KEYS];  /* for normal pthread keys */
    const void        *tsd_data[MAX_TSD];   /* for libc internal tsd variables */
};

typedef struct pthread_descr_struct *pthread_descr;

static struct pthread_descr_struct initial_descr;

pthread_descr __pthread_thread_self(void)
{
    struct pthread_descr_struct *descr = NtCurrentTeb()->pthread_data;
    if (!descr) return &initial_descr;
    return descr;
}
strong_alias(__pthread_thread_self, pthread_thread_self);

/* pthread functions redirection */

struct pthread_functions
{
  pid_t (*ptr_pthread_fork) (struct fork_block *);
  int (*ptr_pthread_attr_destroy) (pthread_attr_t *);
  int (*ptr___pthread_attr_init_2_0) (pthread_attr_t *);
  int (*ptr___pthread_attr_init_2_1) (pthread_attr_t *);
  int (*ptr_pthread_attr_getdetachstate) (const pthread_attr_t *, int *);
  int (*ptr_pthread_attr_setdetachstate) (pthread_attr_t *, int);
  int (*ptr_pthread_attr_getinheritsched) (const pthread_attr_t *, int *);
  int (*ptr_pthread_attr_setinheritsched) (pthread_attr_t *, int);
  int (*ptr_pthread_attr_getschedparam) (const pthread_attr_t *, struct sched_param *);
  int (*ptr_pthread_attr_setschedparam) (pthread_attr_t *, const struct sched_param *);
  int (*ptr_pthread_attr_getschedpolicy) (const pthread_attr_t *, int *);
  int (*ptr_pthread_attr_setschedpolicy) (pthread_attr_t *, int);
  int (*ptr_pthread_attr_getscope) (const pthread_attr_t *, int *);
  int (*ptr_pthread_attr_setscope) (pthread_attr_t *, int);
  int (*ptr_pthread_condattr_destroy) (pthread_condattr_t *);
  int (*ptr_pthread_condattr_init) (pthread_condattr_t *);
  int (*ptr___pthread_cond_broadcast) (pthread_cond_t *);
  int (*ptr___pthread_cond_destroy) (pthread_cond_t *);
  int (*ptr___pthread_cond_init) (pthread_cond_t *, const pthread_condattr_t *);
  int (*ptr___pthread_cond_signal) (pthread_cond_t *);
  int (*ptr___pthread_cond_wait) (pthread_cond_t *, pthread_mutex_t *);
  int (*ptr_pthread_equal) (pthread_t, pthread_t);
  void (*ptr___pthread_exit) (void *);
  int (*ptr_pthread_getschedparam) (pthread_t, int *, struct sched_param *);
  int (*ptr_pthread_setschedparam) (pthread_t, int, const struct sched_param *);
  int (*ptr_pthread_mutex_destroy) (pthread_mutex_t *);
  int (*ptr_pthread_mutex_init) (pthread_mutex_t *, const pthread_mutexattr_t *);
  int (*ptr_pthread_mutex_lock) (pthread_mutex_t *);
  int (*ptr_pthread_mutex_trylock) (pthread_mutex_t *);
  int (*ptr_pthread_mutex_unlock) (pthread_mutex_t *);
  pthread_t (*ptr_pthread_self) (void);
  int (*ptr_pthread_setcancelstate) (int, int *);
  int (*ptr_pthread_setcanceltype) (int, int *);
  void (*ptr_pthread_do_exit) (void *retval, char *currentframe);
  void (*ptr_pthread_cleanup_upto) (jmp_buf target, char *targetframe);
  pthread_descr (*ptr_pthread_thread_self) (void);
  int (*ptr_pthread_internal_tsd_set) (int key, const void *pointer);
  void * (*ptr_pthread_internal_tsd_get) (int key);
  void ** __attribute__ ((__const__)) (*ptr_pthread_internal_tsd_address) (int key);
  int (*ptr_pthread_sigaction) (int sig, const struct sigaction * act, struct sigaction *oact);
  int (*ptr_pthread_sigwait) (const sigset_t *set, int *sig);
  int (*ptr_pthread_raise) (int sig);
};

static struct pthread_functions wine_pthread_functions;
static int init_done;

static pid_t (*libc_fork)(void);
static int (*libc_sigaction)(int signum, const struct sigaction *act, struct sigaction *oldact);
static int (*libc_uselocale)(int set);
static int *(*libc_pthread_init)( const struct pthread_functions *funcs );
static int *libc_multiple_threads;

void PTHREAD_init_done(void)
{
    init_done = 1;
    if (!libc_fork) libc_fork = dlsym( RTLD_NEXT, "fork" );
    if (!libc_sigaction) libc_sigaction = dlsym( RTLD_NEXT, "sigaction" );
}

struct __res_state *__res_state(void)
{
    pthread_descr descr = __pthread_thread_self();
    return &descr->res_state;
}

static inline void writejump( const char *symbol, void *dest )
{
#if defined(__GLIBC__) && defined(__i386__)
    unsigned char *addr = dlsym( RTLD_NEXT, symbol );

    if (!addr) return;

    /* write a relative jump at the function address */
    mprotect((void*)((unsigned int)addr & ~(getpagesize()-1)), 5, PROT_READ|PROT_EXEC|PROT_WRITE);
    addr[0] = 0xe9;
    *(int *)(addr+1) = (unsigned char *)dest - (addr + 5);
    mprotect((void*)((unsigned int)addr & ~(getpagesize()-1)), 5, PROT_READ|PROT_EXEC);

# ifdef HAVE_VALGRIND_MEMCHECK_H
    VALGRIND_DISCARD_TRANSLATIONS( addr, 5 );
# endif
#endif  /* __GLIBC__ && __i386__ */
}

/***********************************************************************
 *           PTHREAD_init_thread
 *
 * Initialization for a newly created thread.
 */
void PTHREAD_init_thread(void)
{
    static int first = 1;

    if (first)
    {
        first = 0;
        NtCurrentTeb()->pthread_data = &initial_descr;
        errno_location_ptr = thread_errno_location;
        h_errno_location_ptr = thread_h_errno_location;
        libc_uselocale = dlsym( RTLD_NEXT, "uselocale" );
        libc_pthread_init = dlsym( RTLD_NEXT, "__libc_pthread_init" );
        if (libc_pthread_init) libc_multiple_threads = libc_pthread_init( &wine_pthread_functions );
        writejump( "__errno_location", thread_errno_location );
        writejump( "__h_errno_location", thread_h_errno_location );
        writejump( "__res_state", __res_state );
    }
    else
    {
        struct pthread_descr_struct *descr = calloc( 1, sizeof(*descr) );
        NtCurrentTeb()->pthread_data = descr;
        if (libc_multiple_threads) *libc_multiple_threads = 1;
    }
    if (libc_uselocale) libc_uselocale( -1 /*LC_GLOBAL_LOCALE*/ );
}

/* redefine this to prevent libpthread from overriding our function pointers */
int *__libc_pthread_init( const struct pthread_functions *funcs )
{
    return libc_multiple_threads;
}

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

typedef struct _wine_cleanup {
  void (*routine)(void *);
  void *arg;
} *wine_cleanup;

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

static CRITICAL_SECTION atfork_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &atfork_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": atfork_section") }
};
static CRITICAL_SECTION atfork_section = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef void (*atfork_handler)();
static atfork_handler atfork_prepare[MAX_ATFORK];
static atfork_handler atfork_parent[MAX_ATFORK];
static atfork_handler atfork_child[MAX_ATFORK];
static int atfork_count;

int __pthread_atfork(void (*prepare)(void),
		     void (*parent)(void),
		     void (*child)(void))
{
    if (init_done) RtlEnterCriticalSection( &atfork_section );
    assert( atfork_count < MAX_ATFORK );
    atfork_prepare[atfork_count] = prepare;
    atfork_parent[atfork_count] = parent;
    atfork_child[atfork_count] = child;
    atfork_count++;
    if (init_done) RtlLeaveCriticalSection( &atfork_section );
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
    RtlEnterCriticalSection( &atfork_section );
    /* prepare handlers are called in reverse insertion order */
    for (i = atfork_count - 1; i >= 0; i--) if (atfork_prepare[i]) atfork_prepare[i]();
    if (!(pid = libc_fork()))
    {
        RtlInitializeCriticalSection( &atfork_section );
        for (i = 0; i < atfork_count; i++) if (atfork_child[i]) atfork_child[i]();
    }
    else
    {
        for (i = 0; i < atfork_count; i++) if (atfork_parent[i]) atfork_parent[i]();
        RtlLeaveCriticalSection( &atfork_section );
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
  RtlInitializeCriticalSection(critsect);

  if (InterlockedCompareExchangePointer((void**)&(((wine_mutex)mutex)->critsect),critsect,NULL) != NULL) {
    /* too late, some other thread already did it */
    RtlDeleteCriticalSection(critsect);
    HeapFree(GetProcessHeap(), 0, critsect);
  }
}

int __pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (!init_done) return 0;
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  RtlEnterCriticalSection(((wine_mutex)mutex)->critsect);
  return 0;
}
strong_alias(__pthread_mutex_lock, pthread_mutex_lock);

int __pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (!init_done) return 0;
  if (!((wine_mutex)mutex)->critsect)
    mutex_real_init( mutex );

  if (!RtlTryEnterCriticalSection(((wine_mutex)mutex)->critsect)) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}
strong_alias(__pthread_mutex_trylock, pthread_mutex_trylock);

int __pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if (!((wine_mutex)mutex)->critsect) return 0;
  RtlLeaveCriticalSection(((wine_mutex)mutex)->critsect);
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
      RtlLeaveCriticalSection(((wine_mutex)mutex)->critsect);
#endif
  }
  RtlDeleteCriticalSection(((wine_mutex)mutex)->critsect);
  HeapFree(GetProcessHeap(), 0, ((wine_mutex)mutex)->critsect);
  ((wine_mutex)mutex)->critsect = NULL;
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
    pthread_descr descr = __pthread_thread_self();
    descr->key_data[key] = pointer;
    return 0;
}
strong_alias(__pthread_setspecific, pthread_setspecific);

void *__pthread_getspecific(pthread_key_t key)
{
    pthread_descr descr = __pthread_thread_self();
    return (void *)descr->key_data[key];
}
strong_alias(__pthread_getspecific, pthread_getspecific);

/* these are not exported, they are only used in the pthread_functions structure */

static int pthread_internal_tsd_set( int key, const void *pointer )
{
    pthread_descr descr = __pthread_thread_self();
    descr->tsd_data[key] = pointer;
    return 0;
}

static void *pthread_internal_tsd_get( int key )
{
    pthread_descr descr = __pthread_thread_self();
    return (void *)descr->tsd_data[key];
}

static void ** __attribute__((const)) pthread_internal_tsd_address( int key )
{
    pthread_descr descr = __pthread_thread_self();
    return (void **)&descr->tsd_data[key];
}

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

void __pthread_cleanup_upto(jmp_buf target, char *frame)
{
    /* FIXME */
}

/***** CONDITIONS *****/
/* not implemented right now */

int __pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
  P_OUTPUT("FIXME:pthread_cond_init\n");
  return 0;
}
strong_alias(__pthread_cond_init, pthread_cond_init);

int __pthread_cond_destroy(pthread_cond_t *cond)
{
  P_OUTPUT("FIXME:pthread_cond_destroy\n");
  return 0;
}
strong_alias(__pthread_cond_destroy, pthread_cond_destroy);

int __pthread_cond_signal(pthread_cond_t *cond)
{
  P_OUTPUT("FIXME:pthread_cond_signal\n");
  return 0;
}
strong_alias(__pthread_cond_signal, pthread_cond_signal);

int __pthread_cond_broadcast(pthread_cond_t *cond)
{
  P_OUTPUT("FIXME:pthread_cond_broadcast\n");
  return 0;
}
strong_alias(__pthread_cond_broadcast, pthread_cond_broadcast);

int __pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  P_OUTPUT("FIXME:pthread_cond_wait\n");
  return 0;
}
strong_alias(__pthread_cond_wait, pthread_cond_wait);

int __pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
  P_OUTPUT("FIXME:pthread_cond_timedwait\n");
  return 0;
}
strong_alias(__pthread_cond_timedwait, pthread_cond_timedwait);

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

void __pthread_do_exit(void *retval, char *currentframe)
{
  /* FIXME: pthread cleanup */
  ExitThread((DWORD)retval);
}

void __pthread_exit(void *retval)
{
    __pthread_do_exit( retval, NULL );
}
strong_alias(__pthread_exit, pthread_exit);

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

static struct pthread_functions wine_pthread_functions =
{
    NULL,                          /* ptr_pthread_fork */
    NULL, /* FIXME */              /* ptr_pthread_attr_destroy */
    NULL, /* FIXME */              /* ptr___pthread_attr_init_2_0 */
    NULL, /* FIXME */              /* ptr___pthread_attr_init_2_1 */
    NULL, /* FIXME */              /* ptr_pthread_attr_getdetachstate */
    NULL, /* FIXME */              /* ptr_pthread_attr_setdetachstate */
    NULL, /* FIXME */              /* ptr_pthread_attr_getinheritsched */
    NULL, /* FIXME */              /* ptr_pthread_attr_setinheritsched */
    NULL, /* FIXME */              /* ptr_pthread_attr_getschedparam */
    pthread_attr_setschedparam,    /* ptr_pthread_attr_setschedparam */
    NULL, /* FIXME */              /* ptr_pthread_attr_getschedpolicy) (const pthread_attr_t *, int *); */
    NULL, /* FIXME */              /* ptr_pthread_attr_setschedpolicy) (pthread_attr_t *, int); */
    NULL, /* FIXME */              /* ptr_pthread_attr_getscope) (const pthread_attr_t *, int *); */
    NULL, /* FIXME */              /* ptr_pthread_attr_setscope) (pthread_attr_t *, int); */
    pthread_condattr_destroy,      /* ptr_pthread_condattr_destroy */
    pthread_condattr_init,         /* ptr_pthread_condattr_init */
    __pthread_cond_broadcast,      /* ptr___pthread_cond_broadcast */
    __pthread_cond_destroy,        /* ptr___pthread_cond_destroy */
    __pthread_cond_init,           /* ptr___pthread_cond_init */
    __pthread_cond_signal,         /* ptr___pthread_cond_signal */
    __pthread_cond_wait,           /* ptr___pthread_cond_wait */
    pthread_equal,                 /* ptr_pthread_equal */
    __pthread_exit,                /* ptr___pthread_exit */
    NULL, /* FIXME */              /* ptr_pthread_getschedparam */
    NULL, /* FIXME */              /* ptr_pthread_setschedparam */
    __pthread_mutex_destroy,       /* ptr_pthread_mutex_destroy */
    __pthread_mutex_init,          /* ptr_pthread_mutex_init */
    __pthread_mutex_lock,          /* ptr_pthread_mutex_lock */
    __pthread_mutex_trylock,       /* ptr_pthread_mutex_trylock */
    __pthread_mutex_unlock,        /* ptr_pthread_mutex_unlock */
    pthread_self,                  /* ptr_pthread_self */
    NULL, /* FIXME */              /* ptr_pthread_setcancelstate */
    pthread_setcanceltype,         /* ptr_pthread_setcanceltype */
    __pthread_do_exit,             /* ptr_pthread_do_exit */
    __pthread_cleanup_upto,        /* ptr_pthread_cleanup_upto */
    __pthread_thread_self,         /* ptr_pthread_thread_self */
    pthread_internal_tsd_set,      /* ptr_pthread_internal_tsd_set */
    pthread_internal_tsd_get,      /* ptr_pthread_internal_tsd_get */
    pthread_internal_tsd_address,  /* ptr_pthread_internal_tsd_address */
    NULL,                          /* ptr_pthread_sigaction */
    NULL,                          /* ptr_pthread_sigwait */
    NULL                           /* ptr_pthread_raise */
};

#endif /* __GLIBC__ || __FREEBSD__ */

#else  /* HAVE_NPTL */

void PTHREAD_init_done(void)
{
}

#endif  /* HAVE_NPTL */
