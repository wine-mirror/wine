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

#include "config.h"

#include <stdarg.h>

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"

struct fiber_data
{
    LPVOID                param;             /* 00/00 fiber param */
    void                 *except;            /* 04/08 saved exception handlers list */
    void                 *stack_base;        /* 08/10 top of fiber stack */
    void                 *stack_limit;       /* 0c/18 fiber stack low-water mark */
    void                 *stack_allocation;  /* 10/20 base of the fiber stack allocation */
    CONTEXT               context;           /* 14/30 fiber context */
    DWORD                 flags;             /*       fiber flags */
    LPFIBER_START_ROUTINE start;             /*       start routine */
    void                **fls_slots;         /*       fiber storage slots */
};


extern void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new );
#ifdef __i386__
__ASM_STDCALL_FUNC( switch_fiber, 8,
                    "movl 4(%esp),%ecx\n\t"     /* old */
                    "movl %edi,0x9c(%ecx)\n\t"  /* old->Edi */
                    "movl %esi,0xa0(%ecx)\n\t"  /* old->Esi */
                    "movl %ebx,0xa4(%ecx)\n\t"  /* old->Ebx */
                    "movl %ebp,0xb4(%ecx)\n\t"  /* old->Ebp */
                    "movl 0(%esp),%eax\n\t"
                    "movl %eax,0xb8(%ecx)\n\t"  /* old->Eip */
                    "leal 12(%esp),%eax\n\t"
                    "movl %eax,0xc4(%ecx)\n\t"  /* old->Esp */
                    "movl 8(%esp),%ecx\n\t"     /* new */
                    "movl 0x9c(%ecx),%edi\n\t"  /* new->Edi */
                    "movl 0xa0(%ecx),%esi\n\t"  /* new->Esi */
                    "movl 0xa4(%ecx),%ebx\n\t"  /* new->Ebx */
                    "movl 0xb4(%ecx),%ebp\n\t"  /* new->Ebp */
                    "movl 0xc4(%ecx),%esp\n\t"  /* new->Esp */
                    "jmp *0xb8(%ecx)" )         /* new->Eip */
#elif defined(__x86_64__)
__ASM_STDCALL_FUNC( switch_fiber, 8,
                    "movq %rbx,0x90(%rcx)\n\t"       /* old->Rbx */
                    "leaq 0x8(%rsp),%rax\n\t"
                    "movq %rax,0x98(%rcx)\n\t"       /* old->Rsp */
                    "movq %rbp,0xa0(%rcx)\n\t"       /* old->Rbp */
                    "movq %rsi,0xa8(%rcx)\n\t"       /* old->Rsi */
                    "movq %rdi,0xb0(%rcx)\n\t"       /* old->Rdi */
                    "movq %r12,0xd8(%rcx)\n\t"       /* old->R12 */
                    "movq %r13,0xe0(%rcx)\n\t"       /* old->R13 */
                    "movq %r14,0xe8(%rcx)\n\t"       /* old->R14 */
                    "movq %r15,0xf0(%rcx)\n\t"       /* old->R15 */
                    "movq (%rsp),%rax\n\t"
                    "movq %rax,0xf8(%rcx)\n\t"       /* old->Rip */
                    "movdqa %xmm6,0x200(%rcx)\n\t"   /* old->Xmm6 */
                    "movdqa %xmm7,0x210(%rcx)\n\t"   /* old->Xmm7 */
                    "movdqa %xmm8,0x220(%rcx)\n\t"   /* old->Xmm8 */
                    "movdqa %xmm9,0x230(%rcx)\n\t"   /* old->Xmm9 */
                    "movdqa %xmm10,0x240(%rcx)\n\t"  /* old->Xmm10 */
                    "movdqa %xmm11,0x250(%rcx)\n\t"  /* old->Xmm11 */
                    "movdqa %xmm12,0x260(%rcx)\n\t"  /* old->Xmm12 */
                    "movdqa %xmm13,0x270(%rcx)\n\t"  /* old->Xmm13 */
                    "movdqa %xmm14,0x280(%rcx)\n\t"  /* old->Xmm14 */
                    "movdqa %xmm15,0x290(%rcx)\n\t"  /* old->Xmm15 */
                    "movq 0x90(%rdx),%rbx\n\t"       /* new->Rbx */
                    "movq 0xa0(%rdx),%rbp\n\t"       /* new->Rbp */
                    "movq 0xa8(%rdx),%rsi\n\t"       /* new->Rsi */
                    "movq 0xb0(%rdx),%rdi\n\t"       /* new->Rdi */
                    "movq 0xd8(%rdx),%r12\n\t"       /* new->R12 */
                    "movq 0xe0(%rdx),%r13\n\t"       /* new->R13 */
                    "movq 0xe8(%rdx),%r14\n\t"       /* new->R14 */
                    "movq 0xf0(%rdx),%r15\n\t"       /* new->R15 */
                    "movdqa 0x200(%rdx),%xmm6\n\t"   /* new->Xmm6 */
                    "movdqa 0x210(%rdx),%xmm7\n\t"   /* new->Xmm7 */
                    "movdqa 0x220(%rdx),%xmm8\n\t"   /* new->Xmm8 */
                    "movdqa 0x230(%rdx),%xmm9\n\t"   /* new->Xmm9 */
                    "movdqa 0x240(%rdx),%xmm10\n\t"  /* new->Xmm10 */
                    "movdqa 0x250(%rdx),%xmm11\n\t"  /* new->Xmm11 */
                    "movdqa 0x260(%rdx),%xmm12\n\t"  /* new->Xmm12 */
                    "movdqa 0x270(%rdx),%xmm13\n\t"  /* new->Xmm13 */
                    "movdqa 0x280(%rdx),%xmm14\n\t"  /* new->Xmm14 */
                    "movdqa 0x290(%rdx),%xmm15\n\t"  /* new->Xmm15 */
                    "movq 0x98(%rdx),%rsp\n\t"       /* new->Rsp */
                    "jmp *0xf8(%rdx)" )              /* new->Rip */
