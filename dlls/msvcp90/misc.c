/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <stdarg.h>
#include <limits.h>

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

struct __Container_proxy;

typedef struct {
    struct __Container_proxy *proxy;
} _Container_base12;

typedef struct __Iterator_base12 {
    struct __Container_proxy *proxy;
    struct __Iterator_base12 *next;
} _Iterator_base12;

typedef struct __Container_proxy {
    const _Container_base12 *cont;
    _Iterator_base12 *head;
} _Container_proxy;

/* ??0_Mutex@std@@QAE@XZ */
/* ??0_Mutex@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(mutex_ctor, 4)
mutex* __thiscall mutex_ctor(mutex *this)
{
    CRITICAL_SECTION *cs = MSVCRT_operator_new(sizeof(*cs));
    if(!cs) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }

    InitializeCriticalSection(cs);
    cs->DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": _Mutex critical section");
    this->mutex = cs;
    return this;
}

/* ??1_Mutex@std@@QAE@XZ */
/* ??1_Mutex@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(mutex_dtor, 4)
void __thiscall mutex_dtor(mutex *this)
{
    ((CRITICAL_SECTION*)this->mutex)->DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(this->mutex);
    MSVCRT_operator_delete(this->mutex);
}

/* ?_Lock@_Mutex@std@@QAEXXZ */
/* ?_Lock@_Mutex@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(mutex_lock, 4)
void __thiscall mutex_lock(mutex *this)
{
    EnterCriticalSection(this->mutex);
}

/* ?_Unlock@_Mutex@std@@QAEXXZ */
/* ?_Unlock@_Mutex@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(mutex_unlock, 4)
void __thiscall mutex_unlock(mutex *this)
{
    LeaveCriticalSection(this->mutex);
}

/* ?_Mutex_Lock@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_Lock@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_lock(mutex *m)
{
    mutex_lock(m);
}

/* ?_Mutex_Unlock@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_Unlock@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_unlock(mutex *m)
{
    mutex_unlock(m);
}

/* ?_Mutex_ctor@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_ctor@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_ctor(mutex *m)
{
    mutex_ctor(m);
}

/* ?_Mutex_dtor@_Mutex@std@@CAXPAV12@@Z */
/* ?_Mutex_dtor@_Mutex@std@@CAXPEAV12@@Z */
void CDECL mutex_mutex_dtor(mutex *m)
{
    mutex_dtor(m);
}

static CRITICAL_SECTION lockit_cs[_MAX_LOCK];

#if _MSVCP_VER >= 70
static inline int get_locktype( _Lockit *lockit ) { return lockit->locktype; }
static inline void set_locktype( _Lockit *lockit, int type ) { lockit->locktype = type; }
#else
static inline int get_locktype( _Lockit *lockit ) { return 0; }
static inline void set_locktype( _Lockit *lockit, int type ) { }
#endif

/* ?_Lockit_ctor@_Lockit@std@@SAXH@Z */
void __cdecl _Lockit_init(int locktype) {
    InitializeCriticalSection(&lockit_cs[locktype]);
    lockit_cs[locktype].DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": _Lockit critical section");
}

/* ?_Lockit_dtor@_Lockit@std@@SAXH@Z */
void __cdecl _Lockit_free(int locktype)
{
    lockit_cs[locktype].DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&lockit_cs[locktype]);
}

void init_lockit(void) {
    int i;

    for(i=0; i<_MAX_LOCK; i++)
        _Lockit_init(i);
}

void free_lockit(void) {
    int i;

    for(i=0; i<_MAX_LOCK; i++)
        _Lockit_free(i);
}

/* ?_Lockit_ctor@_Lockit@std@@CAXPAV12@H@Z */
/* ?_Lockit_ctor@_Lockit@std@@CAXPEAV12@H@Z */
void __cdecl _Lockit__Lockit_ctor_locktype(_Lockit *lockit, int locktype)
{
    set_locktype( lockit, locktype );
    EnterCriticalSection(&lockit_cs[locktype]);
}

/* ?_Lockit_ctor@_Lockit@std@@CAXPAV12@@Z */
/* ?_Lockit_ctor@_Lockit@std@@CAXPEAV12@@Z */
void __cdecl _Lockit__Lockit_ctor(_Lockit *lockit)
{
    _Lockit__Lockit_ctor_locktype(lockit, 0);
}

/* ??0_Lockit@std@@QAE@H@Z */
/* ??0_Lockit@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(_Lockit_ctor_locktype, 8)
_Lockit* __thiscall _Lockit_ctor_locktype(_Lockit *this, int locktype)
{
    _Lockit__Lockit_ctor_locktype(this, locktype);
    return this;
}

/* ??0_Lockit@std@@QAE@XZ */
/* ??0_Lockit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Lockit_ctor, 4)
_Lockit* __thiscall _Lockit_ctor(_Lockit *this)
{
    _Lockit__Lockit_ctor_locktype(this, 0);
    return this;
}

/* ?_Lockit_dtor@_Lockit@std@@CAXPAV12@@Z */
/* ?_Lockit_dtor@_Lockit@std@@CAXPEAV12@@Z */
void __cdecl _Lockit__Lockit_dtor(_Lockit *lockit)
{
    LeaveCriticalSection(&lockit_cs[get_locktype( lockit )]);
}

