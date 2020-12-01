/*
 * msvcrt.dll C++ objects
 *
 * Copyright 2017 Piotr Caban
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

#include <stdarg.h>
#include <stdbool.h>

#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "msvcrt.h"
#include "cppexcept.h"
#include "cxx.h"

#if _MSVCR_VER >= 100

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static int context_id = -1;
static int scheduler_id = -1;

typedef enum {
    SchedulerKind,
    MaxConcurrency,
    MinConcurrency,
    TargetOversubscriptionFactor,
    LocalContextCacheSize,
    ContextStackSize,
    ContextPriority,
    SchedulingProtocol,
    DynamicProgressFeedback,
    WinRTInitialization,
    last_policy_id
} PolicyElementKey;

typedef struct {
    struct _policy_container {
        unsigned int policies[last_policy_id];
    } *policy_container;
} SchedulerPolicy;

typedef struct {
    const vtable_ptr *vtable;
} Context;
#define call_Context_GetId(this) CALL_VTBL_FUNC(this, 0, \
        unsigned int, (const Context*), (this))
#define call_Context_GetVirtualProcessorId(this) CALL_VTBL_FUNC(this, 4, \
        unsigned int, (const Context*), (this))
#define call_Context_GetScheduleGroupId(this) CALL_VTBL_FUNC(this, 8, \
        unsigned int, (const Context*), (this))
#define call_Context_dtor(this, flags) CALL_VTBL_FUNC(this, 20, \
        Context*, (Context*, unsigned int), (this, flags))

typedef struct {
    Context *context;
} _Context;

union allocator_cache_entry {
    struct _free {
        int depth;
        union allocator_cache_entry *next;
    } free;
    struct _alloc {
        int bucket;
        char mem[1];
    } alloc;
};

struct scheduler_list {
    struct Scheduler *scheduler;
    struct scheduler_list *next;
};

typedef struct {
    Context context;
    struct scheduler_list scheduler;
    unsigned int id;
    union allocator_cache_entry *allocator_cache[8];
} ExternalContextBase;
extern const vtable_ptr ExternalContextBase_vtable;
static void ExternalContextBase_ctor(ExternalContextBase*);

typedef struct Scheduler {
    const vtable_ptr *vtable;
} Scheduler;
#define call_Scheduler_Id(this) CALL_VTBL_FUNC(this, 4, unsigned int, (const Scheduler*), (this))
#define call_Scheduler_GetNumberOfVirtualProcessors(this) CALL_VTBL_FUNC(this, 8, unsigned int, (const Scheduler*), (this))
#define call_Scheduler_GetPolicy(this,policy) CALL_VTBL_FUNC(this, 12, \
        SchedulerPolicy*, (Scheduler*,SchedulerPolicy*), (this,policy))
#define call_Scheduler_Reference(this) CALL_VTBL_FUNC(this, 16, unsigned int, (Scheduler*), (this))
#define call_Scheduler_Release(this) CALL_VTBL_FUNC(this, 20, unsigned int, (Scheduler*), (this))
#define call_Scheduler_RegisterShutdownEvent(this,event) CALL_VTBL_FUNC(this, 24, void, (Scheduler*,HANDLE), (this,event))
#define call_Scheduler_Attach(this) CALL_VTBL_FUNC(this, 28, void, (Scheduler*), (this))
#if _MSVCR_VER > 100
#define call_Scheduler_CreateScheduleGroup_loc(this,placement) CALL_VTBL_FUNC(this, 32, \
        /*ScheduleGroup*/void*, (Scheduler*,/*location*/void*), (this,placement))
#define call_Scheduler_CreateScheduleGroup(this) CALL_VTBL_FUNC(this, 36, /*ScheduleGroup*/void*, (Scheduler*), (this))
#define call_Scheduler_ScheduleTask_loc(this,proc,data,placement) CALL_VTBL_FUNC(this, 40, \
        void, (Scheduler*,void (__cdecl*)(void*),void*,/*location*/void*), (this,proc,data,placement))
#define call_Scheduler_ScheduleTask(this,proc,data) CALL_VTBL_FUNC(this, 44, \
        void, (Scheduler*,void (__cdecl*)(void*),void*), (this,proc,data))
#define call_Scheduler_IsAvailableLocation(this,placement) CALL_VTBL_FUNC(this, 48, \
        bool, (Scheduler*,const /*location*/void*), (this,placement))
#else
#define call_Scheduler_CreateScheduleGroup(this) CALL_VTBL_FUNC(this, 32, /*ScheduleGroup*/void*, (Scheduler*), (this))
#define call_Scheduler_ScheduleTask(this,proc,data) CALL_VTBL_FUNC(this, 36, \
        void, (Scheduler*,void (__cdecl*)(void*),void*), (this,proc,data))
#endif

typedef struct {
    Scheduler scheduler;
    LONG ref;
    unsigned int id;
    unsigned int virt_proc_no;
    SchedulerPolicy policy;
    int shutdown_count;
    int shutdown_size;
    HANDLE *shutdown_events;
    CRITICAL_SECTION cs;
} ThreadScheduler;
extern const vtable_ptr ThreadScheduler_vtable;

typedef struct {
    Scheduler *scheduler;
} _Scheduler;

typedef struct {
    char empty;
} _CurrentScheduler;

static int context_tls_index = TLS_OUT_OF_INDEXES;

static CRITICAL_SECTION default_scheduler_cs;
static CRITICAL_SECTION_DEBUG default_scheduler_cs_debug =
{
    0, 0, &default_scheduler_cs,
    { &default_scheduler_cs_debug.ProcessLocksList, &default_scheduler_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": default_scheduler_cs") }
};
static CRITICAL_SECTION default_scheduler_cs = { &default_scheduler_cs_debug, -1, 0, 0, 0, 0 };
static SchedulerPolicy default_scheduler_policy;
static ThreadScheduler *default_scheduler;

