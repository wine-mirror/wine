/*
 * Fiber support
 *
 * Copyright 2002 Alexandre Julliard
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
 * FIXME:
 * - proper handling of 16-bit stack and signal stack
 */

/* Fortify source chokes on siglongjmp stack switching, so disable it */
#undef _FORTIFY_SOURCE
#define _FORTIFY_SOURCE 0

#include "config.h"
#include "wine/port.h"

#include <setjmp.h>
#include <stdarg.h>

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/library.h"

struct fiber_data
{
    LPVOID                param;             /* 00 fiber param */
    void                 *except;            /* 04 saved exception handlers list */
    void                 *stack_base;        /* 08 top of fiber stack */
    void                 *stack_limit;       /* 0c fiber stack low-water mark */
    void                 *stack_allocation;  /* 10 base of the fiber stack allocation */
    sigjmp_buf            jmpbuf;            /* 14 setjmp buffer (on Windows: CONTEXT) */
    DWORD                 flags;             /*    fiber flags */
    LPFIBER_START_ROUTINE start;             /*    start routine */
    void                **fls_slots;         /*    fiber storage slots */
};


/* call the fiber initial function once we have switched stack */
static void start_fiber( void *arg )
{
    struct fiber_data *fiber = arg;
    LPFIBER_START_ROUTINE start = fiber->start;

    __TRY
    {
        fiber->start = NULL;
        start( fiber->param );
        ExitThread( 1 );
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}


/***********************************************************************
 *           CreateFiber   (KERNEL32.@)
 */
LPVOID WINAPI CreateFiber( SIZE_T stack, LPFIBER_START_ROUTINE start, LPVOID param )
{
    return CreateFiberEx( stack, 0, 0, start, param );
}


/***********************************************************************
 *           CreateFiberEx   (KERNEL32.@)
 */
LPVOID WINAPI CreateFiberEx( SIZE_T stack_commit, SIZE_T stack_reserve, DWORD flags,
                             LPFIBER_START_ROUTINE start, LPVOID param )
{
    struct fiber_data *fiber;
    INITIAL_TEB stack;
    NTSTATUS status;

    if (!(fiber = HeapAlloc( GetProcessHeap(), 0, sizeof(*fiber) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    if ((status = RtlCreateUserStack( stack_commit, stack_reserve, 0, 1, 1, &stack )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    fiber->stack_allocation = stack.DeallocationStack;
    fiber->stack_base = stack.StackBase;
    fiber->stack_limit = stack.StackLimit;
    fiber->param       = param;
    fiber->except      = (void *)-1;
    fiber->start       = start;
    fiber->flags       = flags;
    fiber->fls_slots   = NULL;
    return fiber;
}


/***********************************************************************
 *           DeleteFiber   (KERNEL32.@)
 */
void WINAPI DeleteFiber( LPVOID fiber_ptr )
{
    struct fiber_data *fiber = fiber_ptr;

    if (!fiber) return;
    if (fiber == NtCurrentTeb()->Tib.u.FiberData)
    {
        HeapFree( GetProcessHeap(), 0, fiber );
        ExitThread(1);
    }
    RtlFreeUserStack( fiber->stack_allocation );
    HeapFree( GetProcessHeap(), 0, fiber->fls_slots );
    HeapFree( GetProcessHeap(), 0, fiber );
}


/***********************************************************************
 *           ConvertThreadToFiber   (KERNEL32.@)
 */
LPVOID WINAPI ConvertThreadToFiber( LPVOID param )
{
    return ConvertThreadToFiberEx( param, 0 );
}


/***********************************************************************
 *           ConvertThreadToFiberEx   (KERNEL32.@)
 */
LPVOID WINAPI ConvertThreadToFiberEx( LPVOID param, DWORD flags )
{
    struct fiber_data *fiber;

    if (!(fiber = HeapAlloc( GetProcessHeap(), 0, sizeof(*fiber) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    fiber->param            = param;
    fiber->except           = NtCurrentTeb()->Tib.ExceptionList;
    fiber->stack_base       = NtCurrentTeb()->Tib.StackBase;
    fiber->stack_limit      = NtCurrentTeb()->Tib.StackLimit;
    fiber->stack_allocation = NtCurrentTeb()->DeallocationStack;
    fiber->start            = NULL;
    fiber->flags            = flags;
    fiber->fls_slots        = NtCurrentTeb()->FlsSlots;
    NtCurrentTeb()->Tib.u.FiberData = fiber;
    return fiber;
}


/***********************************************************************
 *           ConvertFiberToThread   (KERNEL32.@)
 */
BOOL WINAPI ConvertFiberToThread(void)
{
    struct fiber_data *fiber = NtCurrentTeb()->Tib.u.FiberData;

    if (fiber)
    {
        NtCurrentTeb()->Tib.u.FiberData = NULL;
        HeapFree( GetProcessHeap(), 0, fiber );
    }
    return TRUE;
}


/***********************************************************************
 *           SwitchToFiber   (KERNEL32.@)
 */
void WINAPI SwitchToFiber( LPVOID fiber )
{
    struct fiber_data *new_fiber = fiber;
    struct fiber_data *current_fiber = NtCurrentTeb()->Tib.u.FiberData;

    current_fiber->except      = NtCurrentTeb()->Tib.ExceptionList;
    current_fiber->stack_limit = NtCurrentTeb()->Tib.StackLimit;
    current_fiber->fls_slots   = NtCurrentTeb()->FlsSlots;
    /* stack_allocation and stack_base never change */

    /* FIXME: should save floating point context if requested in fiber->flags */
    if (!sigsetjmp( current_fiber->jmpbuf, 0 ))
    {
        NtCurrentTeb()->Tib.u.FiberData   = new_fiber;
        NtCurrentTeb()->Tib.ExceptionList = new_fiber->except;
        NtCurrentTeb()->Tib.StackBase     = new_fiber->stack_base;
        NtCurrentTeb()->Tib.StackLimit    = new_fiber->stack_limit;
        NtCurrentTeb()->DeallocationStack = new_fiber->stack_allocation;
        NtCurrentTeb()->FlsSlots          = new_fiber->fls_slots;
        if (new_fiber->start)  /* first time */
            wine_switch_to_stack( start_fiber, new_fiber, new_fiber->stack_base );
        else
            siglongjmp( new_fiber->jmpbuf, 1 );
    }
}

/***********************************************************************
 *           FlsAlloc   (KERNEL32.@)
 */
DWORD WINAPI FlsAlloc( PFLS_CALLBACK_FUNCTION callback )
{
    DWORD index;
    PEB * const peb = NtCurrentTeb()->Peb;

    RtlAcquirePebLock();
    if (!peb->FlsCallback &&
        !(peb->FlsCallback = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        8 * sizeof(peb->FlsBitmapBits) * sizeof(void*) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        index = FLS_OUT_OF_INDEXES;
    }
    else
    {
        index = RtlFindClearBitsAndSet( peb->FlsBitmap, 1, 1 );
        if (index != ~0U)
        {
            if (!NtCurrentTeb()->FlsSlots &&
                !(NtCurrentTeb()->FlsSlots = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                        8 * sizeof(peb->FlsBitmapBits) * sizeof(void*) )))
            {
                RtlClearBits( peb->FlsBitmap, index, 1 );
                index = FLS_OUT_OF_INDEXES;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }
            else
            {
                NtCurrentTeb()->FlsSlots[index] = 0; /* clear the value */
                peb->FlsCallback[index] = callback;
            }
        }
        else SetLastError( ERROR_NO_MORE_ITEMS );
    }
    RtlReleasePebLock();
    return index;
}

/***********************************************************************
 *           FlsFree   (KERNEL32.@)
 */
BOOL WINAPI FlsFree( DWORD index )
{
    BOOL ret;

    RtlAcquirePebLock();
    ret = RtlAreBitsSet( NtCurrentTeb()->Peb->FlsBitmap, index, 1 );
    if (ret) RtlClearBits( NtCurrentTeb()->Peb->FlsBitmap, index, 1 );
    if (ret)
    {
        /* FIXME: call Fls callback */
        /* FIXME: add equivalent of ThreadZeroTlsCell here */
        if (NtCurrentTeb()->FlsSlots) NtCurrentTeb()->FlsSlots[index] = 0;
    }
    else SetLastError( ERROR_INVALID_PARAMETER );
    RtlReleasePebLock();
    return ret;
}

/***********************************************************************
 *           FlsGetValue   (KERNEL32.@)
 */
PVOID WINAPI FlsGetValue( DWORD index )
{
    if (!index || index >= 8 * sizeof(NtCurrentTeb()->Peb->FlsBitmapBits) || !NtCurrentTeb()->FlsSlots)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return NtCurrentTeb()->FlsSlots[index];
}

/***********************************************************************
 *           FlsSetValue   (KERNEL32.@)
 */
BOOL WINAPI FlsSetValue( DWORD index, PVOID data )
{
    if (!index || index >= 8 * sizeof(NtCurrentTeb()->Peb->FlsBitmapBits))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!NtCurrentTeb()->FlsSlots &&
        !(NtCurrentTeb()->FlsSlots = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        8 * sizeof(NtCurrentTeb()->Peb->FlsBitmapBits) * sizeof(void*) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    NtCurrentTeb()->FlsSlots[index] = data;
    return TRUE;
}

/***********************************************************************
 *           IsThreadAFiber   (KERNEL32.@)
 */
BOOL WINAPI IsThreadAFiber(void)
{
    return NtCurrentTeb()->Tib.u.FiberData != NULL;
}