/* ??1_Lockit@std@@QAE@XZ */
/* ??1_Lockit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Lockit_dtor, 4)
void __thiscall _Lockit_dtor(_Lockit *this)
{
    _Lockit__Lockit_dtor(this);
}

/* wctype */
unsigned short __cdecl wctype(const char *property)
{
    static const struct {
        const char *name;
        unsigned short mask;
    } properties[] = {
        { "alnum", _DIGIT|_ALPHA },
        { "alpha", _ALPHA },
        { "cntrl", _CONTROL },
        { "digit", _DIGIT },
        { "graph", _DIGIT|_PUNCT|_ALPHA },
        { "lower", _LOWER },
        { "print", _DIGIT|_PUNCT|_BLANK|_ALPHA },
        { "punct", _PUNCT },
        { "space", _SPACE },
        { "upper", _UPPER },
        { "xdigit", _HEX }
    };
    unsigned int i;

    for(i=0; i<sizeof(properties)/sizeof(properties[0]); i++)
        if(!strcmp(property, properties[i].name))
            return properties[i].mask;

    return 0;
}

typedef void (__cdecl *MSVCP_new_handler_func)(void);
static MSVCP_new_handler_func MSVCP_new_handler;
static int __cdecl new_handler_wrapper(MSVCP_size_t unused)
{
    MSVCP_new_handler();
    return 1;
}

/* ?set_new_handler@std@@YAP6AXXZP6AXXZ@Z */
MSVCP_new_handler_func __cdecl set_new_handler(MSVCP_new_handler_func new_handler)
{
    MSVCP_new_handler_func old_handler = MSVCP_new_handler;

    TRACE("%p\n", new_handler);

    MSVCP_new_handler = new_handler;
    MSVCRT_set_new_handler(new_handler ? new_handler_wrapper : NULL);
    return old_handler;
}

/* ?set_new_handler@std@@YAP6AXXZH@Z */
MSVCP_new_handler_func __cdecl set_new_handler_reset(int unused)
{
    return set_new_handler(NULL);
}

/* _Container_base0 is used by apps compiled without iterator checking
 * (i.e. with _ITERATOR_DEBUG_LEVEL=0 ).
 * It provides empty versions of methods used by visual c++'s stl's
 * iterator checking.
 * msvcr100 has to provide them in case apps are compiled with /Od
 * or the optimizer fails to inline those (empty) calls.
 */

/* ?_Orphan_all@_Container_base0@std@@QAEXXZ */
/* ?_Orphan_all@_Container_base0@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(Container_base0_Orphan_all, 4)
void __thiscall Container_base0_Orphan_all(void *this)
{
}

/* ?_Swap_all@_Container_base0@std@@QAEXAAU12@@Z */
/* ?_Swap_all@_Container_base0@std@@QEAAXAEAU12@@Z */
DEFINE_THISCALL_WRAPPER(Container_base0_Swap_all, 8)
void __thiscall Container_base0_Swap_all(void *this, void *that)
{
}

/* ??4_Container_base0@std@@QAEAAU01@ABU01@@Z */
/* ??4_Container_base0@std@@QEAAAEAU01@AEBU01@@Z */
DEFINE_THISCALL_WRAPPER(Container_base0_op_assign, 8)
void* __thiscall Container_base0_op_assign(void *this, const void *that)
{
    return this;
}

/* ??0_Container_base12@std@@QAE@ABU01@@Z */
/* ??0_Container_base12@std@@QEAA@AEBU01@@Z */
DEFINE_THISCALL_WRAPPER(_Container_base12_copy_ctor, 8)
_Container_base12* __thiscall _Container_base12_copy_ctor(
        _Container_base12 *this, _Container_base12 *that)
{
    this->proxy = NULL;
    return this;
}

/* ??0_Container_base12@std@@QAE@XZ */
/* ??0_Container_base12@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Container_base12_ctor, 4)
_Container_base12* __thiscall _Container_base12_ctor(_Container_base12 *this)
{
    this->proxy = NULL;
    return this;
}

/* ??1_Container_base12@std@@QAE@XZ */
/* ??1_Container_base12@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Container_base12_dtor, 4)
void __thiscall _Container_base12_dtor(_Container_base12 *this)
{
}

/* ??4_Container_base12@std@@QAEAAU01@ABU01@@Z */
/* ??4_Container_base12@std@@QEAAAEAU01@AEBU01@@ */
DEFINE_THISCALL_WRAPPER(_Container_base12_op_assign, 8)
_Container_base12* __thiscall _Container_base12_op_assign(
        _Container_base12 *this, const _Container_base12 *that)
{
    return this;
}

/* ?_Getpfirst@_Container_base12@std@@QBEPAPAU_Iterator_base12@2@XZ */
/* ?_Getpfirst@_Container_base12@std@@QEBAPEAPEAU_Iterator_base12@2@XZ */
DEFINE_THISCALL_WRAPPER(_Container_base12__Getpfirst, 4)
_Iterator_base12** __thiscall _Container_base12__Getpfirst(_Container_base12 *this)
{
    return this->proxy ? &this->proxy->head : NULL;
}

/* ?_Orphan_all@_Container_base12@std@@QAEXXZ */
/* ?_Orphan_all@_Container_base12@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Container_base12__Orphan_all, 4)
void __thiscall _Container_base12__Orphan_all(_Container_base12 *this)
{
}

/* ?_Swap_all@_Container_base12@std@@QAEXAAU12@@Z */
/* ?_Swap_all@_Container_base12@std@@QEAAXAEAU12@@Z */
DEFINE_THISCALL_WRAPPER(_Container_base12__Swap_all, 8)
void __thiscall _Container_base12__Swap_all(
        _Container_base12 *this, _Container_base12 *that)
{
    _Container_proxy *tmp;

    tmp = this->proxy;
    this->proxy = that->proxy;
    that->proxy = tmp;

    if(this->proxy)
        this->proxy->cont = this;
    if(that->proxy)
        that->proxy->cont = that;
}