static void create_default_scheduler(void);

static Context* try_get_current_context(void)
{
    if (context_tls_index == TLS_OUT_OF_INDEXES)
        return NULL;
    return TlsGetValue(context_tls_index);
}

static Context* get_current_context(void)
{
    Context *ret;

    if (context_tls_index == TLS_OUT_OF_INDEXES) {
        int tls_index = TlsAlloc();
        if (tls_index == TLS_OUT_OF_INDEXES) {
            throw_exception(EXCEPTION_SCHEDULER_RESOURCE_ALLOCATION_ERROR,
                    HRESULT_FROM_WIN32(GetLastError()), NULL);
            return NULL;
        }

        if(InterlockedCompareExchange(&context_tls_index, tls_index, TLS_OUT_OF_INDEXES) != TLS_OUT_OF_INDEXES)
            TlsFree(tls_index);
    }

    ret = TlsGetValue(context_tls_index);
    if (!ret) {
        ExternalContextBase *context = operator_new(sizeof(ExternalContextBase));
        ExternalContextBase_ctor(context);
        TlsSetValue(context_tls_index, context);
        ret = &context->context;
    }
    return ret;
}

static Scheduler* try_get_current_scheduler(void)
{
    ExternalContextBase *context = (ExternalContextBase*)try_get_current_context();

    if (!context)
        return NULL;

    if (context->context.vtable != &ExternalContextBase_vtable) {
        ERR("unknown context set\n");
        return NULL;
    }
    return context->scheduler.scheduler;
}

static Scheduler* get_current_scheduler(void)
{
    ExternalContextBase *context = (ExternalContextBase*)get_current_context();

    if (context->context.vtable != &ExternalContextBase_vtable) {
        ERR("unknown context set\n");
        return NULL;
    }
    return context->scheduler.scheduler;
}

/* ?CurrentContext@Context@Concurrency@@SAPAV12@XZ */
/* ?CurrentContext@Context@Concurrency@@SAPEAV12@XZ */
Context* __cdecl Context_CurrentContext(void)
{
    TRACE("()\n");
    return get_current_context();
}

/* ?Id@Context@Concurrency@@SAIXZ */
unsigned int __cdecl Context_Id(void)
{
    Context *ctx = try_get_current_context();
    TRACE("()\n");
    return ctx ? call_Context_GetId(ctx) : -1;
}

/* ?Block@Context@Concurrency@@SAXXZ */
void __cdecl Context_Block(void)
{
    FIXME("()\n");
}

/* ?Yield@Context@Concurrency@@SAXXZ */
/* ?_Yield@_Context@details@Concurrency@@SAXXZ */
void __cdecl Context_Yield(void)
{
    FIXME("()\n");
}

/* ?_SpinYield@Context@Concurrency@@SAXXZ */
void __cdecl Context__SpinYield(void)
{
    FIXME("()\n");
}

/* ?IsCurrentTaskCollectionCanceling@Context@Concurrency@@SA_NXZ */
bool __cdecl Context_IsCurrentTaskCollectionCanceling(void)
{
    FIXME("()\n");
    return FALSE;
}

/* ?Oversubscribe@Context@Concurrency@@SAX_N@Z */
void __cdecl Context_Oversubscribe(bool begin)
{
    FIXME("(%x)\n", begin);
}

/* ?ScheduleGroupId@Context@Concurrency@@SAIXZ */
unsigned int __cdecl Context_ScheduleGroupId(void)
{
    Context *ctx = try_get_current_context();
    TRACE("()\n");
    return ctx ? call_Context_GetScheduleGroupId(ctx) : -1;
}

/* ?VirtualProcessorId@Context@Concurrency@@SAIXZ */
unsigned int __cdecl Context_VirtualProcessorId(void)
{
    Context *ctx = try_get_current_context();
    TRACE("()\n");
    return ctx ? call_Context_GetVirtualProcessorId(ctx) : -1;
}

#if _MSVCR_VER > 100
/* ?_CurrentContext@_Context@details@Concurrency@@SA?AV123@XZ */
_Context *__cdecl _Context__CurrentContext(_Context *ret)
{
    TRACE("(%p)\n", ret);
    ret->context = Context_CurrentContext();
    return ret;
}
#endif

DEFINE_THISCALL_WRAPPER(ExternalContextBase_GetId, 4)
unsigned int __thiscall ExternalContextBase_GetId(const ExternalContextBase *this)
{
    TRACE("(%p)->()\n", this);
    return this->id;
}

DEFINE_THISCALL_WRAPPER(ExternalContextBase_GetVirtualProcessorId, 4)
unsigned int __thiscall ExternalContextBase_GetVirtualProcessorId(const ExternalContextBase *this)
{
    FIXME("(%p)->() stub\n", this);
    return -1;
}

DEFINE_THISCALL_WRAPPER(ExternalContextBase_GetScheduleGroupId, 4)
unsigned int __thiscall ExternalContextBase_GetScheduleGroupId(const ExternalContextBase *this)
{
    FIXME("(%p)->() stub\n", this);
    return -1;
}

DEFINE_THISCALL_WRAPPER(ExternalContextBase_Unblock, 4)
void __thiscall ExternalContextBase_Unblock(ExternalContextBase *this)
{
    FIXME("(%p)->() stub\n", this);
}

DEFINE_THISCALL_WRAPPER(ExternalContextBase_IsSynchronouslyBlocked, 4)
bool __thiscall ExternalContextBase_IsSynchronouslyBlocked(const ExternalContextBase *this)
{
    FIXME("(%p)->() stub\n", this);
    return FALSE;
}

