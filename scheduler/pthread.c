/*
 * pthread emulation for re-entrant libcs
 *
 * We can't use pthreads directly, so why not let libcs
 * that wants pthreads use Wine's own threading instead...
 *
 * Copyright 1999 Ove Kåven
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "winbase.h"
#include "heap.h"
#include "thread.h"

/* Currently this probably works only for glibc2,
 * which checks for the presence of double-underscore-prepended
 * pthread primitives, and use them if available.
 * If they are not available, the libc defaults to
 * non-threadsafe operation (not good). */

#if defined(__GLIBC__)
#include <pthread.h>
#include <signal.h>

#ifdef NEED_UNDERSCORE_PREFIX
# define PREFIX "_"
#else
# define PREFIX
#endif

#define PSTR(str) PREFIX #str

/* adapt as necessary (a construct like this is used in glibc sources) */
#define strong_alias(orig, alias) \
 asm(".globl " PSTR(alias) "\n\t.set " PSTR(alias) "," PSTR(orig))

/* strong_alias does not work on external symbols (.o format limitation?),
 * so for those, we need to use the pogo stick */
#ifdef __i386__
#define jump_alias(orig, alias) \
 asm(".globl " PSTR(alias) "\n\t" PSTR(alias) ":\n\tjmp " PSTR(orig))
#endif

/* get necessary libc symbols */
#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1) && defined(HAVE___LIBC_FORK)
#define LIBC_FORK __libc_fork
#define PTHREAD_FORK __fork
#define ALIAS_FORK
#else
#define LIBC_FORK __fork
#define PTHREAD_FORK fork
#endif
extern pid_t LIBC_FORK(void);

#define LIBC_SIGACTION __sigaction

/* NOTE: This is a truly extremely incredibly ugly hack!
 * But it does seem to work... */

/* assume that pthread_mutex_t has room for at least one pointer,
 * and hope that the users of pthread_mutex_t considers it opaque
 * (never checks what's in it) */
typedef struct {
  CRITICAL_SECTION *critsect;
} *wine_mutex;

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

int __pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
  static pthread_once_t the_once = PTHREAD_ONCE_INIT;
  LONG once_now = *(LONG *)&the_once;

  if (InterlockedCompareExchange((PVOID*)once_control, (PVOID)(once_now+1), (PVOID)once_now) == (PVOID)once_now)
    (*init_routine)();
  return 0;
}
strong_alias(__pthread_once, pthread_once);

void __pthread_kill_other_threads_np(void)
{
  /* FIXME: this is supposed to be preparation for exec() */
  if (SystemHeap) P_OUTPUT("fixme:pthread_kill_other_threads_np\n");
}
strong_alias(__pthread_kill_other_threads_np, pthread_kill_other_threads_np);

/***** atfork *****/

#define MAX_ATFORK 8  /* libc doesn't need that many anyway */

static CRITICAL_SECTION atfork_section = CRITICAL_SECTION_INIT;
typedef void (*atfork_handler)();
static atfork_handler atfork_prepare[MAX_ATFORK];
static atfork_handler atfork_parent[MAX_ATFORK];
static atfork_handler atfork_child[MAX_ATFORK];
static int atfork_count;

int __pthread_atfork(void (*prepare)(void),
		     void (*parent)(void),
		     void (*child)(void))
{
    EnterCriticalSection( &atfork_section );
    assert( atfork_count < MAX_ATFORK );
    atfork_prepare[atfork_count] = prepare;
    atfork_parent[atfork_count] = parent;
    atfork_child[atfork_count] = child;
    atfork_count++;
    LeaveCriticalSection( &atfork_section );
    return 0;
}
strong_alias(__pthread_atfork, pthread_atfork);

pid_t PTHREAD_FORK(void)
{
    pid_t pid;
    int i;

    EnterCriticalSection( &atfork_section );
    /* prepare handlers are called in reverse insertion order */
    for (i = atfork_count - 1; i >= 0; i--) atfork_prepare[i]();
    if (!(pid = LIBC_FORK()))
    {
        InitializeCriticalSection( &atfork_section );
        for (i = 0; i < atfork_count; i++) atfork_child[i]();
    }
    else
    {
        for (i = 0; i < atfork_count; i++) atfork_parent[i]();
        LeaveCriticalSection( &atfork_section );
    }
    return pid;
}
#ifdef ALIAS_FORK
strong_alias(PTHREAD_FORK, fork);
#endif

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
  CRITICAL_SECTION *critsect = HeapAlloc(SystemHeap, 0, sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(critsect);

  if (InterlockedCompareExchange((PVOID*)&(((wine_mutex)mutex)->critsect),critsect,NULL) != NULL) {
    /* too late, some other thread already did it */
    DeleteCriticalSection(critsect);
    HeapFree(SystemHeap, 0, critsect);
  }
}

int __pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (!SystemHeap) return 0;
  if (!((wine_mutex)mutex)->critsect) 
    mutex_real_init( mutex );

  EnterCriticalSection(((wine_mutex)mutex)->critsect);
  return 0;
}
strong_alias(__pthread_mutex_lock, pthread_mutex_lock);

int __pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (!SystemHeap) return 0;
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
  HeapFree(SystemHeap, 0, ((wine_mutex)mutex)->critsect);
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
  *kind = PTHREAD_MUTEX_RECURSIVE_NP;
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
  *kind = PTHREAD_MUTEX_RECURSIVE_NP;
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

#ifdef jump_alias
jump_alias(LIBC_SIGACTION, sigaction);
#else
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
  return LIBC_SIGACTION(signum, act, oldact);
}
#endif

#endif /* __GLIBC__ */
