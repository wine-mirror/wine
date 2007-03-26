/*
 * pthread emulation based on kernel threads
 *
 * We can't use pthreads directly, so why not let libcs
 * that want pthreads use Wine's own threading instead...
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

struct _pthread_cleanup_buffer;

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
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
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#include "wine/library.h"
#include "wine/pthread.h"

#define P_OUTPUT(stuff) write(2,stuff,strlen(stuff))

#define PSTR(str) __ASM_NAME(#str)

static struct wine_pthread_callbacks funcs;

/* thread descriptor */

#define FIRST_KEY 0
#define MAX_KEYS 16 /* libc6 doesn't use that many, but... */
#define MAX_TSD  16

struct pthread_descr_struct
{
    char               dummy[2048];
    int                thread_errno;
    int                thread_h_errno;
    int                cancel_state;
    int                cancel_type;
    struct __res_state res_state;
    const void        *key_data[MAX_KEYS];  /* for normal pthread keys */
    const void        *tsd_data[MAX_TSD];   /* for libc internal tsd variables */
};

typedef struct pthread_descr_struct *pthread_descr;

static struct pthread_descr_struct initial_descr;

pthread_descr __pthread_thread_self(void)
{
    struct pthread_descr_struct *descr;
    if (!funcs.ptr_get_thread_data) return &initial_descr;
    descr = funcs.ptr_get_thread_data();
    if (!descr) return &initial_descr;
    return descr;
}

static int (*libc_uselocale)(int set);
static int *libc_multiple_threads;

/***********************************************************************
 *           __errno_location/__error/__errno/___errno/__thr_errno
 *
 * Get the per-thread errno location.
 */
int *__errno_location(void)                            /* Linux */
{
    pthread_descr descr = __pthread_thread_self();
    return &descr->thread_errno;
}
int *__error(void)     { return __errno_location(); }  /* FreeBSD */
int *__errno(void)     { return __errno_location(); }  /* NetBSD */
int *___errno(void)    { return __errno_location(); }  /* Solaris */
int *__thr_errno(void) { return __errno_location(); }  /* UnixWare */

/***********************************************************************
 *           __h_errno_location
 *
 * Get the per-thread h_errno location.
 */
int *__h_errno_location(void)
{
    pthread_descr descr = __pthread_thread_self();
    return &descr->thread_h_errno;
}

struct __res_state *__res_state(void)
{
    pthread_descr descr = __pthread_thread_self();
    return &descr->res_state;
}

static inline void writejump( const char *symbol, void *dest )
{
#if defined(__GLIBC__) && defined(__i386__)
    unsigned char *addr = wine_dlsym( RTLD_NEXT, symbol, NULL, 0 );

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

/* temporary stacks used on thread exit */
#define TEMP_STACK_SIZE 1024
#define NB_TEMP_STACKS  8
static char temp_stacks[NB_TEMP_STACKS][TEMP_STACK_SIZE];
static int next_temp_stack;  /* next temp stack to use */

/***********************************************************************
 *           get_temp_stack
 *
 * Get a temporary stack address to run the thread exit code on.
 */
static inline char *get_temp_stack(void)
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
    struct wine_pthread_thread_info info = *(struct wine_pthread_thread_info *)ptr;
    wine_ldt_free_fs( info.teb_sel );
    if (info.stack_size) munmap( info.stack_base, info.stack_size );
    if (info.teb_size) munmap( info.teb_base, info.teb_size );
    _exit( info.exit_status );
}


/***********************************************************************
 *           init_process
 *
 * Initialization for a newly created process.
 */
static void init_process( const struct wine_pthread_callbacks *callbacks, size_t size )
{
    memcpy( &funcs, callbacks, min( size, sizeof(funcs) ));
    funcs.ptr_set_thread_data( &initial_descr );
}


/***********************************************************************
 *           init_thread
 *
 * Initialization for a newly created thread.
 */
static void init_thread( struct wine_pthread_thread_info *info )
{
    struct pthread_descr_struct *descr;

    if (funcs.ptr_set_thread_data)
    {
        descr = calloc( 1, sizeof(*descr) );
        funcs.ptr_set_thread_data( descr );
        if (libc_multiple_threads) *libc_multiple_threads = 1;
    }
    else  /* first thread */
    {
        descr = &initial_descr;
        writejump( "__errno_location", __errno_location );
        writejump( "__h_errno_location", __h_errno_location );
        writejump( "__res_state", __res_state );
    }
    descr->cancel_state = PTHREAD_CANCEL_ENABLE;
    descr->cancel_type  = PTHREAD_CANCEL_ASYNCHRONOUS;
    if (libc_uselocale) libc_uselocale( -1 /*LC_GLOBAL_LOCALE*/ );
}