#if _MSVCP_VER >= 110

#define SECSPERDAY 86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970 ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC 10000000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)
#define NANOSEC_PER_MILLISEC 1000000
#define MILLISEC_PER_SEC 1000

typedef int MSVCRT_long;

/* xtime */
typedef struct {
    __time64_t sec;
    MSVCRT_long nsec;
} xtime;

/* _Xtime_get_ticks */
LONGLONG __cdecl _Xtime_get_ticks(void)
{
    FILETIME ft;

    TRACE("\n");

    GetSystemTimeAsFileTime(&ft);
    return ((LONGLONG)ft.dwHighDateTime<<32) + ft.dwLowDateTime - TICKS_1601_TO_1970;
}

/* _xtime_get */
int __cdecl xtime_get(xtime* t, int unknown)
{
    LONGLONG ticks;

    TRACE("(%p)\n", t);

    if(unknown != 1)
        return 0;

    ticks = _Xtime_get_ticks();
    t->sec = ticks / TICKSPERSEC;
    t->nsec = ticks % TICKSPERSEC * 100;
    return 1;
}

/* _Xtime_diff_to_millis2 */
MSVCRT_long __cdecl _Xtime_diff_to_millis2(const xtime *t1, const xtime *t2)
{
    __time64_t diff_sec;
    MSVCRT_long diff_nsec, ret;

    TRACE("(%p, %p)\n", t1, t2);

    diff_sec = t1->sec - t2->sec;
    diff_nsec = t1->nsec - t2->nsec;
    ret = diff_sec * MILLISEC_PER_SEC +
            (diff_nsec + NANOSEC_PER_MILLISEC - 1) / NANOSEC_PER_MILLISEC;
    return ret > 0 ? ret : 0;
}

/* _Xtime_diff_to_millis */
MSVCRT_long __cdecl _Xtime_diff_to_millis(const xtime *t)
{
    xtime now;

    TRACE("%p\n", t);

    xtime_get(&now, 1);
    return _Xtime_diff_to_millis2(t, &now);
}
#endif

#if _MSVCP_VER >= 90
unsigned int __cdecl _Random_device(void)
{
    unsigned int ret;

    TRACE("\n");

    /* TODO: throw correct exception in case of failure */
    if(rand_s(&ret))
        throw_exception(EXCEPTION, "random number generator failed\n");
    return ret;
}
#endif

#if _MSVCP_VER >= 110
#if defined(__i386__) && !defined(__arm__)

extern void *call_thiscall_func;
__ASM_GLOBAL_FUNC(call_thiscall_func,
        "popl %eax\n\t"
        "popl %edx\n\t"
        "popl %ecx\n\t"
        "pushl %eax\n\t"
        "jmp *%edx\n\t")

#define call_func1(func,this) ((void* (WINAPI*)(void*,void*))&call_thiscall_func)(func,this)

#else /* __i386__ */

#define call_func1(func,this) func(this)

#endif /* __i386__ */

#define MTX_MULTI_LOCK 0x100
#define MTX_LOCKED 3
typedef struct
{
    DWORD flags;
    critical_section cs;
    DWORD thread_id;
    DWORD count;
} *_Mtx_t;

#if _MSVCP_VER >= 140
typedef _Mtx_t _Mtx_arg_t;
#define MTX_T_FROM_ARG(m)   (m)
#define MTX_T_TO_ARG(m)     (m)
#else
typedef _Mtx_t *_Mtx_arg_t;
#define MTX_T_FROM_ARG(m)   (*(m))
#define MTX_T_TO_ARG(m)     (&(m))
#endif

void __cdecl _Mtx_init_in_situ(_Mtx_t mtx, int flags)
{
    if(flags & ~MTX_MULTI_LOCK)
        FIXME("unknown flags ignored: %x\n", flags);

    mtx->flags = flags;
    call_func1(critical_section_ctor, &mtx->cs);
    mtx->thread_id = -1;
    mtx->count = 0;
}

int __cdecl _Mtx_init(_Mtx_t *mtx, int flags)
{
    *mtx = MSVCRT_operator_new(sizeof(**mtx));
    _Mtx_init_in_situ(*mtx, flags);
    return 0;
}

void __cdecl _Mtx_destroy_in_situ(_Mtx_t mtx)
{
    call_func1(critical_section_dtor, &mtx->cs);
}

void __cdecl _Mtx_destroy(_Mtx_arg_t mtx)
{
    call_func1(critical_section_dtor, &MTX_T_FROM_ARG(mtx)->cs);
    MSVCRT_operator_delete(MTX_T_FROM_ARG(mtx));
}

int __cdecl _Mtx_current_owns(_Mtx_arg_t mtx)
{
    return MTX_T_FROM_ARG(mtx)->thread_id == GetCurrentThreadId();
}

int __cdecl _Mtx_lock(_Mtx_arg_t mtx)
{
    if(MTX_T_FROM_ARG(mtx)->thread_id != GetCurrentThreadId()) {
        call_func1(critical_section_lock, &MTX_T_FROM_ARG(mtx)->cs);
        MTX_T_FROM_ARG(mtx)->thread_id = GetCurrentThreadId();
    }else if(!(MTX_T_FROM_ARG(mtx)->flags & MTX_MULTI_LOCK)) {
        return MTX_LOCKED;
    }

    MTX_T_FROM_ARG(mtx)->count++;
    return 0;
}

