/*
 * Helpers used for WinRT event dispatch.
 *
 * Copyright 2025 Vibhav Pant
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
 *
 * These functions are used to implement WinRT event handling using delegates.
 * A delegate is an ITypedEventHandler<T> object with an Invoke method. Components
 * register delegates with an event source, and receive an EventRegistrationToken in return.
 * The token can be later used to unregister the delegate in order to stop receiving events.
 */

#define COBJMACROS
#include "eventtoken.h"
#include "inspectable.h"
#include "weakreference.h"

#include "wine/exception.h"
#include "wine/debug.h"

#include "cxx.h"
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

void *__cdecl Delegate_ctor(void *this)
{
    FIXME("(%p): stub!\n", this);
    return NULL;
}

void WINAPI EventSourceInitialize(IInspectable **source)
{
    TRACE("(%p)\n", source);
    *source = NULL;
}

void WINAPI EventSourceUninitialize(IInspectable **source)
{
    TRACE("(%p)\n", source);

    if (*source)
        IInspectable_Release(*source);
    *source = NULL;
}

/* Probably not binary compatible with native, but it shouldn't be needed as programs only modify the delegate list with
 * EventSourceAdd/Remove, and read it though EventSourceGetTargetArrayEvent/Size. */
struct EventTargetArray
{
    IInspectable IInspectable_iface;
    IWeakReferenceSource IWeakReferenceSource_iface;
    struct control_block *block;
    IUnknown *marshal;
    ULONG count;
    /* Use agile references heres so that delegates can be invoked from any apartment. */
    IAgileReference *delegates[1];
};

static inline struct EventTargetArray *impl_from_IInspectable(IInspectable *iface)
{
    return CONTAINING_RECORD(iface, struct EventTargetArray, IInspectable_iface);
}