/***********************************************************************
 *           create_thread
 */
static int create_thread( struct wine_pthread_thread_info *info )
{
    if (!info->stack_base)
    {
        info->stack_base = wine_anon_mmap( NULL, info->stack_size, PROT_READ | PROT_WRITE, 0 );
        if (info->stack_base == (void *)-1) return -1;
    }
#ifdef HAVE_CLONE
    if (clone( (int (*)(void *))info->entry, (char *)info->stack_base + info->stack_size,
               CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, info ) < 0)
        return -1;
    return 0;
#elif defined(HAVE_RFORK)
    {
        void **sp = (void **)((char *)info->stack_base + info->stack_size);
        *--sp = info;
        *--sp = 0;
        *--sp = info->entry;
        __asm__ __volatile__(
            "pushl %2;\n\t"             /* flags */
            "pushl $0;\n\t"             /* 0 ? */
            "movl %1,%%eax;\n\t"        /* SYS_rfork */
            ".byte 0x9a; .long 0; .word 7;\n\t" /* lcall 7:0... FreeBSD syscall */
            "cmpl $0, %%edx;\n\t"
            "je 1f;\n\t"
            "movl %0,%%esp;\n\t"        /* child -> new thread */
            "ret;\n"
            "1:\n\t"                    /* parent -> caller thread */
            "addl $8,%%esp" :
            : "r" (sp), "r" (SYS_rfork), "r" (RFPROC | RFMEM | RFTHREAD)
            : "eax", "edx");
        return 0;
    }
#endif
    return -1;
}


/***********************************************************************
 *           init_current_teb
 *
 * Set the current TEB for a new thread.
 */
static void init_current_teb( struct wine_pthread_thread_info *info )
{
    /* On the i386, the current thread is in the %fs register */
    LDT_ENTRY fs_entry;

    wine_ldt_set_base( &fs_entry, info->teb_base );
    wine_ldt_set_limit( &fs_entry, info->teb_size - 1 );
    wine_ldt_set_flags( &fs_entry, WINE_LDT_FLAGS_DATA|WINE_LDT_FLAGS_32BIT );
    wine_ldt_init_fs( info->teb_sel, &fs_entry );

    /* set pid and tid */
    info->pid = getpid();
    info->tid = -1;
}


/***********************************************************************
 *           get_current_teb
 */
static void *get_current_teb(void)
{
    void *ret;
    __asm__( ".byte 0x64\n\tmovl 0x18,%0" : "=r" (ret) );
    return ret;
}


/***********************************************************************
 *           exit_thread
 */
static void DECLSPEC_NORETURN exit_thread( struct wine_pthread_thread_info *info )
{
    wine_switch_to_stack( cleanup_thread, info, get_temp_stack() );
}


/***********************************************************************
 *           abort_thread
 */
static void DECLSPEC_NORETURN abort_thread( long status )
{
    _exit( status );
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
    sigprocmask
};


/* Currently this probably works only for glibc2,
 * which checks for the presence of double-underscore-prepended
 * pthread primitives, and use them if available.
 * If they are not available, the libc defaults to
 * non-threadsafe operation (not good). */

#if defined(HAVE_PTHREAD_H) && (defined(__GLIBC__) || defined(__FreeBSD__))

/* adapt as necessary (a construct like this is used in glibc sources) */
#define strong_alias(orig, alias) \
 asm(".globl " PSTR(alias) "\n" \
     "\t.set " PSTR(alias) "," PSTR(orig))

struct fork_block;

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
  int (*ptr___pthread_cond_timedwait) (pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
  void (*ptr__pthread_cleanup_push) (struct _pthread_cleanup_buffer * buffer, void (*routine)(void *), void * arg);
  void (*ptr__pthread_cleanup_pop) (struct _pthread_cleanup_buffer * buffer, int execute);
};

static pid_t (*libc_fork)(void);
static int (*libc_sigaction)(int signum, const struct sigaction *act, struct sigaction *oldact);
static int *(*libc_pthread_init)( const struct pthread_functions *funcs );

