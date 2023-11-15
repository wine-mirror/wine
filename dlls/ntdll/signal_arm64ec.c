/*
 * ARM64EC signal handling routines
 *
 * Copyright 1999, 2005, 2023 Alexandre Julliard
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

#ifdef __arm64ec__

#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(unwind);
WINE_DECLARE_DEBUG_CHANNEL(relay);


static UINT64 mxcsr_to_fpcsr( UINT mxcsr )
{
    UINT fpcr = 0, fpsr = 0;

    if (mxcsr & 0x0001) fpsr |= 0x0001;    /* invalid operation */
    if (mxcsr & 0x0002) fpsr |= 0x0080;    /* denormal */
    if (mxcsr & 0x0004) fpsr |= 0x0002;    /* zero-divide */
    if (mxcsr & 0x0008) fpsr |= 0x0004;    /* overflow */
    if (mxcsr & 0x0010) fpsr |= 0x0008;    /* underflow */
    if (mxcsr & 0x0020) fpsr |= 0x0010;    /* precision */

    if (mxcsr & 0x0040) fpcr |= 0x0001;    /* denormals are zero */
    if (mxcsr & 0x0080) fpcr |= 0x0100;    /* invalid operation mask */
    if (mxcsr & 0x0100) fpcr |= 0x8000;    /* denormal mask */
    if (mxcsr & 0x0200) fpcr |= 0x0200;    /* zero-divide mask */
    if (mxcsr & 0x0400) fpcr |= 0x0400;    /* overflow mask */
    if (mxcsr & 0x0800) fpcr |= 0x0800;    /* underflow mask */
    if (mxcsr & 0x1000) fpcr |= 0x1000;    /* precision mask */
    if (mxcsr & 0x2000) fpcr |= 0x800000;  /* round down */
    if (mxcsr & 0x4000) fpcr |= 0x400000;  /* round up */
    if (mxcsr & 0x8000) fpcr |= 0x1000000; /* flush to zero */
    return fpcr | ((UINT64)fpsr << 32);
}

static UINT fpcsr_to_mxcsr( UINT fpcr, UINT fpsr )
{
    UINT ret = 0;

    if (fpsr & 0x0001) ret |= 0x0001;      /* invalid operation */
    if (fpsr & 0x0002) ret |= 0x0004;      /* zero-divide */
    if (fpsr & 0x0004) ret |= 0x0008;      /* overflow */
    if (fpsr & 0x0008) ret |= 0x0010;      /* underflow */
    if (fpsr & 0x0010) ret |= 0x0020;      /* precision */
    if (fpsr & 0x0080) ret |= 0x0002;      /* denormal */

    if (fpcr & 0x0000001) ret |= 0x0040;   /* denormals are zero */
    if (fpcr & 0x0000100) ret |= 0x0080;   /* invalid operation mask */
    if (fpcr & 0x0000200) ret |= 0x0200;   /* zero-divide mask */
    if (fpcr & 0x0000400) ret |= 0x0400;   /* overflow mask */
    if (fpcr & 0x0000800) ret |= 0x0800;   /* underflow mask */
    if (fpcr & 0x0001000) ret |= 0x1000;   /* precision mask */
    if (fpcr & 0x0008000) ret |= 0x0100;   /* denormal mask */
    if (fpcr & 0x0400000) ret |= 0x4000;   /* round up */
    if (fpcr & 0x0800000) ret |= 0x2000;   /* round down */
    if (fpcr & 0x1000000) ret |= 0x8000;   /* flush to zero */
    return ret;
}


/***********************************************************************
 *           LdrpGetX64Information
 */
static NTSTATUS WINAPI LdrpGetX64Information( ULONG type, void *output, void *extra_info )
{
    switch (type)
    {
    case 0:
    {
        UINT64 fpcr, fpsr;

        __asm__ __volatile__( "mrs %0, fpcr; mrs %1, fpsr" : "=r" (fpcr), "=r" (fpsr) );
        *(UINT *)output = fpcsr_to_mxcsr( fpcr, fpsr );
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "not implemented type %u\n", type );
        return STATUS_INVALID_PARAMETER;
    }
}