int __cdecl _Mtx_unlock(_Mtx_arg_t mtx)
{
    if(--MTX_T_FROM_ARG(mtx)->count)
        return 0;

    MTX_T_FROM_ARG(mtx)->thread_id = -1;
    call_func1(critical_section_unlock, &MTX_T_FROM_ARG(mtx)->cs);
    return 0;
}

int __cdecl _Mtx_trylock(_Mtx_arg_t mtx)
{
    if(MTX_T_FROM_ARG(mtx)->thread_id != GetCurrentThreadId()) {
        if(!call_func1(critical_section_trylock, &MTX_T_FROM_ARG(mtx)->cs))
            return MTX_LOCKED;
        MTX_T_FROM_ARG(mtx)->thread_id = GetCurrentThreadId();
    }else if(!(MTX_T_FROM_ARG(mtx)->flags & MTX_MULTI_LOCK)) {
        return MTX_LOCKED;
    }

    MTX_T_FROM_ARG(mtx)->count++;
    return 0;
}

critical_section* __cdecl _Mtx_getconcrtcs(_Mtx_arg_t mtx)
{
    return &MTX_T_FROM_ARG(mtx)->cs;
}

static inline LONG interlocked_dec_if_nonzero( LONG *dest )
{
    LONG val, tmp;
    for (val = *dest;; val = tmp)
    {
        if (!val || (tmp = InterlockedCompareExchange( dest, val - 1, val )) == val)
            break;
    }
    return val;
}

#define CND_TIMEDOUT 2

typedef struct
{
    CONDITION_VARIABLE cv;
} *_Cnd_t;

#if _MSVCP_VER >= 140
typedef _Cnd_t _Cnd_arg_t;
#define CND_T_FROM_ARG(c)   (c)
#define CND_T_TO_ARG(c)     (c)
#else
typedef _Cnd_t *_Cnd_arg_t;
#define CND_T_FROM_ARG(c)   (*(c))
#define CND_T_TO_ARG(c)     (&(c))
#endif

static HANDLE keyed_event;

void __cdecl _Cnd_init_in_situ(_Cnd_t cnd)
{
    InitializeConditionVariable(&cnd->cv);

    if(!keyed_event) {
        HANDLE event;

        NtCreateKeyedEvent(&event, GENERIC_READ|GENERIC_WRITE, NULL, 0);
        if(InterlockedCompareExchangePointer(&keyed_event, event, NULL) != NULL)
            NtClose(event);
    }
}

int __cdecl _Cnd_init(_Cnd_t *cnd)
{
    *cnd = MSVCRT_operator_new(sizeof(**cnd));
    _Cnd_init_in_situ(*cnd);
    return 0;
}

int __cdecl _Cnd_wait(_Cnd_arg_t cnd, _Mtx_arg_t mtx)
{
    CONDITION_VARIABLE *cv = &CND_T_FROM_ARG(cnd)->cv;

    InterlockedExchangeAdd( (LONG *)&cv->Ptr, 1 );
    _Mtx_unlock(mtx);

    NtWaitForKeyedEvent(keyed_event, &cv->Ptr, FALSE, NULL);

    _Mtx_lock(mtx);
    return 0;
}

int __cdecl _Cnd_timedwait(_Cnd_arg_t cnd, _Mtx_arg_t mtx, const xtime *xt)
{
    CONDITION_VARIABLE *cv = &CND_T_FROM_ARG(cnd)->cv;
    LARGE_INTEGER timeout;
    NTSTATUS status;

    InterlockedExchangeAdd( (LONG *)&cv->Ptr, 1 );
    _Mtx_unlock(mtx);

    timeout.QuadPart = (ULONGLONG)_Xtime_diff_to_millis(xt) * -10000;
    status = NtWaitForKeyedEvent(keyed_event, &cv->Ptr, FALSE, &timeout);
    if (status)
    {
        if (!interlocked_dec_if_nonzero( (LONG *)&cv->Ptr ))
            status = NtWaitForKeyedEvent( keyed_event, &cv->Ptr, FALSE, NULL );
    }

    _Mtx_lock(mtx);
    return status ? CND_TIMEDOUT : 0;
}

int __cdecl _Cnd_broadcast(_Cnd_arg_t cnd)
{
    CONDITION_VARIABLE *cv = &CND_T_FROM_ARG(cnd)->cv;
    LONG val = InterlockedExchange( (LONG *)&cv->Ptr, 0 );
    while (val-- > 0)
        NtReleaseKeyedEvent( keyed_event, &cv->Ptr, FALSE, NULL );
    return 0;
}

int __cdecl _Cnd_signal(_Cnd_arg_t cnd)
{
    CONDITION_VARIABLE *cv = &CND_T_FROM_ARG(cnd)->cv;
    if (interlocked_dec_if_nonzero( (LONG *)&cv->Ptr ))
        NtReleaseKeyedEvent( keyed_event, &cv->Ptr, FALSE, NULL );
    return 0;
}

void __cdecl _Cnd_destroy_in_situ(_Cnd_t cnd)
{
    _Cnd_broadcast(CND_T_TO_ARG(cnd));
}

void __cdecl _Cnd_destroy(_Cnd_arg_t cnd)
{
    if(cnd) {
        _Cnd_broadcast(cnd);
        MSVCRT_operator_delete(CND_T_FROM_ARG(cnd));
    }
}

static struct {
    int used;
    int size;

    struct _to_broadcast {
        DWORD thread_id;
        _Cnd_arg_t cnd;
        _Mtx_arg_t mtx;
        int *p;
    } *to_broadcast;
} broadcast_at_thread_exit;