static struct pthread_functions libc_pthread_functions;

strong_alias(__pthread_thread_self, pthread_thread_self);

/* redefine this to prevent libpthread from overriding our function pointers */
int *__libc_pthread_init( const struct pthread_functions *funcs )
{
    return libc_multiple_threads;
}

typedef struct _wine_cleanup {
  void (*routine)(void *);
  void *arg;
} *wine_cleanup;

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void*
        (*start_routine)(void *), void* arg)
{
    assert( funcs.ptr_pthread_create );
    return funcs.ptr_pthread_create( thread, attr, start_routine, arg );
}

int pthread_cancel(pthread_t thread)
{
    assert( funcs.ptr_pthread_cancel );
    return funcs.ptr_pthread_cancel( thread );
}

int pthread_join(pthread_t thread, void **value_ptr)
{
    assert( funcs.ptr_pthread_join );
    return funcs.ptr_pthread_join( thread, value_ptr );
}

int pthread_detach(pthread_t thread)
{
    assert( funcs.ptr_pthread_detach );
    return funcs.ptr_pthread_detach( thread );
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

/* FIXME */
int pthread_attr_setstack(pthread_attr_t *attr, void *addr, size_t size)
{
  return 0; /* return success */
}

int __pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    static pthread_once_t the_once = PTHREAD_ONCE_INIT;
    int once_now;

    memcpy(&once_now,&the_once,sizeof(once_now));
    if (interlocked_cmpxchg((int*)once_control, once_now+1, once_now) == once_now)
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

static pthread_mutex_t atfork_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef void (*atfork_handler)(void);
static atfork_handler atfork_prepare[MAX_ATFORK];
static atfork_handler atfork_parent[MAX_ATFORK];
static atfork_handler atfork_child[MAX_ATFORK];
static int atfork_count;

int __pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    pthread_mutex_lock( &atfork_mutex );
    assert( atfork_count < MAX_ATFORK );
    atfork_prepare[atfork_count] = prepare;
    atfork_parent[atfork_count] = parent;
    atfork_child[atfork_count] = child;
    atfork_count++;
    pthread_mutex_unlock( &atfork_mutex );
    return 0;
}
strong_alias(__pthread_atfork, pthread_atfork);

pid_t __fork(void)
{
    pid_t pid;
    int i;

    if (!libc_fork)
    {
        libc_fork = wine_dlsym( RTLD_NEXT, "fork", NULL, 0 );
        assert( libc_fork );
    }
    pthread_mutex_lock( &atfork_mutex );
    /* prepare handlers are called in reverse insertion order */
    for (i = atfork_count - 1; i >= 0; i--) if (atfork_prepare[i]) atfork_prepare[i]();
    if (!(pid = libc_fork()))
    {
        pthread_mutex_init( &atfork_mutex, NULL );
        for (i = 0; i < atfork_count; i++) if (atfork_child[i]) atfork_child[i]();
    }
    else
    {
        for (i = 0; i < atfork_count; i++) if (atfork_parent[i]) atfork_parent[i]();
        pthread_mutex_unlock( &atfork_mutex );
    }
    return pid;
}
strong_alias(__fork, fork);

/***** MUTEXES *****/

int __pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
    if (!funcs.ptr_pthread_mutex_init) return 0;
    return funcs.ptr_pthread_mutex_init( mutex, mutexattr );
}
strong_alias(__pthread_mutex_init, pthread_mutex_init);

int __pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (!funcs.ptr_pthread_mutex_lock) return 0;
    return funcs.ptr_pthread_mutex_lock( mutex );
}
strong_alias(__pthread_mutex_lock, pthread_mutex_lock);

int __pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (!funcs.ptr_pthread_mutex_trylock) return 0;
    return funcs.ptr_pthread_mutex_trylock( mutex );
}
strong_alias(__pthread_mutex_trylock, pthread_mutex_trylock);

int __pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!funcs.ptr_pthread_mutex_unlock) return 0;
    return funcs.ptr_pthread_mutex_unlock( mutex );
}
strong_alias(__pthread_mutex_unlock, pthread_mutex_unlock);

int __pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!funcs.ptr_pthread_mutex_destroy) return 0;
    return funcs.ptr_pthread_mutex_destroy( mutex );
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