static void ExternalContextBase_dtor(ExternalContextBase *this)
{
    struct scheduler_list *scheduler_cur, *scheduler_next;
    union allocator_cache_entry *next, *cur;
    int i;

    /* TODO: move the allocator cache to scheduler so it can be reused */
    for(i=0; i<ARRAY_SIZE(this->allocator_cache); i++) {
        for(cur = this->allocator_cache[i]; cur; cur=next) {
            next = cur->free.next;
            operator_delete(cur);
        }
    }

    if (this->scheduler.scheduler) {
        call_Scheduler_Release(this->scheduler.scheduler);

        for(scheduler_cur=this->scheduler.next; scheduler_cur; scheduler_cur=scheduler_next) {
            scheduler_next = scheduler_cur->next;
            call_Scheduler_Release(scheduler_cur->scheduler);
            operator_delete(scheduler_cur);
        }
    }
}

DEFINE_THISCALL_WRAPPER(ExternalContextBase_vector_dtor, 8)
Context* __thiscall ExternalContextBase_vector_dtor(ExternalContextBase *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ExternalContextBase_dtor(this+i);
        operator_delete(ptr);
    } else {
        ExternalContextBase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return &this->context;
}

static void ExternalContextBase_ctor(ExternalContextBase *this)
{
    TRACE("(%p)->()\n", this);

    memset(this, 0, sizeof(*this));
    this->context.vtable = &ExternalContextBase_vtable;
    this->id = InterlockedIncrement(&context_id);

    create_default_scheduler();
    this->scheduler.scheduler = &default_scheduler->scheduler;
    call_Scheduler_Reference(&default_scheduler->scheduler);
}

/* ?Alloc@Concurrency@@YAPAXI@Z */
/* ?Alloc@Concurrency@@YAPEAX_K@Z */
void * CDECL Concurrency_Alloc(size_t size)
{
    ExternalContextBase *context = (ExternalContextBase*)get_current_context();
    union allocator_cache_entry *p;

    size += FIELD_OFFSET(union allocator_cache_entry, alloc.mem);
    if (size < sizeof(*p))
        size = sizeof(*p);

    if (context->context.vtable != &ExternalContextBase_vtable) {
        p = operator_new(size);
        p->alloc.bucket = -1;
    }else {
        int i;

        C_ASSERT(sizeof(union allocator_cache_entry) <= 1 << 4);
        for(i=0; i<ARRAY_SIZE(context->allocator_cache); i++)
            if (1 << (i+4) >= size) break;

        if(i==ARRAY_SIZE(context->allocator_cache)) {
            p = operator_new(size);
            p->alloc.bucket = -1;
        }else if (context->allocator_cache[i]) {
            p = context->allocator_cache[i];
            context->allocator_cache[i] = p->free.next;
            p->alloc.bucket = i;
        }else {
            p = operator_new(1 << (i+4));
            p->alloc.bucket = i;
        }
    }

    TRACE("(%Iu) returning %p\n", size, p->alloc.mem);
    return p->alloc.mem;
}

/* ?Free@Concurrency@@YAXPAX@Z */
/* ?Free@Concurrency@@YAXPEAX@Z */
void CDECL Concurrency_Free(void* mem)
{
    union allocator_cache_entry *p = (union allocator_cache_entry*)((char*)mem-FIELD_OFFSET(union allocator_cache_entry, alloc.mem));
    ExternalContextBase *context = (ExternalContextBase*)get_current_context();
    int bucket = p->alloc.bucket;

    TRACE("(%p)\n", mem);

    if (context->context.vtable != &ExternalContextBase_vtable) {
        operator_delete(p);
    }else {
        if(bucket >= 0 && bucket < ARRAY_SIZE(context->allocator_cache) &&
            (!context->allocator_cache[bucket] || context->allocator_cache[bucket]->free.depth < 20)) {
            p->free.next = context->allocator_cache[bucket];
            p->free.depth = p->free.next ? p->free.next->free.depth+1 : 0;
            context->allocator_cache[bucket] = p;
        }else {
            operator_delete(p);
        }
    }
}

/* ?SetPolicyValue@SchedulerPolicy@Concurrency@@QAEIW4PolicyElementKey@2@I@Z */
/* ?SetPolicyValue@SchedulerPolicy@Concurrency@@QEAAIW4PolicyElementKey@2@I@Z */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_SetPolicyValue, 12)
unsigned int __thiscall SchedulerPolicy_SetPolicyValue(SchedulerPolicy *this,
        PolicyElementKey policy, unsigned int val)
{
    unsigned int ret;

    TRACE("(%p %d %d)\n", this, policy, val);

    if (policy == MinConcurrency)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_KEY, 0, "MinConcurrency");
    if (policy == MaxConcurrency)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_KEY, 0, "MaxConcurrency");
    if (policy >= last_policy_id)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_KEY, 0, "Invalid policy");

    switch(policy) {
    case SchedulerKind:
        if (val)
            throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_VALUE, 0, "SchedulerKind");
        break;
    case TargetOversubscriptionFactor:
        if (!val)
            throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_VALUE,
                    0, "TargetOversubscriptionFactor");
        break;
    case ContextPriority:
        if (((int)val < -7 /* THREAD_PRIORITY_REALTIME_LOWEST */
                    || val > 6 /* THREAD_PRIORITY_REALTIME_HIGHEST */)
            && val != THREAD_PRIORITY_IDLE && val != THREAD_PRIORITY_TIME_CRITICAL
            && val != INHERIT_THREAD_PRIORITY)
            throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_VALUE, 0, "ContextPriority");
        break;
    case SchedulingProtocol:
    case DynamicProgressFeedback:
    case WinRTInitialization:
        if (val != 0 && val != 1)
            throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_VALUE, 0, "SchedulingProtocol");
        break;
    default:
        break;
    }

    ret = this->policy_container->policies[policy];
    this->policy_container->policies[policy] = val;
    return ret;
}