/***********************************************************************
 *           LdrpSetX64Information
 */
static NTSTATUS WINAPI LdrpSetX64Information( ULONG type, ULONG_PTR input, void *extra_info )
{
    switch (type)
    {
    case 0:
    {
        UINT64 fpcsr = mxcsr_to_fpcsr( input );
        __asm__ __volatile__( "msr fpcr, %0; msr fpsr, %1" :: "r" (fpcsr), "r" (fpcsr >> 32) );
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "not implemented type %u\n", type );
        return STATUS_INVALID_PARAMETER;
    }
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
NTSTATUS WINAPI KiUserExceptionDispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    FIXME( "not implemented\n" );
    return STATUS_INVALID_DISPOSITION;
}


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
void WINAPI KiUserApcDispatcher( CONTEXT *context, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                 PNTAPCFUNC apc )
{
    FIXME( "not implemented\n" );
}


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
void WINAPI KiUserCallbackDispatcher( ULONG id, void *args, ULONG len )
{
    FIXME( "not implemented\n" );
}


/**************************************************************************
 *              RtlIsEcCode (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsEcCode( const void *ptr )
{
    const UINT64 *map = (const UINT64 *)NtCurrentTeb()->Peb->EcCodeBitMap;
    ULONG_PTR page = (ULONG_PTR)ptr / page_size;
    return (map[page / 64] >> (page & 63)) & 1;
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
void WINAPI RtlCaptureContext( CONTEXT *context )
{
    FIXME( "not implemented\n" );
}


/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    FIXME( "not implemented\n" );
}


/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind( ULONG type, ULONG64 base, ULONG64 pc,
                               RUNTIME_FUNCTION *function, CONTEXT *context,
                               PVOID *data, ULONG64 *frame_ret,
                               KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    FIXME( "not implemented\n" );
    return NULL;
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    FIXME( "not implemented\n" );
}


/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    FIXME( "not implemented\n" );
}


/*******************************************************************
 *		_local_unwind (NTDLL.@)
 */
void WINAPI _local_unwind( void *frame, void *target_ip )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, NULL, NULL, &context, NULL );
}


/*******************************************************************
 *		__C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec,
                                                   void *frame,
                                                   CONTEXT *context,
                                                   struct _DISPATCHER_CONTEXT *dispatch )
{
    FIXME( "not implemented\n" );
    return ExceptionContinueSearch;
}


/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "not implemented\n" );
    return 0;
}


static int code_match( BYTE *code, const BYTE *seq, size_t len )
{
    for ( ; len; len--, code++, seq++) if (*seq && *code != *seq) return 0;
    return 1;
}

void *check_call( void **target, void *exit_thunk, void *dest )
{
    static const BYTE jmp_sequence[] =
    {
        0xff, 0x25              /* jmp *xxx(%rip) */
    };
    static const BYTE fast_forward_sequence[] =
    {
        0x48, 0x8b, 0xc4,       /* mov  %rsp,%rax */
        0x48, 0x89, 0x58, 0x20, /* mov  %rbx,0x20(%rax) */
        0x55,                   /* push %rbp */
        0x5d,                   /* pop  %rbp */
        0xe9                    /* jmp  arm_code */
    };
    static const BYTE syscall_sequence[] =
    {
        0x4c, 0x8b, 0xd1,       /* mov  %rcx,%r10 */
        0xb8, 0, 0, 0, 0,       /* mov  $xxx,%eax */
        0xf6, 0x04, 0x25, 0x08, /* testb $0x1,0x7ffe0308 */
        0x03, 0xfe, 0x7f, 0x01,
        0x75, 0x03,             /* jne 1f */
        0x0f, 0x05,             /* syscall */
        0xc3,                   /* ret */
        0xcd, 0x2e,             /* 1: int $0x2e */
        0xc3                    /* ret */
    };

    for (;;)
    {
        if (dest == __wine_unix_call_dispatcher) return dest;
        if (RtlIsEcCode( dest )) return dest;
        if (code_match( dest, jmp_sequence, sizeof(jmp_sequence) ))
        {
            int *off_ptr = (int *)((char *)dest + sizeof(jmp_sequence));
            void **addr_ptr = (void **)((char *)(off_ptr + 1) + *off_ptr);
            dest = *addr_ptr;
            continue;
        }
        if (!((ULONG_PTR)dest & 15))  /* fast-forward and syscall thunks are always aligned */
        {
            if (code_match( dest, fast_forward_sequence, sizeof(fast_forward_sequence) ))
            {
                int *off_ptr = (int *)((char *)dest + sizeof(fast_forward_sequence));
                return (char *)(off_ptr + 1) + *off_ptr;
            }
            if (code_match( dest, syscall_sequence, sizeof(syscall_sequence) ))
            {
                ULONG id = ((ULONG *)dest)[1];
                FIXME( "syscall %x at %p not implemented\n", id, dest );
            }
        }
        *target = dest;
        return exit_thunk;
    }
}