static CRITICAL_SECTION broadcast_at_thread_exit_cs;
static CRITICAL_SECTION_DEBUG broadcast_at_thread_exit_cs_debug =
{
        0, 0, &broadcast_at_thread_exit_cs,
        { &broadcast_at_thread_exit_cs_debug.ProcessLocksList, &broadcast_at_thread_exit_cs_debug.ProcessLocksList },
        0, 0, { (DWORD_PTR)(__FILE__ ": broadcast_at_thread_exit_cs") }
};
static CRITICAL_SECTION broadcast_at_thread_exit_cs = { &broadcast_at_thread_exit_cs_debug, -1, 0, 0, 0, 0 };

void __cdecl _Cnd_register_at_thread_exit(_Cnd_arg_t cnd, _Mtx_arg_t mtx, int *p)
{
    struct _to_broadcast *add;

    TRACE("(%p %p %p)\n", cnd, mtx, p);

    EnterCriticalSection(&broadcast_at_thread_exit_cs);
    if(!broadcast_at_thread_exit.size) {
        broadcast_at_thread_exit.to_broadcast = HeapAlloc(GetProcessHeap(),
                0, 8*sizeof(broadcast_at_thread_exit.to_broadcast[0]));
        if(!broadcast_at_thread_exit.to_broadcast) {
            LeaveCriticalSection(&broadcast_at_thread_exit_cs);
            return;
        }
        broadcast_at_thread_exit.size = 8;
    } else if(broadcast_at_thread_exit.size == broadcast_at_thread_exit.used) {
        add = HeapReAlloc(GetProcessHeap(), 0, broadcast_at_thread_exit.to_broadcast,
                broadcast_at_thread_exit.size*2*sizeof(broadcast_at_thread_exit.to_broadcast[0]));
        if(!add) {
            LeaveCriticalSection(&broadcast_at_thread_exit_cs);
            return;
        }
        broadcast_at_thread_exit.to_broadcast = add;
        broadcast_at_thread_exit.size *= 2;
    }

    add = broadcast_at_thread_exit.to_broadcast + broadcast_at_thread_exit.used++;
    add->thread_id = GetCurrentThreadId();
    add->cnd = cnd;
    add->mtx = mtx;
    add->p = p;
    LeaveCriticalSection(&broadcast_at_thread_exit_cs);
}

void __cdecl _Cnd_unregister_at_thread_exit(_Mtx_arg_t mtx)
{
    int i;

    TRACE("(%p)\n", mtx);

    EnterCriticalSection(&broadcast_at_thread_exit_cs);
    for(i=0; i<broadcast_at_thread_exit.used; i++) {
        if(broadcast_at_thread_exit.to_broadcast[i].mtx != mtx)
            continue;

        memmove(broadcast_at_thread_exit.to_broadcast+i, broadcast_at_thread_exit.to_broadcast+i+1,
                (broadcast_at_thread_exit.used-i-1)*sizeof(broadcast_at_thread_exit.to_broadcast[0]));
        broadcast_at_thread_exit.used--;
        i--;
    }
    LeaveCriticalSection(&broadcast_at_thread_exit_cs);
}

void __cdecl _Cnd_do_broadcast_at_thread_exit(void)
{
    int i, id = GetCurrentThreadId();

    TRACE("()\n");

    EnterCriticalSection(&broadcast_at_thread_exit_cs);
    for(i=0; i<broadcast_at_thread_exit.used; i++) {
        if(broadcast_at_thread_exit.to_broadcast[i].thread_id != id)
            continue;

        _Mtx_unlock(broadcast_at_thread_exit.to_broadcast[i].mtx);
        _Cnd_broadcast(broadcast_at_thread_exit.to_broadcast[i].cnd);
        if(broadcast_at_thread_exit.to_broadcast[i].p)
            *broadcast_at_thread_exit.to_broadcast[i].p = 1;

        memmove(broadcast_at_thread_exit.to_broadcast+i, broadcast_at_thread_exit.to_broadcast+i+1,
                (broadcast_at_thread_exit.used-i-1)*sizeof(broadcast_at_thread_exit.to_broadcast[0]));
        broadcast_at_thread_exit.used--;
        i--;
    }
    LeaveCriticalSection(&broadcast_at_thread_exit_cs);
}

#endif

#if _MSVCP_VER == 100
typedef struct {
    const vtable_ptr *vtable;
} error_category;

typedef struct {
    error_category base;
    const char *type;
} custom_category;
static custom_category iostream_category;

DEFINE_RTTI_DATA0(error_category, 0, ".?AVerror_category@std@@")
DEFINE_RTTI_DATA1(iostream_category, 0, &error_category_rtti_base_descriptor, ".?AV_Iostream_error_category@std@@")

extern const vtable_ptr MSVCP_iostream_category_vtable;

static void iostream_category_ctor(custom_category *this)
{
    this->base.vtable = &MSVCP_iostream_category_vtable;
    this->type = "iostream";
}