/* ?SetConcurrencyLimits@SchedulerPolicy@Concurrency@@QAEXII@Z */
/* ?SetConcurrencyLimits@SchedulerPolicy@Concurrency@@QEAAXII@Z */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_SetConcurrencyLimits, 12)
void __thiscall SchedulerPolicy_SetConcurrencyLimits(SchedulerPolicy *this,
        unsigned int min_concurrency, unsigned int max_concurrency)
{
    TRACE("(%p %d %d)\n", this, min_concurrency, max_concurrency);

    if (min_concurrency > max_concurrency)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_THREAD_SPECIFICATION, 0, NULL);
    if (!max_concurrency)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_VALUE, 0, "MaxConcurrency");

    this->policy_container->policies[MinConcurrency] = min_concurrency;
    this->policy_container->policies[MaxConcurrency] = max_concurrency;
}

/* ?GetPolicyValue@SchedulerPolicy@Concurrency@@QBEIW4PolicyElementKey@2@@Z */
/* ?GetPolicyValue@SchedulerPolicy@Concurrency@@QEBAIW4PolicyElementKey@2@@Z */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_GetPolicyValue, 8)
unsigned int __thiscall SchedulerPolicy_GetPolicyValue(
        const SchedulerPolicy *this, PolicyElementKey policy)
{
    TRACE("(%p %d)\n", this, policy);

    if (policy >= last_policy_id)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_KEY, 0, "Invalid policy");
    return this->policy_container->policies[policy];
}

/* ??0SchedulerPolicy@Concurrency@@QAE@XZ */
/* ??0SchedulerPolicy@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_ctor, 4)
SchedulerPolicy* __thiscall SchedulerPolicy_ctor(SchedulerPolicy *this)
{
    TRACE("(%p)\n", this);

    this->policy_container = operator_new(sizeof(*this->policy_container));
    /* TODO: default values can probably be affected by CurrentScheduler */
    this->policy_container->policies[SchedulerKind] = 0;
    this->policy_container->policies[MaxConcurrency] = -1;
    this->policy_container->policies[MinConcurrency] = 1;
    this->policy_container->policies[TargetOversubscriptionFactor] = 1;
    this->policy_container->policies[LocalContextCacheSize] = 8;
    this->policy_container->policies[ContextStackSize] = 0;
    this->policy_container->policies[ContextPriority] = THREAD_PRIORITY_NORMAL;
    this->policy_container->policies[SchedulingProtocol] = 0;
    this->policy_container->policies[DynamicProgressFeedback] = 1;
    return this;
}

/* ??0SchedulerPolicy@Concurrency@@QAA@IZZ */
/* ??0SchedulerPolicy@Concurrency@@QEAA@_KZZ */
/* TODO: don't leak policy_container on exception */
SchedulerPolicy* WINAPIV SchedulerPolicy_ctor_policies(
        SchedulerPolicy *this, size_t n, ...)
{
    unsigned int min_concurrency, max_concurrency;
    __ms_va_list valist;
    size_t i;

    TRACE("(%p %Iu)\n", this, n);

    SchedulerPolicy_ctor(this);
    min_concurrency = this->policy_container->policies[MinConcurrency];
    max_concurrency = this->policy_container->policies[MaxConcurrency];

    __ms_va_start(valist, n);
    for(i=0; i<n; i++) {
        PolicyElementKey policy = va_arg(valist, PolicyElementKey);
        unsigned int val = va_arg(valist, unsigned int);

        if(policy == MinConcurrency)
            min_concurrency = val;
        else if(policy == MaxConcurrency)
            max_concurrency = val;
        else
            SchedulerPolicy_SetPolicyValue(this, policy, val);
    }
    __ms_va_end(valist);

    SchedulerPolicy_SetConcurrencyLimits(this, min_concurrency, max_concurrency);
    return this;
}