static void __attribute__((naked)) arm64x_check_call(void)
{
    asm( "stp x29, x30, [sp,#-0xb0]!\n\t"
         "mov x29, sp\n\t"
         "stp x0, x1,   [sp, #0x10]\n\t"
         "stp x2, x3,   [sp, #0x20]\n\t"
         "stp x4, x5,   [sp, #0x30]\n\t"
         "stp x6, x7,   [sp, #0x40]\n\t"
         "stp x8, x9,   [sp, #0x50]\n\t"
         "stp x10, x15, [sp, #0x60]\n\t"
         "stp d0, d1,   [sp, #0x70]\n\t"
         "stp d2, d3,   [sp, #0x80]\n\t"
         "stp d4, d5,   [sp, #0x90]\n\t"
         "stp d6, d7,   [sp, #0xa0]\n\t"
         "add x0, sp, #0x58\n\t"  /* x9 = &target */
         "mov x1, x10\n\t"        /* x10 = exit_thunk */
         "mov x2, x11\n\t"        /* x11 = dest */
         "bl " __ASM_NAME("check_call") "\n\t"
         "mov x11, x0\n\t"
         "ldp x0, x1,   [sp, #0x10]\n\t"
         "ldp x2, x3,   [sp, #0x20]\n\t"
         "ldp x4, x5,   [sp, #0x30]\n\t"
         "ldp x6, x7,   [sp, #0x40]\n\t"
         "ldp x8, x9,   [sp, #0x50]\n\t"
         "ldp x10, x15, [sp, #0x60]\n\t"
         "ldp d0, d1,   [sp, #0x70]\n\t"
         "ldp d2, d3,   [sp, #0x80]\n\t"
         "ldp d4, d5,   [sp, #0x90]\n\t"
         "ldp d6, d7,   [sp, #0xa0]\n\t"
         "ldp x29, x30, [sp], #0xb0\n\t"
         "ret" );
}


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
void __attribute__((naked)) __chkstk(void)
{
    asm( "ret" );
}


/**************************************************************************
 *		__chkstk_arm64ec (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
void __attribute__((naked)) __chkstk_arm64ec(void)
{
    asm( "ret" );
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI RtlRaiseException( struct _EXCEPTION_RECORD * rec)
{
    FIXME( "not implemented\n" );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
}


/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    if (!__os_arm64x_check_call)
    {
        __os_arm64x_check_call = arm64x_check_call;
        __os_arm64x_check_icall = arm64x_check_call;
        __os_arm64x_check_icall_cfg = arm64x_check_call;
        __os_arm64x_get_x64_information = LdrpGetX64Information;
        __os_arm64x_set_x64_information = LdrpSetX64Information;
    }

    loader_init( context, (void **)&context->Rcx );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->Rcx, (void *)context->Rdx );
    NtContinue( context, TRUE );
}


/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
void __attribute__((naked)) DbgBreakPoint(void)
{
    asm( "brk #0xf000; ret" );
}


/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
void __attribute__((naked)) DbgUserBreakPoint(void)
{
    asm( "brk #0xf000; ret" );
}

#endif  /* __arm64ec__ */
