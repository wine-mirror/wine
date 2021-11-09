/*
 * ARM signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
 * Copyright 2010-2013, 2015 André Hentschel
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

#ifdef __arm__

#include <stdlib.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/*******************************************************************
 *         is_valid_frame
 */
static inline BOOL is_valid_frame( void *frame )
{
    if ((ULONG_PTR)frame & 3) return FALSE;
    return (frame >= NtCurrentTeb()->Tib.StackLimit &&
            (void **)frame < (void **)NtCurrentTeb()->Tib.StackBase - 1);
}


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Incoming r4 contains words to allocate, converting to bytes then return
 */
__ASM_GLOBAL_FUNC( __chkstk, "lsl r4, r4, #2\n\t"
                             "bx lr" )

/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlCaptureContext, 4,
                    "str r1, [r0, #0x8]\n\t"   /* context->R1 */
                    "mov r1, #0x0200000\n\t"   /* CONTEXT_ARM */
                    "add r1, r1, #0x7\n\t"     /* CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_FLOATING_POINT */
                    "str r1, [r0]\n\t"         /* context->ContextFlags */
                    "str SP, [r0, #0x38]\n\t"  /* context->Sp */
                    "str LR, [r0, #0x40]\n\t"  /* context->Pc */
                    "mrs r1, CPSR\n\t"
                    "bfi r1, lr, #5, #1\n\t"   /* Thumb bit */
                    "str r1, [r0, #0x44]\n\t"  /* context->Cpsr */
                    "mov r1, #0\n\t"
                    "str r1, [r0, #0x4]\n\t"   /* context->R0 */
                    "str r1, [r0, #0x3c]\n\t"  /* context->Lr */
                    "add r0, #0x0c\n\t"
                    "stm r0, {r2-r12}\n\t"     /* context->R2..R12 */
                    "bx lr" )


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch, *nested_frame;
    DWORD res;

    frame = NtCurrentTeb()->Tib.ExceptionList;
    nested_frame = NULL;
    while (frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL)
    {
        /* Check frame address */
        if (!is_valid_frame( frame ))
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, rec->ExceptionCode, rec->ExceptionFlags );
        res = frame->Handler( rec, frame, context, &dispatch );
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return STATUS_SUCCESS;
            return STATUS_NONCONTINUABLE_EXCEPTION;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            return STATUS_INVALID_DISPOSITION;
        }
        frame = frame->Prev;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