/* ??4SchedulerPolicy@Concurrency@@QAEAAV01@ABV01@@Z */
/* ??4SchedulerPolicy@Concurrency@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_op_assign, 8)
SchedulerPolicy* __thiscall SchedulerPolicy_op_assign(
        SchedulerPolicy *this, const SchedulerPolicy *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    memcpy(this->policy_container->policies, rhs->policy_container->policies,
            sizeof(this->policy_container->policies));
    return this;
}

/* ??0SchedulerPolicy@Concurrency@@QAE@ABV01@@Z */
/* ??0SchedulerPolicy@Concurrency@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_copy_ctor, 8)
SchedulerPolicy* __thiscall SchedulerPolicy_copy_ctor(
        SchedulerPolicy *this, const SchedulerPolicy *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    SchedulerPolicy_ctor(this);
    return SchedulerPolicy_op_assign(this, rhs);
}

/* ??1SchedulerPolicy@Concurrency@@QAE@XZ */
/* ??1SchedulerPolicy@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_dtor, 4)
void __thiscall SchedulerPolicy_dtor(SchedulerPolicy *this)
{
    TRACE("(%p)\n", this);
    operator_delete(this->policy_container);
}

static void ThreadScheduler_dtor(ThreadScheduler *this)
{
    int i;

    if(this->ref != 0) WARN("ref = %d\n", this->ref);
    SchedulerPolicy_dtor(&this->policy);

    for(i=0; i<this->shutdown_count; i++)
        SetEvent(this->shutdown_events[i]);
    operator_delete(this->shutdown_events);

    this->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&this->cs);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_Id, 4)
unsigned int __thiscall ThreadScheduler_Id(const ThreadScheduler *this)
{
    TRACE("(%p)\n", this);
    return this->id;
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_GetNumberOfVirtualProcessors, 4)
unsigned int __thiscall ThreadScheduler_GetNumberOfVirtualProcessors(const ThreadScheduler *this)
{
    TRACE("(%p)\n", this);
    return this->virt_proc_no;
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_GetPolicy, 8)
SchedulerPolicy* __thiscall ThreadScheduler_GetPolicy(
        const ThreadScheduler *this, SchedulerPolicy *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return SchedulerPolicy_copy_ctor(ret, &this->policy);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_Reference, 4)
unsigned int __thiscall ThreadScheduler_Reference(ThreadScheduler *this)
{
    TRACE("(%p)\n", this);
    return InterlockedIncrement(&this->ref);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_Release, 4)
unsigned int __thiscall ThreadScheduler_Release(ThreadScheduler *this)
{
    unsigned int ret = InterlockedDecrement(&this->ref);

    TRACE("(%p)\n", this);

    if(!ret) {
        ThreadScheduler_dtor(this);
        operator_delete(this);
    }
    return ret;
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_RegisterShutdownEvent, 8)
void __thiscall ThreadScheduler_RegisterShutdownEvent(ThreadScheduler *this, HANDLE event)
{
    HANDLE *shutdown_events;
    int size;

    TRACE("(%p %p)\n", this, event);

    EnterCriticalSection(&this->cs);

    size = this->shutdown_size ? this->shutdown_size * 2 : 1;
    shutdown_events = operator_new(size * sizeof(*shutdown_events));
    memcpy(shutdown_events, this->shutdown_events,
            this->shutdown_count * sizeof(*shutdown_events));
    operator_delete(this->shutdown_events);
    this->shutdown_size = size;
    this->shutdown_events = shutdown_events;
    this->shutdown_events[this->shutdown_count++] = event;

    LeaveCriticalSection(&this->cs);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_Attach, 4)
void __thiscall ThreadScheduler_Attach(ThreadScheduler *this)
{
    ExternalContextBase *context = (ExternalContextBase*)get_current_context();

    TRACE("(%p)\n", this);

    if(context->context.vtable != &ExternalContextBase_vtable) {
        ERR("unknown context set\n");
        return;
    }

    if(context->scheduler.scheduler == &this->scheduler)
        throw_exception(EXCEPTION_IMPROPER_SCHEDULER_ATTACH, 0, NULL);

    if(context->scheduler.scheduler) {
        struct scheduler_list *l = operator_new(sizeof(*l));
        *l = context->scheduler;
        context->scheduler.next = l;
    }
    context->scheduler.scheduler = &this->scheduler;
    ThreadScheduler_Reference(this);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_CreateScheduleGroup_loc, 8)
/*ScheduleGroup*/void* __thiscall ThreadScheduler_CreateScheduleGroup_loc(
        ThreadScheduler *this, /*location*/void *placement)
{
    FIXME("(%p %p) stub\n", this, placement);
    return NULL;
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_CreateScheduleGroup, 4)
/*ScheduleGroup*/void* __thiscall ThreadScheduler_CreateScheduleGroup(ThreadScheduler *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_ScheduleTask_loc, 16)
void __thiscall ThreadScheduler_ScheduleTask_loc(ThreadScheduler *this,
        void (__cdecl *proc)(void*), void* data, /*location*/void *placement)
{
    FIXME("(%p %p %p %p) stub\n", this, proc, data, placement);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_ScheduleTask, 12)
void __thiscall ThreadScheduler_ScheduleTask(ThreadScheduler *this,
        void (__cdecl *proc)(void*), void* data)
{
    FIXME("(%p %p %p) stub\n", this, proc, data);
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_IsAvailableLocation, 8)
bool __thiscall ThreadScheduler_IsAvailableLocation(
        const ThreadScheduler *this, const /*location*/void *placement)
{
    FIXME("(%p %p) stub\n", this, placement);
    return FALSE;
}

DEFINE_THISCALL_WRAPPER(ThreadScheduler_vector_dtor, 8)
Scheduler* __thiscall ThreadScheduler_vector_dtor(ThreadScheduler *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ThreadScheduler_dtor(this+i);
        operator_delete(ptr);
    } else {
        ThreadScheduler_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return &this->scheduler;
}

static ThreadScheduler* ThreadScheduler_ctor(ThreadScheduler *this,
        const SchedulerPolicy *policy)
{
    SYSTEM_INFO si;

    TRACE("(%p)->()\n", this);

    this->scheduler.vtable = &ThreadScheduler_vtable;
    this->ref = 1;
    this->id = InterlockedIncrement(&scheduler_id);
    SchedulerPolicy_copy_ctor(&this->policy, policy);

    GetSystemInfo(&si);
    this->virt_proc_no = SchedulerPolicy_GetPolicyValue(&this->policy, MaxConcurrency);
    if(this->virt_proc_no > si.dwNumberOfProcessors)
        this->virt_proc_no = si.dwNumberOfProcessors;

    this->shutdown_count = this->shutdown_size = 0;
    this->shutdown_events = NULL;

    InitializeCriticalSection(&this->cs);
    this->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ThreadScheduler");
    return this;
}

/* ?Create@Scheduler@Concurrency@@SAPAV12@ABVSchedulerPolicy@2@@Z */
/* ?Create@Scheduler@Concurrency@@SAPEAV12@AEBVSchedulerPolicy@2@@Z */
Scheduler* __cdecl Scheduler_Create(const SchedulerPolicy *policy)
{
    ThreadScheduler *ret;

    TRACE("(%p)\n", policy);

    ret = operator_new(sizeof(*ret));
    return &ThreadScheduler_ctor(ret, policy)->scheduler;
}