#elif defined(__arm__)
__ASM_STDCALL_FUNC( switch_fiber, 8,
                   "str r4, [r0, #0x14]\n\t"   /* old->R4 */
                   "str r5, [r0, #0x18]\n\t"   /* old->R5 */
                   "str r6, [r0, #0x1c]\n\t"   /* old->R6 */
                   "str r7, [r0, #0x20]\n\t"   /* old->R7 */
                   "str r8, [r0, #0x24]\n\t"   /* old->R8 */
                   "str r9, [r0, #0x28]\n\t"   /* old->R9 */
                   "str r10, [r0, #0x2c]\n\t"  /* old->R10 */
                   "str r11, [r0, #0x30]\n\t"  /* old->R11 */
                   "str sp, [r0, #0x38]\n\t"   /* old->Sp */
                   "str lr, [r0, #0x40]\n\t"   /* old->Pc */
                   "ldr r4, [r1, #0x14]\n\t"   /* new->R4 */
                   "ldr r5, [r1, #0x18]\n\t"   /* new->R5 */
                   "ldr r6, [r1, #0x1c]\n\t"   /* new->R6 */
                   "ldr r7, [r1, #0x20]\n\t"   /* new->R7 */
                   "ldr r8, [r1, #0x24]\n\t"   /* new->R8 */
                   "ldr r9, [r1, #0x28]\n\t"   /* new->R9 */
                   "ldr r10, [r1, #0x2c]\n\t"  /* new->R10 */
                   "ldr r11, [r1, #0x30]\n\t"  /* new->R11 */
                   "ldr sp, [r1, #0x38]\n\t"   /* new->Sp */
                   "ldr r2, [r1, #0x40]\n\t"   /* new->Pc */
                   "bx r2" )
#elif defined(__aarch64__)
__ASM_STDCALL_FUNC( switch_fiber, 8,
                   "stp x19, x20, [x0, #0xa0]\n\t"  /* old->X19,X20 */
                   "stp x21, x22, [x0, #0xb0]\n\t"  /* old->X21,X22 */
                   "stp x23, x24, [x0, #0xc0]\n\t"  /* old->X23,X24 */
                   "stp x25, x26, [x0, #0xd0]\n\t"  /* old->X25,X26 */
                   "stp x27, x28, [x0, #0xe0]\n\t"  /* old->X27,X28 */
                   "str x29, [x0, #0xf0]\n\t"       /* old->Fp */
                   "mov x2, sp\n\t"
                   "str x2, [x0, #0x100]\n\t"       /* old->Sp */
                   "str x30, [x0, #0x108]\n\t"      /* old->Pc */
                   "ldp x19, x20, [x1, #0xa0]\n\t"  /* new->X19,X20 */
                   "ldp x21, x22, [x1, #0xb0]\n\t"  /* new->X21,X22 */
                   "ldp x23, x24, [x1, #0xc0]\n\t"  /* new->X23,X24 */
                   "ldp x25, x26, [x1, #0xd0]\n\t"  /* new->X25,X26 */
                   "ldp x27, x28, [x1, #0xe0]\n\t"  /* new->X27,X28 */
                   "ldr x29, [x1, #0xf0]\n\t"       /* new->Fp */
                   "ldr x2, [x1, #0x100]\n\t"       /* new->Sp */
                   "ldr x30, [x1, #0x108]\n\t"      /* new->Pc */
                   "mov sp, x2\n\t"
                   "ret" )
#else
void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new )
{
    DbgBreakPoint();
}
#endif

/* call the fiber initial function once we have switched stack */
static void CDECL start_fiber(void)
{
    struct fiber_data *fiber = NtCurrentTeb()->Tib.u.FiberData;
    LPFIBER_START_ROUTINE start = fiber->start;

    __TRY
    {
        start( fiber->param );
        ExitThread( 1 );
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}

static void init_fiber_context( struct fiber_data *fiber )
{
#ifdef __i386__
    fiber->context.Esp = (ULONG_PTR)fiber->stack_base - 4;
    fiber->context.Eip = (ULONG_PTR)start_fiber;
#elif defined(__x86_64__)
    fiber->context.Rsp = (ULONG_PTR)fiber->stack_base - 0x28;
    fiber->context.Rip = (ULONG_PTR)start_fiber;
#elif defined(__arm__)
    fiber->context.Sp = (ULONG_PTR)fiber->stack_base;
    fiber->context.Pc = (ULONG_PTR)start_fiber;
#elif defined(__aarch64__)
    fiber->context.Sp = (ULONG_PTR)fiber->stack_base;
    fiber->context.Pc = (ULONG_PTR)start_fiber;
#endif
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

    if (!(fiber = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*fiber) )))
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
    init_fiber_context( fiber );
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
    NtCurrentTeb()->Tib.u.FiberData   = new_fiber;
    NtCurrentTeb()->Tib.ExceptionList = new_fiber->except;
    NtCurrentTeb()->Tib.StackBase     = new_fiber->stack_base;
    NtCurrentTeb()->Tib.StackLimit    = new_fiber->stack_limit;
    NtCurrentTeb()->DeallocationStack = new_fiber->stack_allocation;
    NtCurrentTeb()->FlsSlots          = new_fiber->fls_slots;
    switch_fiber( &current_fiber->context, &new_fiber->context );
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
