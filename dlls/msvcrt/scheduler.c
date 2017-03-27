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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "msvcrt.h"
#include "cppexcept.h"
#include "cxx.h"

#if _MSVCR_VER >= 100

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static int context_id = -1;

#ifdef __i386__

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
DEFINE_VTBL_WRAPPER(20);

#endif

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

typedef struct {
    Context context;
    unsigned int id;
    union allocator_cache_entry *allocator_cache[8];
} ExternalContextBase;
extern const vtable_ptr MSVCRT_ExternalContextBase_vtable;
static void ExternalContextBase_ctor(ExternalContextBase*);

static int context_tls_index = TLS_OUT_OF_INDEXES;

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
        ExternalContextBase *context = MSVCRT_operator_new(sizeof(ExternalContextBase));
        ExternalContextBase_ctor(context);
        TlsSetValue(context_tls_index, context);
        ret = &context->context;
    }
    return ret;
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
MSVCRT_bool __cdecl Context_IsCurrentTaskCollectionCanceling(void)
{
    FIXME("()\n");
    return FALSE;
}

/* ?Oversubscribe@Context@Concurrency@@SAX_N@Z */
void __cdecl Context_Oversubscribe(MSVCRT_bool begin)
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
    FIXME("()\n");
    return ctx ? call_Context_GetVirtualProcessorId(ctx) : -1;
}

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
MSVCRT_bool __thiscall ExternalContextBase_IsSynchronouslyBlocked(const ExternalContextBase *this)
{
    FIXME("(%p)->() stub\n", this);
    return FALSE;
}

static void ExternalContextBase_dtor(ExternalContextBase *this)
{
    union allocator_cache_entry *next, *cur;
    int i;

    /* TODO: move the allocator cache to scheduler so it can be reused */
    for(i=0; i<sizeof(this->allocator_cache)/sizeof(this->allocator_cache[0]); i++) {
        for(cur = this->allocator_cache[i]; cur; cur=next) {
            next = cur->free.next;
            MSVCRT_operator_delete(cur);
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
        MSVCRT_operator_delete(ptr);
    } else {
        ExternalContextBase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return &this->context;
}

static void ExternalContextBase_ctor(ExternalContextBase *this)
{
    TRACE("(%p)->()\n", this);

    this->context.vtable = &MSVCRT_ExternalContextBase_vtable;
    this->id = InterlockedIncrement(&context_id);
    memset(this->allocator_cache, 0, sizeof(this->allocator_cache));
}

/* ?Alloc@Concurrency@@YAPAXI@Z */
/* ?Alloc@Concurrency@@YAPEAX_K@Z */
void * CDECL Concurrency_Alloc(MSVCRT_size_t size)
{
    ExternalContextBase *context = (ExternalContextBase*)get_current_context();
    union allocator_cache_entry *p;

    size += FIELD_OFFSET(union allocator_cache_entry, alloc.mem);
    if (size < sizeof(*p))
        size = sizeof(*p);

    if (context->context.vtable != &MSVCRT_ExternalContextBase_vtable) {
        p = MSVCRT_operator_new(size);
        p->alloc.bucket = -1;
    }else {
        int i;

        C_ASSERT(sizeof(union allocator_cache_entry) <= 1 << 4);
        for(i=0; i<sizeof(context->allocator_cache)/sizeof(context->allocator_cache[0]); i++)
            if (1 << (i+4) >= size) break;

        if(i==sizeof(context->allocator_cache)/sizeof(context->allocator_cache[0])) {
            p = MSVCRT_operator_new(size);
            p->alloc.bucket = -1;
        }else if (context->allocator_cache[i]) {
            p = context->allocator_cache[i];
            context->allocator_cache[i] = p->free.next;
            p->alloc.bucket = i;
        }else {
            p = MSVCRT_operator_new(1 << (i+4));
            p->alloc.bucket = i;
        }
    }

    TRACE("(%ld) returning %p\n", size, p->alloc.mem);
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

    if (context->context.vtable != &MSVCRT_ExternalContextBase_vtable) {
        MSVCRT_operator_delete(p);
    }else {
        if(bucket >= 0 && bucket < sizeof(context->allocator_cache)/sizeof(context->allocator_cache[0]) &&
            (!context->allocator_cache[bucket] || context->allocator_cache[bucket]->free.depth < 20)) {
            p->free.next = context->allocator_cache[bucket];
            p->free.depth = p->free.next ? p->free.next->free.depth+1 : 0;
            context->allocator_cache[bucket] = p;
        }else {
            MSVCRT_operator_delete(p);
        }
    }
}

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
    if (policy < SchedulerKind || policy >= last_policy_id)
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

    if (policy < SchedulerKind || policy >= last_policy_id)
        throw_exception(EXCEPTION_INVALID_SCHEDULER_POLICY_KEY, 0, "Invalid policy");
    return this->policy_container->policies[policy];
}

/* ??0SchedulerPolicy@Concurrency@@QAE@XZ */
/* ??0SchedulerPolicy@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(SchedulerPolicy_ctor, 4)
SchedulerPolicy* __thiscall SchedulerPolicy_ctor(SchedulerPolicy *this)
{
    TRACE("(%p)\n", this);

    this->policy_container = MSVCRT_operator_new(sizeof(*this->policy_container));
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
SchedulerPolicy* __cdecl SchedulerPolicy_ctor_policies(
        SchedulerPolicy *this, MSVCRT_size_t n, ...)
{
    unsigned int min_concurrency, max_concurrency;
    __ms_va_list valist;
    MSVCRT_size_t i;

    TRACE("(%p %ld)\n", this, n);

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
    MSVCRT_operator_delete(this->policy_container);
}

extern const vtable_ptr MSVCRT_type_info_vtable;
DEFINE_RTTI_DATA0(Context, 0, ".?AVContext@Concurrency@@")
DEFINE_RTTI_DATA1(ContextBase, 0, &Context_rtti_base_descriptor, ".?AVContextBase@details@Concurrency@@")
DEFINE_RTTI_DATA2(ExternalContextBase, 0, &ContextBase_rtti_base_descriptor,
        &Context_rtti_base_descriptor, ".?AVExternalContextBase@details@Concurrency@@")

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(ExternalContextBase,
            VTABLE_ADD_FUNC(ExternalContextBase_GetId)
            VTABLE_ADD_FUNC(ExternalContextBase_GetVirtualProcessorId)
            VTABLE_ADD_FUNC(ExternalContextBase_GetScheduleGroupId)
            VTABLE_ADD_FUNC(ExternalContextBase_Unblock)
            VTABLE_ADD_FUNC(ExternalContextBase_IsSynchronouslyBlocked)
            VTABLE_ADD_FUNC(ExternalContextBase_vector_dtor));
#ifndef __GNUC__
}
#endif

void msvcrt_init_scheduler(void *base)
{
#ifdef __x86_64__
    init_Context_rtti(base);
    init_ContextBase_rtti(base);
    init_ExternalContextBase_rtti(base);
#endif
}

void msvcrt_free_scheduler(void)
{
    if (context_tls_index != TLS_OUT_OF_INDEXES)
        TlsFree(context_tls_index);
}

void msvcrt_free_scheduler_thread(void)
{
    Context *context = try_get_current_context();
    if (!context) return;
    call_Context_dtor(context, 1);
}

#endif /* _MSVCR_VER >= 100 */