/* ?ResetDefaultSchedulerPolicy@Scheduler@Concurrency@@SAXXZ */
void __cdecl Scheduler_ResetDefaultSchedulerPolicy(void)
{
    TRACE("()\n");

    EnterCriticalSection(&default_scheduler_cs);
    if(default_scheduler_policy.policy_container)
        SchedulerPolicy_dtor(&default_scheduler_policy);
    SchedulerPolicy_ctor(&default_scheduler_policy);
    LeaveCriticalSection(&default_scheduler_cs);
}

/* ?SetDefaultSchedulerPolicy@Scheduler@Concurrency@@SAXABVSchedulerPolicy@2@@Z */
/* ?SetDefaultSchedulerPolicy@Scheduler@Concurrency@@SAXAEBVSchedulerPolicy@2@@Z */
void __cdecl Scheduler_SetDefaultSchedulerPolicy(const SchedulerPolicy *policy)
{
    TRACE("(%p)\n", policy);

    EnterCriticalSection(&default_scheduler_cs);
    if(!default_scheduler_policy.policy_container)
        SchedulerPolicy_copy_ctor(&default_scheduler_policy, policy);
    else
        SchedulerPolicy_op_assign(&default_scheduler_policy, policy);
    LeaveCriticalSection(&default_scheduler_cs);
}

/* ?Create@CurrentScheduler@Concurrency@@SAXABVSchedulerPolicy@2@@Z */
/* ?Create@CurrentScheduler@Concurrency@@SAXAEBVSchedulerPolicy@2@@Z */
void __cdecl CurrentScheduler_Create(const SchedulerPolicy *policy)
{
    Scheduler *scheduler;

    TRACE("(%p)\n", policy);

    scheduler = Scheduler_Create(policy);
    call_Scheduler_Attach(scheduler);
}

/* ?Detach@CurrentScheduler@Concurrency@@SAXXZ */
void __cdecl CurrentScheduler_Detach(void)
{
    ExternalContextBase *context = (ExternalContextBase*)try_get_current_context();

    TRACE("()\n");

    if(!context)
        throw_exception(EXCEPTION_IMPROPER_SCHEDULER_DETACH, 0, NULL);

    if(context->context.vtable != &ExternalContextBase_vtable) {
        ERR("unknown context set\n");
        return;
    }

    if(!context->scheduler.next)
        throw_exception(EXCEPTION_IMPROPER_SCHEDULER_DETACH, 0, NULL);

    call_Scheduler_Release(context->scheduler.scheduler);
    if(!context->scheduler.next) {
        context->scheduler.scheduler = NULL;
    }else {
        struct scheduler_list *entry = context->scheduler.next;
        context->scheduler.scheduler = entry->scheduler;
        context->scheduler.next = entry->next;
        operator_delete(entry);
    }
}

static void create_default_scheduler(void)
{
    if(default_scheduler)
        return;

    EnterCriticalSection(&default_scheduler_cs);
    if(!default_scheduler) {
        ThreadScheduler *scheduler;

        if(!default_scheduler_policy.policy_container)
            SchedulerPolicy_ctor(&default_scheduler_policy);

        scheduler = operator_new(sizeof(*scheduler));
        ThreadScheduler_ctor(scheduler, &default_scheduler_policy);
        default_scheduler = scheduler;
    }
    LeaveCriticalSection(&default_scheduler_cs);
}

/* ?Get@CurrentScheduler@Concurrency@@SAPAVScheduler@2@XZ */
/* ?Get@CurrentScheduler@Concurrency@@SAPEAVScheduler@2@XZ */
Scheduler* __cdecl CurrentScheduler_Get(void)
{
    TRACE("()\n");
    return get_current_scheduler();
}

#if _MSVCR_VER > 100
/* ?CreateScheduleGroup@CurrentScheduler@Concurrency@@SAPAVScheduleGroup@2@AAVlocation@2@@Z */
/* ?CreateScheduleGroup@CurrentScheduler@Concurrency@@SAPEAVScheduleGroup@2@AEAVlocation@2@@Z */
/*ScheduleGroup*/void* __cdecl CurrentScheduler_CreateScheduleGroup_loc(/*location*/void *placement)
{
    TRACE("(%p)\n", placement);
    return call_Scheduler_CreateScheduleGroup_loc(get_current_scheduler(), placement);
}
#endif

/* ?CreateScheduleGroup@CurrentScheduler@Concurrency@@SAPAVScheduleGroup@2@XZ */
/* ?CreateScheduleGroup@CurrentScheduler@Concurrency@@SAPEAVScheduleGroup@2@XZ */
/*ScheduleGroup*/void* __cdecl CurrentScheduler_CreateScheduleGroup(void)
{
    TRACE("()\n");
    return call_Scheduler_CreateScheduleGroup(get_current_scheduler());
}

/* ?GetNumberOfVirtualProcessors@CurrentScheduler@Concurrency@@SAIXZ */
unsigned int __cdecl CurrentScheduler_GetNumberOfVirtualProcessors(void)
{
    Scheduler *scheduler = try_get_current_scheduler();

    TRACE("()\n");

    if(!scheduler)
        return -1;
    return call_Scheduler_GetNumberOfVirtualProcessors(scheduler);
}

/* ?GetPolicy@CurrentScheduler@Concurrency@@SA?AVSchedulerPolicy@2@XZ */
SchedulerPolicy* __cdecl CurrentScheduler_GetPolicy(SchedulerPolicy *policy)
{
    TRACE("(%p)\n", policy);
    return call_Scheduler_GetPolicy(get_current_scheduler(), policy);
}