DEFINE_THISCALL_WRAPPER(custom_category_vector_dtor, 8)
custom_category* __thiscall custom_category_vector_dtor(custom_category *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCRT_operator_delete(ptr);
    } else {
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(custom_category_name, 4)
const char* __thiscall custom_category_name(const custom_category *this)
{
    return this->type;
}

DEFINE_THISCALL_WRAPPER(custom_category_message, 12)
basic_string_char* __thiscall custom_category_message(const custom_category *this,
        basic_string_char *ret, int err)
{
    return MSVCP_basic_string_char_ctor_cstr(ret, strerror(err));
}

DEFINE_THISCALL_WRAPPER(custom_category_default_error_condition, 12)
/*error_condition*/void* __thiscall custom_category_default_error_condition(
        custom_category *this, /*error_condition*/void *ret, int code)
{
    FIXME("(%p %p %x) stub\n", this, ret, code);
    return NULL;
}

DEFINE_THISCALL_WRAPPER(custom_category_equivalent, 12)
MSVCP_bool __thiscall custom_category_equivalent(const custom_category *this,
        int code, const /*error_condition*/void *condition)
{
    FIXME("(%p %x %p) stub\n", this, code, condition);
    return FALSE;
}

DEFINE_THISCALL_WRAPPER(custom_category_equivalent_code, 12)
MSVCP_bool __thiscall custom_category_equivalent_code(custom_category *this,
        const /*error_code*/void *code, int condition)
{
    FIXME("(%p %p %x) stub\n", this, code, condition);
    return FALSE;
}

DEFINE_THISCALL_WRAPPER(iostream_category_message, 12)
basic_string_char* __thiscall iostream_category_message(const custom_category *this,
        basic_string_char *ret, int err)
{
    if(err == 1) return MSVCP_basic_string_char_ctor_cstr(ret, "iostream error");
    return MSVCP_basic_string_char_ctor_cstr(ret, strerror(err));
}

/* ?iostream_category@std@@YAABVerror_category@1@XZ */
/* ?iostream_category@std@@YAAEBVerror_category@1@XZ */
const error_category* __cdecl std_iostream_category(void)
{
    TRACE("()\n");
    return &iostream_category.base;
}

static custom_category system_category;
DEFINE_RTTI_DATA1(system_category, 0, &error_category_rtti_base_descriptor, ".?AV_System_error_category@std@@")

extern const vtable_ptr MSVCP_system_category_vtable;

static void system_category_ctor(custom_category *this)
{
    this->base.vtable = &MSVCP_system_category_vtable;
    this->type = "system";
}

/* ?system_category@std@@YAABVerror_category@1@XZ */
/* ?system_category@std@@YAAEBVerror_category@1@XZ */
const error_category* __cdecl std_system_category(void)
{
    TRACE("()\n");
    return &system_category.base;
}

static custom_category generic_category;
DEFINE_RTTI_DATA1(generic_category, 0, &error_category_rtti_base_descriptor, ".?AV_Generic_error_category@std@@")

extern const vtable_ptr MSVCP_generic_category_vtable;

static void generic_category_ctor(custom_category *this)
{
    this->base.vtable = &MSVCP_generic_category_vtable;
    this->type = "generic";
}

/* ?generic_category@std@@YAABVerror_category@1@XZ */
/* ?generic_category@std@@YAAEBVerror_category@1@XZ */
const error_category* __cdecl std_generic_category(void)
{
    TRACE("()\n");
    return &generic_category.base;
}
#endif

#if _MSVCP_VER >= 110
static CRITICAL_SECTION call_once_cs;
static CRITICAL_SECTION_DEBUG call_once_cs_debug =
{
    0, 0, &call_once_cs,
    { &call_once_cs_debug.ProcessLocksList, &call_once_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": call_once_cs") }
};
static CRITICAL_SECTION call_once_cs = { &call_once_cs_debug, -1, 0, 0, 0, 0 };

void __cdecl _Call_onceEx(int *once, void (__cdecl *func)(void*), void *argv)
{
    TRACE("%p %p %p\n", once, func, argv);

    EnterCriticalSection(&call_once_cs);
    if(!*once) {
        /* FIXME: handle exceptions */
        func(argv);
        *once = 1;
    }
    LeaveCriticalSection(&call_once_cs);
}

static void __cdecl call_once_func_wrapper(void *func)
{
    ((void (__cdecl*)(void))func)();
}

void __cdecl _Call_once(int *once, void (__cdecl *func)(void))
{
    TRACE("%p %p\n", once, func);
    _Call_onceEx(once, call_once_func_wrapper, func);
}

void __cdecl _Do_call(void *this)
{
    CALL_VTBL_FUNC(this, 0, void, (void*), (this));
}
#endif

#if _MSVCP_VER >= 110
typedef struct
{
    HANDLE hnd;
    DWORD  id;
} _Thrd_t;

typedef int (__cdecl *_Thrd_start_t)(void*);

#define _THRD_ERROR 4

int __cdecl _Thrd_equal(_Thrd_t a, _Thrd_t b)
{
    TRACE("(%p %u %p %u)\n", a.hnd, a.id, b.hnd, b.id);
    return a.id == b.id;
}

int __cdecl _Thrd_lt(_Thrd_t a, _Thrd_t b)
{
    TRACE("(%p %u %p %u)\n", a.hnd, a.id, b.hnd, b.id);
    return a.id < b.id;
}

void __cdecl _Thrd_sleep(const xtime *t)
{
    TRACE("(%p)\n", t);
    Sleep(_Xtime_diff_to_millis(t));
}

void __cdecl _Thrd_yield(void)
{
    TRACE("()\n");
    Sleep(0);
}

static _Thrd_t thread_current(void)
{
    _Thrd_t ret;

    if(DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                       GetCurrentProcess(), &ret.hnd, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        CloseHandle(ret.hnd);
    } else {
        ret.hnd = 0;
    }
    ret.id  = GetCurrentThreadId();

    TRACE("(%p %u)\n", ret.hnd, ret.id);
    return ret;
}