int __pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *kind)
{
  *kind = PTHREAD_MUTEX_RECURSIVE;
  return 0;
}
strong_alias(__pthread_mutexattr_gettype, pthread_mutexattr_gettype);


/***** THREAD-SPECIFIC VARIABLES (KEYS) *****/

int __pthread_key_create(pthread_key_t *key, void (*destr_function)(void *))
{
    static int keycnt = FIRST_KEY;
    *key = interlocked_xchg_add(&keycnt, 1);
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

static int pthread_internal_tsd_set( int key, const void *pointer )
{
    pthread_descr descr = __pthread_thread_self();
    descr->tsd_data[key] = pointer;
    return 0;
}
int (*__libc_internal_tsd_set)(int, const void *) = pthread_internal_tsd_set;

static void *pthread_internal_tsd_get( int key )
{
    pthread_descr descr = __pthread_thread_self();
    return (void *)descr->tsd_data[key];
}
void* (*__libc_internal_tsd_get)(int) = pthread_internal_tsd_get;

static void ** __attribute__((const)) pthread_internal_tsd_address( int key )
{
    pthread_descr descr = __pthread_thread_self();
    return (void **)&descr->tsd_data[key];
}
void** (*__libc_internal_tsd_address)(int) = pthread_internal_tsd_address;

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

int __pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
    if (!funcs.ptr_pthread_cond_init) return 0;
    return funcs.ptr_pthread_cond_init(cond, cond_attr);
}
strong_alias(__pthread_cond_init, pthread_cond_init);

int __pthread_cond_destroy(pthread_cond_t *cond)
{
    if (!funcs.ptr_pthread_cond_destroy) return 0;
    return funcs.ptr_pthread_cond_destroy(cond);
}
strong_alias(__pthread_cond_destroy, pthread_cond_destroy);

int __pthread_cond_signal(pthread_cond_t *cond)
{
    if (!funcs.ptr_pthread_cond_signal) return 0;
    return funcs.ptr_pthread_cond_signal(cond);
}
strong_alias(__pthread_cond_signal, pthread_cond_signal);

int __pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (!funcs.ptr_pthread_cond_broadcast) return 0;
    return funcs.ptr_pthread_cond_broadcast(cond);
}
strong_alias(__pthread_cond_broadcast, pthread_cond_broadcast);

int __pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (!funcs.ptr_pthread_cond_wait) return 0;
    return funcs.ptr_pthread_cond_wait(cond, mutex);
}
strong_alias(__pthread_cond_wait, pthread_cond_wait);

int __pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    if (!funcs.ptr_pthread_cond_timedwait) return 0;
    return funcs.ptr_pthread_cond_timedwait(cond, mutex, abstime);
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

/***** READ-WRITE LOCKS *****/

int __pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *rwlock_attr)
{
    assert( funcs.ptr_pthread_rwlock_init );
    return funcs.ptr_pthread_rwlock_init( rwlock, rwlock_attr );
}
strong_alias(__pthread_rwlock_init, pthread_rwlock_init);

int __pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    assert( funcs.ptr_pthread_rwlock_destroy );
    return funcs.ptr_pthread_rwlock_destroy( rwlock );
}
strong_alias(__pthread_rwlock_destroy, pthread_rwlock_destroy);

int __pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    if (!funcs.ptr_pthread_rwlock_rdlock) return 0;
    return funcs.ptr_pthread_rwlock_rdlock( rwlock );
}
strong_alias(__pthread_rwlock_rdlock, pthread_rwlock_rdlock);

int __pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    assert( funcs.ptr_pthread_rwlock_tryrdlock );
    return funcs.ptr_pthread_rwlock_tryrdlock( rwlock );
}
strong_alias(__pthread_rwlock_tryrdlock, pthread_rwlock_tryrdlock);

int __pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    assert( funcs.ptr_pthread_rwlock_wrlock );
    return funcs.ptr_pthread_rwlock_wrlock( rwlock );
}
strong_alias(__pthread_rwlock_wrlock, pthread_rwlock_wrlock);

int __pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
    assert( funcs.ptr_pthread_rwlock_trywrlock );
    return funcs.ptr_pthread_rwlock_trywrlock( rwlock );
}
strong_alias(__pthread_rwlock_trywrlock, pthread_rwlock_trywrlock);

int __pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    if (!funcs.ptr_pthread_rwlock_unlock) return 0;
    return funcs.ptr_pthread_rwlock_unlock( rwlock );
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