/* ?Id@CurrentScheduler@Concurrency@@SAIXZ */
unsigned int __cdecl CurrentScheduler_Id(void)
{
    Scheduler *scheduler = try_get_current_scheduler();

    TRACE("()\n");

    if(!scheduler)
        return -1;
    return call_Scheduler_Id(scheduler);
}

#if _MSVCR_VER > 100
/* ?IsAvailableLocation@CurrentScheduler@Concurrency@@SA_NABVlocation@2@@Z */
/* ?IsAvailableLocation@CurrentScheduler@Concurrency@@SA_NAEBVlocation@2@@Z */
bool __cdecl CurrentScheduler_IsAvailableLocation(const /*location*/void *placement)
{
    Scheduler *scheduler = try_get_current_scheduler();

    TRACE("(%p)\n", placement);

    if(!scheduler)
        return FALSE;
    return call_Scheduler_IsAvailableLocation(scheduler, placement);
}
#endif

/* ?RegisterShutdownEvent@CurrentScheduler@Concurrency@@SAXPAX@Z */
/* ?RegisterShutdownEvent@CurrentScheduler@Concurrency@@SAXPEAX@Z */
void __cdecl CurrentScheduler_RegisterShutdownEvent(HANDLE event)
{
    TRACE("(%p)\n", event);
    call_Scheduler_RegisterShutdownEvent(get_current_scheduler(), event);
}

#if _MSVCR_VER > 100
/* ?ScheduleTask@CurrentScheduler@Concurrency@@SAXP6AXPAX@Z0AAVlocation@2@@Z */
/* ?ScheduleTask@CurrentScheduler@Concurrency@@SAXP6AXPEAX@Z0AEAVlocation@2@@Z */
void __cdecl CurrentScheduler_ScheduleTask_loc(void (__cdecl *proc)(void*),
        void *data, /*location*/void *placement)
{
    TRACE("(%p %p %p)\n", proc, data, placement);
    call_Scheduler_ScheduleTask_loc(get_current_scheduler(), proc, data, placement);
}
#endif

/* ?ScheduleTask@CurrentScheduler@Concurrency@@SAXP6AXPAX@Z0@Z */
/* ?ScheduleTask@CurrentScheduler@Concurrency@@SAXP6AXPEAX@Z0@Z */
void __cdecl CurrentScheduler_ScheduleTask(void (__cdecl *proc)(void*), void *data)
{
    TRACE("(%p %p)\n", proc, data);
    call_Scheduler_ScheduleTask(get_current_scheduler(), proc, data);
}

/* ??0_Scheduler@details@Concurrency@@QAE@PAVScheduler@2@@Z */
/* ??0_Scheduler@details@Concurrency@@QEAA@PEAVScheduler@2@@Z */
DEFINE_THISCALL_WRAPPER(_Scheduler_ctor_sched, 8)
_Scheduler* __thiscall _Scheduler_ctor_sched(_Scheduler *this, Scheduler *scheduler)
{
    TRACE("(%p %p)\n", this, scheduler);

    this->scheduler = scheduler;
    return this;
}

/* ??_F_Scheduler@details@Concurrency@@QAEXXZ */
/* ??_F_Scheduler@details@Concurrency@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Scheduler_ctor, 4)
_Scheduler* __thiscall _Scheduler_ctor(_Scheduler *this)
{
    return _Scheduler_ctor_sched(this, NULL);
}

/* ?_GetScheduler@_Scheduler@details@Concurrency@@QAEPAVScheduler@3@XZ */
/* ?_GetScheduler@_Scheduler@details@Concurrency@@QEAAPEAVScheduler@3@XZ */
DEFINE_THISCALL_WRAPPER(_Scheduler__GetScheduler, 4)
Scheduler* __thiscall _Scheduler__GetScheduler(_Scheduler *this)
{
    TRACE("(%p)\n", this);
    return this->scheduler;
}

/* ?_Reference@_Scheduler@details@Concurrency@@QAEIXZ */
/* ?_Reference@_Scheduler@details@Concurrency@@QEAAIXZ */
DEFINE_THISCALL_WRAPPER(_Scheduler__Reference, 4)
unsigned int __thiscall _Scheduler__Reference(_Scheduler *this)
{
    TRACE("(%p)\n", this);
    return call_Scheduler_Reference(this->scheduler);
}

/* ?_Release@_Scheduler@details@Concurrency@@QAEIXZ */
/* ?_Release@_Scheduler@details@Concurrency@@QEAAIXZ */
DEFINE_THISCALL_WRAPPER(_Scheduler__Release, 4)
unsigned int __thiscall _Scheduler__Release(_Scheduler *this)
{
    TRACE("(%p)\n", this);
    return call_Scheduler_Release(this->scheduler);
}

/* ?_Get@_CurrentScheduler@details@Concurrency@@SA?AV_Scheduler@23@XZ */
_Scheduler* __cdecl _CurrentScheduler__Get(_Scheduler *ret)
{
    TRACE("()\n");
    return _Scheduler_ctor_sched(ret, get_current_scheduler());
}

/* ?_GetNumberOfVirtualProcessors@_CurrentScheduler@details@Concurrency@@SAIXZ */
unsigned int __cdecl _CurrentScheduler__GetNumberOfVirtualProcessors(void)
{
    TRACE("()\n");
    get_current_scheduler();
    return CurrentScheduler_GetNumberOfVirtualProcessors();
}

/* ?_Id@_CurrentScheduler@details@Concurrency@@SAIXZ */
unsigned int __cdecl _CurrentScheduler__Id(void)
{
    TRACE("()\n");
    get_current_scheduler();
    return CurrentScheduler_Id();
}

