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
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

#define SECSPERDAY 86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970 ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC 10000000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)
#define NANOSEC_PER_MILLISEC 1000000
#define MILLISEC_PER_SEC 1000

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
MSVCRT_long __cdecl _Xtime_diff_to_millis2(xtime *t1, xtime *t2)
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
MSVCRT_long __cdecl _Xtime_diff_to_millis(xtime *t)
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

#define THISCALL(func) __thiscall_ ## func
#define __thiscall __stdcall
#define DEFINE_THISCALL_WRAPPER(func,args) \
    extern void THISCALL(func)(void); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
            "popl %eax\n\t" \
            "pushl %ecx\n\t" \
            "pushl %eax\n\t" \
            "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )

extern void *call_thiscall_func;
__ASM_GLOBAL_FUNC(call_thiscall_func,
        "popl %eax\n\t"
        "popl %edx\n\t"
        "popl %ecx\n\t"
        "pushl %eax\n\t"
        "jmp *%edx\n\t")

#define call_func1(func,this) ((void* (WINAPI*)(void*,void*))&call_thiscall_func)(func,this)

#else /* __i386__ */

#define __thiscall __cdecl
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

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

int __cdecl _Mtx_init(_Mtx_t *mtx, int flags)
{
    if(flags & ~MTX_MULTI_LOCK)
        FIXME("unknown flags ignorred: %x\n", flags);

    *mtx = MSVCRT_operator_new(sizeof(**mtx));
    (*mtx)->flags = flags;
    call_func1(critical_section_ctor, &(*mtx)->cs);
    (*mtx)->thread_id = -1;
    (*mtx)->count = 0;
    return 0;
}

void __cdecl _Mtx_destroy(_Mtx_t *mtx)
{
    call_func1(critical_section_dtor, &(*mtx)->cs);
    MSVCRT_operator_delete(*mtx);
}

int __cdecl _Mtx_lock(_Mtx_t *mtx)
{
    if((*mtx)->thread_id != GetCurrentThreadId()) {
        call_func1(critical_section_lock, &(*mtx)->cs);
        (*mtx)->thread_id = GetCurrentThreadId();
    }else if(!((*mtx)->flags & MTX_MULTI_LOCK)) {
        return MTX_LOCKED;
    }

    (*mtx)->count++;
    return 0;
}

int __cdecl _Mtx_unlock(_Mtx_t *mtx)
{
    if(--(*mtx)->count)
        return 0;

    (*mtx)->thread_id = -1;
    call_func1(critical_section_unlock, &(*mtx)->cs);
    return 0;
}

int __cdecl _Mtx_trylock(_Mtx_t *mtx)
{
    if((*mtx)->thread_id != GetCurrentThreadId()) {
        if(!call_func1(critical_section_trylock, &(*mtx)->cs))
            return MTX_LOCKED;
        (*mtx)->thread_id = GetCurrentThreadId();
    }else if(!((*mtx)->flags & MTX_MULTI_LOCK)) {
        return MTX_LOCKED;
    }

    (*mtx)->count++;
    return 0;
}

critical_section* __cdecl _Mtx_getconcrtcs(_Mtx_t *mtx)
{
    return &(*mtx)->cs;
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
    if(err == 1) return MSVCP_basic_string_char_ctor_cstr(ret, "iostream error");
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

/* ?iostream_category@std@@YAABVerror_category@1@XZ */
/* ?iostream_category@std@@YAAEBVerror_category@1@XZ */
const error_category* __cdecl std_iostream_category(void)
{
    TRACE("()\n");
    return &iostream_category.base;
}

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(iostream_category,
            VTABLE_ADD_FUNC(custom_category_vector_dtor)
            VTABLE_ADD_FUNC(custom_category_name)
            VTABLE_ADD_FUNC(custom_category_message)
            VTABLE_ADD_FUNC(custom_category_default_error_condition)
            VTABLE_ADD_FUNC(custom_category_equivalent)
            VTABLE_ADD_FUNC(custom_category_equivalent_code));
#ifndef __GNUC__
}
#endif
#endif

void init_misc(void *base)
{
#ifdef __x86_64__
#if _MSVCP_VER == 100
    init_error_category_rtti(base);
    init_iostream_category_rtti(base);
#endif
#endif

#if _MSVCP_VER == 100
    iostream_category_ctor(&iostream_category);
#endif
}