NTSTATUS WINAPI KiUserExceptionDispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;
    DWORD c;

    TRACE( "code=%x flags=%x addr=%p pc=%08x tid=%04x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           context->Pc, GetCurrentThreadId() );
    for (c = 0; c < rec->NumberParameters; c++)
        TRACE( " info[%d]=%08lx\n", c, rec->ExceptionInformation[c] );

    if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
    {
        if (rec->ExceptionInformation[1] >> 16)
            MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
        else
            MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
    }
    else if (rec->ExceptionCode == EXCEPTION_WINE_NAME_THREAD && rec->ExceptionInformation[0] == 0x1000)
    {
        WARN( "Thread %04x renamed to %s\n", (DWORD)rec->ExceptionInformation[2], debugstr_a((char *)rec->ExceptionInformation[1]) );
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_C)
    {
        WARN( "%s\n", debugstr_an((char *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C)
    {
        WARN( "%s\n", debugstr_wn((WCHAR *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else
    {
        if (rec->ExceptionCode == STATUS_ASSERTION_FAILURE)
            ERR( "%s exception (code=%x) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );
        else
            WARN( "%s exception (code=%x) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );

        TRACE( " r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x\n",
               context->R0, context->R1, context->R2, context->R3, context->R4, context->R5 );
        TRACE( " r6=%08x r7=%08x r8=%08x r9=%08x r10=%08x r11=%08x\n",
               context->R6, context->R7, context->R8, context->R9, context->R10, context->R11 );
        TRACE( " r12=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
               context->R12, context->Sp, context->Lr, context->Pc, context->Cpsr );
    }

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
        NtContinue( context, FALSE );

    if ((status = call_stack_handlers( rec, context )) == STATUS_SUCCESS)
        NtContinue( context, FALSE );

    if (status != STATUS_UNHANDLED_EXCEPTION) RtlRaiseStatus( status );
    return NtRaiseException( rec, context, FALSE );
}


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
void WINAPI KiUserApcDispatcher( CONTEXT *context, ULONG_PTR ctx, ULONG_PTR arg1, ULONG_PTR arg2,
                                 PNTAPCFUNC func )
{
    func( ctx, arg1, arg2 );
    NtContinue( context, TRUE );
}


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
void WINAPI KiUserCallbackDispatcher( ULONG id, void *args, ULONG len )
{
    NTSTATUS (WINAPI *func)(void *, ULONG) = ((void **)NtCurrentTeb()->Peb->KernelCallbackTable)[id];

    RtlRaiseStatus( NtCallbackReturn( NULL, 0, func( args, len )));
}


/***********************************************************************
 * Definitions for Win32 unwind tables
 */

struct unwind_info
{
    DWORD function_length : 18;
    DWORD version : 2;
    DWORD x : 1;
    DWORD e : 1;
    DWORD f : 1;
    DWORD epilog : 5;
    DWORD codes : 4;
};

struct unwind_info_ext
{
    WORD epilog;
    BYTE codes;
    BYTE reserved;
};

struct unwind_info_epilog
{
    DWORD offset : 18;
    DWORD res : 2;
    DWORD cond : 4;
    DWORD index : 8;
};

static const BYTE unwind_code_len[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* a0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* c0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* e0 */ 1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,4,3,4,1,1,1,1,1
};

static const BYTE unwind_instr_len[256] =
{
/* 00 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 20 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 40 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 60 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 80 */ 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
/* a0 */ 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
/* c0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,
/* e0 */ 4,4,4,4,4,4,4,4,4,4,4,4,2,2,2,4,0,0,0,0,0,4,4,2,2,4,4,2,4,2,4,0
};

/***********************************************************************
 *           get_sequence_len
 */
static unsigned int get_sequence_len( BYTE *ptr, BYTE *end, int include_end )
{
    unsigned int ret = 0;

    while (ptr < end)
    {
        if (*ptr >= 0xfd)
        {
            if (*ptr <= 0xfe && include_end)
                ret += unwind_instr_len[*ptr];
            break;
        }
        ret += unwind_instr_len[*ptr];
        ptr += unwind_code_len[*ptr];
    }
    return ret;
}


/***********************************************************************
 *           pop_regs_mask
 */
static void pop_regs_mask( int mask, CONTEXT *context,
                           KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    for (i = 0; i <= 12; i++)
    {
        if (!(mask & (1 << i))) continue;
        if (ptrs && i >= 4 && i <= 11) (&ptrs->R4)[i - 4] = (DWORD *)context->Sp;
        if (i >= 4) (&context->R0)[i] = *(DWORD *)context->Sp;
        context->Sp += 4;
    }
}


/***********************************************************************
 *           pop_regs_range
 */
static void pop_regs_range( int last, CONTEXT *context,
                            KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    for (i = 4; i <= last; i++)
    {
        if (ptrs) (&ptrs->R4)[i - 4] = (DWORD *)context->Sp;
        (&context->R0)[i] = *(DWORD *)context->Sp;
        context->Sp += 4;
    }
}


/***********************************************************************
 *           pop_lr
 */
static void pop_lr( int increment, CONTEXT *context,
                    KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    if (ptrs) ptrs->Lr = (DWORD *)context->Sp;
    context->Lr = *(DWORD *)context->Sp;
    context->Sp += increment;
}


/***********************************************************************
 *           pop_fpregs_range
 */
static void pop_fpregs_range( int first, int last, CONTEXT *context,
                              KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    for (i = first; i <= last; i++)
    {
        if (ptrs && i >= 8 && i <= 15) (&ptrs->D8)[i - 8] = (ULONGLONG *)context->Sp;
        context->u.D[i] = *(ULONGLONG *)context->Sp;
        context->Sp += 8;
    }
}


/***********************************************************************
 *           process_unwind_codes
 */
static void process_unwind_codes( BYTE *ptr, BYTE *end, CONTEXT *context,
                                  KNONVOLATILE_CONTEXT_POINTERS *ptrs, int skip )
{
    unsigned int val, len;
    unsigned int i;

    /* skip codes */
    while (ptr < end && skip)
    {
        if (*ptr >= 0xfd) break;
        skip -= unwind_instr_len[*ptr];
        ptr += unwind_code_len[*ptr];
    }

    while (ptr < end)
    {
        len = unwind_code_len[*ptr];
        if (ptr + len > end) break;
        val = 0;
        for (i = 0; i < len; i++)
            val = (val << 8) | ptr[i];

        if (*ptr <= 0x7f)      /* add sp, sp, #x */
            context->Sp += 4 * (val & 0x7f);
        else if (*ptr <= 0xbf) /* pop {r0-r12,lr} */
        {
            pop_regs_mask( val & 0x1fff, context, ptrs );
            if (val & 0x2000)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xcf) /* mov sp, rX */
            context->Sp = (&context->R0)[val & 0x0f];
        else if (*ptr <= 0xd7) /* pop {r4-rX,lr} */
        {
            pop_regs_range( (val & 0x03) + 4, context, ptrs );
            if (val & 0x04)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xdf) /* pop {r4-rX,lr} */
        {
            pop_regs_range( (val & 0x03) + 8, context, ptrs );
            if (val & 0x04)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xe7) /* vpop {d8-dX} */
            pop_fpregs_range( 8, (val & 0x07) + 8, context, ptrs );
        else if (*ptr <= 0xeb) /* add sp, sp, #x */
            context->Sp += 4 * (val & 0x3ff);
        else if (*ptr <= 0xed) /* pop {r0-r12,lr} */
        {
            pop_regs_mask( val & 0xff, context, ptrs );
            if (val & 0x100)
                pop_lr( 4, context, ptrs );
        }
        else if (*ptr <= 0xee) /* Microsoft-specific 0x00-0x0f, Available 0x10-0xff */
            WARN( "unsupported code %02x\n", *ptr );
        else if (*ptr <= 0xef && ((val & 0xff) <= 0x0f)) /* ldr lr, [sp], #x */
            pop_lr( 4 * (val & 0x0f), context, ptrs );
        else if (*ptr <= 0xf4) /* Available */
            WARN( "unsupported code %02x\n", *ptr );
        else if (*ptr <= 0xf5) /* vpop {dS-dE} */
            pop_fpregs_range( (val & 0xf0) >> 4, (val & 0x0f), context, ptrs );
        else if (*ptr <= 0xf6) /* vpop {dS-dE} */
            pop_fpregs_range( ((val & 0xf0) >> 4) + 16, (val & 0x0f) + 16, context, ptrs );
        else if (*ptr == 0xf7 || *ptr == 0xf9) /* add sp, sp, #x */
            context->Sp += 4 * (val & 0xffff);
        else if (*ptr == 0xf8 || *ptr == 0xfa) /* add sp, sp, #x */
            context->Sp += 4 * (val & 0xffffff);
        else if (*ptr <= 0xfc)  /* nop */
            /* nop */ ;
        else                    /* end */
            break;

        ptr += len;
    }
}


/***********************************************************************
 *           unwind_packed_data
 */
static void *unwind_packed_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                                 CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i, pos = 0;
    int pf = 0, ef = 0, fpoffset = 0, stack = func->u.s.StackAdjust;
    int prologue_regmask = 0;
    int epilogue_regmask = 0;
    unsigned int offset, len;
    BYTE prologue[10], *prologue_end, epilogue[20], *epilogue_end;

    TRACE( "function %lx-%lx: len=%#x flag=%x ret=%u H=%u reg=%u R=%u L=%u C=%u stackadjust=%x\n",
           base + func->BeginAddress, base + func->BeginAddress + func->u.s.FunctionLength * 2,
           func->u.s.FunctionLength, func->u.s.Flag, func->u.s.Ret,
           func->u.s.H, func->u.s.Reg, func->u.s.R, func->u.s.L, func->u.s.C, func->u.s.StackAdjust );

    offset = (pc - base) - func->BeginAddress;
    if (func->u.s.StackAdjust >= 0x03f4)
    {
        pf = func->u.s.StackAdjust & 0x04;
        ef = func->u.s.StackAdjust & 0x08;
        stack = (func->u.s.StackAdjust & 3) + 1;
    }

    if (!func->u.s.R || pf)
    {
        int first = 4, last = func->u.s.Reg + 4;
        if (pf)
        {
            first = (~func->u.s.StackAdjust) & 3;
            if (func->u.s.R)
                last = 3;
        }
        for (i = first; i <= last; i++)
            prologue_regmask |= 1 << i;
        fpoffset = last + 1 - first;
    }

    if (!func->u.s.R || ef)
    {
        int first = 4, last = func->u.s.Reg + 4;
        if (ef)
        {
            first = (~func->u.s.StackAdjust) & 3;
            if (func->u.s.R)
                last = 3;
        }
        for (i = first; i <= last; i++)
            epilogue_regmask |= 1 << i;
    }

    if (func->u.s.C)
    {
        prologue_regmask |= 1 << 11;
        epilogue_regmask |= 1 << 11;
    }

    if (func->u.s.L)
    {
        prologue_regmask |= 1 << 14; /* lr */
        if (func->u.s.Ret != 0)
            epilogue_regmask |= 1 << 14; /* lr */
        else if (!func->u.s.H)
            epilogue_regmask |= 1 << 15; /* pc */
    }

    /* Synthesize prologue opcodes */
    if (stack && !pf)
    {
        if (stack <= 0x7f)
        {
            prologue[pos++] = stack; /* sub sp, sp, #x */
        }
        else
        {
            prologue[pos++] = 0xe8 | (stack >> 8); /* sub.w sp, sp, #x */
            prologue[pos++] = stack & 0xff;
        }
    }

    if (func->u.s.R && func->u.s.Reg != 7)
        prologue[pos++] = 0xe0 | func->u.s.Reg; /* vpush {d8-dX} */

    if (func->u.s.C && fpoffset == 0)
        prologue[pos++] = 0xfb; /* mov r11, sp - handled as nop16 */
    else if (func->u.s.C)
        prologue[pos++] = 0xfc; /* add r11, sp, #x - handled as nop32 */

    if (prologue_regmask & 0xf00) /* r8-r11 set */
    {
        int bitmask = prologue_regmask & 0x1fff;
        if (prologue_regmask & (1 << 14)) /* lr */
            bitmask |= 0x2000;
        prologue[pos++] = 0x80 | (bitmask >> 8); /* push.w {r0-r12,lr} */
        prologue[pos++] = bitmask & 0xff;
    }
    else if (prologue_regmask) /* r0-r7, lr set */
    {
        int bitmask = prologue_regmask & 0xff;
        if (prologue_regmask & (1 << 14)) /* lr */
            bitmask |= 0x100;
        prologue[pos++] = 0xec | (bitmask >> 8); /* push {r0-r7,lr} */
        prologue[pos++] = bitmask & 0xff;
    }

    if (func->u.s.H)
        prologue[pos++] = 0x04; /* push {r0-r3} - handled as sub sp, sp, #16 */

    prologue[pos++] = 0xff; /* end */
    prologue_end = &prologue[pos];

    /* Synthesize epilogue opcodes */
    pos = 0;
    if (stack && !ef)
    {
        if (stack <= 0x7f)
        {
            epilogue[pos++] = stack; /* sub sp, sp, #x */
        }
        else
        {
            epilogue[pos++] = 0xe8 | (stack >> 8); /* sub.w sp, sp, #x */
            epilogue[pos++] = stack & 0xff;
        }
    }

    if (func->u.s.R && func->u.s.Reg != 7)
        epilogue[pos++] = 0xe0 | func->u.s.Reg; /* vpush {d8-dX} */

    if (epilogue_regmask & 0x7f00) /* r8-r11, lr set */
    {
        int bitmask = epilogue_regmask & 0x1fff;
        if (epilogue_regmask & (3 << 14)) /* lr or pc */
            bitmask |= 0x2000;
        epilogue[pos++] = 0x80 | (bitmask >> 8); /* push.w {r0-r12,lr} */
        epilogue[pos++] = bitmask & 0xff;
    }
    else if (epilogue_regmask) /* r0-r7, pc set */
    {
        int bitmask = epilogue_regmask & 0xff;
        if (epilogue_regmask & (1 << 15)) /* pc */
            bitmask |= 0x100; /* lr */
        epilogue[pos++] = 0xec | (bitmask >> 8); /* push {r0-r7,lr} */
        epilogue[pos++] = bitmask & 0xff;
    }

    if (func->u.s.H && !(func->u.s.L && func->u.s.Ret == 0))
        epilogue[pos++] = 0x04; /* add sp, sp, #16 */
    else if (func->u.s.H && (func->u.s.L && func->u.s.Ret == 0))
    {
        epilogue[pos++] = 0xef; /* ldr lr, [sp], #20 */
        epilogue[pos++] = 5;
    }

    if (func->u.s.Ret == 1)
        epilogue[pos++] = 0xfd; /* bx lr */
    else if (func->u.s.Ret == 2)
        epilogue[pos++] = 0xfe; /* b address */
    else
        epilogue[pos++] = 0xff; /* end */
    epilogue_end = &epilogue[pos];

    if (func->u.s.Flag == 1 && offset < 4 * (prologue_end - prologue)) {
        /* Check prologue */
        len = get_sequence_len( prologue, prologue_end, 0 );
        if (offset < len)
        {
            process_unwind_codes( prologue, prologue_end, context, ptrs, len - offset );
            return NULL;
        }
    }

    if (func->u.s.Ret != 3 && 2 * func->u.s.FunctionLength - offset <= 4 * (epilogue_end - epilogue)) {
        /* Check epilogue */
        len = get_sequence_len( epilogue, epilogue_end, 1 );
        if (offset >= 2 * func->u.s.FunctionLength - len)
        {
            process_unwind_codes( epilogue, epilogue_end, context, ptrs, offset - (2 * func->u.s.FunctionLength - len) );
            return NULL;
        }
    }

    /* Execute full prologue */
    process_unwind_codes( prologue, prologue_end, context, ptrs, 0 );

    return NULL;
}


/***********************************************************************
 *           unwind_full_data
 */
static void *unwind_full_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                               CONTEXT *context, PVOID *handler_data, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    struct unwind_info *info;
    struct unwind_info_epilog *info_epilog;
    unsigned int i, codes, epilogs, len, offset;
    void *data;
    BYTE *end;

    info = (struct unwind_info *)((char *)base + func->u.UnwindData);
    data = info + 1;
    epilogs = info->epilog;
    codes = info->codes;
    if (!codes && !epilogs)
    {
        struct unwind_info_ext *infoex = data;
        codes = infoex->codes;
        epilogs = infoex->epilog;
        data = infoex + 1;
    }
    info_epilog = data;
    if (!info->e) data = info_epilog + epilogs;

    offset = (pc - base) - func->BeginAddress;
    end = (BYTE *)data + codes * 4;

    TRACE( "function %lx-%lx: len=%#x ver=%u X=%u E=%u F=%u epilogs=%u codes=%u\n",
           base + func->BeginAddress, base + func->BeginAddress + info->function_length * 4,
           info->function_length, info->version, info->x, info->e, info->f, epilogs, codes * 4 );

    /* check for prolog */
    if (offset < codes * 4 * 4 && !info->f)
    {
        len = get_sequence_len( data, end, 0 );
        if (offset < len)
        {
            process_unwind_codes( data, end, context, ptrs, len - offset );
            return NULL;
        }
    }

    /* check for epilog */
    if (!info->e)
    {
        for (i = 0; i < epilogs; i++)
        {
            /* TODO: Currently not checking epilogue conditions. */
            if (offset < 2 * info_epilog[i].offset) break;
            if (offset - 2 * info_epilog[i].offset < (codes * 4 - info_epilog[i].index) * 4)
            {
                BYTE *ptr = (BYTE *)data + info_epilog[i].index;
                len = get_sequence_len( ptr, end, 1 );
                if (offset <= 2 * info_epilog[i].offset + len)
                {
                    process_unwind_codes( ptr, end, context, ptrs, offset - 2 * info_epilog[i].offset );
                    return NULL;
                }
            }
        }
    }
    else if (2 * info->function_length - offset <= (codes * 4 - epilogs) * 4)
    {
        BYTE *ptr = (BYTE *)data + epilogs;
        len = get_sequence_len( ptr, end, 1 );
        if (offset >= 2 * info->function_length - len)
        {
            process_unwind_codes( ptr, end, context, ptrs, offset - (2 * info->function_length - len) );
            return NULL;
        }
    }

    process_unwind_codes( data, end, context, ptrs, 0 );

    /* get handler since we are inside the main code */
    if (info->x)
    {
        DWORD *handler_rva = (DWORD *)data + codes;
        *handler_data = handler_rva + 1;
        return (char *)base + *handler_rva;
    }
    return NULL;
}

/***********************************************************************
 *            RtlVirtualUnwind  (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                               RUNTIME_FUNCTION *func, CONTEXT *context,
                               PVOID *handler_data, ULONG_PTR *frame_ret,
                               KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    void *handler;

    TRACE( "type %x pc %lx sp %x func %lx\n", type, pc, context->Sp, base + func->BeginAddress );

    *handler_data = NULL;

    context->Pc = 0;
    if (func->u.s.Flag)
        handler = unwind_packed_data( base, pc, func, context, ctx_ptr );
    else
        handler = unwind_full_data( base, pc, func, context, handler_data, ctx_ptr );

    TRACE( "ret: lr=%x sp=%x handler=%p\n", context->Lr, context->Sp, handler );
    if (!context->Pc)
        context->Pc = context->Lr;
    context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
    *frame_ret = context->Sp;
    return handler;
}


/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( void *endframe, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    EXCEPTION_RECORD record;
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch;
    DWORD res;

    RtlCaptureContext( &context );
    context.R0 = (DWORD)retval;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context.Pc;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (endframe ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%x flags=%x\n", rec->ExceptionCode, rec->ExceptionFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->Tib.ExceptionList;
    while ((frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL) && (frame != endframe))
    {
        /* Check frame address */
        if (endframe && ((void*)frame > endframe))
            raise_status( STATUS_INVALID_UNWIND_TARGET, rec );

        if (!is_valid_frame( frame )) raise_status( STATUS_BAD_STACK, rec );

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, rec->ExceptionCode, rec->ExceptionFlags );
        res = frame->Handler(rec, frame, &context, &dispatch);
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        switch(res)
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            raise_status( STATUS_INVALID_DISPOSITION, rec );
            break;
        }
        frame = __wine_pop_frame( frame );
    }
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlRaiseException, 4,
                    "push {r0, lr}\n\t"
                    "sub sp, sp, #0x1a0\n\t"  /* sizeof(CONTEXT) */
                    "mov r0, sp\n\t"  /* context */
                    "bl " __ASM_NAME("RtlCaptureContext") "\n\t"
                    "ldr r0, [sp, #0x1a0]\n\t" /* rec */
                    "ldr r1, [sp, #0x1a4]\n\t"
                    "str r1, [sp, #0x40]\n\t"  /* context->Pc */
                    "mrs r2, CPSR\n\t"
                    "bfi r2, r1, #5, #1\n\t"   /* Thumb bit */
                    "str r2, [sp, #0x44]\n\t"  /* context->Cpsr */
                    "str r1, [r0, #12]\n\t"    /* rec->ExceptionAddress */
                    "add r1, sp, #0x1a8\n\t"
                    "str r1, [sp, #0x38]\n\t"  /* context->Sp */
                    "mov r1, sp\n\t"
                    "mov r2, #1\n\t"
                    "bl " __ASM_NAME("NtRaiseException") "\n\t"
                    "bl " __ASM_NAME("RtlRaiseStatus") )

/*************************************************************************
 *             RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%d, %d, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "mov sp, r0\n\t"  /* context */
                   "mov r1, #1\n\t"
                   "b " __ASM_NAME("NtContinue") )

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgBreakPoint, 0, "udf #0xfe; bx lr; nop; nop; nop; nop" );

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgUserBreakPoint, 0, "udf #0xfe; bx lr; nop; nop; nop; nop" );

#endif  /* __arm__ */