/* ?_ScheduleTask@_CurrentScheduler@details@Concurrency@@SAXP6AXPAX@Z0@Z */
/* ?_ScheduleTask@_CurrentScheduler@details@Concurrency@@SAXP6AXPEAX@Z0@Z */
void __cdecl _CurrentScheduler__ScheduleTask(void (__cdecl *proc)(void*), void *data)
{
    TRACE("(%p %p)\n", proc, data);
    CurrentScheduler_ScheduleTask(proc, data);
}

#ifdef __ASM_USE_THISCALL_WRAPPER

#define DEFINE_VTBL_WRAPPER(off)            \
    __ASM_GLOBAL_FUNC(vtbl_wrapper_ ## off, \
        "popl %eax\n\t"                     \
        "popl %ecx\n\t"                     \
        "pushl %eax\n\t"                    \
        "movl 0(%ecx), %eax\n\t"            \
        "jmp *" #off "(%eax)\n\t")

DEFINE_VTBL_WRAPPER(0);
DEFINE_VTBL_WRAPPER(4);
DEFINE_VTBL_WRAPPER(8);
DEFINE_VTBL_WRAPPER(12);
DEFINE_VTBL_WRAPPER(16);
DEFINE_VTBL_WRAPPER(20);
DEFINE_VTBL_WRAPPER(24);
DEFINE_VTBL_WRAPPER(28);
DEFINE_VTBL_WRAPPER(32);
DEFINE_VTBL_WRAPPER(36);
DEFINE_VTBL_WRAPPER(40);
DEFINE_VTBL_WRAPPER(44);
DEFINE_VTBL_WRAPPER(48);

#endif

extern const vtable_ptr type_info_vtable;
DEFINE_RTTI_DATA0(Context, 0, ".?AVContext@Concurrency@@")
DEFINE_RTTI_DATA1(ContextBase, 0, &Context_rtti_base_descriptor, ".?AVContextBase@details@Concurrency@@")
DEFINE_RTTI_DATA2(ExternalContextBase, 0, &ContextBase_rtti_base_descriptor,
        &Context_rtti_base_descriptor, ".?AVExternalContextBase@details@Concurrency@@")
DEFINE_RTTI_DATA0(Scheduler, 0, ".?AVScheduler@Concurrency@@")
DEFINE_RTTI_DATA1(SchedulerBase, 0, &Scheduler_rtti_base_descriptor, ".?AVSchedulerBase@details@Concurrency@@")
DEFINE_RTTI_DATA2(ThreadScheduler, 0, &SchedulerBase_rtti_base_descriptor,
        &Scheduler_rtti_base_descriptor, ".?AVThreadScheduler@details@Concurrency@@")

__ASM_BLOCK_BEGIN(scheduler_vtables)
    __ASM_VTABLE(ExternalContextBase,
            VTABLE_ADD_FUNC(ExternalContextBase_GetId)
            VTABLE_ADD_FUNC(ExternalContextBase_GetVirtualProcessorId)
            VTABLE_ADD_FUNC(ExternalContextBase_GetScheduleGroupId)
            VTABLE_ADD_FUNC(ExternalContextBase_Unblock)
            VTABLE_ADD_FUNC(ExternalContextBase_IsSynchronouslyBlocked)
            VTABLE_ADD_FUNC(ExternalContextBase_vector_dtor));
    __ASM_VTABLE(ThreadScheduler,
            VTABLE_ADD_FUNC(ThreadScheduler_vector_dtor)
            VTABLE_ADD_FUNC(ThreadScheduler_Id)
            VTABLE_ADD_FUNC(ThreadScheduler_GetNumberOfVirtualProcessors)
            VTABLE_ADD_FUNC(ThreadScheduler_GetPolicy)
            VTABLE_ADD_FUNC(ThreadScheduler_Reference)
            VTABLE_ADD_FUNC(ThreadScheduler_Release)
            VTABLE_ADD_FUNC(ThreadScheduler_RegisterShutdownEvent)
            VTABLE_ADD_FUNC(ThreadScheduler_Attach)
#if _MSVCR_VER > 100
            VTABLE_ADD_FUNC(ThreadScheduler_CreateScheduleGroup_loc)
#endif
            VTABLE_ADD_FUNC(ThreadScheduler_CreateScheduleGroup)
#if _MSVCR_VER > 100
            VTABLE_ADD_FUNC(ThreadScheduler_ScheduleTask_loc)
#endif
            VTABLE_ADD_FUNC(ThreadScheduler_ScheduleTask)
#if _MSVCR_VER > 100
            VTABLE_ADD_FUNC(ThreadScheduler_IsAvailableLocation)
#endif
            );
__ASM_BLOCK_END

void msvcrt_init_scheduler(void *base)
{
#ifdef __x86_64__
    init_Context_rtti(base);
    init_ContextBase_rtti(base);
    init_ExternalContextBase_rtti(base);
    init_Scheduler_rtti(base);
    init_SchedulerBase_rtti(base);
    init_ThreadScheduler_rtti(base);
#endif
}

void msvcrt_free_scheduler(void)
{
    if (context_tls_index != TLS_OUT_OF_INDEXES)
        TlsFree(context_tls_index);
    if(default_scheduler_policy.policy_container)
        SchedulerPolicy_dtor(&default_scheduler_policy);
    if(default_scheduler) {
        ThreadScheduler_dtor(default_scheduler);
        operator_delete(default_scheduler);
    }
}

void msvcrt_free_scheduler_thread(void)
{
    Context *context = try_get_current_context();
    if (!context) return;
    call_Context_dtor(context, 1);
}

#endif /* _MSVCR_VER >= 100 */
