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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME:
 * - proper handling of 16-bit stack and signal stack
 */

#include <setjmp.h>

#include "winbase.h"
#include "winerror.h"
#include "thread.h"

struct fiber_data
{
    LPVOID                param;       /* 00 fiber param */
    void                 *except;      /* 04 saved exception handlers list */
    void                 *stack_top;   /* 08 top of fiber stack */
    void                 *stack_low;   /* 0c fiber stack low-water mark */
    void                 *stack_base;  /* 10 base of the fiber stack */
    jmp_buf               jmpbuf;      /* 14 setjmp buffer (on Windows: CONTEXT) */
    DWORD                 flags;       /*    fiber flags */
    LPFIBER_START_ROUTINE start;       /*    start routine */
};


/* call the fiber initial function once we have switched stack */
static void start_fiber(void)
{
    struct fiber_data *fiber = NtCurrentTeb()->fiber;
    LPFIBER_START_ROUTINE start = fiber->start;

    fiber->start = NULL;
    start( fiber->param );
    ExitThread( 1 );
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

    if (!(fiber = HeapAlloc( GetProcessHeap(), 0, sizeof(*fiber) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    /* FIXME: should use the thread stack allocation routines here */
    if (!stack_reserve) stack_reserve = 1024*1024;
    if(!(fiber->stack_base = VirtualAlloc( 0, stack_reserve, MEM_COMMIT, PAGE_EXECUTE_READWRITE )))
    {
        HeapFree( GetProcessHeap(), 0, fiber );
        return NULL;
    }
    fiber->stack_top  = (char *)fiber->stack_base + stack_reserve;
    fiber->stack_low  = fiber->stack_base;
    fiber->param      = param;
    fiber->except     = (void *)-1;
    fiber->start      = start;
    fiber->flags      = flags;
    return fiber;
}


/***********************************************************************
 *           DeleteFiber   (KERNEL32.@)
 */
void WINAPI DeleteFiber( LPVOID fiber_ptr )
{
    struct fiber_data *fiber = fiber_ptr;

    if (!fiber) return;
    if (fiber == NtCurrentTeb()->fiber)
    {
        HeapFree( GetProcessHeap(), 0, fiber );
        ExitThread(1);
    }
    VirtualFree( fiber->stack_base, 0, MEM_RELEASE );
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
    fiber->param      = param;
    fiber->except     = NtCurrentTeb()->except;
    fiber->stack_top  = NtCurrentTeb()->stack_top;
    fiber->stack_low  = NtCurrentTeb()->stack_low;
    fiber->stack_base = NtCurrentTeb()->stack_base;
    fiber->start      = NULL;
    fiber->flags      = flags;
    NtCurrentTeb()->fiber = fiber;
    return fiber;
}


/***********************************************************************
 *           ConvertFiberToThread   (KERNEL32.@)
 */
BOOL WINAPI ConvertFiberToThread(void)
{
    struct fiber_data *fiber = NtCurrentTeb()->fiber;

    if (fiber)
    {
        NtCurrentTeb()->fiber = NULL;
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
    struct fiber_data *current_fiber = NtCurrentTeb()->fiber;

    current_fiber->except    = NtCurrentTeb()->except;
    current_fiber->stack_low = NtCurrentTeb()->stack_low;
    /* stack_base and stack_top never change */

    /* FIXME: should save floating point context if requested in fiber->flags */
    if (!setjmp( current_fiber->jmpbuf ))
    {
        NtCurrentTeb()->fiber      = new_fiber;
        NtCurrentTeb()->except     = new_fiber->except;
        NtCurrentTeb()->stack_top  = new_fiber->stack_top;
        NtCurrentTeb()->stack_low  = new_fiber->stack_low;
        NtCurrentTeb()->stack_base = new_fiber->stack_base;
        if (new_fiber->start)  /* first time */
            SYSDEPS_SwitchToThreadStack( start_fiber );
        else
            longjmp( new_fiber->jmpbuf, 1 );
    }
}