#ifndef __i386__
_Thrd_t __cdecl _Thrd_current(void)
{
    return thread_current();
}
#else
ULONGLONG __cdecl _Thrd_current(void)
{
    union {
        _Thrd_t thr;
        ULONGLONG ull;
    } ret;

    C_ASSERT(sizeof(_Thrd_t) <= sizeof(ULONGLONG));

    ret.thr = thread_current();
    return ret.ull;
}
#endif

int __cdecl _Thrd_join(_Thrd_t thr, int *code)
{
    TRACE("(%p %u %p)\n", thr.hnd, thr.id, code);
    if (WaitForSingleObject(thr.hnd, INFINITE))
        return _THRD_ERROR;

    if (code)
        GetExitCodeThread(thr.hnd, (DWORD *)code);

    CloseHandle(thr.hnd);
    return 0;
}

int __cdecl _Thrd_start(_Thrd_t *thr, LPTHREAD_START_ROUTINE proc, void *arg)
{
    TRACE("(%p %p %p)\n", thr, proc, arg);
    thr->hnd = CreateThread(NULL, 0, proc, arg, 0, &thr->id);
    return thr->hnd ? 0 : _THRD_ERROR;
}

typedef struct
{
    _Thrd_start_t proc;
    void *arg;
} thread_proc_arg;

static DWORD WINAPI thread_proc_wrapper(void *arg)
{
    thread_proc_arg wrapped_arg = *((thread_proc_arg*)arg);
    free(arg);
    return wrapped_arg.proc(wrapped_arg.arg);
}

int __cdecl _Thrd_create(_Thrd_t *thr, _Thrd_start_t proc, void *arg)
{
    thread_proc_arg *wrapped_arg;
    int ret;

    TRACE("(%p %p %p)\n", thr, proc, arg);

    wrapped_arg = malloc(sizeof(*wrapped_arg));
    if(!wrapped_arg)
        return _THRD_ERROR; /* TODO: probably different error should be returned here */

    wrapped_arg->proc = proc;
    wrapped_arg->arg = arg;
    ret = _Thrd_start(thr, thread_proc_wrapper, wrapped_arg);
    if(ret) free(wrapped_arg);
    return ret;
}

int __cdecl _Thrd_detach(_Thrd_t thr)
{
    return CloseHandle(thr.hnd) ? 0 : _THRD_ERROR;
}

typedef struct
{
    const vtable_ptr *vtable;
    _Cnd_t cnd;
    _Mtx_t mtx;
    MSVCP_bool launched;
} _Pad;

DEFINE_RTTI_DATA0(_Pad, 0, ".?AV_Pad@std@@")

/* ??_7_Pad@std@@6B@ */
extern const vtable_ptr MSVCP__Pad_vtable;

unsigned int __cdecl _Thrd_hardware_concurrency(void)
{
    static unsigned int val = -1;

    TRACE("()\n");

    if(val == -1) {
        SYSTEM_INFO si;

        GetSystemInfo(&si);
        val = si.dwNumberOfProcessors;
    }

    return val;
}

unsigned int __cdecl _Thrd_id(void)
{
    TRACE("()\n");
    return GetCurrentThreadId();
}

/* ??0_Pad@std@@QAE@XZ */
/* ??0_Pad@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Pad_ctor, 4)
_Pad* __thiscall _Pad_ctor(_Pad *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &MSVCP__Pad_vtable;
    _Cnd_init(&this->cnd);
    _Mtx_init(&this->mtx, 0);
    this->launched = FALSE;
    _Mtx_lock(MTX_T_TO_ARG(this->mtx));
    return this;
}

/* ??4_Pad@std@@QAEAAV01@ABV01@@Z */
/* ??4_Pad@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Pad_op_assign, 8)
_Pad* __thiscall _Pad_op_assign(_Pad *this, const _Pad *copy)
{
    TRACE("(%p %p)\n", this, copy);

    this->cnd = copy->cnd;
    this->mtx = copy->mtx;
    this->launched = copy->launched;
    return this;
}

/* ??0_Pad@std@@QAE@ABV01@@Z */
/* ??0_Pad@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Pad_copy_ctor, 8)
_Pad* __thiscall _Pad_copy_ctor(_Pad *this, const _Pad *copy)
{
    TRACE("(%p %p)\n", this, copy);

    this->vtable = &MSVCP__Pad_vtable;
    return _Pad_op_assign(this, copy);
}

/* ??1_Pad@std@@QAE@XZ */
/* ??1_Pad@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Pad_dtor, 4)
void __thiscall _Pad_dtor(_Pad *this)
{
    TRACE("(%p)\n", this);

    _Mtx_unlock(MTX_T_TO_ARG(this->mtx));
    _Mtx_destroy(MTX_T_TO_ARG(this->mtx));
    _Cnd_destroy(CND_T_TO_ARG(this->cnd));
}

DEFINE_THISCALL_WRAPPER(_Pad__Go, 4)
#define call__Pad__Go(this) CALL_VTBL_FUNC(this, 0, unsigned int, (_Pad*), (this))
unsigned int __thiscall _Pad__Go(_Pad *this)
{
    ERR("(%p) should not be called\n", this);
    return 0;
}

static DWORD WINAPI launch_thread_proc(void *arg)
{
    _Pad *this = arg;
    return call__Pad__Go(this);
}

/* ?_Launch@_Pad@std@@QAEXPAU_Thrd_imp_t@@@Z */
/* ?_Launch@_Pad@std@@QEAAXPEAU_Thrd_imp_t@@@Z */
DEFINE_THISCALL_WRAPPER(_Pad__Launch, 8)
void __thiscall _Pad__Launch(_Pad *this, _Thrd_t *thr)
{
    TRACE("(%p %p)\n", this, thr);

    _Thrd_start(thr, launch_thread_proc, this);
    _Cnd_wait(CND_T_TO_ARG(this->cnd), MTX_T_TO_ARG(this->mtx));
}