/***** MISC *****/

pthread_t pthread_self(void)
{
    assert( funcs.ptr_pthread_self );
    return funcs.ptr_pthread_self();
}

int pthread_equal(pthread_t thread1, pthread_t thread2)
{
    assert( funcs.ptr_pthread_equal );
    return funcs.ptr_pthread_equal( thread1, thread2 );
}

void __pthread_do_exit(void *retval, char *currentframe)
{
    assert( funcs.ptr_pthread_exit );
    return funcs.ptr_pthread_exit( retval, currentframe );
}

void __pthread_exit(void *retval)
{
    __pthread_do_exit( retval, NULL );
}
strong_alias(__pthread_exit, pthread_exit);

int pthread_setcancelstate(int state, int *oldstate)
{
    pthread_descr descr = __pthread_thread_self();
    if (oldstate) *oldstate = descr->cancel_state;
    descr->cancel_state = state;
    return 0;
}

int pthread_setcanceltype(int type, int *oldtype)
{
    pthread_descr descr = __pthread_thread_self();
    if (oldtype) *oldtype = descr->cancel_type;
    descr->cancel_type = type;
    return 0;
}

/***** ANTI-OVERRIDES *****/
/* pthreads tries to override these, point them back to libc */

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    if (!libc_sigaction)
    {
        libc_sigaction = wine_dlsym( RTLD_NEXT, "sigaction", NULL, 0 );
        assert( libc_sigaction );
    }
    return libc_sigaction(signum, act, oldact);
}

void __pthread_initialize(void)
{
    static int done;

    if (!done)
    {
        done = 1;
        /* check for exported epoll_create to detect glibc versions that we cannot support */
        if (wine_dlsym( RTLD_DEFAULT, "epoll_create", NULL, 0 ))
        {
            static const char warning[] =
                "wine: glibc >= 2.3 without NPTL or TLS is not a supported combination.\n"
                "      It will most likely crash. Please upgrade to a glibc with NPTL support.\n";
            write( 2, warning, sizeof(warning)-1 );
        }
        libc_fork = wine_dlsym( RTLD_NEXT, "fork", NULL, 0 );
        libc_sigaction = wine_dlsym( RTLD_NEXT, "sigaction", NULL, 0 );
        libc_uselocale = wine_dlsym( RTLD_DEFAULT, "uselocale", NULL, 0 );
        libc_pthread_init = wine_dlsym( RTLD_NEXT, "__libc_pthread_init", NULL, 0 );
        if (libc_pthread_init) libc_multiple_threads = libc_pthread_init( &libc_pthread_functions );
    }
}

#ifdef __GNUC__
static void init(void) __attribute__((constructor));
static void init(void)
{
    __pthread_initialize();
}
#endif

static struct pthread_functions libc_pthread_functions =
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
    NULL, /* FIXME */              /* ptr_pthread_attr_getschedpolicy */
    NULL, /* FIXME */              /* ptr_pthread_attr_setschedpolicy */
    NULL, /* FIXME */              /* ptr_pthread_attr_getscope */
    NULL, /* FIXME */              /* ptr_pthread_attr_setscope */
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
    pthread_setcancelstate,        /* ptr_pthread_setcancelstate */
    pthread_setcanceltype,         /* ptr_pthread_setcanceltype */
    __pthread_do_exit,             /* ptr_pthread_do_exit */
    __pthread_cleanup_upto,        /* ptr_pthread_cleanup_upto */
    __pthread_thread_self,         /* ptr_pthread_thread_self */
    pthread_internal_tsd_set,      /* ptr_pthread_internal_tsd_set */
    pthread_internal_tsd_get,      /* ptr_pthread_internal_tsd_get */
    pthread_internal_tsd_address,  /* ptr_pthread_internal_tsd_address */
    NULL,                          /* ptr_pthread_sigaction */
    NULL,                          /* ptr_pthread_sigwait */
    NULL,                          /* ptr_pthread_raise */
    __pthread_cond_timedwait,      /* ptr___pthread_cond_timedwait */
    _pthread_cleanup_push,         /* ptr__pthread_cleanup_push */
    _pthread_cleanup_pop           /* ptr__pthread_cleanup_pop */
};

#endif /* HAVE_PTHREAD_H && (__GLIBC__ || __FREEBSD__) */