static HRESULT WINAPI EventTargetArray_QueryInterface(IInspectable *iface, const GUID *iid, void **out)
{
    struct EventTargetArray *impl = impl_from_IInspectable(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject))
    {
        IInspectable_AddRef((*out = &impl->IInspectable_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IWeakReferenceSource))
    {
        IWeakReferenceSource_AddRef((*out = &impl->IWeakReferenceSource_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IMarshal))
        return IUnknown_QueryInterface(impl->marshal, iid, out);

    *out = NULL;
    ERR("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI EventTargetArray_AddRef(IInspectable *iface)
{
    struct EventTargetArray *impl = impl_from_IInspectable(iface);
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&impl->block->ref_strong);
}

static ULONG WINAPI EventTargetArray_Release(IInspectable *iface)
{
    struct EventTargetArray *impl = impl_from_IInspectable(iface);
    struct control_block *block;
    ULONG ref;

    TRACE("(%p)\n", iface);

    block = impl->block;
    if (!(ref = InterlockedDecrement(&block->ref_strong)))
    {
        ULONG i;

        for (i = 0; i < impl->count; i++)
            IAgileReference_Release(impl->delegates[i]);
        IUnknown_Release(impl->marshal);
        /* ReleaseTarget will only free the object if ref_strong < 0. */
        WriteNoFence(&impl->block->ref_strong, -1);
        control_block_ReleaseTarget(impl->block);
        IWeakReference_Release(&block->IWeakReference_iface);
    }
    return ref;
}

static HRESULT WINAPI EventTargetArray_GetIids(IInspectable *iface, ULONG *ret_count, IID **ret_iids)
{
    IID *iids;

    TRACE("(%p, %p, %p)\n", iface, ret_count, ret_iids);

    if (!(iids = CoTaskMemAlloc(2 * sizeof(*iids))))
        return E_OUTOFMEMORY;

    iids[0] = IID_IInspectable;
    iids[1] = IID_IWeakReferenceSource;
    *ret_iids = iids;
    *ret_count = 2;
    return S_OK;
}

static HRESULT WINAPI EventTargetArray_GetRuntimeClassName(IInspectable *iface, HSTRING *name)
{
    TRACE("(%p, %p)\n", iface, name);
    /* Native returns an empty string. */
    *name = NULL;
    return S_OK;
}

static HRESULT WINAPI EventTargetArray_GetTrustLevel(IInspectable *iface, TrustLevel *level)
{
    FIXME("(%p, %p): stub!\n", iface, level);
    return E_NOTIMPL;
}

DEFINE_RTTI_DATA(EventTargetArray, 0, ".?AVEventTargetArray@Details@Platform@@");
COM_VTABLE_RTTI_START(IInspectable, EventTargetArray)
COM_VTABLE_ENTRY(EventTargetArray_QueryInterface)
COM_VTABLE_ENTRY(EventTargetArray_AddRef)
COM_VTABLE_ENTRY(EventTargetArray_Release)
COM_VTABLE_ENTRY(EventTargetArray_GetIids)
COM_VTABLE_ENTRY(EventTargetArray_GetRuntimeClassName)
COM_VTABLE_ENTRY(EventTargetArray_GetTrustLevel)
COM_VTABLE_RTTI_END;

static inline struct EventTargetArray *impl_from_IWeakReferenceSource(IWeakReferenceSource *iface)
{
    return CONTAINING_RECORD(iface, struct EventTargetArray, IWeakReferenceSource_iface);
}

static HRESULT WINAPI EventTargetArray_weakref_src_QueryInterface(IWeakReferenceSource *iface, const GUID *iid, void **out)
{
    struct EventTargetArray *impl = impl_from_IWeakReferenceSource(iface);
    return IInspectable_QueryInterface(&impl->IInspectable_iface, iid, out);
}

static ULONG WINAPI EventTargetArray_weakref_src_AddRef(IWeakReferenceSource *iface)
{
    struct EventTargetArray *impl = impl_from_IWeakReferenceSource(iface);
    return IInspectable_AddRef(&impl->IInspectable_iface);
}

static ULONG WINAPI EventTargetArray_weakref_src_Release(IWeakReferenceSource *iface)
{
    struct EventTargetArray *impl = impl_from_IWeakReferenceSource(iface);
    return IInspectable_Release(&impl->IInspectable_iface);
}

static HRESULT WINAPI EventTargetArray_weakref_src_GetWeakReference(IWeakReferenceSource *iface, IWeakReference **out)
{
    struct EventTargetArray *impl = impl_from_IWeakReferenceSource(iface);

    TRACE("(%p, %p)\n", iface, out);
    IWeakReference_AddRef((*out = &impl->block->IWeakReference_iface));
    return S_OK;
}

DEFINE_RTTI_DATA(EventTargetArray_weakref_src,
                 offsetof(struct EventTargetArray, IWeakReferenceSource_iface),
                 ".?AVEventTargetArray@Details@Platform@@");
COM_VTABLE_RTTI_START(IWeakReferenceSource, EventTargetArray_weakref_src)
COM_VTABLE_ENTRY(EventTargetArray_weakref_src_QueryInterface)
COM_VTABLE_ENTRY(EventTargetArray_weakref_src_AddRef)
COM_VTABLE_ENTRY(EventTargetArray_weakref_src_Release)
COM_VTABLE_ENTRY(EventTargetArray_weakref_src_GetWeakReference)
COM_VTABLE_RTTI_END;

void init_delegate(void *base)
{
    INIT_RTTI(EventTargetArray, base);
    INIT_RTTI(EventTargetArray_weakref_src, base);
}

static struct EventTargetArray *create_EventTargetArray(ULONG delegates)
{
    struct EventTargetArray *impl;
    HRESULT hr;

    impl = AllocateWithWeakRef(offsetof(struct EventTargetArray, block),
                               offsetof(struct EventTargetArray, delegates[delegates]));
    impl->IInspectable_iface.lpVtbl = &EventTargetArray_vtable.vtable;
    impl->IWeakReferenceSource_iface.lpVtbl = &EventTargetArray_weakref_src_vtable.vtable;
    if (FAILED(hr = CoCreateFreeThreadedMarshaler((IUnknown *)&impl->IInspectable_iface, &impl->marshal)))
    {
        struct control_block *block = impl->block;

        WriteNoFence(&impl->block->ref_strong, -1);
        control_block_ReleaseTarget(block);
        IWeakReference_Release(&block->IWeakReference_iface);
        __abi_WinRTraiseCOMException(hr);
    }
    impl->count = delegates;
    return impl;
}

/* According to MSDN's C++/CX documentation, this consists of two locks.
 * The exact usage of these locks is still unclear. */
struct EventLock
{
    /* Protects access to the EventTargetArray object.
     * During event dispatch, programs hold a read-lock on this before iterating through the delegate list.
     * We also hold this before replacing the old EventTargetArray object in EventSourceAdd/Remove, but I'm
     * unsure if native does this as well. */
    SRWLOCK targets_ptr_lock;
    /* Protects modification of the delegate list.
     * EventSourceAdd/Remove will hold a write-lock on this. */
    SRWLOCK add_remove_lock;
};
struct EventSourceAdd_cleanup_ctx
{
    IAgileReference *agile;
    SRWLOCK *add_remove_lock;
};

static CALLBACK void EventSourceAdd_cleanup(BOOL normal, void *param)
{
    struct EventSourceAdd_cleanup_ctx *ctx = param;

    IAgileReference_Release(ctx->agile);
    ReleaseSRWLockExclusive(ctx->add_remove_lock);
}

/* Create a new EventTargetArray object with the newly added delegate and place it in *source, releasing the earlier
 * value (if any). Returns the token associated with the added delegate. */
EventRegistrationToken WINAPI EventSourceAdd(struct EventTargetArray **source, struct EventLock *lock, IUnknown *delegate)
{
    struct EventTargetArray *impl = NULL, *new_impl;
    struct EventSourceAdd_cleanup_ctx finally_ctx;
    EventRegistrationToken token;
    IAgileReference *agile;
    ULONG i, old_count = 0;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", source, lock, delegate);

    /* Native throws InvalidArgumentException if delegate is NULL. */
    if (!delegate)
        __abi_WinRTraiseInvalidArgumentException();

    /* Get an agile reference outside the __TRY block to make cleanup simpler.
     * FIXME: This uses eager marshaling, which marshals the interface right now. Native seems to use delayed/lazy marshaling, as
     * that only increases the refcount on the delegate by 1 (eager marshaling increases it by 2). However, our current
     * implementation of lazy marshaling is incomplete, as lazy marshaling needs to occur inside the apartment/context that
     * the object originally belongs to, instead of the caller's apartment. */
    if (FAILED((hr = RoGetAgileReference(AGILEREFERENCE_DEFAULT, &IID_IUnknown, delegate, &agile))))
        __abi_WinRTraiseCOMException(hr);
    finally_ctx.agile = agile;

    AcquireSRWLockExclusive((finally_ctx.add_remove_lock = &lock->add_remove_lock));
    if ((impl = *source))
        old_count = impl->count;
    __TRY
    {
        new_impl = create_EventTargetArray(old_count + 1);
        for (i = 0; i < old_count; i++)
            IAgileReference_AddRef((new_impl->delegates[i] = impl->delegates[i]));
        IAgileReference_AddRef((new_impl->delegates[old_count] = agile)); /* Additional AddRef for cleanup. */
        /* Using the IAgileReference address as the token saves us from having an additional stable token field for every
         * delegate. */
        token.value = (ULONG_PTR)agile;
        /* Finally, update source and release the old list.
         * Programs will acquire a read lock on targets_ptr_lock before iterating through the delegate list while
         * dispatching events. */
        AcquireSRWLockExclusive(&lock->targets_ptr_lock);
        *source = new_impl;
        ReleaseSRWLockExclusive(&lock->targets_ptr_lock);
        if (impl)
            IInspectable_Release(&impl->IInspectable_iface);
    }
    __FINALLY_CTX(EventSourceAdd_cleanup, &finally_ctx)

    return token;
}

static void CALLBACK EventSourceRemove_cleanup(BOOL normal, void *lock)
{
    ReleaseSRWLockExclusive(lock);
}

/* Create a new EventTargetArray without the delegate associated with the provided token and place it in *source,
 * releasing the earlier value (if any). Does nothing if *source is NULL or if the token is invalid. */
void WINAPI EventSourceRemove(struct EventTargetArray **source, struct EventLock *lock, EventRegistrationToken token)
{
    IAgileReference *delegate = NULL;
    struct EventTargetArray *impl;
    ULONG i, j;

    TRACE("(%p, %p, {%I64u}): stub!\n", source, lock, token.value);

    if (!(impl = *source))
        return;

    AcquireSRWLockExclusive(&lock->add_remove_lock);
    for (i = 0; i < impl->count; i++)
    {
        if (token.value == (ULONG_PTR)impl->delegates[i])
        {
            delegate = impl->delegates[i];
            break;
        }
    }
    if (!delegate)
    {
        ReleaseSRWLockExclusive(&lock->add_remove_lock);
        return;
    }
    __TRY
    {
        /* If the only delegate has been removed, native sets source back to NULL. */
        struct EventTargetArray *new_impl = NULL;

        if (impl->count > 1)
        {
            new_impl = create_EventTargetArray(impl->count - 1);
            for (j = 0; j < i; j++)
                IAgileReference_AddRef((new_impl->delegates[j] = impl->delegates[j]));
            for (j = i; j < impl->count - 1; j++)
                IAgileReference_AddRef((new_impl->delegates[j] = impl->delegates[j + 1]));
        }

        AcquireSRWLockExclusive(&lock->targets_ptr_lock);
        *source = new_impl;
        ReleaseSRWLockExclusive(&lock->targets_ptr_lock);
        IInspectable_Release(&impl->IInspectable_iface);
    }
    __FINALLY_CTX(EventSourceRemove_cleanup, &lock->add_remove_lock);
}

/* Used before dispatching an event. By incrementing the refcount on the EventTargetArray, the caller gets a "snapshot"
 * of the currently registered delegates it can safetly invoke. */
struct EventTargetArray *WINAPI EventSourceGetTargetArray(struct EventTargetArray *targets, struct EventLock *lock)
{
    TRACE("(%p, %p)\n", targets, lock);

    /* TODO: The lock pointer received by this function is different from what EventSourceAdd/Remove get, so it's
     * probably guarding something else. Figure out what the lock is for. Holding targets_ptr_lock before calling this
     * function shows that native holds a read-lock on targets_ptr_lock, so we'll do the same.
     * The only thing we do here is increment the refcount, so do it inside the shared lock, even though it's
     * unnecessary. */
    AcquireSRWLockShared(&lock->targets_ptr_lock);
    if (targets)
        IInspectable_AddRef((&targets->IInspectable_iface));
    ReleaseSRWLockShared(&lock->targets_ptr_lock);
    return targets;
}

void *WINAPI EventSourceGetTargetArrayEvent(struct EventTargetArray *targets, ULONG idx, const GUID *iid, EventRegistrationToken *token)
{
    HRESULT hr;
    void *out;

    TRACE("(%p, %lu, %s, %p)\n", targets, idx, debugstr_guid(iid), token);

    if (FAILED((hr = IAgileReference_Resolve(targets->delegates[idx], iid, &out))))
        __abi_WinRTraiseCOMException(hr);
    token->value = (ULONG_PTR)targets->delegates[idx];
    return out;
}

ULONG WINAPI EventSourceGetTargetArraySize(struct EventTargetArray *targets)
{
    TRACE("(%p)\n", targets);
    return targets->count;
}