/* ?_Release@_Pad@std@@QAEXXZ */
/* ?_Release@_Pad@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Pad__Release, 4)
void __thiscall _Pad__Release(_Pad *this)
{
    TRACE("(%p)\n", this);

    _Mtx_lock(MTX_T_TO_ARG(this->mtx));
    this->launched = TRUE;
    _Cnd_signal(CND_T_TO_ARG(this->cnd));
    _Mtx_unlock(MTX_T_TO_ARG(this->mtx));
}
#endif

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
#if _MSVCP_VER == 100
    __ASM_VTABLE(iostream_category,
            VTABLE_ADD_FUNC(custom_category_vector_dtor)
            VTABLE_ADD_FUNC(custom_category_name)
            VTABLE_ADD_FUNC(iostream_category_message)
            VTABLE_ADD_FUNC(custom_category_default_error_condition)
            VTABLE_ADD_FUNC(custom_category_equivalent)
            VTABLE_ADD_FUNC(custom_category_equivalent_code));
    __ASM_VTABLE(system_category,
            VTABLE_ADD_FUNC(custom_category_vector_dtor)
            VTABLE_ADD_FUNC(custom_category_name)
            VTABLE_ADD_FUNC(custom_category_message)
            VTABLE_ADD_FUNC(custom_category_default_error_condition)
            VTABLE_ADD_FUNC(custom_category_equivalent)
            VTABLE_ADD_FUNC(custom_category_equivalent_code));
    __ASM_VTABLE(generic_category,
            VTABLE_ADD_FUNC(custom_category_vector_dtor)
            VTABLE_ADD_FUNC(custom_category_name)
            VTABLE_ADD_FUNC(custom_category_message)
            VTABLE_ADD_FUNC(custom_category_default_error_condition)
            VTABLE_ADD_FUNC(custom_category_equivalent)
            VTABLE_ADD_FUNC(custom_category_equivalent_code));
#endif
#if _MSVCP_VER >= 110
    __ASM_VTABLE(_Pad,
            VTABLE_ADD_FUNC(_Pad__Go));
#endif
#ifndef __GNUC__
}
#endif

/*********************************************************************
 *  __crtInitializeCriticalSectionEx (MSVCP140.@)
 */
BOOL CDECL MSVCP__crtInitializeCriticalSectionEx(
        CRITICAL_SECTION *cs, DWORD spin_count, DWORD flags)
{
    TRACE("(%p %x %x)\n", cs, spin_count, flags);
    return InitializeCriticalSectionEx(cs, spin_count, flags);
}

/*********************************************************************
 *  __crtCreateEventExW (MSVCP140.@)
 */
HANDLE CDECL MSVCP__crtCreateEventExW(
        SECURITY_ATTRIBUTES *attribs, LPCWSTR name, DWORD flags, DWORD access)
{
    TRACE("(%p %s 0x%08x 0x%08x)\n", attribs, debugstr_w(name), flags, access);
    return CreateEventExW(attribs, name, flags, access);
}

/*********************************************************************
 *  __crtGetTickCount64 (MSVCP140.@)
 */
ULONGLONG CDECL MSVCP__crtGetTickCount64(void)
{
    return GetTickCount64();
}

/*********************************************************************
 *  __crtCreateSemaphoreExW (MSVCP140.@)
 */
HANDLE CDECL MSVCP__crtCreateSemaphoreExW(
        SECURITY_ATTRIBUTES *attribs, LONG initial_count, LONG max_count, LPCWSTR name,
        DWORD flags, DWORD access)
{
    TRACE("(%p %d %d %s 0x%08x 0x%08x)\n", attribs, initial_count, max_count, debugstr_w(name),
            flags, access);
    return CreateSemaphoreExW(attribs, initial_count, max_count, name, flags, access);
}

/* ?_Execute_once@std@@YAHAAUonce_flag@1@P6GHPAX1PAPAX@Z1@Z */
/* ?_Execute_once@std@@YAHAEAUonce_flag@1@P6AHPEAX1PEAPEAX@Z1@Z */
BOOL __cdecl _Execute_once(INIT_ONCE *flag, PINIT_ONCE_FN func, void *param)
{
    return InitOnceExecuteOnce(flag, func, param, NULL);
}

void init_misc(void *base)
{
#ifdef __x86_64__
#if _MSVCP_VER == 100
    init_error_category_rtti(base);
    init_iostream_category_rtti(base);
    init_system_category_rtti(base);
    init_generic_category_rtti(base);
#endif
#if _MSVCP_VER >= 110
    init__Pad_rtti(base);
#endif
#endif

#if _MSVCP_VER == 100
    iostream_category_ctor(&iostream_category);
    system_category_ctor(&system_category);
    generic_category_ctor(&generic_category);
#endif
}

void free_misc(void)
{
#if _MSVCP_VER >= 110
    if(keyed_event)
        NtClose(keyed_event);
    HeapFree(GetProcessHeap(), 0, broadcast_at_thread_exit.to_broadcast);
#endif
}

#if _MSVCP_VER >= 140
LONGLONG __cdecl _Query_perf_counter(void)
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

LONGLONG __cdecl _Query_perf_frequency(void)
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}
#endif
