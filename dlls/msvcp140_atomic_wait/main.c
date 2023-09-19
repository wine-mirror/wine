/*
 * Copyright 2022 Daniel Lehman
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
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

extern unsigned int __cdecl _Thrd_hardware_concurrency(void);

unsigned int __stdcall __std_parallel_algorithms_hw_threads(void)
{
    TRACE("()\n");
    return _Thrd_hardware_concurrency();
}

void __stdcall __std_bulk_submit_threadpool_work(PTP_WORK work, size_t count)
{
    TRACE("(%p %Iu)\n", work, count);
    while (count--)
        SubmitThreadpoolWork(work);
}

void __stdcall __std_close_threadpool_work(PTP_WORK work)
{
    TRACE("(%p)\n", work);
    return CloseThreadpoolWork(work);
}

PTP_WORK __stdcall __std_create_threadpool_work(PTP_WORK_CALLBACK callback, void *context,
                                                PTP_CALLBACK_ENVIRON environ)
{
    TRACE("(%p %p %p)\n", callback, context, environ);
    return CreateThreadpoolWork(callback, context, environ);
}

void __stdcall __std_submit_threadpool_work(PTP_WORK work)
{
    TRACE("(%p)\n", work);
    return SubmitThreadpoolWork(work);
}

void __stdcall __std_wait_for_threadpool_work_callbacks(PTP_WORK work, BOOL cancel)
{
    TRACE("(%p %d)\n", work, cancel);
    return WaitForThreadpoolWorkCallbacks(work, cancel);
}

void __stdcall __std_atomic_notify_one_direct(void *addr)
{
    TRACE("(%p)\n", addr);
    WakeByAddressSingle(addr);
}

void __stdcall __std_atomic_notify_all_direct(void *addr)
{
    TRACE("(%p)\n", addr);
    WakeByAddressAll(addr);
}

BOOL __stdcall __std_atomic_wait_direct(volatile void *addr, void *cmp,
                                        SIZE_T size, DWORD timeout)
{
    TRACE("(%p %p %Id %ld)\n", addr, cmp, size, timeout);
    return WaitOnAddress(addr, cmp, size, timeout);
}

typedef struct
{
    SRWLOCK srwlock;
} shared_mutex;

struct shared_mutex_elem
{
    struct list entry;
    int ref;
    void *ptr;
    shared_mutex mutex;
};

static CRITICAL_SECTION shared_mutex_cs;
static CRITICAL_SECTION_DEBUG shared_mutex_cs_debug =
{
    0, 0, &shared_mutex_cs,
    { &shared_mutex_cs_debug.ProcessLocksList, &shared_mutex_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": shared_mutex_cs") }
};
static CRITICAL_SECTION shared_mutex_cs = { &shared_mutex_cs_debug, -1, 0, 0, 0, 0 };
static struct list shared_mutex_list = LIST_INIT(shared_mutex_list);

/* shared_mutex_cs must be held by caller */
static struct shared_mutex_elem* find_shared_mutex(void *ptr)
{
    struct shared_mutex_elem *sme;

    LIST_FOR_EACH_ENTRY(sme, &shared_mutex_list, struct shared_mutex_elem, entry)
    {
        if (sme->ptr == ptr)
            return sme;
    }
    return NULL;
}

shared_mutex* __stdcall __std_acquire_shared_mutex_for_instance(void *ptr)
{
    struct shared_mutex_elem *sme;

    TRACE("(%p)\n", ptr);

    EnterCriticalSection(&shared_mutex_cs);
    sme = find_shared_mutex(ptr);
    if (sme)
    {
        sme->ref++;
        LeaveCriticalSection(&shared_mutex_cs);
        return &sme->mutex;
    }

    sme = malloc(sizeof(*sme));
    sme->ref = 1;
    sme->ptr = ptr;
    InitializeSRWLock(&sme->mutex.srwlock);
    list_add_head(&shared_mutex_list, &sme->entry);
    LeaveCriticalSection(&shared_mutex_cs);
    return &sme->mutex;
}

void __stdcall __std_release_shared_mutex_for_instance(void *ptr)
{
    struct shared_mutex_elem *sme;

    TRACE("(%p)\n", ptr);

    EnterCriticalSection(&shared_mutex_cs);
    sme = find_shared_mutex(ptr);
    if (!sme)
    {
        LeaveCriticalSection(&shared_mutex_cs);
        return;
    }

    sme->ref--;
    if (!sme->ref)
    {
        list_remove(&sme->entry);
        free(sme);
    }
    LeaveCriticalSection(&shared_mutex_cs);
}
